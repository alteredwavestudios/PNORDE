// ============================================================================
// PluginProcessor.h — The engine interface and persistent fields
// ============================================================================
// Declares the processor used by JUCE, plus pattern, parameter, MIDI and state-management members. The editor talks to this object; it does not own the audio clock.
//
// Beginner notes:
//   - A header (.h) usually DECLARES names, types and helper functions.
//   - A source file (.cpp) usually IMPLEMENTS behaviour.
//   - `noexcept` promises an operation will not throw C++ exceptions.
//   - `const` generally indicates a value cannot be modified through that use.
//   - Avoid changing audio-thread behaviour merely to add documentation.
// See docs/SOURCE_GUIDE.md for terms, architecture and call-flow diagrams.
// ============================================================================

#pragma once

#include <JuceHeader.h>
#include <cstdint>
#include <atomic>
#include <array>
#include <memory>
#include "PnordeLane.h"
#include "PnordeScales.h"
#include "PnordeMicroOutput.h"
#include "PnordePlayback.h"
#include "PnordeGlobalReverse.h"
#include "PnordeSwing.h"
#include "PnordeRotation.h"
#include "PnordeDeveloperStats.h"
#include "PnordeMidiBlackBox.h"

class PolyrhythmMIDIProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    PolyrhythmMIDIProcessor();
    ~PolyrhythmMIDIProcessor() override;

    void prepareToPlay(double sampleRate,
                       int samplesPerBlockExpected) override;

    void releaseResources() override;

    bool isBusesLayoutSupported(
        const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>& audio,
                      juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override
    {
        return JucePlugin_Name;
    }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }

    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(
        juce::MemoryBlock& destData) override;

    void setStateInformation(
        const void* data, int sizeInBytes) override;

    // Validates a bounded native state before mutating live sequencer data.
    // Intended for loading user-controlled .pnorde files on the message thread.
    bool loadValidatedState(const void* data, int sizeInBytes);

    // Experimental, opt-in diagnostics: NOT saved to projects, NOT automated.
    // String creation happens only when the Settings UI requests a report.
    bool isDeveloperModeEnabled() const noexcept { return developerStats.enabled(); }
    void setDeveloperModeEnabled(bool on) noexcept { developerStats.setEnabled(on); }
    void resetDeveloperStats() noexcept
    {
        developerStats.reset();
        midiBlackBox.requestReset(); // Audio thread will reset its own history.
    }
    juce::String getDeveloperReport() const;

    // Four independent complete scenes. 0..3, host-automatable as "PNORDE Program".
    // Programme changes are applied on the JUCE message thread, never by
    // serialising XML or allocating storage in the real-time audio callback.
    static constexpr int programCount = 4;
    int getSelectedProgram() const noexcept;
    void selectProgram(int index);
    void applyPendingProgramChange(int destination);
    // Program-bank operations run on the message thread. Copy/reset are atomic
    // at the bank level, never on the real-time audio callback.
    bool copyProgram(int source, int destination);
    bool resetProgram(int index);

    // A PROGRAM trigger is active WHENEVER THE PROGRAM LANE IS ON.
    // There is no second AUTO/MANUAL switch to remember. Selecting a Program
    // manually changes it now; the next enabled PROGRAM cell can change it
    // again, just like any other sequenced automation.
    //
    // We keep the old AutoArmed method names as BACKWARD-COMPATIBLE ALIASES:
    // older source code and old project-state attributes may still use them,
    // but they no longer represent a separate user-facing setting.
    bool getProgramLaneEnabled() const noexcept
    { return programLaneEnabled.load(std::memory_order_acquire); }
    void setProgramLaneEnabled(bool enabled) noexcept
    {
        programLaneEnabled.store(enabled, std::memory_order_release);
        // Mirror the historic AUTO flag purely for old state compatibility.
        programLaneAutoArmed.store(enabled, std::memory_order_release);
        // Discard any message prepared under the previous enabled state.
        pendingSequencedProgram.store(0, std::memory_order_release);
        manualProgramEpoch.fetch_add(1, std::memory_order_acq_rel);
        programLaneRestartRequested.store(true, std::memory_order_release);
    }
    bool getProgramLaneAutoArmed() const noexcept
    { return getProgramLaneEnabled(); }
    void setProgramLaneAutoArmed(bool armed) noexcept
    { setProgramLaneEnabled(armed); }
    // The PROGRAM row can rotate too. Rotation never alters saved step data.
    int getProgramLaneRotation() const noexcept;
    void setProgramLaneRotation(int steps) noexcept;
    int getProgramLaneValue(int step) const noexcept;
    void setProgramLaneValue(int step, int value) noexcept;
    int getProgramLaneLength() const noexcept;
    void setProgramLaneLength(int length) noexcept;
    int getProgramLaneDivision() const noexcept;
    void setProgramLaneDivision(int value) noexcept;
    int getProgramLaneTiming() const noexcept;
    void setProgramLaneTiming(int value) noexcept;
    int getProgramLaneStart() const noexcept;
    void setProgramLaneStart(int value) noexcept;
    int getProgramLaneCurrentStep() const noexcept;
    float getProgramLaneProgress() const noexcept;
    bool duplicateLanePattern(int row); // doubles all relevant step data (<=128)


    // External routing: empty identifier = host MIDI, otherwise a Windows MIDI output.
    juce::Array<juce::MidiDeviceInfo> getAvailableMidiOutputs() const;
    juce::String getMidiOutputIdentifier() const;
    juce::String setMidiOutputIdentifier(const juce::String& identifier);
    bool isExternalMidiOutputActive() const noexcept;
    // Diagnostic counter (read from the UI); no host-visible parameter added.
    unsigned getExternalMidiDroppedEvents() const noexcept;

    void requestPlay();
    bool getPlayEnabled() const noexcept
    { return parameters.getRawParameterValue("pn_play_enabled")->load() >= 0.5f; }
    void setPlayEnabled(bool enabled) noexcept;
    void requestSequenceReset() noexcept;
    void requestLaneReset(int row) noexcept;
    int getLaneResetCounter(int row) const noexcept;
    double getLanePhaseOrigin(int row) const noexcept;
    int getHppdFps() const noexcept
    { return juce::jlimit(1,60,static_cast<int>(parameters.getRawParameterValue(
        "pn_hppd_fps")->load(std::memory_order_relaxed))); }
    void setHppdFps(int fps) noexcept;
    int getVibesBleed() const noexcept
    { return static_cast<int>(parameters.getRawParameterValue("pn_vibes_bleed")->load(std::memory_order_relaxed)); }
    void setVibesBleed(int amount) noexcept;
    int getResetCounter() const noexcept
    { return static_cast<int>(parameters.getRawParameterValue("pn_reset_counter")->load()); }
    bool getGlobalReverse() const noexcept
    { return parameters.getRawParameterValue("pn_global_reverse")->load(std::memory_order_relaxed) >= 0.5f; }
    void setGlobalReverse(bool on) noexcept;
    // 0=Forward, 1=Backward, 2=Ping Pong, 3=Random.
    // RANDOM overrides (but does not erase) the older host Direction value.
    // +1 moves the pattern right by one cell, but leaves original cells alone.
    // These offsets belong to the selected Quick Program and the undo history.
    int getLaneRotation(int row) const noexcept;
    void setLaneRotation(int row, int steps) noexcept;
    int getLaneDirection(int row) const noexcept;
    void setLaneDirection(int row, int direction) noexcept;
    bool getLowPitchAnchor() const noexcept { return lowPitchAnchor.load(std::memory_order_relaxed); }
    bool getPsychedelicEnabled() const noexcept { return psychedelicEnabled.load(std::memory_order_relaxed); }
    void setPsychedelicEnabled(bool enabled) noexcept { psychedelicEnabled.store(enabled,std::memory_order_relaxed); }
    // HPPD modes: 0 = Neon Nodal, 1 = Phase Weave, 2 = Prism Spikes, 3 = Kumiko Checker, 4 = Chromatic Drift.
    int getVibesMode() const noexcept { return vibesMode.load(std::memory_order_relaxed); }
    void setVibesMode(int mode) noexcept
    { vibesMode.store(juce::jlimit(0, 4, mode), std::memory_order_relaxed); }
    int getPsychedelicIntensity() const noexcept { return psychedelicIntensity.load(std::memory_order_relaxed); }
    void setPsychedelicIntensity(int percent) noexcept
    { psychedelicIntensity.store(juce::jlimit(0,40,percent),std::memory_order_relaxed); }
    bool getVisualGlowEnabled() const noexcept { return visualGlowEnabled.load(std::memory_order_relaxed); }
    void setVisualGlowEnabled(bool enabled) noexcept { visualGlowEnabled.store(enabled, std::memory_order_relaxed); }


    bool getStepEnabled(int index) const noexcept;
    void setStepEnabled(int index, bool enabled) noexcept;

    int getSequenceLength() const noexcept;
    void setSequenceLength(int length) noexcept;

    int getClockDivisionIndex() const noexcept;
    void setClockDivisionIndex(int index) noexcept;

    int getCurrentStep() const noexcept
    {
        return currentStep.load(std::memory_order_relaxed);
    }

    static constexpr int laneCount = 21;
    static constexpr int modRow = 15, expressionRow = 16, vibratoRow = 17;
    static constexpr int panRow = 18, sustainRow = 19, channelLane = 20;
    static constexpr int dedicatedCount = 5;
    static constexpr int baseLanes = 7;
    static constexpr int maxCustomCC = 8;
    static constexpr int optionalCount = PnordeLane::rowCount;
    using LaneType = PnordeLane::Type;
    bool isOptionalLaneEnabled(int row) const noexcept;
    int getOptionalLaneType(int row) const noexcept;
    bool setOptionalLaneType(int row, int type) noexcept;
    bool hasActiveLaneType(int type, int excludingRow = -1) const noexcept;
    int activeCCMacroCount() const noexcept;
    int addOptionalLane(int type) noexcept;
    void removeOptionalLane(int row) noexcept;
    int getCustomCCCount() const noexcept { return customCCCount.load(); }
    void setCustomCCCount(int count) noexcept { customCCCount.store(juce::jlimit(0, 8, count)); }
    // 0 = custom CC, 1 = expression (CC11), 2 = mod wheel (CC1),
    // 3 = vibrato rate (CC76), 4 = vibrato depth (CC77).
    bool isDedicatedEnabled(int row) const noexcept;
    void setDedicatedEnabled(int row, bool enabled) noexcept;
    int getVibratoDepth(int step) const noexcept;
    void setVibratoDepth(int step, int value) noexcept;
    int getDarknessPercent() const noexcept { return static_cast<int>(parameters.getRawParameterValue("pn_darkness")->load()); }
    void setDarknessPercent(int value) noexcept;
    bool getAnimationsEnabled() const noexcept { return parameters.getRawParameterValue("pn_animations")->load() >= 0.5f; }
    void setAnimationsEnabled(bool b) noexcept;
    bool getScrollFadesEnabled() const noexcept { return scrollFadesEnabled.load(); }
    void setScrollFadesEnabled(bool b) noexcept { scrollFadesEnabled.store(b); }
    bool getTooltipsEnabled() const noexcept { return tooltipsEnabled.load(); }
    void setTooltipsEnabled(bool b) noexcept { tooltipsEnabled.store(b); }
    void requestMidiPanic() noexcept { panicRequested.store(true); }
    bool getExperimentalLightsEnabled() const noexcept { return experimentalLights.load(std::memory_order_relaxed); }
    void setExperimentalLightsEnabled(bool yes) noexcept { experimentalLights.store(yes, std::memory_order_relaxed); }
    double getHostBpm() const noexcept { return hostBpm.load(std::memory_order_relaxed); }
    // 0: standard pitch bend; 1: experimental lower-zone MPE; 2: legacy
    // saved alias for standard. All microtonal scales use an effective +/-2
    // range and keep SLIDE as a tempo-relative pitch approach control.
    int getMicroOutputMode() const noexcept
    { return static_cast<int>(parameters.getRawParameterValue("pn_micro_mode")->load()); }
    void setMicroOutputMode(int mode) noexcept;
    bool getExperimentalPolyPressure() const noexcept
    { return parameters.getRawParameterValue("pn_poly_pressure")->load() >= 0.5f; }
    void setExperimentalPolyPressure(bool enabled) noexcept;
    bool isMicrotonalScaleSelected() const noexcept
    { return PnordeMicroOutput::isMicrotonalScale(getScale()); }
    int getEffectiveBendRangeIndex() const noexcept
    { return PnordeMicroOutput::effectiveBendRangeIndex(isMicrotonalScaleSelected(), getBendRangeIndex()); }
    // Diagnostics show messages constructed by PNORDE, NOT host delivery.
    int getLastPitchWheelValue() const noexcept { return lastPitchWheel.load(std::memory_order_relaxed); }
    int getPitchWheelSendCount() const noexcept { return pitchWheelSends.load(std::memory_order_relaxed); }
    int getLastEDOOffsetCents() const noexcept { return lastEDOOffsetCents.load(std::memory_order_relaxed); }
    bool getManualTempoEnabled() const noexcept
    { return parameters.getRawParameterValue("pn_manual_mode")->load() >= 0.5f; }
    void setManualTempoEnabled(bool enabled) noexcept;
    double getManualTempoBpm() const noexcept
    { return parameters.getRawParameterValue("pn_manual_bpm")->load(); }
    void setManualTempoBpm(double bpm) noexcept;
    int getMacroType(int slot) const noexcept;
    void setMacroType(int slot, int type) noexcept;
    int getMacroChannel(int slot) const noexcept; // 0 = follow notes (currently channel 1)
    void setMacroChannel(int slot, int channel) noexcept;
    int getCustomCCNumber(int slot) const noexcept;
    void setCustomCCNumber(int slot, int number) noexcept;
    void removeCustomCC(int slot) noexcept;
    // 0..100 = percentage of lane clock duration; -1 = older AUTO ramp.
    // 101..201 retain explicit seconds from v0.5.5-6.1 presets.
    int getAftertouchGlide(int step) const noexcept;
    void setAftertouchGlide(int step, int value) noexcept;
    // Visual row ordering never changes MIDI lane identity, data or automation.
    int getDisplayRowAt(int optionalPosition) const noexcept;
    void moveDisplayRow(int sourceRow, int targetRow) noexcept;
    bool getExperimentalReorderEnabled() const noexcept { return experimentalReorder.load(std::memory_order_relaxed); }
    void setExperimentalReorderEnabled(bool enabled) noexcept { experimentalReorder.store(enabled, std::memory_order_relaxed); }
    // Per-step tempo-relative glide for Mod Wheel, Expression, Vibrato and Pan.
    // Sustain, MIDI Channel and CC Macro keep their original separate behaviour.
    int getOptionalGlide(int row, int step) const noexcept;
    void setOptionalGlide(int row, int step, int value) noexcept;
    int getCCTransitionMs(int slot, int step) const noexcept;
    void setCCTransitionMs(int slot, int step, int ms) noexcept;
    juce::String getCustomCCLabel(int slot) const;
    // Purely visual, per-physical-lane name overrides. Empty means the existing
    // factory/default label, preserving projects saved before v0.6.8.
    juce::String getLaneName(int row) const;
    void setLaneName(int row, const juce::String& name);
    void setCustomCCLabel(int slot, const juce::String& label);
    static constexpr int maxSteps = 128;
    bool hasMidiChannelLane() const noexcept { return midiChannelLaneEnabled.load(std::memory_order_relaxed); }
    void setMidiChannelLaneEnabled(bool enabled) noexcept { midiChannelLaneEnabled.store(enabled, std::memory_order_relaxed); }
    int getDefaultMidiChannel() const noexcept { return static_cast<int>(parameters.getRawParameterValue("pn_channel")->load()); }
    void setDefaultMidiChannel(int ch) noexcept;
    int getLaneLength(int row) const noexcept;
    void setLaneLength(int row, int length) noexcept;
    int getLaneDivision(int row) const noexcept;
    void setLaneDivision(int row, int division) noexcept;
    // PROBABILITY (0..100, 100 means always). This is separate from the
    // musical cell value, so changing a percentage NEVER destroys a note.
    // Row 21 is the dedicated PROGRAM lane; rows 0..20 are regular lanes.
    static constexpr int probabilityRows = laneCount + 1;
    int getCellProbability(int row, int step) const noexcept;
    void setCellProbability(int row, int step, int percent) noexcept;
    int getLaneValue(int row, int step) const noexcept;
    void setLaneValue(int row, int step, int value) noexcept;
    int getLaneCurrentStep(int row) const noexcept;
    int getLaneStartStep(int row) const noexcept;
    void setLaneStartStep(int row, int step) noexcept;
    int getLaneTiming(int row) const noexcept;
    void setLaneTiming(int row, int mode) noexcept;
    // Pair-based per-lane swing. 50=straight, 66/67 ~ triplet feel, 75=3:1.
    // The SEQ lane delays actual note onset; expression lanes delay their
    // value changes independently. PROGRAM switching timing stays untouched.
    int getLaneSwing(int row) const noexcept;
    void setLaneSwing(int row, int percent) noexcept;
    // Global transpose is clamped at the final MIDI note (0..127).
    int getGlobalOctave() const noexcept
    { return parameters.getRawParameterValue("pn_octave_v2_active")->load() >= 0.5f
        ? static_cast<int>(parameters.getRawParameterValue("pn_octave_v2")->load())
        : static_cast<int>(parameters.getRawParameterValue("pn_octave")->load()); }
    void setGlobalOctave(int octave) noexcept;
    int getGlobalClockIndex() const noexcept { return static_cast<int>(parameters.getRawParameterValue("pn_clock")->load()); }
    void setGlobalClockIndex(int index) noexcept;
    int getBendRangeIndex() const noexcept { return static_cast<int>(parameters.getRawParameterValue("pn_bend")->load()); }
    void setBendRangeIndex(int index) noexcept;

    int getThemeIndex() const noexcept
    {
        const int extra = static_cast<int>(parameters.getRawParameterValue("pn_theme_extension")->load());
        return extra > 0 ? juce::jlimit(8, 11, 7 + extra)
                         : static_cast<int>(parameters.getRawParameterValue("pn_theme")->load());
    }
    int getHypnosis() const noexcept { return juce::jlimit(0, 100, static_cast<int>(parameters.getRawParameterValue("pn_hypnosis")->load())); }
    void setHypnosis(int n) noexcept;
    void setThemeIndex(int index) noexcept;
    int getKey() const noexcept { return static_cast<int>(parameters.getRawParameterValue("pn_key")->load()); }
    int getScale() const noexcept
    {
        const int edo = static_cast<int>(parameters.getRawParameterValue("pn_scale_edo")->load());
        if (edo > 0) return PnordeEDO::firstScaleIndex - 1 + edo;
        const int extended = static_cast<int>(parameters.getRawParameterValue("pn_scale_ex")->load());
        return extended > 0 ? PnordeScales::legacyCount - 1 + extended
                            : static_cast<int>(parameters.getRawParameterValue("pn_scale")->load());
    }
    void setKey(int value) noexcept;
    void setScale(int value) noexcept;
    int quantizePitchTenths(int tenths, int octave) const noexcept;
    // DISPLAY: relative interval after SCALE snapping; ignores KEY.
    double getDisplayedPitchOffsetSemitones(int storedTenths) const noexcept;
    // PLAYBACK: displayed relative interval + KEY transposition.
    double getEffectivePitchOffsetSemitones(int storedTenths) const noexcept;
    void deletePage(int row, int page) noexcept;
    void appendPage(int row) noexcept;

    double getHostPpq() const noexcept { return hostPpq.load(std::memory_order_relaxed); }
    double getHostClockUpdateMs() const noexcept { return hostClockUpdateMs.load(std::memory_order_relaxed); }

    bool isHostPlaying() const noexcept
    {
        return hostPlaying.load(std::memory_order_relaxed);
    }

    int getEditorWidth() const noexcept
    {
        return editorWidth.load(std::memory_order_relaxed);
    }

    int getEditorHeight() const noexcept
    {
        return editorHeight.load(std::memory_order_relaxed);
    }

    void setEditorSize(int width, int height) noexcept
    {
        editorWidth.store(
            juce::jlimit(800, 2800, width),
            std::memory_order_relaxed);

        editorHeight.store(
            juce::jlimit(760, 1620, height),
            std::memory_order_relaxed);
    }

    juce::AudioProcessorValueTreeState parameters;

private:
    void timerCallback() override;
    // Both external file loads and trusted Quick Program switches run through
    // the SAME complete state validation. Only decoding differs.
    bool loadValidatedTree(juce::ValueTree prepared);
    void captureCurrentProgram(juce::MemoryBlock& destData);
    juce::CriticalSection programMutex;
    std::array<juce::MemoryBlock, programCount> savedPrograms;
    juce::MemoryBlock factoryProgram; // captured once at construction for Reset
    // Per-program trigger patterns; visibility and AUTO/MANUAL status are global.
    // New instances start with the lane hidden and automatic changes disarmed.
    // Do not add a host parameter: preserve every existing automation ID.
    std::atomic<bool> programLaneEnabled {false};
    std::atomic<bool> programLaneAutoArmed {false};
    std::array<std::atomic<int>, maxSteps> programLaneCells {};
    std::atomic<int> programLaneLength {16}, programLaneDivision {2};
    std::atomic<int> programLaneTiming {0}, programLaneStart {0};
    std::atomic<int> programLaneCurrent {-1};
    std::atomic<float> programLaneProgress {0.0f};
    PnordeDeveloperStats::Counters developerStats;
    PnordeMidiBlackBox::Recorder midiBlackBox;
    // Message thread posts; audio thread consumes and writes a single trace marker.
    std::atomic<unsigned> midiTraceProgramMarker {0};
    // Approximate, diagnostic-only timestamp of the most recently queued
    // PROGRAM request. Never used to decide which musical program to select.
    std::atomic<std::uint64_t> programRequestTimestampUs {0};
    std::uint64_t switchQueueDelayUs = 0; // message-thread-only
    int switchSource = 2; // message-thread-only: 1=lane, 2=manual/host
    std::atomic<std::uint64_t> pendingSequencedProgram {0};
    std::atomic<std::uint32_t> manualProgramEpoch {1};
    int observedHostProgram = 0; // message-thread-only last requested host value
    // Audio-thread-only counter, advances with the retained beat clock.
    double programLaneOrigin = 0.0;
    double programLaneBeat = 0.0;
    std::atomic<bool> programLaneRestartRequested {false};
    double lastProgramLaneBoundary = -1.0;
    // The visual settings are global; bank loads must never change them.
    // Optional timing outputs are used ONLY by the developer report.
    // Ordinary callers retain their original two-argument interface.
    bool restoreProgramSnapshot(const juce::MemoryBlock& snapshot, int destination,
                                std::uint64_t* preparationUs = nullptr,
                                std::uint64_t* validatedLoadUs = nullptr);

    std::atomic<int> activeProgram {0};
    int snapshotTargetProgram = -1;
    bool applyingProgramSnapshot = false; // message/host state thread, protected by mutex
    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();

    double currentSampleRate = 44100.0;
    std::atomic<double> developerSampleRate {44100.0};
    std::atomic<int> developerBlockSize {0};

    // All cells default to 100%. Atomic integers permit the GUI to change a
    // percentage while the audio callback reads it without taking a lock.
    std::array<std::array<std::atomic<int>, maxSteps>, probabilityRows> cellProbabilities {};
    // AUDIO THREAD ONLY: memoise one chance decision per independent lane
    // clock tick. HeldStep points to the last successfully accepted cell.
    std::array<std::int64_t, laneCount> probabilityLastTick {};
    std::array<int, laneCount> probabilityHeldStep {};
    std::array<std::atomic<bool>, 256> pattern;

    std::atomic<int> sequenceLength { 16 };
    std::atomic<int> clockDivisionIndex { 2 };
    std::atomic<int> currentStep { -1 };

    std::atomic<bool> hostPlaying { false };
    std::atomic<double> hostPpq { 0.0 }, hostBpm { 120.0 };
    std::atomic<double> hostClockUpdateMs { 0.0 };

    std::atomic<int> editorWidth { 1560 };
    std::atomic<int> editorHeight { 980 };

    std::atomic<int> key { 0 }, scale { 0 }, themeIndex { 0 };
    // GUI writes only small atomic offsets; the audio callback just READS them.
    // Value 0 = unchanged pattern; values 1..length-1 = rotate right.
    std::array<std::atomic<int>, laneCount> laneRotations {};
    std::atomic<int> programLaneRotation { 0 };
    std::array<std::atomic<int>, laneCount> laneLengths;
    std::array<std::atomic<int>, laneCount> laneDivisions;
    std::array<std::atomic<int>, laneCount> laneTimings;
    std::array<std::atomic<float>*,laneCount> hostedDivisions {};
    std::array<std::atomic<float>*,laneCount> hostedTimings {};
    std::array<std::atomic<float>*,laneCount> hostedDirections {};
    // Each lane gets a NEW automation flag for RANDOM. We keep the original
    // 0..2 direction parameter unchanged so older Ableton automation survives.
    std::array<std::atomic<float>*,laneCount> hostedRandomDirections {};
    std::array<std::atomic<float>*,laneCount> hostedSwings {};
    std::atomic<int> globalClock { 2 }, bendRangeIndex { 2 };
    std::atomic<int> defaultMidiChannel { 1 };
    std::atomic<bool> midiChannelLaneEnabled { false };
    std::array<std::atomic<bool>, dedicatedCount> dedicatedEnabled {};
    std::array<std::atomic<int>, maxSteps> vibratoDepthValues {};
    std::atomic<int> darknessPercent { 0 };
    std::atomic<bool> animationsEnabled { true }, scrollFadesEnabled { true }, tooltipsEnabled { true };
    std::atomic<bool> panicRequested { false };
    std::atomic<bool> experimentalLights { true };
    std::atomic<bool> psychedelicEnabled { false };
    std::atomic<int> psychedelicIntensity { 20 };
    std::atomic<int> vibesMode { 4 };
    std::atomic<bool> visualGlowEnabled { true };
    std::atomic<bool> lowPitchAnchor { true };
    std::atomic<bool> experimentalReorder { false };
    std::atomic<int> lastPitchWheel { 8192 }, pitchWheelSends { 0 }, lastEDOOffsetCents { 0 };
    std::array<std::atomic<int>, optionalCount> displayOrder {};
    std::array<std::atomic<int>, maxSteps> aftertouchGlide {};
    std::array<std::array<std::atomic<int>, maxSteps>, optionalCount> optionalGlides {};
    std::atomic<int> customCCCount { 0 };
    std::array<std::atomic<int>, optionalCount> laneTypes {};
    std::array<std::atomic<int>, maxCustomCC> macroTypes {};
    std::array<std::atomic<int>, maxCustomCC> macroChannels {};
    std::array<std::atomic<int>, optionalCount> customCCNumbers {};
    mutable juce::SpinLock customCCLabelsLock;
    std::array<juce::String, optionalCount> customCCLabels;
    std::array<juce::String, laneCount> customLaneNames {};
    std::array<std::array<std::atomic<int>, maxSteps>, optionalCount> ccTransitionMs {};
    std::array<std::array<std::atomic<int>, maxSteps>, laneCount> laneValues;
    std::array<std::atomic<int>, laneCount> laneCurrentSteps;
    std::array<std::atomic<int>, laneCount> laneStartSteps {};

    // Audio-thread-only state; never touched by the editor.
    bool previousPlayEnabled = true;
    int previousResetCounter = 0;
    // Audio-thread-owned clock; frozen during either kind of pause/stop.
    // Each lane has its own phase origin. All published atomics are UI read-only.
    double retainedBeatPpq = 0.0;
    PnordeGlobalReverse::Phase globalReversePhase; // audio-thread only, no allocations
    std::array<double, laneCount> lanePhaseOrigins {};
    std::array<std::atomic<double>, laneCount> publishedLanePhaseOrigins {};
    std::array<int, laneCount> previousLaneResetCounters {};
    std::array<std::atomic<float>*, laneCount> laneResetParameters {};
    // Older transportPhaseOffset replaced by the retained local PPQ clock.
    bool mpeZoneConfigured = false;
    int previousMicroMode = -1;
    bool noteIsActive = false;
    int activeNote = 60;
    int activeChannel = 1;
    int activeBend = 8192;
    int lastSlide = -1;
    double noteOffPpq = 0.0;
    double previousBlockEndPpq = 0.0;
    bool previousBlockWasPlaying = false;
    // Audio-thread-owned beat accumulator for opt-in tempo override.
    bool previousManualMode = false;
    double manualBlockEndPpq = 0.0;
    double previousHostBlockEndPpq = 0.0;
    double glideStartPpq = 0.0, glideEndPpq = 0.0;
    double glideFromSemis = 60.0, glideToSemis = 60.0;
    double lastPitchSemis = 60.0;
    double lastPressure = 0.0;
    double pressureFrom = 0.0, pressureTo = 0.0;
    double pressureStartPpq = 0.0, pressureEndPpq = 0.0;
    double nextModulationPpq = 0.0;
    int lastPressureSent = -1;
    int lastAftertouchStep = -1;
    std::array<int, optionalCount> lastCCSent {};
    std::array<int, optionalCount> lastCCStep {};
    std::array<double, optionalCount> ccFrom {}, ccTo {}, ccStartPpq {}, ccEndPpq {};
    std::array<int, optionalCount> lastCCNumber {};
    // The last BEAT at which each custom Macro sent a CC event. Without this,
    // up to 14 animated Macro lanes can collectively emit thousands of MIDI
    // events per second. This is owned by the audio thread (not shared).
    std::array<double, optionalCount> lastCCEmitPpq {};
    std::array<int, optionalCount> lastOptionalType {};
    std::array<int, optionalCount * 2> dedicatedSent {};
    std::array<int, optionalCount * 2> dedicatedSentChannel {};
    std::array<int, optionalCount * 2> dedicatedTargetStep {}, dedicatedController {};
    std::array<double, optionalCount * 2> dedicatedFrom {}, dedicatedTo {}, dedicatedStartPpq {}, dedicatedEndPpq {};
    bool hadPreviousPitch = false;
    bool legatoVoice = false;

    class ExternalMidiWorker;
    std::unique_ptr<ExternalMidiWorker> externalMidiWorker;
    juce::String selectedMidiOutputId; // message thread only

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        PolyrhythmMIDIProcessor
    )
};