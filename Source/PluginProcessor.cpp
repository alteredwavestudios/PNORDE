#include "PluginProcessor.h"
#include "PnordeSafeCC.h"
#include "PnordeVisualOrder.h"
#include "PnordeAftertouchGlide.h"
#include "PnordeTempoGlide.h"
#include "PnordeManualTempo.h"
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

    void setPort(std::shared_ptr<juce::MidiOutput> next)
    {
        std::atomic_store(&port, std::move(next));
        // Flush pending messages so events from the old port don't leak to the new one.
        // The worker owns the read index; it notices the generation change.
        generation.fetch_add(1, std::memory_order_release);
        notify();
    }
    bool active() const { return std::atomic_load(&port) != nullptr; }
    void enqueue(const juce::MidiMessage& msg, double dueMs)
    {
        if (msg.getRawDataSize() < 1 || msg.getRawDataSize() > 3) return;
        const auto w = write.load(std::memory_order_relaxed);
        const auto next = (w + 1) % capacity;
        if (next == read.load(std::memory_order_acquire)) return;
        auto& e = events[w];
        e.dueMs = dueMs; e.size = msg.getRawDataSize();
        for (int i = 0; i < e.size; ++i) e.data[i] = msg.getRawData()[i];
        write.store(next, std::memory_order_release);
        notify();
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
            if (r == write.load(std::memory_order_acquire)) { wait(2); continue; }
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
    for (auto& step : pattern)
        step.store(false, std::memory_order_relaxed);
    for (auto& glide : aftertouchGlide)
        glide.store(0, std::memory_order_relaxed);
    for (auto& row : optionalGlides)
        for (auto& glide : row) glide.store(0, std::memory_order_relaxed);

    for (int row = 0; row < laneCount; ++row)
    {
        laneLengths[row].store(row == 0 ? 16 : row == 1 ? 7 : row == 2 ? 5 : row == 3 ? 3 : 16);
        laneDivisions[row].store(2);
        laneTimings[row].store(0);
        laneCurrentSteps[row].store(-1);
        for (int i = 0; i < maxSteps; ++i)
            laneValues[row][i].store(row == channelLane || row == 1 || row == 4 || row == 5 || row == 6 || row >= 7 ? (getOptionalLaneType(row) == 4 ? 64 : 0) : row == 2 ? 70 : row == 3 ? 50 : 80);
    }

    // Restore the familiar original 16-step pattern.
    for (int i = 0; i < 16; i += 2)
        pattern[static_cast<size_t>(i)].store(
            true, std::memory_order_relaxed);
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
    addInt("pn_darkness", "Darkness", 0, 75, 0);
    addInt("pn_theme", "Colour Theme", 0, 7, 0);
    addInt("pn_key", "Key", 0, 11, 0);
    addInt("pn_scale", "Scale", 0, 13, 0);
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
    return layout;
}

namespace
{
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
{ pnordeSetAutomatable(parameters, "pn_octave", juce::jlimit(-2, 2, v)); }
void PolyrhythmMIDIProcessor::setGlobalClockIndex(int v) noexcept
{ pnordeSetAutomatable(parameters, "pn_clock", juce::jlimit(0, 4, v)); }
void PolyrhythmMIDIProcessor::setBendRangeIndex(int v) noexcept
{ pnordeSetAutomatable(parameters, "pn_bend", juce::jlimit(0, 3, v)); }
void PolyrhythmMIDIProcessor::setThemeIndex(int v) noexcept
{ pnordeSetAutomatable(parameters, "pn_theme", juce::jlimit(0, 7, v)); }
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
            pnordeSetAutomatable(parameters, "pn_scale_ex", v - (PnordeScales::legacyCount - 1));
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

void PolyrhythmMIDIProcessor::prepareToPlay(
    double sampleRate,
    int)
{
    currentSampleRate = sampleRate;
    noteIsActive = false;
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
    lastOptionalType.fill(-1);
    dedicatedTargetStep.fill(-1); dedicatedController.fill(-1);
    dedicatedSent.fill(-1); dedicatedSentChannel.fill(-1);
    for (auto& step : laneCurrentSteps) step.store(-1);
    currentStep.store(-1, std::memory_order_relaxed);
    hostPlaying.store(false, std::memory_order_relaxed);
    hostPpq.store(0.0, std::memory_order_relaxed);
    hostClockUpdateMs.store(0.0, std::memory_order_relaxed);
    hostBpm.store(120.0, std::memory_order_relaxed);
}

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

void PolyrhythmMIDIProcessor::requestPlay()
{
    // Ableton's transport controls playback.
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
}

int PolyrhythmMIDIProcessor::getLaneLength(int row) const noexcept
{
    return row >= 0 && row < laneCount ? laneLengths[row].load(std::memory_order_relaxed) : 16;
}
void PolyrhythmMIDIProcessor::setLaneLength(int row, int length) noexcept
{
    if (row < 0 || row >= laneCount) return;
    if (row == 0) { setSequenceLength(length); return; }
    laneLengths[row].store(juce::jlimit(1, maxSteps, length), std::memory_order_relaxed);
}
int PolyrhythmMIDIProcessor::getLaneDivision(int row) const noexcept
{
    return row >= 0 && row < laneCount ? laneDivisions[row].load(std::memory_order_relaxed) : 2;
}
void PolyrhythmMIDIProcessor::setLaneDivision(int row, int division) noexcept
{
    if (row < 0 || row >= laneCount) return;
    if (row == 0) { setClockDivisionIndex(division); return; }
    laneDivisions[row].store(juce::jlimit(0, 10, division), std::memory_order_relaxed);
}
int PolyrhythmMIDIProcessor::getLaneValue(int row, int step) const noexcept
{
    if (row < 1 || row >= laneCount || step < 0 || step >= maxSteps) return 0;
    return laneValues[row][step].load(std::memory_order_relaxed);
}
void PolyrhythmMIDIProcessor::setLaneValue(int row, int step, int value) noexcept
{
    if (row < 1 || row >= laneCount || step < 0 || step >= maxSteps) return;
    const int kind = getOptionalLaneType(row);
    laneValues[row][step].store(juce::jlimit(row == 1 ? -120 : row == 4 ? -4 : row >= 5 ? 0 : 1,
                                             kind == 6 ? 16 : kind == 4 ? 127 : row == 1 ? 120 : row == 4 ? 4 : 100, value),
                                std::memory_order_relaxed);
}
int PolyrhythmMIDIProcessor::quantizePitchTenths(int tenths, int octave) const noexcept
{
    tenths = juce::jlimit(-120, 120, tenths);
    octave = juce::jlimit(-4, 4, octave);
    const int mode = getScale();
    if (mode == 13 || mode == 0) return tenths;
    if (mode >= PnordeScales::legacyCount)
        return PnordeScales::quantiseExtra(tenths, octave, getKey(), mode);
    // Quantize absolute pitch classes relative to selected key, keeping octave offsets.
    static constexpr int masks[] = { 0xfff, 0xab5, 0x5ad, 0x6ad, 0x5ab, 0x56b, 0x4e9, 0x29d, 0xebd, 0x9ad, 0x18b, 0xaad, 0x6b5 };
    const int keyPc = getKey();
    const int base = 60 + octave * 12;
    const int target = base + static_cast<int>(std::round(tenths / 10.0));
    int best = target, distance = 999;
    for (int candidate = juce::jmax(0, target - 12); candidate <= juce::jmin(127, target + 12); ++candidate)
    {
        const int pc = ((candidate - keyPc) % 12 + 12) % 12;
        if ((masks[mode] & (1 << pc)) == 0) continue;
        const int d = std::abs(candidate * 10 - (base * 10 + tenths));
        if (d < distance) { distance = d; best = candidate; }
    }
    return juce::jlimit(-120, 120, (best - base) * 10);
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
    setLaneLength(row, juce::jmax(1, length - removed));
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
        const int dest = i + baseLanes, src = dest + 1;
        laneLengths[dest].store(laneLengths[src].load());
        laneDivisions[dest].store(laneDivisions[src].load());
        laneTimings[dest].store(laneTimings[src].load());
        for (int j = 0; j < maxSteps; ++j) laneValues[dest][j].store(laneValues[src][j].load());
    }
    if (getOptionalLaneType(baseLanes + count - 1) == 3)
        for (int step = 0; step < maxSteps; ++step) setVibratoDepth(step, 0);
    setCustomCCLabel(count - 1, {});
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
    return row >= 0 && row < laneCount ? laneTimings[row].load(std::memory_order_relaxed) : 0;
}
void PolyrhythmMIDIProcessor::setLaneTiming(int row, int mode) noexcept
{
    if (row >= 0 && row < laneCount)
        laneTimings[row].store(juce::jlimit(0, 2, mode), std::memory_order_relaxed);
}

int PolyrhythmMIDIProcessor::getLaneCurrentStep(int row) const noexcept
{
    if (row < 0 || row >= laneCount) return -1;
    return laneCurrentSteps[row].load(std::memory_order_relaxed);
}

void PolyrhythmMIDIProcessor::processBlock(
    juce::AudioBuffer<float>& audio, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    audio.clear();
    const int numSamples = audio.getNumSamples();
    if (numSamples <= 0) return;

    const bool external = isExternalMidiOutputActive();
    const double blockStartMs = juce::Time::getMillisecondCounterHiRes();
    const auto emit = [&](const juce::MidiMessage& message, int offset)
    {
        const int sample = juce::jlimit(0, numSamples - 1, offset);
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
        dedicatedSent.fill(-1); dedicatedSentChannel.fill(-1);
        dedicatedTargetStep.fill(-1); dedicatedController.fill(-1);
        nextModulationPpq = 0.0;
        previousBlockWasPlaying = false;
        previousManualMode = false;
        mpeZoneConfigured = false;
        previousMicroMode = -1;
        hostPlaying.store(false, std::memory_order_relaxed);
        hostPpq.store(0.0, std::memory_order_relaxed);
    hostClockUpdateMs.store(0.0, std::memory_order_relaxed);
        currentStep.store(-1, std::memory_order_relaxed);
        for (auto& s : laneCurrentSteps) s.store(-1, std::memory_order_relaxed);
    };

    const auto* playhead = getPlayHead();
    if (playhead == nullptr) { reset(); return; }
    const auto position = playhead->getPosition();
    if (!position.hasValue() || !position->getIsPlaying()) { reset(); return; }
    const auto ppq = position->getPpqPosition();
    const auto bpm = position->getBpm();
    if (!ppq.hasValue() || !bpm.hasValue() || !std::isfinite(*ppq) ||
        std::abs(*ppq) > 1.0e12 || !std::isfinite(*bpm) || *bpm > 1000.0 ||
        *bpm <= 0.0 || !std::isfinite(currentSampleRate) || currentSampleRate <= 0.0) { reset(); return; }

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
    const double start = beats.start;
    const double end = beats.end;
    const double perSample = effectiveBpm / (60.0 * currentSampleRate);
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
        dedicatedSent.fill(-1); dedicatedSentChannel.fill(-1);
        dedicatedTargetStep.fill(-1); dedicatedController.fill(-1);
        nextModulationPpq = start;
        glideStartPpq = glideEndPpq = start;
        pressureStartPpq = pressureEndPpq = start;
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
        const auto tick = static_cast<juce::int64>(std::floor(at / laneDuration(row) + eps));
        const int len = getLaneLength(row);
        return static_cast<int>((tick % len + len) % len);
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
                if (value != lastCCSent[slot])
                {
                    emit(juce::MidiMessage::controllerEvent(ch, number, value), offset);
                    lastCCSent[slot] = value;
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
            if (type == 1) emitDedicated(0, 1, juce::roundToInt(vertical * 1.27f));
            else if (type == 2) emitDedicated(0, 11, juce::roundToInt(vertical * 1.27f));
            else if (type == 3)
            {
                emitDedicated(0, 76, juce::roundToInt(vertical * 1.27f));
                emitDedicated(1, 77, juce::roundToInt(getVibratoDepth(step) * 1.27f));
            }
            else if (type == 4) emitDedicated(0, 10, vertical);
            else if (type == 5) emitDedicated(0, 64, vertical >= 50 ? 127 : 0);
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
    auto tick = static_cast<juce::int64>(std::ceil(start / seqDuration - eps));
    double boundary = tick * seqDuration;
    // 200 Hz bend/pressure sampling; event offsets remain sample-accurate.
    const double modInterval = juce::jmax(perSample, effectiveBpm / 12000.0);
    if (nextModulationPpq < start || nextModulationPpq > end + 1.0)
        nextModulationPpq = start;

    // Merge trigger, gate-off and modulation events chronologically.
    int guard = 0;
    while (++guard < 200000)
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
            const int index = static_cast<int>((tick % seqLength + seqLength) % seqLength);
            currentStep.store(index, std::memory_order_relaxed);
            if (getStepEnabled(index))
            {
                const int octave = getLaneValue(4, stepAt(4, boundary));
                const int rawPitch = getLaneValue(1, stepAt(1, boundary));
                const int tenths = quantizePitchTenths(rawPitch, octave);
                // Exact EDO semitones are carried through the pitch-bend path.
                // Pitch-lane storage remains legacy tenths (0.1 semitone), so
                // converting stored data directly to a note would lose up to
                // five cents in 17/19/22/31-EDO.
                const double fineTenths = PnordeEDO::isMode(getScale())
                    ? PnordeEDO::quantiseSemitones(rawPitch,octave,getKey(),getScale())
                    : tenths / 10.0;
                const double targetSemis = juce::jlimit(0.0,127.0,
                    60.0+(octave+getGlobalOctave())*12.0+fineTenths);
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
                glideEndPpq = boundary + PnordeMicroOutput::slideApproachBeats(
                    slide, seqDuration, gate);
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
                noteOffPpq = boundary + seqDuration * gate / 100.0;
                lastAftertouchStep = -1; // Begin pressure ramp from current value on the new voice.
                nextModulationPpq = boundary;
            }
            else if (noteIsActive) noteOff(offset);
            ++tick;
            boundary = tick * seqDuration;
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
    const double lastPpq = start + (numSamples - 1) * perSample;
    for (int row = 0; row < laneCount; ++row)
    {
        const int step = stepAt(row, lastPpq);
        laneCurrentSteps[row].store(step, std::memory_order_relaxed);
        if (row == 0) currentStep.store(step, std::memory_order_relaxed);
    }
}

juce::AudioProcessorEditor*
PolyrhythmMIDIProcessor::createEditor()
{
    return new PolyrhythmMIDIEditor(*this);
}

void PolyrhythmMIDIProcessor::getStateInformation(
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

    // Versioned lane data; older saved projects still load the original SEQ properties.
    state.setProperty("NovaLaneDataVersion", 12, nullptr);
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

namespace
{
// Preflight untrusted state BEFORE JUCE's XML parser is invoked. Reject XML
// declarations that could introduce entities/DTDs, excessively deep or large
// trees, and invalid tags. The test is intentionally conservative because
// PNORDE generates a shallow, predictable state tree.
bool pnordeStateEnvelopeIsSafe(const void* data, int size) noexcept
{
    if (data == nullptr || size < 16 || size > 1024 * 1024) return false;
    const auto* bytes = static_cast<const unsigned char*>(data);
    int tagCount = 0, depth = 0;
    for (int i = 0; i + 1 < size; ++i)
    {
        if (bytes[i] != '<') continue;
        const unsigned char next = bytes[i + 1];
        if (next == '!') return false; // DTD, ENTITY, CDATA, comments
        if (next == '?') continue; // ordinary XML declaration
        const bool closing = next == '/';
        if (++tagCount > 256) return false;
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

bool pnordeStateTreeIsSafe(const juce::ValueTree& state)
{
    if (!state.isValid() || state.getNumChildren() > 32 || state.getNumProperties() > 256)
        return false;
    for (int p = 0; p < state.getNumProperties(); ++p)
    {
        if (state.getPropertyName(p).toString().length() > 96 ||
            state.getProperty(state.getPropertyName(p)).toString().length() > 8192)
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

void PolyrhythmMIDIProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Hosts can supply state too, so apply the same limits as preset imports.
    (void) loadValidatedState(data, sizeInBytes);
}

bool PolyrhythmMIDIProcessor::loadValidatedState(const void* data, int sizeInBytes)
{
    if (!pnordeStateEnvelopeIsSafe(data, sizeInBytes)) return false;
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml == nullptr || !xml->hasTagName(parameters.state.getType()))
        return false;
    auto state = juce::ValueTree::fromXml(*xml);
    if (!pnordeStateTreeIsSafe(state)) return false;
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
    // All validation happens before modifying any live sequencer parameters.
    parameters.replaceState(state);

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
    setExperimentalLightsEnabled(static_cast<bool>(state.getProperty("PnordeExperimentalLights", false)));
    setExperimentalReorderEnabled(static_cast<bool>(state.getProperty("PnordeExperimentalReorder", false)));
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
    setGlobalOctave(static_cast<int>(state.getProperty("PnordeGlobalOctave", 0)));
    setKey(static_cast<int>(state.getProperty("NovaKey", 0)));
    setScale(static_cast<int>(state.getProperty("NovaScale", 0)));
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
        const auto values = juce::StringArray::fromTokens(
            state.getProperty(prefix + "Values").toString(), ",", "");
        if (values.size() == maxSteps)
            for (int i = 0; i < maxSteps; ++i)
                setLaneValue(row, i, row == 1 && savedVersion < 2 ? values[i].getIntValue() * 10 : values[i].getIntValue());
    }

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