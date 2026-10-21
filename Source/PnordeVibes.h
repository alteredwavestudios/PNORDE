#pragma once

// PNORDE HPPD v0.6.13. UI-ONLY, selected-effect-only architecture.
// One polymorphic renderer, one bounded ARGB frame, fixed-size musical snapshot.
// All work happens on the JUCE message thread, never in processBlock().
// update() obeys the editor's user-selected 1-60 FPS visual deadline.
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <memory>

namespace PnordeVibes
{
constexpr int maxLanes = 21;
constexpr float tau = juce::MathConstants<float>::twoPi;

struct LaneSample
{
    int step = 0;
    int length = 16;
    int division = 2;
    int value = 0;
    float phase = 0.0f;
    bool enabled = false;
};

struct Snapshot
{
    std::array<LaneSample, maxLanes> lanes {};
    double ppq = 0.0;
    bool running = false;
    int aligned = 0;
    std::array<juce::Colour, 8> palette {};
};

// A Japanese-inspired chromatic accent for each existing theme; keep hue identity.
// The shared sharp-light rule is a SINGLE bright stroke, not expensive fuzzy bloom.
inline juce::Colour neon(const Snapshot& s, int lane, float alpha)
{
    const auto source = s.palette[static_cast<size_t>((lane * 3 + lane / 5) % 8)];
    const float hue = source.getHue();
    const float sat = juce::jlimit(0.52f, 0.90f, source.getSaturation() * 1.13f);
    return juce::Colour::fromHSV(hue, sat, 1.0f,
        juce::jlimit(0.0f, 1.0f, alpha));
}

inline void glow(juce::Graphics& g, const juce::Path& path,
                 juce::Colour colour, float width = 1.6f)
{
    g.setColour(colour);
    g.strokePath(path, juce::PathStrokeType(width));
}

inline void glowingNode(juce::Graphics& g, float x, float y,
                        juce::Colour colour, float r)
{
    g.setColour(colour);
    g.fillEllipse(x-r, y-r, r*2.0f, r*2.0f);
}

inline void atmosphere(juce::Graphics& g, int w, int h, float pulse,
                       juce::Colour first, juce::Colour second)
{
    // Gently shifting coloured ambience, transparent for UI legibility.
    juce::ColourGradient wash(first.withAlpha(0.17f), w * (0.23f + 0.08f * pulse), 0.0f,
                              second.withAlpha(0.22f), w * (0.80f - 0.08f * pulse),
                              static_cast<float>(h), false);
    wash.addColour(0.52, first.interpolatedWith(second, 0.5f).withAlpha(0.095f));
    g.setGradientFill(wash);
    g.fillRect(0, 0, w, h);
}

class Effect
{
public:
    virtual ~Effect() = default;
    virtual void render(juce::Graphics&, int width, int height, const Snapshot&) = 0;
};

// EFFECT 1: NEON NODAL. Sixfold mirrored moving nodes and convergent orbits.
class NeonNodal final : public Effect
{
public:
    void render(juce::Graphics& g, int w, int h, const Snapshot& s) override
    {
        const float t = static_cast<float>(std::fmod(s.ppq * 0.018, 1.0));
        atmosphere(g, w, h, t, s.palette[3], s.palette[5]);
        const float cx = w * 0.5f, cy = h * 0.5f;
        // Diagonal-relative radius fills ultrawide backgrounds; same seven-lane budget.
        const float rMax = std::sqrt(float(w*w + h*h)) * 0.78f;
        for (int ring = 1; ring <= 8; ++ring)
        {
            const float r = rMax * static_cast<float>(ring) / 9.0f;
            juce::Path orbit;
            orbit.addEllipse(cx-r, cy-r, 2.0f*r, 2.0f*r);
            glow(g, orbit, neon(s, ring * 3, 0.18f), 1.0f);
        }
        // All enabled lanes feed the drawing, not merely the first seven.
        for (int row = 0; row < maxLanes; ++row)
        {
            const auto& l = s.lanes[static_cast<size_t>(row)];
            if (!l.enabled) continue;
            const float fraction = static_cast<float>(l.step) / static_cast<float>(l.length);
            const float relative = juce::jlimit(0.0f, 1.0f, std::abs(l.value) / 127.0f);
            // Slight asymmetric orbital precession for an uncanny, unstable lattice.
            const float angle = tau * (fraction + 0.03f * row + l.phase * 0.09f)
                + 0.07f * std::sin(tau * t + row * 0.41f);
            const float radius = rMax * (0.25f + 0.66f * (row + 1.0f) / (maxLanes + 1.0f));
            const float beat = s.running ? std::exp(-8.0f * l.phase) : 0.0f;
            const auto colour = neon(s, row, juce::jlimit(0.0f, 1.0f, 0.69f + 0.13f * relative + 0.18f * beat));
            for (int sector = 0; sector < 6; ++sector)
            {
                const float a = angle + tau * sector / 6.0f;
                const float x = cx + std::cos(a) * radius;
                const float y = cy + std::sin(a) * radius;
                juce::Path ray;
                ray.startNewSubPath(cx + std::cos(a) * radius * 0.42f,
                                    cy + std::sin(a) * radius * 0.42f);
                ray.lineTo(x, y);
                glow(g, ray, colour, 1.8f + 1.6f * relative + 1.0f * beat);
                glowingNode(g, x, y, colour, 4.6f + 3.4f * relative + 2.8f * beat);
            }
        }
        // Symmetry convergence is visible but capped: no sudden white flashes.
        if (s.aligned >= 2)
        {
            const float r = 3.0f + juce::jmin(9, s.aligned) * 1.5f;
            glowingNode(g, cx, cy, neon(s, s.aligned, 0.35f), r);
        }
    }
};

// EFFECT 2: PHASE WEAVE. Harmonic ribbons, NOT orbit/kaleidoscope geometry.
// Wave frequency derives from length and division; phase, pitch/velocity and
// all active modulation lanes change movement, shape, hue, and thickness.
class PhaseWeave final : public Effect
{
public:
    void render(juce::Graphics& g, int w, int h, const Snapshot& s) override
    {
        const float t = static_cast<float>(std::fmod(s.ppq * 0.011, 1.0));
        atmosphere(g, w, h, t, s.palette[6], s.palette[1]);
        for (int row = 0; row < maxLanes; ++row)
        {
            const auto& l = s.lanes[static_cast<size_t>(row)];
            if (!l.enabled) continue;
            const float division = static_cast<float>(juce::jlimit(0, 10, l.division));
            const float cycles = 1.0f + 9.0f / static_cast<float>(l.length) + division * 0.24f;
            const float phase = tau * (l.phase + l.step / static_cast<float>(l.length));
            const float magnitude = juce::jlimit(0.0f, 1.0f, std::abs(l.value) / 127.0f);
            const float baseline = h * (0.18f + (row + 0.5f) * 0.63f / maxLanes);
            const float amplitude = h * (0.016f + magnitude * 0.046f);
            juce::Path ribbon;
            constexpr int segments = 62; // fixed, bounded drawing cost
            for (int i = 0; i <= segments; ++i)
            {
                const float u = static_cast<float>(i) / segments;
                const float x = w * u;
                const float y = baseline + amplitude * std::sin(tau * cycles * u + phase)
                            + amplitude * 0.38f * std::sin(tau * u * 2.0f - phase * 0.45f)
                            + amplitude * 0.13f * std::sin(13.0f * u + 0.5f * phase);
                if (i == 0) ribbon.startNewSubPath(x, y);
                else ribbon.lineTo(x, y);
            }
            const auto colour = neon(s, row, 0.66f + magnitude * 0.21f);
            glow(g, ribbon, colour, 0.85f + 0.65f * magnitude +
                (s.running ? 1.0f * std::exp(-8.0f * l.phase) : 0.0f));
            const float xNode = w * static_cast<float>(l.step + 0.5f) / l.length;
            const float yNode = baseline + amplitude * std::sin(tau * cycles * (xNode/w) + phase)
                + amplitude * 0.38f * std::sin(tau * (xNode/w) * 2.0f - phase * 0.45f);
            glowingNode(g, xNode, yNode, colour, 1.9f + magnitude +
                (s.running ? 1.8f * std::exp(-8.0f * l.phase) : 0.0f));
        }
    }
};

// EFFECT 3: PRISM SPIKES. Triangles arrive FROM the four window edges.
// No central starburst: the triangles travel inward, moving as the lanes cycle.
class PrismSpikes final : public Effect
{
public:
    void render(juce::Graphics& g, int w, int h, const Snapshot& s) override
    {
        const float cx = w * 0.5f, cy = h * 0.5f;
        const float t = static_cast<float>(std::fmod(s.ppq * 0.014, 1.0));
        atmosphere(g, w, h, t, s.palette[3], s.palette[0]);
        for (int row = 0; row < maxLanes; ++row)
        {
            const auto& l = s.lanes[static_cast<size_t>(row)];
            if (!l.enabled) continue;
            const float level = juce::jlimit(0.0f, 1.0f, std::abs(l.value) / 127.0f);
            const float phase = (l.step + l.phase) / static_cast<float>(l.length);
            const float cycle = phase - std::floor(phase);
            for (int sector = 0; sector < 3; ++sector)
            {
                const int side = (row + sector) % 4;
                const float u = std::fmod(0.17f * row + 0.31f * sector +
                                          phase * 0.12f, 1.0f);
                const float ex = side == 0 ? 0.0f : side == 1 ? static_cast<float>(w)
                               : u * w;
                const float ey = side == 2 ? 0.0f : side == 3 ? static_cast<float>(h)
                               : u * h;
                const float tx = cx + std::sin(row * 1.2f + sector) * w * 0.10f;
                const float ty = cy + std::cos(row * 0.7f + sector) * h * 0.10f;
                const float advance = 0.08f + 0.73f * cycle;
                const float x = ex + (tx - ex) * advance;
                const float y = ey + (ty - ey) * advance;
                const float dx = tx - ex, dy = ty - ey;
                const float norm = juce::jmax(1.0f, std::sqrt(dx*dx + dy*dy));
                const float ux = dx/norm, uy = dy/norm;
                const float px = -uy, py = ux;
                const float size = 8.0f + 17.0f * level + 1.1f * l.division
                    + 2.2f * std::sin(tau * (cycle + 0.17f * sector));
                juce::Path triangle;
                triangle.startNewSubPath(x + ux * size, y + uy * size);
                triangle.lineTo(x - ux * size * 0.55f + px * size * 0.40f,
                                y - uy * size * 0.55f + py * size * 0.40f);
                triangle.lineTo(x - ux * size * 0.55f - px * size * 0.40f,
                                y - uy * size * 0.55f - py * size * 0.40f);
                triangle.closeSubPath();
                const auto colour = neon(s, row + sector, 0.85f);
                g.setColour(colour.withMultipliedAlpha(0.10f));
                g.fillPath(triangle);
                glow(g, triangle, colour, 0.9f + level * 0.55f +
                    (s.running ? 0.85f * std::exp(-8.0f * l.phase) : 0.0f));
            }
        }
    }
};

// EFFECT 4: UNCANNY CHESS. A skewed chessboard collapsing into a perspective
// vortex. Slow drifting optical unease rather than rapid luminance flashing.
// Fixed 12x8 tiles, no blur and no extra frame allocation.
class KumikoChecker final : public Effect
{
public:
    void render(juce::Graphics& g, int w, int h, const Snapshot& s) override
    {
        const float t = static_cast<float>(std::fmod(s.ppq * 0.00223, 1.0));
        atmosphere(g, w, h, t, s.palette[4], s.palette[3]);
        constexpr int cols = 12, rows = 8;
        const float cw = static_cast<float>(w) / cols;
        const float ch = static_cast<float>(h) / rows;
        const float cx = w * (0.52f + 0.0083f * std::sin(tau * t));
        const float cy = h * (0.49f + 0.02f * std::cos(tau * t));
        for (int y = 0; y < rows; ++y)
        {
            for (int x = 0; x < cols; ++x)
            {
                const int idx = (x * 7 + y * 11) % maxLanes;
                const auto& lane = s.lanes[static_cast<size_t>(idx)];
                const float phase = lane.enabled ? lane.phase : t;
                const float shift = lane.enabled ? (lane.step % 5) * 0.07f : 0.0f;
                auto warp = [&](float px, float py)
                {
                    const float nx = (px - cx) / juce::jmax(1.0f, float(w));
                    const float ny = (py - cy) / juce::jmax(1.0f, float(h));
                    const float ripple = 0.032f * std::sin(19.0f * (nx*nx + ny*ny)
                                              - tau * (t + phase * 0.13f) + shift);
                    const float skew = 0.035f * std::sin(ny * 9.0f + t * tau);
                    return juce::Point<float>(px + w * (ripple * nx + skew * ny),
                                              py + h * (ripple * ny - skew * nx));
                };
                const float x0 = x * cw, x1 = (x + 1) * cw;
                const float y0 = y * ch, y1 = (y + 1) * ch;
                const auto a = warp(x0, y0), b = warp(x1, y0);
                const auto c = warp(x1, y1), d = warp(x0, y1);
                juce::Path tile;
                tile.startNewSubPath(a); tile.lineTo(b); tile.lineTo(c);
                tile.lineTo(d); tile.closeSubPath();
                const auto col = neon(s, idx, 0.75f);
                if (((x + y) & 1) == 0)
                {
                    g.setColour(col.withMultipliedAlpha(0.23f));
                    g.fillPath(tile);
                }
                g.setColour(col.withMultipliedAlpha(0.82f));
                g.strokePath(tile, juce::PathStrokeType(0.85f +
                    (s.running ? 0.8f * std::exp(-8.0f * lane.phase) : 0.0f)));
                // Sparse diagonal scratches create an unstable, intricate chess motif.
                if (lane.enabled && ((x * 3 + y) % 7 == 0))
                {
                    juce::Path crack;
                    crack.startNewSubPath(a); crack.lineTo(c);
                    g.strokePath(crack, juce::PathStrokeType(0.75f));
                }
            }
        }
    }
};

// EFFECT 5: CHROMATIC DRIFT. A colour-first HPPD mode, not geometry:
// seven rotating lane samples set hue, gradient position, band spacing,
// saturation and rhythmic highlights. No new images, shaders or audio work.
class ChromaticDrift final : public Effect
{
public:
    void render(juce::Graphics& g, int w, int h, const Snapshot& s) override
    {
        float valueSum = 0.0f, phaseSum = 0.0f, divisionSum = 0.0f;
        float flash = 0.0f;
        int n = 0;
        for (const auto& lane : s.lanes)
        {
            if (!lane.enabled) continue;
            valueSum += juce::jlimit(0.0f, 1.0f, std::abs(lane.value) / 127.0f);
            phaseSum += lane.phase;
            divisionSum += lane.division / 10.0f;
            flash += s.running ? std::exp(-8.0f * lane.phase) : 0.0f;
            ++n;
        }
        if (n < 1) n = 1;
        const float level = valueSum / n;
        const float phase = phaseSum / n;
        const float division = divisionSum / n;
        const float hueShift = std::fmod(float(s.ppq) * 0.00117f + phase * 0.13f, 1.0f);
        const float brightness = juce::jlimit(0.0f, 1.0f, 0.35f + 0.27f * level + 0.17f * flash / n);
        auto colourAt = [&](int i, float a)
        {
            return s.palette[static_cast<size_t>(i % 8)]
                .withRotatedHue(hueShift + i * 0.037f)
                .withMultipliedSaturation(1.25f + 0.25f * division)
                .withAlpha(a);
        };
        const float drift = 0.20f * std::sin(tau * phase + float(s.ppq) * 0.0083f);
        const float x = w * (0.26f + drift);
        const float y = h * (0.18f + 0.22f * level);
        juce::ColourGradient gradient(colourAt(0, brightness), x, y,
            colourAt(5, brightness + 0.15f), w * (0.78f - drift), h * 0.92f, false);
        // Band spacing/gradation count adapt to division and active values.
        const int bands = juce::jlimit(3, 9, 3 + juce::roundToInt(5.0f * division + level));
        for (int i = 1; i < bands; ++i)
        {
            const float u = float(i) / bands;
            const float warped = juce::jlimit(0.01f, 0.99f,
                u + 0.055f * std::sin(tau * (u * (1.0f + division * 2.0f) + phase)));
            gradient.addColour(warped, colourAt(i+1, 0.54f + 0.35f * level));
        }
        g.setGradientFill(gradient);
        g.fillRect(0, 0, w, h);
        // Maximum three translucent soft colour pools. Large, gentle motion,
        // no rapidly flashing fills or offscreen buffers.
        for (int i = 0; i < 3; ++i)
        {
            const auto& l = s.lanes[static_cast<size_t>((i*3) % maxLanes)];
            const float a = tau * (phase + 0.31f * i + float(s.ppq) * 0.00047f);
            const float cx = w * (0.5f + 0.35f * std::cos(a));
            const float cy = h * (0.5f + 0.33f * std::sin(a * 0.73f));
            const float r = juce::jmax(float(w), float(h)) * (0.23f + level * 0.08f);
            juce::ColourGradient pool(colourAt(i+3, 0.20f + 0.09f * (l.enabled ? 1.0f : 0.0f)),
                 cx, cy, colourAt(i+3, 0.0f), cx+r, cy, true);
            g.setGradientFill(pool);
            g.fillEllipse(cx-r, cy-r, 2*r, 2*r);
        }
        // Subtle visual snow / feedback veil: sparse bright specks and thin
        // horizontal echoes react to the same musical phase without allocating
        // extra frame buffers or blur passes.
        const int specks = juce::jlimit(24, 90, 24 + juce::roundToInt(level * 54.0f + flash * 3.0f));
        for (int i = 0; i < specks; ++i)
        {
            const float u = std::fmod(0.6180339f * i + phase * 0.37f + hueShift * 0.53f, 1.0f);
            const float v = std::fmod(0.4142135f * i + division * 0.21f + float(s.ppq) * 0.0003f, 1.0f);
            const float x = u * w;
            const float y = v * h;
            const float r = 0.8f + 1.8f * std::fmod(level * 7.0f + i * 0.13f, 1.0f);
            g.setColour(colourAt(i, 0.06f + 0.10f * level + 0.08f * (i % 7 == 0)));
            g.fillEllipse(x - r, y - r, r * 2.0f, r * 2.0f);
        }
        for (int i = 0; i < 7; ++i)
        {
            const float yy = h * (0.08f + 0.12f * i + 0.03f * std::sin(tau * (phase + 0.12f * i)));
            g.setColour(colourAt(i + 2, 0.035f + 0.03f * level));
            g.fillRect(0.0f, yy, (float) w, 1.0f + (i % 3 == 0 ? 1.0f : 0.0f));
        }
    }
};

// Only the chosen Effect subclass and one frame buffer exist at a time.
// Frame allocated on demand, capped at 768x512 = 1.5 MiB ARGB.
class Renderer final
{
public:
    void setMode(int next)
    {
        next = juce::jlimit(0, 5, next);
        if (next == mode) return;
        // Destructor frees resources belonging to the old effect immediately.
        effect.reset();
        frame = {};
        dirty = true;
        width = height = 0;
        mode = next;
        if (mode == 1) effect = std::make_unique<NeonNodal>();
        if (mode == 2) effect = std::make_unique<PhaseWeave>();
        if (mode == 3) effect = std::make_unique<PrismSpikes>();
        if (mode == 4) effect = std::make_unique<KumikoChecker>();
        if (mode == 5) effect = std::make_unique<ChromaticDrift>();
    }

    void setPalette(const std::array<juce::Colour, 8>& next)
    {
        if (palette == next) return;
        palette = next;
        dirty = true;
    }

    void resize(int editorWidth, int editorHeight)
    {
        if (!effect) return;
        const int newW = juce::jlimit(1, 768, juce::roundToInt(editorWidth * 0.325f));
        const int newH = juce::jlimit(1, 512, juce::roundToInt(editorHeight * 0.325f));
        if (newW == width && newH == height) return;
        width = newW; height = newH;
        frame = juce::Image(juce::Image::ARGB, width, height, true);
        dirty = true;
    }

    template <typename Processor>
    void update(int editorWidth, int editorHeight, const Processor& p)
    {
        if (!effect) return;
        resize(editorWidth, editorHeight);
        if (!frame.isValid()) return;
        Snapshot s;
        s.palette = palette;
        s.ppq = p.getHostPpq();
        s.running = p.isHostPlaying();
        // Process seven of 21 lanes each half-second, round-robin. This
        // reduces rendering cost by about two thirds while preserving access
        // to every lane over a 1.5-second cycle. No buffers for inactive lanes.
        const int bank = s.running
            ? static_cast<int>(juce::Time::getMillisecondCounterHiRes() * 0.002) % 3
            : 0;
        for (int row = 0; row < maxLanes; ++row)
        {
            auto& l = s.lanes[static_cast<size_t>(row)];
            l.enabled = (row % 3 == bank) && (row < 7 || p.isOptionalLaneEnabled(row));
            if (!l.enabled) continue;
            l.length = juce::jmax(1, p.getLaneLength(row));
            l.step = juce::jlimit(0, l.length-1, p.getLaneCurrentStep(row));
            l.division = p.getLaneDivision(row);
            l.value = p.getLaneValue(row, l.step);
            // Per-lane phase uses its true subdivision/global clock/timing,
            // not the fractional part of the global PPQ. Visual flashes
            // therefore line up with each polymetric lane's own step cycle.
            static constexpr double divisionBeats[] {0.125, 0.25, 0.5, 1.0, 2.0,
                4.0, 8.0, 16.0, 32.0, 64.0, 128.0};
            static constexpr double globalFactors[] {4.0, 2.0, 1.0, 0.5, 0.25};
            static constexpr double timingFactors[] {1.0, 2.0 / 3.0, 1.5};
            const double beats = divisionBeats[juce::jlimit(0, 10, l.division)]
                * globalFactors[juce::jlimit(0, 4, p.getGlobalClockIndex())]
                * timingFactors[juce::jlimit(0, 2, p.getLaneTiming(row))];
            const double relative = (s.ppq - p.getLanePhaseOrigin(row)) /
                juce::jmax(0.00001, beats);
            const double frac = relative - std::floor(relative);
            l.phase = static_cast<float>(frac);
            if (l.step == 0) ++s.aligned;
        }
        frame.clear(frame.getBounds(), juce::Colours::transparentBlack);
        juce::Graphics g(frame);
        effect->render(g, width, height, s);
        dirty = false;
    }

    void paintBackground(juce::Graphics& g, juce::Rectangle<int> area, int intensity) const
    {
        if (!frame.isValid() || intensity <= 0) return;
        g.setOpacity(juce::jlimit(0.0f, 1.0f, 0.85f * intensity / 40.0f));
        g.drawImageWithin(frame, area.getX(), area.getY(), area.getWidth(), area.getHeight(),
                          juce::RectanglePlacement::stretchToFit);
        g.setOpacity(1.0f);
    }

    void paintOverlay(juce::Graphics& g, juce::Rectangle<int> area,
                      int bleed, int intensity) const
    {
        if (!frame.isValid() || bleed <= 0 || intensity <= 0) return;
        // Stronger post-darkness luminous layer: neon lines remain visibly emissive.
        // UI control fills/text are still drawn independently above/below as appropriate.
        g.setOpacity(juce::jlimit(0.0f, 0.82f,
            0.82f * bleed / 50.0f * intensity / 40.0f));
        g.drawImageWithin(frame, area.getX(), area.getY(), area.getWidth(), area.getHeight(),
                          juce::RectanglePlacement::stretchToFit);
        g.setOpacity(1.0f);
    }

    bool hasFrame() const noexcept { return frame.isValid() && !dirty; }
    int getMode() const noexcept { return mode; }
private:
    int mode = 0, width = 0, height = 0;
    bool dirty = true;
    std::unique_ptr<Effect> effect;
    juce::Image frame;
    std::array<juce::Colour, 8> palette {
        juce::Colour(0xffff6b86), juce::Colour(0xff8ca9ee),
        juce::Colour(0xff8bd0a7), juce::Colour(0xffc9a2f1),
        juce::Colour(0xffffc577), juce::Colour(0xff65d5ce),
        juce::Colour(0xffff91c7), juce::Colour(0xff6ce6fa) };
};
} // namespace PnordeVibes

using PnordeVibesRenderer = PnordeVibes::Renderer;
