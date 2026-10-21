#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PnordeDials.h"
#include "PnordeCompositionEngine.h"
#include "PnordeVibes.h"
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

// Quick-program buttons keep their ordinary left-click behaviour; right-click
// owns a bank management menu without changing the selected program.
class PnordeQuickProgramButton : public juce::TextButton
{
public:
    std::function<void()> onContext;
    void mouseDown(const juce::MouseEvent& event) override
    {
        if (event.mods.isPopupMenu()) { if (onContext) onContext(); return; }
        juce::TextButton::mouseDown(event);
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

// Keeps editable Macro names and playback-direction menus independent.
class PnordeMacroNameEditor final : public juce::TextEditor
{
public:
    PnordeMacroNameEditor()
    {
        setOpaque(false);
        setBorder(juce::BorderSize<int>(0));
        setIndents(0, 0);
    }
    std::function<void()> onDirectionMenu;
    std::function<void()> onDirectionDial;
    void setEditingTextColour(juce::Colour colour)
    {
        editingTextColour = colour;
        updateTextVisibility();
    }
    void focusGained(FocusChangeType cause) override
    {
        juce::TextEditor::focusGained(cause);
        updateTextVisibility();
    }
    void focusLost(FocusChangeType cause) override
    {
        juce::TextEditor::focusLost(cause);
        updateTextVisibility();
    }
    void mouseDown(const juce::MouseEvent& event) override
    {
        if ((event.mods.isCtrlDown() || event.mods.isCommandDown()) && event.mods.isLeftButtonDown())
        { if (onDirectionDial) onDirectionDial(); return; }
        if (event.mods.isPopupMenu())
        { if (onDirectionMenu) onDirectionMenu(); return; }
        juce::TextEditor::mouseDown(event);
    }
    void paint(juce::Graphics& g) override
    {
        // The editor's final luminous pass owns the ordinary label pixels.
        // Showing this TextEditor at the same time caused doubled names in
        // v0.6.12/13. Display the native editor ONLY while actually editing.
        // Keep this component visible for hit-testing and Ctrl/Cmd-click.
        if (!hasKeyboardFocus(true)) return;
        const auto bg = findColour(juce::TextEditor::backgroundColourId);
        if (!bg.isTransparent())
        {
            g.setColour(bg);
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
        }
        juce::TextEditor::paint(g);
    }
private:
    void updateTextVisibility()
    {
        // A native TextEditor may paint its glyphs using internal child
        // components, so skipping paint() alone is insufficient.
        setColour(juce::TextEditor::textColourId,
                  hasKeyboardFocus(true) ? editingTextColour : juce::Colours::transparentBlack);
        repaint();
    }
    juce::Colour editingTextColour { juce::Colours::white };
};

class PnordeOverlayHost;

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
    static constexpr int lanes = 21;
    int visibleSteps = 16; // 16 / 8 / 4: display pages only, never modifies MIDI lanes
    int columnsForWidth(int width) const noexcept;
    float stepStride() const noexcept;
    float workWidth() const noexcept;
    float workLeft() const noexcept;
    float gridStartX() const noexcept;
    float clockStartX() const noexcept;
    float timingStartX() const noexcept;
    float pageStartX() const noexcept;
    juce::Rectangle<int> headerBounds(int control) const noexcept;
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
        float interactionFlash[128] {};
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
    PnordeVibesRenderer vibes;

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
    juce::TextButton playButton { "PAUSE" }, resetButton { "RESET" };
    void showLoadPresetMenu();
    void applyFactoryPreset(int index);
    void showLaneDirectionMenu(int row);
    void showLaneDirectionPanel(int row);
    juce::TextButton randomiseButton { "GENERATE" };
    juce::TextButton globalReverseButton { "REV: OFF" };
    std::array<PnordeQuickProgramButton, 4> quickPrograms {};
    void showProgramMenu(int index);
    void showProgramLaneMenu();
    PnordeClockDial programClockDial;
    PnordeTimingDial programTimingDial;
    int programHoverStep = -1;
    int lastProgramPlayheadStep = -2;
    int programLanePage = 0;
    float programLaneTop() const noexcept;
    juce::Rectangle<float> programLaneCell(int localStep) const noexcept;
    void paintProgramLane(juce::Graphics&);
    void paintProgramLaneForeground(juce::Graphics&);

    int lastDisplayedProgram = -1;
    void refreshProgramButtons();
    void showClocksMenu(int row);
    void openContainedPanel(std::unique_ptr<juce::Component> panel,
                            juce::Rectangle<int> anchor);
    void dismissContainedPanel();
    std::unique_ptr<PnordeOverlayHost> popupOverlay;
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
    void applyMusicalPhrase(const PnordeCompositionEngine::Phrase& phrase);
    void applyComposition(int index, std::uint32_t seed);
    void syncFromProcessor();
    juce::Slider darknessSlider;
    bool lastTransportPlaying = false;
    int animationTimerHz = 24;
    double nextVibesFrameMs = 0.0; // Message thread; HPPD 1-60 FPS deadline
    int lastVibesIntensity = -1;
    int lastVibesMode = -1;
    bool lastVibesEnabled = false;
    void showSettingsMenu();
    void applyTooltips();
    bool rowVisible(int row) const noexcept;
    int visibleIndex(int row) const noexcept;
    int visibleRowCount() const noexcept;
    std::array<juce::ComboBox, optionalLanes> ccSelect;
    std::array<PnordeMacroNameEditor, optionalLanes> ccLabelEditors;
    std::array<PnordeMacroNameEditor, lanes> laneNameEditors;
    std::array<juce::String, lanes> savedLaneNames;
    bool bottomLeftResizeActive = false;
    juce::Point<int> bottomLeftResizeOrigin;
    juce::Rectangle<float> footerCreditBounds() const noexcept;
    juce::Rectangle<int> bottomLeftResizeBounds() const noexcept
    {
        const int h = getHeight();
        return { 8, juce::roundToInt(h * 0.925f) + 6, 22, juce::jmax(16, juce::roundToInt(h * 0.040f)) };
    }
    juce::String midiDescription(int row) const;
    juce::String laneHoverDescription(int row) const;
    void syncLaneNames();
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
    juce::Component::SafePointer<juce::CallOutBox> directionCallout;
    juce::Component::SafePointer<juce::CallOutBox> infoCallout;
    // Child controls live inside a clipped viewport; the fixed header never scrolls.
    juce::Component scrollViewport, scrollContent;
    float scrollY = 0.0f, targetScrollY = 0.0f;
    float maxScroll() const;
    void syncCCVisibility();
    double pulse = 0.0, backgroundPhase = 0.0, huePhase = 0.0;
    bool logoHover = false, creditHover = false;
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
