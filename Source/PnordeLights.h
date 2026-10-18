#pragma once
#include <JuceHeader.h>
#include <cmath>

// Message-thread-only, allocation-free drawing helpers. No timer or transport
// state is owned here: all pulse phases come from the processor's musical clock.
// Keeping one renderer for accents, outlines and indicators avoids duplicate
// glow paths and makes their intensity consistent across colour themes.
struct PnordeLights final
{
    static float pulse(double elapsedMs, double stepMs) noexcept
    {
        // Analogue LED: immediate attack, fast exponential release. Slower lanes
        // get slightly longer pulses, but even a very slow division will not
        // leave a light glaring for multiple seconds.
        const double window = juce::jlimit(70.0, 330.0, stepMs * 0.24);
        return static_cast<float>(std::exp(-4.6 * juce::jmax(0.0, elapsedMs) / window));
    }

    static float shimmer(double phase, int row, double cycles = 1.0) noexcept
    {
        // Decorative secondary glow, staggered by lane. It never changes
        // the actual MIDI clock or the main beat-boundary attack.
        const double stagger = std::fmod(0.173 * row + 0.17, 1.0);
        return 0.5f + 0.5f * static_cast<float>(std::sin(
            juce::MathConstants<double>::twoPi * (phase * cycles + stagger)));
    }

    static void draw(juce::Graphics& g, juce::Rectangle<float> shape,
                     juce::Colour colour, float pulseStrength, float darkness,
                     float glowScale = 1.0f, float corner = 4.0f)
    {
        if (shape.isEmpty()) return;
        const float p = juce::jlimit(0.0f, 1.0f, pulseStrength);
        const float dark = juce::jlimit(0.0f, 0.75f, darkness);
        // The darker the UI, the greater the apparent luminous intensity;
        // the glow remains theme-coloured, but the LED core flashes to white.
        const float bloom = p * (0.58f + dark * 0.50f) * glowScale;
        if (bloom > 0.006f)
        {
            g.setColour(colour.withAlpha(juce::jlimit(0.0f, 0.28f,
                bloom * (0.10f + dark * 0.16f))));
            g.fillRoundedRectangle(shape.expanded(6.0f * glowScale), corner + 6.0f);
            g.setColour(colour.withAlpha(juce::jlimit(0.0f, 0.50f,
                bloom * (0.21f + dark * 0.20f))));
            g.fillRoundedRectangle(shape.expanded(2.5f * glowScale), corner + 2.5f);
        }
        g.setColour(colour.interpolatedWith(juce::Colours::white,
            juce::jlimit(0.0f, 1.0f, p * (0.86f + dark * 0.18f))));
        g.fillRoundedRectangle(shape, corner);
    }

    // Low-cost emissive cell pass. Invoked AFTER the global dimmer, so both
    // filled parameter bars and unfilled circuit traces remain luminous.
    // Only the active beat uses the expensive halo renderer above.
    static void cellBackdrop(juce::Graphics& g, juce::Rectangle<float> rect,
                             juce::Colour colour, float darkness,
                             float travellingGlow, bool enabled)
    {
        if (rect.isEmpty()) return;
        const float dark = juce::jlimit(0.0f, 0.75f, darkness);
        const float motion = juce::jlimit(0.0f, 1.0f, travellingGlow);
        const float alpha = juce::jlimit(0.0f, 0.32f,
            0.018f + dark * 0.055f + motion * (0.11f + dark * 0.12f));
        g.setColour(colour.withAlpha(alpha));
        g.fillRoundedRectangle(rect.reduced(0.8f), 4.0f);
        if (motion > 0.025f || enabled)
        {
            const float edge = juce::jlimit(0.0f, 0.65f,
                0.045f + (enabled ? 0.12f : 0.0f) +
                motion * (0.33f + dark * 0.24f));
            g.setColour(colour.interpolatedWith(juce::Colours::white,
                motion * 0.30f).withAlpha(edge));
            g.drawRoundedRectangle(rect.reduced(0.8f), 4.0f, 1.0f);
        }
    }

    static void cellFill(juce::Graphics& g, juce::Rectangle<float> bar,
                         juce::Colour colour, float darkness,
                         float travellingGlow, float beatPulse)
    {
        if (bar.isEmpty() || bar.getWidth() < 0.1f || bar.getHeight() < 0.1f) return;
        const float dark = juce::jlimit(0.0f, 0.75f, darkness);
        const float motion = juce::jlimit(0.0f, 1.0f, travellingGlow);
        const float pulse = juce::jlimit(0.0f, 1.0f, beatPulse);
        const float alpha = juce::jlimit(0.0f, 0.96f,
            0.08f + dark * 0.72f + motion * 0.19f + pulse * 0.12f);
        g.setColour(colour.interpolatedWith(juce::Colours::white,
            juce::jlimit(0.0f, 0.92f, pulse * 0.76f + motion * 0.14f))
            .withAlpha(alpha));
        g.fillRoundedRectangle(bar, juce::jmin(3.0f, bar.getHeight() * 0.5f));
    }

    // Lightweight, allocation-free halo for labels. Heavy bloom is reserved
    // for the main title and the LДNTERNS signature; cell labels use only
    // two draw passes so the UI can animate at full speed.
    static void text(juce::Graphics& g, const juce::String& value,
                     juce::Rectangle<int> bounds, juce::Font font,
                     juce::Colour colour, float intensity, float darkness,
                     juce::Justification alignment = juce::Justification::centredLeft,
                     int spread = 1)
    {
        if (value.isEmpty() || bounds.isEmpty()) return;
        const float p = juce::jlimit(0.0f, 1.0f, intensity);
        const float d = juce::jlimit(0.0f, 0.75f, darkness);
        g.setFont(font);
        // Outer halo is cheap and subtle for ordinary labels. The signature
        // deliberately gets wider, more saturated falloff with 3 shells.
        if (spread > 0 && p > 0.025f)
        {
            const int shells = juce::jlimit(1, 3, spread);
            for (int shell = shells; shell >= 1; --shell)
            {
                const int radius = spread >= 3 ? shell * 2 : shell;
                const float alpha = (0.032f + d * 0.055f) * p / shell;
                g.setColour(colour.withAlpha(alpha));
                g.drawText(value, bounds.translated(-radius, 0), alignment);
                g.drawText(value, bounds.translated(radius, 0), alignment);
                g.drawText(value, bounds.translated(0, -radius), alignment);
                g.drawText(value, bounds.translated(0, radius), alignment);
            }
        }
        // Emissive core goes on top of DARKNESS. Theme colour is retained,
        // with a slight whitening when the clock pulse reaches full intensity.
        const auto core = colour.interpolatedWith(juce::Colours::white,
            juce::jlimit(0.0f, 0.73f, 0.18f + p * (0.26f + d * 0.42f)));
        g.setColour(core.withAlpha(juce::jlimit(0.0f, 1.0f,
            0.68f + p * 0.25f + d * 0.08f)));
        g.drawText(value, bounds, alignment);
    }

    // Saturated, extra-wide one-off sign glow: intended for the footer credit.
    // Drawn above the dimmer; no blur textures or heap allocation each frame.
    static void signature(juce::Graphics& g, const juce::String& name,
                          juce::Rectangle<int> bounds, juce::Font type,
                          juce::Colour glowColour, float pulse, float darkness)
    {
        if (bounds.isEmpty()) return;
        const float p = juce::jlimit(0.0f, 1.0f, pulse);
        const float d = juce::jlimit(0.0f, 0.75f, darkness);
        g.setFont(type);
        for (int radius : { 10, 6, 3 })
        {
            const float falloff = radius == 10 ? 0.035f : radius == 6 ? 0.060f : 0.095f;
            g.setColour(glowColour.withAlpha(
                juce::jlimit(0.0f, 0.22f, falloff * (0.53f + p + d * 0.60f))));
            for (int dx : { -1, 0, 1 })
                for (int dy : { -1, 0, 1 })
                    if (dx != 0 || dy != 0)
                        g.drawText(name, bounds.translated(dx * radius, dy * radius),
                                   juce::Justification::centredRight);
        }
        // Coloured centre preserves strong saturation at maximum darkness.
        g.setColour(glowColour.interpolatedWith(juce::Colours::white,
            0.15f + p * 0.29f).withAlpha(0.94f));
        g.drawText(name, bounds, juce::Justification::centredRight);
    }

    static float offsetPulse(float phase, float phaseOffset, double stepMs) noexcept
    {
        if (!std::isfinite(phase) || !std::isfinite(stepMs) || stepMs <= 0.0)
            return 0.0f;
        const float shifted = phase + phaseOffset;
        const float wrapped = shifted - std::floor(shifted);
        return pulse(wrapped * stepMs, stepMs);
    }

    static float softPulse(float phase, float offset) noexcept
    {
        const float x = phase + offset;
        return 0.5f + 0.5f * std::sin(
            juce::MathConstants<float>::twoPi * x);
    }

    static void outline(juce::Graphics& g, juce::Rectangle<float> rect,
                        juce::Colour colour, float intensity, float darkness,
                        float radius = 5.0f)
    {
        const float p = juce::jlimit(0.0f, 1.0f, intensity);
        if (p < 0.01f || rect.isEmpty()) return;
        if (p > 0.14f)
        {
            g.setColour(colour.withAlpha(juce::jlimit(0.0f, 0.45f,
                p * (0.11f + darkness * 0.20f))));
            g.drawRoundedRectangle(rect.expanded(2.5f), radius + 2.0f, 3.0f);
        }
        g.setColour(colour.interpolatedWith(juce::Colours::white,
            juce::jlimit(0.0f, 1.0f, p * (0.68f + darkness * 0.24f)))
            .withAlpha(juce::jlimit(0.0f, 1.0f, 0.38f + p * 0.62f)));
        g.drawRoundedRectangle(rect, radius, 1.3f + p * 1.5f);
    }
};
