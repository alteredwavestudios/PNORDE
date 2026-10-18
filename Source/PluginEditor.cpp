#include "PluginEditor.h"
#include "AudiowideEmbedded.h"
#include "PnordeLights.h"
#include "PnordePresetFormat.h"
#include <cmath>

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
                                             float proportion)
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
    if (row == PolyrhythmMIDIProcessor::panRow)
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
        setSize(408, 550);
        for (int i = 0; i < themeCount; ++i) theme.addItem(themes[i].name, i + 1);
        for (int i = 1; i <= 16; ++i)
            channel.addItem("Channel " + juce::String(i), i);
        const char* ranges[] { "±2 semitones", "±12 semitones",
                                "±24 semitones", "±48 semitones" };
        for (int i = 0; i < 4; ++i) bend.addItem(ranges[i], i + 1);
        theme.setSelectedId(processor.getThemeIndex() + 1, juce::dontSendNotification);
        channel.setSelectedId(processor.getDefaultMidiChannel(), juce::dontSendNotification);
        bend.setSelectedId(processor.getBendRangeIndex() + 1, juce::dontSendNotification);
        theme.onChange = [this]
        {
            processor.setThemeIndex(theme.getSelectedId() - 1);
            repaint(); if (onChanged) onChanged();
        };
        channel.onChange = [this]
        { processor.setDefaultMidiChannel(channel.getSelectedId()); };
        bend.onChange = [this]
        { processor.setBendRangeIndex(bend.getSelectedId() - 1); };
        for (auto* combo : { &theme, &channel, &bend }) addAndMakeVisible(*combo);

        animations.setButtonText("LED animations and clock flashes");
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

        darkness.setSliderStyle(juce::Slider::LinearHorizontal);
        darkness.setTextBoxStyle(juce::Slider::TextBoxRight, false, 42, 22);
        darkness.setRange(0.0, 75.0, 1.0);
        darkness.setTextValueSuffix("%");
        darkness.setValue(processor.getDarknessPercent(), juce::dontSendNotification);
        darkness.onValueChange = [this]
        { processor.setDarknessPercent(juce::roundToInt(darkness.getValue()));
          if (onChanged) onChanged(); };
        addAndMakeVisible(darkness);

        panic.setButtonText("MIDI PANIC   •   All Notes Off");
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
        g.drawText("DARKNESS  /  lights remain illuminated", 20, 249, 350, 18,
                   juce::Justification::centredLeft);
        g.setColour(p.ink.withAlpha(0.18f));
        g.drawHorizontalLine(320, 18.0f, static_cast<float>(getWidth() - 18));
        g.drawHorizontalLine(476, 18.0f, static_cast<float>(getWidth() - 18));
        g.setColour(p.ink.withAlpha(0.70f));
        g.drawText("PNORDE – Dream Sequencer   v0.5.2", 18, 493,
                   getWidth() - 36, 17, juce::Justification::centredLeft);
        g.drawText("Altered Wave Studios", 18, 514,
                   getWidth() - 36, 17, juce::Justification::centredLeft);
    }
    void resized() override
    {
        const int w = getWidth() - 40;
        theme.setBounds(20, 95, w, 28);
        channel.setBounds(20, 153, w, 28);
        bend.setBounds(20, 211, w, 28);
        darkness.setBounds(20, 271, w, 32);
        animations.setBounds(20, 332, w, 26);
        fades.setBounds(20, 361, w, 26);
        tooltips.setBounds(20, 390, w, 26);
        panic.setBounds(20, 434, w, 27);
    }
private:
    PolyrhythmMIDIProcessor& processor;
    std::function<void()> onChanged;
    juce::ComboBox theme, channel, bend;
    juce::ToggleButton animations, fades, tooltips;
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
    const char* scales[] {"Chromatic", "Major", "Minor", "Dorian", "Phrygian", "Locrian", "Blues", "Major Blues", "Bebop", "Harmonic Minor", "Pelog", "Melodic Minor", "Mixolydian", "Free (0.1)"};
    for (int i = 0; i < 12; ++i) keySelect.addItem(keys[i], i + 1);
    for (int i = 0; i < 14; ++i) scaleSelect.addItem(scales[i], i + 1);
    for (auto* box : { &keySelect, &scaleSelect })
    {
        box->setColour(juce::ComboBox::backgroundColourId, paper);
        box->setColour(juce::ComboBox::textColourId, ink);
        addAndMakeVisible(*box);
    }
    keySelect.setSelectedId(processor.getKey() + 1, juce::dontSendNotification);
    scaleSelect.setSelectedId(processor.getScale() + 1, juce::dontSendNotification);
    keySelect.onChange = [this] { processor.setKey(keySelect.getSelectedId() - 1); repaint(); };
    scaleSelect.onChange = [this]
    {
        processor.setScale(scaleSelect.getSelectedId() - 1);
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
        addAndMakeVisible(*box);
    }
    globalClockSelect.setSelectedId(processor.getGlobalClockIndex() + 1, juce::dontSendNotification);
    bendRangeSelect.setSelectedId(processor.getBendRangeIndex() + 1, juce::dontSendNotification);
    globalClockSelect.onChange = [this] { processor.setGlobalClockIndex(globalClockSelect.getSelectedId() - 1); };
    globalClockSelect.onReset = [this]
    {
        globalClockSelect.setSelectedId(3, juce::sendNotificationSync); // 1x
    };
    bendRangeSelect.onChange = [this] { processor.setBendRangeIndex(bendRangeSelect.getSelectedId() - 1); };
    // Only modulation-oriented, non-channel-mode CCs are offered here.
    for (int slot = 0; slot < 8; ++slot)
    {
        auto& select = ccSelect[slot];
        for (int cc = 2; cc <= 119; ++cc)
            if (cc != 6 && cc != 10 && cc != 11 && cc != 32 && cc != 38 &&
                cc != 64 && cc != 76 && cc != 77 && cc != 98 && cc != 99 &&
                cc != 100 && cc != 101)
                select.addItem("CC " + juce::String(cc), cc + 1);
        select.setSelectedId(processor.getCustomCCNumber(slot) + 1, juce::dontSendNotification);
        select.onChange = [this, slot] { processor.setCustomCCNumber(slot, ccSelect[slot].getSelectedId() - 1); };
        select.setColour(juce::ComboBox::backgroundColourId, paper);
        select.setColour(juce::ComboBox::textColourId, ink);
        scrollContent.addAndMakeVisible(select);
        auto& labelEditor = ccLabelEditors[slot];
        labelEditor.setTextToShowWhenEmpty("MACRO " + juce::String(slot + 1), ink.withAlpha(0.88f));
        labelEditor.setText(processor.getCustomCCLabel(slot), false);
        committedCCLabels[slot] = processor.getCustomCCLabel(slot);
        labelEditor.setInputRestrictions(12);
        labelEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        labelEditor.setColour(juce::TextEditor::textColourId, juce::Colours::black);
        labelEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        labelEditor.setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::black);
        labelEditor.setColour(juce::TextEditor::focusedOutlineColourId, teal);
        labelEditor.setFont(font(getHeight() * 0.0185f, true));
        labelEditor.setJustification(juce::Justification::centredLeft);
        labelEditor.setSelectAllWhenFocused(true);
        labelEditor.setTooltip("Click to rename this custom CC lane");
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
        deleteCC[slot].onClick = [this, slot]
        {
            processor.removeCustomCC(slot);
            for (int i = 0; i < 8; ++i)
            {
                ccSelect[i].setSelectedId(processor.getCustomCCNumber(i) + 1, juce::dontSendNotification);
            }
            for (int i = 0; i < 8; ++i)
            {
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
        menu.addItem(1, "CC Macro (Macro " + juce::String(processor.getCustomCCCount() + 1) + ")",
            processor.getCustomCCCount() < 8);
        menu.addSeparator();
        const char* dedicated[] { "Mod Wheel (CC1)", "Expression Pedal (CC11)",
            "Vibrato - Rate CC76 + Depth CC77", "Pan (CC10)", "Sustain Pedal (CC64)" };
        for (int i = 0; i < 5; ++i)
            menu.addItem(i + 2, dedicated[i], !processor.isDedicatedEnabled(modRow + i));
        menu.addItem(7, "MIDI Channel Select", !processor.hasMidiChannelLane());
        auto safe = juce::Component::SafePointer<PolyrhythmMIDIEditor>(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&addCCButton),
            [safe](int id)
            {
                if (safe == nullptr || id <= 0) return;
                if (id == 1) safe->addMacroOfType(0);
                else if (id == 7) safe->processor.setMidiChannelLaneEnabled(true);
                else safe->processor.setDedicatedEnabled(modRow + id - 2, true);
                safe->syncCCVisibility();
                safe->targetScrollY = safe->maxScroll();
                safe->resized(); safe->repaint();
            });
    };
    deleteChannelButton.onClick = [this]
    {
        processor.setMidiChannelLaneEnabled(false);
        syncCCVisibility();
        targetScrollY = juce::jmin(targetScrollY, maxScroll());
        resized(); repaint();
    };
    for (int i = 0; i < 5; ++i)
    {
        deleteDedicated[i].setButtonText("X");
        deleteDedicated[i].onClick = [this, i]
        {
            processor.setDedicatedEnabled(modRow + i, false);
            targetScrollY = juce::jmin(targetScrollY, maxScroll());
            syncCCVisibility(); resized(); repaint();
        };
        scrollContent.addAndMakeVisible(deleteDedicated[i]);
    }
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
    addAndMakeVisible(darknessSlider);
    initialisePresetDirectory();
    presetFolderButton.onClick = [this] { showPresetFolderMenu(); };
    presetFolderButton.setTooltip("Open, choose, or reset the PNORDE preset folder");
    addAndMakeVisible(presetFolderButton);
    savePresetButton.onClick = [this] { savePreset(); };
    loadPresetButton.onClick = [this] { loadPreset(); };
    randomiseButton.onClick = [this] { randomisePattern(); };
    for (auto* control : { &savePresetButton, &loadPresetButton, &randomiseButton })
        addAndMakeVisible(*control);

    scrollContent.addAndMakeVisible(deleteChannelButton);
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
        scrollContent.addAndMakeVisible(timing);
        for (int i = 0; i < 128; ++i)
            l.values[static_cast<size_t>(i)] = processor.getLaneValue(row, i);
        for (int i = 0; i < 128; ++i) l.visualFill[i] = row == channelRow ? l.values[i] / 16.0f : row == panRow ? l.values[i] / 127.0f : row == 1 || row == 4 ? 0.5f : row == 5 || row == 6 ? 0.0f : l.values[i] / 100.0f;
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
        scrollContent.addAndMakeVisible(box);
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
    refreshMidiOutputs.setButtonText("↻");
    refreshMidiOutputs.onClick = [this] { refreshMidiOutputMenu(); };
    addAndMakeVisible(refreshMidiOutputs);
    refreshMidiOutputMenu();
    addAndMakeVisible(themeSelect);
    applyTheme();
    applyTooltips();
    syncCCVisibility();
    updateButtons();
    startTimerHz(45); // Scoped redraws preserve smooth LED motion; MIDI stays sample-accurate.
}

bool PolyrhythmMIDIEditor::rowVisible(int row) const noexcept
{
    if (row < 0 || row >= lanes) return false;
    if (row < 7) return true;
    if (row < modRow) return row - 7 < processor.getCustomCCCount();
    if (row < channelRow) return processor.isDedicatedEnabled(row);
    return processor.hasMidiChannelLane();
}
int PolyrhythmMIDIEditor::visibleIndex(int row) const noexcept
{
    int index = 0;
    for (int r = 0; r < row; ++r) if (rowVisible(r)) ++index;
    return index;
}
int PolyrhythmMIDIEditor::visibleRowCount() const noexcept
{
    int count = 0;
    for (int r = 0; r < lanes; ++r) if (rowVisible(r)) ++count;
    return count;
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
    midiOutputSelect.setBounds(juce::roundToInt(w * 0.239f), juce::roundToInt(h * 0.057f),
                               juce::roundToInt(w * 0.098f), juce::roundToInt(h * 0.034f));
    refreshMidiOutputs.setBounds(juce::roundToInt(w * 0.341f), juce::roundToInt(h * 0.057f),
                                 juce::roundToInt(w * 0.020f), juce::roundToInt(h * 0.034f));
    place(keySelect, 0.365f, 0.060f);
    place(scaleSelect, 0.433f, 0.140f);
    place(globalClockSelect, 0.581f, 0.080f);
    place(bendRangeSelect, 0.669f, 0.080f);
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
    themeSelect.setBounds(juce::roundToInt(w * 0.761f), juce::roundToInt(h * 0.057f),
                          juce::roundToInt(w * 0.117f), juce::roundToInt(h * 0.034f));
    for (int row = 0; row < lanes; ++row)
    {
        // Native controls are positioned in the unscrolled content coordinate system.
        const float rowY = rowTop(row) + scrollY - clipTop;
        const int y = juce::roundToInt(rowY + h * 0.004f);
        divisions[static_cast<size_t>(row)].setBounds(
            juce::roundToInt(w * 0.847f), y,
            juce::roundToInt(w * 0.074f), juce::roundToInt(h * 0.030f));
        timings[static_cast<size_t>(row)].setBounds(
            juce::roundToInt(w * 0.847f), juce::roundToInt(rowY + h * 0.039f),
            juce::roundToInt(w * 0.074f), juce::roundToInt(h * 0.029f));
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
        if (row >= 7 && row < modRow)
        {
            ccSelect[row - 7].setBounds(juce::roundToInt(w * 0.240f),
                juce::roundToInt(rowY + h * 0.003f), juce::roundToInt(w * 0.066f), juce::roundToInt(h * 0.027f));
            ccLabelEditors[row - 7].setFont(font(h * 0.0185f, true));
            ccLabelEditors[row - 7].setBounds(juce::roundToInt(w * 0.068f),
                juce::roundToInt(rowY + h * 0.003f), juce::roundToInt(w * 0.159f), juce::roundToInt(h * 0.029f));
            deleteCC[row - 7].setBounds(juce::roundToInt(w * 0.315f),
                juce::roundToInt(rowY + h * 0.003f), juce::roundToInt(w * 0.023f), juce::roundToInt(h * 0.027f));
        }
    }
    deleteChannelButton.setBounds(juce::roundToInt(w * 0.369f),
        juce::roundToInt(rowTop(channelRow) + scrollY - clipTop + h * 0.003f),
        juce::roundToInt(w * 0.023f), juce::roundToInt(h * 0.027f));
    for (int i = 0; i < 5; ++i)
        deleteDedicated[i].setBounds(juce::roundToInt(w * 0.280f),
            juce::roundToInt(rowTop(modRow + i) + scrollY - clipTop + h * 0.003f),
            juce::roundToInt(w * 0.023f), juce::roundToInt(h * 0.027f));
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
    const bool playing = processor.isHostPlaying();
    const float shift = playing ? static_cast<float>(std::sin(huePhase * juce::MathConstants<double>::twoPi) * 0.008) : 0.0f;
    juce::ColourGradient bg(paper.withRotatedHue(shift),
                           0.0f, 0.0f, cream.withRotatedHue(shift),
                           static_cast<float>(w), static_cast<float>(h), false);
    bg.addColour(0.5, palette.middle.withRotatedHue(shift));
    g.setGradientFill(bg); g.fillAll();
    g.setColour(paper.withAlpha(0.95f)); g.fillRect(0, 0, w, juce::roundToInt(h * 0.105f));
    g.setColour(coral); g.fillRect(0, juce::roundToInt(h * 0.103f), w, 2);
    // Audiowide is embedded in this plugin: no system font installation required.
    const float titleFlash = processor.getAnimationsEnabled() && processor.isHostPlaying()
        ? lane[0].stepLed : 0.0f;
    g.setColour(coral.interpolatedWith(juce::Colours::white, titleFlash * 0.7f));
    if (titleFlash > 0.02f)
    {
        g.setColour(coral.withAlpha(titleFlash * 0.24f));
        g.fillRoundedRectangle(w * 0.046f, h * 0.015f, 11.0f, h * 0.073f, 4.0f);
        g.setColour(coral.interpolatedWith(juce::Colours::white, titleFlash * 0.7f));
    }
    g.fillRoundedRectangle(w * 0.049f, h * 0.018f, 4.0f, h * 0.067f, 2.0f);
    // Header typography is rendered exactly once in paintOverChildren so it
    // stays luminous above DARKNESS without duplicating expensive glyph runs.
    g.setColour(teal); g.fillRect(w * 0.061f, h * 0.090f, w * 0.031f, 2.0f);
    g.setColour(coral); g.fillRect(w * 0.099f, h * 0.090f, w * 0.011f, 2.0f);
    g.setColour(lavender); g.fillRect(w * 0.116f, h * 0.090f, w * 0.006f, 2.0f);
    // Header labels, host indicator and dropdown surrounds share the light bus.
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
        g.setColour(colour.withAlpha(0.20f)); g.fillRoundedRectangle(length, 6.0f);
        g.setColour(colour); g.drawRoundedRectangle(length, 6.0f, 1.3f);
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
        const int seqStep = l.activeStep;
        for (int i = 0; i < visibleSteps; ++i)
        {
            const int index = pageStart + i;
            const bool inRange = index < l.length;
            const bool enabled = row == 0 && inRange && processor.getStepEnabled(index);
            const bool active = seqStep == index && inRange;
            const auto bounds = cell(row, i);
            const bool editing = l.valueDrag && l.draggedStep == index;
            const float wiggle = editing ? static_cast<float>(std::sin(pulse * 2.3) * 0.035) : 0.0f;
            const float scale = editing ? 1.055f + wiggle : 1.0f;
            const auto drawBounds = bounds.withSizeKeepingCentre(bounds.getWidth() * scale,
                                                                  bounds.getHeight() * scale);
            g.setColour(!inRange ? ink.withAlpha(0.08f) : paper.withAlpha(0.86f));
            g.fillRoundedRectangle(drawBounds, 5.0f);
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
                    else if (row == panRow)
                    {
                        g.setColour(ink.withAlpha(0.40f));
                        g.fillRect(inner.getCentreX(), inner.getY(), 1.0f, inner.getHeight());
                    }
                    else if (row == vibratoRow)
                    {
                        const float depth = processor.getVibratoDepth(index) / 100.0f;
                        g.setColour(colour.withAlpha(0.30f));
                        g.fillRoundedRectangle(inner.withWidth(inner.getWidth() * depth), 2.0f);
                    }
                    g.setColour(colour.withAlpha(0.84f));
                    const auto bar = parameterFillBounds(row, drawBounds, f);
                    if (!bar.isEmpty()) g.fillRoundedRectangle(bar, 3.0f);
                }
            }
            if (row >= 7 && row < modRow && inRange)
            {
                const float glide = processor.getCCTransitionMs(row - 7, index) / 100.0f;
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
            g.setColour(colour.withAlpha(0.7f));
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
        if (row == panRow)
        {
            g.setColour(ink.withAlpha(0.65f)); g.setFont(font(h * 0.009f, true));
            g.drawText("L    C    R", juce::roundToInt(w * 0.120f),
                juce::roundToInt(top + h * 0.075f), juce::roundToInt(w * 0.064f),
                juce::roundToInt(h * 0.014f), juce::Justification::centred);
        }
        g.setColour(ink.withAlpha(0.65f)); g.setFont(font(h * 0.012f));
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
    // The credit is only drawn in the emissive footer overlay.

}

void PolyrhythmMIDIEditor::paintOverChildren(juce::Graphics& g)
{
    const auto& palette = themes[themeIndex];
    const auto ink = palette.ink;
    const juce::Colour colours[] {palette.coral, palette.blue, palette.sage, palette.lavender,
        palette.amber, palette.teal, palette.pink, palette.teal, palette.amber, palette.blue, palette.coral, palette.lavender, palette.sage, palette.pink, palette.amber, palette.teal, palette.blue, palette.lavender, palette.pink, palette.sage, palette.amber};
    const int w = getWidth(), h = getHeight();
    // Draw a neutral dimmer ABOVE all JUCE child controls, preserving their
    // colours, then draw the musical clock lights above the dimmer.
    const float darkness = processor.getDarknessPercent() / 100.0f;
    if (darkness > 0.001f)
    {
        g.setColour(juce::Colours::black.withAlpha(darkness));
        g.fillRect(getLocalBounds());
    }
    // All light emitters share the same PnordeLights envelope and bloom model.
    // Clock-accurate lane rails stay in phase with the sound; secondary lights
    // use lane-specific offsets for a gentle polyrhythmic shimmer.
    // Emitters stay faintly illuminated when stopped; only the timed pulse
    // requires transport playback. This keeps gate-on LEDs visible in darkness.
    const bool playingLights = processor.getAnimationsEnabled() && processor.isHostPlaying();
    {
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
                const float trail = current < 0 ? 0.0f :
                    juce::jlimit(0.0f, 1.0f, static_cast<float>(
                        std::exp(-1.4f * (static_cast<float>(behind) + phase))
                        + 0.13f * std::exp(-1.8f * static_cast<float>(ahead))));
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
                else if (row != channelRow)
                {
                    const float f = juce::jlimit(0.0f, 1.0f, l.visualFill[index]);
                    const auto fill = parameterFillBounds(row, bounds, f);
                    PnordeLights::cellFill(g, fill, colour, darkness,
                        motion, current == index ? led : 0.0f);
                    if (row == vibratoRow)
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
                    if (row != 0 && row != channelRow)
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
            // Clock-division control gets a subtle lighting trace whose speed
            // follows the same lane (not the global SEQ lane).
            const auto control = divisions[row].getBounds();
            // Child coordinates belong to scrollContent; translate to editor.
            const float cy = top + h * 0.004f;
            PnordeLights::outline(g,
                {static_cast<float>(control.getX()), cy,
                 static_cast<float>(control.getWidth()), h * 0.030f},
                colour, 0.20f + 0.42f * led, darkness, 3.0f);

            // Independent lane-length LED: its attack is HALF A CYCLE away
            // from this lane's main rail pulse, while still respecting that
            // lane's division and swing/triplet timing.
            const float opposite = playingLights
                ? PnordeLights::offsetPulse(l.stepPhase, 0.5f, l.stepDurationMs)
                : 0.0f;
            const auto lengthRect = lengthBox(row);
            PnordeLights::outline(g, lengthRect.expanded(1.3f), colour,
                0.13f + opposite * 0.86f, darkness, 6.0f);

            // Restore text ABOVE the darkness layer, including light-emitting
            // lane names, step numbers and values. Static cells get a clean,
            // low-cost core; only the active cell gets the multi-pass halo.
            if (row < 7 || row >= modRow)
                PnordeLights::text(g, names[row],
                    {juce::roundToInt(w * 0.068f), juce::roundToInt(top),
                     juce::roundToInt(w * 0.125f), juce::roundToInt(h * 0.024f)},
                    font(h * (row >= modRow ? 0.014f : 0.0185f), true),
                    colour, 0.26f + 0.50f * led, darkness);
            else if (!ccLabelEditors[row - 7].hasKeyboardFocus(true))
                PnordeLights::text(g, ccLabelEditors[row - 7].getText(),
                    {juce::roundToInt(w * 0.068f), juce::roundToInt(top + h * 0.003f),
                     juce::roundToInt(w * 0.159f), juce::roundToInt(h * 0.029f)},
                    font(h * 0.0185f, true), colour,
                    0.24f + 0.55f * led, darkness);
            PnordeLights::text(g, "SEQ LENGTH",
                {juce::roundToInt(w * 0.073f), juce::roundToInt(top + h * 0.025f),
                 juce::roundToInt(w * 0.089f), juce::roundToInt(h * 0.017f)},
                font(h * 0.011f), colour, 0.19f + 0.35f * opposite,
                darkness, juce::Justification::centred, 0);
            PnordeLights::text(g, juce::String(l.length) + "  ↔",
                lengthRect.toNearestInt(), font(h * 0.020f, true), colour,
                0.35f + 0.65f * opposite, darkness,
                juce::Justification::centred, 1);

            for (int i = 0; i < visibleSteps; ++i)
            {
                const int index = l.page * visibleSteps + i;
                if (index >= l.length) continue;
                const auto b = cell(row, i);
                const bool selected = l.activeStep == index;
                const float brightness = selected ? juce::jlimit(0.0f, 1.0f,
                    0.45f + 0.50f * led)
                    : l.hovered == i ? 0.48f : 0.12f;
                PnordeLights::text(g, stepNumberLabel(index),
                    {juce::roundToInt(b.getX()), juce::roundToInt(top + h * 0.029f),
                     juce::roundToInt(b.getWidth()), juce::roundToInt(h * 0.020f)},
                    font(h * 0.014f, true), colour, brightness,
                    darkness, juce::Justification::centred, selected ? 1 : 0);

                if (row == 6) continue;
                const int v = l.values[static_cast<size_t>(index)];
                const juce::String value = row == 0
                    ? (processor.getStepEnabled(index) ? "ON" : "OFF")
                    : row == 1 ? (v > 0 ? "+" : "") +
                        (processor.getScale() == 13 ? juce::String(v / 10.0, 1) : juce::String(v / 10))
                    : row == 4 ? (v > 0 ? "+" : "") + juce::String(v)
                    : row == channelRow ? (v == 0 ? "D" : juce::String(v))
                    : row == panRow ? (v == 64 ? "C" : v < 64
                        ? "L" + juce::String(64 - v) : "R" + juce::String(v - 64))
                    : row == vibratoRow ? ("R" + juce::String(v) + "/D" +
                        juce::String(processor.getVibratoDepth(index)))
                    : row == sustainRow ? (v >= 50 ? "ON" : "OFF")
                    : juce::String(v) + "%";
                PnordeLights::text(g, value, b.toNearestInt(),
                    font(h * (row == vibratoRow ? 0.010f : 0.015f), true),
                    colour, brightness * 0.75f, darkness,
                    juce::Justification::centred, selected ? 1 : 0);
            }

            // The child ComboBox text stays dark for contrast. Only the
            // housing takes on its theme-paper colour and an LED outline.
            for (auto* combo : std::array<juce::ComboBox*, 2>{ { &divisions[row], &timings[row] } })
            {
                const auto local = combo->getBounds();
                const auto rect = juce::Rectangle<float>(
                    static_cast<float>(local.getX()),
                    static_cast<float>(local.getY()) - scrollY + h * 0.110f,
                    static_cast<float>(local.getWidth()),
                    static_cast<float>(local.getHeight()));
                g.setColour(palette.paper.withAlpha(0.035f + 0.085f * led));
                g.fillRoundedRectangle(rect.reduced(0.5f), 3.5f);
                PnordeLights::outline(g, rect.expanded(1.0f),
                    palette.paper.interpolatedWith(colour, 0.20f),
                    0.21f + 0.27f * led, darkness, 4.0f);
            }
            if (row >= 7 && row < modRow)
            {
                const auto cb = ccSelect[row - 7].getBounds();
                PnordeLights::outline(g,
                    {static_cast<float>(cb.getX()),
                     static_cast<float>(cb.getY()) - scrollY + h * 0.110f,
                     static_cast<float>(cb.getWidth()), static_cast<float>(cb.getHeight())},
                    palette.paper, 0.25f + 0.19f * led, darkness, 4.0f);
            }

            // Small page, delete and navigation button labels also belong to
            // the emissive text family. Their parent is scrollContent, so
            // translate rectangles to editor coordinates only when visible.
            auto lightButton = [&](const juce::TextButton& button)
            {
                if (!button.isVisible() || !button.isEnabled()) return;
                auto rect = button.getBounds();
                rect.translate(0, juce::roundToInt(h * 0.110f - scrollY));
                if (rect.getBottom() <= h * 0.110f || rect.getY() >= h * 0.925f)
                    return;
                PnordeLights::outline(g, rect.toFloat().expanded(0.6f),
                    palette.paper, 0.12f + led * 0.21f, darkness, 3.0f);
                PnordeLights::text(g, button.getButtonText(), rect,
                    font(h * 0.012f, true), colour, 0.18f + 0.43f * led,
                    darkness, juce::Justification::centred, 0);
            };
            lightButton(prev[row]);
            lightButton(next[row]);
            lightButton(addPage[row]);
            lightButton(removePage[row]);
            if (row >= 7 && row < modRow) lightButton(deleteCC[row - 7]);
            if (row == channelRow) lightButton(deleteChannelButton);
            if (row >= modRow && row < channelRow)
                lightButton(deleteDedicated[row - modRow]);
        }
        g.restoreState();
    }
    // Global light bus: the footer breathes with the host clock, advanced 45°.
    // Unlike lane clocks, this uses the global division to keep buttons and
    // signature gently unified without pushing any real MIDI events.
    const float footerGlow = playingLights
        ? PnordeLights::softPulse(globalStepPhase, 0.125f) : 0.22f;
    const float footerIntensity = 0.18f + 0.64f * footerGlow;
    const auto footerColour = palette.pink.interpolatedWith(palette.cyan, 0.17f);

    // Highlight the middle of the header with luminous traces rather than a
    // large opaque wash that would obscure dropdown text.
    {
        const float activity = playingLights ? globalLed : 0.0f;
        const juce::Rectangle<float> trace(w * 0.230f, h * 0.099f,
                                           w * 0.755f, 2.0f);
        g.setColour(palette.teal.withAlpha(0.020f + 0.055f * activity));
        g.fillRoundedRectangle(w * 0.230f, h * 0.024f,
                               w * 0.754f, h * 0.071f, 12.0f);
        g.setColour(palette.teal.withAlpha(0.085f + 0.24f * activity));
        g.fillRoundedRectangle(trace.expanded(0.0f, 4.0f), 3.0f);
        g.setColour(palette.cyan.withAlpha(0.28f + 0.43f * activity));
        g.fillRoundedRectangle(trace, 1.0f);
    }
    PnordeLights::text(g, "Pnorde",
        {juce::roundToInt(w * 0.061f), juce::roundToInt(h * 0.014f),
         juce::roundToInt(w * 0.28f), juce::roundToInt(h * 0.052f)},
        [&] { static const auto face = juce::Typeface::createSystemTypefaceFor(
            AudiowideEmbedded::fontData, AudiowideEmbedded::fontSize);
            auto f = face != nullptr
                ? juce::Font(juce::FontOptions(face).withHeight(h * 0.038f))
                : font(h * 0.038f, true);
            f.setHorizontalScale(1.10f); return f; }(),
        palette.coral, 0.42f + 0.53f * globalLed, darkness,
        juce::Justification::centredLeft, 2);
    PnordeLights::text(g, "D R E A M   S E Q U E N C E R",
        {juce::roundToInt(w * 0.062f), juce::roundToInt(h * 0.065f),
         juce::roundToInt(w * 0.171f), juce::roundToInt(h * 0.017f)},
        font(h * 0.012f), palette.teal,
        0.29f + 0.47f * globalLed, darkness);
    PnordeLights::text(g, "MIDI OUT",
        {juce::roundToInt(w * 0.239f), juce::roundToInt(h * 0.034f),
         juce::roundToInt(w * 0.098f), juce::roundToInt(h * 0.020f)},
        font(h * 0.012f, true), palette.teal, 0.20f + 0.53f * globalLed,
        darkness);
    const char* labels[] { "KEY", "SCALE", "GLOBAL CLOCK", "BEND RANGE", "THEME", "ADD LANE" };
    const float labelXs[] { 0.365f, 0.433f, 0.581f, 0.669f, 0.761f, 0.885f };
    const float labelWidths[] { 0.060f, 0.140f, 0.080f, 0.080f, 0.120f, 0.100f };
    for (int i = 0; i < 6; ++i)
        PnordeLights::text(g, labels[i],
            {juce::roundToInt(w * labelXs[i]), juce::roundToInt(h * 0.034f),
             juce::roundToInt(w * labelWidths[i]), juce::roundToInt(h * 0.020f)},
            font(h * 0.012f, true), palette.teal,
            0.20f + 0.41f * globalLed, darkness);
    PnordeLights::text(g, processor.isHostPlaying() ? "HOST PLAYING" : "HOST STOPPED",
        {juce::roundToInt(w * 0.865f), juce::roundToInt(h * 0.011f),
         juce::roundToInt(w * 0.108f), juce::roundToInt(h * 0.030f)},
        font(h * 0.012f, true), palette.sage,
        0.21f + 0.61f * globalLed, darkness, juce::Justification::centredRight);

    // All closed dropdowns keep dark selected-item text. Only their themed
    // backgrounds / outlines bloom; popup menu text isn't touched at all.
    for (auto* cb : std::array<juce::ComboBox*, 6>{ {
         &midiOutputSelect, &keySelect, &scaleSelect,
         &globalClockSelect, &bendRangeSelect, &themeSelect } })
    {
        g.setColour(palette.paper.withAlpha(0.035f + 0.080f * globalLed));
        g.fillRoundedRectangle(cb->getBounds().toFloat().reduced(0.5f), 4.0f);
        PnordeLights::outline(g, cb->getBounds().toFloat().expanded(1.1f),
            palette.paper.interpolatedWith(palette.teal, 0.12f),
            0.24f + 0.30f * globalLed, darkness, 4.5f);
    }
    PnordeLights::outline(g, refreshMidiOutputs.getBounds().toFloat().expanded(0.5f),
        palette.paper, 0.22f + globalLed * 0.42f, darkness, 3.0f);
    PnordeLights::text(g, refreshMidiOutputs.getButtonText(),
        refreshMidiOutputs.getBounds(), font(h * 0.011f, true),
        palette.teal, 0.18f + 0.46f * globalLed,
        darkness, juce::Justification::centred, 0);
    // ADD LANE deliberately receives the stronger header pulse.
    PnordeLights::outline(g, addCCButton.getBounds().toFloat().expanded(1.0f),
        palette.teal, 0.29f + 0.57f * globalLed, darkness, 4.5f);
    PnordeLights::text(g, "+ ADD LANE", addCCButton.getBounds(),
        font(h * 0.013f, true), palette.teal, 0.31f + 0.62f * globalLed,
        darkness, juce::Justification::centred);

    // Footer text, controls and settings all share the phase-shifted pulse.
    for (auto* control : { &savePresetButton, &loadPresetButton,
                           &randomiseButton, &presetFolderButton, &settingsButton })
    {
        const auto bounds = control->getBounds();
        PnordeLights::outline(g, bounds.toFloat().expanded(0.6f),
            palette.paper.interpolatedWith(palette.coral, 0.24f),
            footerIntensity, darkness, 4.0f);
        PnordeLights::text(g, control->getButtonText(), bounds,
            font(h * 0.012f, true), footerColour,
            footerIntensity, darkness, juce::Justification::centred);
    }
    // The slider is a child control too; illuminate its track and value box
    // around (not over) the value so it remains readable.
    PnordeLights::outline(g, darknessSlider.getBounds().toFloat().reduced(1.0f),
        palette.paper, 0.12f + footerIntensity * 0.44f, darkness, 4.0f);
    PnordeLights::text(g, "DARKNESS",
        {juce::roundToInt(w * 0.042f), juce::roundToInt(h * 0.942f),
         juce::roundToInt(w * 0.070f), juce::roundToInt(h * 0.031f)},
        font(h * 0.011f, true), footerColour, footerIntensity,
        darkness, juce::Justification::centredRight);
    PnordeLights::text(g, "Vibe Coded By",
        {juce::roundToInt(w * 0.672f), juce::roundToInt(h * 0.941f),
         juce::roundToInt(w * 0.120f), juce::roundToInt(h * 0.029f)},
        font(h * 0.012f), footerColour, footerIntensity,
        darkness, juce::Justification::centredRight, 2);
    PnordeLights::signature(g, juce::String::fromUTF8("LДNTERNS"),
        {juce::roundToInt(w * 0.796f),
         juce::roundToInt(h * 0.941f) - juce::roundToInt(h * 0.005f),
         juce::roundToInt(w * 0.141f), juce::roundToInt(h * 0.040f)},
        font(h * 0.023f, true), palette.pink,
        0.35f + footerGlow * 0.65f, darkness);

    // Drawn above child controls, anchored to the edited step (not the pointer).
    g.reduceClipRegion(0, juce::roundToInt(h * 0.110f), w, juce::roundToInt(h * 0.815f));
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
    if (ccPopupRow >= 7 && ccPopupRow < modRow && ccPopupStep >= 0 && ccPopupAlpha > 0.0f)
    {
        const int slot = ccPopupRow - 7;
        const int percent = processor.getCCTransitionMs(slot, ccPopupStep);
        const int visibleIndex = ccPopupStep - lane[static_cast<size_t>(ccPopupRow)].page * 16;
        if (visibleIndex < 0 || visibleIndex >= visibleSteps) return;
        const auto stepBounds = cell(ccPopupRow, visibleIndex);
        const float x = juce::jlimit(45.0f, w - 45.0f, stepBounds.getCentreX());
        // Anchor above the step, regardless of mouse movement. Can overlap adjacent rows.
        const float y = juce::jlimit(47.0f, h - 48.0f, stepBounds.getY() - 44.0f);
        const juce::Rectangle<float> dial(x - 32.0f, y - 32.0f, 64.0f, 64.0f);
        g.setColour(juce::Colours::white.withAlpha(ccPopupAlpha * 0.96f));
        g.fillEllipse(dial);
        g.setColour(ink.withAlpha(ccPopupAlpha * 0.18f));
        g.drawEllipse(dial.reduced(5.0f), 5.0f);
        const float start = juce::MathConstants<float>::pi * 0.75f;
        juce::Path arc;
        arc.addCentredArc(x, y, 25.0f, 25.0f, 0.0f, start,
            start + juce::MathConstants<float>::pi * 1.5f * percent / 100.0f, true);
        const auto dialColour = glideColour(colours[static_cast<size_t>(ccPopupRow)], percent / 100.0f);
        g.setColour(dialColour.withAlpha(ccPopupAlpha));
        g.strokePath(arc, juce::PathStrokeType(4.5f));
        g.setColour(ink.withAlpha(ccPopupAlpha));
        g.setFont(font(14.0f, true));
        g.drawText(juce::String(percent) + "%", dial.toNearestInt(), juce::Justification::centred);
        g.setFont(font(10.0f, true));
        g.drawText("GLIDE", juce::roundToInt(x - 28), juce::roundToInt(y + 34),
            56, 14, juce::Justification::centred);

    }
}
void PolyrhythmMIDIEditor::mouseDown(const juce::MouseEvent& e)
{
    if (!e.mods.isLeftButtonDown()) return;
    if (e.position.y < getHeight() * 0.110f || e.position.y >= getHeight() * 0.925f) return;
    for (int row = 0; row < lanes; ++row)
    {
        if (!rowVisible(row)) continue;
        auto& l = lane[static_cast<size_t>(row)];
        if (lengthBox(row).contains(e.position))
        {
            l.lengthDrag = true; l.startLength = l.length; l.dragOrigin = e.getPosition(); return;
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
                if (row == sustainRow)
                {
                    const int newValue = l.values[index] >= 50 ? 0 : 100;
                    processor.setLaneValue(row, index, newValue);
                    l.values[index] = newValue;
                    l.valueDrag = false;
                }
                if (row == vibratoRow)
                    l.dragStartDepth = processor.getVibratoDepth(index);
                if (row >= 7 && row < modRow)
                {
                    l.dragStartTransitionMs = processor.getCCTransitionMs(row - 7, index);
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
    if (row >= 7 && row < modRow)
    {
        // Horizontal drag: 50 ms per pixel, 200 px spans 0..10 seconds.
        const int dx = e.getPosition().x - l.dragOrigin.x;
        processor.setCCTransitionMs(row - 7, l.draggedStep,
            juce::jlimit(0, 100, l.dragStartTransitionMs + juce::roundToInt(dx / 2.0f)));
        ccPopupAlpha = 1.0f;
    }
    if (row == vibratoRow)
    {
        const int dx = e.getPosition().x - l.dragOrigin.x;
        processor.setVibratoDepth(l.draggedStep,
            l.dragStartDepth + juce::roundToInt(dx / (e.mods.isShiftDown() ? 8.0f : 2.5f)));
    }
    const float sensitivity = e.mods.isShiftDown() ? 12.0f : row == 1 && processor.getScale() == 13 ? 1.8f : 3.5f;
    const int maximum = row == channelRow ? 16 : row == panRow ? 127 : row == 1 ? 120 : row == 4 ? 4 : 100;
    const int minimum = row == 1 ? -120 : row == 4 ? -4 : row >= 5 ? 0 : 1;
    const int amount = row == panRow
        ? juce::roundToInt((e.getPosition().x - l.dragOrigin.x) / (e.mods.isShiftDown() ? 6.0f : 2.0f))
        : juce::roundToInt(dy / sensitivity);
    const int value = juce::jlimit(minimum, maximum, l.dragStartValue + amount);
    const int adjusted = row == 1 && processor.getScale() != 13
        ? processor.quantizePitchTenths(juce::roundToInt(value / 10.0) * 10, 0) : value;
    const auto step = static_cast<size_t>(l.draggedStep);
    l.values[step] = adjusted;
    // Keep the audio engine in sync so the timer cannot reset the dragged value.
    processor.setLaneValue(row, l.draggedStep, adjusted);
    // Animate towards the new value immediately; the timer continues the easing.
    const float targetFill = row == channelRow ? adjusted / 16.0f : row == panRow ? adjusted / 127.0f : row == 1 ? (adjusted + 120.0f) / 240.0f
                           : row == 4 ? (adjusted + 4.0f) / 8.0f : adjusted / 100.0f;
    l.visualFill[step] += (targetFill - l.visualFill[step]) * 0.45f;
    repaint();
}
void PolyrhythmMIDIEditor::mouseDrag(const juce::MouseEvent& e)
{
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
    // Keep the tooltip visible briefly, then fade it out in timerCallback.
    for (auto& l : lane) { l.lengthDrag = false; l.markerDrag = false; l.numberDrag = false; l.valueDrag = false; l.draggedStep = -1; }
}
void PolyrhythmMIDIEditor::mouseMove(const juce::MouseEvent& e)
{
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
            const int reset = row == 2 ? 70 : row == 3 ? 50 : row == panRow ? 64 : 0;
            processor.setLaneValue(row, step, reset);
            if (row >= 7 && row < modRow)
                processor.setCCTransitionMs(row - 7, step, 0);
            if (row == vibratoRow) processor.setVibratoDepth(step, 0);
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

    // Global footer lights use the global step duration, not the UI timer.
    // 45 degrees = one eighth of a complete pulse cycle.
    if (playing && animate)
    {
        const double beats = 0.5 * globalFactor;
        const double cycles = ppqNow / juce::jmax(0.00001, beats);
        const double frac = cycles - std::floor(cycles);
        globalStepPhase = static_cast<float>(frac);
        globalLed = PnordeLights::pulse(frac * beats * 60000.0 / bpm,
                                        beats * 60000.0 / bpm);
    }
    else
    {
        globalStepPhase = 0.0f;
        globalLed = 0.0f;
    }

    bool shouldRepaint = (playing != lastTransportPlaying);
    bool fullRepaint = shouldRepaint;
    lastTransportPlaying = playing;
    if (animate && playing) { pulse += 0.14; shouldRepaint = true; }
    if (scrollbarOpacity > 0.01f) {
        scrollbarOpacity *= 0.91f;
        shouldRepaint = true;
    }
    if (ccPopupRow >= 7 && ccPopupRow < modRow &&
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
    if (globalClockSelect.getSelectedId() != processor.getGlobalClockIndex() + 1)
        globalClockSelect.setSelectedId(processor.getGlobalClockIndex() + 1, juce::dontSendNotification);
    if (bendRangeSelect.getSelectedId() != processor.getBendRangeIndex() + 1)
        bendRangeSelect.setSelectedId(processor.getBendRangeIndex() + 1, juce::dontSendNotification);
    if (keySelect.getSelectedId() != processor.getKey() + 1)
        keySelect.setSelectedId(processor.getKey() + 1, juce::dontSendNotification);
    if (scaleSelect.getSelectedId() != processor.getScale() + 1)
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
            const int value = processor.getLaneValue(row, i);
            if (l.values[i] != value) { l.values[i] = value; shouldRepaint = true; }
            const float targetFill = row == channelRow ? value / 16.0f
                : row == panRow ? value / 127.0f
                : row == 1 ? (value + 120.0f) / 240.0f
                : row == 4 ? (value + 4.0f) / 8.0f
                : value / 100.0f;
            if (!animate || !playing)
                l.visualFill[i] = targetFill;
            else if (std::abs(targetFill - l.visualFill[i]) > 0.001f)
            {
                l.visualFill[i] += (targetFill - l.visualFill[i]) * 0.42f;
                shouldRepaint = true;
            }
        }
        if (playing && animate)
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
            // Repaint only the animated viewport rows and the title flash.
            // Only the narrow footer band is animated for footer lighting;
            // the remaining editor stays on the existing partial repaint path.
            const float h = static_cast<float>(getHeight());
            repaint(0, 0, getWidth(), juce::roundToInt(h * 0.108f));
            repaint(0, juce::roundToInt(h * 0.924f), getWidth(),
                    getHeight() - juce::roundToInt(h * 0.924f));
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

void PolyrhythmMIDIEditor::addMacroOfType(int type)
{
    const int slot = processor.getCustomCCCount();
    if (slot >= 8) return;
    processor.setMacroType(slot, type);
    processor.setMacroChannel(slot, 0);
    processor.setCustomCCLabel(slot, "Macro " + juce::String(slot + 1));
    processor.setCustomCCCount(slot + 1);
    committedCCLabels[slot] = processor.getCustomCCLabel(slot);
    ccLabelEditors[slot].setText(committedCCLabels[slot], false);
    syncCCVisibility();
    targetScrollY = maxScroll();
    resized(); repaint();
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
        divisions[row].setVisible(visible);
        timings[row].setVisible(visible);
        prev[row].setVisible(visible);
        next[row].setVisible(visible);
        addPage[row].setVisible(visible);
        removePage[row].setVisible(visible);
        if (row >= 7 && row < modRow)
        {
            const int slot = row - 7;
            ccSelect[slot].setVisible(visible);
            ccLabelEditors[slot].setVisible(visible);
            deleteCC[slot].setVisible(visible);
        }
        if (row >= modRow && row < channelRow)
            deleteDedicated[row - modRow].setVisible(visible);
    }
    deleteChannelButton.setVisible(processor.hasMidiChannelLane());
    addCCButton.setEnabled(processor.getCustomCCCount() < 8 ||
        !processor.hasMidiChannelLane() ||
        !processor.isDedicatedEnabled(modRow) || !processor.isDedicatedEnabled(expressionRow) ||
        !processor.isDedicatedEnabled(vibratoRow) || !processor.isDedicatedEnabled(panRow) ||
        !processor.isDedicatedEnabled(sustainRow));
}

void PolyrhythmMIDIEditor::applyTooltips()
{
    const bool enabled = processor.getTooltipsEnabled();
    for (int slot = 0; slot < 8; ++slot)
    {
        ccLabelEditors[slot].setTooltip(enabled ? "Rename this Macro lane. Press Enter to save." : "");
        ccSelect[slot].setTooltip(enabled ? "Choose the MIDI CC number for this Macro." : "");
    }
    for (int row = 0; row < lanes; ++row)
        divisions[row].setTooltip(enabled ? "Lane clock division. Double-click to reset." : "");
    globalClockSelect.setTooltip(enabled ? "Global clock multiplier. Double-click to reset to 1x." : "");
    addCCButton.setTooltip(enabled ? "Add up to eight Macros or one of each dedicated MIDI lane." : "");
    settingsButton.setTooltip(enabled ? "Open PNORDE settings." : "");
    savePresetButton.setTooltip(enabled ? "Save every pattern and setting as a .pnorde preset." : "");
    loadPresetButton.setTooltip(enabled ? "Load a .pnorde preset." : "");
    randomiseButton.setTooltip(enabled ? "Randomise only active steps in the selected lane or all lanes." : "");
    presetFolderButton.setTooltip(enabled ? "Open or choose a preset library folder." : "");
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
    juce::CallOutBox::launchAsynchronously(std::move(panel),
        settingsButton.getScreenBounds(), this);
}

void PolyrhythmMIDIEditor::syncFromProcessor()
{
    themeIndex = juce::jlimit(0, themeCount - 1, processor.getThemeIndex());
    themeSelect.setSelectedId(themeIndex + 1, juce::dontSendNotification);
    keySelect.setSelectedId(processor.getKey() + 1, juce::dontSendNotification);
    scaleSelect.setSelectedId(processor.getScale() + 1, juce::dontSendNotification);
    bendRangeSelect.setSelectedId(processor.getBendRangeIndex() + 1, juce::dontSendNotification);
    globalClockSelect.setSelectedId(processor.getGlobalClockIndex() + 1, juce::dontSendNotification);
    darknessSlider.setValue(processor.getDarknessPercent(), juce::dontSendNotification);
    for (int slot = 0; slot < 8; ++slot)
    {
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
        for (int i = 0; i < 128; ++i)
        {
            const int v = processor.getLaneValue(row, i);
            l.values[i] = v;
            l.visualFill[i] = row == channelRow ? v / 16.0f
                : row == panRow ? v / 127.0f
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
    for (int row = 0; row < lanes; ++row)
    {
        if (!rowVisible(row)) continue;
        const juce::String title = row >= 7 && row < modRow
            ? processor.getCustomCCLabel(row - 7) : juce::String(names[row]);
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
                    else if (row == panRow) value = 24 + rng.nextInt(81);
                    else if (row == sustainRow) value = rng.nextBool() ? 100 : 0;
                    else if (row == channelRow) value = rng.nextInt(17);
                    else value = rng.nextInt(101);
                    engine.setLaneValue(row, step, value);
                    if (row == vibratoRow)
                        engine.setVibratoDepth(step, rng.nextInt(101));
                }
            };
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
    for (auto* box : std::array<juce::ComboBox*, 4>{ { &keySelect, &scaleSelect, &globalClockSelect, &bendRangeSelect } })
    {
        box->setColour(juce::ComboBox::backgroundColourId, t.paper);
        box->setColour(juce::ComboBox::textColourId, t.ink);
        box->setColour(juce::ComboBox::arrowColourId, t.ink);
    }
    for (int row = 0; row < lanes; ++row)
    {
        const juce::Colour c[] {t.coral,t.blue,t.sage,t.lavender,t.amber,t.teal,t.pink,t.teal,t.amber,t.blue,t.coral,t.lavender,t.sage,t.pink,t.amber,t.teal,t.blue,t.lavender,t.pink,t.sage,t.amber};
        for (auto* box : std::array<juce::ComboBox*, 2>{ { &divisions[row], &timings[row] } })
        {
            box->setColour(juce::ComboBox::backgroundColourId, t.paper);
            box->setColour(juce::ComboBox::textColourId, t.ink);
            box->setColour(juce::ComboBox::arrowColourId, t.ink);
        }
        divisions[row].setColour(juce::ComboBox::outlineColourId, c[row]);
        for (auto* button : {&prev[row], &next[row]})
        {
            button->setColour(juce::TextButton::buttonColourId, t.paper);
            button->setColour(juce::TextButton::textColourOffId, t.ink);
        }
    }
    for (int slot = 0; slot < 8; ++slot)
    {
        ccSelect[slot].setColour(juce::ComboBox::backgroundColourId, t.paper);
        ccSelect[slot].setColour(juce::ComboBox::textColourId, t.ink);
        ccSelect[slot].setColour(juce::ComboBox::arrowColourId, t.ink);
        auto& editor = ccLabelEditors[slot];
        editor.setColour(juce::TextEditor::textColourId, t.ink);
        editor.setColour(juce::TextEditor::highlightedTextColourId, t.ink);
        editor.setColour(juce::TextEditor::focusedOutlineColourId, t.teal);
        editor.setColour(juce::TextEditor::highlightColourId, t.teal.withAlpha(0.28f));
    }
    addCCButton.setColour(juce::TextButton::buttonColourId, t.teal);
    addCCButton.setColour(juce::TextButton::textColourOffId, t.ink);
    for (auto* b : {&savePresetButton, &loadPresetButton, &presetFolderButton,
                    &randomiseButton, &settingsButton})
    {
        b->setColour(juce::TextButton::buttonColourId, t.paper);
        b->setColour(juce::TextButton::textColourOffId, t.ink);
    }
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
