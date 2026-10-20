#include "PluginEditor.h"
#include "AudiowideEmbedded.h"
#include "PnordeLights.h"
#include "PnordePresetFormat.h"
#include "PnordePatternLibrary.h"
#include "PnordeSafeCC.h"
#include <cmath>
#include <limits>

namespace
{
struct ThemeDefinition
{
    const char* name;
    juce::Colour paper, cream, middle, ink, coral, blue, sage, lavender, amber, teal, pink, cyan;
};
// Original palettes, inspired by seasonal Japanese art, lacquer, textiles and neon streets.
const ThemeDefinition themes[] = {
    {"Sakura Haze", juce::Colour (juce::uint32 (0xffF8F0DE)),juce::Colour (juce::uint32 (0xffF0E5D1)),juce::Colour (juce::uint32 (0xffE8B9A9)),juce::Colour (juce::uint32 (0xff29324A)),juce::Colour (juce::uint32 (0xffD97870)),juce::Colour (juce::uint32 (0xff597EAD)),juce::Colour (juce::uint32 (0xff83A77A)),juce::Colour (juce::uint32 (0xffA48BBE)),juce::Colour (juce::uint32 (0xffC69A56)),juce::Colour (juce::uint32 (0xff65AAA8)),juce::Colour (juce::uint32 (0xffCE91A6)),juce::Colour (juce::uint32 (0xff00E5FF))},
    {"Indigo Rain", juce::Colour (juce::uint32 (0xff141E35)),juce::Colour (juce::uint32 (0xff283F58)),juce::Colour (juce::uint32 (0xff304C72)),juce::Colour (juce::uint32 (0xffF3E9D9)),juce::Colour (juce::uint32 (0xffF28D92)),juce::Colour (juce::uint32 (0xff7FA8F1)),juce::Colour (juce::uint32 (0xff88C7AD)),juce::Colour (juce::uint32 (0xffC6A4ED)),juce::Colour (juce::uint32 (0xffEBC17C)),juce::Colour (juce::uint32 (0xff61CED1)),juce::Colour (juce::uint32 (0xffF1A8C9)),juce::Colour (juce::uint32 (0xff6DF1FF))},
    {"Matcha Silk", juce::Colour (juce::uint32 (0xffE7E9D5)),juce::Colour (juce::uint32 (0xffC8D5B4)),juce::Colour (juce::uint32 (0xffA5C1A5)),juce::Colour (juce::uint32 (0xff243A30)),juce::Colour (juce::uint32 (0xffBD705D)),juce::Colour (juce::uint32 (0xff6D8F96)),juce::Colour (juce::uint32 (0xff6C9666)),juce::Colour (juce::uint32 (0xffA08DAB)),juce::Colour (juce::uint32 (0xffBE9B53)),juce::Colour (juce::uint32 (0xff569E8D)),juce::Colour (juce::uint32 (0xffCB8C91)),juce::Colour (juce::uint32 (0xff27D5CB))},
    {"Neon Matsuri",juce::Colour (juce::uint32 (0xff21102C)),juce::Colour (juce::uint32 (0xff401B45)),juce::Colour (juce::uint32 (0xff702C61)),juce::Colour (juce::uint32 (0xffFFF0DA)),juce::Colour (juce::uint32 (0xffFF5D8F)),juce::Colour (juce::uint32 (0xff6A9AFF)),juce::Colour (juce::uint32 (0xffB2F96A)),juce::Colour (juce::uint32 (0xffCD7FFF)),juce::Colour (juce::uint32 (0xffFFC55A)),juce::Colour (juce::uint32 (0xff3EE5CE)),juce::Colour (juce::uint32 (0xffFF8AC4)),juce::Colour (juce::uint32 (0xff64F6FF))},
    {"Koi Lantern",juce::Colour (juce::uint32 (0xffF6E5C9)),juce::Colour (juce::uint32 (0xffF5C99D)),juce::Colour (juce::uint32 (0xffEBAF87)),juce::Colour (juce::uint32 (0xff4D2C2D)),juce::Colour (juce::uint32 (0xffD75B43)),juce::Colour (juce::uint32 (0xff567D99)),juce::Colour (juce::uint32 (0xff829F6C)),juce::Colour (juce::uint32 (0xffAD7D91)),juce::Colour (juce::uint32 (0xffD6A14E)),juce::Colour (juce::uint32 (0xff559F94)),juce::Colour (juce::uint32 (0xffC67B87)),juce::Colour (juce::uint32 (0xff22C7D9))},
    {"Sumi Moon",juce::Colour (juce::uint32 (0xff181B21)),juce::Colour (juce::uint32 (0xff30333A)),juce::Colour (juce::uint32 (0xff4C5058)),juce::Colour (juce::uint32 (0xffE9E8E3)),juce::Colour (juce::uint32 (0xffD17E7B)),juce::Colour (juce::uint32 (0xff8B9FBF)),juce::Colour (juce::uint32 (0xff9AB5A2)),juce::Colour (juce::uint32 (0xffB4A3C9)),juce::Colour (juce::uint32 (0xffD6BB88)),juce::Colour (juce::uint32 (0xff8DBCB9)),juce::Colour (juce::uint32 (0xffCBA5B4)),juce::Colour (juce::uint32 (0xff8ADFE9))},
    {"Yuzu Pop",juce::Colour (juce::uint32 (0xffFFF3D3)),juce::Colour (juce::uint32 (0xffFFE3A5)),juce::Colour (juce::uint32 (0xffF9BFC0)),juce::Colour (juce::uint32 (0xff413047)),juce::Colour (juce::uint32 (0xffF16D70)),juce::Colour (juce::uint32 (0xff6781CF)),juce::Colour (juce::uint32 (0xff74AE85)),juce::Colour (juce::uint32 (0xffAD7AC6)),juce::Colour (juce::uint32 (0xffE2AC37)),juce::Colour (juce::uint32 (0xff42B4AC)),juce::Colour (juce::uint32 (0xffF08BB2)),juce::Colour (juce::uint32 (0xff00C9E0))},
    {"Ume Shadow",juce::Colour (juce::uint32 (0xff301B29)),juce::Colour (juce::uint32 (0xff57304A)),juce::Colour (juce::uint32 (0xff79465A)),juce::Colour (juce::uint32 (0xffF6E4E6)),juce::Colour (juce::uint32 (0xffF18C9D)),juce::Colour (juce::uint32 (0xff9B9DE3)),juce::Colour (juce::uint32 (0xffA5C3A0)),juce::Colour (juce::uint32 (0xffD3A4D2)),juce::Colour (juce::uint32 (0xffE7BA83)),juce::Colour (juce::uint32 (0xff78C8BD)),juce::Colour (juce::uint32 (0xffF5B2C9)),juce::Colour (juce::uint32 (0xff72E9ED))}
};
constexpr int themeCount = static_cast<int>(sizeof(themes) / sizeof(themes[0]));
const char* names[] {"SEQ", "PITCH", "VELOCITY", "GATE LENGTH", "OCTAVE", "SLIDE", "AFTERTOUCH", "MACRO 1", "MACRO 2", "MACRO 3", "MACRO 4", "MACRO 5", "MACRO 6", "MACRO 7", "MACRO 8", "MOD WHEEL", "EXPRESSION PEDAL", "VIBRATO", "PAN", "SUSTAIN PEDAL", "MIDI CHANNEL SELECT"};
const char* divisionsText[] {"1/32","1/16","1/8","1/4","1/2","1","2","4","8","16","32"};
// Preserve the CC lane's identity at the midpoint while showing cool-to-hot glide.
juce::Colour glideColour(juce::Colour laneColour, float amount)
{
    amount = juce::jlimit(0.0f, 1.0f, amount);
    const auto cool = juce::Colour(0xff3A9BD5);
    const auto hot = juce::Colour(0xffE65E47);
    if (amount < 0.5f)
        return cool.interpolatedWith(laneColour, amount * 2.0f);
    return laneColour.interpolatedWith(hot, (amount - 0.5f) * 2.0f);
}

juce::Font font(float size, bool bold = false)
{
    return juce::Font(juce::FontOptions(juce::String("Bahnschrift"), size,
        bold ? juce::Font::bold : juce::Font::plain));
}

// Shared value-bar geometry prevents the dark-mode light mask from drifting
// away from the underlying cell fill as layout/parameter types change.
const juce::String& stepNumberLabel(int zeroBasedIndex)
{
    static const auto labels = []
    {
        std::array<juce::String, 128> texts;
        for (int i = 0; i < 128; ++i) texts[static_cast<size_t>(i)] = juce::String(i + 1);
        return texts;
    }();
    return labels[static_cast<size_t>(juce::jlimit(0, 127, zeroBasedIndex))];
}

juce::Rectangle<float> parameterFillBounds(int row, juce::Rectangle<float> outer,
                                             float proportion, int optionalType)
{
    const auto inner = outer.reduced(2.0f);
    const float f = juce::jlimit(0.0f, 1.0f, proportion);
    const float height = inner.getHeight() *
        ((row == 1 || row == 4) ? std::abs(f - 0.5f) * 2.0f : f);
    if (row == 1 || row == 4)
    {
        const float mid = inner.getCentreY();
        return f >= 0.5f
            ? juce::Rectangle<float>(inner.getX(), mid - height * 0.5f,
                                     inner.getWidth(), height * 0.5f)
            : juce::Rectangle<float>(inner.getX(), mid,
                                     inner.getWidth(), height * 0.5f);
    }
    if (optionalType == 4)
    {
        const float mid = inner.getCentreX();
        const float x = inner.getX() + inner.getWidth() * f;
        return {juce::jmin(mid, x), inner.getY(), std::abs(x - mid), inner.getHeight()};
    }
    return {inner.getX(), inner.getBottom() - height, inner.getWidth(), height};
}
}

// Self-contained gear panel. Only message-thread controls live here;
// processor MIDI/clock state remains atomic and the audio callback unchanged.
class PnordeSettingsPanel final : public juce::Component
{
public:
    PnordeSettingsPanel(PolyrhythmMIDIProcessor& p, std::function<void()> changed)
        : processor(p), onChanged(std::move(changed))
    {
        setSize(408, 735);
        setOpaque(true);
        for (int i = 0; i < themeCount; ++i) theme.addItem(themes[i].name, i + 1);
        for (int i = 1; i <= 16; ++i)
            channel.addItem("Channel " + juce::String(i), i);
        const char* ranges[] { "+/-2 semitones", "+/-12 semitones",
                                "+/-24 semitones", "+/-48 semitones" };
        for (int i = 0; i < 4; ++i) bend.addItem(ranges[i], i + 1);
        theme.setSelectedId(processor.getThemeIndex() + 1, juce::dontSendNotification);
        channel.setSelectedId(processor.getDefaultMidiChannel(), juce::dontSendNotification);
        bend.setSelectedId(processor.getEffectiveBendRangeIndex() + 1, juce::dontSendNotification);
        bend.setEnabled(!processor.isMicrotonalScaleSelected());
        bend.setTooltip("Microtonal scales automatically use +/-2 semitones. Match your receiving synth; standard scales restore your saved preference.");
        theme.onChange = [this]
        {
            processor.setThemeIndex(theme.getSelectedId() - 1);
            applyPalette();
            repaint(); if (onChanged) onChanged();
        };
        channel.onChange = [this]
        { processor.setDefaultMidiChannel(channel.getSelectedId()); };
        bend.onChange = [this]
        {
            if (!processor.isMicrotonalScaleSelected())
                processor.setBendRangeIndex(bend.getSelectedId() - 1);
            else
                bend.setSelectedId(1, juce::dontSendNotification);
        };
        for (auto* combo : { &theme, &channel, &bend }) addAndMakeVisible(*combo);
        applyPalette();

        animations.setButtonText("Smooth playhead and marker animations");
        fades.setButtonText("Fading scroll edges");
        tooltips.setButtonText("Parameter tooltips");
        animations.setToggleState(processor.getAnimationsEnabled(), juce::dontSendNotification);
        fades.setToggleState(processor.getScrollFadesEnabled(), juce::dontSendNotification);
        tooltips.setToggleState(processor.getTooltipsEnabled(), juce::dontSendNotification);
        animations.onClick = [this]
        { processor.setAnimationsEnabled(animations.getToggleState()); };
        fades.onClick = [this]
        { processor.setScrollFadesEnabled(fades.getToggleState()); if (onChanged) onChanged(); };
        tooltips.onClick = [this]
        { processor.setTooltipsEnabled(tooltips.getToggleState()); if (onChanged) onChanged(); };
        for (auto* toggle : { &animations, &fades, &tooltips }) addAndMakeVisible(*toggle);
        reorder.setButtonText("EXPERIMENTAL: Drag coloured lane rails to reorder");
        reorder.setToggleState(processor.getExperimentalReorderEnabled(), juce::dontSendNotification);
        reorder.onClick = [this] { processor.setExperimentalReorderEnabled(reorder.getToggleState()); };
        addAndMakeVisible(reorder);

        microMode.addItem("Standard bend: +/-2; SLIDE = pitch approach", 1);
        microMode.addItem("EXPERIMENTAL MPE: +/-2; member channel 2", 2);
        // Older mode 2 is an alias for standard; do not expose a duplicate.
        microMode.setSelectedId(processor.getMicroOutputMode() == 1 ? 2 : 1,
                                juce::dontSendNotification);
        microMode.onChange = [this]
        {
            processor.setMicroOutputMode(microMode.getSelectedId()-1);
            processor.requestMidiPanic();
            if (onChanged) onChanged();
        };
        addAndMakeVisible(microMode);
        mpePoly.setButtonText("EXPERIMENTAL: Poly Aftertouch (independent)");
        mpePoly.setToggleState(processor.getExperimentalPolyPressure(), juce::dontSendNotification);
        mpePoly.onClick = [this]
        { processor.setExperimentalPolyPressure(mpePoly.getToggleState()); };
        addAndMakeVisible(mpePoly);
        diagnostics.setJustificationType(juce::Justification::centredLeft);
        diagnostics.setFont(font(11.0f));
        addAndMakeVisible(diagnostics);
        refreshDiagnostics.setButtonText("Refresh MIDI Diagnostics");
        refreshDiagnostics.setTooltip("Displays PNORDE-generated pitch bends, not what Ableton delivers.");
        refreshDiagnostics.onClick = [this] { updateDiagnostics(); };
        addAndMakeVisible(refreshDiagnostics);
        updateDiagnostics();
        experimental.setButtonText("EXPERIMENTAL: Sequencer lighting + darkness");
        experimental.setToggleState(processor.getExperimentalLightsEnabled(), juce::dontSendNotification);
        experimental.onClick = [this]
        {
            processor.setExperimentalLightsEnabled(experimental.getToggleState());
            darkness.setEnabled(experimental.getToggleState());
            if (onChanged) onChanged();
        };
        addAndMakeVisible(experimental);

        darkness.setSliderStyle(juce::Slider::LinearHorizontal);
        darkness.setTextBoxStyle(juce::Slider::TextBoxRight, false, 42, 22);
        darkness.setRange(0.0, 75.0, 1.0);
        darkness.setTextValueSuffix("%");
        darkness.setValue(processor.getDarknessPercent(), juce::dontSendNotification);
        darkness.onValueChange = [this]
        { processor.setDarknessPercent(juce::roundToInt(darkness.getValue()));
          if (onChanged) onChanged(); };
        addAndMakeVisible(darkness);
        darkness.setEnabled(processor.getExperimentalLightsEnabled());

        panic.setButtonText("MIDI PANIC - All Notes Off");
        panic.onClick = [this] { processor.requestMidiPanic(); };
        addAndMakeVisible(panic);
    }

    void paint(juce::Graphics& g) override
    {
        const auto& p = themes[juce::jlimit(0, themeCount - 1, processor.getThemeIndex())];
        g.fillAll(p.paper);
        g.setColour(p.coral); g.fillRect(0, 0, getWidth(), 5);
        g.setColour(p.ink); g.setFont(font(22.0f, true));
        g.drawText("PNORDE  /  SETTINGS", 18, 18, getWidth() - 36, 35,
                   juce::Justification::centredLeft);
        g.setFont(font(12.0f)); g.setColour(p.ink.withAlpha(0.73f));
        g.drawText("Colour theme", 20, 75, 160, 18, juce::Justification::centredLeft);
        g.drawText("Default MIDI channel", 20, 133, 230, 18,
                   juce::Justification::centredLeft);
        g.drawText("Pitch bend range", 20, 191, 230, 18,
                   juce::Justification::centredLeft);
        g.drawText("DARKNESS  /  experimental only", 20, 249, 350, 18,
                   juce::Justification::centredLeft);
        g.setColour(p.ink.withAlpha(0.18f));
        g.drawHorizontalLine(320, 18.0f, static_cast<float>(getWidth() - 18));
        g.drawHorizontalLine(652, 18.0f, static_cast<float>(getWidth() - 18));
        g.drawText("Microtonal MIDI output", 20, 476, 330, 18,
                   juce::Justification::centredLeft);
        g.setColour(p.ink.withAlpha(0.70f));
        g.drawText("PNORDE - Dream Sequencer   v0.5.9 Candidate", 18, 672,
                   getWidth() - 36, 17, juce::Justification::centredLeft);
        g.drawText("Altered Wave Studios", 18, 693,
                   getWidth() - 36, 17, juce::Justification::centredLeft);
    }
    void resized() override
    {
        const int w = getWidth() - 40;
        theme.setBounds(20, 95, w, 28);
        channel.setBounds(20, 153, w, 28);
        bend.setBounds(20, 211, w, 28);
        darkness.setBounds(20, 271, w, 32);
        experimental.setBounds(20, 315, w, 26);
        animations.setBounds(20, 352, w, 26);
        fades.setBounds(20, 382, w, 26);
        tooltips.setBounds(20, 412, w, 26);
        reorder.setBounds(20, 442, w, 26);
        microMode.setBounds(20, 498, w, 27);
        mpePoly.setBounds(20, 530, w, 27);
        diagnostics.setBounds(20, 565, w, 19);
        refreshDiagnostics.setBounds(20, 589, w, 23);
        panic.setBounds(20, 621, w, 27);
    }
private:
    void applyPalette()
    {
        const auto& p = themes[juce::jlimit(0, themeCount - 1, processor.getThemeIndex())];
        for (auto* combo : { &theme, &channel, &bend, &microMode })
        {
            combo->setColour(juce::ComboBox::backgroundColourId, p.cream);
            combo->setColour(juce::ComboBox::textColourId, p.ink);
            combo->setColour(juce::ComboBox::arrowColourId, p.ink);
            combo->setColour(juce::ComboBox::outlineColourId, p.teal);
        }
        for (auto* toggle : { &animations, &fades, &tooltips, &experimental, &reorder, &mpePoly })
        {
            toggle->setColour(juce::ToggleButton::textColourId, p.ink);
            toggle->setColour(juce::ToggleButton::tickColourId,
                p.paper.getPerceivedBrightness() > 0.55f ? juce::Colours::black : p.ink);
            toggle->setColour(juce::ToggleButton::tickDisabledColourId, p.ink.withAlpha(0.5f));
        }
        darkness.setColour(juce::Slider::textBoxTextColourId, p.ink);
        darkness.setColour(juce::Slider::textBoxBackgroundColourId, p.cream);
        darkness.setColour(juce::Slider::textBoxOutlineColourId, p.middle);
        darkness.setColour(juce::Slider::thumbColourId, p.teal);
        darkness.setColour(juce::Slider::trackColourId, p.teal);
        diagnostics.setColour(juce::Label::textColourId, p.ink);
        refreshDiagnostics.setColour(juce::TextButton::buttonColourId, p.cream);
        refreshDiagnostics.setColour(juce::TextButton::textColourOffId, p.ink);
        panic.setColour(juce::TextButton::buttonColourId, p.cream);
        panic.setColour(juce::TextButton::textColourOffId, p.ink);
    }
    void updateDiagnostics()
    {
        const int bend = processor.getLastPitchWheelValue();
        diagnostics.setText("Bend=" + juce::String(bend) +
            "  EDO offset=" + juce::String(processor.getLastEDOOffsetCents()) +
            "c  Messages=" + juce::String(processor.getPitchWheelSendCount()),
            juce::dontSendNotification);
    }
    PolyrhythmMIDIProcessor& processor;
    std::function<void()> onChanged;
    juce::ComboBox theme, channel, bend, microMode;
    juce::ToggleButton animations, fades, tooltips, experimental, reorder, mpePoly;
    juce::Label diagnostics;
    juce::TextButton refreshDiagnostics;
    juce::Slider darkness;
    juce::TextButton panic;
};

PolyrhythmMIDIEditor::PolyrhythmMIDIEditor(PolyrhythmMIDIProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    const auto& palette = themes[0];
    const auto paper = palette.paper, ink = palette.ink, teal = palette.teal;
    setResizable(true, true);
    setResizeLimits(1120, 840, 2160, 1620);
    getConstrainer()->setFixedAspectRatio(4.0 / 3.0);
    const int initialWidth = juce::jlimit(1120, 2160, processor.getEditorWidth());
    setSize(initialWidth, juce::roundToInt(initialWidth * 0.75f));
    // Parent clipping is applied to native controls, not just the painted lanes.
    addAndMakeVisible(scrollViewport);
    scrollViewport.setInterceptsMouseClicks(false, true);
    scrollViewport.addAndMakeVisible(scrollContent);
    scrollContent.setInterceptsMouseClicks(false, true);

    const char* keys[] {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    for (int i = 0; i < PnordeScales::count; ++i)
        scaleSelect.addItem(PnordeScales::names[i], i + 1);
    for (auto* box : { &keySelect, &scaleSelect })
    {
        box->setColour(juce::ComboBox::backgroundColourId, paper);
        box->setColour(juce::ComboBox::textColourId, ink);
        // These legacy ComboBoxes synchronise parameter values but are hidden;
        // the rotary dials are the actual mouse-accessible controls.
    }
    keySelect.setSelectedId(processor.getKey() + 1, juce::dontSendNotification);
    scaleSelect.setSelectedId(processor.getScale() + 1, juce::dontSendNotification);
    keySelect.onChange = [this]
    {
        const int id = keySelect.getSelectedId();
        if (id < 1 || id > 12) return;
        processor.setKey(id - 1);
        repaint();
    };
    scaleSelect.onChange = [this]
    {
        const int id = scaleSelect.getSelectedId();
        if (id < 1 || id > PnordeScales::count) return;
        processor.setScale(id - 1);
        if (processor.getScale() != 13)
            for (int i = 0; i < 128; ++i)
                processor.setLaneValue(1, i, processor.quantizePitchTenths(processor.getLaneValue(1, i), 0));
        repaint();
    };
    for (int i = 0; i < 5; ++i) globalClockSelect.addItem(
        juce::StringArray { "1/4x", "1/2x", "1x", "2x", "4x" }[i], i + 1);
    for (int i = 0; i < 4; ++i) bendRangeSelect.addItem(
        juce::StringArray { "+/-2", "+/-12", "+/-24", "+/-48" }[i], i + 1);
    for (auto* box : std::array<juce::ComboBox*, 2>{ { &globalClockSelect, &bendRangeSelect } })
    {
        box->setColour(juce::ComboBox::backgroundColourId, paper);
        box->setColour(juce::ComboBox::textColourId, ink);
        // Replaced by compact rotary dials for Global Clock.
    }
    addAndMakeVisible(bendRangeSelect);
    globalClockSelect.setSelectedId(processor.getGlobalClockIndex() + 1, juce::dontSendNotification);
    bendRangeSelect.setSelectedId(processor.getEffectiveBendRangeIndex() + 1, juce::dontSendNotification);
    bendRangeSelect.setEnabled(!processor.isMicrotonalScaleSelected());
    globalClockSelect.onChange = [this] { processor.setGlobalClockIndex(globalClockSelect.getSelectedId() - 1); };
    globalClockSelect.onReset = [this]
    {
        globalClockSelect.setSelectedId(3, juce::sendNotificationSync); // 1x
    };
    bendRangeSelect.onChange = [this]
    {
        if (!processor.isMicrotonalScaleSelected())
            processor.setBendRangeIndex(bendRangeSelect.getSelectedId() - 1);
        else
            bendRangeSelect.setSelectedId(1, juce::dontSendNotification);
    };
    // Dedicated, discrete top-bar dials. All bounds are set in resized().
    octaveDial.setChoices(juce::StringArray {"-2", "-1", "0", "+1", "+2"},
                         processor.getGlobalOctave() + 2);
    octaveDial.setDescription("GLOBAL OCTAVE: transpose notes within MIDI 0-127");
    octaveDial.onChange = [this](int idx) { processor.setGlobalOctave(idx-2); repaint(); };
    addAndMakeVisible(octaveDial);
    keyDial.setChoices(juce::StringArray {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, processor.getKey());
    keyDial.setDescription("KEY");
    keyDial.onChange = [this](int idx) { processor.setKey(idx); repaint(); };
    addAndMakeVisible(keyDial);
    juce::StringArray allScales;
    for (int i = 0; i < PnordeScales::count; ++i)
        allScales.add(PnordeScales::names[i]);
    scaleDial.setChoices(allScales, processor.getScale());
    scaleDial.setDescription("SCALE");
    scaleDial.onChange = [this](int idx)
    {
        processor.setScale(idx);
        bendRangeSelect.setSelectedId(processor.getEffectiveBendRangeIndex() + 1,
                                      juce::dontSendNotification);
        bendRangeSelect.setEnabled(!processor.isMicrotonalScaleSelected());
        if (idx != 13)
            for (int step=0; step<128; ++step)
                processor.setLaneValue(1, step,
                    processor.quantizePitchTenths(processor.getLaneValue(1,step),0));
        repaint();
    };
    addAndMakeVisible(scaleDial);
    globalClockDial.setChoices(juce::StringArray {"1/4x","1/2x","1x","2x","4x"}, processor.getGlobalClockIndex());
    globalClockDial.setDescription("GLOBAL CLOCK");
    globalClockDial.onChange = [this](int idx) { processor.setGlobalClockIndex(idx); repaint(); };
    addAndMakeVisible(globalClockDial);
    manualTempoToggle.setToggleState(processor.getManualTempoEnabled(), juce::dontSendNotification);
    manualTempoToggle.setTooltip("Override Ableton BPM while following its transport start/stop and seek.");
    manualTempoToggle.onClick = [this] {
        processor.setManualTempoEnabled(manualTempoToggle.getToggleState());
        manualTempoInput.setEnabled(manualTempoToggle.getToggleState());
    };
    addAndMakeVisible(manualTempoToggle);
    manualTempoInput.setInputRestrictions(6, "0123456789.");
    manualTempoInput.setText(juce::String(processor.getManualTempoBpm(), 1), false);
    manualTempoInput.setTooltip("Manual tempo in BPM (20.0 to 300.0). Enter to apply.");
    manualTempoInput.onReturnKey = [this] {
        processor.setManualTempoBpm(manualTempoInput.getText().getDoubleValue());
        manualTempoInput.setText(juce::String(processor.getManualTempoBpm(), 1), false);
        manualTempoInput.giveAwayKeyboardFocus();
    };
    manualTempoInput.onFocusLost = [this] {
        manualTempoInput.setText(juce::String(processor.getManualTempoBpm(), 1), false);
    };
    manualTempoInput.setEnabled(processor.getManualTempoEnabled());
    addAndMakeVisible(manualTempoInput);
    // Only modulation-oriented, non-channel-mode CCs are offered here.
    for (int slot = 0; slot < optionalLanes; ++slot)
    {
        auto& laneType = laneTypeSelect[slot];
        laneType.addItem("CC MACRO", 1);
        // Macro CC numbers are chosen ONLY with ccSelect, not this type menu.
        laneType.addSeparator();
        laneType.addItem("MOD WHEEL", 2);
        laneType.addItem("EXPRESSION PEDAL", 3);
        laneType.addItem("VIBRATO", 4);
        laneType.addItem("PAN", 5);
        laneType.addItem("SUSTAIN PEDAL", 6);
        laneType.addItem("MIDI CHANNEL SELECT", 7);
        laneType.setSelectedId(processor.getOptionalLaneType(slot + 7) + 1, juce::dontSendNotification);
        laneType.onChange = [this, slot]
        {
            const int row = slot + 7;
            const int selected = laneTypeSelect[slot].getSelectedId();
            if (selected < 1 || selected > 7) return;
            const int desired = selected - 1;
            if (!processor.setOptionalLaneType(row, desired))
            {
                laneTypeSelect[slot].setSelectedId(processor.getOptionalLaneType(row) + 1,
                                                   juce::dontSendNotification);
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                    "PNORDE lane type", "Each fixed parameter can be used only once, and there can be at most eight CC Macros.");
                return;
            }
            laneTypeSelect[slot].setSelectedId(desired + 1, juce::dontSendNotification);
            // Reuse existing vertical values; discard vibrato's horizontal
            // depth when changing role. The processor enforces uniqueness.
            for (int step = 0; step < 128; ++step)
            {
                auto& l = lane[row];
                l.values[step] = processor.getLaneValue(row, step);
                l.visualFill[step] = desired == 6 ? l.values[step] / 16.0f :
                    desired == 4 ? l.values[step] / 127.0f : l.values[step] / 100.0f;
            }
            if (ccPopupRow == row) { ccPopupRow = -1; ccPopupAlpha = 0.0f; }
            syncCCVisibility(); resized(); repaint();
        };
        laneType.setColour(juce::ComboBox::backgroundColourId, paper);
        laneType.setColour(juce::ComboBox::textColourId, ink);
        scrollContent.addAndMakeVisible(laneType);
        auto& select = ccSelect[slot];
        // Deliberately conservative: the macro range is CC20..31 only.
        // CC0/32 bank selection, 6/38 data entry, 98..101 NRPN/RPN,
        // 120..127 channel mode and fixed PNORDE CCs cannot be selected.
        for (int cc = PnordeSafeCC::first; cc <= PnordeSafeCC::last; ++cc)
            select.addItem("CC " + juce::String(cc), cc + 1);
        select.setSelectedId(processor.getCustomCCNumber(slot) + 1, juce::dontSendNotification);
        select.onChange = [this, slot]
        {
            const int selectedCC = ccSelect[slot].getSelectedId() - 1;
            if (PnordeSafeCC::allowed(selectedCC))
                processor.setCustomCCNumber(slot, selectedCC);
        };
        select.setColour(juce::ComboBox::backgroundColourId, paper);
        select.setColour(juce::ComboBox::textColourId, ink);
        scrollContent.addAndMakeVisible(select);
        auto& labelEditor = ccLabelEditors[slot];
        // Updated by refreshMacroPlaceholders() using the visible Macro ordinal.
        labelEditor.setTextToShowWhenEmpty("MACRO", ink.withAlpha(0.88f));
        labelEditor.setText(processor.getCustomCCLabel(slot), false);
        committedCCLabels[slot] = processor.getCustomCCLabel(slot);
        labelEditor.setInputRestrictions(12);
        labelEditor.setColour(juce::TextEditor::backgroundColourId, paper);
        labelEditor.setOpaque(true); // Never show a painted label underneath.
        labelEditor.setColour(juce::TextEditor::textColourId, juce::Colours::black);
        labelEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        labelEditor.setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::black);
        labelEditor.setColour(juce::TextEditor::focusedOutlineColourId, teal);
        labelEditor.setFont(font(getHeight() * 0.0185f, true));
        labelEditor.setJustification(juce::Justification::centred);
        labelEditor.setSelectAllWhenFocused(true);
        labelEditor.setTooltip("Click to rename this CC Macro. Press Enter to save, Escape to cancel.");
        // Do not forward TextEditor clicks into the lane drag handler.
        // The coloured rail has its own dedicated hit target.
        // Only Return commits. Clicking elsewhere discards uncommitted edits.
        labelEditor.onReturnKey = [this, slot]
        {
            auto& editor = ccLabelEditors[slot];
            const auto text = editor.getText().trim().substring(0, 12);
            processor.setCustomCCLabel(slot, text);
            committedCCLabels[slot] = text;
            editor.setText(text, false);
            editor.giveAwayKeyboardFocus();
        };
        labelEditor.onEscapeKey = [this, slot]
        {
            ccLabelEditors[slot].setText(committedCCLabels[slot], false);
            ccLabelEditors[slot].giveAwayKeyboardFocus();
        };
        labelEditor.onFocusLost = [this, slot]
        {
            ccLabelEditors[slot].setText(committedCCLabels[slot], false);
        };
        scrollContent.addAndMakeVisible(labelEditor);

        deleteCC[slot].setButtonText("X");
        deleteCC[slot].setTooltip("Remove this optional lane");
        deleteCC[slot].onClick = [this, slot]
        {
            processor.removeOptionalLane(slot + 7);
            ccPopupRow = -1; ccPopupAlpha = 0.0f;
            for (int i = 0; i < optionalLanes; ++i)
            {
                laneTypeSelect[i].setSelectedId(processor.getOptionalLaneType(i + 7) + 1, juce::dontSendNotification);
                ccSelect[i].setSelectedId(processor.getCustomCCNumber(i) + 1, juce::dontSendNotification);
                committedCCLabels[i] = processor.getCustomCCLabel(i);
                ccLabelEditors[i].setText(committedCCLabels[i], false);
            }
            for (int row = 7; row < lanes; ++row)
            {
                lane[row].page = 0;
                lane[row].length = processor.getLaneLength(row);
            }
            targetScrollY = juce::jmin(targetScrollY, maxScroll());
            syncCCVisibility(); resized(); repaint();
        };
        scrollContent.addAndMakeVisible(deleteCC[slot]);
    }
    addCCButton.onClick = [this]
    {
        juce::PopupMenu menu;
        const char* types[] { "CC MACRO", "MOD WHEEL", "EXPRESSION PEDAL",
                              "VIBRATO", "PAN", "SUSTAIN PEDAL", "MIDI CHANNEL SELECT" };
        for (int kind = 0; kind < 7; ++kind)
        {
            const bool permitted = kind == 0
                ? processor.activeCCMacroCount() < 8
                : !processor.hasActiveLaneType(kind);
            menu.addItem(kind + 1, types[kind], permitted);
        }
        auto safe = juce::Component::SafePointer<PolyrhythmMIDIEditor>(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&addCCButton),
            [safe](int id)
            {
                if (safe == nullptr || id < 1 || id > 7) return;
                const int row = safe->processor.addOptionalLane(id - 1);
                if (row < 0) return;
                const int slot = row - 7;
                if (id == 1) safe->processor.setCustomCCLabel(slot, {});
                safe->laneTypeSelect[slot].setSelectedId(id, juce::dontSendNotification);
                safe->committedCCLabels[slot] = safe->processor.getCustomCCLabel(slot);
                safe->ccLabelEditors[slot].setText(safe->committedCCLabels[slot], false);
                safe->syncCCVisibility();
                safe->targetScrollY = safe->maxScroll();
                safe->resized(); safe->repaint();
            });
    };
    settingsButton.onClick = [this] { showSettingsMenu(); };
    addAndMakeVisible(settingsButton);
    darknessSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    darknessSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 45, 20);
    darknessSlider.setRange(0.0, 75.0, 1.0);
    darknessSlider.setValue(processor.getDarknessPercent(), juce::dontSendNotification);
    darknessSlider.setTextValueSuffix("%");
    darknessSlider.setTooltip("Dim the interface up to 75%; clock accents stay bright.");
    darknessSlider.onValueChange = [this]
    {
        processor.setDarknessPercent(juce::roundToInt(darknessSlider.getValue()));
        repaint();
    };
    addChildComponent(darknessSlider); // Experimental darkness is controlled in Settings.
    initialisePresetDirectory();
    presetFolderButton.onClick = [this] { showPresetFolderMenu(); };
    presetFolderButton.setTooltip("Open, choose, or reset the PNORDE preset folder");
    addAndMakeVisible(presetFolderButton);
    savePresetButton.onClick = [this] { savePreset(); };
    loadPresetButton.onClick = [this] { loadPreset(); };
    randomiseButton.onClick = [this] { randomisePattern(); };
    for (auto* control : { &savePresetButton, &loadPresetButton, &randomiseButton })
        addAndMakeVisible(*control);


    addCCButton.setColour(juce::TextButton::buttonColourId, teal);
    addCCButton.setColour(juce::TextButton::textColourOffId, ink);
    addAndMakeVisible(addCCButton);
    for (int row = 0; row < lanes; ++row)
    {
        auto& l = lane[static_cast<size_t>(row)];
        l.length = processor.getLaneLength(row);
        l.division = processor.getLaneDivision(row);
        auto& timing = timings[static_cast<size_t>(row)];
        timing.addItem("Straight", 1); timing.addItem("Triplet", 2); timing.addItem("Dotted", 3);
        timing.setSelectedId(processor.getLaneTiming(row) + 1, juce::dontSendNotification);
        timing.setColour(juce::ComboBox::backgroundColourId, paper);
        timing.setColour(juce::ComboBox::textColourId, ink);
        timing.onChange = [this, row]
        {
            processor.setLaneTiming(row, timings[static_cast<size_t>(row)].getSelectedId() - 1);
        };
        // Clock dial replaces timing + division ComboBoxes in the work area.
        for (int i = 0; i < 128; ++i)
            l.values[static_cast<size_t>(i)] = processor.getLaneValue(row, i);
        for (int i = 0; i < 128; ++i) l.visualFill[i] = isLaneType(row, PnordeLane::Type::midiChannel) ? l.values[i] / 16.0f : isLaneType(row, PnordeLane::Type::pan) ? l.values[i] / 127.0f : row == 1 || row == 4 ? 0.5f : row == 5 || row == 6 ? 0.0f : l.values[i] / 100.0f;
        for (int i = 0; i < 11; ++i)
            divisions[static_cast<size_t>(row)].addItem(divisionsText[i], i + 1);
        auto& box = divisions[static_cast<size_t>(row)];
        box.setSelectedId(l.division + 1, juce::dontSendNotification);
        box.setColour(juce::ComboBox::backgroundColourId, paper);
        box.setColour(juce::ComboBox::textColourId, ink);
        box.setColour(juce::ComboBox::outlineColourId,
            palette.coral); // applyTheme() sets the selected lane colour below
        box.onChange = [this, row]
        {
            const int value = divisions[static_cast<size_t>(row)].getSelectedId() - 1;
            if (value < 0 || value > 10) return;
            lane[static_cast<size_t>(row)].division = value;
            processor.setLaneDivision(row, value);
            repaint();
        };
        box.onReset = [this, row]
        {
            divisions[row].setSelectedId(3, juce::sendNotificationSync); // 1/8 default
        };
        auto& dial = clockDials[static_cast<size_t>(row)];
        dial.setValue(l.division);
        dial.onChange = [this, row](int divisionIndex)
        {
            processor.setLaneDivision(row, divisionIndex);
            lane[row].division = divisionIndex;
            divisions[row].setSelectedId(divisionIndex+1, juce::dontSendNotification);
            repaint();
        };
        scrollContent.addAndMakeVisible(dial);
        auto& timingDial = timingDials[static_cast<size_t>(row)];
        timingDial.setValue(processor.getLaneTiming(row));
        timingDial.onChange = [this, row](int mode)
        {
            processor.setLaneTiming(row, mode);
            timings[row].setSelectedId(mode+1, juce::dontSendNotification);
            repaint();
        };
        scrollContent.addAndMakeVisible(timingDial);
        auto& left = prev[static_cast<size_t>(row)];
        auto& right = next[static_cast<size_t>(row)];
        left.setButtonText("<"); right.setButtonText(">");
        left.setColour(juce::TextButton::buttonColourId, paper);
        right.setColour(juce::TextButton::buttonColourId, paper);
        left.setColour(juce::TextButton::textColourOffId, ink);
        right.setColour(juce::TextButton::textColourOffId, ink);
        left.onClick = [this, row]
        {
            auto& a = lane[static_cast<size_t>(row)];
            a.page = juce::jmax(0, a.page - 1);
            a.displayX = -1.0f; updateButtons(); repaint();
        };
        right.onClick = [this, row]
        {
            auto& a = lane[static_cast<size_t>(row)];
            a.page = juce::jmin((a.length - 1) / visibleSteps, a.page + 1);
            a.displayX = -1.0f; updateButtons(); repaint();
        };
        scrollContent.addAndMakeVisible(left); scrollContent.addAndMakeVisible(right);
        auto& minus = removePage[static_cast<size_t>(row)];
        auto& plus = addPage[static_cast<size_t>(row)];
        minus.setButtonText("-"); plus.setButtonText("+");
        minus.onClick = [this, row]
        {
            auto& a = lane[static_cast<size_t>(row)];
            processor.deletePage(row, a.page);
            a.length = processor.getLaneLength(row);
            a.page = juce::jmin(a.page, (a.length - 1) / 16);
            a.displayX = -1.0f; updateButtons(); repaint();
        };
        plus.onClick = [this, row]
        {
            auto& a = lane[static_cast<size_t>(row)];
            processor.appendPage(row);
            a.length = processor.getLaneLength(row);
            a.page = (a.length - 1) / 16;
            a.displayX = -1.0f; updateButtons(); repaint();
        };
        scrollContent.addAndMakeVisible(minus); scrollContent.addAndMakeVisible(plus);
    }
    themeIndex = juce::jlimit(0, themeCount - 1, processor.getThemeIndex());
    for (int i = 0; i < themeCount; ++i) themeSelect.addItem(themes[i].name, i + 1);
    themeSelect.setLookAndFeel(&themeLookAndFeel);
    themeSelect.setSelectedId(themeIndex + 1, juce::dontSendNotification);
    themeSelect.onChange = [this]
    {
        themeIndex = juce::jlimit(0, themeCount - 1, themeSelect.getSelectedId() - 1);
        processor.setThemeIndex(themeIndex);
        applyTheme();
    };
    addAndMakeVisible(midiOutputSelect);
    midiOutputSelect.onChange = [this]
    {
        const int index = midiOutputSelect.getSelectedId() - 1;
        if (!juce::isPositiveAndBelow(index, midiOutputIds.size())) return;
        const auto error = processor.setMidiOutputIdentifier(midiOutputIds[index]);
        if (error.isNotEmpty())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                "MIDI Output", error);
            midiOutputSelect.setSelectedId(1, juce::dontSendNotification);
        }
    };
    refreshMidiOutputs.setButtonText("R");
    refreshMidiOutputs.onClick = [this] { refreshMidiOutputMenu(); };
    addAndMakeVisible(refreshMidiOutputs);
    refreshMidiOutputMenu();
    // Theme selection remains in Settings; the header has no theme ComboBox.
    applyTheme();
    applyTooltips();
    syncCCVisibility();
    updateButtons();
    animationTimerHz = processor.getExperimentalLightsEnabled() ? 30 : 24;
    startTimerHz(animationTimerHz); // Fast effects opt-in; MIDI timing unaffected.
}

bool PolyrhythmMIDIEditor::rowVisible(int row) const noexcept
{
    if (row < 0 || row >= lanes) return false;
    if (row < 7) return true;
    return processor.isOptionalLaneEnabled(row);
}
int PolyrhythmMIDIEditor::visibleIndex(int row) const noexcept
{
    int index = 0;
    if (row < 7) return row;
    index = 7;
    for (int i = 0; i < optionalLanes; ++i)
    {
        const int displayedRow = processor.getDisplayRowAt(i);
        if (displayedRow == row) return index;
        if (rowVisible(displayedRow)) ++index;
    }
    return index;
}
int PolyrhythmMIDIEditor::visibleRowCount() const noexcept
{
    int count = 0;
    for (int r = 0; r < lanes; ++r) if (rowVisible(r)) ++count;
    return count;
}
juce::String PolyrhythmMIDIEditor::laneDisplayName(int row) const
{
    if (row < 0 || row >= lanes) return {};
    if (row < 7) return names[row];
    if (isLaneType(row, PnordeLane::Type::ccMacro))
    {
        const auto label = processor.getCustomCCLabel(row - 7).trim();
        if (label.isNotEmpty()) return label;
        int ordinal = 0;
        for (int i = 0; i < optionalLanes; ++i)
        {
            const int r = processor.getDisplayRowAt(i);
            if (processor.isOptionalLaneEnabled(r) && isLaneType(r, PnordeLane::Type::ccMacro)) ++ordinal;
            if (r == row) break;
        }
        return "MACRO " + juce::String(juce::jmax(1, ordinal));
    }
    return PnordeLane::label(static_cast<PnordeLane::Type>(processor.getOptionalLaneType(row)));
}

float PolyrhythmMIDIEditor::rowTop(int row) const
{
    return getHeight() * (0.155f + visibleIndex(row) * 0.110f) - scrollY;
}
juce::Rectangle<float> PolyrhythmMIDIEditor::cell(int row, int step) const
{
    const float w = static_cast<float>(getWidth());
    const float x0 = w * 0.198f;
    const float available = w * 0.630f;
    const float gap = w * 0.0031f;
    const float cw = (available - gap * 15.0f) / 16.0f;
    return {x0 + step * (cw + gap), rowTop(row) + getHeight() * 0.048f,
            cw, getHeight() * 0.040f};
}
juce::Rectangle<float> PolyrhythmMIDIEditor::lengthBox(int row) const
{
    return {getWidth() * 0.069f, rowTop(row) + getHeight() * 0.049f,
            getWidth() * 0.090f, getHeight() * 0.034f};
}

juce::Rectangle<float> PolyrhythmMIDIEditor::reorderHandle(int row) const
{
    // The visible rail is 4 px wide at x = 0.055 * width.
    // This much larger hit target ends before the Macro text editor at 0.068.
    // Delete buttons end at 0.046, so neither interaction overlaps.
    return {getWidth() * 0.048f, rowTop(row) - getHeight() * 0.011f,
            getWidth() * 0.018f, getHeight() * 0.097f};
}
float PolyrhythmMIDIEditor::markerXFor(int row, int length) const
{
    const int local = juce::jlimit(1, 16, length - lane[static_cast<size_t>(row)].page * 16);
    return cell(row, local - 1).getRight() + getWidth() * 0.002f;
}
float PolyrhythmMIDIEditor::markerX(int row) const
{
    return markerXFor(row, lane[static_cast<size_t>(row)].length);
}
juce::Rectangle<float> PolyrhythmMIDIEditor::markerBox(int row) const
{
    return {markerX(row) - 16.0f, rowTop(row) + getHeight() * 0.015f,
            32.0f, getHeight() * 0.050f};
}
void PolyrhythmMIDIEditor::refreshMidiOutputMenu()
{
    const auto previous = processor.getMidiOutputIdentifier();
    midiOutputSelect.clear(juce::dontSendNotification);
    midiOutputIds.clear();
    midiOutputSelect.addItem("HOST / ABLETON", 1);
    midiOutputIds.add(juce::String());
    int selected = 1;
    for (const auto& device : processor.getAvailableMidiOutputs())
    {
        midiOutputIds.add(device.identifier);
        const int id = midiOutputIds.size();
        midiOutputSelect.addItem(device.name, id);
        if (device.identifier == previous) selected = id;
    }
    midiOutputSelect.setSelectedId(selected, juce::dontSendNotification);
}

void PolyrhythmMIDIEditor::resized()
{
    processor.setEditorSize(getWidth(), getHeight());
    const int w = getWidth(), h = getHeight();
    const int clipTop = juce::roundToInt(h * 0.110f);
    const int clipBottom = juce::roundToInt(h * 0.925f);
    scrollViewport.setBounds(0, clipTop, w, clipBottom - clipTop);
    const int scrollOffset = juce::roundToInt(scrollY);
    // scrollContent is taller than the visible viewport. Parent clips every child.
    scrollContent.setBounds(0, -scrollOffset, w,
                            juce::roundToInt(h * 2.80f));
    auto place = [&](juce::ComboBox& box, float x, float width)
    {
        box.setBounds(juce::roundToInt(w * x), juce::roundToInt(h * 0.057f),
                      juce::roundToInt(w * width), juce::roundToInt(h * 0.034f));
    };
    // Footer left bay, clear of SAVE PRESET starting at 0.285.
    midiOutputSelect.setBounds(juce::roundToInt(w * 0.061f), juce::roundToInt(h * 0.943f),
                               juce::roundToInt(w * 0.166f), juce::roundToInt(h * 0.031f));
    refreshMidiOutputs.setBounds(juce::roundToInt(w * 0.231f), juce::roundToInt(h * 0.943f),
                                 juce::roundToInt(w * 0.039f), juce::roundToInt(h * 0.031f));
    const auto setDial = [w,h](PnordeChoiceDial& d, float x, float width)
    {
        d.setBounds(juce::roundToInt(w*x), juce::roundToInt(h*0.051f),
                    juce::roundToInt(w*width), juce::roundToInt(h*0.047f));
    };
    setDial(octaveDial, 0.249f, 0.097f);
    setDial(keyDial, 0.360f, 0.075f);
    setDial(scaleDial, 0.440f, 0.137f);
    setDial(globalClockDial, 0.584f, 0.078f);
    place(bendRangeSelect, 0.669f, 0.080f);
    manualTempoToggle.setBounds(juce::roundToInt(w*0.750f), juce::roundToInt(h*0.057f),
                                juce::roundToInt(w*0.066f), juce::roundToInt(h*0.033f));
    manualTempoInput.setBounds(juce::roundToInt(w*0.820f), juce::roundToInt(h*0.058f),
                               juce::roundToInt(w*0.061f), juce::roundToInt(h*0.031f));
    addCCButton.setBounds(juce::roundToInt(w * 0.885f), juce::roundToInt(h * 0.057f),
                          juce::roundToInt(w * 0.099f), juce::roundToInt(h * 0.034f));
    settingsButton.setBounds(juce::roundToInt(w * 0.956f), juce::roundToInt(h * 0.944f),
        juce::roundToInt(w * 0.030f), juce::roundToInt(h * 0.029f));
    darknessSlider.setBounds(juce::roundToInt(w * 0.117f), juce::roundToInt(h * 0.942f),
        juce::roundToInt(w * 0.157f), juce::roundToInt(h * 0.032f));
    savePresetButton.setBounds(juce::roundToInt(w * 0.285f), juce::roundToInt(h * 0.943f),
        juce::roundToInt(w * 0.097f), juce::roundToInt(h * 0.031f));
    loadPresetButton.setBounds(juce::roundToInt(w * 0.388f), juce::roundToInt(h * 0.943f),
        juce::roundToInt(w * 0.097f), juce::roundToInt(h * 0.031f));
    randomiseButton.setBounds(juce::roundToInt(w * 0.490f), juce::roundToInt(h * 0.943f),
        juce::roundToInt(w * 0.101f), juce::roundToInt(h * 0.031f));
    presetFolderButton.setBounds(juce::roundToInt(w * 0.597f), juce::roundToInt(h * 0.943f),
        juce::roundToInt(w * 0.070f), juce::roundToInt(h * 0.031f));
    // Theme moved into Settings to leave room for the new rotary dials.
    for (int row = 0; row < lanes; ++row)
    {
        // Native controls are positioned in the unscrolled content coordinate system.
        const float rowY = rowTop(row) + scrollY - clipTop;
        timings[static_cast<size_t>(row)].setBounds(0,0,0,0);
        divisions[static_cast<size_t>(row)].setBounds(0,0,0,0);
        // Separate compact dials fit between the step grid (ends at 0.828)
        // and the page buttons (begin at 0.939); no overlap at 1120px.
        clockDials[row].setBounds(juce::roundToInt(w*0.841f),
            juce::roundToInt(rowY + h*0.015f),
            juce::roundToInt(w*0.050f), juce::roundToInt(h*0.050f));
        timingDials[row].setBounds(juce::roundToInt(w*0.895f),
            juce::roundToInt(rowY + h*0.020f),
            juce::roundToInt(w*0.040f), juce::roundToInt(h*0.040f));
        prev[static_cast<size_t>(row)].setBounds(
            juce::roundToInt(getWidth() * 0.169f),
            juce::roundToInt(rowY + getHeight() * 0.051f),
            juce::roundToInt(getWidth() * 0.023f), juce::roundToInt(getHeight() * 0.034f));
        removePage[static_cast<size_t>(row)].setBounds(
            juce::roundToInt(getWidth() * 0.169f), juce::roundToInt(rowY + getHeight() * 0.010f),
            juce::roundToInt(getWidth() * 0.023f), juce::roundToInt(getHeight() * 0.029f));
        addPage[static_cast<size_t>(row)].setBounds(
            juce::roundToInt(getWidth() * 0.939f), juce::roundToInt(rowY + getHeight() * 0.010f),
            juce::roundToInt(getWidth() * 0.026f), juce::roundToInt(getHeight() * 0.029f));
        next[static_cast<size_t>(row)].setBounds(
            juce::roundToInt(getWidth() * 0.939f),
            juce::roundToInt(rowY + getHeight() * 0.051f),
            juce::roundToInt(getWidth() * 0.026f), juce::roundToInt(getHeight() * 0.034f));
        if (row >= 7)
        {
            const int slot = row - 7;
            ccLabelEditors[slot].setFont(font(h * 0.0152f, true));
            ccLabelEditors[slot].setBounds(juce::roundToInt(w * 0.068f),
                juce::roundToInt(rowY + h * 0.003f), juce::roundToInt(w * 0.098f), juce::roundToInt(h * 0.029f));
            // Delete lives in the LEFT GUTTER, outside the lane panel.
            deleteCC[slot].setBounds(juce::roundToInt(w * 0.021f),
                juce::roundToInt(rowY + h * 0.002f), juce::roundToInt(w * 0.025f), juce::roundToInt(h * 0.031f));
            // Compact role selectors remain in the upper (non-step) band.
            laneTypeSelect[slot].setBounds(juce::roundToInt(w * 0.207f),
                juce::roundToInt(rowY + h * 0.003f), juce::roundToInt(w * 0.105f), juce::roundToInt(h * 0.022f));
            ccSelect[slot].setBounds(juce::roundToInt(w * 0.318f),
                juce::roundToInt(rowY + h * 0.003f), juce::roundToInt(w * 0.065f), juce::roundToInt(h * 0.022f));
        }
    }
    syncCCVisibility();
}
void PolyrhythmMIDIEditor::updateButtons()
{
    for (int row = 0; row < lanes; ++row)
    {
        const auto& l = lane[static_cast<size_t>(row)];
        prev[static_cast<size_t>(row)].setEnabled(l.page > 0);
        next[static_cast<size_t>(row)].setEnabled(l.page < (l.length - 1) / 16);
        addPage[static_cast<size_t>(row)].setEnabled(l.length < 128);
        removePage[static_cast<size_t>(row)].setEnabled(l.length > 16);
    }
}
void PolyrhythmMIDIEditor::setLength(int row, int length, bool follow)
{
    auto& l = lane[static_cast<size_t>(row)];
    l.length = juce::jlimit(1, 128, length);
    processor.setLaneLength(row, l.length);
    if (follow) l.page = (l.length - 1) / 16;
    else l.page = juce::jmin(l.page, (l.length - 1) / 16);
    updateButtons(); repaint();
}
void PolyrhythmMIDIEditor::paint(juce::Graphics& g)
{
    const int w = getWidth(), h = getHeight();
    const auto& palette = themes[themeIndex];
    const auto paper = palette.paper, cream = palette.cream, ink = palette.ink;
    const auto coral = palette.coral, blue = palette.blue, sage = palette.sage;
    const auto lavender = palette.lavender, amber = palette.amber, teal = palette.teal;
    const auto pink = palette.pink, cyan = palette.cyan;
    const juce::Colour colours[] {coral, blue, sage, lavender, amber, teal, pink, teal, amber, blue, coral, lavender, sage, pink, amber, teal, blue, lavender, pink, sage, amber};
    // No animated colour shifts: the header and footer stay static.
    juce::ColourGradient bg(paper, 0.0f, 0.0f, cream,
                            static_cast<float>(w), static_cast<float>(h), false);
    bg.addColour(0.5, palette.middle);
    g.setGradientFill(bg); g.fillAll();
    g.setColour(paper.withAlpha(0.95f)); g.fillRect(0, 0, w, juce::roundToInt(h * 0.105f));
    g.setColour(coral); g.fillRect(0, juce::roundToInt(h * 0.103f), w, 2);
    // Audiowide is embedded in this plugin: no system font installation required.
    g.setColour(coral); // Static identity bar.
    g.fillRoundedRectangle(w * 0.049f, h * 0.018f, 4.0f, h * 0.067f, 2.0f);
    // Header identity is drawn as normal text in paintStaticLabels().
    g.setColour(teal); g.fillRect(w * 0.061f, h * 0.090f, w * 0.031f, 2.0f);
    g.setColour(coral); g.fillRect(w * 0.099f, h * 0.090f, w * 0.011f, 2.0f);
    g.setColour(lavender); g.fillRect(w * 0.116f, h * 0.090f, w * 0.006f, 2.0f);
    // Neon lane outlines are ordinary static graphics.
    g.saveState();
    g.reduceClipRegion(0, juce::roundToInt(h * 0.110f), w, juce::roundToInt(h * 0.815f));
    for (int row = 0; row < lanes; ++row)
    {
        if (!rowVisible(row)) continue;
        const float top = rowTop(row);
        if (top > h * 0.925f || top + h * 0.105f < h * 0.110f) continue;
        if (!g.getClipBounds().toFloat().intersects(
                juce::Rectangle<float>(0, top - h * 0.013f, static_cast<float>(w), h * 0.128f)))
            continue;
        auto& l = lane[static_cast<size_t>(row)];
        const auto colour = colours[row];
        g.setColour(paper.withAlpha(0.60f));
        g.fillRoundedRectangle(juce::Rectangle<float>(w * 0.048f, top - h * 0.012f,
                               w * 0.920f, h * 0.099f), 11.0f);
        // Static rail lives in the lane paint; the shared light renderer adds
        // the beat-boundary attack AFTER the darkness overlay.
        const auto accent = juce::Rectangle<float>(w * 0.055f, top - h * 0.005f,
                                                   4.0f, h * 0.085f);
        g.setColour(colour);
        g.fillRoundedRectangle(accent, 2.0f);
        // Lane names and sequence-length captions are drawn in the light pass.
        const auto length = lengthBox(row);
        // Fully coloured, steady length control: only the vertical rail pulses.
        g.setColour(colour.withAlpha(0.93f)); g.fillRoundedRectangle(length, 6.0f);
        g.setColour(colour.brighter(0.28f)); g.drawRoundedRectangle(length, 6.0f, 1.3f);
        // The counter-pulsing length value is drawn above the dimmer.
        // Low-contrast four-step grouping keeps visual phrasing readable.
        for (int group = 0; group < 4; ++group)
        {
            const auto first = cell(row, group * 4);
            const auto last = cell(row, group * 4 + 3);
            g.setColour(colour.withAlpha(group % 2 == 0 ? 0.055f : 0.027f));
            g.fillRoundedRectangle(juce::Rectangle<float>(first.getX() - 3.0f,
                first.getY() - 3.0f, last.getRight() - first.getX() + 6.0f,
                first.getHeight() + 6.0f), 5.0f);
        }
        const int pageStart = l.page * 16;
        for (int i = 0; i < visibleSteps; ++i)
        {
            const int index = pageStart + i;
            const bool inRange = index < l.length;
            const bool enabled = row == 0 && inRange && processor.getStepEnabled(index);
            const auto bounds = cell(row, i);
            const bool editing = l.valueDrag && l.draggedStep == index;
            const float wiggle = editing ? static_cast<float>(std::sin(pulse * 2.3) * 0.035) : 0.0f;
            const float scale = editing ? 1.055f + wiggle : 1.0f;
            const auto drawBounds = bounds.withSizeKeepingCentre(bounds.getWidth() * scale,
                                                                  bounds.getHeight() * scale);
            const bool darkTheme = paper.getPerceivedBrightness() < 0.50f;
            // One-to-two-notch stronger constant lane-coloured backlight.
            const float activity = juce::jlimit(0.0f, 1.0f, l.rangeOpacity[index]);
            const auto lit = paper.interpolatedWith(colour,
                darkTheme ? (0.32f + 0.30f * activity) : (0.19f + 0.22f * activity));
            g.setColour(!inRange ? ink.withAlpha(0.08f) : lit);
            g.fillRoundedRectangle(drawBounds, 5.0f);
            // A four-beat repeating gradation, dark -> light, reset every bar.
            // Translucent fill only: border luminance stays unchanged.
            if (inRange)
            {
                const float columnLight = 0.018f + (i % 4) * 0.023f;
                g.setColour(colour.withAlpha(columnLight));
                g.fillRoundedRectangle(drawBounds.reduced(1.0f), 4.0f);
            }
            if (inRange)
            {
                if (row == 0)
                {
                    if (enabled) { g.setColour(colour); g.fillRoundedRectangle(drawBounds, 5.0f); }
                }
                else
                {
                    const float f = juce::jlimit(0.0f, 1.0f, l.visualFill[index]);
                    const auto inner = drawBounds.reduced(2.0f);
                    if (row == 1 || row == 4)
                    {
                        g.setColour(ink.withAlpha(0.28f));
                        g.fillRect(inner.getX(), inner.getCentreY(), inner.getWidth(), 1.0f);
                    }
                    else if (isLaneType(row, PnordeLane::Type::pan))
                    {
                        g.setColour(ink.withAlpha(0.40f));
                        g.fillRect(inner.getCentreX(), inner.getY(), 1.0f, inner.getHeight());
                    }
                    else if (isLaneType(row, PnordeLane::Type::vibrato))
                    {
                        const float depth = processor.getVibratoDepth(index) / 100.0f;
                        g.setColour(colour.withAlpha(0.30f));
                        g.fillRoundedRectangle(inner.withWidth(inner.getWidth() * depth), 2.0f);
                    }
                    g.setColour(colour.withAlpha(0.84f));
                    const auto bar = parameterFillBounds(row, drawBounds, f, processor.getOptionalLaneType(row));
                    if (!bar.isEmpty()) g.fillRoundedRectangle(bar, 3.0f);
                }
            }
            if ((row == 6 || (row >= 7 &&
                !isLaneType(row, PnordeLane::Type::sustain) &&
                !isLaneType(row, PnordeLane::Type::midiChannel))) && inRange)
            {
                const int glidePercent = row == 6 ? processor.getAftertouchGlide(index)
                    : isLaneType(row, PnordeLane::Type::ccMacro) ? processor.getCCTransitionMs(row-7,index)
                    : processor.getOptionalGlide(row,index);
                const float glide = juce::jlimit(0.0f, 1.0f, glidePercent / 100.0f);
                const juce::Rectangle<float> meter(bounds.getX() + 2.5f,
                    bounds.getBottom() - 8.0f,
                    juce::jmax(0.0f, bounds.getWidth() - 5.0f), 5.0f);
                // Opaque dark track remains legible over pale AND filled bars.
                g.setColour(ink.withAlpha(0.82f));
                g.fillRoundedRectangle(meter, 2.5f);
                g.setColour(juce::Colours::white.withAlpha(0.8f));
                g.drawRoundedRectangle(meter, 2.5f, 0.9f);
                if (glide > 0.0f)
                {
                    const auto fill = meter.reduced(1.0f).withWidth(
                        juce::jmax(0.0f, (meter.getWidth() - 2.0f) * glide));
                    g.setColour(glideColour(colour, glide));
                    g.fillRoundedRectangle(fill, 1.6f);
                }
            }
            g.setColour(colour.withAlpha(inRange ? 0.65f : 0.18f));
            g.drawRoundedRectangle(drawBounds, 5.0f, 1.0f);
            // Active text is rendered ONCE in the emissive overlay. Only the
            // off-page placeholders live in the static pass, avoiding hundreds
            // of duplicate glyph layouts on every LED animation frame.
            if (!inRange)
            {
                g.setFont(font(h * 0.014f, true));
                g.setColour(ink.withAlpha(0.25f));
                g.drawText(stepNumberLabel(index),
                    juce::Rectangle<int> {juce::roundToInt(bounds.getX()),
                     juce::roundToInt(top + h * 0.029f),
                     juce::roundToInt(bounds.getWidth()), juce::roundToInt(h * 0.020f)},
                    juce::Justification::centred);
                g.setFont(font(h * 0.015f, true));
                g.drawText("--", bounds.toNearestInt(), juce::Justification::centred);
            }
        }
        // Number-hover preview: ghost boundary, no MIDI length change until click.
        if (l.hovered >= 0)
        {
            const float previewX = cell(row, l.hovered).getRight() + w * 0.002f;
            g.setColour(colour.withAlpha(0.38f));
            const float dashes[] {3.0f, 3.0f};
            g.drawDashedLine(juce::Line<float>(previewX, top + h * 0.041f,
                previewX, top + h * 0.103f), dashes, 2, 1.5f);
        }
        if (l.page == (l.length - 1) / 16)
        {
            const float x = l.displayX < 0.0f ? markerX(row) : l.displayX;
            g.setColour(colour.brighter(0.65f).withAlpha(0.95f));
            g.fillRect(x - 1.5f, top + h * 0.040f, 3.0f, h * 0.060f);
            if (l.rippleProgress < 1.0f)
            {
                const float t = l.rippleProgress;
                const float r = 5.0f + 29.0f * t;
                g.setColour(colour.withAlpha((1.0f - t) * 0.55f));
                g.drawEllipse(l.rippleX - r, top + h * 0.025f - r,
                              r * 2.0f, r * 2.0f, 2.0f);
            }
            juce::Path triangle;
            triangle.addTriangle(-9.0f, -7.0f, 9.0f, -7.0f, 0.0f, 8.0f);
            const float scale = l.markerDrag ? 1.18f : 1.0f;
            const float wiggle = l.markerDrag ? static_cast<float>(std::sin(pulse * 2.0) * 0.10) : 0.0f;
            g.setColour(colour);
            g.fillPath(triangle, juce::AffineTransform::scale(scale)
                        .rotated(wiggle).translated(x, top + h * 0.025f));
        }
        if (isLaneType(row, PnordeLane::Type::pan))
        {
            g.setColour(ink.withAlpha(0.65f)); g.setFont(font(h * 0.009f, true));
            g.drawText("L    C    R", juce::roundToInt(w * 0.120f),
                juce::roundToInt(top + h * 0.075f), juce::roundToInt(w * 0.064f),
                juce::roundToInt(h * 0.014f), juce::Justification::centred);
        }
        g.setColour(ink.withAlpha(0.76f)); g.setFont(font(h * 0.012f));
        g.drawText("PAGE " + juce::String(l.page + 1) + "/" +
                    juce::String((l.length - 1) / 16 + 1),
                   juce::roundToInt(w * 0.847f), juce::roundToInt(top + h * 0.073f),
                   juce::roundToInt(w * 0.074f), juce::roundToInt(h * 0.012f),
                   juce::Justification::centred);
    }
    g.restoreState();
    // Theme-coloured feathering at the scroll viewport edges.
    if (processor.getScrollFadesEnabled())
    {
        const float topEdge = h * 0.110f;
        const float bottomEdge = h * 0.925f;
        const float fadeHeight = h * 0.027f;
        if (scrollY > 0.5f)
        {
            juce::ColourGradient gradient(paper.withAlpha(0.86f), 0.0f, topEdge,
                paper.withAlpha(0.0f), 0.0f, topEdge + fadeHeight, false);
            g.setGradientFill(gradient);
            g.fillRect(0.0f, topEdge, static_cast<float>(w), fadeHeight);
        }
        if (scrollY + 0.5f < maxScroll())
        {
            juce::ColourGradient gradient(paper.withAlpha(0.0f), 0.0f, bottomEdge - fadeHeight,
                paper.withAlpha(0.86f), 0.0f, bottomEdge, false);
            g.setGradientFill(gradient);
            g.fillRect(0.0f, bottomEdge - fadeHeight, static_cast<float>(w), fadeHeight);
        }
    }
    // Fixed footer, independent of scroll position. CC rows clip above it.
    g.setColour(paper.withAlpha(0.96f));
    g.fillRect(0, juce::roundToInt(h * 0.925f), w, h - juce::roundToInt(h * 0.925f));
    g.setColour(coral.withAlpha(0.75f));
    g.fillRect(0, juce::roundToInt(h * 0.925f), w, 2);
    // Static typography is shared by normal and experimental modes.
    // Paint BEFORE native child controls so labels can never cover editors.
    paintStaticLabels(g);
}

void PolyrhythmMIDIEditor::paintStaticLabels(juce::Graphics& g)
{
    // Default public-alpha renderer: clean theme colours, crisp neon outlines,
    // no multi-pass text halos, off-screen buffers or whole-window darkness.
    const auto& t = themes[themeIndex];
    const int w = getWidth(), h = getHeight();
    const juce::Colour colours[] {t.coral,t.blue,t.sage,t.lavender,t.amber,t.teal,t.pink,
        t.teal,t.amber,t.blue,t.coral,t.lavender,t.sage,t.pink,t.amber,
        t.teal,t.blue,t.lavender,t.pink,t.sage,t.amber};
    const auto label = [&](const juce::String& value, juce::Rectangle<int> bounds,
                           juce::Colour colour, float size, bool strong = false)
    {
        g.setColour(colour);
        g.setFont(font(size, strong));
        g.drawText(value, bounds, juce::Justification::centred, false);
    };
    // Draw the header's identity once. Native dropdowns and buttons retain
    // their ordinary JUCE text (no bloom pass).
    // Identity typography is ALWAYS ON, regardless of Experimental.
    // Load embedded Audiowide once, not once per repaint.
    static const auto face = juce::Typeface::createSystemTypefaceFor(
        AudiowideEmbedded::fontData, AudiowideEmbedded::fontSize);
    auto logoFont = face != nullptr
        ? juce::Font(juce::FontOptions(face).withHeight(h * 0.038f))
        : font(h * 0.038f, true);
    logoFont.setHorizontalScale(1.10f);
    g.setColour(t.coral);
    g.setFont(logoFont);
    g.drawText("Pnorde", juce::Rectangle<int>{juce::roundToInt(w * 0.061f), juce::roundToInt(h * 0.014f),
        juce::roundToInt(w * 0.165f), juce::roundToInt(h * 0.052f)},
        juce::Justification::centredLeft, false);
    label("D R E A M   S E Q U E N C E R",
        {juce::roundToInt(w * 0.062f), juce::roundToInt(h * 0.065f),
         juce::roundToInt(w * 0.171f), juce::roundToInt(h * 0.017f)},
        t.teal, h * 0.012f);
    const char* header[] {"GLOBAL OCTAVE", "KEY", "SCALE", "GLOBAL CLOCK", "BEND RANGE", "MANUAL BPM", "ADD LANE"};
    const float headerX[] {0.249f, 0.360f, 0.440f, 0.584f, 0.669f, 0.750f, 0.885f};
    const float headerW[] {0.097f, 0.075f, 0.137f, 0.078f, 0.080f, 0.131f, 0.099f};
    for (int i = 0; i < 7; ++i)
        label(header[i], {juce::roundToInt(w * headerX[i]), juce::roundToInt(h * 0.028f),
                          juce::roundToInt(w * headerW[i]), juce::roundToInt(h * 0.024f)},
              t.teal, h * 0.011f, true);
    label(processor.isHostPlaying() ? "HOST PLAYING" : "HOST STOPPED",
        {juce::roundToInt(w * 0.890f), juce::roundToInt(h * 0.010f),
         juce::roundToInt(w * 0.098f), juce::roundToInt(h * 0.017f)},
        t.sage, h * 0.011f);
    {
        juce::Graphics::ScopedSaveState clip(g);
        g.reduceClipRegion(0, juce::roundToInt(h * 0.110f), w,
                           juce::roundToInt(h * 0.815f));
        for (int row = 0; row < lanes; ++row)
        {
            if (!rowVisible(row)) continue;
            const float top = rowTop(row);
            if (top + h * 0.110f < h * 0.11f || top > h * 0.925f) continue;
            const auto colour = colours[row];
            const auto& l = lane[row];
            if (row < 7 || !isLaneType(row, PnordeLane::Type::ccMacro))
                label(laneDisplayName(row),
                    {juce::roundToInt(w * 0.064f), juce::roundToInt(top + h * 0.011f),
                     juce::roundToInt(w * 0.105f), juce::roundToInt(h * 0.030f)},
                    (t.paper.getPerceivedBrightness() < 0.50f
                        ? colour.brighter(0.72f) : colour),
                    h * (row < 7 ? 0.020f : 0.0145f), true);
            // No permanent SEQ LENGTH caption: the length box gives contextual help.
            auto lengthRect = lengthBox(row).toNearestInt();
            const auto numeralColour = colour.getPerceivedBrightness() > 0.53f
                ? juce::Colours::black : juce::Colours::white;
            label(juce::String(l.length), lengthRect, numeralColour, h * 0.020f, true);
            // Neon arrows are constant (not polyrhythmically animated).
            g.setColour(colour.brighter(0.65f));
            g.setFont(font(h * 0.017f, true));
            g.drawText("<", lengthRect.removeFromLeft(juce::jmax(13, lengthRect.getWidth()/5)), juce::Justification::centred);
            auto rightArrowRect = lengthBox(row).toNearestInt();
            g.drawText(">", rightArrowRect.removeFromRight(juce::jmax(13, rightArrowRect.getWidth()/5)), juce::Justification::centred);
            for (int i = 0; i < visibleSteps; ++i)
            {
                const int index = i + l.page * visibleSteps;
                if (index >= l.length) continue;
                const auto rect = cell(row, i);
                const int val = l.values[index];
                juce::String content;
                if (row == 0) content = processor.getStepEnabled(index) ? "ON" : "OFF";
                else if (isLaneType(row, PnordeLane::Type::midiChannel)) content = val ? juce::String(val) : "D";
                else if (isLaneType(row, PnordeLane::Type::pan))
                    content = val == 64 ? "C" : val < 64 ? "L" + juce::String(64 - val) : "R" + juce::String(val - 64);
                else if (isLaneType(row, PnordeLane::Type::vibrato))
                    content = "R" + juce::String(val) + "/D" + juce::String(processor.getVibratoDepth(index));
                else if (isLaneType(row, PnordeLane::Type::sustain)) content = val >= 50 ? "ON" : "OFF";
                else if (row == 1) content = juce::String(val / 10.0, 1);
                else if (row == 4) content = juce::String(val);
                else content = juce::String(val) + "%";
                const bool activeNumber = processor.isHostPlaying() && l.activeStep == index;
                label(stepNumberLabel(index),
                    {juce::roundToInt(rect.getX()), juce::roundToInt(top + h * 0.029f),
                     juce::roundToInt(rect.getWidth()), juce::roundToInt(h * 0.017f)},
                    activeNumber ? colour.brighter(0.95f) : colour.withAlpha(0.84f),
                    h * 0.0135f, activeNumber);
                label(content, rect.toNearestInt(),
                    t.paper.getPerceivedBrightness() < 0.50f ? colour.brighter(0.45f) : colour,
                    h * (isLaneType(row, PnordeLane::Type::vibrato) ? 0.009f : 0.0135f), true);
                if (l.activeStep == index && processor.isHostPlaying())
                {
                    g.setColour(colour.brighter(0.35f));
                    g.drawRoundedRectangle(rect.expanded(1.3f), 4.5f, 1.9f);
                }
            }
        }
    }
    label("MIDI OUT", {juce::roundToInt(w * 0.012f), juce::roundToInt(h * 0.946f),
        juce::roundToInt(w * 0.047f), juce::roundToInt(h * 0.023f)},
        t.teal, h * 0.010f, true);
    // Footer credit is crisp and STATIC; no glow or light-bus redraws.
    label("Vibe Coded By",
        {juce::roundToInt(w * 0.673f), juce::roundToInt(h * 0.940f),
         juce::roundToInt(w * 0.119f), juce::roundToInt(h * 0.032f)},
        t.pink, h * 0.012f);
    g.setColour(t.pink);
    g.setFont(font(h * 0.023f, true));
    g.drawText(juce::String::fromUTF8("LДNTERNS"),
        juce::Rectangle<int>{juce::roundToInt(w * 0.796f), juce::roundToInt(h * 0.938f),
         juce::roundToInt(w * 0.141f), juce::roundToInt(h * 0.039f)},
        juce::Justification::centred, false);
}

void PolyrhythmMIDIEditor::paintOverChildren(juce::Graphics& g)
{
    const auto& palette = themes[themeIndex];
    const auto ink = palette.ink;
    const juce::Colour colours[] {palette.coral, palette.blue, palette.sage, palette.lavender,
        palette.amber, palette.teal, palette.pink, palette.teal, palette.amber,
        palette.blue, palette.coral, palette.lavender, palette.sage, palette.pink,
        palette.amber, palette.teal, palette.blue, palette.lavender,
        palette.pink, palette.sage, palette.amber};
    const int w = getWidth(), h = getHeight();
    juce::Graphics::ScopedSaveState restoreAll(g);
    // Parent paintOverChildren is composited after its children. Never repaint
    // over the CallOutBox, including its border, even if it overlaps lanes.
    if (settingsCallout != nullptr)
        g.excludeClipRegion(settingsCallout->getBounds().reduced(3));

    // The lighting bus is restricted to the sequencer rectangle. Header,
    // footer, normal labels and native controls do not participate.
    if (processor.getExperimentalLightsEnabled())
    {
        juce::Graphics::ScopedSaveState laneClip(g);
        g.reduceClipRegion(0, juce::roundToInt(h * 0.110f), w,
                           juce::roundToInt(h * 0.815f));
        const float darkness = processor.getDarknessPercent() / 100.0f;
        if (darkness > 0.001f)
        {
            // v0.5.6.1: Restore the full-width v0.5.5 work-area dimmer.
            // v0.5.6 restricted it to the step grid; light-theme side panels
            // therefore stayed glaringly bright when DARKNESS was enabled.
            // Keep only the four kinds of interactive lane dropdowns and Macro
            // text editors clear. Exclusions apply ONLY to this darkness pass,
            // never to the step-light pass that follows.
            {
                juce::Graphics::ScopedSaveState dimmerClip(g);
                const int contentToEditorY = juce::roundToInt(h * 0.110f - scrollY);
                const auto protectControl = [&g, contentToEditorY](const juce::Component& control)
                {
                    if (!control.isVisible()) return;
                    auto bounds = control.getBounds();
                    bounds.translate(0, contentToEditorY);
                    if (!bounds.isEmpty()) g.excludeClipRegion(bounds);
                };
                for (int row = 0; row < lanes; ++row)
                {
                    if (!rowVisible(row)) continue;
                    const float top = rowTop(row);
                    if (top > h * 0.925f || top + h * 0.105f < h * 0.110f)
                        continue;
                    // Do not cut a rectangular hole in the darkness for a round dial.
                    // The circles/rings are rendered over the dark mask below.
                    if (row >= 7)
                    {
                        const int slot = row - 7;
                        protectControl(laneTypeSelect[slot]);
                        protectControl(ccSelect[slot]);
                        // Preserve native Macro editing/caret visibility. The
                        // editor's text must not be drawn a second time.
                        protectControl(ccLabelEditors[slot]);
                        if (deleteCC[slot].isMouseOver()) protectControl(deleteCC[slot]);
                    }
                }
                g.setColour(juce::Colours::black.withAlpha(darkness));
                g.fillRect(0, juce::roundToInt(h * 0.110f), w,
                           juce::roundToInt(h * 0.815f));
            }
            // Overlay only the two small luminous dial faces after darkness.
            // No excluded rectangular regions, no animated control bloom.
            for (int row = 0; row < lanes; ++row)
            {
                if (!rowVisible(row)) continue;
                const float top = rowTop(row);
                if (top > h * 0.925f || top + h * 0.105f < h * 0.110f) continue;
                const int translateY = juce::roundToInt(h * 0.110f - scrollY);
                const auto a = clockDials[row].getBounds();
                const auto b = timingDials[row].getBounds();
                clockDials[row].drawIllumination(g, (float)a.getX(), (float)(a.getY()+translateY));
                timingDials[row].drawIllumination(g, (float)b.getX(), (float)(b.getY()+translateY));
            }
            // Titles are static text in the underlay; redraw only the fixed
            // lane names above the dimmer. No re-render of Macro text editors.
            // This does not modulate their brightness with the lane clocks.
            for (int row = 0; row < lanes; ++row)
            {
                if (!rowVisible(row)) continue;
                const float top = rowTop(row);
                if (top > h * 0.925f || top + h * 0.105f < h * 0.110f)
                    continue;
                if (row >= 7 && isLaneType(row, PnordeLane::Type::ccMacro))
                    continue;
                const auto colour = colours[row];
                g.setColour(colour.brighter(0.72f));
                g.setFont(font(h * (row < 7 ? 0.020f : 0.0145f), true));
                g.drawText(laneDisplayName(row),
                    juce::Rectangle<int>{juce::roundToInt(w * 0.064f),
                        juce::roundToInt(top + h * 0.011f),
                        juce::roundToInt(w * 0.105f),
                        juce::roundToInt(h * 0.030f)},
                    juce::Justification::centred, false);
            }
        }
        const bool playingLights = processor.getAnimationsEnabled() && processor.isHostPlaying();
        for (int row = 0; row < lanes; ++row)
        {
            if (!rowVisible(row)) continue;
            const float top = rowTop(row);
            if (top > h * 0.925f || top + h * 0.105f < h * 0.110f) continue;
            if (!g.getClipBounds().toFloat().intersects(
                    juce::Rectangle<float>(0, top - h * 0.013f,
                                           static_cast<float>(w), h * 0.128f)))
                continue;
            const auto& l = lane[static_cast<size_t>(row)];
            const auto colour = colours[row];
            const float led = l.stepLed;
            // Circuit-board light mask: every current-page step gets a faint
            // backlight. Filled controls remain theme-coloured above DARKNESS.
            // A short trail moves at this lane's real clock division, with
            // different lane phases supplied by their independent MIDI clocks.
            for (int i = 0; i < visibleSteps; ++i)
            {
                const int index = l.page * visibleSteps + i;
                if (index >= l.length) continue;
                const auto bounds = cell(row, i);
                const int length = juce::jmax(1, l.length);
                const int current = l.activeStep;
                const int behind = current < 0 ? length
                    : (current - index + length) % length;
                const int ahead = current < 0 ? length
                    : (index - current + length) % length;
                const float phase = l.stepPhase;
                // Only nearby cells can have a perceptible travelling halo.
                // Avoid dozens of unnecessary exponential calls per repaint.
                const float trail = current < 0 ? 0.0f :
                    juce::jlimit(0.0f, 1.0f,
                        (behind < 5 ? std::exp(-1.4f * (static_cast<float>(behind) + phase)) : 0.0f)
                        + (ahead < 3 ? 0.13f * std::exp(-1.8f * static_cast<float>(ahead)) : 0.0f));
                const float motion = processor.getAnimationsEnabled() ? trail : 0.0f;
                const bool gateOn = row == 0 && processor.getStepEnabled(index);
                PnordeLights::cellBackdrop(g, bounds, colour, darkness,
                    motion, gateOn);
                if (row == 0)
                {
                    if (gateOn)
                        PnordeLights::cellFill(g, bounds.reduced(1.6f), colour,
                            darkness, motion, current == index ? led : 0.0f);
                }
                else if (!isLaneType(row, PnordeLane::Type::midiChannel))
                {
                    const float f = juce::jlimit(0.0f, 1.0f, l.visualFill[index]);
                    const auto fill = parameterFillBounds(row, bounds, f, processor.getOptionalLaneType(row));
                    PnordeLights::cellFill(g, fill, colour, darkness,
                        motion, current == index ? led : 0.0f);
                    if (isLaneType(row, PnordeLane::Type::vibrato))
                    {
                        const auto inner = bounds.reduced(2.0f);
                        const float depth = processor.getVibratoDepth(index) / 100.0f;
                        PnordeLights::cellFill(g,
                            inner.withWidth(inner.getWidth() * depth).withHeight(3.0f),
                            colour, darkness, motion * 0.65f,
                            current == index ? led * 0.6f : 0.0f);
                    }
                }
            }
            PnordeLights::draw(g,
                {w * 0.055f, top - h * 0.005f, 4.0f, h * 0.085f},
                colour, 0.13f + 0.87f * led, darkness, 1.0f, 2.0f);

            // Enabled gate indicators shine steadily, then flash white when
            // the gate is the current step. Only one SEQ row has these LEDs.
            if (row == 0)
            {
                for (int i = 0; i < visibleSteps; ++i)
                {
                    const int index = l.page * visibleSteps + i;
                    if (index >= l.length || !processor.getStepEnabled(index)) continue;
                    const auto b = cell(row, i);
                    const float intensity = index == l.activeStep
                        ? juce::jlimit(0.0f, 1.0f, 0.55f + led * 0.45f)
                        : 0.16f;
                    PnordeLights::draw(g,
                        {b.getRight() - 6.0f, b.getY() + 2.0f, 3.0f, 5.0f},
                        colour, intensity, darkness, 0.35f, 1.0f);
                }
            }

            // Step outlines chase the currently selected beat, advancing at
            // each lane's own division. No per-step animation allocations.
            if (l.activeStep >= 0 && l.activeStep < l.length)
            {
                const int localStep = l.activeStep - l.page * visibleSteps;
                if (localStep >= 0 && localStep < visibleSteps)
                {
                    const auto bounds = cell(row, localStep);
                    const float phase = l.stepPhase;
                    const float shimmer = PnordeLights::shimmer(phase, row, 1.0);
                    const float outlineIntensity = juce::jlimit(0.0f, 1.0f,
                        0.42f + led * 0.44f + 0.12f * shimmer);
                    PnordeLights::outline(g, bounds.expanded(1.5f), colour,
                        outlineIntensity, darkness);
                    // Bright beat-progress trace on the current step outline.
                    const float travel = bounds.getWidth() * phase;
                    g.setColour(colour.interpolatedWith(juce::Colours::white,
                        0.40f + 0.42f * led).withAlpha(0.48f + 0.45f * shimmer));
                    g.fillRoundedRectangle(bounds.getX() + 2.0f, bounds.getY() + 1.0f,
                        juce::jmax(1.0f, travel - 4.0f), 2.0f, 1.0f);

                    // Illuminated per-parameter level slider tip.
                    if (row != 0 && !isLaneType(row, PnordeLane::Type::midiChannel))
                    {
                        const int index = l.activeStep;
                        const float f = juce::jlimit(0.0f, 1.0f, l.visualFill[index]);
                        const float y = bounds.getBottom() - 2.0f - f * (bounds.getHeight() - 4.0f);
                        const float side = 2.0f + (0.7f * led);
                        PnordeLights::draw(g,
                            {bounds.getRight() - 5.0f, y - side * 0.5f, 3.5f, side},
                            colour, juce::jlimit(0.0f, 1.0f,
                                0.32f + 0.45f * PnordeLights::shimmer(phase, row, 2.0)),
                            darkness, 0.42f, 1.0f);
                    }
                }
            }
            // The current step number must be luminous ABOVE Experimental Darkness.
            // Redraw only one label per visible lane; the other numbers stay static.
            if (playingLights && l.activeStep >= 0 && l.activeStep < l.length)
            {
                const int localStep = l.activeStep - l.page * visibleSteps;
                if (localStep >= 0 && localStep < visibleSteps)
                {
                    const auto b = cell(row, localStep);
                    g.setColour(colour.brighter(0.95f));
                    g.setFont(font(h * 0.0145f, true));
                    g.drawText(stepNumberLabel(l.activeStep),
                        juce::Rectangle<int>(juce::roundToInt(b.getX()),
                            juce::roundToInt(top + h * 0.029f),
                            juce::roundToInt(b.getWidth()), juce::roundToInt(h * 0.017f)),
                        juce::Justification::centred);
                }
            }
            // The draggable end-of-sequence marker is always bright, not tempo-pulsed.
            if (l.page == (l.length - 1) / visibleSteps)
            {
                const auto x = l.displayX < 0.0f ? markerX(row) : l.displayX;
                const auto y = top + h * 0.025f;
                g.setColour(colour.brighter(0.55f).withAlpha(0.92f));
                g.fillRoundedRectangle(x-2.0f, top + h*0.039f,
                    4.0f, h*0.051f, 1.7f);
                juce::Path triangle;
                triangle.addTriangle(x-7.0f,y-5.0f,x+7.0f,y-5.0f,x,y+7.0f);
                g.fillPath(triangle);
            }
            // Clock division, lane type and CC number controls are statically
            // backlit through their native JUCE themed backgrounds, not animated.

            // Bright, steady label/length controls. The narrow vertical rail
            // above is the ONLY polyrhythmically flashing lane-header element.
            if (darkness > 0.005f)
            {
                const auto box = lengthBox(row);
                g.setColour(colour.withAlpha(0.96f));
                g.fillRoundedRectangle(box, 6.0f);
                g.setFont(font(h * 0.020f, true));
                g.setColour(colour.getPerceivedBrightness() > 0.53f
                    ? juce::Colours::black : juce::Colours::white);
                g.drawText(juce::String(l.length), box.toNearestInt(), juce::Justification::centred);
                g.setColour(colour.brighter(0.65f));
                g.setFont(font(h * 0.017f, true));
                auto left = box.toNearestInt(); auto right = left;
                g.drawText("<", left.removeFromLeft(juce::jmax(13, left.getWidth()/5)), juce::Justification::centred);
                g.drawText(">", right.removeFromRight(juce::jmax(13, right.getWidth()/5)), juce::Justification::centred);
            }

        }
    }

    // Experimental reorder indicator is separate from data and MIDI routing.
    if (laneDragActive && dragTargetRow >= 7 && rowVisible(dragTargetRow))
    {
        const float y = rowTop(dragTargetRow) - h * 0.012f;
        g.setColour(palette.cyan.withAlpha(0.88f));
        g.fillRoundedRectangle(w * 0.050f, y, w * 0.917f, 3.0f, 1.5f);
    }
    // SLIDE is intentionally visible and interactive in microtonal mode:
    // its saved 0..100% values set the tempo-relative pitch approach time.
    // Contextual help only while a sequence-length box is pressed/dragged.
    // The lane title remains uncluttered when the control is idle.
    for (int row = 0; row < lanes; ++row)
    {
        if (!rowVisible(row) || !lane[row].lengthDrag) continue;
        const auto b = lengthBox(row);
        const float helpW = juce::jmin(210.0f, w * 0.20f);
        const float x = juce::jlimit(8.0f, w - helpW - 8.0f, b.getX());
        const float y = juce::jlimit(h * 0.115f, h * 0.925f - 28.0f,
                                   b.getBottom() + 5.0f);
        const juce::Rectangle<float> popup(x, y, helpW, 24.0f);
        g.setColour(palette.paper.contrasting(0.14f));
        g.fillRoundedRectangle(popup, 4.0f);
        g.setColour(colours[row].brighter(0.30f));
        g.drawRoundedRectangle(popup, 4.0f, 1.4f);
        g.setFont(font(12.0f, true));
        g.setColour(palette.ink);
        g.drawText("Set Sequence Length: " + juce::String(lane[row].length),
                   popup.toNearestInt(), juce::Justification::centred, false);
        break;
    }
    // Transient widgets are UI feedback, NOT part of the Experimental bus.
    // Protect the work-area clipping but let the popup extend above the cell.
    // Slim glowing scrollbar that fades after interaction.
    if (maxScroll() > 0.5f && scrollbarOpacity > 0.01f)
    {
        const float top = h * 0.115f;
        const float trackHeight = h * 0.802f;
        const float thumbHeight = juce::jmax(25.0f, trackHeight *
            (trackHeight / (trackHeight + maxScroll())));
        const float y = top + (trackHeight - thumbHeight) *
            juce::jlimit(0.0f, 1.0f, scrollY / maxScroll());
        const float x = w * 0.987f;
        g.setColour(palette.teal.withAlpha(scrollbarOpacity * 0.15f));
        g.fillRoundedRectangle(x - 4.0f, y - 2.0f, 10.0f, thumbHeight + 4.0f, 5.0f);
        g.setColour(palette.cyan.withAlpha(scrollbarOpacity * 0.85f));
        g.fillRoundedRectangle(x, y, 3.0f, thumbHeight, 2.0f);
    }
    if (ccPopupRow >= 6 && ccPopupRow < lanes &&
        (ccPopupRow == 6 || (ccPopupRow >= 7 &&
         !isLaneType(ccPopupRow, PnordeLane::Type::sustain) &&
         !isLaneType(ccPopupRow, PnordeLane::Type::midiChannel))) &&
        ccPopupStep >= 0 && ccPopupStep < PolyrhythmMIDIProcessor::maxSteps &&
        ccPopupAlpha > 0.0f)
    {
        const int slot = ccPopupRow - 7;
        const int percent = ccPopupRow == 6 ? processor.getAftertouchGlide(ccPopupStep)
            : isLaneType(ccPopupRow, PnordeLane::Type::ccMacro)
                ? processor.getCCTransitionMs(slot, ccPopupStep)
                : processor.getOptionalGlide(ccPopupRow, ccPopupStep);
        const int scaled = juce::jlimit(0, 100, percent);
        const int visibleIndex = ccPopupStep - lane[static_cast<size_t>(ccPopupRow)].page * 16;
        if (visibleIndex < 0 || visibleIndex >= visibleSteps) return;
        const auto stepBounds = cell(ccPopupRow, visibleIndex);
        const float x = juce::jlimit(45.0f, w - 45.0f, stepBounds.getCentreX());
        // Anchor above the step, regardless of mouse movement. Can overlap adjacent rows.
        const float y = juce::jlimit(62.0f, h - 48.0f, stepBounds.getY() - 44.0f);
        const juce::Rectangle<float> dial(x - 32.0f, y - 32.0f, 64.0f, 64.0f);
        g.setColour(juce::Colours::white.withAlpha(ccPopupAlpha * 0.96f));
        g.fillEllipse(dial);
        g.setColour(ink.withAlpha(ccPopupAlpha * 0.18f));
        g.drawEllipse(dial.reduced(5.0f), 5.0f);
        const float start = juce::MathConstants<float>::pi * 0.75f;
        juce::Path arc;
        arc.addCentredArc(x, y, 25.0f, 25.0f, 0.0f, start,
            start + juce::MathConstants<float>::pi * 1.5f * scaled / 100.0f, true);
        const auto dialColour = glideColour(colours[static_cast<size_t>(ccPopupRow)], scaled / 100.0f);
        g.setColour(dialColour.withAlpha(ccPopupAlpha));
        g.strokePath(arc, juce::PathStrokeType(4.5f));
        g.setColour(juce::Colours::black.withAlpha(ccPopupAlpha));
        g.setFont(font(14.0f, true));
        const juce::String glideText = percent < 0 ? "AUTO"
            : percent > 100 ? juce::String((percent-101)*0.1f,1)+" s"
            : juce::String(scaled) + "%"; // New glide is tempo-relative.
        g.drawText(glideText, dial.toNearestInt(), juce::Justification::centred);
        const juce::Rectangle<float> heading(x - 48.0f, y - 54.0f, 96.0f, 20.0f);
        g.setColour(palette.cream.withAlpha(ccPopupAlpha * 0.98f));
        g.fillRoundedRectangle(heading, 4.0f);
        g.setColour(palette.ink.withAlpha(ccPopupAlpha));
        g.setFont(font(11.0f, true));
        g.drawText("GLIDE TIME", heading.toNearestInt(), juce::Justification::centred);

    }
}

void PolyrhythmMIDIEditor::mouseDown(const juce::MouseEvent& e)
{
    if (!e.mods.isLeftButtonDown()) return;
    // For events forwarded from the editable Macro label, coordinates are
    // relative to the editor, not to the TextEditor child.
    const auto local = e.getEventRelativeTo(this).position;
    dragFromRow = -1;
    dragTargetRow = -1;
    laneDragActive = false;
    if (processor.getExperimentalReorderEnabled() &&
        local.y >= getHeight() * 0.110f && local.y < getHeight() * 0.925f)
    {
        for (int row = 7; row < lanes; ++row)
        {
            if (!rowVisible(row)) continue;
            if (reorderHandle(row).contains(local))
            {
                dragFromRow = dragTargetRow = row;
                laneDragOrigin = local;
                repaint();
                return; // No child text editor receives the event.
            }
        }
    }
    if (e.eventComponent != this) return;
    if (local.y < getHeight() * 0.110f || local.y >= getHeight() * 0.925f) return;
    for (int row = 0; row < lanes; ++row)
    {
        if (!rowVisible(row)) continue;
        auto& l = lane[static_cast<size_t>(row)];
        if (lengthBox(row).contains(e.position))
        {
            l.lengthDrag = true; l.startLength = l.length; l.dragOrigin = e.getPosition();
            repaint(); // Show Set Sequence Length help immediately on click-and-hold.
            return;
        }
        if (l.page == (l.length - 1) / 16 && markerBox(row).contains(e.position))
        {
            l.markerDrag = true; l.rippleX = markerX(row); l.rippleProgress = 0.0f; return;
        }
        // Clicking a step NUMBER (not its value bar) sets the lane length.
        // The audio/MIDI length changes immediately; the boundary arrow springs
        // visually toward its new position in timerCallback().
        for (int i = 0; i < visibleSteps; ++i)
        {
            const auto bounds = cell(row, i);
            const juce::Rectangle<float> numberHit(bounds.getX(),
                rowTop(row) + getHeight() * 0.027f,
                bounds.getWidth(), getHeight() * 0.023f);
            if (numberHit.contains(e.position))
            {
                l.numberDrag = true;
                setLength(row, l.page * 16 + i + 1, false);
                l.rippleX = markerX(row);
                l.rippleProgress = 0.0f;
                return;
            }
        }
        for (int i = 0; i < visibleSteps; ++i)
        {
            const int index = l.page * 16 + i;
            if (index >= l.length || !cell(row, i).contains(e.position)) continue;
            if (row == 0)
                processor.setStepEnabled(index, !processor.getStepEnabled(index));
            else
            {
                l.valueDrag = true;
                l.draggedStep = index;
                l.dragStartValue = l.values[static_cast<size_t>(index)];
                l.dragOrigin = e.getPosition();
                if (isLaneType(row, PnordeLane::Type::sustain))
                {
                    const int newValue = l.values[index] >= 50 ? 0 : 100;
                    processor.setLaneValue(row, index, newValue);
                    l.values[index] = newValue;
                    l.valueDrag = false;
                }
                if (isLaneType(row, PnordeLane::Type::vibrato))
                    l.dragStartDepth = processor.getVibratoDepth(index);
                const bool eligibleGlide = row == 6 || (row >= 7 &&
                    (isLaneType(row, PnordeLane::Type::ccMacro) ||
                     isLaneType(row, PnordeLane::Type::modWheel) ||
                     isLaneType(row, PnordeLane::Type::expression) ||
                     isLaneType(row, PnordeLane::Type::vibrato) ||
                     isLaneType(row, PnordeLane::Type::pan)));
                if (eligibleGlide)
                {
                    const int previous = row == 6 ? processor.getAftertouchGlide(index)
                        : isLaneType(row, PnordeLane::Type::ccMacro) ? processor.getCCTransitionMs(row-7, index)
                        : processor.getOptionalGlide(row, index);
                    l.dragStartTransitionMs = juce::jlimit(0, 100, previous);
                    ccPopupRow = row;
                    ccPopupStep = index;
                    ccPopupAlpha = 1.0f;
                    ccPopupPoint = e.position;
                }
            }
            repaint(); return;
        }
    }
}
void PolyrhythmMIDIEditor::updateDragValue(int row, const juce::MouseEvent& e)
{
    auto& l = lane[static_cast<size_t>(row)];
    if (!l.valueDrag || l.draggedStep < 0) return;
    const int dy = l.dragOrigin.y - e.getPosition().y;
    const bool macro = row >= 7 && isLaneType(row, PnordeLane::Type::ccMacro);
    const bool vibrato = isLaneType(row, PnordeLane::Type::vibrato);
    const bool pan = isLaneType(row, PnordeLane::Type::pan);
    const bool dedicatedGlide = row >= 7 && (
        isLaneType(row, PnordeLane::Type::modWheel) ||
        isLaneType(row, PnordeLane::Type::expression) || vibrato || pan);
    const int dx = e.getPosition().x - l.dragOrigin.x;
    // PAN: left/right -> CC10 (0..127), up/down -> Glide 0..100.
    // These two axes never influence each other.
    // VIBRATO retains horizontal depth; Shift+horizontal adjusts glide.
    if (row == 6 || macro || dedicatedGlide)
    {
        const bool editingGlide = !vibrato || e.mods.isShiftDown();
        if ((pan ? dy != 0 : dx != 0) && editingGlide)
        {
            const float glideSensitivity = e.mods.isShiftDown() ? 6.0f : 2.0f;
            const int glideDelta = pan ? dy : dx;
            const int glide = juce::jlimit(0, 100,
                l.dragStartTransitionMs + juce::roundToInt(glideDelta / glideSensitivity));
            if (row == 6) processor.setAftertouchGlide(l.draggedStep, glide);
            else if (macro) processor.setCCTransitionMs(row-7, l.draggedStep, glide);
            else processor.setOptionalGlide(row, l.draggedStep, glide);
        }
        ccPopupAlpha = 1.0f;
    }
    if (vibrato && !e.mods.isShiftDown())
        processor.setVibratoDepth(l.draggedStep,
            l.dragStartDepth + juce::roundToInt(dx / 2.5f));
    // EDO/quartertone scales need 10-cent pitch-lane movement: rounding to
    // 12-TET semitones before quantising would make some notes unreachable.
    const int selectedScale = processor.getScale();
    const bool finePitch = selectedScale == 13 || selectedScale == 32 ||
                           selectedScale == 33 || PnordeEDO::isMode(selectedScale);
    const float sensitivity = e.mods.isShiftDown() ? 12.0f : row == 1 && finePitch ? 1.8f : 3.5f;
    const int maximum = isLaneType(row, PnordeLane::Type::midiChannel) ? 16 : isLaneType(row, PnordeLane::Type::pan) ? 127 : row == 1 ? 120 : row == 4 ? 4 : 100;
    const int minimum = row == 1 ? -120 : row == 4 ? -4 : row >= 5 ? 0 : 1;
    // For Pan, a vertical-only drag cannot modify the Pan value.
    const int amount = juce::roundToInt((pan ? dx : dy) /
        (pan ? (e.mods.isShiftDown() ? 6.0f : 2.0f) : sensitivity));
    const int value = juce::jlimit(minimum, maximum, l.dragStartValue + amount);
    const int adjusted = row == 1 && selectedScale != 13
        ? processor.quantizePitchTenths(
            finePitch ? value : juce::roundToInt(value/10.0)*10,0)
        : value;
    const auto step = static_cast<size_t>(l.draggedStep);
    l.values[step] = adjusted;
    // Keep the audio engine in sync so the timer cannot reset the dragged value.
    processor.setLaneValue(row, l.draggedStep, adjusted);
    // Animate towards the new value immediately; the timer continues the easing.
    const float targetFill = isLaneType(row, PnordeLane::Type::midiChannel) ? adjusted / 16.0f : isLaneType(row, PnordeLane::Type::pan) ? adjusted / 127.0f : row == 1 ? (adjusted + 120.0f) / 240.0f
                           : row == 4 ? (adjusted + 4.0f) / 8.0f : adjusted / 100.0f;
    l.visualFill[step] += (targetFill - l.visualFill[step]) * 0.45f;
    repaint();
}
void PolyrhythmMIDIEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (dragFromRow >= 7)
    {
        const auto pos = e.getEventRelativeTo(this).position;
        if (std::abs(pos.y - laneDragOrigin.y) > 8.0f)
            laneDragActive = true;
        if (!laneDragActive) return;
        // Smooth scrolling of the normal viewport is retained during drag.
        if (pos.y < getHeight() * 0.16f) targetScrollY -= getHeight() * 0.03f;
        if (pos.y > getHeight() * 0.87f) targetScrollY += getHeight() * 0.03f;
        float nearest = std::numeric_limits<float>::max();
        for (int row = 7; row < lanes; ++row)
        {
            if (!rowVisible(row)) continue;
            const float d = std::abs(pos.y - (rowTop(row) + getHeight() * 0.025f));
            if (d < nearest) { nearest = d; dragTargetRow = row; }
        }
        repaint();
        return;
    }
    if (e.eventComponent != this) return;
    for (int row = 0; row < lanes; ++row)
    {
        auto& l = lane[static_cast<size_t>(row)];
        if (l.valueDrag) updateDragValue(row, e);
        if (l.numberDrag)
        {
            for (int i = 0; i < visibleSteps; ++i)
            {
                const auto bounds = cell(row, i);
                if (e.position.x >= bounds.getX() && e.position.x < bounds.getRight())
                {
                    const int length = l.page * 16 + i + 1;
                    if (length != l.length) setLength(row, length, false);
                    break;
                }
            }
        }
        if (l.lengthDrag)
        {
            const auto d = e.getPosition() - l.dragOrigin;
            const float divisor = e.mods.isShiftDown() ? 28.0f : 8.0f;
            setLength(row, l.startLength + juce::roundToInt((d.x - d.y) / divisor), true);
        }
        if (l.markerDrag)
        {
            const int base = l.page * 16;
            int best = base + 1;
            float distance = std::abs(e.position.x - markerXFor(row, best));
            for (int i = 2; i <= 16; ++i)
            {
                const float candidate = std::abs(e.position.x - markerXFor(row, base + i));
                if (candidate < distance) {distance = candidate; best = base + i;}
            }
            setLength(row, best, false);
        }
    }
}
void PolyrhythmMIDIEditor::mouseUp(const juce::MouseEvent&)
{
    if (dragFromRow >= 7)
    {
        if (laneDragActive && dragTargetRow >= 7 && dragTargetRow != dragFromRow)
        {
            processor.moveDisplayRow(dragFromRow, dragTargetRow);
            // Only the visual permutation changes: same physical MIDI rows,
            // automation parameter IDs and step data remain untouched.
            refreshMacroPlaceholders();
            resized();
        }
        dragFromRow = dragTargetRow = -1;
        laneDragActive = false;
        repaint();
    }
    // Dismiss the contextual length helper immediately on mouse release,
    // including when the host transport is stopped (no animation repaint).
    bool dismissedLengthHelper = false;
    for (auto& l : lane)
    {
        dismissedLengthHelper |= l.lengthDrag;
        l.lengthDrag = false; l.markerDrag = false;
        l.numberDrag = false; l.valueDrag = false; l.draggedStep = -1;
    }
    if (dismissedLengthHelper) repaint();
}
void PolyrhythmMIDIEditor::mouseMove(const juce::MouseEvent& e)
{
    bool overRail = false;
    if (processor.getExperimentalReorderEnabled())
        for (int row = 7; row < lanes; ++row)
            if (rowVisible(row) && reorderHandle(row).contains(e.position))
                { overRail = true; break; }
    setMouseCursor(overRail ? juce::MouseCursor::DraggingHandCursor
                            : juce::MouseCursor::NormalCursor);
    for (int row = 0; row < lanes; ++row)
    {
        auto& l = lane[static_cast<size_t>(row)];
        l.hovered = -1;
        if (!rowVisible(row)) continue;
        for (int i = 0; i < visibleSteps; ++i)
        {
            const auto bounds = cell(row, i);
            const juce::Rectangle<float> numberHit(bounds.getX(),
                rowTop(row) + getHeight() * 0.027f,
                bounds.getWidth(), getHeight() * 0.023f);
            if (numberHit.contains(e.position)) { l.hovered = i; break; }
        }
    }
    repaint();
}
void PolyrhythmMIDIEditor::mouseExit(const juce::MouseEvent&)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
    for (auto& l : lane) l.hovered = -1;
    repaint();
}
void PolyrhythmMIDIEditor::mouseDoubleClick(const juce::MouseEvent& e)
{
    for (int row = 0; row < lanes; ++row)
    {
        if (!rowVisible(row)) continue;
        if (lengthBox(row).contains(e.position)) { setLength(row, 16, true); return; }
        for (int i = 0; i < visibleSteps; ++i)
        {
            const int step = lane[row].page * visibleSteps + i;
            if (step >= processor.getLaneLength(row) || !cell(row, i).contains(e.position))
                continue;
            if (row == 0) return; // Note on/off is a toggle, not a value slider.
            const int reset = row == 2 ? 70 : row == 3 ? 50 : isLaneType(row, PnordeLane::Type::pan) ? 64 : 0;
            processor.setLaneValue(row, step, reset);
            if (row >= 7 && isLaneType(row, PnordeLane::Type::ccMacro))
                processor.setCCTransitionMs(row - 7, step, 0);
            if (row == 6) processor.setAftertouchGlide(step, 0);
            if (isLaneType(row, PnordeLane::Type::vibrato)) processor.setVibratoDepth(step, 0);
            lane[row].values[step] = reset;
            repaint();
            return;
        }
    }
}
void PolyrhythmMIDIEditor::timerCallback()
{
    const bool playing = processor.isHostPlaying();
    const bool animate = processor.getAnimationsEnabled();
    const bool experimentalLights = processor.getExperimentalLightsEnabled();
    const int desiredHz = experimentalLights ? 30 : 24;
    if (animationTimerHz != desiredHz) { animationTimerHz = desiredHz; startTimerHz(desiredHz); }
    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    const double sampleMs = processor.getHostClockUpdateMs();
    const double extrapolateMs = playing && sampleMs > 0.0
        ? juce::jlimit(0.0, 220.0, nowMs - sampleMs) : 0.0;
    const double bpm = juce::jmax(1.0, processor.getHostBpm());
    const double ppqNow = processor.getHostPpq() + extrapolateMs * bpm / 60000.0;
    static constexpr double divisionsBeats[] {0.125, 0.25, 0.5, 1.0, 2.0, 4.0,
        8.0, 16.0, 32.0, 64.0, 128.0};
    static constexpr double globalFactors[] {4.0, 2.0, 1.0, 0.5, 0.25};
    static constexpr double timeFactors[] {1.0, 2.0 / 3.0, 1.5};
    const double globalFactor = globalFactors[juce::jlimit(0, 4,
        processor.getGlobalClockIndex())];

    // The lane clock phases are computed below only for visible rows.

    bool shouldRepaint = (playing != lastTransportPlaying);
    bool fullRepaint = shouldRepaint;
    lastTransportPlaying = playing;
    if (animate && playing && experimentalLights) { pulse += 0.14; shouldRepaint = true; }
    if (scrollbarOpacity > 0.01f) {
        scrollbarOpacity *= 0.91f;
        shouldRepaint = true;
    }
    if (ccPopupRow >= 6 && ccPopupRow < lanes &&
        (ccPopupRow == 6 || (ccPopupRow >= 7 &&
         !isLaneType(ccPopupRow, PnordeLane::Type::sustain) &&
         !isLaneType(ccPopupRow, PnordeLane::Type::midiChannel))) &&
        !lane[static_cast<size_t>(ccPopupRow)].valueDrag)
    {
        ccPopupAlpha = juce::jmax(0.0f, ccPopupAlpha - 0.075f);
        if (ccPopupAlpha == 0.0f) ccPopupRow = -1;
        shouldRepaint = true;
        fullRepaint = true;
    }
    targetScrollY = juce::jlimit(0.0f, maxScroll(), targetScrollY);
    if (std::abs(targetScrollY - scrollY) > 0.06f)
    {
        scrollY += (targetScrollY - scrollY) * 0.32f;
        // Scrolling must not relayout 130+ child components on each frame.
        scrollContent.setTopLeftPosition(0, -juce::roundToInt(scrollY));
        shouldRepaint = true;
        fullRepaint = true;
    }
    else if (scrollY != targetScrollY)
    {
        scrollY = targetScrollY;
        scrollContent.setTopLeftPosition(0, -juce::roundToInt(scrollY));
        shouldRepaint = true;
        fullRepaint = true;
    }

    // Automation can change global UI parameters while the editor is open.
    const int updatedTheme = processor.getThemeIndex();
    if (updatedTheme != themeIndex)
    {
        themeIndex = updatedTheme;
        themeSelect.setSelectedId(themeIndex + 1, juce::dontSendNotification);
        applyTheme();
        shouldRepaint = true;
        fullRepaint = true;
    }
    const int darkness = processor.getDarknessPercent();
    if (juce::roundToInt(darknessSlider.getValue()) != darkness)
    {
        darknessSlider.setValue(darkness, juce::dontSendNotification);
        shouldRepaint = true;
        fullRepaint = true;
    }
    manualTempoToggle.setToggleState(processor.getManualTempoEnabled(), juce::dontSendNotification);
    manualTempoInput.setEnabled(processor.getManualTempoEnabled());
    if (!manualTempoInput.hasKeyboardFocus(true))
    {
        const auto bpmText = juce::String(processor.getManualTempoBpm(), 1);
        if (manualTempoInput.getText() != bpmText)
            manualTempoInput.setText(bpmText, false);
    }
    octaveDial.setIndex(processor.getGlobalOctave()+2);
    keyDial.setIndex(processor.getKey());
    scaleDial.setIndex(processor.getScale());
    globalClockDial.setIndex(processor.getGlobalClockIndex());
    for (int row = 0; row < lanes; ++row)
        if (rowVisible(row))
        {
            clockDials[row].setValue(processor.getLaneDivision(row));
            timingDials[row].setValue(processor.getLaneTiming(row));
        }
    if (globalClockSelect.getSelectedId() != processor.getGlobalClockIndex() + 1)
        globalClockSelect.setSelectedId(processor.getGlobalClockIndex() + 1, juce::dontSendNotification);
    const int effectiveBendId = processor.getEffectiveBendRangeIndex() + 1;
    if (bendRangeSelect.getSelectedId() != effectiveBendId)
        bendRangeSelect.setSelectedId(effectiveBendId, juce::dontSendNotification);
    const bool bendEditable = !processor.isMicrotonalScaleSelected();
    if (bendRangeSelect.isEnabled() != bendEditable)
        bendRangeSelect.setEnabled(bendEditable);
    // Never change a ComboBox selection while its popup is open: doing so
    // can cancel the user's first click on Key/Scale under some hosts.
    if (!keySelect.isPopupActive() && keySelect.getSelectedId() != processor.getKey() + 1)
        keySelect.setSelectedId(processor.getKey() + 1, juce::dontSendNotification);
    if (!scaleSelect.isPopupActive() && scaleSelect.getSelectedId() != processor.getScale() + 1)
        scaleSelect.setSelectedId(processor.getScale() + 1, juce::dontSendNotification);

    // Work only on visible rows and on their current 16-step page. Previously
    // this loop read and animated all 2,688 cells at 60 fps, even offscreen.
    for (int row = 0; row < lanes; ++row)
    {
        if (!rowVisible(row)) continue;
        auto& l = lane[static_cast<size_t>(row)];
        const int length = processor.getLaneLength(row);
        if (l.length != length) { l.length = length; shouldRepaint = true; }
        l.division = processor.getLaneDivision(row);
        const int step = processor.getLaneCurrentStep(row);
        if (l.activeStep != step) { l.activeStep = step; shouldRepaint = true; }
        const float top = rowTop(row);
        if (row != 0 && (top > getHeight() * 0.925f ||
                         top + getHeight() * 0.105f < getHeight() * 0.110f))
            continue;
        const int first = juce::jlimit(0, 112, l.page * 16);
        for (int i = first; i < first + visibleSteps; ++i)
        {
            // The currently playing step remains highlighted; previous steps
            // fade smoothly to their constant idle backlight, in both modes.
            const float previous = l.rangeOpacity[i];
            const float activity = (playing && i == l.activeStep) ? 1.0f : previous * 0.76f;
            l.rangeOpacity[i] = activity < 0.012f ? 0.0f : activity;
            if (std::abs(previous - l.rangeOpacity[i]) > 0.012f) shouldRepaint = true;
            const int value = processor.getLaneValue(row, i);
            if (l.values[i] != value) { l.values[i] = value; shouldRepaint = true; }
            const float targetFill = isLaneType(row, PnordeLane::Type::midiChannel) ? value / 16.0f
                : isLaneType(row, PnordeLane::Type::pan) ? value / 127.0f
                : row == 1 ? (value + 120.0f) / 240.0f
                : row == 4 ? (value + 4.0f) / 8.0f
                : value / 100.0f;
            if (!animate || !playing || !experimentalLights)
                l.visualFill[i] = targetFill;
            else if (std::abs(targetFill - l.visualFill[i]) > 0.001f)
            {
                l.visualFill[i] += (targetFill - l.visualFill[i]) * 0.42f;
                shouldRepaint = true;
            }
        }
        if (playing && animate && experimentalLights)
        {
            const double durationBeats = divisionsBeats[juce::jlimit(0, 10, l.division)]
                * globalFactor * timeFactors[juce::jlimit(0, 2, processor.getLaneTiming(row))];
            double phase = std::fmod(ppqNow / juce::jmax(0.00001, durationBeats), 1.0);
            if (phase < 0.0) phase += 1.0;
            l.stepPhase = static_cast<float>(phase);
            const double stepMs = durationBeats * 60000.0 / bpm;
            l.stepDurationMs = stepMs;
            l.stepLed = PnordeLights::pulse(phase * stepMs, stepMs);
            shouldRepaint = true;
        }
        else
        {
            l.stepPhase = 0.0f;
            l.stepLed = 0.0f;
        }
        const float target = markerX(row);
        if (l.displayX < 0.0f) l.displayX = target;
        if (animate)
        {
            l.markerVelocity = (l.markerVelocity + (target - l.displayX) * 0.17f) * 0.68f;
            l.displayX += l.markerVelocity;
        }
        else { l.displayX = target; l.markerVelocity = 0.0f; }
        if (std::abs(target - l.displayX) > 0.08f ||
            std::abs(l.markerVelocity) > 0.08f) shouldRepaint = true;
        else { l.displayX = target; l.markerVelocity = 0.0f; }
        if (l.rippleProgress < 1.0f)
        {
            l.rippleProgress = juce::jmin(1.0f, l.rippleProgress + 0.12f);
            shouldRepaint = true;
        }
    }
    if (shouldRepaint)
    {
        if (fullRepaint || !playing || !animate)
        {
            repaint();
        }
        else
        {
            // No per-frame header/footer repaints: both are now static.
            // On transport/theme/layout changes the full repaint path runs.
            const float h = static_cast<float>(getHeight());
            for (int row = 0; row < lanes; ++row)
            {
                if (!rowVisible(row)) continue;
                const float y = rowTop(row);
                if (y > h * 0.925f || y + h * 0.105f < h * 0.110f) continue;
                const int top = juce::jmax(juce::roundToInt(h * 0.110f),
                    juce::roundToInt(y - h * 0.013f));
                const int bottom = juce::jmin(juce::roundToInt(h * 0.925f),
                    juce::roundToInt(y + h * 0.108f));
                if (bottom > top) repaint(0, top, getWidth(), bottom - top);
            }
            if (scrollbarOpacity > 0.01f)
                repaint(juce::roundToInt(getWidth() * 0.975f),
                    juce::roundToInt(h * 0.110f),
                    juce::roundToInt(getWidth() * 0.025f),
                    juce::roundToInt(h * 0.815f));
        }
    }
}

float PolyrhythmMIDIEditor::maxScroll() const
{
    const float h = static_cast<float>(getHeight());
    const int count = visibleRowCount();
    return juce::jmax(0.0f, h * (0.155f + (count - 1) * 0.110f + 0.105f - 0.925f));
}
void PolyrhythmMIDIEditor::syncCCVisibility()
{
    for (int row = 0; row < lanes; ++row)
    {
        const bool visible = rowVisible(row);
        divisions[row].setVisible(false);
        timings[row].setVisible(false);
        clockDials[row].setVisible(visible);
        timingDials[row].setVisible(visible);
        prev[row].setVisible(visible);
        next[row].setVisible(visible);
        addPage[row].setVisible(visible);
        removePage[row].setVisible(visible);
        if (row >= 7)
        {
            const int slot = row - 7;
            const bool macro = isLaneType(row, PnordeLane::Type::ccMacro);
            deleteCC[slot].setVisible(visible);
            laneTypeSelect[slot].setVisible(visible);
            laneTypeSelect[slot].setSelectedId(processor.getOptionalLaneType(row) + 1,
                                                 juce::dontSendNotification);
            ccSelect[slot].setVisible(visible && macro);
            ccLabelEditors[slot].setVisible(visible && macro);
        }
    }
    refreshMacroPlaceholders();
    bool hasCapacity = processor.activeCCMacroCount() < 8;
    for (int kind = 1; kind <= 6; ++kind)
        if (!processor.hasActiveLaneType(kind)) hasCapacity = true;
    addCCButton.setEnabled(hasCapacity);
}

void PolyrhythmMIDIEditor::refreshMacroPlaceholders()
{
    int ordinal = 0;
    const auto textColour = themes[juce::jlimit(0, themeCount - 1, themeIndex)].ink;
    for (int pos = 0; pos < optionalLanes; ++pos)
    {
        const int row = processor.getDisplayRowAt(pos);
        if (!rowVisible(row) || !isLaneType(row, PnordeLane::Type::ccMacro)) continue;
        ccLabelEditors[static_cast<size_t>(row - 7)].setTextToShowWhenEmpty(
            "MACRO " + juce::String(++ordinal), textColour.withAlpha(0.92f));
    }
}

void PolyrhythmMIDIEditor::applyTooltips()
{
    const bool enabled = processor.getTooltipsEnabled();
    const auto hint = [enabled](const char* value) -> juce::String
        { return enabled ? juce::String(value) : juce::String(); };
    for (auto* d : { &octaveDial, &keyDial, &scaleDial, &globalClockDial }) d->setTooltipsEnabled(enabled);
    for (auto& dial : clockDials) dial.setTooltipsEnabled(enabled);
    for (auto& dial : timingDials) dial.setTooltipsEnabled(enabled);
    for (int slot = 0; slot < optionalLanes; ++slot)
    {
        ccLabelEditors[slot].setTooltip(hint("Rename CC Macro. Enter saves; Escape cancels."));
        ccSelect[slot].setTooltip(hint("Choose the MIDI controller (CC20 to CC31) sent by this Macro."));
        laneTypeSelect[slot].setTooltip(hint("Change this lane's MIDI function. Existing step values are preserved."));
        deleteCC[slot].setTooltip(hint("Remove this optional lane."));
    }
    for (int row = 0; row < lanes; ++row)
    {
        divisions[row].setTooltip(hint("Clock division for this lane. Double-click to reset."));
        timings[row].setTooltip(hint("Choose straight, triplet or dotted timing for this lane."));
        prev[row].setTooltip(hint("Previous page: show the preceding 16 steps."));
        next[row].setTooltip(hint("Next page: show the next 16 steps."));
        addPage[row].setTooltip(hint("Add page: append 16 steps to this lane, up to 128."));
        removePage[row].setTooltip(hint("Remove page: delete this page of steps (minimum 16)."));
    }
    keySelect.setTooltip(hint("Select the root key used for pitch quantisation."));
    scaleSelect.setTooltip(hint("Select the pitch scale for this sequence."));
    midiOutputSelect.setTooltip(hint("Choose Ableton host MIDI or a MIDI output device."));
    refreshMidiOutputs.setTooltip(hint("Refresh the available MIDI output devices."));
    themeSelect.setTooltip(hint("Choose PNORDE's colour theme."));
    bendRangeSelect.setTooltip(hint("Microtonal scales force +/-2 semitones; match the receiving synth. For standard scales, choose your preferred range."));
    globalClockSelect.setTooltip(hint("Global clock multiplier. Double-click to reset to 1x."));
    addCCButton.setTooltip(hint("Add a CC Macro or one of each dedicated MIDI lane."));
    settingsButton.setTooltip(hint("Open PNORDE settings."));
    savePresetButton.setTooltip(hint("Save every pattern and setting as a .pnorde preset."));
    loadPresetButton.setTooltip(hint("Load a .pnorde preset."));
    randomiseButton.setTooltip(hint("Randomise lanes or generate a harmonic microtonal PITCH pattern when an EDO mode is selected."));
    presetFolderButton.setTooltip(hint("Open or choose the preset library folder."));
}

juce::String PolyrhythmMIDIEditor::getTooltip()
{
    if (!processor.getTooltipsEnabled()) return {};
    const auto point = getMouseXYRelative().toFloat();
    const float h = static_cast<float>(getHeight());
    if (point.y < h * 0.110f || point.y >= h * 0.925f) return {};
    for (int row = 0; row < lanes; ++row)
    {
        if (!rowVisible(row)) continue;
        if (row >= 7 && reorderHandle(row).contains(point))
            return processor.getExperimentalReorderEnabled()
                ? "Drag the coloured rail to reorder this lane."
                : "Lane clock indicator. Enable Experimental Reorder in Settings to drag.";
        if (lengthBox(row).contains(point))
            return "Set Sequence Length: drag to change; double-click to reset to 16.";
        if (lane[row].page == (lane[row].length - 1) / visibleSteps && markerBox(row).contains(point))
            return "Drag the sequence end marker to set the number of steps.";
        for (int i = 0; i < visibleSteps; ++i)
        {
            const auto b = cell(row, i);
            const juce::Rectangle<float> numberHit(b.getX(), rowTop(row) + h * 0.027f,
                                                      b.getWidth(), h * 0.023f);
            if (numberHit.contains(point))
                return "Click or drag a step number to set the sequence length.";
            if (!b.contains(point) || lane[row].page * visibleSteps + i >= lane[row].length)
                continue;
            if (row == 0) return "Click to enable or disable this note gate.";
            if (row == 5 && processor.isMicrotonalScaleSelected())
                return "Pitch Approach: 0% instant, 100% takes the sounding gate duration (tempo-relative). Drag vertically.";
            if (row == 6) return "Drag vertically for Aftertouch; horizontally for Glide Time.";
            if (isLaneType(row, PnordeLane::Type::ccMacro))
                return "Drag vertically for CC value; horizontally for CC glide.";
            if (isLaneType(row, PnordeLane::Type::vibrato))
                return "Drag vertically for Vibrato rate; horizontally for depth.";
            if (isLaneType(row, PnordeLane::Type::sustain))
                return "Click to toggle the Sustain pedal.";
            if (isLaneType(row, PnordeLane::Type::pan))
                return "Pan: left/right changes CC10 (L / C / R); up/down changes glide time. Shift = fine.";
            if (isLaneType(row, PnordeLane::Type::midiChannel))
                return "Drag vertically to select MIDI channel; D uses the default.";
            return "Drag vertically to set this step's value. Shift gives finer control.";
        }
    }
    return {};
}

void PolyrhythmMIDIEditor::showSettingsMenu()
{
    juce::Component::SafePointer<PolyrhythmMIDIEditor> safe(this);
    auto panel = std::make_unique<PnordeSettingsPanel>(processor,
        [safe]
        {
            if (safe == nullptr) return;
            const int next = safe->processor.getThemeIndex();
            if (safe->themeIndex != next)
            {
                safe->themeIndex = next;
                safe->themeSelect.setSelectedId(next + 1, juce::dontSendNotification);
                safe->applyTheme();
            }
            safe->darknessSlider.setValue(safe->processor.getDarknessPercent(),
                                          juce::dontSendNotification);
            safe->applyTooltips();
            safe->repaint();
        });
    auto& box = juce::CallOutBox::launchAsynchronously(std::move(panel),
        settingsButton.getBounds(), this);
    settingsCallout = &box;
    box.setDismissalMouseClicksAreAlwaysConsumed(true);
}

void PolyrhythmMIDIEditor::syncFromProcessor()
{
    themeIndex = juce::jlimit(0, themeCount - 1, processor.getThemeIndex());
    ccPopupRow = -1; ccPopupAlpha = 0.0f;
    dragFromRow = dragTargetRow = -1; laneDragActive = false;
    themeSelect.setSelectedId(themeIndex + 1, juce::dontSendNotification);
    keySelect.setSelectedId(processor.getKey() + 1, juce::dontSendNotification);
    scaleSelect.setSelectedId(processor.getScale() + 1, juce::dontSendNotification);
    bendRangeSelect.setSelectedId(processor.getEffectiveBendRangeIndex() + 1, juce::dontSendNotification);
    bendRangeSelect.setEnabled(!processor.isMicrotonalScaleSelected());
    globalClockSelect.setSelectedId(processor.getGlobalClockIndex() + 1, juce::dontSendNotification);
    darknessSlider.setValue(processor.getDarknessPercent(), juce::dontSendNotification);
    manualTempoToggle.setToggleState(processor.getManualTempoEnabled(), juce::dontSendNotification);
    manualTempoInput.setEnabled(processor.getManualTempoEnabled());
    if (!manualTempoInput.hasKeyboardFocus(true))
    {
        const auto bpmText = juce::String(processor.getManualTempoBpm(), 1);
        if (manualTempoInput.getText() != bpmText)
            manualTempoInput.setText(bpmText, false);
    }
    octaveDial.setIndex(processor.getGlobalOctave()+2);
    keyDial.setIndex(processor.getKey());
    scaleDial.setIndex(processor.getScale());
    globalClockDial.setIndex(processor.getGlobalClockIndex());
    for (int slot = 0; slot < optionalLanes; ++slot)
    {
        laneTypeSelect[slot].setSelectedId(processor.getOptionalLaneType(slot + 7) + 1,
                                          juce::dontSendNotification);
        ccSelect[slot].setSelectedId(processor.getCustomCCNumber(slot) + 1,
                                     juce::dontSendNotification);
        committedCCLabels[slot] = processor.getCustomCCLabel(slot);
        ccLabelEditors[slot].setText(committedCCLabels[slot], false);
    }
    for (int row = 0; row < lanes; ++row)
    {
        auto& l = lane[row];
        l.length = processor.getLaneLength(row);
        l.division = processor.getLaneDivision(row);
        l.page = 0;
        l.displayX = -1.0f;
        l.markerVelocity = 0.0f;
        l.valueDrag = false;
        divisions[row].setSelectedId(l.division + 1, juce::dontSendNotification);
        timings[row].setSelectedId(processor.getLaneTiming(row) + 1,
                                   juce::dontSendNotification);
        clockDials[row].setValue(l.division);
        timingDials[row].setValue(processor.getLaneTiming(row));
        for (int i = 0; i < 128; ++i)
        {
            const int v = processor.getLaneValue(row, i);
            l.values[i] = v;
            l.visualFill[i] = isLaneType(row, PnordeLane::Type::midiChannel) ? v / 16.0f
                : isLaneType(row, PnordeLane::Type::pan) ? v / 127.0f
                : row == 1 ? (v + 120.0f) / 240.0f
                : row == 4 ? (v + 4.0f) / 8.0f : v / 100.0f;
        }
    }
    targetScrollY = scrollY = 0.0f;
    scrollContent.setTopLeftPosition(0, 0);
    applyTheme();
    applyTooltips();
    syncCCVisibility();
    resized();
    updateButtons();
    repaint();
}

juce::File PolyrhythmMIDIEditor::defaultPresetDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("Altered Wave Studios")
        .getChildFile("PNORDE")
        .getChildFile("Presets");
}

void PolyrhythmMIDIEditor::initialisePresetDirectory()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "PNORDE";
    options.filenameSuffix = "settings";
    options.folderName = "Altered Wave Studios/PNORDE";
    options.storageFormat = juce::PropertiesFile::StorageFormat::storeAsXML;
    presetPreferences = std::make_unique<juce::PropertiesFile>(options);
    const auto saved = presetPreferences->getValue("presetDirectory");
    presetDirectory = defaultPresetDirectory();
    if (saved.isNotEmpty() && juce::File::isAbsolutePath(saved))
    {
        const juce::File previous(saved);
        if (previous.isDirectory()) presetDirectory = previous;
    }
    // Only create folders when the user invokes a file/folder action.
}

void PolyrhythmMIDIEditor::showPresetFolderMenu()
{
    juce::PopupMenu menu;
    menu.addItem(1, "Open presets folder");
    menu.addItem(2, "Choose presets folder...");
    menu.addItem(3, "Use default Documents/Altered Wave Studios/PNORDE/Presets");
    juce::Component::SafePointer<PolyrhythmMIDIEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetFolderButton),
        [safe](int command)
        {
            if (safe == nullptr || command <= 0) return;
            if (command == 2) { safe->choosePresetDirectory(); return; }
            if (command == 3)
            {
                safe->presetDirectory = safe->defaultPresetDirectory();
                safe->presetPreferences->setValue("presetDirectory",
                    safe->presetDirectory.getFullPathName());
                safe->presetPreferences->saveIfNeeded();
            }
            if (!safe->presetDirectory.isDirectory())
                safe->presetDirectory.createDirectory();
            if (command == 1 && safe->presetDirectory.isDirectory())
                safe->presetDirectory.revealToUser();
        });
}

void PolyrhythmMIDIEditor::choosePresetDirectory()
{
    activeFileChooser = std::make_unique<juce::FileChooser>(
        "Choose where PNORDE presets are stored", presetDirectory, "*");
    juce::Component::SafePointer<PolyrhythmMIDIEditor> safe(this);
    activeFileChooser->launchAsync(juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectDirectories,
        [safe](const juce::FileChooser& chooser)
        {
            if (safe == nullptr) return;
            const auto folder = chooser.getResult();
            if (!folder.isDirectory()) return;
            safe->presetDirectory = folder;
            safe->presetPreferences->setValue("presetDirectory", folder.getFullPathName());
            safe->presetPreferences->saveIfNeeded();
        });
}

void PolyrhythmMIDIEditor::savePreset()
{
    if (!presetDirectory.isDirectory() && !presetDirectory.createDirectory().wasOk())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "PNORDE preset", "Couldn't create the presets directory.");
        return;
    }
    activeFileChooser = std::make_unique<juce::FileChooser>(
        "Save PNORDE preset", presetDirectory.getChildFile("New Preset.pnorde"),
        "*.pnorde");
    juce::Component::SafePointer<PolyrhythmMIDIEditor> safe(this);
    activeFileChooser->launchAsync(juce::FileBrowserComponent::saveMode |
        juce::FileBrowserComponent::canSelectFiles |
        juce::FileBrowserComponent::warnAboutOverwriting,
        [safe](const juce::FileChooser& chooser)
        {
            if (safe == nullptr) return;
            auto file = chooser.getResult();
            if (file == juce::File()) return;
            if (!file.hasFileExtension(".pnorde"))
                file = file.withFileExtension(".pnorde");
            juce::MemoryBlock state, encoded;
            safe->processor.getStateInformation(state);
            if (!PnordePresetFormat::encode(state, encoded) ||
                !PnordePresetFormat::writeAtomically(file, encoded))
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon,
                    "PNORDE preset", "Could not safely save the preset file.");
        });
}

void PolyrhythmMIDIEditor::loadPreset()
{
    if (!presetDirectory.isDirectory() && !presetDirectory.createDirectory().wasOk())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "PNORDE preset", "Couldn't open the presets directory.");
        return;
    }
    activeFileChooser = std::make_unique<juce::FileChooser>(
        "Load PNORDE preset", presetDirectory, "*.pnorde");
    juce::Component::SafePointer<PolyrhythmMIDIEditor> safe(this);
    activeFileChooser->launchAsync(juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectFiles,
        [safe](const juce::FileChooser& chooser)
        {
            if (safe == nullptr) return;
            const auto file = chooser.getResult();
            if (file == juce::File()) return;
            juce::MemoryBlock stored, state;
            if (!PnordePresetFormat::readBounded(file, stored) ||
                !PnordePresetFormat::decode(stored, state) ||
                !safe->processor.loadValidatedState(state.getData(),
                    static_cast<int>(state.getSize())))
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon,
                    "PNORDE preset", "The preset is corrupt, unsupported or too large. Nothing was loaded.");
                return;
            }
            safe->processor.requestMidiPanic();
            safe->syncFromProcessor();
        });
}

void PolyrhythmMIDIEditor::randomisePattern()
{
    juce::PopupMenu menu;
    menu.addItem(1, "Randomise all active lanes");
    menu.addItem(2, "Randomise SEQ gate pattern only");
    menu.addSeparator();
    // Four researched mood-oriented patterns use curated EDO scale degrees.
    // They only replace the PITCH lane; existing gates and other lanes survive.
    juce::PopupMenu edoGenerator;
    for (int i=0; i<PnordeEDO::moodCount; ++i)
        edoGenerator.addItem(300+i,PnordeEDO::moodNames[i]);
    menu.addSubMenu("Generate melody - selected EDO (PITCH)", edoGenerator,
                    PnordeEDO::isMode(processor.getScale()));
    // Pick any EDO AND a mood in one menu action. This is a deliberate
    // scale change, labelled explicitly to avoid surprising users.
    juce::PopupMenu modes;
    for (int edo=0; edo<PnordeEDO::modeCount; ++edo)
    {
        juce::PopupMenu moods;
        for (int mood=0; mood<PnordeEDO::moodCount; ++mood)
            moods.addItem(1000+edo*16+mood, PnordeEDO::moodNames[mood]);
        modes.addSubMenu(PnordeEDO::modes[static_cast<size_t>(edo)].name, moods);
    }
    menu.addSubMenu("Choose EDO + generate PITCH", modes);
    juce::PopupMenu gateRecipes, velocityRecipes;
    for (int i=0; i<PnordePatternLibrary::gateCount; ++i)
        gateRecipes.addItem(500+i, PnordePatternLibrary::gateNames[i]);
    for (int i=0; i<PnordePatternLibrary::velocityCount; ++i)
        velocityRecipes.addItem(600+i, PnordePatternLibrary::velocityNames[i]);
    menu.addSubMenu("Generate GATE rhythm", gateRecipes);
    menu.addSubMenu("Generate VELOCITY groove", velocityRecipes);
    menu.addSeparator();
    for (int row = 0; row < lanes; ++row)
    {
        if (!rowVisible(row)) continue;
        const juce::String title = laneDisplayName(row);
        menu.addItem(100 + row, "Randomise " + title);
    }
    juce::Component::SafePointer<PolyrhythmMIDIEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&randomiseButton),
        [safe](int id)
        {
            if (safe == nullptr || id == 0) return;
            auto& engine = safe->processor;
            auto& rng = juce::Random::getSystemRandom(); // UI thread, never audio.
            const auto randomLane = [&](int row)
            {
                const int count = juce::jlimit(1, 128, engine.getLaneLength(row));
                for (int step = 0; step < count; ++step)
                {
                    if (row == 0)
                    {
                        engine.setStepEnabled(step, rng.nextInt(100) < 65);
                        continue;
                    }
                    int value = 0;
                    if (row == 1)
                    {
                        value = (rng.nextInt(25) - 12) * 10;
                        value = engine.quantizePitchTenths(value, 0);
                    }
                    else if (row == 2) value = 45 + rng.nextInt(56);
                    else if (row == 3) value = 20 + rng.nextInt(81);
                    else if (row == 4) value = rng.nextInt(5) - 2;
                    else if (row == 5) value = rng.nextInt(101);
                    else if (row == 6) value = rng.nextInt(91);
                    else if (safe->isLaneType(row, PnordeLane::Type::pan)) value = 24 + rng.nextInt(81);
                    else if (safe->isLaneType(row, PnordeLane::Type::sustain)) value = rng.nextBool() ? 100 : 0;
                    else if (safe->isLaneType(row, PnordeLane::Type::midiChannel)) value = rng.nextInt(17);
                    else value = rng.nextInt(101);
                    engine.setLaneValue(row, step, value);
                    if (safe->isLaneType(row, PnordeLane::Type::vibrato))
                        engine.setVibratoDepth(step, rng.nextInt(101));
                }
            };
            if (id >= 500 && id < 500+PnordePatternLibrary::gateCount)
            {
                std::array<bool,128> pattern{};
                const int count=juce::jlimit(1,128,engine.getLaneLength(0));
                PnordePatternLibrary::gates(pattern,count,id-500,
                    static_cast<std::uint32_t>(rng.nextInt()));
                for (int step=0; step<count; ++step) engine.setStepEnabled(step,pattern[static_cast<size_t>(step)]);
                safe->syncFromProcessor();
                return;
            }
            if (id >= 600 && id < 600+PnordePatternLibrary::velocityCount)
            {
                std::array<int,128> values{};
                const int count=juce::jlimit(1,128,engine.getLaneLength(2));
                PnordePatternLibrary::velocities(values,count,id-600,
                    static_cast<std::uint32_t>(rng.nextInt()));
                for (int step=0; step<count; ++step) engine.setLaneValue(2,step,values[static_cast<size_t>(step)]);
                safe->syncFromProcessor();
                return;
            }
            if (id >= 1000 && id < 1000+PnordeEDO::modeCount*16)
            {
                const int slot=(id-1000)/16;
                const int mood=(id-1000)%16;
                if (mood>=PnordeEDO::moodCount) return;
                engine.setScale(PnordeEDO::firstScaleIndex+slot);
                std::array<int,128> pitches{};
                const int count=juce::jlimit(1,128,engine.getLaneLength(1));
                PnordeEDO::generate(pitches,count,engine.getKey(),engine.getScale(),
                    static_cast<PnordeEDO::Mood>(mood), static_cast<std::uint32_t>(rng.nextInt()));
                for (int step=0; step<count; ++step) engine.setLaneValue(1,step,pitches[static_cast<size_t>(step)]);
                safe->syncFromProcessor();
                return;
            }
            if (id >= 300 && id < 300+PnordeEDO::moodCount)
            {
                if (!PnordeEDO::isMode(engine.getScale())) return;
                std::array<int,128> pitches{};
                const int count = juce::jlimit(1,128,engine.getLaneLength(1));
                PnordeEDO::generate(pitches,count,engine.getKey(),engine.getScale(),
                    static_cast<PnordeEDO::Mood>(id-300),
                    static_cast<std::uint32_t>(rng.nextInt()));
                for (int step=0; step<count; ++step)
                    engine.setLaneValue(1,step,pitches[static_cast<size_t>(step)]);
                safe->syncFromProcessor();
                return;
            }
            if (id == 1)
            {
                for (int row = 0; row < lanes; ++row)
                    if (safe->rowVisible(row)) randomLane(row);
            }
            else if (id == 2) randomLane(0);
            else if (id >= 100 && id < 100 + lanes && safe->rowVisible(id - 100))
                randomLane(id - 100);
            safe->syncFromProcessor();
        });
}

void PolyrhythmMIDIEditor::mouseWheelMove(const juce::MouseEvent& e,
                                         const juce::MouseWheelDetails& wheel)
{
    if (e.position.y >= getHeight() * 0.11f && e.position.y < getHeight() * 0.925f)
    {
        targetScrollY = juce::jlimit(0.0f, maxScroll(),
                                     targetScrollY - wheel.deltaY * getHeight() * 0.34f);
        scrollbarOpacity = 1.0f;
        return;
    }
    juce::AudioProcessorEditor::mouseWheelMove(e, wheel);
}

PolyrhythmMIDIEditor::~PolyrhythmMIDIEditor()
{
    themeSelect.setLookAndFeel(nullptr);
}

void PolyrhythmMIDIEditor::applyTheme()
{
    const auto& t = themes[themeIndex];
    const bool dark = t.paper.getPerceivedBrightness() < 0.50f;
    const auto tooltipBackground = dark ? juce::Colour(0xff24334A) : juce::Colour(0xfffff4db);
    const auto tooltipText = dark ? juce::Colour(0xfff7f1ea) : juce::Colour(0xff20283A);
    tooltipWindow.setColour(juce::TooltipWindow::backgroundColourId, tooltipBackground);
    tooltipWindow.setColour(juce::TooltipWindow::textColourId, tooltipText);
    tooltipWindow.setColour(juce::TooltipWindow::outlineColourId, t.teal);
    // Ensures the standard tooltip is drawn as a fully opaque foreground window.
    tooltipWindow.setOpaque(true);
    // Brighter but steady controls. Unlike the lane rail, these never pulse.
    // Backlit controls are brighter than v0.5.5 but remain STATIC.
    // A moderate blend raises dark-theme background luminance without
    // sacrificing legibility of the light foreground text.
    const auto controlBg = dark ? t.cream.interpolatedWith(t.ink, 0.33f) : t.cream;
    const auto controlInk = t.ink;
    for (auto* box : std::array<juce::ComboBox*, 4>{ { &keySelect, &scaleSelect, &globalClockSelect, &bendRangeSelect } })
    {
        box->setColour(juce::ComboBox::backgroundColourId, controlBg);
        box->setColour(juce::ComboBox::textColourId, controlInk);
        box->setColour(juce::ComboBox::arrowColourId, controlInk);
    }
    for (auto* dial : { &octaveDial, &keyDial, &scaleDial, &globalClockDial })
        dial->setPalette(t.paper, t.ink, t.teal);
    const juce::Colour laneColours[] {t.coral,t.blue,t.sage,t.lavender,t.amber,t.teal,t.pink,
        t.teal,t.amber,t.blue,t.coral,t.lavender,t.sage,t.pink,t.amber,t.teal,t.blue,t.lavender,t.pink,t.sage,t.amber};
    for (int row = 0; row < lanes; ++row)
    {
        clockDials[row].setPalette(t.paper, controlInk, laneColours[row]);
        timingDials[row].setPalette(t.paper, controlInk, laneColours[row]);
        for (auto* box : std::array<juce::ComboBox*, 2>{ { &divisions[row], &timings[row] } })
        {
            box->setColour(juce::ComboBox::backgroundColourId, controlBg);
            box->setColour(juce::ComboBox::textColourId, controlInk);
            box->setColour(juce::ComboBox::arrowColourId, controlInk);
        }
        divisions[row].setColour(juce::ComboBox::outlineColourId, t.ink.withAlpha(dark ? 0.60f : 0.33f));
        timings[row].setColour(juce::ComboBox::outlineColourId, t.ink.withAlpha(dark ? 0.60f : 0.33f));
        for (auto* button : {&prev[row], &next[row], &addPage[row], &removePage[row]})
        {
            button->setColour(juce::TextButton::buttonColourId,
                dark ? t.cream.interpolatedWith(t.ink, 0.16f) : t.paper);
            button->setColour(juce::TextButton::buttonOnColourId, laneColours[row]);
            button->setColour(juce::TextButton::textColourOffId,
                dark ? laneColours[row].brighter(0.85f) : laneColours[row].darker(0.20f));
        }
        removePage[row].setWarning(true);
    }
    for (int slot = 0; slot < optionalLanes; ++slot)
    {
        ccSelect[slot].setColour(juce::ComboBox::backgroundColourId, controlBg);
        ccSelect[slot].setColour(juce::ComboBox::textColourId, t.ink);
        ccSelect[slot].setColour(juce::ComboBox::arrowColourId, t.ink);
        laneTypeSelect[slot].setColour(juce::ComboBox::backgroundColourId, controlBg);
        laneTypeSelect[slot].setColour(juce::ComboBox::textColourId, t.ink);
        laneTypeSelect[slot].setColour(juce::ComboBox::arrowColourId, t.ink);
        for (auto* cb : { &ccSelect[slot], &laneTypeSelect[slot] })
            cb->setColour(juce::ComboBox::outlineColourId,
                          t.ink.withAlpha(dark ? 0.60f : 0.33f));
        deleteCC[slot].setColour(juce::TextButton::buttonColourId, controlBg);
        deleteCC[slot].setColour(juce::TextButton::textColourOffId, dark ? t.ink : t.coral);
        auto& editor = ccLabelEditors[slot];
        editor.setColour(juce::TextEditor::backgroundColourId, t.paper);
        editor.setColour(juce::TextEditor::textColourId, laneColours[slot+7].brighter(dark ? 0.72f : 0.12f));
        editor.setColour(juce::TextEditor::highlightedTextColourId,
                         laneColours[slot+7].brighter(dark ? 0.72f : 0.12f));
        editor.setColour(juce::TextEditor::focusedOutlineColourId, t.teal);
        editor.setColour(juce::TextEditor::highlightColourId, t.teal.withAlpha(0.28f));
    }
    addCCButton.setColour(juce::TextButton::buttonColourId, t.teal);
    addCCButton.setColour(juce::TextButton::textColourOffId, t.ink);
    for (auto* b : std::array<juce::TextButton*, 5>{ {
        &savePresetButton, &loadPresetButton, &presetFolderButton,
        &randomiseButton, &settingsButton } })
    {
        b->setColour(juce::TextButton::buttonColourId, t.paper);
        b->setColour(juce::TextButton::textColourOffId, t.ink);
    }
    manualTempoToggle.setColour(juce::ToggleButton::textColourId, t.teal);
    manualTempoToggle.setColour(juce::ToggleButton::tickColourId, t.teal);
    manualTempoInput.setColour(juce::TextEditor::backgroundColourId,
                               dark ? t.cream.interpolatedWith(t.ink, 0.25f) : t.cream);
    manualTempoInput.setColour(juce::TextEditor::textColourId, t.ink);
    manualTempoInput.setColour(juce::TextEditor::outlineColourId, t.teal.withAlpha(0.45f));
    midiOutputSelect.setColour(juce::ComboBox::backgroundColourId, t.paper);
    midiOutputSelect.setColour(juce::ComboBox::textColourId, t.ink);
    midiOutputSelect.setColour(juce::ComboBox::arrowColourId, t.ink);
    midiOutputSelect.setColour(juce::ComboBox::outlineColourId, t.teal);
    refreshMidiOutputs.setColour(juce::TextButton::buttonColourId, t.teal);
    refreshMidiOutputs.setColour(juce::TextButton::textColourOffId, t.ink);
    themeSelect.setColour(juce::ComboBox::backgroundColourId, t.paper);
    themeSelect.setColour(juce::ComboBox::textColourId, t.ink);
    themeSelect.setColour(juce::ComboBox::arrowColourId, t.ink);
    themeSelect.setColour(juce::ComboBox::outlineColourId, t.teal);
    settingsButton.setColour(juce::TextButton::buttonColourId, t.paper);
    settingsButton.setColour(juce::TextButton::textColourOffId, t.ink);
    refreshMacroPlaceholders();
    themeSelect.repaint();
    repaint();
}

void PnordeThemeLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height,
                                          bool, int, int, int, int, juce::ComboBox& box)
{
    const auto& t = themes[juce::jlimit(0, themeCount - 1, box.getSelectedId() - 1)];
    g.setColour(t.paper);
    g.fillRoundedRectangle(0.0f, 0.0f, (float) width, (float) height, 5.0f);
    g.setColour(t.teal);
    g.drawRoundedRectangle(0.5f, 0.5f, width - 1.0f, height - 1.0f, 5.0f, 1.0f);
    const float r = height * 0.27f;
    juce::ColourGradient grad(t.paper, 10.0f - r, height * 0.5f - r,
                              t.cream, 10.0f + r, height * 0.5f + r, false);
    grad.addColour(0.5, t.middle);
    g.setGradientFill(grad);
    g.fillEllipse(10.0f - r, height * 0.5f - r, r * 2, r * 2);
    g.setColour(t.ink.withAlpha(0.5f));
    g.drawEllipse(10.0f - r, height * 0.5f - r, r * 2, r * 2, 0.8f);
    juce::Path arrow;
    arrow.addTriangle(width - 14.0f, height * 0.43f, width - 6.0f, height * 0.43f,
                      width - 10.0f, height * 0.62f);
    g.setColour(t.ink); g.fillPath(arrow);
}

void PnordeThemeLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(24, 1, box.getWidth() - 41, box.getHeight() - 2);
    label.setFont(font(juce::jmax(12.0f, box.getHeight() * 0.44f)));
}

void PnordeThemeLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
        bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
        const juce::String& text, const juce::String& shortcutKeyText,
        const juce::Drawable* icon, const juce::Colour* textColour)
{
    int index = -1;
    for (int i = 0; i < themeCount; ++i)
        if (text == themes[i].name) { index = i; break; }
    if (index < 0)
    {
        juce::LookAndFeel_V4::drawPopupMenuItem(g, area, isSeparator, isActive,
            isHighlighted, isTicked, hasSubMenu, text, shortcutKeyText, icon, textColour);
        return;
    }
    const auto& t = themes[index];
    g.setColour(isHighlighted ? juce::Colour(0xff424454) : juce::Colour(0xff242631));
    g.fillRect(area);
    const float d = juce::jmin(22.0f, area.getHeight() - 7.0f);
    const float x = area.getX() + 11.0f, y = area.getCentreY() - d * 0.5f;
    juce::ColourGradient grad(t.paper, x, y, t.cream, x + d, y + d, false);
    grad.addColour(0.5, t.middle);
    g.setGradientFill(grad); g.fillEllipse(x, y, d, d);
    g.setColour(juce::Colours::white.withAlpha(0.45f)); g.drawEllipse(x, y, d, d, 0.8f);
    g.setColour(juce::Colours::white);
    g.setFont(font(15.0f));
    g.drawText(text, area.withTrimmedLeft(42).withTrimmedRight(12), juce::Justification::centredLeft);
    if (isTicked) { g.setColour(t.teal); g.fillEllipse(area.getRight() - 13.0f, area.getCentreY() - 3.0f, 6.0f, 6.0f); }
}
