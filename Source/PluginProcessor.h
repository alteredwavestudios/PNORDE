#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <array>
#include <memory>
#include "PnordeLane.h"
#include "PnordeScales.h"
#include "PnordeMicroOutput.h"

class PolyrhythmMIDIProcessor : public juce::AudioProcessor
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

    // External routing: empty identifier = host MIDI, otherwise a Windows MIDI output.
    juce::Array<juce::MidiDeviceInfo> getAvailableMidiOutputs() const;
    juce::String getMidiOutputIdentifier() const;
    juce::String setMidiOutputIdentifier(const juce::String& identifier);
    bool isExternalMidiOutputActive() const noexcept;

    void requestPlay();

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
    int getLaneValue(int row, int step) const noexcept;
    void setLaneValue(int row, int step, int value) noexcept;
    int getLaneCurrentStep(int row) const noexcept;
    int getLaneTiming(int row) const noexcept;
    void setLaneTiming(int row, int mode) noexcept;
    // Global transpose is clamped at the final MIDI note (0..127).
    int getGlobalOctave() const noexcept { return static_cast<int>(parameters.getRawParameterValue("pn_octave")->load()); }
    void setGlobalOctave(int octave) noexcept;
    int getGlobalClockIndex() const noexcept { return static_cast<int>(parameters.getRawParameterValue("pn_clock")->load()); }
    void setGlobalClockIndex(int index) noexcept;
    int getBendRangeIndex() const noexcept { return static_cast<int>(parameters.getRawParameterValue("pn_bend")->load()); }
    void setBendRangeIndex(int index) noexcept;

    int getThemeIndex() const noexcept { return static_cast<int>(parameters.getRawParameterValue("pn_theme")->load()); }
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
            juce::jlimit(1120, 2160, width),
            std::memory_order_relaxed);

        editorHeight.store(
            juce::jlimit(840, 1620, height),
            std::memory_order_relaxed);
    }

    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();

    double currentSampleRate = 44100.0;

    std::array<std::atomic<bool>, 256> pattern;

    std::atomic<int> sequenceLength { 16 };
    std::atomic<int> clockDivisionIndex { 2 };
    std::atomic<int> currentStep { -1 };

    std::atomic<bool> hostPlaying { false };
    std::atomic<double> hostPpq { 0.0 }, hostBpm { 120.0 };
    std::atomic<double> hostClockUpdateMs { 0.0 };

    std::atomic<int> editorWidth { 1280 };
    std::atomic<int> editorHeight { 960 };

    std::atomic<int> key { 0 }, scale { 0 }, themeIndex { 0 };
    std::array<std::atomic<int>, laneCount> laneLengths;
    std::array<std::atomic<int>, laneCount> laneDivisions;
    std::array<std::atomic<int>, laneCount> laneTimings;
    std::atomic<int> globalClock { 2 }, bendRangeIndex { 2 };
    std::atomic<int> defaultMidiChannel { 1 };
    std::atomic<bool> midiChannelLaneEnabled { false };
    std::array<std::atomic<bool>, dedicatedCount> dedicatedEnabled {};
    std::array<std::atomic<int>, maxSteps> vibratoDepthValues {};
    std::atomic<int> darknessPercent { 0 };
    std::atomic<bool> animationsEnabled { true }, scrollFadesEnabled { true }, tooltipsEnabled { true };
    std::atomic<bool> panicRequested { false };
    std::atomic<bool> experimentalLights { false };
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
    std::array<std::array<std::atomic<int>, maxSteps>, optionalCount> ccTransitionMs {};
    std::array<std::array<std::atomic<int>, maxSteps>, laneCount> laneValues;
    std::array<std::atomic<int>, laneCount> laneCurrentSteps;

    // Audio-thread-only state; never touched by the editor.
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