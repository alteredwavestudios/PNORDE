#include "PluginEditor.h"
#include "AudiowideEmbedded.h"
#include "PnordeLights.h"
#include "PnordePresetFormat.h"
#include "PnordeFactoryPresets.h"
#include "PnordeCompositionEngine.h"
#include "PnordePlayback.h"
#include "PnordePatternLibrary.h"
#include "PnordeSafeCC.h"
#include "PnordeGlobalReverse.h"
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
    {"Ume Shadow",juce::Colour (juce::uint32 (0xff301B29)),juce::Colour (juce::uint32 (0xff57304A)),juce::Colour (juce::uint32 (0xff79465A)),juce::Colour (juce::uint32 (0xffF6E4E6)),juce::Colour (juce::uint32 (0xffF18C9D)),juce::Colour (juce::uint32 (0xff9B9DE3)),juce::Colour (juce::uint32 (0xffA5C3A0)),juce::Colour (juce::uint32 (0xffD3A4D2)),juce::Colour (juce::uint32 (0xffE7BA83)),juce::Colour (juce::uint32 (0xff78C8BD)),juce::Colour (juce::uint32 (0xffF5B2C9)),juce::Colour (juce::uint32 (0xff72E9ED))},
    // v0.6.9: appended palettes, preserving IDs and saved themes 0-7.
    {"Shibuya Neon", juce::Colour(0xff160F31), juce::Colour(0xff30204B), juce::Colour(0xff5D316C), juce::Colour(0xffFAE8F4), juce::Colour(0xffFE64A3), juce::Colour(0xff6BB5F8), juce::Colour(0xff91DFAF), juce::Colour(0xffC594F3), juce::Colour(0xffFFC59C), juce::Colour(0xff6DEBDD), juce::Colour(0xffF89BD9), juce::Colour(0xff8EEBFF)},
    {"Matcha Vapor", juce::Colour(0xff142820), juce::Colour(0xff284437), juce::Colour(0xff386956), juce::Colour(0xffF0F5E7), juce::Colour(0xffEFA0B3), juce::Colour(0xff8FBFCA), juce::Colour(0xff98DD9D), juce::Colour(0xffC5A6D9), juce::Colour(0xffE9C88B), juce::Colour(0xff7CE6C0), juce::Colour(0xffF5ADC4), juce::Colour(0xffAFEDE2)},
    {"Asakusa Night", juce::Colour(0xff15192F), juce::Colour(0xff2E3554), juce::Colour(0xff555077), juce::Colour(0xffFFF1DC), juce::Colour(0xffE8737B), juce::Colour(0xff869ED9), juce::Colour(0xffA5C6A7), juce::Colour(0xffB69BD8), juce::Colour(0xffF8CF76), juce::Colour(0xff75CBBF), juce::Colour(0xffF3A4B6), juce::Colour(0xffA4E7F3)},
    {"Koi Sunset", juce::Colour(0xff342131), juce::Colour(0xff573548), juce::Colour(0xff8C5163), juce::Colour(0xffFFF0DA), juce::Colour(0xffF58275), juce::Colour(0xff85A9C2), juce::Colour(0xff98BE9B), juce::Colour(0xffCD99C4), juce::Colour(0xffFFD08F), juce::Colour(0xff63D0C5), juce::Colour(0xffFAB7B5), juce::Colour(0xff9CDDE9)}
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

// Try user-installed Orbitron for futuristic lane labels. Distributing font
// binaries is unnecessary: without Orbitron, Bahnschrift SemiCondensed provides
// a legible geometric fallback. Font family enumeration is cached once.
juce::Font laneLabelFont(float size)
{
    static const bool hasOrbitron = juce::Font::findAllTypefaceNames()
        .contains("Orbitron", true);
    juce::Font chosen(juce::FontOptions(
        hasOrbitron ? juce::String("Orbitron") : juce::String("Bahnschrift SemiCondensed"),
        size, juce::Font::bold));
    chosen.setExtraKerningFactor(0.045f);
    return chosen;
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
// Dismiss-on-click-away JUCE callouts, never on the audio thread.
// The Bandcamp address is SELECTABLE TEXT, not an actionable hyperlink.
// v0.6.15: Popups are owned by the plugin editor itself. Unlike a native
// CallOutBox their layers/shadows cannot overdraw the Ableton host window.
class PnordeOverlayHost final : public juce::Component
{
public:
    PnordeOverlayHost(std::unique_ptr<juce::Component> panel,
                      juce::Rectangle<int> requested, std::function<void()> onClose)
        : body(std::move(panel)), anchor(requested), close(std::move(onClose))
    {
        setInterceptsMouseClicks(true, true);
        addAndMakeVisible(*body);
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.20f));
        const auto r = body->getBounds().toFloat();
        g.setColour(juce::Colours::white.withAlpha(0.24f));
        g.drawRoundedRectangle(r.expanded(2.0f), 5.0f, 1.5f);
    }
    void resized() override
    {
        constexpr int margin = 12;
        const int width = juce::jmin(body->getWidth(), juce::jmax(100, getWidth() - 2 * margin));
        const int height = juce::jmin(body->getHeight(), juce::jmax(100, getHeight() - 2 * margin));
        const int left = juce::jlimit(margin, juce::jmax(margin, getWidth() - width - margin),
            anchor.getCentreX() - width / 2);
        const int preferredY = anchor.getBottom() + 8;
        const int top = juce::jlimit(margin, juce::jmax(margin, getHeight() - height - margin),
            preferredY);
        body->setBounds(left, top, width, height);
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (!body->getBounds().contains(e.getPosition()) && close) close();
    }
private:
    std::unique_ptr<juce::Component> body;
    juce::Rectangle<int> anchor;
    std::function<void()> close;
};

// Four-step graduated beat groups: 1 DARK -> 2 softer -> 3 lighter ->
// 4 LIGHT, then repeat every four. These are BACKGROUNDS only, never painted
// over interactive cells; lane colours and light-up playheads stay intact.
static juce::Colour pnordeStepColumnBackground(juce::Colour paper,
                                               juce::Colour accent,
                                               int absoluteStep) noexcept
{
    const int beat = juce::jlimit(0, 3, absoluteStep % 4);
    const bool darkPalette = paper.getPerceivedBrightness() < 0.50f;
    const float shadows[] {0.23f, 0.16f, 0.09f, 0.02f};
    const auto shadow = paper.interpolatedWith(juce::Colours::black,
        shadows[beat] * (darkPalette ? 1.05f : 0.80f));
    return shadow.interpolatedWith(accent,
        (darkPalette ? 0.10f : 0.065f) + 0.018f * static_cast<float>(beat));
}

class PnordeInfoPanel final : public juce::Component
{
public:
    enum class Kind { bandcamp, about };
    PnordeInfoPanel(Kind kind, juce::Colour paper, juce::Colour ink,
                    juce::Colour accent)
        : isBandcamp(kind == Kind::bandcamp),
          background(juce::Colour::fromRGB(20, 27, 41)),
          foreground(juce::Colours::white), line(accent)
    {
        juce::ignoreUnused(ink, paper);
        setOpaque(true);
        setSize(390, 139);
        heading.setText(isBandcamp ? "Check out my bandcamp!!" : "PNORDE - DREAM SEQUENCER",
                        juce::dontSendNotification);
        heading.setFont(font(19.0f, true));
        heading.setColour(juce::Label::textColourId, foreground);
        heading.setBounds(15, 13, 360, 32);
        addAndMakeVisible(heading);
        if (isBandcamp)
        {
            copyableUrl.setMultiLine(false);
            copyableUrl.setReadOnly(true);
            copyableUrl.setText("l4nterns.bandcamp.com", false);
            copyableUrl.setColour(juce::TextEditor::backgroundColourId,
                                  juce::Colour::fromRGB(34, 43, 61));
            copyableUrl.setColour(juce::TextEditor::textColourId,
                                  juce::Colours::white);
            copyableUrl.setColour(juce::TextEditor::highlightColourId, line.withAlpha(0.30f));
            copyableUrl.setColour(juce::TextEditor::highlightedTextColourId,
                                  juce::Colours::white);
            copyableUrl.setColour(juce::TextEditor::outlineColourId, line.withAlpha(0.55f));
            copyableUrl.setFont(font(18.0f));
            copyableUrl.setBounds(18, 57, 352, 35);
            copyableUrl.setPopupMenuEnabled(true);
            addAndMakeVisible(copyableUrl);
            hint.setText("Select and copy the address to open it yourself.",
                         juce::dontSendNotification);
            hint.setBounds(17, 103, 361, 21);
            addAndMakeVisible(hint);
        }
        else
        {
            hint.setText("Version 0.6.21  |  HPPD visual engine", juce::dontSendNotification);
            hint.setBounds(17, 47, 361, 23);
            addAndMakeVisible(hint);
            copyableUrl.setMultiLine(false);
            copyableUrl.setReadOnly(true);
            copyableUrl.setText("github.com/alteredwavestudios/PNORDE", false);
            copyableUrl.setColour(juce::TextEditor::backgroundColourId,
                                  juce::Colour::fromRGB(34, 43, 61));
            copyableUrl.setColour(juce::TextEditor::textColourId,
                                  juce::Colours::white);
            copyableUrl.setColour(juce::TextEditor::highlightColourId, line.withAlpha(0.30f));
            copyableUrl.setColour(juce::TextEditor::highlightedTextColourId,
                                  juce::Colours::white);
            copyableUrl.setColour(juce::TextEditor::outlineColourId, line.withAlpha(0.55f));
            copyableUrl.setFont(font(14.0f));
            copyableUrl.setBounds(18, 80, 352, 34);
            copyableUrl.setPopupMenuEnabled(true);
            addAndMakeVisible(copyableUrl);
        }
        hint.setColour(juce::Label::textColourId, juce::Colours::white);
        copyableUrl.setColour(juce::TextEditor::highlightedTextColourId,
                              juce::Colours::white);
        // Force a real opaque background and contrasting text in EVERY palette.
        // Relying on the inherited TextEditor colours made dark themes unreadable.
        copyableUrl.setOpaque(true);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(background);
        g.setColour(line);
        g.fillRect(0, 0, getWidth(), 3);
    }
private:
    bool isBandcamp;
    juce::Colour background, foreground, line;
    juce::Label heading, hint;
    juce::TextEditor copyableUrl;

};

// Lightweight lane-direction dial. Its existing host parameter is the sole
// source of truth, so Ableton automation/MIDI mapping and this UI stay in sync.
class PnordeDirectionDial final : public PnordeLaneDial
{
public:
    PnordeDirectionDial()
    { setup(2, 0, labels, "Direction (host-automatable)"); }
private:
    static constexpr const char* labels[] { "FWD", "REV", "PING" };
};

class PnordeDirectionPanel final : public juce::Component, private juce::Timer
{
public:
    PnordeDirectionPanel(PolyrhythmMIDIProcessor& p, int lane, juce::Colour bg,
                         juce::Colour ink, juce::Colour accent)
        : processor(p), row(lane), background(bg), foreground(ink)
    {
        setSize(238, 164);
        setOpaque(true);
        dial.setPalette(bg, ink, accent);
        dial.setValue(processor.getLaneDirection(row));
        dial.onChange = [this](int direction)
        { processor.setLaneDirection(row, direction); };
        addAndMakeVisible(dial);
        reset.setButtonText("RESTART LANE");
        reset.onClick = [this] { processor.requestLaneReset(row); };
        reset.setColour(juce::TextButton::buttonColourId, bg);
        reset.setColour(juce::TextButton::textColourOffId, ink);
        addAndMakeVisible(reset);
        startTimerHz(12);
    }
    void resized() override
    {
        dial.setBounds(76, 36, 86, 83);
        reset.setBounds(64, 130, 110, 25);
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(background);
        g.setColour(foreground);
        g.setFont(font(15.0f, true));
        g.drawFittedText("LANE DIRECTION", 8, 8, 222, 23,
            juce::Justification::centred, 1);
        g.setFont(font(10.5f));
        g.drawText("FWD  /  REV  /  PING-PONG", 9, 114, 220, 16,
            juce::Justification::centred);
    }
private:
    void timerCallback() override
    {
        dial.setValue(processor.getLaneDirection(row));
    }
    PolyrhythmMIDIProcessor& processor;
    const int row;
    juce::Colour background, foreground;
    PnordeDirectionDial dial;
    juce::TextButton reset;
};

class PnordeSettingsPanel final : public juce::Component, private juce::Timer
{
public:
    PnordeSettingsPanel(PolyrhythmMIDIProcessor& p, std::function<void()> changed)
        : processor(p), onChanged(std::move(changed))
    {
        setSize(408, 976);
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
        quickProgram.setSliderStyle(juce::Slider::LinearHorizontal);
        quickProgram.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 25);
        quickProgram.setRange(1, 4, 1);
        quickProgram.setValue(processor.getSelectedProgram() + 1, juce::dontSendNotification);
        quickProgram.setTooltip("PNORDE Quick Program 1-4. Host parameter PNORDE Quick Program can be mapped to hardware in Ableton.");
        quickProgram.onValueChange = [this]
        {
            processor.selectProgram(juce::roundToInt(quickProgram.getValue()) - 1);
            if (onChanged) onChanged();
        };
        addAndMakeVisible(quickProgram);
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
        reorder.setButtonText("Drag coloured lane rails to reorder");
        reorder.setToggleState(processor.getExperimentalReorderEnabled(), juce::dontSendNotification);
        reorder.onClick = [this] { processor.setExperimentalReorderEnabled(reorder.getToggleState()); };
        addAndMakeVisible(reorder);
        psychedelic.setButtonText("HPPD  /  Experimental visuals");
        psychedelic.setToggleState(processor.getPsychedelicEnabled(),juce::dontSendNotification);
        psychedelic.onClick=[this]
        { processor.setPsychedelicEnabled(psychedelic.getToggleState());
          if (onChanged) onChanged(); };
        psychedelic.setTooltip("HPPD means Hallucinogenic Polyrhythm Pattern Discombobulator. One effect loaded. Photosensitivity caution; off by default.");
        addAndMakeVisible(psychedelic);
        psychedelicAmount.setSliderStyle(juce::Slider::LinearHorizontal);
        psychedelicAmount.setTextBoxStyle(juce::Slider::TextBoxRight,false,44,23);
        psychedelicAmount.setRange(0,40,1);
        psychedelicAmount.setValue(processor.getPsychedelicIntensity(),juce::dontSendNotification);
        psychedelicAmount.setTextValueSuffix("%");
        psychedelicAmount.setTooltip("HPPD intensity (0-40). Bright, crisp themed geometry.");
        psychedelicAmount.onValueChange=[this]
        {processor.setPsychedelicIntensity(juce::roundToInt(psychedelicAmount.getValue()));
         if (onChanged) onChanged();};
        addAndMakeVisible(psychedelicAmount);
        vibesBleed.setSliderStyle(juce::Slider::LinearHorizontal);
        vibesBleed.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 23);
        vibesBleed.setRange(0, 50, 1);
        vibesBleed.setValue(processor.getVibesBleed(), juce::dontSendNotification);
        vibesBleed.setTextValueSuffix("% bleed");
        vibesBleed.setTooltip("HPPD overlay visibility across the interface. 0 = background only.");
        vibesBleed.onValueChange = [this]
        { processor.setVibesBleed(juce::roundToInt(vibesBleed.getValue()));
          if (onChanged) onChanged(); };
        addAndMakeVisible(vibesBleed);
        hppdFps.setSliderStyle(juce::Slider::LinearHorizontal);
        hppdFps.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 23);
        hppdFps.setRange(1, 60, 1);
        hppdFps.setValue(processor.getHppdFps(), juce::dontSendNotification);
        hppdFps.setTextValueSuffix(" fps");
        hppdFps.setTooltip("Heavy HPPD frames per second (1-60). Default 12. Higher values increase GUI load; MIDI timing is independent.");
        hppdFps.onValueChange = [this]
        { processor.setHppdFps(juce::roundToInt(hppdFps.getValue())); };
        addAndMakeVisible(hppdFps);
        glowEnabled.setButtonText("Grid glow / halos");
        glowEnabled.setToggleState(processor.getVisualGlowEnabled(), juce::dontSendNotification);
        glowEnabled.setTooltip("Disable bloom/halo passes to save GPU/GUI resources while keeping bright flash changes.");
        glowEnabled.onClick = [this]
        { processor.setVisualGlowEnabled(glowEnabled.getToggleState()); if (onChanged) onChanged(); };
        addAndMakeVisible(glowEnabled);
        hypnosis.setSliderStyle(juce::Slider::LinearHorizontal);
        hypnosis.setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 23);
        hypnosis.setRange(0, 100, 1);
        hypnosis.setValue(processor.getHypnosis(), juce::dontSendNotification);
        hypnosis.setTextValueSuffix("% glow");
        hypnosis.setTooltip("Hypnosis: brightness and glow of active grid cells (0-100). Cached HPPD imagery keeps its independent FPS limit.");
        hypnosis.onValueChange = [this]
        { processor.setHypnosis(juce::roundToInt(hypnosis.getValue())); if (onChanged) onChanged(); };
        addAndMakeVisible(hypnosis);
        vibesMode.addItem("NEON NODAL  /  kaleidoscopic geometry", 1);
        vibesMode.addItem("PHASE WEAVE  /  flowing rhythmic ribbons", 2);
        vibesMode.addItem("PRISM SPIKES  /  incoming triangular shards", 3);
        vibesMode.addItem("UNCANNY CHESS  /  warped chessboard", 4);
        vibesMode.addItem("CHROMATIC DRIFT  /  living gradient", 5);
        vibesMode.setSelectedId(processor.getVibesMode() + 1, juce::dontSendNotification);
        vibesMode.setTooltip("Only the selected effect is loaded. Switches release the old renderer and its cached image.");
        vibesMode.onChange = [this]
        {
            processor.setVibesMode(vibesMode.getSelectedId() - 1);
            if (onChanged) onChanged();
        };
        addAndMakeVisible(vibesMode);
        nextVibe.setButtonText("NEXT HPPD >");
        nextVibe.setTooltip("Unload the current HPPD renderer and load the next one; only one stays active.");
        nextVibe.onClick = [this]
        {
            const int nextId = vibesMode.getSelectedId() % 5 + 1;
            vibesMode.setSelectedId(nextId, juce::sendNotificationSync);
        };
        addAndMakeVisible(nextVibe);

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
        experimental.setButtonText("Dark mode lighting");
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
          updateDarknessTip();
          if (onChanged) onChanged(); };
        addAndMakeVisible(darkness);
        darkness.setEnabled(processor.getExperimentalLightsEnabled());
        darkness.setTooltip("Ctrl/Cmd + mouse wheel anywhere over PNORDE: up = brighter, down = darker. Disabled in simple mode.");
        darknessTip.setJustificationType(juce::Justification::centredRight);
        darknessTip.setFont(font(12.0f, true));
        addAndMakeVisible(darknessTip);
        updateDarknessTip();
        startTimerHz(12); // UI only: synchronise host/shortcut brightness updates.

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
        g.setFont(font(12.0f)); g.setColour(p.ink.withAlpha(0.95f));
        g.drawText("Colour theme", 20, 75, 160, 18, juce::Justification::centredLeft);
        {
            const auto& swatch = themes[juce::jlimit(0, themeCount - 1, theme.getSelectedId() - 1)];
            const float d = 20.0f, x = 156.0f, y = 74.0f;
            juce::ColourGradient grad(swatch.coral, x, y, swatch.teal, x + d, y + d, false);
            grad.addColour(0.50, swatch.lavender);
            g.setGradientFill(grad);
            g.fillEllipse(x, y, d, d);
            g.setColour(swatch.ink.withAlpha(0.55f));
            g.drawEllipse(x, y, d, d, 1.0f);
        }
        g.drawText("Default MIDI channel", 20, 133, 230, 18,
                   juce::Justification::centredLeft);
        g.drawText("Quick-load program (1-4)", 20, 191, 230, 18, juce::Justification::centredLeft);
        g.drawText("Pitch bend range", 20, 249, 230, 18,
                   juce::Justification::centredLeft);
        g.drawText("DARK MODE", 20, 307, 155, 18,
                   juce::Justification::centredLeft);
        g.setColour(p.ink.withAlpha(0.24f));
        g.drawHorizontalLine(378, 18.0f, static_cast<float>(getWidth() - 18));
        g.drawHorizontalLine(664, 18.0f, static_cast<float>(getWidth() - 18));
        g.drawHorizontalLine(912, 18.0f, static_cast<float>(getWidth() - 18));
        g.setColour(p.ink.withAlpha(0.92f));
        g.drawText("HPPD RATE / HYPNOSIS", 20, 666, 355, 17,
                   juce::Justification::centredLeft);
        g.drawText("Microtonal MIDI output", 20, 776, 330, 16,
                   juce::Justification::centredLeft);
        g.setColour(p.ink.withAlpha(0.85f));
        g.drawText("PNORDE v0.6.21  /  HPPD", 18, 941,
                   getWidth() - 36, 15, juce::Justification::centredLeft);
    }
    void resized() override
    {
        const int w = getWidth() - 40;
        theme.setBounds(20, 95, w, 28);
        channel.setBounds(20, 153, w, 28);
        quickProgram.setBounds(20, 211, w, 28);
        bend.setBounds(20, 269, w, 28);
        darkness.setBounds(20, 329, w, 32);
        darknessTip.setBounds(178, 300, w - 160, 28);
        experimental.setBounds(20, 373, w, 26);
        animations.setBounds(20, 410, w, 26);
        fades.setBounds(20, 440, w, 26);
        tooltips.setBounds(20, 470, w, 26);
        reorder.setBounds(20, 500, w, 26);
        psychedelic.setBounds(20, 534, w, 26);
        psychedelicAmount.setBounds(20, 565, w, 26);
        vibesBleed.setBounds(20, 598, w, 26);
        vibesMode.setBounds(20, 628, w - 118, 26);
        nextVibe.setBounds(20 + w - 110, 628, 110, 26);
        // Same compact Settings footprint as v0.6.8, including at narrow heights.
        const int split = (w - 10) / 2;
        hppdFps.setBounds(20, 686, split, 25);
        glowEnabled.setBounds(20, 718, w, 23);
        hypnosis.setBounds(20, 744, w, 25);
        microMode.setBounds(20, 797, w, 26);
        mpePoly.setBounds(20, 828, w, 23);
        diagnostics.setBounds(20, 854, w, 15);
        refreshDiagnostics.setBounds(20, 872, w, 24);
        panic.setBounds(20, 903, w, 25);
    }
private:
    void updateDarknessTip()
    {
        const auto next = juce::String(processor.getDarknessPercent())
            + "% dimmed  |  Ctrl/Cmd + wheel";
        if (darknessTip.getText() != next)
            darknessTip.setText(next, juce::dontSendNotification);
    }
    void timerCallback() override
    {
        const auto enabled = processor.getExperimentalLightsEnabled();
        if (experimental.getToggleState() != enabled)
            experimental.setToggleState(enabled, juce::dontSendNotification);
        darkness.setEnabled(enabled);
        if (juce::roundToInt(quickProgram.getValue()) != processor.getSelectedProgram() + 1)
            quickProgram.setValue(processor.getSelectedProgram() + 1, juce::dontSendNotification);
        if (juce::roundToInt(darkness.getValue()) != processor.getDarknessPercent())
            darkness.setValue(processor.getDarknessPercent(), juce::dontSendNotification);
        updateDarknessTip();
    }
    void applyPalette()
    {
        const auto& p = themes[juce::jlimit(0, themeCount - 1, processor.getThemeIndex())];
        for (auto* combo : { &theme, &channel, &bend, &microMode, &vibesMode })
        {
            combo->setColour(juce::ComboBox::backgroundColourId, p.cream);
            combo->setColour(juce::ComboBox::textColourId, p.ink);
            combo->setColour(juce::ComboBox::arrowColourId, p.ink);
            combo->setColour(juce::ComboBox::outlineColourId, p.teal);
        }
        for (auto* toggle : { &animations, &fades, &tooltips, &experimental, &reorder, &mpePoly, &psychedelic, &glowEnabled })
        {
            toggle->setColour(juce::ToggleButton::textColourId, p.ink);
            toggle->setColour(juce::ToggleButton::tickColourId,
                p.paper.getPerceivedBrightness() > 0.55f ? juce::Colours::black : p.ink);
            toggle->setColour(juce::ToggleButton::tickDisabledColourId, p.ink.withAlpha(0.5f));
        }
        psychedelicAmount.setColour(juce::Slider::textBoxTextColourId,p.ink);
        psychedelicAmount.setColour(juce::Slider::textBoxBackgroundColourId,p.cream);
        psychedelicAmount.setColour(juce::Slider::thumbColourId,p.teal);
        vibesBleed.setColour(juce::Slider::textBoxTextColourId,p.ink);
        vibesBleed.setColour(juce::Slider::textBoxBackgroundColourId,p.cream);
        vibesBleed.setColour(juce::Slider::thumbColourId,p.lavender);
        hppdFps.setColour(juce::Slider::textBoxTextColourId,p.ink);
        hppdFps.setColour(juce::Slider::textBoxBackgroundColourId,p.cream);
        hppdFps.setColour(juce::Slider::thumbColourId,p.cyan);
        hypnosis.setColour(juce::Slider::textBoxTextColourId,p.ink);
        hypnosis.setColour(juce::Slider::textBoxBackgroundColourId,p.cream);
        hypnosis.setColour(juce::Slider::thumbColourId,p.pink);
        darknessTip.setColour(juce::Label::textColourId, p.ink);
        darknessTip.setColour(juce::Label::backgroundColourId, p.teal.withAlpha(0.10f));
        darkness.setColour(juce::Slider::textBoxTextColourId, p.ink);
        darkness.setColour(juce::Slider::textBoxBackgroundColourId, p.cream);
        darkness.setColour(juce::Slider::textBoxOutlineColourId, p.middle);
        darkness.setColour(juce::Slider::thumbColourId, p.teal);
        darkness.setColour(juce::Slider::trackColourId, p.teal);
        quickProgram.setColour(juce::Slider::thumbColourId, p.teal);
        quickProgram.setColour(juce::Slider::trackColourId, p.teal);
        quickProgram.setColour(juce::Slider::textBoxTextColourId, p.ink);
        quickProgram.setColour(juce::Slider::textBoxBackgroundColourId, p.cream);
        diagnostics.setColour(juce::Label::textColourId, p.ink);
        refreshDiagnostics.setColour(juce::TextButton::buttonColourId, p.cream);
        refreshDiagnostics.setColour(juce::TextButton::textColourOffId, p.ink);
        panic.setColour(juce::TextButton::buttonColourId, p.cream);
        panic.setColour(juce::TextButton::textColourOffId, p.ink);
    }
    void updateDiagnostics()
    {
        const int bendValue = processor.getLastPitchWheelValue();
        diagnostics.setText("Bend=" + juce::String(bendValue) +
            "  EDO offset=" + juce::String(processor.getLastEDOOffsetCents()) +
            "c  Messages=" + juce::String(processor.getPitchWheelSendCount()),
            juce::dontSendNotification);
    }
    PolyrhythmMIDIProcessor& processor;
    std::function<void()> onChanged;
    juce::ComboBox theme, channel, bend, microMode, vibesMode;
    juce::ToggleButton animations, fades, tooltips, experimental, reorder, mpePoly, psychedelic, glowEnabled;
    juce::Label diagnostics;
    juce::TextButton refreshDiagnostics;
    juce::TextButton nextVibe;
    juce::Slider darkness, psychedelicAmount, vibesBleed, hppdFps, hypnosis, quickProgram;
    juce::Label darknessTip;
    juce::TextButton panic;
};


// Settings are intentionally taller than a minimum-height PNORDE editor.
// Keep their full-size controls legible and scroll within a bounded child,
// rather than allowing a 914px CallOutBox to escape a 760px host window.
class PnordeSettingsViewport final : public juce::Component
{
public:
    PnordeSettingsViewport(std::unique_ptr<juce::Component> contents, int maxHeight)
    {
        const int contentWidth = contents->getWidth();
        const int contentHeight = contents->getHeight();
        const int panelHeight = juce::jmin(contentHeight, juce::jmax(240, maxHeight));
        setSize(contentWidth + 12, panelHeight);
        viewport.setViewedComponent(contents.release(), true);
        viewport.setScrollBarsShown(true, false);
        viewport.setScrollBarThickness(10);
        addAndMakeVisible(viewport);
    }
    void resized() override { viewport.setBounds(getLocalBounds()); }
private:
    juce::Viewport viewport;
};

PolyrhythmMIDIEditor::PolyrhythmMIDIEditor(PolyrhythmMIDIProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    const auto& palette = themes[0];
    const auto paper = palette.paper, ink = palette.ink, teal = palette.teal;
    setResizable(true, true);
    setResizeLimits(800, 760, 2800, 1620);
    // Unlocked width: extra room outside the fixed grid reveals more HPPD.
    const int initialWidth = juce::jlimit(800, 2800, processor.getEditorWidth());
    setSize(initialWidth, juce::jlimit(760, 1620, processor.getEditorHeight()));
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
        // Quantisation is applied at note generation, not destructively to stored pitch steps.
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
    octaveDial.setChoices(juce::StringArray {"-3", "-2", "-1", "0", "+1", "+2", "+3"},
                         processor.getGlobalOctave() + 3);
    octaveDial.setDescription("GLOBAL OCTAVE: transpose notes within MIDI 0-127");
    octaveDial.onChange = [this](int idx) { processor.setGlobalOctave(idx-3); repaint(); };
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
        // Free 0.1 deliberately ignores the Key dial; changing away from Free
        // resumes quantisation without ever rewriting stored pitch values.
        // Preserve source pitch steps so keys/scales remain reversible.
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
        laneType.addItem("MOD WHEEL  (CC1)", 2);
        laneType.addItem("EXPRESSION PEDAL  (CC11)", 3);
        laneType.addItem("VIBRATO  (CC76/77)", 4);
        laneType.addItem("PAN  (CC10)", 5);
        laneType.addItem("SUSTAIN PEDAL  (CC64)", 6);
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
            {
                processor.setCustomCCNumber(slot, selectedCC);
                applyTooltips();
                syncLaneNames();
            }
        };
        select.setColour(juce::ComboBox::backgroundColourId, paper);
        select.setColour(juce::ComboBox::textColourId, ink);
        scrollContent.addAndMakeVisible(select);
        auto& labelEditor = ccLabelEditors[slot];
        labelEditor.onDirectionMenu = [this,slot] { showLaneDirectionMenu(slot+7); };
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
        labelEditor.setFont(laneLabelFont(getHeight() * 0.0185f));
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
        menu.addSeparator();
        menu.addItem(100, "PROGRAM (automatic program changes)",
                     !processor.getProgramLaneEnabled());
        auto safe = juce::Component::SafePointer<PolyrhythmMIDIEditor>(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&addCCButton),
            [safe](int id)
            {
                if (safe == nullptr) return;
                if (id == 100)
                {
                    safe->processor.setProgramLaneEnabled(true);
                    safe->targetScrollY = safe->maxScroll();
                    safe->syncCCVisibility();
                    safe->resized(); safe->repaint();
                    return;
                }
                if (id < 1 || id > 7) return;
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
    addChildComponent(darknessSlider); // Hidden legacy control; Settings owns the visible slider.
    initialisePresetDirectory();
    presetFolderButton.onClick = [this] { showPresetFolderMenu(); };
    presetFolderButton.setTooltip("Open, choose, or reset the PNORDE preset folder");
    addAndMakeVisible(presetFolderButton);
    savePresetButton.onClick = [this] { savePreset(); };
    loadPresetButton.onClick = [this] { showLoadPresetMenu(); };
    playButton.setButtonText(processor.getPlayEnabled() ? "PAUSE" : "PLAY");
    playButton.onClick=[this]
    {processor.setPlayEnabled(!processor.getPlayEnabled());
     playButton.setButtonText(processor.getPlayEnabled()?"PAUSE":"PLAY"); repaint();};
    resetButton.onClick=[this] {processor.requestSequenceReset(); processor.requestMidiPanic();};
    addAndMakeVisible(playButton);
    addAndMakeVisible(resetButton);
    globalReverseButton.setButtonText("REV: OFF");
    globalReverseButton.setTooltip("Global Reverse is an Ableton-automatable 0/1 parameter. Flips all lane travel without jumping cells.");
    globalReverseButton.onClick = [this]
    { processor.setGlobalReverse(!processor.getGlobalReverse());
      globalReverseButton.setButtonText(processor.getGlobalReverse() ? "REV: ON" : "REV: OFF");
      const auto& t = themes[themeIndex];
      globalReverseButton.setColour(juce::TextButton::buttonColourId,
          processor.getGlobalReverse() ? t.coral.brighter(0.25f) : t.paper); };
    addAndMakeVisible(globalReverseButton);
    for (int i = 0; i < 4; ++i)
    {
        auto& button = quickPrograms[static_cast<size_t>(i)];
        button.setButtonText(juce::String(i+1));
        button.setTooltip("Quick program " + juce::String(i + 1) +
            ": complete scene. Saved with the PNORDE preset. Automatable via PNORDE Quick Program.");
        button.onClick = [this, i]
        {
            processor.selectProgram(i);
            syncFromProcessor();
            lastDisplayedProgram = -1;
            refreshProgramButtons();
        };
        button.onContext = [this, i] { showProgramMenu(i); };
        addAndMakeVisible(button);
    }
    refreshProgramButtons();
    programClockDial.setValue(processor.getProgramLaneDivision());
    programClockDial.onChange = [this](int division)
    {
        processor.setProgramLaneDivision(division);
        repaint();
    };
    programClockDial.onContextMenu = [this] { showProgramLaneMenu(); };
    programTimingDial.setValue(processor.getProgramLaneTiming());
    programTimingDial.onChange = [this](int timing)
    {
        processor.setProgramLaneTiming(timing);
        repaint();
    };
    programTimingDial.onContextMenu = [this] { showProgramLaneMenu(); };
    scrollContent.addAndMakeVisible(programClockDial);
    scrollContent.addAndMakeVisible(programTimingDial);
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
        dial.onContextMenu = [this, row] { showClocksMenu(row); };
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
            // Sequence storage is still 16-step pages; viewport pages may be 4/8/12.
            processor.deletePage(row, (a.page * visibleSteps) / 16);
            a.length = processor.getLaneLength(row);
            a.page = juce::jmin(a.page, (a.length - 1) / visibleSteps);
            a.displayX = -1.0f; updateButtons(); repaint();
        };
        plus.onClick = [this, row]
        {
            auto& a = lane[static_cast<size_t>(row)];
            processor.appendPage(row);
            a.length = processor.getLaneLength(row);
            a.page = (a.length - 1) / visibleSteps;
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
    for (int row = 0; row < lanes; ++row)
    {
        auto& editor = laneNameEditors[static_cast<size_t>(row)];
        editor.onDirectionMenu = [this, row] { showLaneDirectionMenu(row); };
        editor.onDirectionDial = [this, row] { showLaneDirectionPanel(row); };
        savedLaneNames[static_cast<size_t>(row)] = processor.getLaneName(row);
        editor.setText(savedLaneNames[static_cast<size_t>(row)], false);
        editor.setInputRestrictions(24);
        editor.setFont(laneLabelFont(getHeight() * 0.016f));
        editor.setSelectAllWhenFocused(row >= 7);
        editor.setReadOnly(row < 7);
        editor.setInterceptsMouseClicks(row >= 7, row >= 7);
        editor.setJustification(juce::Justification::centredLeft);
        editor.setColour(juce::TextEditor::backgroundColourId,
                         juce::Colours::transparentBlack);
        editor.setEditingTextColour(themes[themeIndex].ink);
        editor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        editor.setColour(juce::TextEditor::focusedOutlineColourId, themes[themeIndex].teal);
        editor.onReturnKey = [this, row]
        {
            const auto text = laneNameEditors[row].getText().trim().substring(0, 24);
            processor.setLaneName(row, text);
            savedLaneNames[row] = processor.getLaneName(row);
            laneNameEditors[row].setText(savedLaneNames[row], false);
            laneNameEditors[row].giveAwayKeyboardFocus();
            syncLaneNames();
        };
        editor.onEscapeKey = [this, row]
        {
            laneNameEditors[row].setText(savedLaneNames[row], false);
            laneNameEditors[row].giveAwayKeyboardFocus();
        };
        editor.onFocusLost = [this, row]
        {
            laneNameEditors[row].setText(savedLaneNames[row], false);
        };
        scrollContent.addAndMakeVisible(editor);
    }
    syncLaneNames();
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
    return count + (processor.getProgramLaneEnabled() ? 1 : 0);
}
juce::String PolyrhythmMIDIEditor::laneDisplayName(int row) const
{
    if (row < 0 || row >= lanes) return {};
    const auto custom = processor.getLaneName(row);
    if (custom.isNotEmpty()) return custom;
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

int PolyrhythmMIDIEditor::columnsForWidth(int width) const noexcept
{
    // 454 px are reserved for labels, page buttons, clock and timing dials.
    // Never offer a column count whose guaranteed minimum spacing will clip.
    return width >= 982 ? 16 : width >= 720 ? 8 : 4;
}
float PolyrhythmMIDIEditor::stepStride() const noexcept
{
    if (visibleSteps == 4) return juce::jlimit(42.0f, 60.0f,
        (getWidth() - 454.0f) / 4.0f);
    const float available = static_cast<float>(getWidth()) - 454.0f;
    return juce::jlimit(visibleSteps == 16 ? 32.0f : 33.0f,
                        60.0f, available / static_cast<float>(visibleSteps));
}
float PolyrhythmMIDIEditor::workWidth() const noexcept
{
    return 454.0f + stepStride() * static_cast<float>(visibleSteps);
}
float PolyrhythmMIDIEditor::workLeft() const noexcept
{
    return juce::jmax(0.0f, (getWidth() - workWidth()) * 0.5f);
}
float PolyrhythmMIDIEditor::gridStartX() const noexcept { return workLeft() + 250.0f; }
float PolyrhythmMIDIEditor::clockStartX() const noexcept { return gridStartX() + visibleSteps * stepStride() + 8.0f; }
float PolyrhythmMIDIEditor::timingStartX() const noexcept { return clockStartX() + 76.0f; }
float PolyrhythmMIDIEditor::pageStartX() const noexcept { return timingStartX() + 66.0f; }
juce::Rectangle<int> PolyrhythmMIDIEditor::headerBounds(int control) const noexcept
{
    // Controls 0-3: musically central; controls 4-7: right-aligned utility group.
    const int w = getWidth(), h = getHeight();
    const bool stacked = w < 1220;
    static constexpr int midOffsets[] {0, 120, 210, 363};
    static constexpr int midWidths[] {112, 82, 145, 105};
    static constexpr int compactOffsets[] {0, 96, 175, 309};
    static constexpr int compactWidths[] {91, 72, 130, 87};
    static constexpr int rightOffsets[] {0, 90, 161, 235};
    static constexpr int rightWidths[] {83, 65, 70, 105};
    if (control < 4)
    {
        // Below 1220px the globals occupy a compact SECOND HEADER ROW,
        // to the right of the identity/program cluster. Utility selects
        // stay in the first header row. No overlap at the 800px minimum.
        // Wide displays get additional horizontal breathing room for four
        // larger global controls; the entire group remains centred and clear
        // of the right-hand utility controls. Compact layout is unchanged.
        const float spread = !stacked && w >= 1450 ? 1.18f : 1.0f;
        const int left = stacked ? 350
            : juce::roundToInt((w - 475.0f * spread) * 0.5f);
        const int x = left + (stacked ? compactOffsets[control]
                                    : juce::roundToInt(midOffsets[control] * spread));
        const int width = stacked ? compactWidths[control]
                                  : midWidths[control] + (w >= 1450 ? 12 : 0);
        return {x, juce::roundToInt(h * (stacked ? 0.063f : 0.043f)), width,
                juce::roundToInt(h * (stacked ? 0.039f : 0.057f))};
    }
    const int i = control - 4;
    return {w - 353 + rightOffsets[i], juce::roundToInt(h * (stacked ? 0.016f : 0.055f)),
            rightWidths[i], juce::roundToInt(h * (stacked ? 0.028f : 0.034f))};
}
juce::String PolyrhythmMIDIEditor::midiDescription(int row) const
{
    if (row == 0) return "Note gate on/off (no MIDI CC).";
    if (row == 1) return "Pitch notes; microtonal pitch bend when required.";
    if (row == 2) return "Note velocity (not a MIDI CC).";
    if (row == 3) return "Note gate length (not a MIDI CC).";
    if (row == 4) return "Note octave / pitch (not a MIDI CC).";
    if (row == 5) return "Slide / pitch bend.";
    if (row == 6) return "Channel aftertouch / pressure (not CC).";
    const auto kind = static_cast<PnordeLane::Type>(processor.getOptionalLaneType(row));
    if (kind == PnordeLane::Type::ccMacro)
        return "MIDI CC" + juce::String(processor.getCustomCCNumber(row - 7));
    if (kind == PnordeLane::Type::vibrato) return "Vibrato rate CC76, depth CC77";
    if (kind == PnordeLane::Type::midiChannel) return "MIDI channel select (not CC)";
    const int cc = PnordeLane::cc(kind);
    return cc >= 0 ? "MIDI CC" + juce::String(cc) : "No dedicated MIDI CC";
}

juce::String PolyrhythmMIDIEditor::laneHoverDescription(int row) const
{
    const int start = processor.getLaneStartStep(row) + 1;
    return laneDisplayName(row) + ". " + midiDescription(row)
        + ". Ctrl/Cmd-click a step NUMBER to set the start marker (inside the lane length). Current start step: "
        + juce::String(start) + ".";
}

void PolyrhythmMIDIEditor::syncLaneNames()
{
    for (int row = 0; row < lanes; ++row)
    {
        auto& e = laneNameEditors[static_cast<size_t>(row)];
        const auto stored = processor.getLaneName(row);
        savedLaneNames[static_cast<size_t>(row)] = stored;
        if (!e.hasKeyboardFocus(true)) e.setText(stored, false);
        const auto& pal = themes[themeIndex];
        const juce::Colour colours[] {pal.coral,pal.blue,pal.sage,pal.lavender,pal.amber,pal.teal,pal.pink,
            pal.teal,pal.amber,pal.blue,pal.coral,pal.lavender,pal.sage,pal.pink,
            pal.amber,pal.teal,pal.blue,pal.lavender,pal.pink,pal.sage,pal.amber};
        const bool dark = pal.paper.getPerceivedBrightness() < 0.50f;
        // The final text pass paints the placeholder too: keep JUCE's own
        // placeholder invisible to prevent doubled names.
        e.setTextToShowWhenEmpty(laneDisplayName(row), juce::Colours::transparentBlack);
        e.setTooltip(processor.getTooltipsEnabled()
            ? (row < 7
                ? laneHoverDescription(row) + " Right-click for direction/reset."
                : "Rename lane: type a name and press Enter. " + midiDescription(row)
                    + ". Right-click for direction/reset.")
            : juce::String());
    }
}
float PolyrhythmMIDIEditor::programLaneTop() const noexcept
{
    return getHeight() * (0.155f + visibleIndex(lanes) * 0.110f) - scrollY;
}
juce::Rectangle<float> PolyrhythmMIDIEditor::programLaneCell(int localStep) const noexcept
{
    const float stride = stepStride();
    return {gridStartX() + localStep * stride,
        programLaneTop() + getHeight() * 0.048f,
        juce::jmin(52.0f, stride - 7.0f), getHeight() * 0.040f};
}
float PolyrhythmMIDIEditor::rowTop(int row) const
{
    return getHeight() * (0.155f + visibleIndex(row) * 0.110f) - scrollY;
}
juce::Rectangle<float> PolyrhythmMIDIEditor::cell(int row, int step) const
{
    const float stride = stepStride();
    const float width = juce::jmin(52.0f, stride - 7.0f);
    return {gridStartX() + step * stride, rowTop(row) + getHeight() * 0.048f,
            width, getHeight() * 0.040f};
}
juce::Rectangle<float> PolyrhythmMIDIEditor::lengthBox(int row) const
{
    return {workLeft() + 47.0f, rowTop(row) + getHeight() * 0.049f,
            137.0f, getHeight() * 0.034f};
}

juce::Rectangle<float> PolyrhythmMIDIEditor::reorderHandle(int row) const
{
    // The visible rail is 4 px wide at x = 0.055 * width.
    // This much larger hit target ends before the Macro text editor at 0.068.
    // Delete buttons end at 0.046, so neither interaction overlaps.
    return {workLeft() + 14.0f, rowTop(row) - getHeight() * 0.011f,
            27.0f, getHeight() * 0.097f};
}
float PolyrhythmMIDIEditor::markerXFor(int row, int length) const
{
    const int local = juce::jlimit(1, visibleSteps,
        length - lane[static_cast<size_t>(row)].page * visibleSteps);
    return cell(row, local - 1).getRight() + 4.0f;
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

juce::Rectangle<float> PolyrhythmMIDIEditor::footerCreditBounds() const noexcept
{
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());
    const bool stacked = w < 1380.0f;
    const float groupWidth = juce::jlimit(230.0f, 315.0f, w * 0.220f);
    const float height = juce::jmax(24.0f, h * 0.035f);
    // Same baseline as footer controls in a wide window; independent second
    // line in a narrow one, safely inside the editor bottom edge.
    const float top = stacked ? h - height - 6.0f : h * 0.941f - 3.0f;
    return { w - groupWidth - 24.0f, top, groupWidth, height };
}

void PolyrhythmMIDIEditor::resized()
{
    const int wantedColumns = columnsForWidth(getWidth());
    if (wantedColumns != visibleSteps)
    {
        const int oldColumns = visibleSteps;
        visibleSteps = wantedColumns;
        // Keep the first step of the old viewport in view when resizing.
        for (auto& ln : lane)
        {
            const int first = ln.page * oldColumns;
            ln.page = juce::jlimit(0, (ln.length - 1) / visibleSteps,
                                   first / visibleSteps);
            ln.displayX = -1.0f;
        }
    }
    vibes.resize(getWidth(), getHeight());
    processor.setEditorSize(getWidth(), getHeight());
    const int w = getWidth(), h = getHeight();
    const int clipTop = juce::roundToInt(h * 0.110f);
    const int clipBottom = juce::roundToInt(h * 0.918f);
    scrollViewport.setBounds(0, clipTop, w, clipBottom - clipTop);
    const int scrollOffset = juce::roundToInt(scrollY);
    // scrollContent is taller than the visible viewport. Parent clips every child.
    scrollContent.setBounds(0, -scrollOffset, w,
                            juce::roundToInt(h * 2.80f));
    // Header controls are grouped independently of the stretchable window.
    octaveDial.setBounds(headerBounds(0));
    keyDial.setBounds(headerBounds(1));
    scaleDial.setBounds(headerBounds(2));
    globalClockDial.setBounds(headerBounds(3));
    bendRangeSelect.setBounds(headerBounds(4));
    manualTempoToggle.setBounds(headerBounds(5));
    manualTempoInput.setBounds(headerBounds(6));
    addCCButton.setBounds(headerBounds(7));
    globalReverseButton.setBounds(w < 1220 ? 230 : 255,
                                 juce::roundToInt(h * 0.019f), 93,
                                 juce::jmax(21, juce::roundToInt(h * 0.026f)));

    const int programY = juce::roundToInt(h * 0.055f);
    for (int i = 0; i < 4; ++i)
        quickPrograms[static_cast<size_t>(i)].setBounds(
            globalReverseButton.getX() + i * 25, programY, 23,
            juce::jmax(19, juce::roundToInt(h * 0.027f)));
    if (popupOverlay != nullptr)
    {
        popupOverlay->setBounds(getLocalBounds());
        popupOverlay->toFront(false);
    }
    // Protected clusters: the credit has its own right-anchored bounds.
    // On narrow windows controls use the first footer row and the credit
    // the second row. Geometry never depends on the display aspect ratio.
    const bool stackedFooter = w < 1380;
    const float roomForControls = stackedFooter ? float(w) - 44.0f
        : footerCreditBounds().getX() - 54.0f;
    const float footerUnit = stackedFooter
        ? juce::jlimit(0.72f, 1.0f, roomForControls / 895.0f) : 1.0f;
    const float footerLeft = stackedFooter ? 24.0f : 30.0f;
    const float spareSpace = stackedFooter ? 0.0f :
        juce::jmax(0.0f, roomForControls - footerLeft - 873.0f * footerUnit);
    const int footerY = juce::roundToInt(h * (stackedFooter ? 0.930f : 0.941f));
    const int footerH = juce::jmax(20, juce::roundToInt(h * 0.029f));
    const auto footer = [=](juce::Component& c, float x, float width)
    {
        // Spread extra ultrawide space BETWEEN clusters, never inside them.
        const float distribute = x >= 837.0f ? spareSpace
            : x >= 563.0f ? spareSpace * (2.0f / 3.0f)
            : x >= 242.0f ? spareSpace / 3.0f : 0.0f;
        c.setBounds(juce::roundToInt(footerLeft + x * footerUnit + distribute),
                    footerY, juce::roundToInt(width * footerUnit), footerH);
    };
    footer(midiOutputSelect, 47, 138);
    footer(refreshMidiOutputs, 191, 39);
    footer(savePresetButton, 242, 101);
    footer(loadPresetButton, 348, 101);
    footer(presetFolderButton, 454, 96);
    footer(randomiseButton, 563, 106);
    footer(playButton, 675, 63);
    footer(resetButton, 745, 72);
    footer(settingsButton, 837, 36);
    darknessSlider.setBounds(0, 0, 0, 0); // Slider lives in Settings, not footer.
    // Theme moved into Settings to leave room for the new rotary dials.
    for (int row = 0; row < lanes; ++row)
    {
        // Native controls are positioned in the unscrolled content coordinate system.
        const float rowY = rowTop(row) + scrollY - clipTop;
        timings[static_cast<size_t>(row)].setBounds(0,0,0,0);
        divisions[static_cast<size_t>(row)].setBounds(0,0,0,0);
        // Separate compact dials fit between the step grid (ends at 0.828)
        // and the page buttons (begin at 0.939); no overlap at 1120px.
        clockDials[row].setBounds(juce::roundToInt(clockStartX()),
            juce::roundToInt(rowY + h*0.015f),
            70, juce::roundToInt(h*0.050f));
        timingDials[row].setBounds(juce::roundToInt(timingStartX()),
            juce::roundToInt(rowY + h*0.020f),
            54, juce::roundToInt(h*0.040f));
        prev[static_cast<size_t>(row)].setBounds(
            juce::roundToInt(workLeft() + 210.0f),
            juce::roundToInt(rowY + getHeight() * 0.051f),
            30, juce::roundToInt(getHeight() * 0.034f));
        removePage[static_cast<size_t>(row)].setBounds(
            juce::roundToInt(workLeft() + 210.0f), juce::roundToInt(rowY + getHeight() * 0.010f),
            30, juce::roundToInt(getHeight() * 0.029f));
        addPage[static_cast<size_t>(row)].setBounds(
            juce::roundToInt(pageStartX()), juce::roundToInt(rowY + getHeight() * 0.010f),
            34, juce::roundToInt(getHeight() * 0.029f));
        next[static_cast<size_t>(row)].setBounds(
            juce::roundToInt(pageStartX()),
            juce::roundToInt(rowY + getHeight() * 0.051f),
            34, juce::roundToInt(getHeight() * 0.034f));
        if (row >= 7)
        {
            const int slot = row - 7;
            ccLabelEditors[slot].setFont(laneLabelFont(h * 0.0152f));
            ccLabelEditors[slot].setBounds(juce::roundToInt(workLeft() + 43.0f),
                juce::roundToInt(rowY + h * 0.003f), 153, juce::roundToInt(h * 0.029f));
            // Delete lives in the LEFT GUTTER, outside the lane panel.
            deleteCC[slot].setBounds(juce::roundToInt(workLeft() + 3.0f),
                juce::roundToInt(rowY + h * 0.002f), 29, juce::roundToInt(h * 0.031f));
            // Compact role selectors remain in the upper (non-step) band.
            // One left-aligned, tightly grouped selector unit. The grid's
            // numbered header starts at rowY + .025h; leave >=6 px gap.
            const int selectX = juce::roundToInt(gridStartX());
            const int selectY = juce::roundToInt(rowY - h * 0.008f);
            const int selectH = juce::jmax(16, juce::roundToInt(h * 0.019f));
            laneTypeSelect[slot].setBounds(selectX, selectY, 126, selectH);
            ccSelect[slot].setBounds(selectX + 132, selectY, 82, selectH);
        }
        laneNameEditors[row].setFont(laneLabelFont(h * (row < 7 ? 0.018f : 0.015f)));
        laneNameEditors[row].setBounds(juce::roundToInt(workLeft() + 42.0f),
            juce::roundToInt(rowY + h * 0.003f), 160,
            juce::roundToInt(h * 0.033f));
    }
    const float programYInContent = programLaneTop() + scrollY - clipTop;
    programClockDial.setBounds(juce::roundToInt(clockStartX()),
        juce::roundToInt(programYInContent + h * 0.015f),
        70, juce::roundToInt(h * 0.050f));
    programTimingDial.setBounds(juce::roundToInt(timingStartX()),
        juce::roundToInt(programYInContent + h * 0.020f),
        54, juce::roundToInt(h * 0.040f));
    syncCCVisibility();
}
void PolyrhythmMIDIEditor::updateButtons()
{
    for (int row = 0; row < lanes; ++row)
    {
        const auto& l = lane[static_cast<size_t>(row)];
        prev[static_cast<size_t>(row)].setEnabled(l.page > 0);
        next[static_cast<size_t>(row)].setEnabled(l.page < (l.length - 1) / visibleSteps);
        addPage[static_cast<size_t>(row)].setEnabled(l.length < 128);
        removePage[static_cast<size_t>(row)].setEnabled(l.length > 16);
    }
}
void PolyrhythmMIDIEditor::setLength(int row, int length, bool follow)
{
    auto& l = lane[static_cast<size_t>(row)];
    l.length = juce::jlimit(1, 128, length);
    processor.setLaneLength(row, l.length);
    if (follow) l.page = (l.length - 1) / visibleSteps;
    else l.page = juce::jmin(l.page, (l.length - 1) / visibleSteps);
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
    // Header identity stays anchored at top left, regardless of window width.
    g.setColour(coral);
    g.fillRoundedRectangle(21.0f, h * 0.018f, 4.0f, h * 0.067f, 2.0f);
    g.setColour(teal); g.fillRect(33, juce::roundToInt(h * 0.090f), 44, 2);
    g.setColour(coral); g.fillRect(84, juce::roundToInt(h * 0.090f), 16, 2);
    g.setColour(lavender); g.fillRect(107, juce::roundToInt(h * 0.090f), 9, 2);
    // One cached Vibes effect, over the ENTIRE window behind native controls.
    // Generation is capped at 12 Hz in timerCallback; painting only composites.
    if (processor.getPsychedelicEnabled() && processor.getPsychedelicIntensity() > 0)
    {
        juce::Graphics::ScopedSaveState vibesState(g);
        vibes.paintBackground(g, getLocalBounds(), processor.getPsychedelicIntensity());
    }
    // Neon lane outlines are ordinary static graphics.
    g.saveState();
    g.reduceClipRegion(0, juce::roundToInt(h * 0.110f), w, juce::roundToInt(h * 0.815f));
    for (int row = 0; row < lanes; ++row)
    {
        if (!rowVisible(row)) continue;
        const float top = rowTop(row);
        if (top > h * 0.918f || top + h * 0.105f < h * 0.110f) continue;
        if (!g.getClipBounds().toFloat().intersects(
                juce::Rectangle<float>(0, top - h * 0.013f, static_cast<float>(w), h * 0.128f)))
            continue;
        auto& l = lane[static_cast<size_t>(row)];
        const auto colour = colours[row];
        g.setColour(paper.withAlpha(0.60f));
        g.fillRoundedRectangle(juce::Rectangle<float>(workLeft() + 15.0f, top - h * 0.012f,
                               workWidth() - 20.0f, h * 0.099f), 11.0f);
        // Static rail lives in the lane paint; the shared light renderer adds
        // the beat-boundary attack AFTER the darkness overlay.
        const auto accent = juce::Rectangle<float>(workLeft() + 25.0f, top - h * 0.005f,
                                                   4.0f, h * 0.085f);
        g.setColour(colour);
        g.fillRoundedRectangle(accent, 2.0f);
        // Lane names and sequence-length captions are drawn in the light pass.
        const auto length = lengthBox(row);
        // Fully coloured, steady length control: only the vertical rail pulses.
        g.setColour(colour.withAlpha(0.93f)); g.fillRoundedRectangle(length, 6.0f);
        g.setColour(colour.brighter(0.28f)); g.drawRoundedRectangle(length, 6.0f, 1.3f);
        // The counter-pulsing length value is drawn above the dimmer.
        // The four-step grouping is dark/light/dark/light, not an opacity
        // wash. Parity uses the absolute step so pagination stays consistent.
        for (int i = 0; i < visibleSteps; ++i)
        {
            const auto b = cell(row, i);
            const int stepIndex = l.page * visibleSteps + i;
            const auto strip = juce::Rectangle<float>(b.getX() - 3.0f,
                top + h * 0.016f, stepStride() - 2.0f, h * 0.074f);
            g.setColour(pnordeStepColumnBackground(paper, colour, stepIndex));
            g.fillRoundedRectangle(strip, 4.0f);
        }
        const int pageStart = l.page * visibleSteps;
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
            const float activity = index < PolyrhythmMIDIProcessor::maxSteps
                ? juce::jlimit(0.0f, 1.0f, l.rangeOpacity[index]) : 0.0f;
            const auto lit = paper.interpolatedWith(colour,
                darkTheme ? (0.32f + 0.30f * activity) : (0.19f + 0.22f * activity));
            g.setColour(!inRange ? ink.withAlpha(0.08f) : lit);
            g.fillRoundedRectangle(drawBounds, 5.0f);
            // All column gradation is behind the buttons, never inside them.
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
            const float previewX = cell(row, l.hovered).getRight() + 4.0f;
            g.setColour(colour.withAlpha(0.38f));
            const float dashes[] {3.0f, 3.0f};
            g.drawDashedLine(juce::Line<float>(previewX, top + h * 0.041f,
                previewX, top + h * 0.103f), dashes, 2, 1.5f);
        }
        if (l.page == (l.length - 1) / visibleSteps)
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
            g.drawText("L    C    R", juce::roundToInt(workLeft() + 116.0f),
                juce::roundToInt(top + h * 0.075f), 160,
                juce::roundToInt(h * 0.014f), juce::Justification::centred);
        }
        g.setColour(ink.withAlpha(0.76f)); g.setFont(font(h * 0.012f));
        g.drawText("PAGE " + juce::String(l.page + 1) + "/" +
                    juce::String((l.length - 1) / visibleSteps + 1),
                   juce::roundToInt(clockStartX()), juce::roundToInt(top + h * 0.073f),
                   75, juce::roundToInt(h * 0.012f),
                   juce::Justification::centred);
    }
    // PROGRAM is a real lane, not a separate post-darkness white overlay.
    // Its background and cells enter the same clip + darkness pipeline.
    if (processor.getProgramLaneEnabled()) paintProgramLane(g);
    g.restoreState();
    // Theme-coloured feathering at the scroll viewport edges.
    if (processor.getScrollFadesEnabled())
    {
        const float topEdge = h * 0.110f;
        const float bottomEdge = h * 0.918f;
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
    g.fillRect(0, juce::roundToInt(h * 0.918f), w, h - juce::roundToInt(h * 0.918f));
    g.setColour(coral.withAlpha(0.75f));
    g.fillRect(0, juce::roundToInt(h * 0.918f), w, 2);
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
    g.drawText("Pnorde", juce::Rectangle<int>{30, juce::roundToInt(h * 0.012f), 220,
        juce::roundToInt(h * 0.052f)}, juce::Justification::centredLeft, false);
    label("D R E A M   S E Q U E N C E R",
        {33, juce::roundToInt(h * 0.064f), 195, juce::roundToInt(h * 0.017f)},
        t.teal, h * 0.011f);
    const char* header[] {"GLOBAL OCTAVE", "KEY", "SCALE", "GLOBAL CLOCK",
                          "BEND RANGE", "MANUAL", "BPM", "ADD LANE"};
    for (int i = 0; i < 8; ++i)
    {
        const auto bounds = headerBounds(i);
        const bool narrow = w < 1220;
        const int captionHeight = juce::roundToInt(h * (narrow ? 0.016f : 0.020f));
        // Label X and width always come from the SAME rectangle as its dial.
        // The caption baseline is derived from the control's actual top, not
        // a separate position constant that can drift at responsive widths.
        const int gap = juce::roundToInt(h * (i < 4 ? 0.006f : 0.003f));
        const int captionY = bounds.getY() - captionHeight - gap;
        label(header[i], {bounds.getX(), captionY,
            bounds.getWidth(), captionHeight},
            t.teal, h * 0.0105f, true);
    }
    label(processor.isHostPlaying() ? "HOST PLAYING" : "HOST STOPPED",
        {w - 346, juce::roundToInt(h * 0.001f), 120, juce::roundToInt(h * 0.017f)},
        t.sage, h * 0.010f);
    {
        juce::Graphics::ScopedSaveState clip(g);
        g.reduceClipRegion(0, juce::roundToInt(h * 0.110f), w,
                           juce::roundToInt(h * 0.808f));
        for (int row = 0; row < lanes; ++row)
        {
            if (!rowVisible(row)) continue;
            const float top = rowTop(row);
            if (top + h * 0.110f < h * 0.11f || top > h * 0.918f) continue;
            const auto colour = colours[row];
            const auto& l = lane[row];
            // All titles are composed LAST, after the darkness/HPPD passes.
            // Painting them here caused washed-out text and rectangular holes.
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
                // Start markers are drawn in the last light pass below.
                if (l.activeStep == index && processor.isHostPlaying())
                {
                    g.setColour(colour.brighter(0.35f));
                    g.drawRoundedRectangle(rect.expanded(1.3f), 4.5f, 1.9f);
                }
            }
        }
    }
    // Label MIDI OUT from the actual component bounds (not stale percentages).
    const auto midiBounds = midiOutputSelect.getBounds();
    label("MIDI OUT", {midiBounds.getX() - 44, midiBounds.getY(), 42, midiBounds.getHeight()},
          t.teal, h * 0.0095f, true);
    // "Designed by" and the scalable signature share one bounding group.
    auto group = footerCreditBounds();
    const float logoWidth = juce::jmin(183.0f, group.getWidth() - 76.0f);
    const auto logo = group.removeFromRight(logoWidth);
    const auto designed = group.reduced(2.0f, 0.0f);
    g.setColour(t.pink.brighter(0.12f));
    g.setFont(font(h * 0.0112f, true));
    g.drawFittedText("Designed by", designed.toNearestInt(),
        juce::Justification::centredRight, 1);
    g.setColour(t.pink.brighter(0.22f));
    g.setFont(font(juce::jmin(h * 0.030f, logo.getWidth() * 0.145f), true));
    g.drawFittedText(juce::String::fromUTF8("LДNTERNS"), logo.toNearestInt(),
        juce::Justification::centredRight, 1);
    juce::Path resizeHint;
    const auto leftHandle = bottomLeftResizeBounds().toFloat();
    resizeHint.addLineSegment({leftHandle.getX() + 4.0f, leftHandle.getBottom() - 4.0f,
                               leftHandle.getRight() - 3.0f, leftHandle.getY() + 4.0f}, 1.2f);
    resizeHint.addLineSegment({leftHandle.getX() + 4.0f, leftHandle.getBottom() - 9.0f,
                               leftHandle.getRight() - 8.0f, leftHandle.getY() + 4.0f}, 1.2f);
    resizeHint.addLineSegment({leftHandle.getX() + 4.0f, leftHandle.getBottom() - 14.0f,
                               leftHandle.getRight() - 13.0f, leftHandle.getY() + 4.0f}, 1.2f);
    g.setColour(t.teal.withAlpha(0.85f));
    g.strokePath(resizeHint, juce::PathStrokeType(1.2f));
}

void PolyrhythmMIDIEditor::paintProgramLane(juce::Graphics& g)
{
    if (!processor.getProgramLaneEnabled()) return;
    const int h = getHeight(), w = getWidth();
    const float top = programLaneTop();
    if (top + h * 0.103f < h * 0.110f || top > h * 0.918f) return;
    juce::Graphics::ScopedSaveState clipped(g);
    g.reduceClipRegion(0, juce::roundToInt(h * 0.110f), w,
                       juce::roundToInt(h * 0.808f));
    const auto& t = themes[themeIndex];
    const auto colour = t.lavender;
    // Match the fixed lane rail, body, length-control and columns exactly.
    g.setColour(t.paper.withAlpha(0.60f));
    g.fillRoundedRectangle(workLeft() + 15.0f, top - h * 0.012f,
                           workWidth() - 20.0f, h * 0.099f, 11.0f);
    g.setColour(colour);
    g.fillRoundedRectangle(workLeft() + 25.0f, top - h * 0.005f,
                           4.0f, h * 0.085f, 2.0f);
    const juce::Rectangle<float> length(workLeft() + 47.0f,
                                         top + h * 0.049f, 137.0f, h * 0.034f);
    // The PROGRAM length control receives the same luminous fill treatment
    // as the coloured lane length boxes, with visible bloom in both themes.
    g.setColour(colour.brighter(0.85f).withAlpha(0.25f));
    g.fillRoundedRectangle(length.expanded(5.0f), 10.0f);
    g.setColour(colour.brighter(0.48f).withAlpha(0.98f));
    g.fillRoundedRectangle(length, 6.0f);
    g.setColour(colour.brighter(1.0f));
    g.drawRoundedRectangle(length, 6.0f, 1.7f);

    const int lengthValue = processor.getProgramLaneLength();
    const int current = processor.getProgramLaneCurrentStep();
    const float progress = processor.getProgramLaneProgress();
    for (int i = 0; i < visibleSteps; ++i)
    {
        const int index = programLanePage * visibleSteps + i;
        const auto bounds = programLaneCell(i);
        g.setColour(pnordeStepColumnBackground(t.paper, colour, index));
        g.fillRoundedRectangle(juce::Rectangle<float>(bounds.getX() - 3.0f,
            top + h * 0.016f, stepStride() - 2.0f, h * 0.074f), 4.0f);

        const int target = index < lengthValue ? processor.getProgramLaneValue(index) : 0;
        const bool active = index == current && processor.isHostPlaying();
        const bool hovered = i == programHoverStep;
        const bool alert = active && target > 0 && progress > 0.65f;
        // An ordinary playhead step is bright, irrespective of whether it
        // contains a change; the pending change receives extra-wide bloom.
        if (active || hovered)
        {
            g.setColour(colour.brighter(0.65f).withAlpha(active ? 0.28f : 0.19f));
            g.fillRoundedRectangle(bounds.expanded(active ? 8.0f : 5.0f), 11.0f);
        }
        if (alert && processor.getAnimationsEnabled())
        {
            const float flash = 0.60f + 0.40f * std::abs(std::sin(pulse * 5.2));
            g.setColour(colour.withAlpha(0.18f * flash));
            g.fillRoundedRectangle(bounds.expanded(13.0f), 14.0f);
            g.setColour(colour.withAlpha(0.49f * flash));
            g.fillRoundedRectangle(bounds.expanded(5.0f), 9.0f);
        }
        if (index < lengthValue)
        {
            g.setColour(colour.brighter(0.70f).withAlpha(target > 0 ? 0.38f : 0.22f));
            g.fillRoundedRectangle(bounds.expanded(target > 0 ? 4.5f : 2.0f), 8.0f);
        }
        g.setColour(index >= lengthValue ? t.ink.withAlpha(0.08f)
                    : target > 0 ? colour.brighter(0.54f) : colour.brighter(0.28f));
        g.fillRoundedRectangle(bounds, 5.0f);
        g.setColour(colour.brighter(active ? 1.05f : hovered ? 0.85f : 0.48f));
        g.drawRoundedRectangle(bounds, 5.0f, active ? 2.4f : hovered ? 2.0f : 1.3f);
    }
}

// The final text pass is shared with other lane titles/step numbers. Text is
// always legible in light/dark modes; unlike the old PROGRAM overlay, it does
// NOT draw opaque white backing panels above the darkness layer.
void PolyrhythmMIDIEditor::paintProgramLaneForeground(juce::Graphics& g)
{
    if (!processor.getProgramLaneEnabled()) return;
    const int h = getHeight(), w = getWidth();
    const float top = programLaneTop();
    if (top + h * 0.103f < h * 0.110f || top > h * 0.918f) return;
    juce::Graphics::ScopedSaveState clipped(g);
    g.reduceClipRegion(0, juce::roundToInt(h * 0.110f), w,
                       juce::roundToInt(h * 0.808f));
    const auto& t = themes[themeIndex];
    const auto colour = t.lavender;
    const bool darkPalette = t.paper.getPerceivedBrightness() < 0.50f;
    const auto readable = darkPalette || (processor.getExperimentalLightsEnabled()
            && processor.getDarknessPercent() >= 35)
        ? colour.interpolatedWith(juce::Colours::white, 0.74f)
        : colour.interpolatedWith(juce::Colours::black, 0.55f);
    g.setColour(readable);
    g.setFont(laneLabelFont(h * 0.018f));
    g.drawText("PROGRAM", juce::Rectangle<int>(juce::roundToInt(workLeft() + 39.0f),
        juce::roundToInt(top), 156, juce::roundToInt(h * 0.024f)),
        juce::Justification::centred);
    // Visible mode indication: an intentional manual selection disarms AUTO
    // without deleting any trigger values. Click or right-click to re-arm.
    const bool armed = processor.getProgramLaneAutoArmed();
    const auto statusColour = armed ? t.teal.brighter(0.38f) : readable;
    g.setColour(statusColour);
    g.setFont(font(h * 0.0105f, true));
    g.drawText(armed ? "AUTO" : "MANUAL", juce::Rectangle<int>(
        juce::roundToInt(workLeft() + 92.0f), juce::roundToInt(top + h*0.028f),
        82, juce::roundToInt(h * 0.018f)), juce::Justification::centred);
    const juce::Rectangle<float> length(workLeft() + 47.0f,
        top + h * 0.049f, 137.0f, h * 0.034f);
    g.setColour(juce::Colours::black.withAlpha(0.96f));
    g.setFont(font(h * 0.019f, true));
    g.drawText(juce::String(processor.getProgramLaneLength()),
        length.toNearestInt(), juce::Justification::centred);
    const int lengthValue = processor.getProgramLaneLength();
    const int current = processor.getProgramLaneCurrentStep();
    for (int i = 0; i < visibleSteps; ++i)
    {
        const int index = programLanePage * visibleSteps + i;
        const auto bounds = programLaneCell(i);
        const int target = index < lengthValue ? processor.getProgramLaneValue(index) : 0;
        const bool active = index == current && processor.isHostPlaying();
        const bool hovered = i == programHoverStep;
        // PROGRAM cells are re-illuminated after the experimental darkness
        // pass, matching the visible neon intensity of the fixed lane rows.
        if (index < lengthValue && processor.getExperimentalLightsEnabled()
            && processor.getDarknessPercent() >= 35)
        {
            g.setColour(colour.brighter(0.8f).withAlpha(target > 0 ? 0.19f : 0.10f));
            g.fillRoundedRectangle(bounds.expanded(4.0f), 8.0f);
            g.setColour(colour.brighter(target > 0 ? 0.54f : 0.28f));
            g.fillRoundedRectangle(bounds, 5.0f);
        }
        // A pending PROGRAM change emits a wide, high-contrast warning ring.
        // It belongs to the foreground light pass, so darkness cannot hide it.
        if (active && target > 0 && processor.getAnimationsEnabled()
            && processor.getProgramLaneProgress() > 0.65f)
        {
            const float flicker = 0.65f + 0.35f * std::abs(std::sin(pulse * 6.1));
            g.setColour(colour.brighter(0.6f).withAlpha(0.31f * flicker));
            g.drawRoundedRectangle(bounds.expanded(11.0f), 13.0f, 5.0f);
            g.setColour(colour.brighter(0.9f).withAlpha(0.94f * flicker));
            g.drawRoundedRectangle(bounds.expanded(3.0f), 7.0f, 2.6f);
        }
        if (active || hovered)
        {
            g.setColour(colour.brighter(0.6f).withAlpha(active ? 0.95f : 0.72f));
            g.drawRoundedRectangle(bounds.expanded(2.0f), 7.0f,
                                   active ? 2.4f : 1.4f);
            if (active)
            {
                const float progress = juce::jlimit(0.0f, 1.0f,
                    processor.getProgramLaneProgress());
                g.fillRoundedRectangle(bounds.getX() + 3.0f,
                    bounds.getBottom() - 6.0f,
                    juce::jmax(1.0f, (bounds.getWidth() - 6.0f) * progress),
                    3.0f, 1.5f);
            }
        }
        g.setColour(active || hovered ? readable.brighter(0.45f) : readable);
        g.setFont(font(h * 0.0135f, active));
        g.drawText(juce::String(index + 1), juce::Rectangle<int>(
            juce::roundToInt(bounds.getX()), juce::roundToInt(top + h * 0.025f),
            juce::roundToInt(bounds.getWidth()), juce::roundToInt(h * 0.019f)),
            juce::Justification::centred);
        g.setColour(index >= lengthValue ? readable.withAlpha(0.50f)
            : juce::Colours::black.withAlpha(0.94f));
        g.setFont(font(h * 0.0135f, true));
        g.drawText(index >= lengthValue ? "--" : target > 0
            ? "P" + juce::String(target) : "OFF",
            bounds.toNearestInt(), juce::Justification::centred);
        if (index == processor.getProgramLaneStart())
        {
            g.setColour(readable);
            g.fillRect(bounds.getX() - 2.5f, top + h * 0.034f,
                       2.0f, h * 0.056f);
        }
    }
    // PROGRAM pagination matches the other lanes, with neon filled controls.
    // Clock/timing are real draggable dials (laid out in resized()).
    const int lastPage = (lengthValue - 1) / visibleSteps;
    const auto drawPageAction = [&](float x, float y, int width, float height,
                                    const juce::String& symbol, bool enabled,
                                    juce::Colour actionColour)
    {
        const juce::Rectangle<float> button(x, y, static_cast<float>(width), height);
        g.setColour(actionColour.withAlpha(enabled ? 0.83f : 0.18f));
        g.fillRoundedRectangle(button, 5.0f);
        g.setColour(readable.withAlpha(enabled ? 1.0f : 0.35f));
        g.drawRoundedRectangle(button, 5.0f, 1.25f);
        g.setFont(font(h * 0.014f, true));
        g.drawText(symbol, button.toNearestInt(), juce::Justification::centred);
    };
    drawPageAction(workLeft() + 210.0f, top + h * 0.010f, 30,
        h * 0.029f, "-", lengthValue > 16, t.coral);
    drawPageAction(workLeft() + 210.0f, top + h * 0.051f, 30,
        h * 0.034f, "<", programLanePage > 0, colour);
    drawPageAction(pageStartX(), top + h * 0.010f, 34,
        h * 0.029f, "+", lengthValue < PolyrhythmMIDIProcessor::maxSteps, t.sage);
    drawPageAction(pageStartX(), top + h * 0.051f, 34,
        h * 0.034f, ">", programLanePage < lastPage, colour);
    g.setColour(readable);
    g.setFont(font(h * 0.011f, true));
    g.drawFittedText("PAGE " + juce::String(programLanePage + 1) + "/" +
        juce::String(lastPage + 1),
        juce::roundToInt(clockStartX() - 7.0f),
        juce::roundToInt(top + h * 0.072f), 84,
        juce::roundToInt(h * 0.019f), juce::Justification::centred, 1);
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
    // Editor-owned popup is drawn above the grid, with no transparent bleed.
    if (popupOverlay != nullptr && popupOverlay->isVisible())
        g.excludeClipRegion(popupOverlay->getBounds());
    // Parent paintOverChildren is composited after its children. Never repaint
    // over the CallOutBox, including its border, even if it overlaps lanes.
    if (settingsCallout != nullptr)
        g.excludeClipRegion(settingsCallout->getBounds());
    if (infoCallout != nullptr)
        g.excludeClipRegion(infoCallout->getBounds());
    if (directionCallout != nullptr)
        g.excludeClipRegion(directionCallout->getBounds());
    // TooltipWindow is an owned child in some hosts: protect its full opaque
    // rectangle from any subsequent foreground dimmer/HPPD pass.
    if (tooltipWindow.isShowing())
        g.excludeClipRegion(tooltipWindow.getBounds());

    // The lighting bus is restricted to the sequencer rectangle. Header,
    // footer, normal labels and native controls do not participate.
    if (processor.getExperimentalLightsEnabled())
    {
        juce::Graphics::ScopedSaveState laneClip(g);
        g.reduceClipRegion(0, juce::roundToInt(h * 0.110f), w,
                           juce::roundToInt(h * 0.808f));
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
                    if (top > h * 0.918f || top + h * 0.105f < h * 0.110f)
                        continue;
                    // Only editable optional-lane labels punch through the darkness layer.
                    if (row >= 7 && laneNameEditors[row].hasKeyboardFocus(true))
                        protectControl(laneNameEditors[row]);
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
                           juce::roundToInt(h * 0.808f));
            }
            // Overlay only the two small luminous dial faces after darkness.
            // No excluded rectangular regions, no animated control bloom.
            for (int row = 0; row < lanes; ++row)
            {
                if (!rowVisible(row)) continue;
                const float top = rowTop(row);
                if (top > h * 0.918f || top + h * 0.105f < h * 0.110f) continue;
                const int translateY = juce::roundToInt(h * 0.110f - scrollY);
                const auto a = clockDials[row].getBounds();
                const auto b = timingDials[row].getBounds();
                clockDials[row].drawIllumination(g, (float)a.getX(), (float)(a.getY()+translateY));
                timingDials[row].drawIllumination(g, (float)b.getX(), (float)(b.getY()+translateY));
            }
            if (processor.getProgramLaneEnabled())
            {
                const int translateY = juce::roundToInt(h * 0.110f - scrollY);
                const auto clock = programClockDial.getBounds();
                const auto timing = programTimingDial.getBounds();
                programClockDial.drawIllumination(g, static_cast<float>(clock.getX()),
                    static_cast<float>(clock.getY() + translateY));
                programTimingDial.drawIllumination(g, static_cast<float>(timing.getX()),
                    static_cast<float>(timing.getY() + translateY));
            }
            // Page buttons are native children painted BEFORE the dimmer. Restore
            // crisp, low-cost neon faces AFTER it, without full-screen blur passes.
            for (int row = 0; row < lanes; ++row)
            {
                if (!rowVisible(row)) continue;
                const int translateY = juce::roundToInt(h * 0.110f - scrollY);
                const auto drawPage = [&](const PnordePageButton& btn, juce::Colour accent)
                {
                    if (!btn.isVisible()) return;
                    auto b = btn.getBounds().toFloat().translated(0.0f, (float) translateY).reduced(1.2f);
                    if (b.getBottom() < h * 0.110f || b.getY() > h * 0.918f) return;
                    const float enabled = btn.isEnabled() ? 1.0f : 0.42f;
                    const float hover = btn.isMouseOver() ? 1.0f : 0.0f;
                    g.setColour(accent.brighter(0.25f + 0.28f * hover)
                        .withAlpha((0.66f + 0.27f * hover) * enabled));
                    g.fillRoundedRectangle(b, 4.0f);
                    g.setColour(accent.brighter(0.88f)
                        .withAlpha((0.84f + 0.16f * hover) * enabled));
                    g.drawRoundedRectangle(b, 4.0f, hover > 0.0f ? 2.1f : 1.4f);
                    // Contrast is derived from the luminous fill, never from
                    // the darkened text colour of native child buttons.
                    g.setColour(enabled < 0.5f ? accent.brighter(0.25f).withAlpha(0.62f)
                        : (accent.getPerceivedBrightness() > 0.50f
                             ? juce::Colour(0xff102133) : juce::Colours::white));
                    g.setFont(font(juce::jmax(12.0f, b.getHeight() * 0.69f), true));
                    g.drawText(btn.getButtonText(), b.toNearestInt(), juce::Justification::centred);
                };
                drawPage(prev[row], colours[row]);
                drawPage(next[row], colours[row]);
                drawPage(addPage[row], palette.sage);
                drawPage(removePage[row], palette.coral);
            }
        }
        const bool playingLights = processor.getAnimationsEnabled() && processor.isHostPlaying();
        const bool glowEnabled = processor.getVisualGlowEnabled();
        for (int row = 0; row < lanes; ++row)
        {
            if (!rowVisible(row)) continue;
            const float top = rowTop(row);
            if (top > h * 0.918f || top + h * 0.105f < h * 0.110f) continue;
            if (!g.getClipBounds().toFloat().intersects(
                    juce::Rectangle<float>(0, top - h * 0.013f,
                                           static_cast<float>(w), h * 0.128f)))
                continue;
            const auto& l = lane[static_cast<size_t>(row)];
            const auto colour = colours[row];
            const float led = l.stepLed;
            // Cached exponential coefficients: only the continuous phase
            // attenuation needs a single exp() per row, not per step cell.
            constexpr float behindWeights[] { 1.0f, 0.24659696f, 0.06081006f,
                                               0.01499558f, 0.00369786f };
            constexpr float aheadWeights[] { 1.0f, 0.16529889f, 0.02732372f };
            const float phaseAttenuation = std::exp(-1.4f * l.stepPhase);
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
                        (behind < 5 ? behindWeights[behind] * phaseAttenuation : 0.0f)
                        + (ahead < 3 ? 0.13f * aheadWeights[ahead] : 0.0f));
                const float motion = processor.getAnimationsEnabled() ? trail : 0.0f;
                const bool gateOn = row == 0 && processor.getStepEnabled(index);
                const float beatFlash = processor.isHostPlaying()
                    ? l.rangeOpacity[index] : 0.0f;
                const float editFlash = l.interactionFlash[index];
                const float hoverLift = l.hoverStep == i ? 0.23f : 0.0f;
                const float glowStrength = processor.getHypnosis() / 100.0f;
                const float activeFlash = juce::jlimit(0.0f, 1.0f,
                    juce::jmax(beatFlash, editFlash) + hoverLift);
                if (glowEnabled && glowStrength > 0.01f && activeFlash > 0.04f &&
                    (row != 0 || gateOn))
                    PnordeLights::cellHalo(g, bounds, colour,
                        activeFlash, darkness, glowStrength);
                PnordeLights::cellBackdrop(g, bounds, colour, darkness,
                    juce::jlimit(0.0f, 1.0f, motion + activeFlash * glowStrength * 0.55f), gateOn);
                if (row == 0)
                {
                    if (gateOn)
                        PnordeLights::cellFill(g, bounds.reduced(1.6f), colour,
                            darkness, motion, current == index ? juce::jmax(led, activeFlash * glowStrength) :
                                editFlash * glowStrength);
                    else if (editFlash > 0.04f || hoverLift > 0.01f)
                        PnordeLights::outline(g, bounds.reduced(2.0f), colour,
                            juce::jlimit(0.0f, 1.0f, (editFlash + hoverLift) * glowStrength), darkness);
                }
                else if (!isLaneType(row, PnordeLane::Type::midiChannel))
                {
                    const float f = juce::jlimit(0.0f, 1.0f, l.visualFill[index]);
                    const auto fill = parameterFillBounds(row, bounds, f, processor.getOptionalLaneType(row));
                    PnordeLights::cellFill(g, fill, colour, darkness,
                        motion, current == index ? juce::jmax(led, activeFlash * glowStrength) :
                                editFlash * glowStrength);
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
                {workLeft() + 25.0f, top - h * 0.005f, 4.0f, h * 0.085f},
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
                        ? juce::jlimit(0.0f, 1.0f, 0.72f + led * 0.50f
                              + l.rangeOpacity[index] * (0.12f + processor.getHypnosis() / 150.0f))
                        : 0.23f;
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
            // PAGE captions must remain readable after the darkness mask.
            if (darkness >= 0.35f)
            {
                g.setColour(juce::Colours::white.withAlpha(0.93f));
                g.setFont(font(h * 0.012f, true));
                g.drawText("PAGE " + juce::String(l.page + 1) + "/" +
                    juce::String((l.length - 1) / visibleSteps + 1),
                    juce::roundToInt(clockStartX()), juce::roundToInt(top + h * 0.073f),
                    75, juce::roundToInt(h * 0.012f), juce::Justification::centred);
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
        g.fillRoundedRectangle(workLeft() + 18.0f, y, workWidth() - 30.0f, 3.0f, 1.5f);
    }
    // SLIDE is intentionally visible and interactive in microtonal mode:
    // its saved 0..100% values set the tempo-relative pitch approach time.
    // Contextual help only while a sequence-length box is pressed/dragged.
    // The lane title remains uncluttered when the control is idle.
    for (int row = 0; row < lanes; ++row)
    {
        if (!rowVisible(row) || !lane[row].lengthDrag) continue;
        const auto b = lengthBox(row);
        const float helpW = juce::jmin(210.0f, 220.0f);
        const float x = juce::jlimit(8.0f, w - helpW - 8.0f, b.getX());
        const float y = juce::jlimit(h * 0.115f, h * 0.918f - 28.0f,
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
    // Translucent neon over the ENTIRE editor, AFTER darkness and lighting;
    // clipped around settings. 25% hard cap preserves control legibility.
    if (processor.getPsychedelicEnabled() && processor.getVibesBleed() > 0)
    {
        juce::Graphics::ScopedSaveState vibesState(g);
        vibes.paintOverlay(g, getLocalBounds(), processor.getVibesBleed(),
                           processor.getPsychedelicIntensity());
    }
    // Final emission layer: titles are never dimmed, even with HPPD disabled
    // or with darkness at 75%. No opaque label rectangles / highlight holes.
    {
        juce::Graphics::ScopedSaveState titlesClip(g);
        g.reduceClipRegion(0, juce::roundToInt(h * 0.110f), w,
                           juce::roundToInt(h * 0.808f));
        for (int row = 0; row < lanes; ++row)
        {
            if (!rowVisible(row) ||
                (row >= 7 && laneNameEditors[row].hasKeyboardFocus(true))) continue;
            const float y = rowTop(row);
            if (y + h * 0.034f < h * 0.110f || y >= h * 0.918f) continue;
            const float nameX = lengthBox(row).getX() - 8.0f;
            const float nameW = lengthBox(row).getWidth() + 16.0f;
            const juce::Rectangle<int> nameBounds(juce::roundToInt(nameX),
                juce::roundToInt(y + h * 0.002f), juce::roundToInt(nameW),
                juce::roundToInt(h * 0.034f));
            const auto shade = colours[row];
            g.setFont(laneLabelFont(h * (row < 7 ? 0.018f : 0.015f)));
            const auto title = laneDisplayName(row).toUpperCase();
            // On light themes, pastel-on-cream is too low-contrast. Deepen
            // the lane accent while keeping the animated luminous treatment
            // for genuinely dimmed and inherently dark themes.
            const bool darkSurface = palette.paper.getPerceivedBrightness() < 0.5f;
            const float dim = processor.getExperimentalLightsEnabled()
                ? juce::jlimit(0.0f, 1.0f, processor.getDarknessPercent() / 55.0f)
                : 0.0f;
            const auto inkForLight = shade.interpolatedWith(juce::Colours::black, 0.46f);
            const auto inkForDark = shade.interpolatedWith(juce::Colours::white, 0.44f);
            const auto emit = darkSurface ? inkForDark
                                          : inkForLight.interpolatedWith(inkForDark, dim);
            if (processor.getExperimentalLightsEnabled() && (darkSurface || dim > 0.1f))
            {
                g.setColour(emit.withAlpha(0.28f * (darkSurface ? 1.0f : dim)));
                g.drawText(title, nameBounds.translated(-1, 0), juce::Justification::centred);
                g.drawText(title, nameBounds.translated(1, 0), juce::Justification::centred);
            }
            g.setColour(emit.withAlpha(0.98f));
            g.drawText(title, nameBounds, juce::Justification::centred);
            const int startIndex = processor.getLaneStartStep(row);
            const int localIndex = startIndex - lane[row].page * visibleSteps;
            if (localIndex >= 0 && localIndex < visibleSteps &&
                startIndex < processor.getLaneLength(row))
            {
                const auto b = cell(row, localIndex);
                // Small right-pointing play arrow above the cell and a vertical
                // start line on its left edge; neither enters the label gutter.
                const float x = b.getX() - 2.0f;
                const float y = b.getY() - h * 0.016f;
                g.setColour(emit.withAlpha(0.85f));
                g.fillRoundedRectangle(x, y, 2.4f, b.getBottom() - y, 1.0f);
                juce::Path icon;
                icon.addTriangle(x + 3.0f, y - 3.0f, x + 3.0f, y + 3.0f,
                                 x + 9.0f, y);
                g.setColour(emit);
                g.fillPath(icon);
            }
        }
    }
    if (processor.getProgramLaneEnabled()) paintProgramLaneForeground(g); // Text over darkness.
    // Animated neon identity while hovered: two gradients stay clipped to
    // their own text bounds and never touch MIDI controls or popup surfaces.
    if (logoHover || creditHover)
    {
        const float phase = static_cast<float>(std::sin(huePhase) * 0.5 + 0.5);
        const auto first = palette.coral.interpolatedWith(palette.teal, phase);
        const auto second = palette.lavender.interpolatedWith(palette.amber, 1.0f - phase);
        const auto third = palette.cyan.interpolatedWith(palette.pink, phase);
        if (logoHover)
        {
            const auto bounds = juce::Rectangle<int>(30, juce::roundToInt(h * 0.012f),
                220, juce::roundToInt(h * 0.052f));
            static const auto face = juce::Typeface::createSystemTypefaceFor(
                AudiowideEmbedded::fontData, AudiowideEmbedded::fontSize);
            auto textFont = face != nullptr
                ? juce::Font(juce::FontOptions(face).withHeight(h * 0.038f))
                : font(h * 0.038f, true);
            textFont.setHorizontalScale(1.10f);
            g.setFont(textFont);
            g.setColour(first.withAlpha(0.26f));
            g.drawText("Pnorde", bounds.translated(-1, 0), juce::Justification::centredLeft);
            g.drawText("Pnorde", bounds.translated(1, 0), juce::Justification::centredLeft);
            juce::ColourGradient gradient(first, 30.0f, 0.0f, second, 248.0f, 0.0f, false);
            gradient.addColour(0.55, third);
            g.setGradientFill(gradient);
            g.drawText("Pnorde", bounds, juce::Justification::centredLeft);
        }
        if (creditHover)
        {
            auto group = footerCreditBounds();
            group = group.removeFromRight(juce::jmin(183.0f, group.getWidth() - 69.0f));
            const auto bounds = group.toNearestInt();
            g.setFont(font(juce::jmin(h * 0.030f, group.getWidth() * 0.145f), true));
            const auto word = juce::String::fromUTF8("LДNTERNS");
            g.setColour(first.withAlpha(0.28f));
            g.drawFittedText(word, bounds.translated(-1, 0), juce::Justification::centredRight, 1);
            g.drawFittedText(word, bounds.translated(1, 0), juce::Justification::centredRight, 1);
            juce::ColourGradient gradient(second, group.getX(), 0.0f,
                first, group.getRight(), 0.0f, false);
            gradient.addColour(0.53, third);
            g.setGradientFill(gradient);
            g.drawFittedText(word, bounds, juce::Justification::centredRight, 1);
        }
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
        const int visibleIndex = ccPopupStep - lane[static_cast<size_t>(ccPopupRow)].page * visibleSteps;
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
    if (bottomLeftResizeBounds().contains(e.getPosition()))
    {
        bottomLeftResizeActive = true;
        bottomLeftResizeOrigin = e.getPosition();
        return;
    }
    if ((e.mods.isCtrlDown() || e.mods.isCommandDown()) && e.mods.isLeftButtonDown())
    {
        const auto pt = e.getEventRelativeTo(this).position;
        for (int row = 0; row < lanes; ++row)
        {
            if (!rowVisible(row)) continue;
            const auto label = juce::Rectangle<float>(workLeft() + 42.0f,
                rowTop(row) - getHeight()*0.001f, 160.0f, getHeight()*0.034f);
            if (label.contains(pt)) { showLaneDirectionPanel(row); return; }
        }
    }
    if (e.mods.isPopupMenu())
    {
        const auto pt=e.getEventRelativeTo(this).position;
        if (processor.getProgramLaneEnabled() &&
            pt.y >= programLaneTop() - getHeight()*0.008f &&
            pt.y < programLaneTop() + getHeight()*0.099f)
        { showProgramLaneMenu(); return; }
        for (int row=0; row<lanes; ++row)
        {
            if (!rowVisible(row)) continue;
            const auto labelRect=juce::Rectangle<float>(workLeft()+42.0f,
                rowTop(row)-getHeight()*0.001f,160.0f,getHeight()*0.034f);
            if (labelRect.contains(pt)) {showLaneDirectionMenu(row); return;}
        }
        return;
    }
    if (!e.mods.isLeftButtonDown()) return;
    // For events forwarded from the editable Macro label, coordinates are
    // relative to the editor, not to the TextEditor child.
    const auto local = e.getEventRelativeTo(this).position;
    // Dedicated PROGRAM lane: normal click cycles OFF->P1->P2->P3->P4;
    // Ctrl/Cmd-click a cell sets the start marker. Right-click offers bounded
    // length / clock / timing / navigation controls. No MIDI output here.
    if (processor.getProgramLaneEnabled() &&
        local.y >= programLaneTop() - getHeight() * 0.008f &&
        local.y < programLaneTop() + getHeight() * 0.099f)
    {
        if (e.mods.isPopupMenu()) { showProgramLaneMenu(); return; }
        if (e.mods.isLeftButtonDown())
        {
            if (local.x >= workLeft() + 92.0f && local.x <= workLeft() + 174.0f
                && local.y >= programLaneTop() + getHeight() * 0.027f
                && local.y <= programLaneTop() + getHeight() * 0.047f)
            {
                processor.setProgramLaneAutoArmed(
                    !processor.getProgramLaneAutoArmed());
                repaint(); return;
            }
            const float top = programLaneTop(), hh = static_cast<float>(getHeight());
            const bool atTop = local.y >= top + hh * 0.010f &&
                local.y <= top + hh * 0.039f;
            const bool atBottom = local.y >= top + hh * 0.051f &&
                local.y <= top + hh * 0.085f;
            if (local.x >= workLeft() + 210.0f && local.x < workLeft() + 240.0f)
            {
                if (atTop && processor.getProgramLaneLength() > 16)
                {
                    processor.setProgramLaneLength(
                        juce::jmax(16, processor.getProgramLaneLength() - 16));
                    programLanePage = juce::jmin(programLanePage,
                        (processor.getProgramLaneLength() - 1) / visibleSteps);
                }
                else if (atBottom) programLanePage = juce::jmax(0, programLanePage - 1);
                repaint(); return;
            }
            if (local.x >= pageStartX() && local.x < pageStartX() + 34.0f)
            {
                if (atTop && processor.getProgramLaneLength() < 128)
                    processor.setProgramLaneLength(
                        juce::jmin(128, processor.getProgramLaneLength() + 16));
                else if (atBottom)
                    programLanePage = juce::jmin((processor.getProgramLaneLength() - 1)
                        / visibleSteps, programLanePage + 1);
                repaint(); return;
            }
            if (local.x > workLeft()+47.0f && local.x < workLeft()+184.0f)
            { showProgramLaneMenu(); return; }
            for (int i = 0; i < visibleSteps; ++i)
            {
                const int index = programLanePage * visibleSteps + i;
                if (index >= processor.getProgramLaneLength()) continue;
                if (!programLaneCell(i).contains(local)) continue;
                if (e.mods.isCtrlDown() || e.mods.isCommandDown())
                    processor.setProgramLaneStart(index);
                else processor.setProgramLaneValue(index,
                    (processor.getProgramLaneValue(index) + 1) % 5);
                repaint(); return;
            }
        }
        return;
    }
    // Clickable identity text: dismiss-on-outside-click, with no overlay timers.
    if (e.eventComponent == this)
    {
        const auto w = static_cast<float>(getWidth());
        const auto h = static_cast<float>(getHeight());
        const juce::Rectangle<float> logo(20.0f, h*0.010f, 220.0f, h*0.080f);
        const auto credit = footerCreditBounds();
        const auto& t = themes[themeIndex];
        if (logo.contains(local) || credit.contains(local))
        {
            const bool bandcamp = credit.contains(local);
            auto content = std::make_unique<PnordeInfoPanel>(
                bandcamp ? PnordeInfoPanel::Kind::bandcamp
                         : PnordeInfoPanel::Kind::about,
                t.paper, t.ink, t.teal);
            openContainedPanel(std::move(content),
                (bandcamp ? credit : logo).toNearestInt());
            return;
        }
    }
    dragFromRow = -1;
    dragTargetRow = -1;
    laneDragActive = false;
    if (processor.getExperimentalReorderEnabled() &&
        local.y >= getHeight() * 0.110f && local.y < getHeight() * 0.918f)
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
    if (local.y < getHeight() * 0.110f || local.y >= getHeight() * 0.918f) return;
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
        // Ctrl (Windows/Linux) or Cmd (macOS) + step NUMBER selects a start
        // within the CURRENT lane length, without changing sequence length.
        // The number hit has priority over the draggable end-of-lane marker.
        if (e.mods.isCtrlDown() || e.mods.isCommandDown())
        {
            for (int i = 0; i < visibleSteps; ++i)
            {
                const auto b = cell(row, i);
                const int absoluteStep = l.page * visibleSteps + i;
                const juce::Rectangle<float> numberHit(b.getX(),
                    rowTop(row) + getHeight() * 0.027f,
                    b.getWidth(), getHeight() * 0.023f);
                if (numberHit.contains(e.position))
                {
                    if (absoluteStep < l.length)
                    {
                        processor.setLaneStartStep(row, absoluteStep);
                        repaint();
                    }
                    return; // Never shorten the lane via a modified click.
                }
            }
        }
        // Clicking the tiny play-arrow region resets a custom start to 1.
        // The ordinary number click outside the arrow still changes length.
        if (!(e.mods.isCtrlDown() || e.mods.isCommandDown()))
        {
            const int localStart = processor.getLaneStartStep(row) - l.page * visibleSteps;
            if (localStart >= 0 && localStart < visibleSteps)
            {
                const auto b = cell(row, localStart);
                const juce::Rectangle<float> startHit(b.getX() - 9.0f,
                    b.getY() - getHeight() * 0.024f, 14.0f,
                    getHeight() * 0.023f);
                if (startHit.contains(e.position))
                {
                    processor.setLaneStartStep(row, 0);
                    repaint();
                    return;
                }
            }
        }
        if (l.page == (l.length - 1) / visibleSteps && markerBox(row).contains(e.position))
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
                setLength(row, l.page * visibleSteps + i + 1, false);
                l.rippleX = markerX(row);
                l.rippleProgress = 0.0f;
                return;
            }
        }
        for (int i = 0; i < visibleSteps; ++i)
        {
            const int index = l.page * visibleSteps + i;
            if (index >= l.length || !cell(row, i).contains(e.position)) continue;
            l.interactionFlash[index] = 1.0f;
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
    l.interactionFlash[step] = 1.0f;
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
    if (bottomLeftResizeActive)
    {
        const auto delta = e.getPosition() - bottomLeftResizeOrigin;
        if (auto* constrainer = getConstrainer())
            setSize(juce::jlimit(constrainer->getMinimumWidth(), constrainer->getMaximumWidth(), getWidth() - delta.x),
                    juce::jlimit(constrainer->getMinimumHeight(), constrainer->getMaximumHeight(), getHeight() + delta.y));
        bottomLeftResizeOrigin = e.getPosition();
        return;
    }
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
                    const int length = l.page * visibleSteps + i + 1;
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
            const int base = l.page * visibleSteps;
            int best = base + 1;
            float distance = std::abs(e.position.x - markerXFor(row, best));
            for (int i = 2; i <= visibleSteps; ++i)
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
    bottomLeftResizeActive = false;
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
    const auto point = e.getEventRelativeTo(this).position;
    const bool overLogo = juce::Rectangle<float>(20.0f, getHeight() * 0.010f,
        220.0f, getHeight() * 0.080f).contains(point);
    const bool overCredit = footerCreditBounds().contains(point);
    if (logoHover != overLogo || creditHover != overCredit)
    {
        logoHover = overLogo; creditHover = overCredit;
        repaint();
    }
    if (bottomLeftResizeBounds().contains(e.getPosition()))
    {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        return;
    }
    bool overRail = false;
    if (processor.getExperimentalReorderEnabled())
        for (int row = 7; row < lanes; ++row)
            if (rowVisible(row) && reorderHandle(row).contains(e.position))
                { overRail = true; break; }
    setMouseCursor(overRail ? juce::MouseCursor::DraggingHandCursor
                            : juce::MouseCursor::NormalCursor);
    programHoverStep = -1;
    if (processor.getProgramLaneEnabled())
        for (int i = 0; i < visibleSteps; ++i)
            if (programLanePage * visibleSteps + i < processor.getProgramLaneLength()
                && programLaneCell(i).contains(point))
            { programHoverStep = i; break; }
    for (int row = 0; row < lanes; ++row)
    {
        auto& l = lane[static_cast<size_t>(row)];
        l.hovered = -1;
        l.hoverStep = -1;
        if (!rowVisible(row)) continue;
        for (int i = 0; i < visibleSteps; ++i)
        {
            const auto bounds = cell(row, i);
            if (l.page * visibleSteps + i < l.length && bounds.contains(e.position))
                l.hoverStep = i;
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
    logoHover = creditHover = false;
    programHoverStep = -1;
    for (auto& l : lane) { l.hovered = -1; l.hoverStep = -1; }
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
            if (e.mods.isAltDown() || e.mods.isShiftDown())
            {
                processor.setLaneStartStep(row, step);
                repaint();
                return;
            }
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
    if (lastDisplayedProgram != processor.getSelectedProgram())
    {
        syncFromProcessor();
        refreshProgramButtons();
        resized();
    }
    const bool reverseOn = processor.getGlobalReverse();
    if (globalReverseButton.getButtonText() != (reverseOn ? "REV: ON" : "REV: OFF"))
    {
        globalReverseButton.setButtonText(reverseOn ? "REV: ON" : "REV: OFF");
        globalReverseButton.setColour(juce::TextButton::buttonColourId,
            reverseOn ? themes[themeIndex].coral.brighter(0.25f) : themes[themeIndex].paper);
    }
    const bool playing = processor.isHostPlaying();
    const bool animate = processor.getAnimationsEnabled();
    const bool experimentalLights = processor.getExperimentalLightsEnabled();
    const int baseHz = experimentalLights ? 30 : 24;
    const bool hppdRunning = processor.getPsychedelicEnabled()
        && processor.getPsychedelicIntensity() > 0 && playing;
    const int requestedFps = processor.getHppdFps();
    const int desiredHz = hppdRunning
        ? juce::jmin(60, juce::jmax(baseHz, requestedFps * 2)) : baseHz;
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
    if (logoHover || creditHover) { huePhase += 0.085; shouldRepaint = true; }
    bool fullRepaint = shouldRepaint;
    lastTransportPlaying = playing;
    const auto transportCaption=processor.getPlayEnabled()?"PAUSE":"PLAY";
    if (playButton.getButtonText()!=transportCaption)
        playButton.setButtonText(transportCaption);
    if (animate && playing && experimentalLights) { pulse += 0.14; shouldRepaint = true; }
    else if (animate && playing && processor.getProgramLaneEnabled())
        pulse += 0.14; // PROGRAM trigger flash also works without dark mode.
    // HPPD frames: independently capped at user-selected 1-60 FPS on GUI thread.
    // One active renderer, one <= 1.5 MiB image; off means zero effect buffers.
    const bool vibesActive = processor.getPsychedelicEnabled()
        && processor.getPsychedelicIntensity() > 0;
    const int vibesMode = vibesActive ? processor.getVibesMode() + 1 : 0;
    const int vibesIntensity = processor.getPsychedelicIntensity();
    const bool vibesChanged = vibesMode != lastVibesMode
        || vibesIntensity != lastVibesIntensity || vibesActive != lastVibesEnabled;
    // The scheduled frame deadline avoids quantising 40 FPS down to 30 on a
    // 60 Hz GUI timer. Missing a deadline skips frames rather than catching up
    // in a burst, so slow computers cannot accumulate a rendering backlog.
    const double frameIntervalMs = 1000.0 / requestedFps;
    const bool vibesFrame = vibesActive && nowMs + 1.0 >= nextVibesFrameMs
        && (playing || !vibes.hasFrame());
    if (vibesChanged)
    {
        vibes.setMode(vibesMode); // releases inactive subclass and image
        lastVibesMode = vibesMode;
        lastVibesIntensity = vibesIntensity;
        lastVibesEnabled = vibesActive;
        shouldRepaint = true;
        fullRepaint = true;
    }
    if (vibesFrame)
    {
        vibes.update(getWidth(), getHeight(), processor);
        if (nextVibesFrameMs <= 0.0 || nowMs - nextVibesFrameMs > frameIntervalMs)
            nextVibesFrameMs = nowMs + frameIntervalMs;
        else
            nextVibesFrameMs += frameIntervalMs;
        shouldRepaint = true;
    }
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
    octaveDial.setIndex(processor.getGlobalOctave()+3);
    keyDial.setIndex(processor.getKey());
    scaleDial.setIndex(processor.getScale());
    globalClockDial.setIndex(processor.getGlobalClockIndex());
    for (int row = 0; row < lanes; ++row)
        if (rowVisible(row))
        {
            clockDials[row].setValue(processor.getLaneDivision(row));
            timingDials[row].setValue(processor.getLaneTiming(row));
        }
    if (processor.getProgramLaneEnabled())
    {
        programClockDial.setValue(processor.getProgramLaneDivision());
        programTimingDial.setValue(processor.getProgramLaneTiming());
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

    // PROGRAM receives the same lightweight dirty-row redraw cadence as
    // musical lanes; it previously advanced invisibly without full repaints.
    bool programLaneDirty = false;
    if (processor.getProgramLaneEnabled())
    {
        const int step = processor.getProgramLaneCurrentStep();
        programLaneDirty = step != lastProgramPlayheadStep;
        lastProgramPlayheadStep = step;
        const bool warning = playing && animate && step >= 0
            && processor.getProgramLaneValue(step) > 0
            && processor.getProgramLaneProgress() > 0.65f;
        if (warning) programLaneDirty = true;
        if (programLaneDirty) shouldRepaint = true;
    }
    else lastProgramPlayheadStep = -2;
    // Dirty-row tracking avoids repainting untouched rows when lights are off.
    std::array<bool, lanes> rowDirty {};

    // Work only on visible rows and on their current 16-step page. Previously
    // this loop read and animated all 2,688 cells at 60 fps, even offscreen.
    for (int row = 0; row < lanes; ++row)
    {
        if (!rowVisible(row)) continue;
        auto& l = lane[static_cast<size_t>(row)];
        const int length = processor.getLaneLength(row);
        if (l.length != length) { l.length = length; shouldRepaint = true; rowDirty[static_cast<size_t>(row)] = true; }
        l.division = processor.getLaneDivision(row);
        const int step = processor.getLaneCurrentStep(row);
        if (l.activeStep != step) { l.activeStep = step; shouldRepaint = true; rowDirty[static_cast<size_t>(row)] = true; }
        const float top = rowTop(row);
        if (row != 0 && (top > getHeight() * 0.918f ||
                         top + getHeight() * 0.105f < getHeight() * 0.110f))
            continue;
        // A step is a SHORT pulse when first entered, not an endlessly
        // illuminated cell for the entire duration of a slow division.
        if (playing && l.activeStep != l.lastLedStep)
        {
            if (l.activeStep >= 0 && l.activeStep < 128)
                l.rangeOpacity[l.activeStep] = 1.0f;
            l.lastLedStep = l.activeStep;
        }
        if (!playing) l.lastLedStep = -1;
        const float visualDecay = std::exp(-6.8f / juce::jmax(1, animationTimerHz));
        const int first = juce::jlimit(0, 127, l.page * visibleSteps);
        for (int i = first; i < juce::jmin(128, first + visibleSteps); ++i)
        {
            // The currently playing step remains highlighted; previous steps
            // fade smoothly to their constant idle backlight, in both modes.
            const float previous = l.rangeOpacity[i];
            const float activity = previous * visualDecay;
            l.rangeOpacity[i] = activity < 0.012f ? 0.0f : activity;
            const float previousEdit = l.interactionFlash[i];
            l.interactionFlash[i] = previousEdit < 0.014f ? 0.0f : previousEdit * visualDecay;
            if (previousEdit > 0.014f)
            { shouldRepaint = true; rowDirty[static_cast<size_t>(row)] = true; }
            if (std::abs(previous - l.rangeOpacity[i]) > 0.012f)
            { shouldRepaint = true; rowDirty[static_cast<size_t>(row)] = true; }
            const int value = processor.getLaneValue(row, i);
            if (l.values[i] != value) { l.values[i] = value; shouldRepaint = true; rowDirty[static_cast<size_t>(row)] = true; }
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
                shouldRepaint = true; rowDirty[static_cast<size_t>(row)] = true;
            }
        }
        if (playing && animate && experimentalLights)
        {
            const double durationBeats = divisionsBeats[juce::jlimit(0, 10, l.division)]
                * globalFactor * timeFactors[juce::jlimit(0, 2, processor.getLaneTiming(row))];
            double phase = std::fmod((ppqNow - processor.getLanePhaseOrigin(row)) /
                juce::jmax(0.00001, durationBeats), 1.0);
            if (phase < 0.0) phase += 1.0;
            l.stepPhase = static_cast<float>(phase);
            const double stepMs = durationBeats * 60000.0 / bpm;
            l.stepDurationMs = stepMs;
            l.stepLed = PnordeLights::pulse(phase * stepMs, stepMs);
            shouldRepaint = true; rowDirty[static_cast<size_t>(row)] = true;
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
            std::abs(l.markerVelocity) > 0.08f)
        { shouldRepaint = true; rowDirty[static_cast<size_t>(row)] = true; }
        else { l.displayX = target; l.markerVelocity = 0.0f; }
        if (l.rippleProgress < 1.0f)
        {
            l.rippleProgress = juce::jmin(1.0f, l.rippleProgress + 0.12f);
            shouldRepaint = true; rowDirty[static_cast<size_t>(row)] = true;
        }
    }
    if (shouldRepaint)
    {
        if (fullRepaint)
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
                if (!rowDirty[static_cast<size_t>(row)] || !rowVisible(row)) continue;
                const float y = rowTop(row);
                if (y > h * 0.918f || y + h * 0.105f < h * 0.110f) continue;
                const int top = juce::jmax(juce::roundToInt(h * 0.110f),
                    juce::roundToInt(y - h * 0.013f));
                const int bottom = juce::jmin(juce::roundToInt(h * 0.918f),
                    juce::roundToInt(y + h * 0.108f));
                if (bottom > top) repaint(0, top, getWidth(), bottom - top);
            }
            if (programLaneDirty)
            {
                const float y = programLaneTop();
                const int top = juce::jmax(juce::roundToInt(h * 0.110f),
                    juce::roundToInt(y - h * 0.013f));
                const int bottom = juce::jmin(juce::roundToInt(h * 0.918f),
                    juce::roundToInt(y + h * 0.108f));
                if (bottom > top) repaint(0, top, getWidth(), bottom - top);
            }
            if (vibesFrame)
                repaint(); // full-frame refresh ONLY <= 12 times per second
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
    return juce::jmax(0.0f, h * (0.155f + (count - 1) * 0.110f + 0.105f - 0.918f));
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
        laneNameEditors[row].setVisible(visible && row >= 7);
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
            ccLabelEditors[slot].setVisible(false);
        }
    }
    programClockDial.setVisible(processor.getProgramLaneEnabled());
    programTimingDial.setVisible(processor.getProgramLaneEnabled());
    refreshMacroPlaceholders();
    syncLaneNames();
    bool hasCapacity = processor.activeCCMacroCount() < 8;
    for (int kind = 1; kind <= 6; ++kind)
        if (!processor.hasActiveLaneType(kind)) hasCapacity = true;
    addCCButton.setEnabled(hasCapacity || !processor.getProgramLaneEnabled());
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
    programClockDial.setTooltipsEnabled(enabled);
    programTimingDial.setTooltipsEnabled(enabled);
    for (int slot = 0; slot < optionalLanes; ++slot)
    {
        ccLabelEditors[slot].setTooltip(hint("Rename CC Macro. Enter saves; Escape cancels."));
        ccSelect[slot].setTooltip(hint("Choose the MIDI controller (CC20 to CC31) sent by this Macro."));
        laneTypeSelect[slot].setTooltip(hint("Change this lane's MIDI function. Existing step values are preserved."));
        deleteCC[slot].setTooltip(hint("Remove this optional lane."));
    }
    for (int row = 0; row < lanes; ++row)
    {
        laneNameEditors[row].setTooltip(enabled ? laneHoverDescription(row) : juce::String());
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
    addCCButton.setTooltip(hint("Add a CC Macro, optional MIDI lane, or dedicated PROGRAM lane."));
    settingsButton.setTooltip(hint("Open PNORDE settings."));
    savePresetButton.setTooltip(hint("Save every pattern and setting as a .pnorde preset."));
    loadPresetButton.setTooltip(hint("Load user .pnorde presets or browse built-in factory preset categories."));
    playButton.setTooltip(hint("Pause/resume PNORDE within Ableton transport. MIDI mappable."));
    resetButton.setTooltip(hint("Restart PNORDE sequencer phase and stop held notes. MIDI mappable trigger."));
    randomiseButton.setTooltip(hint("Randomise lanes or generate a harmonic microtonal PITCH pattern when an EDO mode is selected."));
    presetFolderButton.setTooltip(hint("Open or choose the preset library folder."));
    darknessSlider.setTooltip(enabled ? juce::String("Dark mode amount. Higher values dim the interface while keeping the active accents readable.") : juce::String());
}

juce::String PolyrhythmMIDIEditor::getTooltip()
{
    if (!processor.getTooltipsEnabled()) return {};
    const auto point = getMouseXYRelative().toFloat();
    const float h = static_cast<float>(getHeight());
    if (point.y < h * 0.110f || point.y >= h * 0.918f) return {};
    for (int row = 0; row < lanes; ++row)
    {
        if (!rowVisible(row)) continue;
        if (row >= 7 && reorderHandle(row).contains(point))
            return processor.getExperimentalReorderEnabled()
                ? "Drag the coloured rail to reorder this lane."
                : "Lane clock indicator. Enable Experimental Reorder in Settings to drag.";
        if (lengthBox(row).contains(point))
            return "Set Sequence Length: drag to change; double-click to reset to 16.";
        const auto labelRect = juce::Rectangle<float>(workLeft() + 42.0f, rowTop(row) + h * 0.002f, 170.0f, h * 0.034f);
        if (row < 7 && labelRect.contains(point))
            return laneHoverDescription(row);
        if (lane[row].page == (lane[row].length - 1) / visibleSteps && markerBox(row).contains(point))
            return "Drag the sequence end marker to set the number of steps.";
        for (int i = 0; i < visibleSteps; ++i)
        {
            const auto b = cell(row, i);
            const juce::Rectangle<float> numberHit(b.getX(), rowTop(row) + h * 0.027f,
                                                      b.getWidth(), h * 0.023f);
            if (numberHit.contains(point))
                return "Click/drag: sequence length. Ctrl-click (Windows/Linux) or Cmd-click (Mac): start position (within current lane length).";
            const int start = processor.getLaneStartStep(row);
            if (lane[row].page * visibleSteps + i == start)
            {
                const juce::Rectangle<float> arrowHit(b.getX() - 9.0f,
                    b.getY() - h * 0.024f, 14.0f, h * 0.023f);
                if (arrowHit.contains(point))
                    return "Lane starts here. Click the play arrow to reset the start to step 1; Ctrl/Cmd-click a different step number to move it.";
            }
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

void PolyrhythmMIDIEditor::dismissContainedPanel()
{
    // Deferred destruction avoids deleting a mouse-event component in its own callback.
    popupOverlay.reset();
    repaint();
}
void PolyrhythmMIDIEditor::openContainedPanel(
    std::unique_ptr<juce::Component> panel, juce::Rectangle<int> anchor)
{
    dismissContainedPanel();
    juce::Component::SafePointer<PolyrhythmMIDIEditor> safe(this);
    popupOverlay = std::make_unique<PnordeOverlayHost>(std::move(panel), anchor,
        [safe]
        {
            juce::MessageManager::callAsync([safe]
            { if (safe != nullptr) safe->dismissContainedPanel(); });
        });
    addAndMakeVisible(*popupOverlay);
    popupOverlay->setBounds(getLocalBounds());
    popupOverlay->toFront(true);
}
void PolyrhythmMIDIEditor::refreshProgramButtons()
{
    const int selected = processor.getSelectedProgram();
    if (selected == lastDisplayedProgram) return;
    lastDisplayedProgram = selected;
    const auto& t = themes[themeIndex];
    for (int i = 0; i < 4; ++i)
    {
        auto& button = quickPrograms[static_cast<size_t>(i)];
        button.setColour(juce::TextButton::buttonColourId,
            i == selected ? t.teal.brighter(0.38f) : t.paper.contrasting(0.12f));
        button.setColour(juce::TextButton::textColourOffId,
            i == selected ? juce::Colours::black : t.ink);
    }
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
            safe->lastDisplayedProgram = -1;
            safe->refreshProgramButtons();
            safe->repaint();
        });
    const auto anchor = settingsButton.getBounds();
    // 70px permits the CallOutBox's border and arrow as well as the host
    // container. A short window scrolls SETTINGS; the main grid stays put.
    auto contents = std::make_unique<PnordeSettingsViewport>(
        std::move(panel), getHeight() - 90);
    openContainedPanel(std::move(contents), anchor);
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
    octaveDial.setIndex(processor.getGlobalOctave()+3);
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
    programClockDial.setValue(processor.getProgramLaneDivision());
    programTimingDial.setValue(processor.getProgramLaneTiming());
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

void PolyrhythmMIDIEditor::showProgramMenu(int index)
{
    if (index < 0 || index >= 4) return;
    juce::PopupMenu menu;
    menu.addItem(1, "Reset Program " + juce::String(index + 1) + " to defaults...");
    menu.addSeparator();
    for (int i = 0; i < 4; ++i)
        if (i != index) menu.addItem(10 + i, "Copy all values to Program " + juce::String(i+1));
    juce::Component::SafePointer<PolyrhythmMIDIEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&quickPrograms[index]),
        [safe, index](int choice)
        {
            if (safe == nullptr) return;
            if (choice == 1)
            {
                juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon,
                    "Reset PNORDE Program", "Restore Program " + juce::String(index + 1) +
                    " to factory defaults? This does not change the other slots.", "Reset", "Cancel", nullptr,
                    juce::ModalCallbackFunction::create([safe, index](int accepted)
                    {
                        if (safe == nullptr || accepted == 0) return;
                        if (safe->processor.resetProgram(index))
                        { safe->syncFromProcessor(); safe->repaint(); }
                    }));
            }
            else if (choice >= 10 && choice < 14 && choice - 10 != index)
            {
                const int dest = choice - 10;
                juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon,
                    "Copy PNORDE Program", "Replace Program " + juce::String(dest+1) +
                    " with every musical value from Program " + juce::String(index+1) + "?",
                    "Copy", "Cancel", nullptr,
                    juce::ModalCallbackFunction::create([safe, index, dest](int accepted)
                    {
                        if (safe == nullptr || accepted == 0) return;
                        if (safe->processor.copyProgram(index, dest))
                        { safe->syncFromProcessor(); safe->repaint(); }
                    }));
            }
        });
}

void PolyrhythmMIDIEditor::showProgramLaneMenu()
{
    juce::PopupMenu menu;
    juce::PopupMenu lengths, division, timing;
    for (int i = 1; i <= 128; i *= 2)
        lengths.addItem(400+i, juce::String(i) + " steps", true,
            processor.getProgramLaneLength() == i);
    for (int i = 0; i < 11; ++i)
        division.addItem(200+i, divisionsText[i], true,
            processor.getProgramLaneDivision() == i);
    for (int i = 0; i < 3; ++i)
        timing.addItem(300+i, juce::StringArray{"Straight","Triplet","Dotted"}[i],
            true, processor.getProgramLaneTiming() == i);
    menu.addSubMenu("Length", lengths);
    menu.addSubMenu("Clock Division", division);
    menu.addSubMenu("Timing", timing);
    menu.addSeparator();
    menu.addItem(12, "AUTO program changes (armed)", true,
        processor.getProgramLaneAutoArmed());
    menu.addItem(10, "Clear all PROGRAM triggers...");
    menu.addSeparator();
    menu.addItem(11, "Remove PROGRAM Lane (keep its values)");
    juce::Component::SafePointer<PolyrhythmMIDIEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(
        localAreaToGlobal(juce::Rectangle<int>(juce::roundToInt(workLeft()+45),
            juce::roundToInt(programLaneTop()), 155, 30))),
        [safe](int id)
        {
            if (safe == nullptr) return;
            if (id >= 401 && id <= 528 && (id - 400) <= 128)
                safe->processor.setProgramLaneLength(id - 400);
            else if (id >= 200 && id < 211)
            {
                safe->processor.setProgramLaneDivision(id - 200);
                safe->programClockDial.setValue(id - 200);
            }
            else if (id >= 300 && id < 303)
            {
                safe->processor.setProgramLaneTiming(id - 300);
                safe->programTimingDial.setValue(id - 300);
            }
            else if (id == 12)
            {
                safe->processor.setProgramLaneAutoArmed(
                    !safe->processor.getProgramLaneAutoArmed());
                safe->repaint();
            }
            else if (id == 11)
            {
                safe->processor.setProgramLaneEnabled(false);
                safe->targetScrollY = juce::jmin(safe->targetScrollY, safe->maxScroll());
                safe->syncCCVisibility();
                safe->resized();
            }
            else if (id == 10)
            {
                juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon,
                    "Clear PROGRAM Lane", "Remove every automatic program-change trigger?",
                    "Clear", "Cancel", nullptr,
                    juce::ModalCallbackFunction::create([safe](int accepted)
                    {
                        if (safe == nullptr || accepted == 0) return;
                        for (int i = 0; i < 128; ++i)
                            safe->processor.setProgramLaneValue(i, 0);
                        safe->repaint();
                    }));
            }
            safe->programLanePage = juce::jlimit(0,
                (safe->processor.getProgramLaneLength()-1)/safe->visibleSteps,
                safe->programLanePage);
            safe->repaint();
        });
}

void PolyrhythmMIDIEditor::showLaneDirectionPanel(int row)
{
    if (!rowVisible(row)) return;
    const auto& theme = themes[themeIndex];
    const auto bounds = juce::Rectangle<int>(juce::roundToInt(workLeft() + 42.0f),
        juce::roundToInt(rowTop(row)), 160, juce::roundToInt(getHeight() * 0.034f));
    auto content = std::make_unique<PnordeDirectionPanel>(processor, row,
        theme.paper, theme.ink, theme.teal);
    openContainedPanel(std::move(content), bounds);
}

void PolyrhythmMIDIEditor::showClocksMenu(int row)
{
    if (!rowVisible(row)) return;
    juce::PopupMenu menu;
    menu.addItem(1, "Randomise this lane's clock division");
    menu.addItem(2, "Randomise Straight / Triplet / Dotted");
    menu.addItem(3, "Randomise both");
    juce::Component::SafePointer<PolyrhythmMIDIEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&clockDials[row]),
        [safe, row](int choice)
        {
            if (safe == nullptr || !safe->rowVisible(row) || choice < 1 || choice > 3) return;
            auto& rng = juce::Random::getSystemRandom();
            if (choice != 2) safe->processor.setLaneDivision(row, rng.nextInt(11));
            if (choice != 1) safe->processor.setLaneTiming(row, rng.nextInt(3));
            safe->syncFromProcessor();
        });
}

void PolyrhythmMIDIEditor::showLaneDirectionMenu(int row)
{
    if (!rowVisible(row)) return;
    juce::PopupMenu menu;
    constexpr const char* directions[] = {"Forwards", "Backwards", "Ping Pong"};
    for (int i=0;i<3;++i)
        menu.addItem(i+1,directions[i],true,processor.getLaneDirection(row)==i);
    menu.addSeparator();
    menu.addItem(4,"RESTART THIS LANE (automatable)");
    menu.addSeparator();
    menu.addItem(5, "Duplicate pattern x2 (extend lane)",
        processor.getLaneLength(row) <= 64);
    juce::Component::SafePointer<PolyrhythmMIDIEditor> safe(this);
    const auto bounds=juce::Rectangle<int>(juce::roundToInt(workLeft()+42.0f),
         juce::roundToInt(rowTop(row)),160,
         juce::roundToInt(getHeight()*0.034f));
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(localAreaToGlobal(bounds)),
        [safe,row](int result)
        {
            if (safe == nullptr || !safe->rowVisible(row)) return;
            if (result == 4) { safe->processor.requestLaneReset(row); return; }
            if (result == 5) { if (safe->processor.duplicateLanePattern(row))
                                  safe->syncFromProcessor(); return; }
            if (result < 1 || result > 3) return;
            safe->processor.setLaneDirection(row,result-1);
            safe->repaint();
        });
}

void PolyrhythmMIDIEditor::showLoadPresetMenu()
{
    juce::PopupMenu menu;
    menu.addItem(1,"Load user .pnorde file...");
    menu.addSeparator();
    juce::PopupMenu factory;
    const char* categories[]={"Ambient","Dance","Experimental","Generative"};
    for (const auto* category:categories)
    {
        juce::PopupMenu choices;
        for (int i=0;i<static_cast<int>(PnordeFactoryPresets::presets.size());++i)
            if (juce::String(PnordeFactoryPresets::presets[static_cast<size_t>(i)].category)==category)
                choices.addItem(100+i,PnordeFactoryPresets::presets[static_cast<size_t>(i)].name);
        factory.addSubMenu(category,choices);
    }
    menu.addSubMenu("FACTORY PRESETS (always available)",factory);
    juce::PopupMenu compositions;
    const char* categories2[]={"Baroque","Classical","Jazz & Bebop","Blues & Funk",
        "Serialism","Maqam-inspired (12-TET)","Maqam-inspired (approx.)",
        "Indian-inspired (12-TET)","Gamelan-inspired (12-TET)",
        "EDO & Xenharmonic","Ambient / Minimal"};
    for(const char* category:categories2)
    {
        juce::PopupMenu group;
        for(int i=0;i<static_cast<int>(PnordeCompositionEngine::compositions.size());++i)
        {
            const auto& c=PnordeCompositionEngine::compositions[static_cast<size_t>(i)];
            if(juce::String(c.category)==category)
                group.addItem(5000+i,juce::String(c.name)+" ("+juce::String(c.length)+" steps)");
        }
        compositions.addSubMenu(category,group);
    }
    menu.addSubMenu("FACTORY COMPOSITIONS (fixed patterns; changes lane lengths)",compositions);
    juce::Component::SafePointer<PolyrhythmMIDIEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&loadPresetButton),
        [safe](int id)
        { if (safe==nullptr) return;
          if (id==1) safe->loadPreset();
          else if (id>=100 && id<100+static_cast<int>(PnordeFactoryPresets::presets.size()))
              safe->applyFactoryPreset(id-100);
          else if (id>=5000 && id<5000+static_cast<int>(PnordeCompositionEngine::compositions.size()))
              safe->applyComposition(id-5000,0x5A6E6A10u+static_cast<std::uint32_t>(id-5000)*1337u);
        });
}

void PolyrhythmMIDIEditor::applyFactoryPreset(int index)
{
    if (index<0 || index>=static_cast<int>(PnordeFactoryPresets::presets.size())) return;
    const auto& f=PnordeFactoryPresets::presets[static_cast<size_t>(index)];
    auto& rng=juce::Random::getSystemRandom();
    const auto seed=static_cast<std::uint32_t>(rng.nextInt());
    std::array<bool,128> gates{};
    std::array<int,128> velocity{},pitch{},slide{},pan{},panGlide{};
    PnordePatternLibrary::gates(gates,16,f.gateRecipe,seed);
    PnordePatternLibrary::velocities(velocity,16,f.velocityRecipe,seed+1);
    PnordeFactoryPresets::pitch(pitch,16,f.pitchSpan,seed+2);
    PnordePatternLibrary::slides(slide,16,f.slideRecipe,seed+3);
    PnordePatternLibrary::pans(pan,panGlide,16,f.panRecipe,seed+4);
    for (int row=0;row<6;++row)
        processor.setLaneLength(row,16);
    processor.setLaneLength(6,16);
    processor.setLaneDivision(0,f.clockDivision);
    for (int i=0;i<16;++i)
    {
        processor.setStepEnabled(i,gates[static_cast<size_t>(i)]);
        processor.setLaneValue(1,i,processor.quantizePitchTenths(pitch[static_cast<size_t>(i)],0));
        processor.setLaneValue(2,i,velocity[static_cast<size_t>(i)]);
        processor.setLaneValue(5,i,slide[static_cast<size_t>(i)]);
    }
    // A factory preset may add Pan if there is capacity; never rename/delete
    // an existing user lane and never create duplicate fixed CC10 instances.
    int panRow=-1;
    for (int row=7;row<lanes;++row)
        if (processor.isOptionalLaneEnabled(row) &&
            processor.getOptionalLaneType(row)==static_cast<int>(PnordeLane::Type::pan))
            panRow=row;
    if (panRow<0) panRow=processor.addOptionalLane(static_cast<int>(PnordeLane::Type::pan));
    if (panRow>=7)
    {
        processor.setLaneLength(panRow,16);
        processor.setLaneDivision(panRow,f.panDivision);
        for (int i=0;i<16;++i)
        {
            processor.setLaneValue(panRow,i,pan[static_cast<size_t>(i)]);
            processor.setOptionalGlide(panRow,i,panGlide[static_cast<size_t>(i)]);
        }
    }
    processor.requestMidiPanic();
    syncFromProcessor();
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

void PolyrhythmMIDIEditor::applyMusicalPhrase(
    const PnordeCompositionEngine::Phrase& phrase)
{
    // UI thread only; all seven lanes share a common temporal grid.
    // Does not overwrite any other saved Program, change global reverse,
    // change lane direction/start, or add/remove optional MIDI CC lanes.
    processor.setScale(phrase.scale);
    const int clock=processor.getLaneDivision(0);
    for(int row=0;row<7;++row)
    {
        processor.setLaneLength(row,phrase.length);
        if(row>0) processor.setLaneDivision(row,clock);
    }
    for(int i=0;i<phrase.length;++i)
    {
        const size_t index=static_cast<size_t>(i);
        processor.setStepEnabled(i,phrase.gates[index]);
        processor.setLaneValue(1,i,phrase.pitch[index]);
        processor.setLaneValue(2,i,phrase.velocity[index]);
        processor.setLaneValue(3,i,phrase.duration[index]);
        processor.setLaneValue(4,i,phrase.octave[index]);
        processor.setLaneValue(5,i,phrase.slide[index]);
        processor.setLaneValue(6,i,phrase.pressure[index]);
    }
    processor.requestMidiPanic();
    syncFromProcessor();
}

void PolyrhythmMIDIEditor::applyComposition(int index,std::uint32_t seed)
{
    if(index<0 || index>=static_cast<int>(PnordeCompositionEngine::compositions.size()))
        return;
    const auto& c=PnordeCompositionEngine::compositions[static_cast<size_t>(index)];
    PnordeCompositionEngine::Phrase phrase;
    PnordeCompositionEngine::fill(phrase,c.scale,processor.getKey(),c.length,
        c.recipe,c.arp,seed);
    if(index==7 || index==8)
        PnordeCompositionEngine::makeSerial(phrase,index==8,processor.getKey());
    applyMusicalPhrase(phrase);
}

void PolyrhythmMIDIEditor::randomisePattern()
{
    juce::PopupMenu menu;
    juce::PopupMenu melodyScales, arpeggioScales, compositions;
    for (int scale=0; scale<PnordeScales::count; ++scale)
    {
        juce::PopupMenu phrases, arps;
        for (int recipe=0;recipe<PnordeCompositionEngine::melodyCount;++recipe)
            phrases.addItem(2000+scale*16+recipe,
                PnordeCompositionEngine::melodyNames[recipe]);
        for (int recipe=0;recipe<PnordeCompositionEngine::arpCount;++recipe)
            arps.addItem(3000+scale*16+recipe,
                PnordeCompositionEngine::arpNames[recipe]);
        if(scale==0)
        {
            juce::PopupMenu jazz;
            for(int i=0;i<PnordeCompositionEngine::chromaticJazzCount;++i)
                jazz.addItem(4800+i,PnordeCompositionEngine::chromaticJazzNames[i]);
            phrases.addSeparator();
            phrases.addSubMenu("Chromatic Jazz / Chord Changes",jazz);
        }
        melodyScales.addSubMenu(PnordeScales::names[scale],phrases);
        arpeggioScales.addSubMenu(PnordeScales::names[scale],arps);
    }
    menu.addSubMenu("Generate Melody (full phrase)",melodyScales);
    menu.addSubMenu("Generate Arpeggio (full phrase)",arpeggioScales);
    const char* genres[]={"Baroque","Classical","Jazz & Bebop","Blues & Funk",
        "Serialism","Maqam-inspired (12-TET)","Maqam-inspired (approx.)",
        "Indian-inspired (12-TET)","Gamelan-inspired (12-TET)",
        "EDO & Xenharmonic","Ambient / Minimal"};
    for (const char* genre:genres)
    {
        juce::PopupMenu group;
        for (int i=0;i<static_cast<int>(PnordeCompositionEngine::compositions.size());++i)
        {
            const auto& c=PnordeCompositionEngine::compositions[static_cast<size_t>(i)];
            if (juce::String(c.category)==genre)
                group.addItem(4000+i,juce::String(c.name)+" ("+juce::String(c.length)+" steps)");
        }
        compositions.addSubMenu(genre,group);
    }
    menu.addSubMenu("Generate Composition (changes lane lengths)",compositions);
    menu.addSeparator();
    juce::PopupMenu legacyRandom;
    legacyRandom.addItem(1, "Randomise all active lanes");
    legacyRandom.addItem(2, "Randomise SEQ gate pattern only");
    menu.addSeparator();
    legacyRandom.addItem(40, "Randomise active lane clock divisions");
    legacyRandom.addItem(41, "Randomise active lane Straight / Triplet / Dotted");
    legacyRandom.addItem(42, "Randomise active lane clocks + timing");
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
    juce::PopupMenu expandedGates;
    expandedGates.addSubMenu("Classic",gateRecipes);
    juce::PopupMenu newGates;
    for(int i=0;i<PnordeCompositionEngine::addedGateCount;++i)
        newGates.addItem(1800+i,PnordeCompositionEngine::addedGateNames[i]);
    expandedGates.addSubMenu("Jazz / Funk / Euclidean / Ambient",newGates);
    menu.addSubMenu("Generate Gate Pattern",expandedGates);
    menu.addSubMenu("Generate VELOCITY groove", velocityRecipes);
    juce::PopupMenu aftertouchRecipes;
    for (int i = 0; i < PnordePatternLibrary::aftertouchCount; ++i)
        aftertouchRecipes.addItem(900+i, PnordePatternLibrary::aftertouchNames[i]);
    menu.addSubMenu("Generate AFTERTOUCH pressure", aftertouchRecipes);
    juce::PopupMenu panRecipes, slideRecipes;
    for (int i=0;i<PnordePatternLibrary::panCount;++i)
        panRecipes.addItem(700+i,PnordePatternLibrary::panNames[i]);
    for (int i=0;i<PnordePatternLibrary::slideCount;++i)
        slideRecipes.addItem(800+i,PnordePatternLibrary::slideNames[i]);
    menu.addSubMenu("Generate PAN movement (stereo)",panRecipes,
                    processor.hasActiveLaneType(static_cast<int>(PnordeLane::Type::pan)));
    menu.addSubMenu("Generate SLIDE movement",slideRecipes);
    menu.addSeparator();
    for (int row = 0; row < lanes; ++row)
    {
        if (!rowVisible(row)) continue;
        const juce::String title = laneDisplayName(row);
        legacyRandom.addItem(100 + row, "Randomise " + title);
    }
    menu.addSubMenu("Randomise (legacy)",legacyRandom);
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
            // The menu IDs encode scale and recipe, never program numbers.
            // Generate into scratch arrays, then commit on the UI thread.
            if(id>=4800 && id<4800+PnordeCompositionEngine::chromaticJazzCount)
            {
                PnordeCompositionEngine::Phrase phrase;
                PnordeCompositionEngine::makeChromaticJazz(phrase,engine.getKey(),
                    engine.getLaneLength(0),id-4800,
                    static_cast<std::uint32_t>(rng.nextInt()));
                safe->applyMusicalPhrase(phrase);return;
            }
            if ((id>=2000 && id<2000+PnordeScales::count*16) ||
                (id>=3000 && id<3000+PnordeScales::count*16) ||
                (id>=4000 && id<4000+static_cast<int>(PnordeCompositionEngine::compositions.size())))
            {
                if(id>=4000)
                {
                    safe->applyComposition(id-4000,
                        static_cast<std::uint32_t>(rng.nextInt()));
                    return;
                }
                const bool arp=id>=3000;
                const int raw=id-(arp?3000:2000);
                const int scale=raw/16,recipe=raw%16;
                if(recipe>=(arp?PnordeCompositionEngine::arpCount:
                               PnordeCompositionEngine::melodyCount)) return;
                PnordeCompositionEngine::Phrase phrase;
                PnordeCompositionEngine::fill(phrase,scale,engine.getKey(),
                    engine.getLaneLength(0),recipe,arp,
                    static_cast<std::uint32_t>(rng.nextInt()));
                safe->applyMusicalPhrase(phrase);
                return;
            }
            if (id>=1800 && id<1800+PnordeCompositionEngine::addedGateCount)
            {
                std::array<bool,128> pattern{};
                const int count=juce::jlimit(1,128,engine.getLaneLength(0));
                PnordeCompositionEngine::makeGate(pattern,count,id-1800,
                    static_cast<std::uint32_t>(rng.nextInt()));
                for(int step=0;step<count;++step)
                    engine.setStepEnabled(step,pattern[static_cast<size_t>(step)]);
                safe->syncFromProcessor();return;
            }
            if (id >= 40 && id <= 42)
            {
                // UI event only: uses the SAME already-automatable lane
                // division/timing setters as the existing clock dials.
                // No pattern/gate/macro values, lane lengths or start steps touched.
                for (int row = 0; row < lanes; ++row)
                {
                    if (!safe->rowVisible(row)) continue;
                    if (id != 41) engine.setLaneDivision(row, rng.nextInt(11));
                    if (id != 40) engine.setLaneTiming(row, rng.nextInt(3));
                }
                safe->syncFromProcessor();
                return;
            }
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
            if (id >= 900 && id < 900 + PnordePatternLibrary::aftertouchCount)
            {
                std::array<int,128> pressure {};
                const int count = juce::jlimit(1,128,engine.getLaneLength(6));
                PnordePatternLibrary::aftertouch(pressure, count, id-900,
                    static_cast<std::uint32_t>(rng.nextInt()));
                for (int step=0; step<count; ++step)
                    engine.setLaneValue(6, step, pressure[static_cast<size_t>(step)]);
                safe->syncFromProcessor(); return;
            }
            if (id >= 700 && id < 700+PnordePatternLibrary::panCount)
            {
                int row=-1;
                for (int r=7;r<lanes;++r)
                    if (engine.isOptionalLaneEnabled(r) &&
                        engine.getOptionalLaneType(r)==static_cast<int>(PnordeLane::Type::pan))
                    { row=r;break; }
                if (row<7) return;
                std::array<int,128> pan{},glide{};
                const int count=juce::jlimit(1,128,engine.getLaneLength(row));
                PnordePatternLibrary::pans(pan,glide,count,id-700,
                    static_cast<std::uint32_t>(rng.nextInt()));
                for (int step=0;step<count;++step)
                { engine.setLaneValue(row,step,pan[static_cast<size_t>(step)]);
                  engine.setOptionalGlide(row,step,glide[static_cast<size_t>(step)]); }
                safe->syncFromProcessor(); return;
            }
            if (id >= 800 && id < 800+PnordePatternLibrary::slideCount)
            {
                std::array<int,128> values{};
                const int count=juce::jlimit(1,128,engine.getLaneLength(5));
                PnordePatternLibrary::slides(values,count,id-800,
                    static_cast<std::uint32_t>(rng.nextInt()));
                for (int step=0;step<count;++step)
                    engine.setLaneValue(5,step,values[static_cast<size_t>(step)]);
                safe->syncFromProcessor(); return;
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
    if (e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        if (std::abs(wheel.deltaY) > 0.0001f)
        {
            const int old = processor.getDarknessPercent();
            const int step = juce::jlimit(1, 8,
                juce::roundToInt(std::abs(wheel.deltaY) * 30.0f));
            processor.setExperimentalLightsEnabled(true);
            processor.setDarknessPercent(juce::jlimit(0, 75,
                old + (wheel.deltaY > 0.0f ? -step : step)));
            repaint();
        }
        return;
    }
    if (e.position.y >= getHeight() * 0.11f && e.position.y < getHeight() * 0.918f)
    {
        if (visibleSteps < 16 && (e.mods.isShiftDown() || std::abs(wheel.deltaX) > 0.015f))
        {
            const float amount = std::abs(wheel.deltaX) > 0.015f
                ? wheel.deltaX : wheel.deltaY;
            if (std::abs(amount) > 0.015f)
                for (int row = 0; row < lanes; ++row)
                    if (rowVisible(row) && e.position.y >= rowTop(row) &&
                        e.position.y < rowTop(row) + getHeight() * 0.100f)
                    {
                        auto& a = lane[row];
                        a.page = juce::jlimit(0, (a.length - 1) / visibleSteps,
                                             a.page + (amount > 0.0f ? -1 : 1));
                        a.displayX = -1.0f;
                        updateButtons(); repaint();
                        return;
                    }
            return;
        }
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
    vibes.setPalette({ t.coral, t.blue, t.sage, t.lavender,
                       t.amber, t.teal, t.pink, t.cyan });
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
    programClockDial.setPalette(t.paper, controlInk, t.lavender);
    programTimingDial.setPalette(t.paper, controlInk, t.lavender);
    const juce::Colour laneColours[] {t.coral,t.blue,t.sage,t.lavender,t.amber,t.teal,t.pink,
        t.teal,t.amber,t.blue,t.coral,t.lavender,t.sage,t.pink,t.amber,t.teal,t.blue,t.lavender,t.pink,t.sage,t.amber};
    for (int row = 0; row < lanes; ++row)
    {
        laneNameEditors[row].setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        laneNameEditors[row].setEditingTextColour(
            laneColours[row].brighter(dark ? 0.85f : 0.12f));
        laneNameEditors[row].setColour(juce::TextEditor::highlightColourId,
            laneColours[row].withAlpha(0.32f));
        laneNameEditors[row].setColour(juce::TextEditor::focusedOutlineColourId, t.teal);
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
        for (auto* button : {&prev[row], &next[row]})
        {
            button->setColour(juce::TextButton::buttonColourId,
                dark ? t.cream.interpolatedWith(t.ink, 0.16f) : t.paper);
            button->setColour(juce::TextButton::buttonOnColourId, laneColours[row]);
            button->setColour(juce::TextButton::textColourOffId,
                dark ? laneColours[row].brighter(0.85f) : laneColours[row].darker(0.20f));
        }
        addPage[row].setColour(juce::TextButton::buttonColourId, dark ? t.sage.interpolatedWith(t.ink, 0.30f) : t.sage.withAlpha(0.18f));
        addPage[row].setColour(juce::TextButton::buttonOnColourId, t.sage);
        addPage[row].setColour(juce::TextButton::textColourOffId, dark ? t.sage.brighter(0.95f) : t.sage.darker(0.28f));
        removePage[row].setColour(juce::TextButton::buttonColourId, dark ? t.coral.interpolatedWith(t.ink, 0.34f) : t.coral.withAlpha(0.16f));
        removePage[row].setColour(juce::TextButton::buttonOnColourId, t.coral);
        removePage[row].setColour(juce::TextButton::textColourOffId, dark ? t.coral.brighter(0.95f) : t.coral.darker(0.22f));
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
        editor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        editor.setColour(juce::TextEditor::textColourId, laneColours[slot+7].brighter(dark ? 0.72f : 0.12f));
        editor.setColour(juce::TextEditor::highlightedTextColourId,
                         laneColours[slot+7].brighter(dark ? 0.72f : 0.12f));
        editor.setColour(juce::TextEditor::focusedOutlineColourId, t.teal);
        editor.setColour(juce::TextEditor::highlightColourId, t.teal.withAlpha(0.28f));
    }
    const bool highDarkness = processor.getExperimentalLightsEnabled() && processor.getDarknessPercent() >= 35;
    for (int row = 0; row < lanes; ++row)
    {
        if (highDarkness)
        {
            for (auto* button : { &prev[row], &next[row] })
            {
                button->setColour(juce::TextButton::buttonColourId, laneColours[row].brighter(0.30f));
                button->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            }
            addPage[row].setColour(juce::TextButton::buttonColourId, t.sage.brighter(0.35f));
            addPage[row].setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            removePage[row].setColour(juce::TextButton::buttonColourId, t.coral.brighter(0.35f));
            removePage[row].setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        }
    }
    globalReverseButton.setColour(juce::TextButton::buttonColourId,
        processor.getGlobalReverse() ? t.coral.brighter(0.28f) : t.paper);
    globalReverseButton.setColour(juce::TextButton::textColourOffId, t.ink);
    addCCButton.setColour(juce::TextButton::buttonColourId, t.teal);
    addCCButton.setColour(juce::TextButton::textColourOffId, t.ink);
    for (auto* b : std::array<juce::TextButton*, 7>{ {
        &savePresetButton, &loadPresetButton, &presetFolderButton,
                     &playButton, &resetButton,
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
    darknessSlider.setColour(juce::Slider::backgroundColourId, dark ? t.ink.withAlpha(0.28f) : t.teal.withAlpha(0.18f));
    darknessSlider.setColour(juce::Slider::trackColourId, t.teal);
    darknessSlider.setColour(juce::Slider::thumbColourId, t.paper);
    darknessSlider.setColour(juce::Slider::textBoxTextColourId, t.ink);
    darknessSlider.setColour(juce::Slider::textBoxOutlineColourId, t.teal.withAlpha(0.45f));
    darknessSlider.setColour(juce::Slider::textBoxBackgroundColourId, controlBg);
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
    programClockDial.setVisible(processor.getProgramLaneEnabled());
    programTimingDial.setVisible(processor.getProgramLaneEnabled());
    refreshMacroPlaceholders();
    syncLaneNames();
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
