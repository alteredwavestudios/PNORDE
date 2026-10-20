#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PnordeDials.h"
#include <array>
#include <cmath>
#include <functional>
#include <memory>

class PnordeThemeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>&, bool, bool, bool, bool,
                           bool, const juce::String&, const juce::String&, const juce::Drawable*,
                           const juce::Colour*) override;
};

// JUCE ComboBox normally opens its menu on click; a second click resets
// the clock division without opening the menu again.
class PnordeResetComboBox : public juce::ComboBox
{
public:
    std::function<void()> onReset;
    void mouseDoubleClick(const juce::MouseEvent& event) override
    {
        if (onReset) onReset();
        else juce::ComboBox::mouseDoubleClick(event);
    }
};

// Vector-drawn gear: never relies on missing platform glyphs or emoji fonts.
class PnordeCogButton final : public juce::TextButton
{
public:
    PnordeCogButton() : juce::TextButton("Settings") {}
    void paintButton(juce::Graphics& g, bool hovered, bool pressed) override
    {
        const auto bg = findColour(juce::TextButton::buttonColourId);
        const auto fg = findColour(juce::TextButton::textColourOffId);
        auto r = getLocalBounds().toFloat().reduced(0.7f);
        g.setColour(bg.brighter(pressed ? 0.0f : hovered ? 0.08f : 0.0f));
        g.fillRoundedRectangle(r, 5.0f);
        g.setColour(fg.withAlpha(0.60f));
        g.drawRoundedRectangle(r, 5.0f, 1.0f);
        juce::Path wheel;
        const float cx = r.getCentreX(), cy = r.getCentreY();
        const float outer = juce::jmin(r.getWidth(), r.getHeight()) * 0.36f;
        for (int i = 0; i < 32; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * i / 32.0f;
            const float rad = i % 4 == 1 || i % 4 == 2 ? outer : outer * 0.78f;
            const float x = cx + rad * std::cos(a), y = cy + rad * std::sin(a);
            if (i == 0) wheel.startNewSubPath(x, y); else wheel.lineTo(x, y);
        }
        wheel.closeSubPath();
        g.setColour(fg);
        g.fillPath(wheel);
        g.setColour(bg);
        g.fillEllipse(cx - outer * 0.31f, cy - outer * 0.31f,
                      outer * 0.62f, outer * 0.62f);
    }
};

class PolyrhythmMIDIEditor : public juce::AudioProcessorEditor, public juce::TooltipClient, private juce::Timer
{
public:
    explicit PolyrhythmMIDIEditor(PolyrhythmMIDIProcessor&);
    ~PolyrhythmMIDIEditor() override;
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    juce::String getTooltip() override;
private:
    static constexpr int lanes = 21, visibleSteps = 16;
    static constexpr int channelRow = PolyrhythmMIDIProcessor::channelLane;
    static constexpr int modRow = PolyrhythmMIDIProcessor::modRow;
    static constexpr int expressionRow = PolyrhythmMIDIProcessor::expressionRow;
    static constexpr int vibratoRow = PolyrhythmMIDIProcessor::vibratoRow;
    static constexpr int panRow = PolyrhythmMIDIProcessor::panRow;
    static constexpr int sustainRow = PolyrhythmMIDIProcessor::sustainRow;
    struct Lane
    {
        int length = 16, division = 2, page = 0, hovered = -1;
        bool markerDrag = false, lengthDrag = false, numberDrag = false;
        float rangeOpacity[128] {};
        float stepLed = 0.0f;
        float stepPhase = 0.0f;
        double stepDurationMs = 500.0;
        int lastLedStep = -1;
        int startLength = 16, activeStep = -1, dragStartValue = 0, draggedStep = -1;
        int dragStartTransitionMs = 0, dragStartDepth = 0;
        bool valueDrag = false;
        float visualFill[128] {};
        float hoverOpacity = 0.0f;
        int hoverStep = -1;
        juce::Point<int> dragOrigin;
        float displayX = -1.0f, markerVelocity = 0.0f, rippleX = 0.0f, rippleProgress = 1.0f;
        std::array<int, 128> values {};
    };
    std::array<Lane, lanes> lane;
    PolyrhythmMIDIProcessor& processor;
    std::array<PnordeResetComboBox, lanes> divisions;
    std::array<juce::ComboBox, lanes> timings;
    std::array<PnordeClockDial, lanes> clockDials;
    std::array<PnordeTimingDial, lanes> timingDials;
    std::array<PnordePageButton, lanes> prev, next, addPage, removePage;
    PnordeThemeLookAndFeel themeLookAndFeel;
    juce::ComboBox themeSelect;
    juce::ComboBox midiOutputSelect;
    juce::TextButton refreshMidiOutputs { "REFRESH" };
    juce::StringArray midiOutputIds;
    void refreshMidiOutputMenu();
    int themeIndex = 0;
    void applyTheme();
    juce::ComboBox keySelect, scaleSelect, bendRangeSelect;
    juce::ToggleButton manualTempoToggle { "MANUAL" };
    juce::TextEditor manualTempoInput;
    PnordeChoiceDial octaveDial, keyDial, scaleDial, globalClockDial;
    PnordeResetComboBox globalClockSelect;
    juce::TextButton addCCButton { "+ ADD LANE" };
    // All optional lanes use these same controls, irrespective of MIDI role.
    static constexpr int optionalLanes = PolyrhythmMIDIProcessor::optionalCount;
    std::array<juce::ComboBox, optionalLanes> laneTypeSelect;
    PnordeCogButton settingsButton;
    juce::TextButton savePresetButton { "SAVE PRESET" };
    juce::TextButton loadPresetButton { "LOAD PRESET" };
    juce::TextButton randomiseButton { "RANDOMISE" };
    juce::TextButton presetFolderButton { "PRESET DIR" };
    std::unique_ptr<juce::PropertiesFile> presetPreferences;
    juce::File presetDirectory;
    juce::File defaultPresetDirectory() const;
    void initialisePresetDirectory();
    void showPresetFolderMenu();
    void choosePresetDirectory();
    std::unique_ptr<juce::FileChooser> activeFileChooser;
    void savePreset();
    void loadPreset();
    void randomisePattern();
    void syncFromProcessor();
    juce::Slider darknessSlider;
    bool lastTransportPlaying = false;
    int animationTimerHz = 24;
    void showSettingsMenu();
    void applyTooltips();
    bool rowVisible(int row) const noexcept;
    int visibleIndex(int row) const noexcept;
    int visibleRowCount() const noexcept;
    std::array<juce::ComboBox, optionalLanes> ccSelect;
    std::array<juce::TextEditor, optionalLanes> ccLabelEditors;
    std::array<juce::String, optionalLanes> committedCCLabels;
    float ccPopupAlpha = 0.0f;
    int ccPopupRow = -1, ccPopupStep = -1;
    int dragFromRow = -1, dragTargetRow = -1;
    bool laneDragActive = false;
    juce::Point<float> laneDragOrigin;
    juce::Point<float> ccPopupPoint;
    std::array<PnordeDeleteButton, optionalLanes> deleteCC;
    bool isLaneType(int row, PnordeLane::Type type) const noexcept
    { return row >= PnordeLane::firstRow &&
        processor.getOptionalLaneType(row) == static_cast<int>(type); }
    juce::String laneDisplayName(int row) const;
    void paintStaticLabels(juce::Graphics&);
    void refreshMacroPlaceholders();
    // Weak pointer: modal CallOutBox is owned by JUCE and can dismiss itself.
    juce::Component::SafePointer<juce::CallOutBox> settingsCallout;
    // Child controls live inside a clipped viewport; the fixed header never scrolls.
    juce::Component scrollViewport, scrollContent;
    float scrollY = 0.0f, targetScrollY = 0.0f;
    float maxScroll() const;
    void syncCCVisibility();
    double pulse = 0.0, backgroundPhase = 0.0, huePhase = 0.0;
    float scrollbarOpacity = 0.0f;
    juce::TooltipWindow tooltipWindow { this, 700 };
    
    juce::Rectangle<float> cell(int row, int step) const;
    juce::Rectangle<float> lengthBox(int row) const;
    juce::Rectangle<float> reorderHandle(int row) const;
    juce::Rectangle<float> markerBox(int row) const;
    float markerX(int row) const;
    float markerXFor(int row, int length) const;
    float rowTop(int row) const;
    void setLength(int row, int length, bool follow);
    void updateDragValue(int row, const juce::MouseEvent& e);
    void updateButtons();
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PolyrhythmMIDIEditor)
};
