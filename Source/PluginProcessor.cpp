// ============================================================================
// PluginProcessor.cpp — The MIDI engine
// ============================================================================
// The DAW calls processBlock repeatedly. Keep it fast: do not do filesystem work, logging, UI rendering, or blocking waits inside an audio callback. This file handles note scheduling, MIDI output, Quick Program state, and the program-change mailbox.
//
// Beginner notes:
//   - A header (.h) usually DECLARES names, types and helper functions.
//   - A source file (.cpp) usually IMPLEMENTS behaviour.
//   - `noexcept` promises an operation will not throw C++ exceptions.
//   - `const` generally indicates a value cannot be modified through that use.
//   - Avoid changing audio-thread behaviour merely to add documentation.
// See docs/SOURCE_GUIDE.md for terms, architecture and call-flow diagrams.
// ============================================================================

#include "PluginProcessor.h"
#include <chrono>
#include "PnordeProjectVersion.h"
#include "PnordeProgramPolling.h"
#include "PnordeStateLimits.h"
#include <string>
#include "PnordeSafeCC.h"
#include "PnordeSafeAutomation.h"
#include "PnordeVisualOrder.h"
#include "PnordeAftertouchGlide.h"
#include "PnordeTempoGlide.h"
#include "PnordeManualTempo.h"
#include "PnordeRetainedTransport.h"
#include "PnordeSwing.h"
#include "PnordeProgramLane.h"
#include "PnordeProgramRequests.h"
#include "PnordeRelativePitch.h"
#include "PnordeProbability.h"
#include "PluginEditor.h"

#include <cmath>
#include <memory>
#include <cstdint>
#include <atomic>
#include <limits>

// Fixed-capacity SPSC queue: processBlock never opens ports, locks, or sends to Windows.
// Short channel MIDI messages (notes, CC, pressure, pitch bend) only.
class PolyrhythmMIDIProcessor::ExternalMidiWorker : public juce::Thread
{
public:
    ExternalMidiWorker() : juce::Thread("Pnorde MIDI Output") { startThread(); }
    ~ExternalMidiWorker() override
    {
        signalThreadShouldExit(); notify(); stopThread(3000);
        // If a driver blocks indefinitely, JUCE cannot guarantee immediate teardown.
    }
    struct Event { double dueMs = 0; juce::uint8 data[3] {}; int size = 0; };
    static constexpr unsigned capacity = 8192;
    std::array<Event, capacity> events {};
    std::atomic<unsigned> read { 0 }, write { 0 };
    std::shared_ptr<juce::MidiOutput> port;
    // Publish only an atomic flag to processBlock; atomic_load(shared_ptr)
    // may use an internal lock and must stay on the worker/UI threads.
    std::atomic<bool> outputReady { false };
    std::atomic<unsigned> droppedEvents { 0 };

    void setPort(std::shared_ptr<juce::MidiOutput> next)
    {
        const bool ready = (next != nullptr);
        std::atomic_store(&port, std::move(next));
        outputReady.store(ready, std::memory_order_release);
        // Flush pending messages so events from the old port don't leak to the new one.
        // The worker owns the read index; it notices the generation change.
        generation.fetch_add(1, std::memory_order_release);
        notify();
    }
    bool active() const noexcept { return outputReady.load(std::memory_order_acquire); }
    void enqueue(const juce::MidiMessage& msg, double dueMs)
    {
        if (msg.getRawDataSize() < 1 || msg.getRawDataSize() > 3) return;
        const auto w = write.load(std::memory_order_relaxed);
        const auto next = (w + 1) % capacity;
        const auto r = read.load(std::memory_order_acquire);
        if (next == r)
        {
            droppedEvents.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        const bool wasEmpty = (w == r);
        auto& e = events[w];
        e.dueMs = dueMs; e.size = msg.getRawDataSize();
        for (int i = 0; i < e.size; ++i) e.data[i] = msg.getRawData()[i];
        write.store(next, std::memory_order_release);
        // Notify only when transitioning from empty to nonempty. While the
        // queue contains events the worker polls their due timestamps itself.
        if (wasEmpty) notify();
    }
    void run() override
    {
        unsigned currentGeneration = generation.load();
        while (!threadShouldExit())
        {
            const auto gen = generation.load(std::memory_order_acquire);
            if (gen != currentGeneration)
            {
                read.store(write.load(std::memory_order_acquire), std::memory_order_release);
                currentGeneration = gen;
            }
            const auto r = read.load(std::memory_order_relaxed);
            if (r == write.load(std::memory_order_acquire)) { wait(50); continue; }
            const auto e = events[r];
            const double remaining = e.dueMs - juce::Time::getMillisecondCounterHiRes();
            if (remaining > 1.5) { wait(juce::jlimit(1, 5, int(remaining))); continue; }
            if (auto output = std::atomic_load(&port))
                output->sendMessageNow(juce::MidiMessage(e.data, e.size));
            read.store((r + 1) % capacity, std::memory_order_release);
        }
    }
private:
    std::atomic<unsigned> generation { 0 };
};

juce::Array<juce::MidiDeviceInfo> PolyrhythmMIDIProcessor::getAvailableMidiOutputs() const
{
    return juce::MidiOutput::getAvailableDevices();
}
juce::String PolyrhythmMIDIProcessor::getMidiOutputIdentifier() const
{
    return selectedMidiOutputId;
}
juce::String PolyrhythmMIDIProcessor::setMidiOutputIdentifier(const juce::String& id)
{
    if (id.isEmpty())
    {
        selectedMidiOutputId.clear();
        externalMidiWorker->setPort({});
        return {};
    }
    auto device = juce::MidiOutput::openDevice(id); // call from UI/message thread ONLY
    if (!device) return "Could not open MIDI output. Is the port connected or in use?";
    selectedMidiOutputId = id;
    externalMidiWorker->setPort(std::shared_ptr<juce::MidiOutput>(std::move(device)));
    return {};
}
bool PolyrhythmMIDIProcessor::isExternalMidiOutputActive() const noexcept
{
    return externalMidiWorker != nullptr && externalMidiWorker->active();
}
unsigned PolyrhythmMIDIProcessor::getExternalMidiDroppedEvents() const noexcept
{
    return externalMidiWorker != nullptr
        ? externalMidiWorker->droppedEvents.load(std::memory_order_relaxed) : 0u;
}
PolyrhythmMIDIProcessor::~PolyrhythmMIDIProcessor() = default;

PolyrhythmMIDIProcessor::PolyrhythmMIDIProcessor()
    : AudioProcessor(
        BusesProperties()
            .withOutput(
                "Output",
                juce::AudioChannelSet::stereo(),
                true)),
      parameters(
          *this,
          nullptr,
          "PARAMETERS",
          createParameterLayout())
{
    externalMidiWorker = std::make_unique<ExternalMidiWorker>();
    for (int i = 0; i < optionalCount; ++i)
    {
        customCCNumbers[i].store(PnordeSafeCC::first + (i % 8));
        laneTypes[i].store(static_cast<int>(PnordeLane::defaultType(i + baseLanes)));
        displayOrder[i].store(i + baseLanes, std::memory_order_relaxed);
    }
    lastOptionalType.fill(-1);
    dedicatedTargetStep.fill(-1); dedicatedController.fill(-1);
    lastCCSent.fill(-1); lastCCStep.fill(-1); lastCCNumber.fill(-1);
    lastCCEmitPpq.fill(-1.0e12);
    // New or legacy projects: an unset probability is ALWAYS 100%, never 0.
    for (auto& row : cellProbabilities)
        for (auto& cell : row) cell.store(100, std::memory_order_relaxed);
    probabilityLastTick.fill(std::numeric_limits<std::int64_t>::min());
    probabilityHeldStep.fill(0);
    for (auto& step : pattern)
        step.store(false, std::memory_order_relaxed);
    for (auto& glide : aftertouchGlide)
        glide.store(0, std::memory_order_relaxed);
    for (auto& row : optionalGlides)
        for (auto& glide : row) glide.store(0, std::memory_order_relaxed);

    for (int row = 0; row < laneCount; ++row)
    {
        laneRotations[row].store(0, std::memory_order_relaxed);
        laneLengths[row].store(row == 0 ? 16 : row == 1 ? 7 : row == 2 ? 5 : row == 3 ? 3 : 16);
        laneDivisions[row].store(2);
        laneTimings[row].store(0);
        laneCurrentSteps[row].store(-1);
        laneStartSteps[row].store(0, std::memory_order_relaxed);
        for (int i = 0; i < maxSteps; ++i)
            laneValues[row][i].store(row == channelLane || row == 1 || row == 4 || row == 5 || row == 6 || row >= 7 ? (getOptionalLaneType(row) == 4 ? 64 : 0) : row == 2 ? 70 : row == 3 ? 50 : 80);
    }

    for (int row = 0; row < laneCount; ++row)
    {
        const auto suffix = juce::String(row);
        hostedDivisions[row] = parameters.getRawParameterValue("pn_lane_div_" + suffix);
        hostedTimings[row] = parameters.getRawParameterValue("pn_lane_timing_" + suffix);
        hostedDirections[row] = parameters.getRawParameterValue("pn_lane_direction_" + suffix);
        hostedRandomDirections[row] = parameters.getRawParameterValue("pn_lane_random_" + suffix);
        hostedSwings[row] = parameters.getRawParameterValue("pn_lane_swing_" + suffix);
        laneResetParameters[row] = parameters.getRawParameterValue("pn_lane_restart_" + suffix);
        previousLaneResetCounters[row] = getLaneResetCounter(row);
        publishedLanePhaseOrigins[row].store(0.0, std::memory_order_relaxed);
    }
    previousResetCounter = getResetCounter();
    // Restore the familiar original 16-step pattern.
    for (int i = 0; i < 16; i += 2)
        pattern[static_cast<size_t>(i)].store(
            true, std::memory_order_relaxed);
    captureCurrentProgram(factoryProgram);
    // A small experimental responsiveness change for v0.6.26: check for
    // PROGRAM-lane/host automation requests twice as often as before.
    // This is a JUCE MESSAGE-THREAD timer, not an audio-thread scheduler:
    // a busy message thread may still delay the callback, and loading an
    // entire program still happens here rather than sample-accurately.
    // Do NOT increase the GUI/HPPD frame rate or the rate of CC output.
    startTimerHz(PnordeProgramPolling::rateHz);
}

juce::AudioProcessorValueTreeState::ParameterLayout
PolyrhythmMIDIProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    // Stable host-visible identifiers; used by Ableton automation. Editor code
    // reads their atomic values; no listener/heap allocation in processBlock.
    const auto addInt = [&layout](const char* id, const char* name,
                                  int low, int high, int initial)
    {
        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { id, 1 }, name, low, high, initial));
    };
    addInt("pn_clock", "Global Clock", 0, 4, 2);
    addInt("pn_channel", "Default MIDI Channel", 1, 16, 1);
    addInt("pn_bend", "Pitch Bend Range", 0, 3, 2);
    addInt("pn_darkness", "Dark Mode Amount", 0, 75, 0);
    addInt("pn_theme", "Colour Theme", 0, 7, 0);
    addInt("pn_key", "Key", 0, 11, 0);
    addInt("pn_scale", "Scale", 0, 13, PnordeProjectVersion::newInstanceMajorScale);
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "pn_animations", 1 }, "Lighting Animations", true));
    // Append, NEVER insert, host-visible parameters: protects older automation indices.
    addInt("pn_octave", "Global Octave", -2, 2, 0);
    // New host IDs appended so old pn_scale automation stays normalized correctly.
    addInt("pn_scale_ex", "Extended Scale", 0, 20, 0);
    addInt("pn_manual_mode", "Manual Tempo On", 0, 1, 0);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pn_manual_bpm", 1 }, "Manual Tempo BPM",
        juce::NormalisableRange<float>(20.0f, 300.0f, 0.1f), 120.0f));
    // Append new parameter: do NOT change legacy pn_scale / pn_scale_ex
    // normalised ranges or old Ableton automation would be reinterpreted.
    addInt("pn_scale_edo", "Microtonal EDO Mode", 0, PnordeEDO::modeCount, 0);
    // Append-only IDs: do not renumber older Live automation parameters.
    addInt("pn_micro_mode", "EDO Output Mode", 0, 2, 0);
    addInt("pn_poly_pressure", "Experimental Poly Aftertouch", 0, 1, 0);
    // Append-only automation IDs. Keep old pn_octave (-2..2) normalisation.
    addInt("pn_octave_v2", "Global Octave Expanded", -3, 3, 0);
    addInt("pn_octave_v2_active", "Expanded Octave Active", 0, 1, 0);
    addInt("pn_play_enabled", "PNORDE Play", 0, 1, 1);
    addInt("pn_reset_counter", "PNORDE Reset Trigger", 0, 127, 0);
    for (int row=0; row<laneCount; ++row)
    {
        addInt(("pn_lane_div_"+std::to_string(row)).c_str(),
               ("Lane "+std::to_string(row+1)+" Clock Division").c_str(),0,10,2);
        addInt(("pn_lane_timing_"+std::to_string(row)).c_str(),
               ("Lane "+std::to_string(row+1)+" Timing").c_str(),0,2,0);
        addInt(("pn_lane_direction_"+std::to_string(row)).c_str(),
               ("Lane "+std::to_string(row+1)+" Direction").c_str(),0,2,0);
    }
    // v0.6.1: append at the end to preserve the index/order of all v0.6.0 automation.
    addInt("pn_vibes_bleed", "HPPD Main-Area Bleed", 0, 50, 25);
    for (int row = 0; row < laneCount; ++row)
        addInt(("pn_lane_restart_" + std::to_string(row)).c_str(),
               ("Restart Lane " + std::to_string(row + 1) + " Trigger").c_str(),
               0, 127, 0);
    // v0.6.7 append-only automation ID: never reorder earlier parameters.
    addInt("pn_hppd_fps", "HPPD Frames Per Second", 1, 60, 12);
    // v0.6.9: extension slot 0=legacy colour, 1..4=new appended themes.
    // Never change pn_theme's original 0..7 normalised range.
    addInt("pn_theme_extension", "Colour Theme Extension", 0, 4, 0);
    addInt("pn_hypnosis", "HPPD Glow Strength", 0, 100, 100);
    // Append only: existing pn_lane_direction_0..20 IDs keep their values/indices.
    addInt("pn_global_reverse", "Global Reverse", 0, 1, 0);
    // v0.6.15: append-only. MIDI mapping/Live Configure -> 0..3.
    addInt("pn_program_slot", "PNORDE Quick Program", 0, 3, 0);
    // v0.6.22: append AFTER existing parameters so Ableton automation IDs
    // remain byte-for-byte compatible. Swing is per lane, per Quick Program.
    for (int row = 0; row < laneCount; ++row)
        addInt(("pn_lane_swing_" + std::to_string(row)).c_str(),
               ("Lane " + std::to_string(row + 1) + " Swing %").c_str(),
               PnordeSwing::straight, PnordeSwing::maximum, PnordeSwing::straight);
    // APPEND these IDs ONLY after the older parameters. Increasing the range
    // of pn_lane_direction_* from 0..2 to 0..3 would CHANGE the meaning of
    // existing normalised Ableton automation: do not do that.
    // Instead, each new 0/1 switch overrides its lane's old direction with
    // RANDOM when ON, and returns to FWD/REV/PING when OFF.
    for (int row = 0; row < laneCount; ++row)
        addInt(("pn_lane_random_" + std::to_string(row)).c_str(),
               ("Lane " + std::to_string(row + 1) + " Random Direction").c_str(),
               0, 1, 0);
    return layout;
}

namespace
{
// JUCE APVTS saves parameter values in child ValueTrees identified by `id`
// (typically <PARAM id="..." value="..."/>), not as root properties.
// Keep compatibility with older/root-property representations as well.
juce::ValueTree pnordeFindSavedParameter(const juce::ValueTree& tree,
                                         const juce::String& parameterId)
{
    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        auto child = tree.getChild(i);
        if (child.getProperty("id").toString() == parameterId)
            return child;
    }
    return {};
}

bool pnordeSavedParameterExists(const juce::ValueTree& tree,
                                const juce::String& parameterId)
{
    return tree.hasProperty(juce::Identifier(parameterId)) ||
           pnordeFindSavedParameter(tree, parameterId).isValid();
}

// Preserve a live host parameter when loading a musical program. Copy its
// actual APVTS child value; adding a same-named root property alone has no
// effect on the plug-in parameter and previously caused selection snap-back.
void pnordeKeepLiveParameter(juce::ValueTree& scene,
                             const juce::ValueTree& live,
                             const juce::String& parameterId)
{
    const auto liveChild = pnordeFindSavedParameter(live, parameterId);
    if (liveChild.isValid())
    {
        auto sceneChild = pnordeFindSavedParameter(scene, parameterId);
        if (sceneChild.isValid())
            sceneChild.copyPropertiesFrom(liveChild, nullptr);
        else
            scene.addChild(liveChild.createCopy(), -1, nullptr);
    }
    const auto property = juce::Identifier(parameterId);
    if (live.hasProperty(property))
        scene.setProperty(property, live.getProperty(property), nullptr);
}

void pnordeSetAutomatable(juce::AudioProcessorValueTreeState& tree,
                         const char* id, int value)
{
    if (auto* parameter = tree.getParameter(id))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(
            static_cast<float>(value)));
}
}
void PolyrhythmMIDIProcessor::setDarknessPercent(int v) noexcept
{ pnordeSetAutomatable(parameters, "pn_darkness", juce::jlimit(0, 75, v)); }
void PolyrhythmMIDIProcessor::setAnimationsEnabled(bool b) noexcept
{ pnordeSetAutomatable(parameters, "pn_animations", b ? 1 : 0); }
void PolyrhythmMIDIProcessor::setDefaultMidiChannel(int v) noexcept
{ pnordeSetAutomatable(parameters, "pn_channel", juce::jlimit(1, 16, v)); }
void PolyrhythmMIDIProcessor::setGlobalOctave(int v) noexcept
{
    pnordeSetAutomatable(parameters, "pn_octave_v2", juce::jlimit(-3,3,v));
    pnordeSetAutomatable(parameters, "pn_octave_v2_active", 1);
}
void PolyrhythmMIDIProcessor::setGlobalClockIndex(int v) noexcept
{ pnordeSetAutomatable(parameters, "pn_clock", juce::jlimit(0, 4, v)); }
void PolyrhythmMIDIProcessor::setBendRangeIndex(int v) noexcept
{ pnordeSetAutomatable(parameters, "pn_bend", juce::jlimit(0, 3, v)); }
void PolyrhythmMIDIProcessor::setThemeIndex(int v) noexcept
{
    v = juce::jlimit(0, 11, v);
    // Both IDs remain appended/unchanged so existing Ableton automation IDs
    // and old saved theme selections are not reinterpreted.
    if (v >= 8)
        pnordeSetAutomatable(parameters, "pn_theme_extension", v - 7);
    else
    {
        pnordeSetAutomatable(parameters, "pn_theme_extension", 0);
        pnordeSetAutomatable(parameters, "pn_theme", v);
    }
}
void PolyrhythmMIDIProcessor::setHypnosis(int n) noexcept
{ pnordeSetAutomatable(parameters, "pn_hypnosis", juce::jlimit(0, 100, n)); }
void PolyrhythmMIDIProcessor::setKey(int v) noexcept
{ pnordeSetAutomatable(parameters, "pn_key", juce::jlimit(0, 11, v)); }
void PolyrhythmMIDIProcessor::setScale(int v) noexcept
{
    v = juce::jlimit(0, PnordeScales::count - 1, v);
    if (PnordeEDO::isMode(v))
    {
        pnordeSetAutomatable(parameters, "pn_scale_ex", 0);
        pnordeSetAutomatable(parameters, "pn_scale_edo",
                            v - (PnordeEDO::firstScaleIndex - 1));
    }
    else
    {
        pnordeSetAutomatable(parameters, "pn_scale_edo", 0);
        if (v < PnordeScales::legacyCount)
        {
            pnordeSetAutomatable(parameters, "pn_scale_ex", 0);
            pnordeSetAutomatable(parameters, "pn_scale", v);
        }
        else
        {
            pnordeSetAutomatable(parameters, "pn_scale", 0);
            pnordeSetAutomatable(parameters, "pn_scale_ex", v - (PnordeScales::legacyCount - 1));
        }
    }
}
void PolyrhythmMIDIProcessor::setMicroOutputMode(int mode) noexcept
{ pnordeSetAutomatable(parameters, "pn_micro_mode", juce::jlimit(0, 2, mode)); }
void PolyrhythmMIDIProcessor::setExperimentalPolyPressure(bool yes) noexcept
{ pnordeSetAutomatable(parameters, "pn_poly_pressure", yes ? 1 : 0); }
void PolyrhythmMIDIProcessor::setManualTempoEnabled(bool yes) noexcept
{ pnordeSetAutomatable(parameters, "pn_manual_mode", yes ? 1 : 0); }
void PolyrhythmMIDIProcessor::setManualTempoBpm(double bpm) noexcept
{
    if (!std::isfinite(bpm)) return;
    if (auto* parameter = parameters.getParameter("pn_manual_bpm"))
    {
        const float value = juce::jlimit(20.0f, 300.0f, static_cast<float>(bpm));
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }
}

// WHAT HAPPENS HERE: INITIALISATION: Called when the host selects a sample rate or block size. Reset scheduler state carefully so old timing cannot leak into the next transport run.
void PolyrhythmMIDIProcessor::prepareToPlay(
    double sampleRate,
    int)
{
    currentSampleRate = sampleRate;
    developerSampleRate.store(sampleRate, std::memory_order_relaxed);
    noteIsActive = false;
    previousPlayEnabled = getPlayEnabled();
    previousResetCounter = getResetCounter();
    retainedBeatPpq = 0.0;
    for (int row = 0; row < laneCount; ++row)
    {
        lanePhaseOrigins[row] = 0.0;
        publishedLanePhaseOrigins[row].store(0.0, std::memory_order_relaxed);
        previousLaneResetCounters[row] = getLaneResetCounter(row);
    }
    // Reset probability memory when a new audio session begins.
    probabilityLastTick.fill(std::numeric_limits<std::int64_t>::min());
    probabilityHeldStep.fill(0);
    previousBlockWasPlaying = false;
    previousManualMode = false;
    manualBlockEndPpq = previousHostBlockEndPpq = 0.0;
    activeNote = 60;
    activeChannel = getDefaultMidiChannel();
    activeBend = 8192;
    lastSlide = -1;
    glideStartPpq = glideEndPpq = 0.0;
    lastPitchSemis = glideFromSemis = glideToSemis = 60.0;
    lastPressure = pressureFrom = pressureTo = 0.0;
    pressureStartPpq = pressureEndPpq = 0.0;
    nextModulationPpq = 0.0;
    lastPressureSent = -1;
    lastAftertouchStep = -1;
    noteOffPpq = 0.0;
    hadPreviousPitch = false;
    legatoVoice = false;
    lastCCSent.fill(-1);
    lastCCStep.fill(-1);
    lastCCNumber.fill(-1);
    lastCCEmitPpq.fill(-1.0e12);
    lastOptionalType.fill(-1);
    dedicatedTargetStep.fill(-1); dedicatedController.fill(-1);
    dedicatedSent.fill(-1); dedicatedSentChannel.fill(-1);
    for (auto& step : laneCurrentSteps) step.store(-1);
    currentStep.store(-1, std::memory_order_relaxed);
    globalReversePhase.reset(getGlobalReverse());
    hostPlaying.store(false, std::memory_order_relaxed);
    hostPpq.store(0.0, std::memory_order_relaxed);
    hostClockUpdateMs.store(0.0, std::memory_order_relaxed);
    hostBpm.store(120.0, std::memory_order_relaxed);
}

// WHAT HAPPENS HERE: CLEANUP: Host releases audio resources. This is separate from saving project state.
void PolyrhythmMIDIProcessor::releaseResources()
{
    hostPlaying.store(false, std::memory_order_relaxed);
    hostPpq.store(0.0, std::memory_order_relaxed);
    hostClockUpdateMs.store(0.0, std::memory_order_relaxed);
    previousBlockWasPlaying = false;
}

bool PolyrhythmMIDIProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet()
        == juce::AudioChannelSet::stereo();
}

// WHAT HAPPENS HERE: PLAY CONTROL: Request a change in PNORDE playback state; this must not silently become a property of one Quick Program.
void PolyrhythmMIDIProcessor::requestPlay()
{ setPlayEnabled(true); }
// WHAT HAPPENS HERE: PLAY CONTROL: Store the global playback switch. Program switching must preserve the user/host-selected setting.
void PolyrhythmMIDIProcessor::setPlayEnabled(bool enabled) noexcept
{ pnordeSetAutomatable(parameters,"pn_play_enabled",enabled ? 1 : 0); }
void PolyrhythmMIDIProcessor::requestSequenceReset() noexcept
{ pnordeSetAutomatable(parameters,"pn_reset_counter", (getResetCounter()+1)%128); }
int PolyrhythmMIDIProcessor::getLaneResetCounter(int row) const noexcept
{
    if (row < 0 || row >= laneCount || laneResetParameters[row] == nullptr) return 0;
    return PnordeSafeAutomation::truncate(laneResetParameters[row]->load(std::memory_order_relaxed), 0, 127, 0);
}
void PolyrhythmMIDIProcessor::requestLaneReset(int row) noexcept
{
    if (row < 0 || row >= laneCount) return;
    const auto id = "pn_lane_restart_" + juce::String(row);
    if (auto* parameter = parameters.getParameter(id))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(
            static_cast<float>((getLaneResetCounter(row) + 1) % 128)));
}
double PolyrhythmMIDIProcessor::getLanePhaseOrigin(int row) const noexcept
{
    return row >= 0 && row < laneCount
        ? publishedLanePhaseOrigins[row].load(std::memory_order_relaxed) : 0.0;
}
void PolyrhythmMIDIProcessor::setHppdFps(int fps) noexcept
{ pnordeSetAutomatable(parameters, "pn_hppd_fps", juce::jlimit(1,60,fps)); }
void PolyrhythmMIDIProcessor::setVibesBleed(int amount) noexcept
{ pnordeSetAutomatable(parameters, "pn_vibes_bleed", juce::jlimit(0,50,amount)); }
void PolyrhythmMIDIProcessor::setGlobalReverse(bool on) noexcept
{ pnordeSetAutomatable(parameters, "pn_global_reverse", on ? 1 : 0); }

// This setting is independent of the direction dial. Forward/Reverse/Random
// choose a VISIBLE slot, then rotation decides which STORED cell occupies it.
// The MIDI thread only reads an atomic integer; it never rearranges arrays.
int PolyrhythmMIDIProcessor::getLaneRotation(int row) const noexcept
{
    return row >= 0 && row < laneCount
        ? laneRotations[static_cast<size_t>(row)].load(std::memory_order_relaxed) : 0;
}
// WHAT HAPPENS HERE: LANE ROTATION: Change the visual/logical start offset without rewriting every step. Rotation and stored step data are different concepts.
void PolyrhythmMIDIProcessor::setLaneRotation(int row, int steps) noexcept
{
    if (row < 0 || row >= laneCount) return;
    laneRotations[static_cast<size_t>(row)].store(
        PnordeRotation::wrap(steps, getLaneLength(row)), std::memory_order_relaxed);
}
int PolyrhythmMIDIProcessor::getProgramLaneRotation() const noexcept
{
    return programLaneRotation.load(std::memory_order_relaxed);
}
void PolyrhythmMIDIProcessor::setProgramLaneRotation(int steps) noexcept
{
    programLaneRotation.store(PnordeRotation::wrap(steps, getProgramLaneLength()),
                              std::memory_order_relaxed);
}
int PolyrhythmMIDIProcessor::getLaneDirection(int row) const noexcept
{
    if (row < 0 || row >= laneCount) return PnordePlayback::forward;
    // Read only simple atomic numbers on the real-time audio thread.
    // RANDOM is a separate host parameter for backwards compatibility.
    if (hostedRandomDirections[row] != nullptr &&
        hostedRandomDirections[row]->load(std::memory_order_relaxed) >= 0.5f)
        return PnordePlayback::randomStep;
    return hostedDirections[row] != nullptr
        ? PnordeSafeAutomation::truncate(hostedDirections[row]->load(std::memory_order_relaxed), 0, 2, 0)
        : PnordePlayback::forward;
}
void PolyrhythmMIDIProcessor::setLaneDirection(int row, int direction) noexcept
{
    if (row < 0 || row >= laneCount) return;
    direction = juce::jlimit(0, 3, direction);
    // The new RANDOM toggle is ON only for choice 3. Switching back to the
    // original modes turns it OFF; choosing RANDOM leaves the previous
    // non-random setting available underneath for old Ableton automation.
    const auto randomId = "pn_lane_random_" + juce::String(row);
    if (auto* parameter = parameters.getParameter(randomId))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(
            direction == PnordePlayback::randomStep ? 1.0f : 0.0f));
    if (direction != PnordePlayback::randomStep)
    {
        const auto oldDirectionId = "pn_lane_direction_" + juce::String(row);
        if (auto* parameter = parameters.getParameter(oldDirectionId))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(
                static_cast<float>(direction)));
    }
}

bool PolyrhythmMIDIProcessor::getStepEnabled(
    int index) const noexcept
{
    if (index < 0 || index >= static_cast<int>(pattern.size()))
        return false;

    return pattern[static_cast<size_t>(index)].load(
        std::memory_order_relaxed);
}

void PolyrhythmMIDIProcessor::setStepEnabled(
    int index,
    bool enabled) noexcept
{
    if (index < 0 || index >= static_cast<int>(pattern.size()))
        return;

    pattern[static_cast<size_t>(index)].store(
        enabled, std::memory_order_relaxed);
}

int PolyrhythmMIDIProcessor::getSequenceLength() const noexcept
{
    return sequenceLength.load(std::memory_order_relaxed);
}

void PolyrhythmMIDIProcessor::setSequenceLength(
    int length) noexcept
{
    sequenceLength.store(
        juce::jlimit(1, 128, length),
        std::memory_order_relaxed);
    laneLengths[0].store(getSequenceLength(), std::memory_order_relaxed);
    setLaneStartStep(0, juce::jmin(getLaneStartStep(0), getSequenceLength() - 1));
}

int PolyrhythmMIDIProcessor::getClockDivisionIndex() const noexcept
{
    return clockDivisionIndex.load(std::memory_order_relaxed);
}

void PolyrhythmMIDIProcessor::setClockDivisionIndex(
    int index) noexcept
{
    clockDivisionIndex.store(
        juce::jlimit(0, 10, index),
        std::memory_order_relaxed);
    laneDivisions[0].store(getClockDivisionIndex(), std::memory_order_relaxed);
    const auto id="pn_lane_div_0";
    if (auto* param=parameters.getParameter(id))
        param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(getClockDivisionIndex())));
}

int PolyrhythmMIDIProcessor::getLaneLength(int row) const noexcept
{
    return row >= 0 && row < laneCount ? laneLengths[row].load(std::memory_order_relaxed) : 16;
}
// WHAT HAPPENS HERE: LANE LENGTH: Clamp the number of active steps to the range the engine supports. Inactive stored steps may still matter when expanding again.
void PolyrhythmMIDIProcessor::setLaneLength(int row, int length) noexcept
{
    if (row < 0 || row >= laneCount) return;
    if (row == 0) { setSequenceLength(length); return; }
    const int clamped = juce::jlimit(1, maxSteps, length);
    laneLengths[row].store(clamped, std::memory_order_relaxed);
    setLaneStartStep(row, juce::jmin(getLaneStartStep(row), clamped - 1));
}
int PolyrhythmMIDIProcessor::getLaneDivision(int row) const noexcept
{
    return row>=0 && row<laneCount && hostedDivisions[row]!=nullptr
        ? PnordeSafeAutomation::truncate(hostedDivisions[row]->load(std::memory_order_relaxed), 0, 10, 2) : 2;
}
void PolyrhythmMIDIProcessor::setLaneDivision(int row, int division) noexcept
{
    if (row < 0 || row >= laneCount) return;
    division=juce::jlimit(0,10,division);
    laneDivisions[row].store(division,std::memory_order_relaxed);
    const auto id="pn_lane_div_"+juce::String(row);
    if (auto* param=parameters.getParameter(id))
        param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(division)));
    if (row==0) clockDivisionIndex.store(division,std::memory_order_relaxed);
}
int PolyrhythmMIDIProcessor::getLaneStartStep(int row) const noexcept
{
    if (row < 0 || row >= laneCount) return 0;
    return juce::jlimit(0, juce::jmax(0, getLaneLength(row) - 1),
        laneStartSteps[static_cast<size_t>(row)].load(std::memory_order_relaxed));
}

void PolyrhythmMIDIProcessor::setLaneStartStep(int row, int step) noexcept
{
    if (row < 0 || row >= laneCount) return;
    laneStartSteps[static_cast<size_t>(row)].store(
        juce::jlimit(0, juce::jmax(0, getLaneLength(row) - 1), step),
        std::memory_order_relaxed);
}

// The rows are numbered from zero. Row 1 is PITCH. Its integer values are
// stored as an offset of tenths of a semitone from the SELECTED KEY.
//
// .load() reads a std::atomic<int>. Think of an atomic as a small shared
// number that the editor and sound-processing thread can access without
// taking turns holding a mutex (lock). 'relaxed' is sufficient for each
// independent lane cell; it does not promise a whole-pattern snapshot.
int PolyrhythmMIDIProcessor::getCellProbability(int row, int step) const noexcept
{
    if (!juce::isPositiveAndBelow(row, probabilityRows) ||
        !juce::isPositiveAndBelow(step, maxSteps)) return 100;
    return cellProbabilities[static_cast<size_t>(row)][static_cast<size_t>(step)]
        .load(std::memory_order_relaxed);
}
// WHAT HAPPENS HERE: PROBABILITY: Each step has a percentage; zero means never trigger, one hundred means always trigger.
void PolyrhythmMIDIProcessor::setCellProbability(int row, int step, int percent) noexcept
{
    if (!juce::isPositiveAndBelow(row, probabilityRows) ||
        !juce::isPositiveAndBelow(step, maxSteps)) return;
    cellProbabilities[static_cast<size_t>(row)][static_cast<size_t>(step)]
        .store(juce::jlimit(0, 100, percent), std::memory_order_relaxed);
}

int PolyrhythmMIDIProcessor::getLaneValue(int row, int step) const noexcept
{
    if (row < 1 || row >= laneCount || step < 0 || step >= maxSteps) return 0;
    return laneValues[row][step].load(std::memory_order_relaxed);
}
// Write the ORIGINAL editable value. juce::jlimit(min, max, value) is JUCE's
// equivalent of 'keep this number inside the allowed interval'. The scale
// must NOT be applied here, or switching scales would lose the user's work.
void PolyrhythmMIDIProcessor::setLaneValue(int row, int step, int value) noexcept
{
    if (row < 1 || row >= laneCount || step < 0 || step >= maxSteps) return;
    const int kind = getOptionalLaneType(row);
    laneValues[row][step].store(juce::jlimit(row == 1 ? -120 : row == 4 ? -4 : row >= 5 ? 0 : 1,
                                             kind == 6 ? 16 : kind == 4 ? 127 : row == 1 ? 120 : row == 4 ? 4 : 100, value),
                                std::memory_order_relaxed);
}
// This is an optional PREVIEW helper. It must never be used when saving a
// PITCH cell, because snapping a stored value destroys the original melody.
// The octave argument is kept for compatibility with existing editor callers;
// scale positions repeat every octave, so it is not needed here.
int PolyrhythmMIDIProcessor::quantizePitchTenths(int tenths, int octave) const noexcept
{
    (void) octave;
    return juce::jlimit(-120, 120, static_cast<int>(std::lround(
        PnordeRelativePitch::scaleRelativeSemitones(tenths, getScale()) * 10.0)));
}

// What a PITCH cell SHOWS ON SCREEN. SCALE may change its displayed offset,
// but KEY must not: these are intervals relative to whichever key is chosen.
//
// A future MIDI-controlled KEY will therefore affect only the sounding note,
// not the numbers that the musician uses to programme the PITCH lane.
// Nothing here changes the underlying, saved laneValues[][] array.
double PolyrhythmMIDIProcessor::getDisplayedPitchOffsetSemitones(int storedTenths) const noexcept
{
    return PnordeRelativePitch::displaySemitones(storedTenths, getScale());
}

// What the PITCH cell will SOUND like after key transposition and scale
// correction. The original value in laneValues[][] remains unchanged.
// WHAT HAPPENS HERE: SCALE QUANTISATION: Interpret a stored relative pitch value using key and scale rules without altering the stored data.
double PolyrhythmMIDIProcessor::getEffectivePitchOffsetSemitones(int storedTenths) const noexcept
{
    return PnordeRelativePitch::outputSemitones(storedTenths, getKey(), getScale());
}

void PolyrhythmMIDIProcessor::deletePage(int row, int page) noexcept
{
    if (row < 0 || row >= laneCount || page < 0 || page >= maxSteps / 16) return;
    const int length = getLaneLength(row);
    const int start = page * 16;
    if (start >= length || length <= 16) return;
    const int removed = juce::jmin(16, length - start);
    for (int i = start; i < maxSteps - removed; ++i)
    {
        if (row == 0) setStepEnabled(i, getStepEnabled(i + removed));
        else
        {
            setLaneValue(row, i, getLaneValue(row, i + removed));
            if (getOptionalLaneType(row) == 3) setVibratoDepth(i, getVibratoDepth(i + removed));
        }
    }
    for (int i = maxSteps - removed; i < maxSteps; ++i)
    {
        if (row == 0) setStepEnabled(i, false);
        else
        {
            setLaneValue(row, i, row == 2 ? 70 : row == 3 ? 50 : getOptionalLaneType(row) == 4 ? 64 : 0);
            if (getOptionalLaneType(row) == 3) setVibratoDepth(i, 0);
        }
    }
    const int oldStart = getLaneStartStep(row);
    setLaneLength(row, juce::jmax(1, length - removed));
    if (oldStart >= start + removed)
        setLaneStartStep(row, oldStart - removed);
    else if (oldStart >= start)
        setLaneStartStep(row, start);
}
void PolyrhythmMIDIProcessor::appendPage(int row) noexcept
{
    if (row < 0 || row >= laneCount) return;
    const int oldLength = getLaneLength(row);
    const int newLength = juce::jmin(maxSteps, ((oldLength + 15) / 16 + 1) * 16);
    for (int i = oldLength; i < newLength; ++i)
    {
        if (row == 0) setStepEnabled(i, false);
        else
        {
            setLaneValue(row, i, row == 2 ? 70 : row == 3 ? 50 : getOptionalLaneType(row) == 4 ? 64 : 0);
            if (getOptionalLaneType(row) == 3) setVibratoDepth(i, 0);
        }
    }
    setLaneLength(row, newLength);
}

bool PolyrhythmMIDIProcessor::isDedicatedEnabled(int row) const noexcept
{
    return row >= modRow && row <= sustainRow &&
        dedicatedEnabled[static_cast<size_t>(row - modRow)].load(std::memory_order_relaxed);
}
void PolyrhythmMIDIProcessor::setDedicatedEnabled(int row, bool enabled) noexcept
{
    if (row >= modRow && row <= sustainRow)
        dedicatedEnabled[static_cast<size_t>(row - modRow)].store(enabled, std::memory_order_relaxed);
}
bool PolyrhythmMIDIProcessor::isOptionalLaneEnabled(int row) const noexcept
{
    if (row < baseLanes || row >= laneCount) return false;
    if (row < modRow) return row - baseLanes < getCustomCCCount();
    if (row == channelLane) return hasMidiChannelLane();
    return isDedicatedEnabled(row);
}
int PolyrhythmMIDIProcessor::getOptionalLaneType(int row) const noexcept
{
    if (row < baseLanes || row >= laneCount) return -1;
    return laneTypes[static_cast<size_t>(row - baseLanes)].load(std::memory_order_relaxed);
}
bool PolyrhythmMIDIProcessor::hasActiveLaneType(int type, int excludingRow) const noexcept
{
    if (type < 1 || type > 6) return false;
    for (int row = baseLanes; row < laneCount; ++row)
        if (row != excludingRow && isOptionalLaneEnabled(row) && getOptionalLaneType(row) == type)
            return true;
    return false;
}
int PolyrhythmMIDIProcessor::activeCCMacroCount() const noexcept
{
    int n = 0;
    for (int row = baseLanes; row < laneCount; ++row)
        if (isOptionalLaneEnabled(row) && getOptionalLaneType(row) == 0) ++n;
    return n;
}
// WHAT HAPPENS HERE: OPTIONAL LANES: A slot can be reassigned to a controller or other lane type; avoid breaking existing per-slot state.
bool PolyrhythmMIDIProcessor::setOptionalLaneType(int row, int type) noexcept
{
    if (row < baseLanes || row >= laneCount || type < 0 || type > 6) return false;
    const int current = getOptionalLaneType(row);
    if (current == type) return true;
    if (type > 0 && hasActiveLaneType(type, row)) return false;
    if (type == 0 && isOptionalLaneEnabled(row) && activeCCMacroCount() >= maxCustomCC) return false;
    // The vertical step values stay attached to the row. Only the second
    // (horizontal vibrato-depth) axis is cleared when its role changes.
    if (current == 3 || type == 3)
        for (int step = 0; step < maxSteps; ++step) setVibratoDepth(step, 0);
    laneTypes[static_cast<size_t>(row - baseLanes)].store(type, std::memory_order_relaxed);
    const int upper = type == 6 ? 16 : type == 4 ? 127 : 100;
    for (int step = 0; step < maxSteps; ++step)
        laneValues[row][step].store(juce::jlimit(0, upper, getLaneValue(row, step)), std::memory_order_relaxed);
    return true;
}
int PolyrhythmMIDIProcessor::addOptionalLane(int type) noexcept
{
    if (type < 0 || type > 6) return -1;
    if (type == 0 && activeCCMacroCount() >= maxCustomCC) return -1;
    if (type > 0 && hasActiveLaneType(type)) return -1;
    int row = -1;
    if (type == 0 && getCustomCCCount() < maxCustomCC)
        row = baseLanes + getCustomCCCount();
    else if (type > 0)
    {
        const int canonical = type == 1 ? modRow : type == 2 ? expressionRow :
                              type == 3 ? vibratoRow : type == 4 ? panRow :
                              type == 5 ? sustainRow : channelLane;
        if (!isOptionalLaneEnabled(canonical)) row = canonical;
    }
    if (row < 0)
        for (int r = modRow; r < laneCount && row < 0; ++r)
            if (!isOptionalLaneEnabled(r)) row = r;
    if (row < 0 && getCustomCCCount() < maxCustomCC)
        row = baseLanes + getCustomCCCount();
    if (row < 0) return -1;
    // Empty rows can change role without conflicts. Their original data is
    // deliberately left intact for re-use; only the type is changed.
    if (!setOptionalLaneType(row, type)) return -1;
    if (row < modRow) setCustomCCCount(getCustomCCCount() + 1);
    else if (row == channelLane) setMidiChannelLaneEnabled(true);
    else setDedicatedEnabled(row, true);
    return row;
}
void PolyrhythmMIDIProcessor::removeOptionalLane(int row) noexcept
{
    if (!isOptionalLaneEnabled(row)) return;
    if (row < modRow)
    {
        // Macro removal compacts physical MIDI rows. Rebase only the visual
        // permutation; the audio thread never reads displayOrder.
        const int lastRow = baseLanes + getCustomCCCount() - 1;
        PnordeVisualOrder::Rows order {};
        for (int i = 0; i < optionalCount; ++i) order[i] = getDisplayRowAt(i);
        const bool safeOrder = PnordeVisualOrder::removeCompacted(order, row, lastRow);
        removeCustomCC(row - baseLanes);
        if (!safeOrder) order = PnordeVisualOrder::identity();
        for (int i = 0; i < optionalCount; ++i)
            displayOrder[i].store(order[i], std::memory_order_relaxed);
        return;
    }
    setLaneName(row, {});
    setLaneStartStep(row, 0);
    const int previousKind = getOptionalLaneType(row);
    if (row == channelLane) setMidiChannelLaneEnabled(false);
    else setDedicatedEnabled(row, false);
    if (previousKind == 3)
        for (int step = 0; step < maxSteps; ++step) setVibratoDepth(step, 0);
    laneTypes[static_cast<size_t>(row - baseLanes)].store(
        static_cast<int>(PnordeLane::defaultType(row)), std::memory_order_relaxed);
}
int PolyrhythmMIDIProcessor::getAftertouchGlide(int step) const noexcept
{
    return juce::isPositiveAndBelow(step, maxSteps)
        ? aftertouchGlide[static_cast<size_t>(step)].load(std::memory_order_relaxed) : 0;
}
void PolyrhythmMIDIProcessor::setAftertouchGlide(int step, int value) noexcept
{
    if (juce::isPositiveAndBelow(step, maxSteps))
        aftertouchGlide[static_cast<size_t>(step)].store(PnordeAftertouchGlide::clampSetting(value),
                                                        std::memory_order_relaxed);
}
int PolyrhythmMIDIProcessor::getDisplayRowAt(int optionalPosition) const noexcept
{
    return juce::isPositiveAndBelow(optionalPosition, optionalCount)
        ? displayOrder[static_cast<size_t>(optionalPosition)].load(std::memory_order_relaxed) : -1;
}
void PolyrhythmMIDIProcessor::moveDisplayRow(int sourceRow, int targetRow) noexcept
{
    if (sourceRow < baseLanes || sourceRow >= laneCount ||
        targetRow < baseLanes || targetRow >= laneCount || sourceRow == targetRow) return;
    PnordeVisualOrder::Rows order {};
    for (int i = 0; i < optionalCount; ++i) order[i] = getDisplayRowAt(i);
    if (!PnordeVisualOrder::move(order, sourceRow, targetRow)) return;
    for (int i = 0; i < optionalCount; ++i)
        displayOrder[i].store(order[i], std::memory_order_relaxed);
}

int PolyrhythmMIDIProcessor::getVibratoDepth(int step) const noexcept
{
    return step >= 0 && step < maxSteps ? vibratoDepthValues[static_cast<size_t>(step)].load() : 0;
}
void PolyrhythmMIDIProcessor::setVibratoDepth(int step, int value) noexcept
{
    if (step >= 0 && step < maxSteps)
        vibratoDepthValues[static_cast<size_t>(step)].store(juce::jlimit(0, 100, value));
}

int PolyrhythmMIDIProcessor::getMacroType(int slot) const noexcept
{
    return slot >= 0 && slot < maxCustomCC ? macroTypes[slot].load() : 0;
}
void PolyrhythmMIDIProcessor::setMacroType(int slot, int type) noexcept
{
    if (slot >= 0 && slot < maxCustomCC)
        macroTypes[slot].store(juce::jlimit(0, 6, type));
}

int PolyrhythmMIDIProcessor::getMacroChannel(int slot) const noexcept
{
    return slot >= 0 && slot < maxCustomCC ? macroChannels[slot].load() : 0;
}
void PolyrhythmMIDIProcessor::setMacroChannel(int slot, int channel) noexcept
{
    if (slot >= 0 && slot < maxCustomCC)
        macroChannels[slot].store(juce::jlimit(0, 16, channel));
}

int PolyrhythmMIDIProcessor::getCustomCCNumber(int slot) const noexcept
{
    return slot >= 0 && slot < optionalCount ? customCCNumbers[slot].load() : 20;
}
// WHAT HAPPENS HERE: CUSTOM MIDI CC: Store the controller number for an optional Macro lane. MIDI CC messages use controller numbers 0–127.
void PolyrhythmMIDIProcessor::setCustomCCNumber(int slot, int number) noexcept
{
    if (slot >= 0 && slot < optionalCount)
        customCCNumbers[slot].store(PnordeSafeCC::sanitise(number));
}
juce::String PolyrhythmMIDIProcessor::getCustomCCLabel(int slot) const
{
    if (slot < 0 || slot >= optionalCount) return {};
    const juce::SpinLock::ScopedLockType lock(customCCLabelsLock);
    return customCCLabels[static_cast<size_t>(slot)];
}
void PolyrhythmMIDIProcessor::setCustomCCLabel(int slot, const juce::String& label)
{
    if (slot < 0 || slot >= optionalCount) return;
    const juce::SpinLock::ScopedLockType lock(customCCLabelsLock);
    customCCLabels[static_cast<size_t>(slot)] = label.substring(0, 12);
}
juce::String PolyrhythmMIDIProcessor::getLaneName(int row) const
{
    if (!juce::isPositiveAndBelow(row, laneCount)) return {};
    const juce::SpinLock::ScopedLockType lock(customCCLabelsLock);
    return customLaneNames[static_cast<size_t>(row)];
}
void PolyrhythmMIDIProcessor::setLaneName(int row, const juce::String& name)
{
    if (!juce::isPositiveAndBelow(row, laneCount)) return;
    // Bounded names; never called from the audio callback. No MIDI impact.
    const auto clean = name.trim().substring(0, 24).removeCharacters("\r\n\t");
    const juce::SpinLock::ScopedLockType lock(customCCLabelsLock);
    customLaneNames[static_cast<size_t>(row)] = clean;
}
int PolyrhythmMIDIProcessor::getOptionalGlide(int row, int step) const noexcept
{
    if (row < baseLanes || row >= laneCount || !juce::isPositiveAndBelow(step, maxSteps)) return 0;
    return optionalGlides[static_cast<size_t>(row - baseLanes)][static_cast<size_t>(step)].load(std::memory_order_relaxed);
}
void PolyrhythmMIDIProcessor::setOptionalGlide(int row, int step, int value) noexcept
{
    if (row < baseLanes || row >= laneCount || !juce::isPositiveAndBelow(step, maxSteps)) return;
    optionalGlides[static_cast<size_t>(row - baseLanes)][static_cast<size_t>(step)].store(
        PnordeTempoGlide::clamp(value), std::memory_order_relaxed);
}
int PolyrhythmMIDIProcessor::getCCTransitionMs(int slot, int step) const noexcept
{
    if (slot < 0 || slot >= optionalCount || step < 0 || step >= maxSteps) return 0;
    return ccTransitionMs[slot][step].load(std::memory_order_relaxed);
}
void PolyrhythmMIDIProcessor::setCCTransitionMs(int slot, int step, int ms) noexcept
{
    if (slot < 0 || slot >= optionalCount || step < 0 || step >= maxSteps) return;
    ccTransitionMs[slot][step].store(juce::jlimit(0, 100, ms), std::memory_order_relaxed);
}
void PolyrhythmMIDIProcessor::removeCustomCC(int slot) noexcept
{
    const int count = getCustomCCCount();
    if (slot < 0 || slot >= count) return;
    for (int i = slot; i < count - 1; ++i)
    {
        customCCNumbers[i].store(customCCNumbers[i + 1].load());
        laneTypes[i].store(laneTypes[i + 1].load());
        setMacroType(i, getMacroType(i + 1));
        setMacroChannel(i, getMacroChannel(i + 1));
        for (int j = 0; j < maxSteps; ++j)
        {
            setCCTransitionMs(i, j, getCCTransitionMs(i + 1, j));
            setOptionalGlide(i + baseLanes, j, getOptionalGlide(i + baseLanes + 1, j));
        }
        setCustomCCLabel(i, getCustomCCLabel(i + 1));
        setLaneName(i + baseLanes, getLaneName(i + baseLanes + 1));
        const int dest = i + baseLanes, src = dest + 1;
        laneLengths[dest].store(laneLengths[src].load());
        setLaneStartStep(dest, getLaneStartStep(src));
        laneDivisions[dest].store(laneDivisions[src].load());
        laneTimings[dest].store(laneTimings[src].load());
        for (int j = 0; j < maxSteps; ++j) laneValues[dest][j].store(laneValues[src][j].load());
    }
    if (getOptionalLaneType(baseLanes + count - 1) == 3)
        for (int step = 0; step < maxSteps; ++step) setVibratoDepth(step, 0);
    setCustomCCLabel(count - 1, {});
    setLaneName(count - 1 + baseLanes, {});
    setLaneStartStep(count - 1 + baseLanes, 0);
    setMacroType(count - 1, 0);
    laneTypes[count - 1].store(0);
    setMacroChannel(count - 1, 0);
    for (int j = 0; j < maxSteps; ++j)
    {
        setCCTransitionMs(count - 1, j, 0);
        setOptionalGlide(baseLanes + count - 1, j, 0);
    }
    setCustomCCCount(count - 1);
}

int PolyrhythmMIDIProcessor::getLaneTiming(int row) const noexcept
{
    return row>=0 && row<laneCount && hostedTimings[row]!=nullptr
        ? PnordeSafeAutomation::truncate(hostedTimings[row]->load(std::memory_order_relaxed), 0, 2, 0) : 0;
}
void PolyrhythmMIDIProcessor::setLaneTiming(int row, int mode) noexcept
{
    if (row<0 || row>=laneCount) return;
    mode=juce::jlimit(0,2,mode);
    laneTimings[row].store(mode,std::memory_order_relaxed);
    const auto id="pn_lane_timing_"+juce::String(row);
    if (auto* param=parameters.getParameter(id))
        param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(mode)));
}

int PolyrhythmMIDIProcessor::getLaneSwing(int row) const noexcept
{
    return row >= 0 && row < laneCount && hostedSwings[row] != nullptr
        ? PnordeSafeAutomation::round(hostedSwings[row]->load(std::memory_order_relaxed),
            PnordeSwing::straight, PnordeSwing::maximum, PnordeSwing::straight)
        : PnordeSwing::straight;
}
void PolyrhythmMIDIProcessor::setLaneSwing(int row, int percent) noexcept
{
    if (row < 0 || row >= laneCount) return;
    const auto id = "pn_lane_swing_" + juce::String(row);
    if (auto* parameter = parameters.getParameter(id))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(
            juce::jlimit(PnordeSwing::straight, PnordeSwing::maximum, percent))));
}

int PolyrhythmMIDIProcessor::getLaneCurrentStep(int row) const noexcept
{
    if (row < 0 || row >= laneCount) return -1;
    return laneCurrentSteps[row].load(std::memory_order_relaxed);
}

// ============================================================================
// PROCESS BLOCK: THE REAL-TIME HEART OF PNORDE
// ============================================================================
// Ableton calls this again and again with a small number of audio samples.
// PNORDE produces MIDI, not sound. This function calculates the note and CC
// messages that fall INSIDE this particular sample block.
//
//   audio         = sound buffer (we clear it because PNORDE makes no audio)
//   midiMessages  = container for MIDI messages sent back to Ableton
//   numSamples    = how many audio samples are in this callback
//
// VERY IMPORTANT: this is the AUDIO THREAD. Avoid locks, reading files,
// resizing vectors, long loops and UI operations here. If it waits too long,
// Ableton can crackle, stop responding or miss scheduled Program changes.
//
// The 'emit' lambda below is just a tiny local function: it schedules one
// message at a specific sample offset, preserving MIDI timing precision.
// Monotonic timestamps are for debugging ONLY; never schedule MIDI from
// wall-clock time. Host musical position remains the timing authority.
static std::uint64_t pnordeDevClockUs() noexcept
{
    using namespace std::chrono;
    return static_cast<std::uint64_t>(duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()).count());
}

// WHAT HAPPENS HERE: REAL-TIME CALLBACK: Ableton invokes this once per audio block. It calculates precisely timed MIDI event offsets. Never use blocking locks, dynamic file IO, UI calls or verbose logging here.
void PolyrhythmMIDIProcessor::processBlock(
    juce::AudioBuffer<float>& audio, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    // A stack-local guard records total callback duration, including early
    // returns. Destructor only touches atomics. There is no allocation.
    struct DeveloperAudioScope
    {
        PnordeDeveloperStats::Counters& counters;
        const bool active;
        const std::uint64_t startUs;
        std::uint64_t emitted = 0;
        explicit DeveloperAudioScope(PnordeDeveloperStats::Counters& c) noexcept
            : counters(c), active(c.enabled()),
              startUs(active ? pnordeDevClockUs() : 0) {}
        ~DeveloperAudioScope() noexcept
        {
            if (active)
                counters.recordAudioBlock(pnordeDevClockUs() - startUs, emitted);
        }
    } devScope(developerStats);
    audio.clear();
    const int numSamples = audio.getNumSamples();
    if (devScope.active)
    {
        developerBlockSize.store(numSamples, std::memory_order_relaxed);
        midiBlackBox.beginBlock(numSamples);
        const unsigned programMarker = midiTraceProgramMarker.exchange(0, std::memory_order_acq_rel);
        if (programMarker != 0)
            midiBlackBox.markProgram(static_cast<int>((programMarker >> 8) & 0xff) - 1,
                                     static_cast<int>(programMarker & 0xff) - 1);
    }
    if (numSamples <= 0) return;

    const bool external = isExternalMidiOutputActive();
    const double blockStartMs = juce::Time::getMillisecondCounterHiRes();
    const auto emit = [&](const juce::MidiMessage& message, int offset)
    {
        const int sample = juce::jlimit(0, numSamples - 1, offset);
        if (devScope.active)
        {
            ++devScope.emitted;
            midiBlackBox.recordMidi(message.getRawData(), message.getRawDataSize(), sample);
        }
        if (message.isPitchWheel())
        {
            lastPitchWheel.store(message.getPitchWheelValue(), std::memory_order_relaxed);
            pitchWheelSends.fetch_add(1, std::memory_order_relaxed);
        }
        if (external)
            externalMidiWorker->enqueue(message,
                blockStartMs + 1000.0 * sample / juce::jmax(1.0, currentSampleRate));
        else
            midiMessages.addEvent(message, sample);
    };
    if (panicRequested.exchange(false))
    {
        for (int ch = 1; ch <= 16; ++ch)
        {
            emit(juce::MidiMessage::controllerEvent(ch, 64, 0), 0);
            emit(juce::MidiMessage::controllerEvent(ch, 123, 0), 0);
            emit(juce::MidiMessage::controllerEvent(ch, 120, 0), 0);
            emit(juce::MidiMessage::pitchWheel(ch, 8192), 0);
        }
        noteIsActive = false;
        dedicatedSent.fill(-1); dedicatedSentChannel.fill(-1);
        dedicatedTargetStep.fill(-1); dedicatedController.fill(-1);
        lastCCSent.fill(-1);
    }
    const auto noteOff = [&](int offset)
    {
        if (!noteIsActive) return;
        if (getExperimentalPolyPressure() && getMicroOutputMode() == 1)
            emit(juce::MidiMessage::aftertouchChange(activeChannel, activeNote, 0), offset);
        emit(juce::MidiMessage::noteOff(activeChannel, activeNote), offset);
        noteIsActive = false;
        // No bend is left on an inactive voice. This is emitted after note-off
        // so the released voice does not change pitch before it ends.
        if (activeBend != 8192)
        {
            emit(juce::MidiMessage::pitchWheel(activeChannel, 8192), offset);
            activeBend = 8192;
        }
    };
    const auto reset = [&]()
    {
        noteOff(0);
        if (activeBend != 8192) emit(juce::MidiMessage::pitchWheel(activeChannel, 8192), 0);
        emit(juce::MidiMessage::channelPressureChange(activeChannel, 0), 0);
        activeBend = 8192;
        lastPressureSent = -1;
        lastAftertouchStep = -1;
        lastSlide = -1;
        hadPreviousPitch = false;
        legatoVoice = false;
        lastCCSent.fill(-1); lastCCStep.fill(-1); lastCCNumber.fill(-1);
    lastCCEmitPpq.fill(-1.0e12);
        dedicatedSent.fill(-1); dedicatedSentChannel.fill(-1);
        dedicatedTargetStep.fill(-1); dedicatedController.fill(-1);
        nextModulationPpq = 0.0;
        previousBlockWasPlaying = false;
        previousManualMode = false;
        globalReversePhase.reset(getGlobalReverse());
        mpeZoneConfigured = false;
        previousMicroMode = -1;
        hostPlaying.store(false, std::memory_order_relaxed);
        hostPpq.store(retainedBeatPpq, std::memory_order_relaxed);
        hostClockUpdateMs.store(0.0, std::memory_order_relaxed);
        // Do not wipe the lane positions. Pausing must preserve each polymetric phase.
    };

    // Trigger counters are absolute host-visible parameters; each change is
    // a restart impulse (suitable for stepped automation in Ableton).
    // Consume while paused too, so RESET immediately sets visual positions to 1.
    const int globalReset = getResetCounter();
    if (globalReset != previousResetCounter)
    {
        previousResetCounter = globalReset;
        reset();
        retainedBeatPpq = 0.0;
        for (int row = 0; row < laneCount; ++row)
        {
            lanePhaseOrigins[row] = 0.0;
            publishedLanePhaseOrigins[row].store(0.0, std::memory_order_relaxed);
            laneCurrentSteps[row].store(getLaneStartStep(row), std::memory_order_relaxed);
        }
        currentStep.store(getLaneStartStep(0), std::memory_order_relaxed);
        hostPpq.store(0.0, std::memory_order_relaxed);
    }
    // Individual restarts are applied below when audio starts, or immediately
    // here when paused. The actual phase-origin adjustment is sample-block exact.
    const bool canAdvance = getPlayEnabled();
    if (!canAdvance)
    {
        for (int row = 0; row < laneCount; ++row)
        {
            const int counter = getLaneResetCounter(row);
            if (counter == previousLaneResetCounters[row]) continue;
            previousLaneResetCounters[row] = counter;
            lanePhaseOrigins[row] = retainedBeatPpq;
            publishedLanePhaseOrigins[row].store(retainedBeatPpq, std::memory_order_relaxed);
            laneCurrentSteps[row].store(getLaneStartStep(row), std::memory_order_relaxed);
            if (row == 0) currentStep.store(getLaneStartStep(0), std::memory_order_relaxed);
        }
    }
    if (!canAdvance)
    {
        if (previousPlayEnabled)
        {
            reset();
            if (devScope.active) midiBlackBox.markStop(1); // PNORDE Play OFF
        }
        previousPlayEnabled = false;
        return;
    }
    previousPlayEnabled = true;
    // Host stop can span thousands of audio callbacks. Reset MIDI just once
    // instead of emitting redundant pressure resets every silent buffer.
    const auto stopIfNecessary = [&]()
    {
        if (previousBlockWasPlaying || noteIsActive)
        {
            reset();
            if (devScope.active) midiBlackBox.markStop(2); // Ableton transport stop
        }
        else hostPlaying.store(false, std::memory_order_relaxed);
    };
    const auto* playhead = getPlayHead();
    if (playhead == nullptr) { stopIfNecessary(); return; }
    const auto position = playhead->getPosition();
    if (!position.hasValue() || !position->getIsPlaying())
    {
        // Keep user lane resets responsive even when the DAW is stopped.
        for (int row = 0; row < laneCount; ++row)
        {
            const int counter = getLaneResetCounter(row);
            if (counter == previousLaneResetCounters[row]) continue;
            previousLaneResetCounters[row] = counter;
            lanePhaseOrigins[row] = retainedBeatPpq;
            publishedLanePhaseOrigins[row].store(retainedBeatPpq, std::memory_order_relaxed);
            laneCurrentSteps[row].store(getLaneStartStep(row), std::memory_order_relaxed);
            if (row == 0) currentStep.store(getLaneStartStep(0), std::memory_order_relaxed);
        }
        stopIfNecessary(); return;
    }
    const auto ppq = position->getPpqPosition();
    const auto bpm = position->getBpm();
    if (!ppq.hasValue() || !bpm.hasValue() || !std::isfinite(*ppq) ||
        std::abs(*ppq) > 1.0e12 || !std::isfinite(*bpm) || *bpm > 1000.0 ||
        *bpm <= 0.0 || !std::isfinite(currentSampleRate) || currentSampleRate <= 0.0)
    { stopIfNecessary(); return; }

    constexpr double divisions[] {0.125, 0.25, 0.5, 1.0, 2.0, 4.0,
                                   8.0, 16.0, 32.0, 64.0, 128.0};
    constexpr double multipliers[] {4.0, 2.0, 1.0, 0.5, 0.25};
    constexpr double timing[] {1.0, 2.0 / 3.0, 1.5};
    constexpr int bendRanges[] {2, 12, 24, 48};
    const bool microtonal = isMicrotonalScaleSelected();
    const bool mpe = PnordeMicroOutput::mpeEnabled(microtonal, getMicroOutputMode());
    // +/-2 is forced for all microtonal scales, including MPE member channel 2.
    // The saved pn_bend parameter is *not* changed (older projects remain intact).
    const int bendRange = PnordeMicroOutput::bendRange(microtonal,
        getMicroOutputMode(), bendRanges[getBendRangeIndex()]);
    const double clockMultiplier = multipliers[getGlobalClockIndex()];
    const auto laneDuration = [&](int row)
    {
        return divisions[getLaneDivision(row)] * timing[getLaneTiming(row)] * clockMultiplier;
    };
    const auto beats = PnordeManualTempo::advance({
        *ppq, *bpm, getManualTempoBpm(), currentSampleRate, numSamples,
        getManualTempoEnabled(), previousBlockWasPlaying, previousManualMode,
        previousHostBlockEndPpq, manualBlockEndPpq
    });
    const double effectiveBpm = beats.bpm;
    const double perSample = effectiveBpm / (60.0 * currentSampleRate);
    // Independent local beat clock: Ableton can stop, pause or seek without
    // moving the retained lane phases. Tempo changes still take effect immediately.
    const double start = retainedBeatPpq;
    const double end = PnordeRetainedTransport::endOfBlock(start, effectiveBpm, numSamples, currentSampleRate);
    retainedBeatPpq = end;
    // Dedicated PROGRAM lane is independent of the 21 musical lanes and
    // continues across scene changes. Trigger at the END of a cell; publish
    // ONLY a tiny atomic request, never XML or locks on the audio thread.
    // Message thread commits the bank at its next timer tick (~8.3 ms at 120 Hz).
    if (programLaneRestartRequested.exchange(false, std::memory_order_acq_rel))
    {
        // Message/host state thread never writes the audio thread's doubles.
        programLaneBeat = 0.0;
        programLaneOrigin = 0.0;
        lastProgramLaneBoundary = -1.0;
    }
    const double progStart = programLaneBeat;
    const double progEnd = progStart + (end - start);
    programLaneBeat = progEnd;
    const double programDuration = divisions[getProgramLaneDivision()] *
        timing[getProgramLaneTiming()] * clockMultiplier;
    if (getProgramLaneEnabled() && std::isfinite(programDuration) && programDuration > 1.0e-9)
    {
        const auto firstTick = PnordeRetainedTransport::nextBoundaryTick(progStart,
            programLaneOrigin, programDuration);
        const auto lastTick = PnordeRetainedTransport::nextBoundaryTick(progEnd,
            programLaneOrigin, programDuration);
        // Bound iteration for pathological host blocks or tempo values.
        for (auto t = firstTick; t < lastTick && t < firstTick + 256; ++t)
        {
            const double at = PnordeRetainedTransport::boundaryAt(t,
                programLaneOrigin, programDuration);
            if (t <= 0 || at < progStart - 1.0e-9 || at >= progEnd - 1.0e-9 ||
                at <= lastProgramLaneBoundary + 1.0e-9) continue;
            lastProgramLaneBoundary = at;
            const int outgoing = PnordeProgramLane::completedIndex(
                t, getProgramLaneLength(), getProgramLaneStart());
            const int source = PnordeRotation::sourceIndex(outgoing,
                getProgramLaneLength(), getProgramLaneRotation());
            const int slot = getProgramLaneValue(source);
            // PROGRAM probability is checked once at the completed cell
            // boundary. A failed roll means NO program request is sent.
            if (slot >= 1 && slot <= programCount &&
                PnordeProbability::passes(getCellProbability(laneCount, source),
                                 laneCount, t - 1))
            {
                if (devScope.active)
                    programRequestTimestampUs.store(pnordeDevClockUs(), std::memory_order_relaxed);
                const auto previousRequest = pendingSequencedProgram.exchange(
                    PnordeProgramRequests::pack(
                        manualProgramEpoch.load(std::memory_order_acquire), slot),
                    std::memory_order_acq_rel);
                developerStats.recordRequest(previousRequest != 0);
            }
        }
        const auto currentTick = PnordeRetainedTransport::tickAt(progEnd,
            programLaneOrigin, programDuration);
        const int len = getProgramLaneLength();
        programLaneCurrent.store(PnordeProgramLane::activeIndex(
            currentTick, len, getProgramLaneStart()), std::memory_order_relaxed);
        const double distance = (progEnd - programLaneOrigin) / programDuration;
        programLaneProgress.store(static_cast<float>(juce::jlimit(0.0, 1.0,
            distance - std::floor(distance))), std::memory_order_relaxed);
    }
    for (int row = 0; row < laneCount; ++row)
    {
        const int counter = getLaneResetCounter(row);
        if (counter == previousLaneResetCounters[row]) continue;
        previousLaneResetCounters[row] = counter;
        lanePhaseOrigins[row] = start;
        globalReversePhase.resetLane(row);
        probabilityLastTick[static_cast<size_t>(row)] = std::numeric_limits<std::int64_t>::min();
        probabilityHeldStep[static_cast<size_t>(row)] = getLaneStartStep(row);
        publishedLanePhaseOrigins[row].store(start, std::memory_order_relaxed);
        laneCurrentSteps[row].store(getLaneStartStep(row), std::memory_order_relaxed);
        if (row == 0)
        {
            // Re-phasing SEQ also ends its currently sounding note safely.
            noteOff(0);
            currentStep.store(getLaneStartStep(0), std::memory_order_relaxed);
            nextModulationPpq = start;
            hadPreviousPitch = false;
        }
        lastCCStep.fill(-1);
        dedicatedTargetStep.fill(-1);
    }
    manualBlockEndPpq = beats.end;
    previousHostBlockEndPpq = beats.hostEnd;
    previousManualMode = beats.manualMode;
    constexpr double eps = 1.0e-9;
    hostPlaying.store(true, std::memory_order_relaxed);
    hostPpq.store(start, std::memory_order_relaxed);
    hostClockUpdateMs.store(blockStartMs, std::memory_order_relaxed);
    hostBpm.store(effectiveBpm, std::memory_order_relaxed);
    if (previousBlockWasPlaying && std::abs(start - previousBlockEndPpq) >
        juce::jmax(0.01, perSample * numSamples * 2.0))
    {
        noteOff(0);
        hadPreviousPitch = false;
        lastCCSent.fill(-1); lastCCStep.fill(-1); lastCCNumber.fill(-1);
    lastCCEmitPpq.fill(-1.0e12);
        dedicatedSent.fill(-1); dedicatedSentChannel.fill(-1);
        dedicatedTargetStep.fill(-1); dedicatedController.fill(-1);
        nextModulationPpq = start;
        glideStartPpq = glideEndPpq = start;
        pressureStartPpq = pressureEndPpq = start;
    }
    // Live global reversal: preserve each lane's currently sounding cell by
    // reflecting its independent tick coordinate at this block boundary.
    // Applied once per block; host automation is block-quantised, never MIDI CC.
    {
        std::array<std::int64_t, laneCount> currentTicks {};
        for (int row = 0; row < laneCount; ++row)
            currentTicks[static_cast<size_t>(row)] = PnordeSwing::tickAt(
                start, lanePhaseOrigins[row], laneDuration(row), getLaneSwing(row));
        globalReversePhase.update(getGlobalReverse(), currentTicks);
    }
    previousBlockWasPlaying = true;
    previousBlockEndPpq = end;

    const auto offsetAt = [&](double at)
    {
        return juce::jlimit(0, numSamples - 1,
            static_cast<int>(std::ceil((at - start) / perSample - eps)));
    };
    const auto stepAt = [&](int row, double at)
    {
        const auto tick = PnordeSwing::tickAt(
            at, lanePhaseOrigins[row], laneDuration(row), getLaneSwing(row));
        const auto position = globalReversePhase.position(row, tick);
        const int selected = PnordePlayback::indexWithStart(position,
            getLaneLength(row), getLaneDirection(row), getLaneStartStep(row), row);
        // A modulation lane may be queried 200 times per second, but it
        // makes ONE probability decision per lane-clock tick. On failure it
        // returns the last successfully accepted step: sample-and-hold.
        // Row zero (the note gate) has separate skip-trigger semantics.
        // selected = VISIBLE clock location, source = ORIGINAL stored cell.
        // Use source for BOTH the value and probability. They must never
        // separate when a pattern is rotated.
        const int source = PnordeRotation::sourceIndex(selected,
            getLaneLength(row), getLaneRotation(row));
        if (row == 0) return source;
        const auto r = static_cast<size_t>(row);
        if (probabilityLastTick[r] != tick)
        {
            probabilityLastTick[r] = tick;
            if (PnordeProbability::passes(getCellProbability(row, source), row, tick))
                probabilityHeldStep[r] = source;
        }
        return probabilityHeldStep[r];
    };
    const auto interpolated = [](double at, double t0, double t1, double from, double to)
    {
        if (t1 <= t0 || at >= t1) return to;
        if (at <= t0) return from;
        const double f = juce::jlimit(0.0, 1.0, (at - t0) / (t1 - t0));
        return from + (to - from) * f;
    };
    const auto bendFor = [&](double semis)
    {
        const double delta = juce::jlimit(-static_cast<double>(bendRange),
                                           static_cast<double>(bendRange), semis - activeNote);
        return juce::jlimit(0, 16383,
            static_cast<int>(std::lround(8192.0 + delta * 8191.0 / bendRange)));
    };
    // At regular short intervals this updates expression: custom MIDI CCs,
    // aftertouch, pitch bend and the optional dedicated controller lanes.
    // A 'lambda' is a helper function written inside another function.
    // '[&]' lets it reuse local variables such as 'effectiveBpm' and 'emit'.
    const auto sendModulation = [&](double at)
    {
        const int offset = offsetAt(at);
        // All optional parameter rows use the same PnordeLane model.
        // The MIDI scheduler stays on the audio thread: no dynamic allocation,
        // text lookup or locks when switching between lane roles.
        const int ch = noteIsActive ? activeChannel : getDefaultMidiChannel();
        for (int row = baseLanes; row < laneCount; ++row)
        {
            if (!isOptionalLaneEnabled(row)) continue;
            const int slot = row - baseLanes;
            const int type = getOptionalLaneType(row);
            if (type != lastOptionalType[slot])
            {
                lastOptionalType[slot] = type;
                lastCCSent[slot] = lastCCStep[slot] = lastCCNumber[slot] = -1;
                dedicatedSent[slot * 2] = dedicatedSent[slot * 2 + 1] = -1;
                dedicatedSentChannel[slot * 2] = dedicatedSentChannel[slot * 2 + 1] = -1;
                dedicatedTargetStep[slot * 2] = dedicatedTargetStep[slot * 2 + 1] = -1;
            }
            if (type == 6) continue; // MIDI Channel Select affects note triggers.
            const int step = stepAt(row, at);
            const int vertical = getLaneValue(row, step);
            if (type == 0)
            {
                const int number = getCustomCCNumber(slot);
                const double target = juce::jlimit(0.0, 127.0, vertical * 127.0 / 100.0);
                const auto valueAt = [&](double t)
                {
                    return interpolated(t, ccStartPpq[slot], ccEndPpq[slot],
                                        ccFrom[slot], ccTo[slot]);
                };
                if (step != lastCCStep[slot] || target != ccTo[slot] ||
                    ch * 128 + number != lastCCNumber[slot])
                {
                    const double current = lastCCSent[slot] < 0 ? target : valueAt(at);
                    ccFrom[slot] = current;
                    ccTo[slot] = target;
                    ccStartPpq[slot] = at;
                    ccEndPpq[slot] = at + juce::jmin(10.0 * (effectiveBpm / 60.0),
                        laneDuration(row) * getCCTransitionMs(slot, step) / 100.0);
                    lastCCStep[slot] = step;
                    if (ch * 128 + number != lastCCNumber[slot]) lastCCSent[slot] = -1;
                    lastCCNumber[slot] = ch * 128 + number;
                }
                const int value = juce::jlimit(0, 127, static_cast<int>(std::lround(valueAt(at))));
                // ------------------------------------------------------------
                // CUSTOM MACRO MIDI FLOOD PROTECTION
                // ------------------------------------------------------------
                // The pitch-bend/aftertouch engine checks movement 200 times
                // per second. CC Macros don't need to send at that same rate:
                // hundreds of little changes from MANY lanes can clog the
                // host MIDI stream or a physical MIDI output device.
                //
                // Keep the original high-resolution glide calculation, but
                // transmit at most about 60 changes/second per Macro inside
                // Ableton (30/second for a potentially slower hardware port).
                // First values and changes of CC/channel routing still send
                // immediately; repeated values are already de-duplicated.
                // If the BPM changes, the interval converts real seconds to
                // beats, because 'at' is measured in quarter-note beats.
                const double ccIntervalBeats = effectiveBpm /
                    (60.0 * (external ? 30.0 : 60.0));
                const double previous = lastCCEmitPpq[slot];
                const bool clockReset = at < previous - 1.0e-8;
                if (value != lastCCSent[slot] &&
                    (lastCCSent[slot] < 0 || clockReset ||
                     at >= previous + ccIntervalBeats - 1.0e-8))
                {
                    emit(juce::MidiMessage::controllerEvent(ch, number, value), offset);
                    lastCCSent[slot] = value;
                    lastCCEmitPpq[slot] = at;
                }
                continue;
            }
            const auto emitDedicated = [&](int axis, int cc, int target)
            {
                target = juce::jlimit(0, 127, target);
                const int index = slot * 2 + axis;
                const bool changedRoute = dedicatedController[index] != cc || dedicatedSentChannel[index] != ch;
                const bool changedStep = dedicatedTargetStep[index] != step;
                if (changedRoute || changedStep || std::abs(dedicatedTo[index] - target) > 0.01)
                {
                    const double previous = (changedRoute || dedicatedTargetStep[index] < 0)
                        ? static_cast<double>(target)
                        : interpolated(at, dedicatedStartPpq[index], dedicatedEndPpq[index],
                                       dedicatedFrom[index], dedicatedTo[index]);
                    dedicatedFrom[index] = previous;
                    dedicatedTo[index] = target;
                    dedicatedStartPpq[index] = at;
                    dedicatedEndPpq[index] = at + (type == 5 ? 0.0
                        : PnordeTempoGlide::durationBeats(getOptionalGlide(row, step), laneDuration(row)));
                    dedicatedTargetStep[index] = step;
                    dedicatedController[index] = cc;
                    if (changedRoute) dedicatedSent[index] = -1;
                }
                const int value = juce::jlimit(0, 127, static_cast<int>(std::lround(
                    interpolated(at, dedicatedStartPpq[index], dedicatedEndPpq[index],
                                 dedicatedFrom[index], dedicatedTo[index]))));
                if (dedicatedSent[index] != value || dedicatedSentChannel[index] != ch)
                {
                    emit(juce::MidiMessage::controllerEvent(ch, cc, value), offset);
                    dedicatedSent[index] = value;
                    dedicatedSentChannel[index] = ch;
                }
            };
            // STAGE 5 MAINTAINABILITY: One MIDI controller-number definition.
            //
            // Previously these six numbers (1, 11, 76, 77, 10, 64) were
            // typed a SECOND time here even though PnordeLane::cc() already
            // defined them for the editor. Now UI and audio code agree by
            // construction. This does NOT change the targets, output values,
            // message order, update rate, or glide calculations.
            //
            // `Type::` picks a named lane role. `cc(...)` returns the existing
            // controller number as an ordinary integer (no allocation/lock).
            if (type == 1)
                emitDedicated(0, PnordeLane::cc(PnordeLane::Type::modWheel), juce::roundToInt(vertical * 1.27f));
            else if (type == 2)
                emitDedicated(0, PnordeLane::cc(PnordeLane::Type::expression), juce::roundToInt(vertical * 1.27f));
            else if (type == 3)
            {
                emitDedicated(0, PnordeLane::cc(PnordeLane::Type::vibrato), juce::roundToInt(vertical * 1.27f));
                emitDedicated(1, PnordeLane::vibratoDepthCC, juce::roundToInt(getVibratoDepth(step) * 1.27f));
            }
            else if (type == 4)
                emitDedicated(0, PnordeLane::cc(PnordeLane::Type::pan), vertical);
            else if (type == 5)
                emitDedicated(0, PnordeLane::cc(PnordeLane::Type::sustain), vertical >= 50 ? 127 : 0);
        }
        if (!noteIsActive) return;
        const double semis = interpolated(at, glideStartPpq, glideEndPpq,
                                          glideFromSemis, glideToSemis);
        const int bend = bendFor(semis);
        if (bend != activeBend)
        {
            emit(juce::MidiMessage::pitchWheel(activeChannel, bend), offset);
            activeBend = bend;
        }
        const int afterStep = stepAt(6, at);
        if (afterStep != lastAftertouchStep)
        {
            const double current = interpolated(at, pressureStartPpq,
                pressureEndPpq, pressureFrom, pressureTo);
            pressureFrom = current;
            pressureTo = getLaneValue(6, afterStep) * 127.0 / 100.0;
            pressureStartPpq = at;
            // Negative sentinel restores v0.5.4's automatic timing in old projects.
            pressureEndPpq = at + PnordeAftertouchGlide::durationBeats(
                getAftertouchGlide(afterStep), effectiveBpm, laneDuration(6));
            lastAftertouchStep = afterStep;
        }
        const int pressure = juce::jlimit(0, 127, static_cast<int>(std::lround(
            interpolated(at, pressureStartPpq, pressureEndPpq, pressureFrom, pressureTo))));
        if (pressure != lastPressureSent)
        {
            // MPE pressure is per-member-channel Channel Pressure. Polyphonic
            // Key Pressure is a distinct, optional compatibility event.
            emit(juce::MidiMessage::channelPressureChange(activeChannel, pressure), offset);
            // Independent optional Poly Key Pressure, not an MPE synonym.
            // Standard channel pressure remains the default for compatibility.
            if (getExperimentalPolyPressure())
                emit(juce::MidiMessage::aftertouchChange(activeChannel, activeNote, pressure), offset);
            lastPressureSent = pressure;
        }

    };

    // MPE lower zone consists of master channel 1 and a single member channel 2.
    // Configure zone and member bend range only on mode entry, never every block.
    const int microModeNow = mpe ? 1 : 0;
    if (previousMicroMode != microModeNow)
    {
        noteOff(0);
        mpeZoneConfigured = false;
        previousMicroMode = microModeNow;
        lastCCSent.fill(-1);
        dedicatedSent.fill(-1);
        lastPressureSent = -1;
    }
    if (mpe && !mpeZoneConfigured)
    {
        // RPN 0,6: Lower MPE zone with one member; RPN 0,0: member bend
        // range. Both PNORDE and the receiving MPE synth must use +/-2.
        for (const auto message : {juce::MidiMessage::controllerEvent(1, 101, 0),
                                   juce::MidiMessage::controllerEvent(1, 100, 6),
                                   juce::MidiMessage::controllerEvent(1, 6, 1),
                                   juce::MidiMessage::controllerEvent(1, 101, 127),
                                   juce::MidiMessage::controllerEvent(1, 100, 127),
                                   juce::MidiMessage::controllerEvent(2, 101, 0),
                                   juce::MidiMessage::controllerEvent(2, 100, 0),
                                   juce::MidiMessage::controllerEvent(2, 6, 2),
                                   juce::MidiMessage::controllerEvent(2, 38, 0),
                                   juce::MidiMessage::controllerEvent(2, 101, 127),
                                   juce::MidiMessage::controllerEvent(2, 100, 127)})
            emit(message, 0);
        mpeZoneConfigured = true;
    }

    const double seqDuration = laneDuration(0);
    const int seqLength = getLaneLength(0);
    const double seqOrigin = lanePhaseOrigins[0];
    // Re-read swing at each block (host automation is block-granular). Only
    // odd SEQ boundaries move; incoming note events stay sample-accurate.
    const int seqSwing = getLaneSwing(0);
    auto tick = PnordeSwing::nextBoundaryTick(start, seqOrigin, seqDuration, seqSwing);
    double boundary = PnordeSwing::boundaryAt(tick, seqOrigin, seqDuration, seqSwing);
    // 200 Hz bend/pressure sampling; event offsets remain sample-accurate.
    const double modInterval = juce::jmax(perSample, effectiveBpm / 12000.0);
    if (nextModulationPpq < start || nextModulationPpq > end + 1.0)
        nextModulationPpq = start;

    // Merge trigger, gate-off and modulation events chronologically.
    // A malformed host block, extreme tempo automation or scheduler regression
    // must not spin for 200,000 iterations on the real-time audio thread.
    const int iterationBudget = juce::jlimit(128, 32768,
        juce::jmin(numSamples, 4096) * 8);
    int guard = 0;
    while (++guard < iterationBudget)
    {
        const double offTime = noteIsActive ? noteOffPpq : end + 1.0;
        const double modTime = nextModulationPpq;
        const double nextTime = juce::jmin(boundary, juce::jmin(offTime, modTime));
        if (nextTime >= end - eps) break;
        // A full-length gate is handled at the next SEQ boundary so that
        // connected slides can retain their held MIDI note.
        if (offTime <= nextTime + eps && offTime < boundary - eps)
        {
            sendModulation(offTime);
            noteOff(offsetAt(offTime));
            if (offTime < nextModulationPpq + eps) nextModulationPpq = offTime + modInterval;
            continue;
        }
        if (boundary <= nextTime + eps)
        {
            const int offset = offsetAt(boundary);
            if (noteIsActive) sendModulation(boundary);
            const bool voiceAtBoundary = noteIsActive;
            const int displayed = PnordePlayback::indexWithStart(
                globalReversePhase.position(0, tick), seqLength,
                getLaneDirection(0), getLaneStartStep(0), 0);
            // Keep the playhead at the VISIBLE position; fetch gate ON/OFF
            // and probability from the rotated SOURCE position instead.
            const int index = PnordeRotation::sourceIndex(displayed,
                seqLength, getLaneRotation(0));
            currentStep.store(displayed, std::memory_order_relaxed);
            // A failed SEQ gate probability means skip this note trigger.
            // Unlike parameter lanes, we DON'T hold the previous gate ON:
            // that could accidentally retrigger or sustain a stuck note.
            if (getStepEnabled(index) &&
                PnordeProbability::passes(getCellProbability(0, index), 0, tick))
            {
                const int octave = getLaneValue(4, stepAt(4, boundary));
                const int rawPitch = getLaneValue(1, stepAt(1, boundary));
                // IMPORTANT: the PITCH cell is a NON-DESTRUCTIVE offset from
                // the current key, not an absolute note name. Every new note
                // is derived from rawPitch; key/scale changes never overwrite
                // that cell or accumulate rounding errors.
                //
                // Example: rawPitch=40 (+4), key=D (+2), scale=Minor:
                // nearest Minor degree to +4 is +3, so final offset=+5.
                // This calculation retains fractions for quartertones/EDO.
                const double pitchOffset = getEffectivePitchOffsetSemitones(rawPitch);
                const double targetSemis = juce::jlimit(0.0, 127.0,
                    (getLowPitchAnchor() ? 48.0 : 60.0)
                    + (octave + getGlobalOctave()) * 12.0 + pitchOffset);
                const int newNote = juce::jlimit(0, 127,
                    static_cast<int>(std::lround(targetSemis)));
                lastEDOOffsetCents.store(microtonal
                    ? static_cast<int>(std::lround(100.0 * (targetSemis - newNote))) : 0,
                    std::memory_order_relaxed);
                // SLIDE is never bypassed in EDO: it specifies the time to
                // approach the exact target pitch, in % of the gate duration.
                const int slide = getLaneValue(5, stepAt(5, boundary));
                int channelStep = 0;
                for (int optionalRow = baseLanes; optionalRow < laneCount; ++optionalRow)
                    if (isOptionalLaneEnabled(optionalRow) && getOptionalLaneType(optionalRow) == 6)
                    {
                        channelStep = getLaneValue(optionalRow, stepAt(optionalRow, boundary));
                        break;
                    }
                // Monophonic MPE lower-zone voice: member channel 2, master channel 1.
                // Existing note-channel lane is preserved and resumes outside MPE.
                const int newChannel = mpe ? 2 : channelStep == 0
                    ? getDefaultMidiChannel() : juce::jlimit(1, 16, channelStep);
                const double previousPitch = interpolated(boundary, glideStartPpq,
                    glideEndPpq, glideFromSemis, glideToSemis);
                const int gate = getLaneValue(3, stepAt(3, boundary));
                // Legato is possible only if the previous note is still held,
                // its outgoing gate actually reaches this boundary, and the
                // entire new target is representable around that MIDI note.
                const bool connected = voiceAtBoundary && noteOffPpq >= boundary - eps;
                const bool canLegato = connected && activeChannel == newChannel && slide > 0
                    && std::abs(targetSemis - activeNote) <= bendRange;
                if (!canLegato)
                {
                    const bool routeChanged = (activeChannel != newChannel);
                    if (noteIsActive) noteOff(offset);
                    activeNote = newNote;
                    activeChannel = newChannel;
                    lastCCSent.fill(-1); // Re-send macros on the new note.
                    dedicatedSent.fill(-1); // Re-send dedicated CC at current interpolated position.
                    if (routeChanged)
                    {
                        dedicatedSentChannel.fill(-1);
                        dedicatedTargetStep.fill(-1);
                        dedicatedController.fill(-1);
                    }
                    lastPressureSent = -1;
                }
                // Retrigger: anchor the new MIDI note at the destination and
                // bend back toward the previous pitch. Legato: retain the old
                // MIDI note number and move its bend continuously.
                // When the interval exceeds the available range, clamp the
                // starting pitch (no false promise of a full-range glide).
                glideFromSemis = slide > 0 && hadPreviousPitch
                    ? juce::jlimit(static_cast<double>(activeNote - bendRange),
                                   static_cast<double>(activeNote + bendRange), previousPitch)
                    : targetSemis;
                glideToSemis = targetSemis;
                glideStartPpq = boundary;
                // SLIDE: 0% = instant; 100% = reach the microtonal target
                // at note-off. Tempo-based beats, so BPM changes affect it.
                const double swungSpan = PnordeSwing::stepSpan(
                    tick, seqOrigin, seqDuration, seqSwing);
                glideEndPpq = boundary + PnordeMicroOutput::slideApproachBeats(
                    slide, swungSpan, gate);
                lastPitchSemis = targetSemis;
                const int velocity = juce::jlimit(1, 127, static_cast<int>(std::lround(
                    getLaneValue(2, stepAt(2, boundary)) * 127.0 / 100.0)));
                const int bend = bendFor(glideFromSemis);
                if (bend != activeBend || !canLegato)
                    emit(juce::MidiMessage::pitchWheel(activeChannel, bend), offset);
                activeBend = bend;
                if (!canLegato)
                    emit(juce::MidiMessage::noteOn(activeChannel, activeNote,
                         static_cast<juce::uint8>(velocity)), offset);
                noteIsActive = true;
                hadPreviousPitch = true;
                legatoVoice = canLegato;
                // Relative gate length follows the actual time to the next
                // swung onset, so 100% gates end precisely at that boundary.
                noteOffPpq = boundary + swungSpan * gate / 100.0;
                lastAftertouchStep = -1; // Begin pressure ramp from current value on the new voice.
                nextModulationPpq = boundary;
            }
            else if (noteIsActive) noteOff(offset);
            ++tick;
            boundary = PnordeSwing::boundaryAt(tick, seqOrigin, seqDuration, seqSwing);
            continue;
        }
        if (modTime <= nextTime + eps)
        {
            sendModulation(modTime);
            nextModulationPpq += modInterval;
            continue;
        }
        break;
    }
    if (guard >= iterationBudget)
    {
        // Fail closed rather than hanging the DAW or leaving a voice held.
        noteOff(numSamples - 1);
        nextModulationPpq = end;
        lastCCStep.fill(-1);
        dedicatedTargetStep.fill(-1);
    }
    const double lastPpq = start + (numSamples - 1) * perSample;
    for (int row = 0; row < laneCount; ++row)
    {
        const int source = stepAt(row, lastPpq);
        // Publish the POSITION the user sees, not the source-cell index.
        // This keeps the glowing playhead at the correct rotated slot.
        const int visible = PnordeRotation::visibleIndex(source,
            getLaneLength(row), getLaneRotation(row));
        laneCurrentSteps[row].store(visible, std::memory_order_relaxed);
        if (row == 0) currentStep.store(visible, std::memory_order_relaxed);
    }
}

juce::AudioProcessorEditor*
PolyrhythmMIDIProcessor::createEditor()
{
    return new PolyrhythmMIDIEditor(*this);
}

// WHAT HAPPENS HERE: QUICK PROGRAM SAVE: Read current editable state into the outgoing slot before another Quick Program replaces it.
void PolyrhythmMIDIProcessor::captureCurrentProgram(
    juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // Save all 256 gates as a compact string of 0s and 1s.
    juce::String patternData;

    for (int i = 0; i < static_cast<int>(pattern.size()); ++i)
        patternData += getStepEnabled(i) ? "1" : "0";

    state.setProperty(
        "NovaPattern", patternData, nullptr);

    state.setProperty(
        "NovaSequenceLength", getSequenceLength(), nullptr);

    state.setProperty(
        "NovaClockDivision", getClockDivisionIndex(), nullptr);

    // New project-state version marker: keeps new instances Major while giving
    // genuinely old, unversioned projects their original Free 0.1 behaviour.
    // Save it with host state/presets so moving machines requires no registry.
    state.setProperty("PnordeStateSchema", PnordeProjectVersion::schema, nullptr);
    state.setProperty("PnordeSavedBuild", PnordeProjectVersion::build, nullptr);
    state.setProperty("PnordeSavedPluginVersion", PnordeProjectVersion::release, nullptr);
    // Each bank snapshot owns all PROGRAM lane musical data, so scene reset,
    // copy and ordinary preset save/load have independent trigger patterns.
    state.setProperty("PnordeProgramLaneLength", getProgramLaneLength(), nullptr);
    state.setProperty("PnordeProgramLaneDivision", getProgramLaneDivision(), nullptr);
    state.setProperty("PnordeProgramLaneTiming", getProgramLaneTiming(), nullptr);
    state.setProperty("PnordeProgramLaneStart", getProgramLaneStart(), nullptr);
    state.setProperty("PnordeProgramLaneRotation", getProgramLaneRotation(), nullptr);
    juce::String programValues;
    for (int n = 0; n < maxSteps; ++n)
    {
        if (n != 0) programValues += ",";
        programValues += juce::String(getProgramLaneValue(n));
    }
    state.setProperty("PnordeProgramLaneValues", programValues, nullptr);
    state.setProperty("PnordeHppdFps", getHppdFps(), nullptr);
    state.setProperty("PnordeHypnosis", getHypnosis(), nullptr);
    state.setProperty("PnordeVisualGlowEnabled", getVisualGlowEnabled(), nullptr);
    for (int row = 0; row < laneCount; ++row)
    {
        state.setProperty("PnordeLaneName" + juce::String(row), getLaneName(row), nullptr);
        state.setProperty("PnordeLaneStart" + juce::String(row), getLaneStartStep(row), nullptr);
        state.setProperty("PnordeLaneRotation" + juce::String(row), getLaneRotation(row), nullptr);
    }

    // Versioned lane data; older saved projects still load the original SEQ properties.
    state.setProperty("NovaLaneDataVersion", 13, nullptr);
    state.setProperty("PnordePitchAnchorLow", getLowPitchAnchor(), nullptr);
    state.setProperty("PnordeOctaveV2Used", parameters.getRawParameterValue("pn_octave_v2_active")->load()>=0.5f,nullptr);
    state.setProperty("PnordePsychedelicEnabled",getPsychedelicEnabled(),nullptr);
    state.setProperty("PnordePsychedelicIntensity",getPsychedelicIntensity(),nullptr);
    state.setProperty("PnordeVibesBleed",getVibesBleed(),nullptr);
    state.setProperty("PnordeVibesMode",getVibesMode(),nullptr);
    state.setProperty("PnordeAftertouchGlideUnits", 1, nullptr); // New tempo percentage units
    state.setProperty("PnordeExperimentalReorder", getExperimentalReorderEnabled(), nullptr);
    state.setProperty("PnordeMicroMode", getMicroOutputMode(), nullptr);
    state.setProperty("PnordePolyPressure", getExperimentalPolyPressure(), nullptr);
    juce::String visualOrder, pressureGlide;
    for (int i = 0; i < optionalCount; ++i)
    {
        if (i) visualOrder += ",";
        visualOrder += juce::String(getDisplayRowAt(i));
    }
    for (int i = 0; i < maxSteps; ++i)
    {
        if (i) pressureGlide += ",";
        pressureGlide += juce::String(getAftertouchGlide(i));
    }
    state.setProperty("PnordeVisualOrder", visualOrder, nullptr);
    state.setProperty("PnordeAftertouchGlide", pressureGlide, nullptr);
    state.setProperty("PnordeGlobalOctave", getGlobalOctave(), nullptr);
    state.setProperty("PnordeDefaultMidiChannel", getDefaultMidiChannel(), nullptr);
    state.setProperty("PnordeChannelLaneEnabled", hasMidiChannelLane(), nullptr);
    state.setProperty("PnordeTheme", getThemeIndex(), nullptr);
    state.setProperty("PnordeDarknessPercent", getDarknessPercent(), nullptr);
    state.setProperty("PnordeAnimations", getAnimationsEnabled(), nullptr);
    state.setProperty("PnordeScrollFades", getScrollFadesEnabled(), nullptr);
    state.setProperty("PnordeTooltips", getTooltipsEnabled(), nullptr);
    state.setProperty("PnordeExperimentalLights", getExperimentalLightsEnabled(), nullptr);
    for (int row = modRow; row <= sustainRow; ++row)
        state.setProperty("PnordeDedicated" + juce::String(row), isDedicatedEnabled(row), nullptr);
    juce::String vibDepth;
    for (int i = 0; i < maxSteps; ++i)
    {
        if (i) vibDepth += ",";
        vibDepth += juce::String(getVibratoDepth(i));
    }
    state.setProperty("PnordeVibratoDepth", vibDepth, nullptr);
    // Lane 0's direction also needs a saved readable value. Older snapshots
    // had no NovaLane0Direction property, so their default remains Forward.
    state.setProperty("NovaLane0Direction", getLaneDirection(0), nullptr);
    // Each scene stores its OWN per-cell probability grid. A missing entry
    // in an old preset means 100% on every cell (original behaviour).
    for (int row = 0; row < probabilityRows; ++row)
    {
        juce::String percentages;
        for (int step = 0; step < maxSteps; ++step)
        {
            if (step) percentages += ",";
            percentages += juce::String(getCellProbability(row, step));
        }
        state.setProperty("PnordeProbability" + juce::String(row), percentages, nullptr);
    }
    state.setProperty("NovaKey", getKey(), nullptr);
    state.setProperty("NovaScale", getScale(), nullptr);
    state.setProperty("NovaGlobalClock", getGlobalClockIndex(), nullptr);
    state.setProperty("NovaBendRange", getBendRangeIndex(), nullptr);
    state.setProperty("NovaCustomCCCount", getCustomCCCount(), nullptr);
    for (int i = 0; i < optionalCount; ++i)
    {
        state.setProperty("PnordeOptionalType" + juce::String(i), getOptionalLaneType(i + baseLanes), nullptr);
        state.setProperty("NovaCustomCC" + juce::String(i), getCustomCCNumber(i), nullptr);
        state.setProperty("PnordeMacroType" + juce::String(i), getMacroType(i), nullptr);
        state.setProperty("PnordeMacroChannel" + juce::String(i), getMacroChannel(i), nullptr);
        state.setProperty("NovaCustomCCLabel" + juce::String(i), getCustomCCLabel(i), nullptr);
        juce::String transitions;
        for (int j = 0; j < maxSteps; ++j)
        {
            if (j > 0) transitions += ",";
            transitions += juce::String(getCCTransitionMs(i, j));
        }
        state.setProperty("NovaCCGlidePercent" + juce::String(i), transitions, nullptr);
        juce::String dedicatedGlides;
        for (int j = 0; j < maxSteps; ++j)
        {
            if (j) dedicatedGlides += ",";
            dedicatedGlides += juce::String(getOptionalGlide(i + baseLanes, j));
        }
        state.setProperty("PnordeOptionalGlide" + juce::String(i), dedicatedGlides, nullptr);
    }
    for (int row = 1; row < laneCount; ++row)
    {
        const auto prefix = "NovaLane" + juce::String(row);
        state.setProperty(prefix + "Length", getLaneLength(row), nullptr);
        state.setProperty(prefix + "Division", getLaneDivision(row), nullptr);
        state.setProperty(prefix + "Timing", getLaneTiming(row), nullptr);
        state.setProperty(prefix + "Direction", getLaneDirection(row), nullptr);
        juce::String values;
        for (int i = 0; i < maxSteps; ++i)
        {
            if (i > 0) values += ",";
            values += juce::String(getLaneValue(row, i));
        }
        state.setProperty(prefix + "Values", values, nullptr);
    }

    state.setProperty(
        "NovaEditorWidth", getEditorWidth(), nullptr);

    state.setProperty(
        "NovaEditorHeight", getEditorHeight(), nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}



// The bounded XML-envelope checker is implemented below with the other
// native-preset validators. Forward-declare it for the internal bank path.
namespace { bool pnordeStateEnvelopeIsSafe(const void*, int, int) noexcept; }

// Performance appearance is intentionally GLOBAL. Host and preset project
// restore may change it, but selecting a musical program cannot.
bool PolyrhythmMIDIProcessor::restoreProgramSnapshot(
    const juce::MemoryBlock& snapshot, int destination,
    std::uint64_t* preparationUs, std::uint64_t* validatedLoadUs)
{
    // A timer is deliberately NOT started for ordinary switches with
    // diagnostics disabled. Scene parsing and loading are unchanged.
    const bool measure = preparationUs != nullptr && validatedLoadUs != nullptr;
    if (preparationUs) *preparationUs = 0;
    if (validatedLoadUs) *validatedLoadUs = 0;
    const auto preparationStartUs = measure ? pnordeDevClockUs() : 0;
    // Do NOT load a scene's old theme/darkness/HPPD parameters then change
    // them back. APVTS replacement may notify Live and momentarily render the
    // wrong scene appearance, even when the final values seem correct.
    // Instead overwrite visual-only fields in a PRIVATE snapshot before
    // the normal validated loader sees it. Musical fields remain untouched.
    // The internal bank still stores normal bounded JUCE binary state.
    // Check its envelope before decoding, as the old validated loader did.
    if (!pnordeStateEnvelopeIsSafe(snapshot.getData(),
        static_cast<int>(snapshot.getSize()), getParameters().size())) return false;
    std::unique_ptr<juce::XmlElement> input(getXmlFromBinary(
        snapshot.getData(), static_cast<int>(snapshot.getSize())));
    if (!input || !input->hasTagName(parameters.state.getType())) return false;
    auto scene = juce::ValueTree::fromXml(*input);
    if (!scene.isValid()) return false;
    const auto liveParameters = parameters.copyState();
    // Keep display controls GLOBAL while also pinning the host's Quick
    // Program parameter. All these are APVTS child nodes, not root properties.
    // Deliberately DO NOT preserve pn_global_reverse: it belongs to each scene.
    // A parameter ID is a local name, not the processor's musical KEY.
    for (const char* globalParameterId : {"pn_darkness", "pn_theme", "pn_theme_extension",
                            "pn_animations", "pn_vibes_bleed", "pn_hppd_fps",
                            "pn_hypnosis", "pn_program_slot", "pn_play_enabled"})
        pnordeKeepLiveParameter(scene, liveParameters, globalParameterId);

    scene.setProperty("PnordeTheme", getThemeIndex(), nullptr);
    scene.setProperty("PnordeDarknessPercent", getDarknessPercent(), nullptr);
    scene.setProperty("PnordeAnimations", getAnimationsEnabled(), nullptr);
    scene.setProperty("PnordeScrollFades", getScrollFadesEnabled(), nullptr);
    scene.setProperty("PnordeTooltips", getTooltipsEnabled(), nullptr);
    scene.setProperty("PnordeExperimentalLights", getExperimentalLightsEnabled(), nullptr);
    scene.setProperty("PnordeVisualGlowEnabled", getVisualGlowEnabled(), nullptr);
    scene.setProperty("PnordePsychedelicEnabled", getPsychedelicEnabled(), nullptr);
    scene.setProperty("PnordePsychedelicIntensity", getPsychedelicIntensity(), nullptr);
    scene.setProperty("PnordeVibesMode", getVibesMode(), nullptr);
    scene.setProperty("PnordeVibesBleed", getVibesBleed(), nullptr);
    scene.setProperty("PnordeHypnosis", getHypnosis(), nullptr);
    scene.setProperty("PnordeHppdFps", getHppdFps(), nullptr);
    scene.setProperty("PnordeExperimentalReorder", getExperimentalReorderEnabled(), nullptr);
    scene.setProperty("NovaEditorWidth", getEditorWidth(), nullptr);
    scene.setProperty("NovaEditorHeight", getEditorHeight(), nullptr);
    juce::String visualOrder;
    for (int i = 0; i < optionalCount; ++i)
    {
        if (i != 0) visualOrder += ",";
        visualOrder += juce::String(getDisplayRowAt(i));
    }
    scene.setProperty("PnordeVisualOrder", visualOrder, nullptr);

    // v0.6.29: the original path converted this already validated ValueTree
    // back to XML + JUCE binary, then parsed XML into another ValueTree.
    // That did redundant work on every Quick Program switch. Keep exactly
    // the same strict loadValidatedTree() checks, without the round trip.
    // The destination is authoritative. Never infer it from the APVTS raw
    // value while replacing a scene: that was the program snap-back race.
    snapshotTargetProgram = juce::jlimit(0, programCount - 1, destination);
    applyingProgramSnapshot = true;
    const auto loadStartUs = measure ? pnordeDevClockUs() : 0;
    if (measure) *preparationUs = loadStartUs - preparationStartUs;
    const bool success = loadValidatedTree(std::move(scene));
    if (measure) *validatedLoadUs = pnordeDevClockUs() - loadStartUs;
    applyingProgramSnapshot = false;
    snapshotTargetProgram = -1;
    return success;
}

int PolyrhythmMIDIProcessor::getProgramLaneValue(int step) const noexcept
{
    return step >= 0 && step < maxSteps
        ? programLaneCells[static_cast<size_t>(step)].load(std::memory_order_relaxed) : 0;
}
void PolyrhythmMIDIProcessor::setProgramLaneValue(int step, int value) noexcept
{
    if (step >= 0 && step < maxSteps)
        programLaneCells[static_cast<size_t>(step)].store(juce::jlimit(0, 4, value), std::memory_order_relaxed);
}
int PolyrhythmMIDIProcessor::getProgramLaneLength() const noexcept
{ return programLaneLength.load(std::memory_order_relaxed); }
void PolyrhythmMIDIProcessor::setProgramLaneLength(int n) noexcept
{
    programLaneLength.store(juce::jlimit(1, maxSteps, n), std::memory_order_relaxed);
    setProgramLaneStart(getProgramLaneStart());
}
int PolyrhythmMIDIProcessor::getProgramLaneDivision() const noexcept
{ return programLaneDivision.load(std::memory_order_relaxed); }
void PolyrhythmMIDIProcessor::setProgramLaneDivision(int n) noexcept
{ programLaneDivision.store(juce::jlimit(0, 10, n), std::memory_order_relaxed); }
int PolyrhythmMIDIProcessor::getProgramLaneTiming() const noexcept
{ return programLaneTiming.load(std::memory_order_relaxed); }
void PolyrhythmMIDIProcessor::setProgramLaneTiming(int n) noexcept
{ programLaneTiming.store(juce::jlimit(0, 2, n), std::memory_order_relaxed); }
int PolyrhythmMIDIProcessor::getProgramLaneStart() const noexcept
{ return programLaneStart.load(std::memory_order_relaxed); }
void PolyrhythmMIDIProcessor::setProgramLaneStart(int n) noexcept
{ programLaneStart.store(juce::jlimit(0, getProgramLaneLength() - 1, n), std::memory_order_relaxed); }
int PolyrhythmMIDIProcessor::getProgramLaneCurrentStep() const noexcept
{ return programLaneCurrent.load(std::memory_order_relaxed); }
float PolyrhythmMIDIProcessor::getProgramLaneProgress() const noexcept
{ return programLaneProgress.load(std::memory_order_relaxed); }

bool PolyrhythmMIDIProcessor::duplicateLanePattern(int row)
{
    if (row < 0 || row >= laneCount) return false;
    const int length = getLaneLength(row);
    if (length < 1 || length > maxSteps / 2) return false;
    for (int i = 0; i < length; ++i)
    {
        if (row == 0) setStepEnabled(i + length, getStepEnabled(i));
        else setLaneValue(row, i + length, getLaneValue(row, i));
        if (row == 6) setAftertouchGlide(i + length, getAftertouchGlide(i));
        if (row >= baseLanes)
        {
            setOptionalGlide(row, i + length, getOptionalGlide(row, i));
            if (getOptionalLaneType(row) == 0)
                setCCTransitionMs(row - baseLanes, i + length,
                    getCCTransitionMs(row - baseLanes, i));
            if (getOptionalLaneType(row) == 3)
                setVibratoDepth(i + length, getVibratoDepth(i));
        }
    }
    setLaneLength(row, length * 2);
    return true;
}

bool PolyrhythmMIDIProcessor::copyProgram(int source, int destination)
{
    if (source < 0 || destination < 0 || source >= programCount ||
        destination >= programCount || source == destination) return false;
    const juce::ScopedLock lock(programMutex);
    if (source == getSelectedProgram()) captureCurrentProgram(savedPrograms[static_cast<size_t>(source)]);
    const auto& from = savedPrograms[static_cast<size_t>(source)];
    juce::MemoryBlock replacement = from.isEmpty() ? factoryProgram : from;
    if (replacement.isEmpty()) return false;
    if (destination == getSelectedProgram())
    {
        if (!restoreProgramSnapshot(replacement, destination)) return false;
        requestMidiPanic(); requestSequenceReset();
    }
    savedPrograms[static_cast<size_t>(destination)] = std::move(replacement);
    return true;
}

bool PolyrhythmMIDIProcessor::resetProgram(int index)
{
    if (index < 0 || index >= programCount || factoryProgram.isEmpty()) return false;
    const juce::ScopedLock lock(programMutex);
    if (index == getSelectedProgram())
    {
        if (!restoreProgramSnapshot(factoryProgram, index)) return false;
        requestMidiPanic(); requestSequenceReset();
    }
    savedPrograms[static_cast<size_t>(index)] = factoryProgram;
    return true;
}

int PolyrhythmMIDIProcessor::getSelectedProgram() const noexcept
{
    return activeProgram.load(std::memory_order_relaxed);
}

void PolyrhythmMIDIProcessor::selectProgram(int index)
{
    index = juce::jlimit(0, programCount - 1, index);
    // Manual selection wins immediately. Cancel an ALREADY-QUEUED trigger,
    // but leave the PROGRAM lane running if its On/Off switch is ON.
    // A later musical trigger may select another Program as expected.
    manualProgramEpoch.fetch_add(1, std::memory_order_acq_rel);
    pendingSequencedProgram.store(0, std::memory_order_release);
    if (auto* manager = juce::MessageManager::getInstanceWithoutCreating())
        if (manager->isThisTheMessageThread())
        {
            switchSource = 2;
            switchQueueDelayUs = 0;
            applyPendingProgramChange(index);
            const int committed = getSelectedProgram();
            observedHostProgram = committed;
            if (juce::roundToInt(parameters.getRawParameterValue("pn_program_slot")->load()) != committed)
                pnordeSetAutomatable(parameters, "pn_program_slot", committed);
        }
}

// Called by JUCE's 120-Hz MESSAGE-THREAD timer, not by the audio callback.
//
// A PROGRAM step can request a scene switch while the audio thread is
// running. Actually loading that scene involves copying state and XML;
// doing it on the audio thread could freeze Ableton. So processBlock()
// leaves a small integer request in an atomic mailbox, and this timer
// picks it up here. A busy message thread can therefore DELAY a switch;
// further profiling in Ableton is needed to verify real-world reliability.
// WHAT HAPPENS HERE: MESSAGE THREAD TIMER: UI/message-thread work is deliberately separated from processBlock. Program restoration may take milliseconds; do not run it on the audio callback.
void PolyrhythmMIDIProcessor::timerCallback()
{
    const auto request = pendingSequencedProgram.exchange(0, std::memory_order_acq_rel);
    // Read the request timestamp only for an active diagnostic session.
    // This is a best-effort measurement: fast overwritten requests may
    // share the latest timestamp, so it is not a sample-accurate timeline.
    const auto stampedUs = (request != 0 && developerStats.enabled())
        ? programRequestTimestampUs.load(std::memory_order_relaxed) : 0;
    const auto timerNowUs = stampedUs == 0 ? 0 : pnordeDevClockUs();
    switchQueueDelayUs = (stampedUs != 0 && timerNowUs >= stampedUs)
        ? timerNowUs - stampedUs : 0;
    switchSource = 1;
    int queuedIndex = -1;
    if (PnordeProgramRequests::unpack(request,
          manualProgramEpoch.load(std::memory_order_acquire), queuedIndex))
    {
        // Programme lane is an explicit destination command. Keep the VST3
        // parameter in sync exactly once for Ableton/hardware visibility.
        if (getProgramLaneEnabled())
            applyPendingProgramChange(queuedIndex);
        // A PROGRAM-lane trigger is only a request. Publish the committed
        // destination AFTER state restoration; this avoids a stale-parameter echo.
        const int committed = getSelectedProgram();
        observedHostProgram = committed;
        if (juce::roundToInt(parameters.getRawParameterValue("pn_program_slot")->load()) != committed)
            pnordeSetAutomatable(parameters, "pn_program_slot", committed);
        return;
    }
    const int hostIndex = juce::jlimit(0, programCount - 1,
        juce::roundToInt(parameters.getRawParameterValue("pn_program_slot")
            ->load(std::memory_order_acquire)));
    if (hostIndex != observedHostProgram)
    {
        switchSource = 2;
        switchQueueDelayUs = 0;
        // Live automation/manual choices also cancel older pending triggers.
        // They do not turn the PROGRAM lane off or silently disarm it.
        manualProgramEpoch.fetch_add(1, std::memory_order_acq_rel);
        pendingSequencedProgram.store(0, std::memory_order_release);
        applyPendingProgramChange(hostIndex);
        observedHostProgram = getSelectedProgram();
        // If a bad program was rejected, restore the host-visible selection.
        if (observedHostProgram != hostIndex)
            pnordeSetAutomatable(parameters, "pn_program_slot", observedHostProgram);
    }
}

void PolyrhythmMIDIProcessor::applyPendingProgramChange(int selected)
{
    selected = juce::jlimit(0, programCount - 1, selected);
    const int previous = activeProgram.load(std::memory_order_acquire);
    if (selected == previous) return;

    const juce::ScopedLock lock(programMutex);
    if (selected == activeProgram.load(std::memory_order_acquire)) return;
    const auto restoreStartUs = developerStats.enabled() ? pnordeDevClockUs() : 0;
    juce::MemoryBlock current;
    captureCurrentProgram(current);
    const auto captureEndUs = restoreStartUs != 0 ? pnordeDevClockUs() : 0;
    const auto captureUs = captureEndUs >= restoreStartUs
        ? captureEndUs - restoreStartUs : 0;
    savedPrograms[static_cast<size_t>(previous)] = std::move(current);
    if (savedPrograms[static_cast<size_t>(selected)].isEmpty())
        savedPrograms[static_cast<size_t>(selected)] = savedPrograms[static_cast<size_t>(previous)];

    std::uint64_t prepareUs = 0, loadUs = 0;
    // Do not display the previous switch's parameter-replace duration if
    // malformed input fails validation before replacement begins.
    if (restoreStartUs != 0) developerStats.recordParameterReplace(0);
    // The failure path still uses the exact same validation and rollback.
    // We pass timing pointers only when Developer Tools is enabled.
    if (!restoreProgramSnapshot(savedPrograms[static_cast<size_t>(selected)], selected,
            restoreStartUs != 0 ? &prepareUs : nullptr,
            restoreStartUs != 0 ? &loadUs : nullptr))
    {
        // Failed states cannot advance the selection. Preserve the bank and
        // recover previous music, then restore host-visible selection.
        (void) restoreProgramSnapshot(savedPrograms[static_cast<size_t>(previous)], previous);
        observedHostProgram = previous;
        if (restoreStartUs != 0)
        {
            developerStats.recordRestoreBreakdown(captureUs, prepareUs, loadUs);
            developerStats.recordSwitch(previous, selected, switchSource,
                switchQueueDelayUs, pnordeDevClockUs() - restoreStartUs, false);
        }
        return;
    }
    if (restoreStartUs != 0)
    {
        developerStats.recordRestoreBreakdown(captureUs, prepareUs, loadUs);
        developerStats.recordSwitch(previous, selected, switchSource,
            switchQueueDelayUs, pnordeDevClockUs() - restoreStartUs, true);
    }
    activeProgram.store(selected, std::memory_order_release);
    if (developerStats.enabled())
        midiTraceProgramMarker.store(static_cast<unsigned>(((previous+1)<<8)|(selected+1)),
                                    std::memory_order_release);
    // Do not notify Ableton AGAIN after scene load: repeated notifications
    // during parameter replacement were causing value echo/reversion.
    requestMidiPanic();
    requestSequenceReset();
}

// Built on the Settings/message thread only. Never call this from
// processBlock(): juce::String may allocate, which real-time code must avoid.
juce::String PolyrhythmMIDIProcessor::getDeveloperReport() const
{
    const auto s = developerStats.snapshot();
    juce::String report;
    report << "PNORDE v0.6.29  |  Developer Tools\n";
    report << "Application build: " << PnordeProjectVersion::appBuild
           << " | Saved-state compatibility: " << PnordeProjectVersion::build << "\n";
    report << "Diagnostics: " << (developerStats.enabled() ? "ON" : "OFF") << " (not saved)\n";
    report << "Program: P" << (getSelectedProgram() + 1) << " / 4\n";
    report << "Host BPM (observed): " << juce::String(getHostBpm(), 2) << "\n";
    report << "Sample rate: " << juce::String(developerSampleRate.load(std::memory_order_relaxed), 0) << " Hz\n";
    report << "Last audio block: " << developerBlockSize.load(std::memory_order_relaxed) << " samples\n";
    report << "Active custom CC Macro lanes: " << activeCCMacroCount() << "\n";
    report << "Callbacks sampled: " << juce::String(static_cast<juce::int64>(s.audioBlocks)) << "\n";
    if (s.audioBlocks != 0)
        report << "Callback average: "
               << juce::String(static_cast<double>(s.audioTotalUs) / s.audioBlocks / 1000.0, 3)
               << " ms\n";
    report << "Callback peak: " << juce::String(s.audioPeakUs / 1000.0, 3) << " ms\n";
    report << "MIDI messages emitted: " << juce::String(static_cast<juce::int64>(s.midiMessages)) << "\n";
    report << "Peak emitted MIDI per audio block: "
           << juce::String(static_cast<juce::int64>(s.midiPeakPerBlock)) << "\n";
    report << "Program requests queued: " << juce::String(static_cast<juce::int64>(s.programRequests)) << "\n";
    report << "Overwritten pending requests: " << juce::String(static_cast<juce::int64>(s.overwrittenRequests)) << "\n";
    report << "Successful switches: " << juce::String(static_cast<juce::int64>(s.programSwitches)) << "\n";
    report << "Failed restores: " << juce::String(static_cast<juce::int64>(s.programRestoreFailures)) << "\n";
    if (s.programSwitches != 0)
    {
        report << "Restoration average / peak: "
               << juce::String(static_cast<double>(s.restoreTotalUs) / s.programSwitches / 1000.0, 3)
               << " / " << juce::String(s.restorePeakUs / 1000.0, 3) << " ms\n";
        report << "Queue wait average / peak: "
               << juce::String(static_cast<double>(s.queueTotalUs) / s.programSwitches / 1000.0, 3)
               << " / " << juce::String(s.queuePeakUs / 1000.0, 3) << " ms\n";
    }
    if (s.lastFrom >= 0 && s.lastTo >= 0)
    {
        report << "Last switch: P" << s.lastFrom + 1 << " -> P" << s.lastTo + 1
               << " (" << (s.lastSource == 1 ? "PROGRAM lane" : "manual/host") << ")\n";
        report << "Last queue wait (approx): " << juce::String(s.lastQueueUs / 1000.0, 3) << " ms\n";
        report << "Last save + restoration: " << juce::String(s.lastRestoreUs / 1000.0, 3) << " ms\n";
        // These three timers divide the expensive path into testable parts.
        // Sum may differ slightly from total due to overhead outside phases.
        report << "  Outgoing snapshot capture: " << juce::String(s.lastCaptureUs / 1000.0, 3) << " ms\n";
        report << "  Incoming snapshot preparation: " << juce::String(s.lastPrepareUs / 1000.0, 3) << " ms\n";
        report << "  Validated state loading: " << juce::String(s.lastLoadUs / 1000.0, 3) << " ms\n";
        report << "    of which JUCE parameter replace: "
               << juce::String(s.lastParameterReplaceUs / 1000.0, 3) << " ms\n";
    }
    if (developerStats.enabled())
    {
        const auto m = midiBlackBox.summary();
        report << "\nMIDI BLACK BOX (output-side, optional)\n";
        report << "Note Ons / Offs: " << juce::String(static_cast<juce::int64>(m.emittedNoteOns))
               << " / " << juce::String(static_cast<juce::int64>(m.emittedNoteOffs)) << "\n";
        report << "Outstanding channel+note keys (approx): " << m.outstandingKeys << "\n";
        report << "Unmatched emitted Note Offs: "
               << juce::String(static_cast<juce::int64>(m.unmatchedNoteOffs)) << "\n";
        report << "CC64 ON/OFF: " << juce::String(static_cast<juce::int64>(m.emittedCC64On))
               << " / " << juce::String(static_cast<juce::int64>(m.emittedCC64Off)) << "\n";
        report << "CC120 / CC123: " << juce::String(static_cast<juce::int64>(m.emittedCC120))
               << " / " << juce::String(static_cast<juce::int64>(m.emittedCC123)) << "\n";
        report << "Other CCs emitted: " << juce::String(static_cast<juce::int64>(m.otherCCs)) << "\n";
        report << "Pitch-wheel messages: " << juce::String(static_cast<juce::int64>(m.pitchWheels)) << "\n";
        report << "Trace buffer wraps: " << juce::String(static_cast<juce::int64>(m.droppedTraceEvents)) << "\n";
        report << "External MIDI queue drops (lifetime): " << getExternalMidiDroppedEvents() << "\n";
        report << "Captured stop: " << (m.stopKind==1 ? "PNORDE Play off" :
                       m.stopKind==2 ? "Host stopped" : "none")
               << (m.frozen ? " (trace frozen until Reset)" : "") << "\n";
        // Show up to the last 64 relevant output events. No MIDI is sent,
        // and no allocations happen until the user explicitly asks for text.
        const std::uint64_t first = m.totalEvents > 96 ? m.totalEvents-96 : 0;
        for (std::uint64_t i=first; i<m.totalEvents; ++i)
        {
            PnordeMidiBlackBox::Event e;
            if (!midiBlackBox.read(i,e)) continue;
            const auto relMs = m.stopKind != 0
                ? (static_cast<double>(e.frame)-static_cast<double>(m.stopFrame)) * 1000.0 /
                    juce::jmax(1.0, developerSampleRate.load(std::memory_order_relaxed))
                : 0.0;
            if (m.stopKind != 0 && relMs < -200.0) continue;
            report << "  #" << juce::String(static_cast<juce::int64>(i+1));
            if (m.stopKind != 0) report << " [" << juce::String(relMs, 1) << " ms]";
            if (e.marker == 4)
                report << " <QUICK PROGRAM P" << int(e.data1) << " -> P" << int(e.data2)
                       << " (next audio block)>";
            else if (e.marker != 0)
                report << " <TRANSPORT STOP: " << (e.marker == 1 ? "PNORDE Play off" : "host") << ">";
            else
            {
                const int kind=e.status&0xf0, channel=(e.status&0x0f)+1;
                report << " ch" << channel << " ";
                if (kind==0x90 && e.data2!=0) report << "NOTE ON " << int(e.data1) << " vel " << int(e.data2);
                else if (kind==0x80 || (kind==0x90 && e.data2==0)) report << "NOTE OFF " << int(e.data1);
                else if (kind==0xb0) report << "CC" << int(e.data1) << "=" << int(e.data2);
                else if (kind==0xe0) report << "PITCH WHEEL "
                                              << (int(e.data1)|(int(e.data2)<<7));
                else report << "MIDI status=" << int(e.status);
            }
            report << "\n";
        }
        report << "Black box is evidence of EMITTED MIDI only; not proof of a synth's sounding voices.\n";
        report << "Trace freezes after stop; click Reset counters to capture another stop.\n";
    }
    report << "NOTE: Figures are approximate; output is what PNORDE emitted, not downstream CPU cost.\n";
    report << "No project name, file paths or identifying information included.\n";
    return report;
}

// WHAT HAPPENS HERE: PROJECT SAVE: Serialize the state to bytes which Ableton can store inside a session. Do not change the schema lightly: old projects depend on it.
void PolyrhythmMIDIProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    const juce::ScopedLock lock(programMutex);
    juce::MemoryBlock current;
    captureCurrentProgram(current);
    savedPrograms[static_cast<size_t>(getSelectedProgram())] = current;
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(current.getData(),
        static_cast<int>(current.getSize())));
    if (xml == nullptr) { destData = current; return; }
    xml->setAttribute("PnordeProgramSelected", getSelectedProgram());
    xml->setAttribute("PnordeProgramLaneEnabled", getProgramLaneEnabled() ? 1 : 0);
    // Historic project files expect this field. It is now just a mirror of ON.
    xml->setAttribute("PnordeProgramLaneAutoArmed", getProgramLaneEnabled() ? 1 : 0);
    xml->setAttribute("PnordeProgramLaneLength", getProgramLaneLength());
    xml->setAttribute("PnordeProgramLaneDivision", getProgramLaneDivision());
    xml->setAttribute("PnordeProgramLaneTiming", getProgramLaneTiming());
    xml->setAttribute("PnordeProgramLaneStart", getProgramLaneStart());
    xml->setAttribute("PnordeProgramLaneRotation", getProgramLaneRotation());
    juce::String programSteps;
    for (int n = 0; n < maxSteps; ++n)
    {
        if (n) programSteps += ",";
        programSteps += juce::String(getProgramLaneValue(n));
    }
    xml->setAttribute("PnordeProgramLaneValues", programSteps);
    for (int i = 0; i < programCount; ++i)
    {
        auto& saved = savedPrograms[static_cast<size_t>(i)];
        if (saved.isEmpty()) saved = current; // all four scenes always persist
        xml->setAttribute("PnordeProgramSlot" + juce::String(i), saved.toBase64Encoding());
    }
    copyXmlToBinary(*xml, destData);
}

namespace
{
// Preflight untrusted state BEFORE JUCE's XML parser is invoked. Reject XML
// declarations that could introduce entities/DTDs, excessively deep or large
// trees, and invalid tags. The test is intentionally conservative because
// PNORDE generates a shallow, predictable state tree.
// The parameter count comes from AudioProcessor::getParameters(), so this
// validator automatically knows when a later version adds automation IDs.
// Do not restore the old hardcoded 400-tag cap: every new <PARAM> can need
// both an opening and a closing tag in the XML serialization.
bool pnordeStateEnvelopeIsSafe(const void* data, int size,
                               int registeredParameters) noexcept
{
    if (data == nullptr || size < 16 || size > 4 * 1024 * 1024) return false;
    const auto* bytes = static_cast<const unsigned char*>(data);
    int tagCount = 0, depth = 0;
    for (int i = 0; i + 1 < size; ++i)
    {
        if (bytes[i] != '<') continue;
        const unsigned char next = bytes[i + 1];
        if (next == '!') return false; // DTD, ENTITY, CDATA, comments
        if (next == '?') continue; // ordinary XML declaration
        const bool closing = next == '/';
        if (++tagCount > PnordeStateLimits::allowedXmlTags(registeredParameters)) return false;
        if (closing)
        {
            if (--depth < 0) return false;
        }
        else if (++depth > 8) return false;
        int end = i + 1;
        unsigned char quote = 0;
        for (; end < size; ++end)
        {
            const auto c = bytes[end];
            if (quote != 0) { if (c == quote) quote = 0; }
            else if (c == '\'' || c == '"') quote = c;
            else if (c == '>') break;
        }
        if (end >= size || quote != 0) return false;
        if (!closing && end > i && bytes[end - 1] == '/') --depth;
        i = end;
    }
    return tagCount > 0 && depth == 0;
}

// A ValueTree child is normally one automatable PARAM value. The permitted
// count must follow the current registered parameter count. Otherwise a new
// control can make a perfectly valid Quick Program impossible to restore.
bool pnordeStateTreeIsSafe(const juce::ValueTree& state,
                           int registeredParameters)
{
    // CHILDREN are host-automation PARAM entries (151 in v0.6.25).
    // PROPERTIES are separate attributes such as saved cell grids and labels.
    // These are different counts and deliberately use different safeguards.
    // The children limit now grows with the plugin's real automation count;
    // 512 remains the established root-property safety limit.
    if (!state.isValid() ||
        !PnordeStateLimits::acceptsChildren(state.getNumChildren(), registeredParameters) ||
        state.getNumProperties() > 512)
        return false;
    for (int p = 0; p < state.getNumProperties(); ++p)
    {
        const auto name = state.getPropertyName(p).toString();
        const bool programBlob = name.startsWith("PnordeProgramSlot") &&
            name.length() == 18 && name.getLastCharacter() >= '0' &&
            name.getLastCharacter() <= '3';
        if (name.length() > 96 ||
            state.getProperty(state.getPropertyName(p)).toString().length() >
                (programBlob ? 524288 : 8192))
            return false;
    }
    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        const auto child = state.getChild(i);
        if (child.getNumChildren() != 0 || child.getNumProperties() > 12) return false;
        for (int p = 0; p < child.getNumProperties(); ++p)
        {
            const auto name = child.getPropertyName(p);
            if (name.toString().length() > 64 || child.getProperty(name).toString().length() > 256)
                return false;
        }
    }
    return true;
}
}

// WHAT HAPPENS HERE: PROJECT LOAD: Validate incoming saved bytes before applying them. A corrupted or older file must not cause unsafe memory access.
void PolyrhythmMIDIProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Hosts can supply state too, so apply the same limits as preset imports.
    (void) loadValidatedState(data, sizeInBytes);
}

bool PolyrhythmMIDIProcessor::loadValidatedState(const void* data, int sizeInBytes)
{
    // Don't partially apply rejected host states. Hosts may invoke this while
    // loading old projects; complete PNORDE snapshots must pass the bounded
    // structural checks, including the optional-lane arrays.
    // This number includes all host-visible parameters registered by JUCE.
    // It is obtained at runtime, NOT copied into a manually maintained list.
    // No host parameter IDs, ranges or orders are changed by this safety fix.
    const int registeredParameters = getParameters().size();
    jassert(PnordeStateLimits::hasExpansionRoom(registeredParameters));
    if (!pnordeStateEnvelopeIsSafe(data, sizeInBytes, registeredParameters)) return false;
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml == nullptr || !xml->hasTagName(parameters.state.getType()))
        return false;
    auto state = juce::ValueTree::fromXml(*xml);
    return loadValidatedTree(std::move(state));
}

// One loader, one set of guards. The Quick Program path has already decoded
// its incoming scene so we don't need to encode+decode it a second time.
// Calling this NEVER skips malformed-state rejection or the legacy defaults.
bool PolyrhythmMIDIProcessor::loadValidatedTree(juce::ValueTree state)
{
    const int registeredParameters = getParameters().size();
    jassert(PnordeStateLimits::hasExpansionRoom(registeredParameters));
    if (!pnordeStateTreeIsSafe(state, registeredParameters)) return false;
    // Refuse a structurally valid but incomplete XML document: otherwise a
    // truncated/malicious preset could silently reset lanes or gate states.
    if (!state.hasProperty("NovaPattern") ||
        !state.hasProperty("NovaLaneDataVersion") ||
        !state.hasProperty("NovaSequenceLength") ||
        !state.hasProperty("NovaCustomCCCount")) return false;
    const auto requiredPattern = state.getProperty("NovaPattern").toString();
    if (requiredPattern.length() != 256) return false;
    for (int step = 0; step < 256; ++step)
        if (requiredPattern[step] != '0' && requiredPattern[step] != '1')
            return false;
    // A missing marker identifies v0.6.6 and earlier. A partial/future schema
    // is refused rather than risking unpredictable restoration.
    const bool hasSchema = state.hasProperty("PnordeStateSchema");
    const bool hasBuild = state.hasProperty("PnordeSavedBuild");
    if (hasSchema != hasBuild) return false;
    if (hasSchema)
    {
        const auto schemaText = state.getProperty("PnordeStateSchema").toString();
        const auto buildText = state.getProperty("PnordeSavedBuild").toString();
        if (!schemaText.containsOnly("0123456789") ||
            !buildText.containsOnly("0123456789") ||
            schemaText.getIntValue() < 1 ||
            schemaText.getIntValue() > PnordeProjectVersion::schema ||
            buildText.getIntValue() < 1 ||
            buildText.getIntValue() > PnordeProjectVersion::build)
            return false;
    }
    // Prevalidate optional 0..100 probability data BEFORE touching any
    // live MIDI parameters. Old files may omit this entire section.
    for (int row = 0; row < probabilityRows; ++row)
    {
        const auto property = "PnordeProbability" + juce::String(row);
        if (!state.hasProperty(property)) continue;
        const auto parts = juce::StringArray::fromTokens(
            state.getProperty(property).toString(), ",", "");
        if (parts.size() != maxSteps) return false;
        for (const auto& part : parts)
            if (part.isEmpty() || part.length() > 3 ||
                !part.containsOnly("0123456789") || part.getIntValue() > 100)
                return false;
    }
    const int stateVersion = static_cast<int>(state.getProperty("NovaLaneDataVersion"));
    if (stateVersion < 1 || stateVersion > 16) return false;
    // Version 7 introduced the complete 21-lane format; all lanes must be
    // present together or nothing should be restored.
    if (stateVersion >= 7)
    {
        for (int row = 1; row < laneCount; ++row)
        {
            const auto prefix = "NovaLane" + juce::String(row);
            if (!state.hasProperty(prefix + "Values") ||
                !state.hasProperty(prefix + "Length") ||
                !state.hasProperty(prefix + "Division")) return false;
        }
    }

    // Reject malformed optional-lane assignments BEFORE applying any state.
    // Fixed types must be unique, and a maximum of eight CC Macro lanes may
    // exist across all fourteen optional row slots.
    if (stateVersion >= 8)
    {
        const int occupiedMacroRows = juce::jlimit(0, maxCustomCC,
            static_cast<int>(state.getProperty("NovaCustomCCCount", 0)));
        int seenKinds = 0, ccCount = 0;
        for (int row = baseLanes; row < laneCount; ++row)
        {
            const int slot = row - baseLanes;
            const int kind = static_cast<int>(state.getProperty(
                "PnordeOptionalType" + juce::String(slot),
                static_cast<int>(PnordeLane::defaultType(row))));
            if (kind < 0 || kind > 6) return false;
            const bool enabled = row < modRow ? slot < occupiedMacroRows
                : row == channelLane ? static_cast<bool>(state.getProperty("PnordeChannelLaneEnabled", false))
                : static_cast<bool>(state.getProperty("PnordeDedicated" + juce::String(row), false));
            if (!enabled) continue;
            if (kind == 0) ++ccCount;
            else
            {
                if ((seenKinds & (1 << kind)) != 0) return false;
                seenKinds |= (1 << kind);
            }
        }
        if (ccCount > maxCustomCC) return false;
    }
    // Reject invalid saved permutations and pressure arrays before touching
    // live state. Old states omit both properties and remain loadable.
    if (state.hasProperty("PnordeVisualOrder"))
    {
        const auto parts = juce::StringArray::fromTokens(
            state.getProperty("PnordeVisualOrder").toString(), ",", "");
        if (parts.size() != optionalCount) return false;
        PnordeVisualOrder::Rows rows {};
        for (int i = 0; i < optionalCount; ++i)
        {
            const auto text = parts[i].trim();
            if (text.isEmpty() || text.containsAnyOf(".abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")) return false;
            rows[i] = text.getIntValue();
        }
        if (!PnordeVisualOrder::valid(rows)) return false;
    }
    if (state.hasProperty("PnordeAftertouchGlide"))
    {
        const auto parts = juce::StringArray::fromTokens(
            state.getProperty("PnordeAftertouchGlide").toString(), ",", "");
        if (parts.size() != maxSteps) return false;
        for (const auto& text : parts)
            if (text.trim().isEmpty() || text.containsAnyOf(".abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") ||
                text.getIntValue() < -1 || text.getIntValue() > (state.hasProperty("PnordeAftertouchGlideUnits") ? 201 : 100)) return false;
    }
    // Malformed optional-glide arrays cannot be partially loaded.
    for (int row = 0; row < optionalCount; ++row)
    {
        const auto property = "PnordeOptionalGlide" + juce::String(row);
        if (!state.hasProperty(property)) continue; // pre-v0.5.7 state
        const auto parts = juce::StringArray::fromTokens(state.getProperty(property).toString(), ",", "");
        if (parts.size() != maxSteps) return false;
        for (const auto& text : parts)
            if (text.isEmpty() || text.containsAnyOf(".abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") ||
                text.getIntValue() < 0 || text.getIntValue() > 100) return false;
    }
    // Prevent unexpectedly large or malformed label payloads in a saved state.
    for (int row = 0; row < laneCount; ++row)
        if (state.getProperty("PnordeLaneName" + juce::String(row), "")
                .toString().length() > 64)
            return false;
    // Validate new v0.6.11+ start fields before ANY live parameter writes.
    for (int row = 0; row < laneCount; ++row)
    {
        const auto prop = "PnordeLaneStart" + juce::String(row);
        if (!state.hasProperty(prop)) continue;
        const auto startText = state.getProperty(prop).toString();
        if (startText.isEmpty() || !startText.containsOnly("0123456789") ||
            startText.getIntValue() < 0 || startText.getIntValue() >= maxSteps)
            return false;
    }
    // v0.6.25: Reject malformed rotation offsets before modifying live MIDI.
    // Old presets lack these properties and therefore use rotation zero.
    for (int row = 0; row < laneCount; ++row)
    {
        // This string identifies a saved rotation property, not musical key.
        const auto rotationPropertyId = "PnordeLaneRotation" + juce::String(row);
        if (!state.hasProperty(rotationPropertyId)) continue;
        const auto text = state.getProperty(rotationPropertyId).toString();
        if (text.isEmpty() || !text.containsOnly("0123456789") ||
            text.getIntValue() >= maxSteps) return false;
    }
    if (state.hasProperty("PnordeProgramLaneRotation"))
    {
        const auto text = state.getProperty("PnordeProgramLaneRotation").toString();
        if (text.isEmpty() || !text.containsOnly("0123456789") ||
            text.getIntValue() >= maxSteps) return false;
    }
    // v0.6.15 bank migration: old/unversioned files become Program 1.
    // Each bank snapshot must be a complete, bounded legacy-format tree.
    // Do this BEFORE writing any live values, so corrupt banks can't erase
    // otherwise recoverable material. No nested banks are accepted.
    std::array<juce::MemoryBlock, programCount> restoredPrograms;
    const bool isBanked = state.hasProperty("PnordeProgramSelected");
    int restoredSelection = 0;
    if (!applyingProgramSnapshot && isBanked)
    {
        const auto indexText = state.getProperty("PnordeProgramSelected").toString();
        if (!indexText.containsOnly("0123456789")) return false;
        restoredSelection = indexText.getIntValue();
        if (restoredSelection < 0 || restoredSelection >= programCount) return false;
        for (int i = 0; i < programCount; ++i)
        {
            const auto name = "PnordeProgramSlot" + juce::String(i);
            if (!state.hasProperty(name) ||
                !restoredPrograms[static_cast<size_t>(i)].fromBase64Encoding(
                    state.getProperty(name).toString())) return false;
            const auto& slot = restoredPrograms[static_cast<size_t>(i)];
            if (slot.getSize() > 1024 * 1024 ||
                !pnordeStateEnvelopeIsSafe(slot.getData(), static_cast<int>(slot.getSize()),
                                           registeredParameters))
                return false;
            std::unique_ptr<juce::XmlElement> slotXml(getXmlFromBinary(
                slot.getData(), static_cast<int>(slot.getSize())));
            if (!slotXml || !slotXml->hasTagName(parameters.state.getType())) return false;
            const auto slotTree = juce::ValueTree::fromXml(*slotXml);
            if (!pnordeStateTreeIsSafe(slotTree, registeredParameters) ||
                !slotTree.hasProperty("NovaPattern") ||
                slotTree.getProperty("NovaPattern").toString().length() != 256 ||
                !slotTree.hasProperty("NovaLaneDataVersion") ||
                slotTree.hasProperty("PnordeProgramSelected")) return false;
        }
    }
    // Visibility and AUTO/MANUAL state are GLOBAL; musical trigger cells
    // belong to each of the four program snapshots.
    const auto armText = state.getProperty("PnordeProgramLaneAutoArmed", "0").toString();
    if (!applyingProgramSnapshot && armText != "0" && armText != "1") return false;
    // Dedicated PROGRAM lane is global and optional for all historical states.
    // Validate fully before changing ANY live values. Missing means OFF.
    std::array<int, maxSteps> restoredProgramSteps {};
    bool restoredProgramEnabled = false;
    int restoredProgramLength = 16, restoredProgramDivision = 2;
    int restoredProgramTiming = 0, restoredProgramStart = 0;
    const bool haveProgramLaneData = state.hasProperty("PnordeProgramLaneValues");
    if (haveProgramLaneData)
    {
        const auto parts = juce::StringArray::fromTokens(
            state.getProperty("PnordeProgramLaneValues").toString(), ",", "");
        if (parts.size() != maxSteps) return false;
        for (int i = 0; i < maxSteps; ++i)
        {
            if (parts[i].length() != 1 || !parts[i].containsOnly("01234")) return false;
            restoredProgramSteps[static_cast<size_t>(i)] = parts[i].getIntValue();
        }
        const auto safeInt = [&](const char* key, int minimum, int maximum, int& target)
        {
            const auto s = state.getProperty(key, "").toString();
            if (s.isEmpty() || !s.containsOnly("0123456789")) return false;
            target = s.getIntValue();
            return target >= minimum && target <= maximum;
        };
        // Old v0.6.16/17 banks had an always-visible PROGRAM lane. Keep it
        // visible if they contain triggers, but keep an empty lane optional.
        bool anyTriggers = false;
        for (int target : restoredProgramSteps) anyTriggers = anyTriggers || target != 0;
        if (!applyingProgramSnapshot)
        {
            if (state.hasProperty("PnordeProgramLaneEnabled"))
            {
                const auto raw = state.getProperty("PnordeProgramLaneEnabled").toString();
                if (raw != "0" && raw != "1") return false;
                restoredProgramEnabled = raw == "1";
            }
            else restoredProgramEnabled = anyTriggers;
        }
        if (!safeInt("PnordeProgramLaneLength", 1, maxSteps, restoredProgramLength) ||
            !safeInt("PnordeProgramLaneDivision", 0, 10, restoredProgramDivision) ||
            !safeInt("PnordeProgramLaneTiming", 0, 2, restoredProgramTiming) ||
            !safeInt("PnordeProgramLaneStart", 0, restoredProgramLength - 1, restoredProgramStart))
            return false;
    }
    // Migrate v0.6.15-18 banks: those versions saved the PROGRAM lane once
    // at bank level, not within each scene. Seed any missing scene lane data
    // with that old common lane. After switching/editing, each scene can
    // evolve its trigger sequence independently without losing the old one.
    if (!applyingProgramSnapshot && isBanked && haveProgramLaneData)
    {
        for (auto& oldScene : restoredPrograms)
        {
            std::unique_ptr<juce::XmlElement> oldXml(getXmlFromBinary(
                oldScene.getData(), static_cast<int>(oldScene.getSize())));
            if (!oldXml) return false;
            auto oldTree = juce::ValueTree::fromXml(*oldXml);
            if (!oldTree.isValid()) return false;
            if (oldTree.hasProperty("PnordeProgramLaneValues")) continue;
            // Name each saved property explicitly to avoid shadowing the KEY control.
            for (const char* programPropertyId : {"PnordeProgramLaneLength", "PnordeProgramLaneDivision",
                                    "PnordeProgramLaneTiming", "PnordeProgramLaneStart",
                                    "PnordeProgramLaneValues", "PnordeProgramLaneRotation"})
                oldTree.setProperty(programPropertyId, state.getProperty(programPropertyId), nullptr);
            juce::MemoryBlock migrated;
            if (auto migratedXml = std::unique_ptr<juce::XmlElement>(oldTree.createXml()))
                copyXmlToBinary(*migratedXml, migrated);
            if (migrated.isEmpty() || migrated.getSize() > 1024 * 1024) return false;
            oldScene = std::move(migrated);
        }
    }
    if (!applyingProgramSnapshot)
    {
        // Snapshot of the selected program without banking metadata.
        auto plain = state.createCopy();
        plain.removeProperty("PnordeProgramSelected", nullptr);
        for (int i = 0; i < programCount; ++i)
            plain.removeProperty("PnordeProgramSlot" + juce::String(i), nullptr);
        for (const char* property : {"PnordeProgramLaneEnabled", "PnordeProgramLaneAutoArmed"})
            plain.removeProperty(property, nullptr);
        juce::MemoryBlock legacySnapshot;
        if (auto xmlCopy = std::unique_ptr<juce::XmlElement>(plain.createXml()))
            copyXmlToBinary(*xmlCopy, legacySnapshot);
        const juce::ScopedLock lock(programMutex);
        if (isBanked) savedPrograms = std::move(restoredPrograms);
        else
        {
            // v0.6.14 and earlier: preserve everything as Program 1.
            for (auto& slot : savedPrograms) slot = legacySnapshot;
        }
        activeProgram.store(restoredSelection, std::memory_order_relaxed);
        setProgramLaneEnabled(restoredProgramEnabled);
        // The legacy AUTO flag is intentionally ignored from v0.6.22 fixes
        // onwards. The saved PROGRAM lane On/Off switch is the only authority.
        // An old project with Lane=ON and AUTO=OFF now plays its triggers.
        (void) armText;
        programLaneRestartRequested.store(true, std::memory_order_release);
        programLaneCurrent.store(-1, std::memory_order_relaxed);
        pendingSequencedProgram.store(0, std::memory_order_relaxed);
        state = plain;
    }
    if (haveProgramLaneData)
    {
        setProgramLaneLength(restoredProgramLength);
        setProgramLaneDivision(restoredProgramDivision);
        setProgramLaneTiming(restoredProgramTiming);
        setProgramLaneStart(restoredProgramStart);
    // Restore the PROGRAM rotation in each Quick Program snapshot too.
    setProgramLaneRotation(static_cast<int>(state.getProperty(
        "PnordeProgramLaneRotation", 0)));
        for (int i = 0; i < maxSteps; ++i)
            setProgramLaneValue(i, restoredProgramSteps[static_cast<size_t>(i)]);
        programLaneRestartRequested.store(true, std::memory_order_release);
    }
    // Program selection is a live host control, NEVER a per-scene parameter.
    // Otherwise loading a scene can revert the selection on the next timer tick.
    const int pinnedProgram = applyingProgramSnapshot
        ? juce::jlimit(0, programCount - 1, snapshotTargetProgram)
        : restoredSelection;
    state.setProperty("pn_program_slot", pinnedProgram, nullptr);
    // v0.6.21 or older state has no swing children. Explicitly seed straight
    // swing in the incoming private tree so changing Quick Programs can never
    // leak swing from the previously active one into a legacy scene.
    for (int row = 0; row < laneCount; ++row)
    {
        // Each swing parameter belongs to one lane. It is not the musical KEY.
        const auto swingParameterId = "pn_lane_swing_" + juce::String(row);
        if (!pnordeSavedParameterExists(state, swingParameterId))
        {
            juce::ValueTree param("PARAM");
            param.setProperty("id", swingParameterId, nullptr);
            param.setProperty("value", PnordeSwing::straight, nullptr);
            state.addChild(param, -1, nullptr);
        }
    }
    // Older projects contain no pn_lane_random_* parameters. Explicitly
    // default every missing one to OFF, so loading an older Quick Program
    // never inherits RANDOM from whichever newer bank was selected before.
    for (int row = 0; row < laneCount; ++row)
    {
        const auto id = "pn_lane_random_" + juce::String(row);
        if (!pnordeSavedParameterExists(state, id))
        {
            juce::ValueTree param("PARAM");
            param.setProperty("id", id, nullptr);
            param.setProperty("value", 0, nullptr);
            state.addChild(param, -1, nullptr);
        }
    }
    // All validation happens before modifying any live sequencer parameters.
    // This is often the most consequential restoration operation: JUCE
    // updates the host-facing parameter tree. Measure just this operation
    // (only while diagnostics are enabled), but don't change its semantics.
    const bool measureParameterReplace = applyingProgramSnapshot && developerStats.enabled();
    const auto parameterReplaceStartUs = measureParameterReplace ? pnordeDevClockUs() : 0;
    parameters.replaceState(state);
    if (measureParameterReplace)
        developerStats.recordParameterReplace(pnordeDevClockUs() - parameterReplaceStartUs);
    if (!applyingProgramSnapshot)
    {
        pnordeSetAutomatable(parameters, "pn_program_slot", restoredSelection);
        observedHostProgram = restoredSelection;
        manualProgramEpoch.fetch_add(1, std::memory_order_acq_rel);
        requestMidiPanic();
        requestSequenceReset();
    }
    // The append-only global-reverse parameter does not exist in pre-v0.6.13
    // states; old sessions must always return to their original forward default.
    // Only truly historical projects lacking this host parameter default OFF.
    // APVTS serialises "pn_global_reverse" under its PARAM child (`id`), not
    // as a property of the PARAMETERS root. Checking hasProperty() here
    // previously forced OFF after EVERY quick-program restore.
    if (!pnordeSavedParameterExists(state, "pn_global_reverse"))
        setGlobalReverse(false);
    for (int row = 0; row < laneCount; ++row)
    {
        setLaneName(row, state.getProperty("PnordeLaneName" + juce::String(row), "").toString());
    }

    setSequenceLength(
        static_cast<int>(state.getProperty(
            "NovaSequenceLength", 16)));

    setClockDivisionIndex(
        static_cast<int>(state.getProperty(
            "NovaClockDivision", 2)));

    setEditorSize(
        static_cast<int>(state.getProperty(
            "NovaEditorWidth", 1500)),
        static_cast<int>(state.getProperty(
            "NovaEditorHeight", 1000)));

    setThemeIndex(static_cast<int>(state.getProperty("PnordeTheme", 0)));
    setDarknessPercent(static_cast<int>(state.getProperty("PnordeDarknessPercent", 0)));
    setAnimationsEnabled(static_cast<bool>(state.getProperty("PnordeAnimations", true)));
    setScrollFadesEnabled(static_cast<bool>(state.getProperty("PnordeScrollFades", true)));
    setTooltipsEnabled(static_cast<bool>(state.getProperty("PnordeTooltips", true)));
    setExperimentalLightsEnabled(static_cast<bool>(state.getProperty("PnordeExperimentalLights", true)));
    setExperimentalReorderEnabled(static_cast<bool>(state.getProperty("PnordeExperimentalReorder", false)));
    lowPitchAnchor.store(static_cast<bool>(state.getProperty("PnordePitchAnchorLow",false)),std::memory_order_relaxed);
    setPsychedelicEnabled(static_cast<bool>(state.getProperty("PnordePsychedelicEnabled",false)));
    setPsychedelicIntensity(static_cast<int>(state.getProperty("PnordePsychedelicIntensity",20)));
    setVibesBleed(static_cast<int>(state.getProperty("PnordeVibesBleed",25)));
    setVibesMode(static_cast<int>(state.getProperty("PnordeVibesMode",4)));
    setHppdFps(static_cast<int>(state.getProperty("PnordeHppdFps",12)));
    setHypnosis(static_cast<int>(state.getProperty("PnordeHypnosis",100)));
    setVisualGlowEnabled(static_cast<bool>(state.getProperty("PnordeVisualGlowEnabled", true)));
    setLaneDirection(0,static_cast<int>(state.getProperty("NovaLane0Direction",0)));
    // v0.5.8 had no micro-mode switch. Preserve the old combined
    // Slide + EDO playback behaviour on those saved projects only.
    const int savedMicroMode = state.hasProperty("PnordeMicroMode")
        ? static_cast<int>(state.getProperty("PnordeMicroMode"))
        : PnordeEDO::isMode(getScale()) ? PnordeMicroOutput::legacySlide : PnordeMicroOutput::bendSafe;
    setMicroOutputMode(juce::jlimit(0, 2, savedMicroMode));
    setExperimentalPolyPressure(static_cast<bool>(state.getProperty("PnordePolyPressure", false)));
    const auto order = juce::StringArray::fromTokens(
        state.getProperty("PnordeVisualOrder", "").toString(), ",", "");
    for (int i = 0; i < optionalCount; ++i)
        displayOrder[i].store(order.size() == optionalCount
            ? order[i].getIntValue() : i + baseLanes, std::memory_order_relaxed);
    const auto glide = juce::StringArray::fromTokens(
        state.getProperty("PnordeAftertouchGlide", "").toString(), ",", "");
    for (int i = 0; i < maxSteps; ++i)
        // No saved glide parameter means the original automatic pressure ramp.
        setAftertouchGlide(i, glide.size() == maxSteps
            ? (state.hasProperty("PnordeAftertouchGlideUnits") || glide[i].getIntValue() < 0
                ? glide[i].getIntValue() : glide[i].getIntValue() + 101)
            : -1);
    for (int row = modRow; row <= sustainRow; ++row)
        setDedicatedEnabled(row, static_cast<bool>(state.getProperty("PnordeDedicated" + juce::String(row), false)));
    const auto vibDepth = juce::StringArray::fromTokens(
        state.getProperty("PnordeVibratoDepth", "").toString(), ",", "");
    for (int i = 0; i < maxSteps; ++i)
        setVibratoDepth(i, vibDepth.size() == maxSteps ? vibDepth[i].getIntValue() : 0);
    // New optional clock parameters must not leak from the previously loaded
    // project into pre-v0.5.8 saved projects that don't know about them.
    if (stateVersion < 11)
    {
        setManualTempoEnabled(false);
        setManualTempoBpm(120.0);
    }
    if (static_cast<bool>(state.getProperty("PnordeOctaveV2Used",false)))
        setGlobalOctave(static_cast<int>(state.getProperty("PnordeGlobalOctave",0)));
    else
    {
        pnordeSetAutomatable(parameters,"pn_octave_v2_active",0);
        pnordeSetAutomatable(parameters,"pn_octave",juce::jlimit(-2,2,
            static_cast<int>(state.getProperty("PnordeGlobalOctave",0))));
    }
    setKey(static_cast<int>(state.getProperty("NovaKey", 0)));
    // An unversioned old project is intentionally restored to Free 0.1.
    // A versioned project retains its exact saved scale; none of its source
    // pitch steps are ever requantised by this migration.
    const int savedScale = static_cast<int>(state.getProperty(
        "NovaScale", PnordeProjectVersion::newInstanceMajorScale));
    setScale(PnordeProjectVersion::restoredScale(hasSchema, savedScale));
    setGlobalClockIndex(static_cast<int>(state.getProperty("NovaGlobalClock", 2)));
    setBendRangeIndex(static_cast<int>(state.getProperty("NovaBendRange", 2)));
    setDefaultMidiChannel(static_cast<int>(state.getProperty("PnordeDefaultMidiChannel", 1)));
    // Older 3C.1A projects had the channel lane permanently enabled.
    setMidiChannelLaneEnabled(static_cast<bool>(state.getProperty("PnordeChannelLaneEnabled",
        state.hasProperty("PnordeDefaultMidiChannel"))));
    setCustomCCCount(static_cast<int>(state.getProperty("NovaCustomCCCount", 0)));
    for (int i = 0; i < optionalCount; ++i)
    {
        setCustomCCNumber(i, static_cast<int>(state.getProperty("NovaCustomCC" + juce::String(i),
                                                    getCustomCCNumber(i))));
        setCustomCCLabel(i, state.getProperty("NovaCustomCCLabel" + juce::String(i), "").toString());
        if (i < maxCustomCC)
        {
            setMacroType(i, 0);
            setMacroChannel(i, static_cast<int>(state.getProperty("PnordeMacroChannel" + juce::String(i), 0)));
        }
        laneTypes[i].store(static_cast<int>(PnordeLane::defaultType(i + baseLanes)));
        const auto tempoGlides = juce::StringArray::fromTokens(
            state.getProperty("PnordeOptionalGlide" + juce::String(i), "").toString(), ",", "");
        for (int j = 0; j < maxSteps; ++j)
            setOptionalGlide(i + baseLanes, j, tempoGlides.size() == maxSteps ? tempoGlides[j].getIntValue() : 0);
        const auto transitions = juce::StringArray::fromTokens(
            state.getProperty("NovaCCGlidePercent" + juce::String(i), "").toString(), ",", "");
        const auto legacy = juce::StringArray::fromTokens(
            state.getProperty("NovaCCTransition" + juce::String(i), "").toString(), ",", "");
        for (int j = 0; j < maxSteps; ++j)
            setCCTransitionMs(i, j, transitions.size() == maxSteps
                ? transitions[j].getIntValue() : legacy.size() == maxSteps
                ? juce::jlimit(0, 100, juce::roundToInt(legacy[j].getIntValue() / 1.25)) : 0);
    }
    // Types are applied BEFORE step values so the correct Pan (0..127),
    // MIDI-channel (0..16), and CC (0..100) bounds are used on import.
    // Type allocation is validated before mutation, and all rows use the
    // shared PnordeLane dispatch model regardless of their original slot.
    for (int i = 0; i < optionalCount; ++i)
    {
        int kind = static_cast<int>(state.getProperty(
            "PnordeOptionalType" + juce::String(i),
            static_cast<int>(PnordeLane::defaultType(i + baseLanes))));
        if (kind < 0 || kind > 6) kind = static_cast<int>(PnordeLane::defaultType(i + baseLanes));
        laneTypes[i].store(kind, std::memory_order_relaxed);
    }
    const int savedVersion = static_cast<int>(state.getProperty("NovaLaneDataVersion", 1));
    for (int row = 1; row < laneCount; ++row)
    {
        const int savedRow = savedVersion < 6 && row == channelLane ? (savedVersion < 5 ? 11 : 15) : row;
        const auto prefix = "NovaLane" + juce::String(savedRow);
        if (savedVersion < 6 && row >= (savedVersion < 5 ? 11 : 15) && row < channelLane) continue;
        setLaneLength(row, static_cast<int>(state.getProperty(prefix + "Length",
                                  row == 1 ? 7 : row == 2 ? 5 : row == 3 ? 3 : 16)));
        setLaneDivision(row, static_cast<int>(state.getProperty(prefix + "Division", 2)));
        setLaneTiming(row, static_cast<int>(state.getProperty(prefix + "Timing", 0)));
        setLaneDirection(row, static_cast<int>(state.getProperty(prefix + "Direction",0)));
        // Rotation is a separate saved offset, NOT a change to source values.
        setLaneRotation(row, static_cast<int>(state.getProperty(
            "PnordeLaneRotation" + juce::String(row), 0)));
        const auto values = juce::StringArray::fromTokens(
            state.getProperty(prefix + "Values").toString(), ",", "");
        if (values.size() == maxSteps)
            for (int i = 0; i < maxSteps; ++i)
                setLaneValue(row, i, row == 1 && savedVersion < 2 ? values[i].getIntValue() * 10 : values[i].getIntValue());
    }

    // Restore start markers only AFTER every saved lane length has been loaded.
    // v0.6.10 and earlier have no PnordeLaneStart property: step 1 remains
    // the migration default. A corrupt oversized offset is bounded safely.
    for (int row = 0; row < laneCount; ++row)
        setLaneStartStep(row, static_cast<int>(
            state.getProperty("PnordeLaneStart" + juce::String(row), 0)));

    // Apply probabilities only after validation. ALL old songs/presets
    // automatically get the default 100% and therefore sound unchanged.
    for (int row = 0; row < probabilityRows; ++row)
    {
        const auto property = "PnordeProbability" + juce::String(row);
        const auto parts = juce::StringArray::fromTokens(
            state.getProperty(property, "").toString(), ",", "");
        for (int step = 0; step < maxSteps; ++step)
            setCellProbability(row, step,
                parts.size() == maxSteps ? parts[step].getIntValue() : 100);
    }
    // Do not touch probabilityLastTick here: it belongs exclusively to
    // processBlock's audio thread. Next visited tick will evaluate its roll.
    // SEQ (lane 0) has no numeric "NovaLane0" group; restore rotation here.
    setLaneRotation(0, static_cast<int>(state.getProperty("PnordeLaneRotation0", 0)));
    const juce::String patternData =
        state.getProperty("NovaPattern").toString();

    if (patternData.length() >= 256)
    {
        for (int i = 0; i < 256; ++i)
        {
            setStepEnabled(i, patternData[i] == '1');
        }
    }
    return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PolyrhythmMIDIProcessor();
}