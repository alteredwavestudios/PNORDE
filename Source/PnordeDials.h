// ============================================================================
// PnordeDials.h — PNORDE Dials
// ============================================================================
// This helper isolates one part of the sequencer so it can be understood and tested without reading the whole plugin.
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
#include <algorithm>
#include <cmath>
#include <functional>

// Lightweight vector-rendered stepped dial. Stable parameter indices, no glyph caches,
// no allocations on the audio thread; all interaction runs on JUCE's UI thread.
class PnordeChoiceDial final : public juce::Component, public juce::TooltipClient
{
public:
    std::function<void(int)> onChange;
    void setChoices(const juce::StringArray& choices, int startingIndex)
    {
        names = choices;
        setIndex(startingIndex);
    }
    void setIndex(int value)
    {
        const int maxIndex = juce::jmax(0, names.size() - 1);
        value = juce::jlimit(0, maxIndex, value);
        if (index != value) { index = value; repaint(); }
    }
    int getIndex() const noexcept { return index; }
    void setPalette(juce::Colour paper, juce::Colour text, juce::Colour accent)
    { bg = paper; fg = text; glow = accent; repaint(); }
    void setDescription(const juce::String& text) { description = text; }
    void setTooltipsEnabled(bool on) noexcept { tooltipsEnabled = on; }
    juce::String getTooltip() override
    { if (!tooltipsEnabled || names.isEmpty()) return {};
      return description + ": " + names[juce::jlimit(0, juce::jmax(0, names.size()-1), index)] +
          (description == "SCALE" && index == 13
             ? ". FREE 0.1 bypasses Key and scale quantisation; Shift-click or select Reset to Major."
             : ". Click to choose, drag/scroll to change; double-click Scale for Major."); }
    void paint(juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        const float diameter = juce::jmin(r.getHeight()-4.0f, r.getWidth()*0.48f);
        const float radius = juce::jmax(3.0f, diameter * 0.5f);
        const float cx = r.getX()+radius+3.0f, cy = r.getCentreY();
        g.setColour(bg.getPerceivedBrightness() > 0.5f ? bg.darker(0.40f) : bg.darker(0.18f)); g.fillEllipse(cx-radius, cy-radius, diameter, diameter);
        g.setColour(fg.withAlpha(0.35f)); g.drawEllipse(cx-radius, cy-radius, diameter, diameter, 1.1f);
        constexpr float from = juce::MathConstants<float>::pi * 0.75f;
        constexpr float extent = juce::MathConstants<float>::pi * 1.5f;
        juce::Path ring;
        const float proportion = (float) index / (float) juce::jmax(1, names.size()-1);
        ring.addCentredArc(cx, cy, radius-3.0f, radius-3.0f, 0.0f, from, from + extent*juce::jmax(0.005f,proportion), true);
        g.setColour(glow.brighter(0.35f)); g.strokePath(ring, juce::PathStrokeType(3.1f));
        const float a = from + extent * proportion;
        const float dotR = radius - 3.0f;
        g.setColour(glow.brighter(0.20f));
        g.fillEllipse(cx + std::sin(a)*dotR - 2.1f,
                      cy - std::cos(a)*dotR - 2.1f, 4.2f, 4.2f);
        g.setColour(fg); g.setFont(juce::Font(juce::FontOptions(juce::jmax(10.0f, r.getHeight()*0.25f))));
        g.drawFittedText(names[juce::jlimit(0, juce::jmax(0,names.size()-1), index)],
            juce::Rectangle<int>(juce::roundToInt(cx+radius+4), 0,
                juce::jmax(1, juce::roundToInt(r.getRight()-cx-radius-5)), getHeight()),
            juce::Justification::centredLeft, 1, 0.85f);
    }
    void mouseDown(const juce::MouseEvent& e) override
    { anchor = e.position; initial = index; dragged = false; }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (e.position.getDistanceFrom(anchor) >= 4.0f) dragged = true;
        if (dragged)
            update(initial + juce::roundToInt((anchor.y-e.position.y + e.position.x-anchor.x) / 10.0f));
    }
    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        if (description == "SCALE") update(1); // Major quick reset
        else if (description == "KEY") update(0); // C quick reset
    }
    void mouseUp(const juce::MouseEvent& e) override
    {
        // Clicking opens a discrete menu; Shift-click Scale reliably resets
        // Major even when the host's popup dismisses double-click events.
        if (dragged || (description != "SCALE" && description != "KEY")) return;
        if (e.mods.isShiftDown() && description == "SCALE") { update(1); return; }
        juce::PopupMenu menu;
        if (description == "SCALE")
        {
            menu.addItem(1001, "RESET TO MAJOR", true, index == 1);
            menu.addSeparator();
        }
        for (int i=0; i<names.size(); ++i)
            menu.addItem(i+1,names[i],true,i==index);
        juce::Component::SafePointer<PnordeChoiceDial> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [safe](int id) { if (safe != nullptr && id>0)
                safe->update(id == 1001 ? 1 : id-1); });
    }
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        // Ctrl/Cmd+wheel belongs to the editor's brightness shortcut.
        if (e.mods.isCtrlDown() || e.mods.isCommandDown())
        { juce::Component::mouseWheelMove(e, wheel); return; }
        update(index + (wheel.deltaY > 0 ? 1 : wheel.deltaY < 0 ? -1 : 0));
    }
private:
    void update(int value)
    {
        const int before = index;
        setIndex(value);
        if (index != before && onChange) onChange(index);
    }
    juce::StringArray names;
    juce::String description;
    bool tooltipsEnabled = true;
    bool dragged = false;
    int index = 0, initial = 0;
    juce::Point<float> anchor;
    juce::Colour bg {0xff27374a}, fg {0xfff3e9d9}, glow {0xff78d7d8};
};

// Compact, transparent, discrete dials. No opaque rectangular backing, no
// gradient caches, no repaint timer. The host draws each only on state/theme
// changes. The editor may redraw JUST the coloured ring above experimental dimming.
class PnordeLaneDial : public juce::Component, public juce::TooltipClient
{
public:
    std::function<void(int)> onChange;
    std::function<void()> onContextMenu;
    void setup(int maximum, int initial, const char* const* labels, const char* purpose)
    {
        maxValue=maximum; names=labels; description=purpose; setValue(initial);
    }
    void setValue(int value)
    {
        value=juce::jlimit(0,maxValue,value);
        if (value!=index) { index=value; repaint(); }
    }
    int getValue() const noexcept { return index; }
    void setTooltipsEnabled(bool enabled) noexcept { tooltipsEnabled=enabled; }
    juce::String getTooltip() override
    {
        return tooltipsEnabled ? description + ": " + names[index] + ". Drag up/down or scroll; double-click resets." : juce::String();
    }
    void setPalette(juce::Colour paper, juce::Colour text, juce::Colour laneColour)
    { bg=paper; fg=text; accent=laneColour; repaint(); }
    void paint(juce::Graphics& g) override { drawFace(g,0.0f,0.0f,false); }
    void drawIllumination(juce::Graphics& g,float x,float y) const
    { drawFace(g,x,y,true); }
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { if (onContextMenu) onContextMenu(); return; }
        original=index; origin=e.position;
    }
    void mouseDrag(const juce::MouseEvent& e) override
    { if (e.mods.isLeftButtonDown()) update(original+juce::roundToInt((origin.y-e.position.y)/9.0f)); }
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        if (e.mods.isCtrlDown() || e.mods.isCommandDown())
        { juce::Component::mouseWheelMove(e, wheel); return; }
        update(index + (wheel.deltaY > 0 ? 1 : wheel.deltaY < 0 ? -1 : 0));
    }
    void mouseDoubleClick(const juce::MouseEvent&) override { update(defaultValue); }
protected:
    void setDefault(int v) noexcept { defaultValue=v; }
private:
    void drawFace(juce::Graphics& g, float x, float y, bool highlightsOnly) const
    {
        if (!isVisible() || getWidth() < 18 || getHeight() < 18 || names==nullptr) return;
        const float size=juce::jmin((float)getHeight()-2.0f,(float)getWidth()*0.79f);
        const float r=juce::jmax(3.0f,size*0.47f);
        const float cx=x+(float)getWidth()*0.50f;
        const float cy=y+(float)getHeight()*0.44f;
        // The face is circular in BOTH passes, never an undimmed rectangle.
        if (highlightsOnly)
            g.setColour(bg.getPerceivedBrightness() > 0.50f
                ? bg.darker(0.76f) : bg.brighter(0.18f));
        else
            g.setColour(bg.getPerceivedBrightness() > 0.50f ? bg.darker(0.48f) : bg.darker(0.15f));
        g.fillEllipse(cx-r,cy-r,2.0f*r,2.0f*r);
        if (!highlightsOnly)
        {
            g.setColour(fg.withAlpha(0.28f));
            g.drawEllipse(cx-r,cy-r,2.0f*r,2.0f*r,0.9f);
        }
        const float start=juce::MathConstants<float>::pi*0.75f;
        const float extent=juce::MathConstants<float>::pi*1.5f;
        const float factor=(float)index/(float)juce::jmax(1,maxValue);
        const bool hovered = isMouseOverOrDragging();
        juce::Path arc;
        arc.addCentredArc(cx,cy,r-2.2f,r-2.2f,0.0f,start,start+extent*juce::jmax(0.007f,factor),true);
        const float hoverBoost = hovered ? (highlightsOnly ? 1.20f : 0.70f) : 0.0f;
        g.setColour(accent.brighter((highlightsOnly?0.90f:0.35f) + hoverBoost));
        g.strokePath(arc,juce::PathStrokeType(highlightsOnly ? (hovered ? 3.6f : 3.0f) : (hovered ? 3.0f : 2.6f)));
        g.setColour(accent.brighter(hovered ? 0.95f : 0.63f));
        const float theta=start+extent*factor;
        g.fillEllipse(cx+std::sin(theta)*(r-2.2f)-1.7f,
                      cy-std::cos(theta)*(r-2.2f)-1.7f,3.4f,3.4f);
        // Value text is INSIDE the dial, bright in experimental darkness and
        // large enough to read on 1120px-wide editors. No rectangular light mask.
        g.setColour(highlightsOnly ? accent.brighter(0.95f) :
                    bg.getPerceivedBrightness() < 0.48f ? fg.brighter(0.40f) : fg);
        const float textSize=juce::jmax(11.0f, juce::jmin(16.0f, r * 0.54f));
        g.setFont(juce::Font(juce::FontOptions("Bahnschrift",textSize,juce::Font::bold)));
        g.drawFittedText(names[index],
            juce::Rectangle<int>(juce::roundToInt(cx-r+3.0f),juce::roundToInt(cy-r*0.45f),
                                 juce::jmax(10,juce::roundToInt(r*2.0f-6.0f)),
                                 juce::jmax(12,juce::roundToInt(r*0.90f))),
            juce::Justification::centred,1,0.75f);
    }
    void update(int value)
    {
        const int old=index; setValue(value);
        if (index!=old && onChange) onChange(index);
    }
    const char* const* names=nullptr;
    juce::String description;
    bool tooltipsEnabled=true;
    int index=-1,maxValue=0,defaultValue=0,original=0;
    juce::Point<float> origin;
    juce::Colour bg {0xff27374A}, fg {0xfff3e9d9}, accent {0xff78d7d8};
};

class PnordeClockDial final : public PnordeLaneDial
{
public:
    PnordeClockDial()
    { setup(10,2,labels,"Clock division"); setDefault(2); }
private:
    static constexpr const char* labels[]={"1/32","1/16","1/8","1/4","1/2","1","2","4","8","16","32"};
};
class PnordeTimingDial final : public PnordeLaneDial
{
public:
    PnordeTimingDial()
    { setup(2,0,labels,"Timing: S=Straight, T=Triplet, D=Dotted"); setDefault(0); }
private:
    static constexpr const char* labels[]={"S","T","D"};
};

// Statically illuminated page controls; remove-page becomes red on hover.
class PnordePageButton final : public juce::TextButton
{
public:
    void setWarning(bool yes) noexcept { warning=yes; }
    void paintButton(juce::Graphics& g, bool hovered, bool pressed) override
    {
        const auto r=getLocalBounds().toFloat().reduced(0.9f);
        const auto background=findColour(juce::TextButton::buttonColourId);
        const auto accent=findColour(juce::TextButton::buttonOnColourId);
        const auto red=juce::Colour(0xffed3348);
        const auto ink=warning && hovered ? red.brighter(0.5f)
            : findColour(juce::TextButton::textColourOffId);
        g.setColour(background);
        g.fillRoundedRectangle(r,4.0f);
        g.setColour(warning && hovered ? red : accent.withAlpha(isEnabled()?0.83f:0.32f));
        g.drawRoundedRectangle(r,4.0f,hovered?1.8f:1.0f);
        g.setColour(isEnabled()?ink:ink.withAlpha(0.25f));
        g.setFont(juce::Font(juce::FontOptions(juce::jmax(12.0f,r.getHeight()*0.64f))));
        g.drawText(getButtonText(),r.toNearestInt(),juce::Justification::centred);
        juce::ignoreUnused(pressed);
    }
private:
    bool warning=false;
};

class PnordeDeleteButton final : public juce::TextButton
{
public:
    PnordeDeleteButton() : juce::TextButton("X") {}
    void paintButton(juce::Graphics& g, bool hovered, bool pressed) override
    {
        auto r = getLocalBounds().toFloat().reduced(1.0f);
        const auto bg = findColour(juce::TextButton::buttonColourId);
        const auto fg = findColour(juce::TextButton::textColourOffId);
        const auto red = juce::Colour(0xffff3952);
        g.setColour(hovered ? red.withAlpha(pressed?0.92f:0.76f) : bg);
        g.fillRoundedRectangle(r, 5.0f);
        g.setColour(hovered ? red.brighter(0.7f) : fg.withAlpha(0.7f));
        g.drawRoundedRectangle(r, 5.0f, hovered?2.0f:1.0f);
        g.setFont(juce::Font(juce::FontOptions(juce::String("Bahnschrift"),
            juce::jmax(13.0f,r.getHeight()*0.58f), juce::Font::bold)));
        g.drawText("X", r.toNearestInt(), juce::Justification::centred);
    }
};
