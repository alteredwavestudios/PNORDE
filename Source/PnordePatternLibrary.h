// ============================================================================
// PnordePatternLibrary.h — PNORDE PatternLibrary
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
#include <array>
#include <cstdint>
#include <cmath>
#include <algorithm>

// Preset-like rhythm recipes. UI thread only. No audio-thread allocations,
// state mutations, or dynamic storage; fills 128-step scratch arrays.
namespace PnordePatternLibrary
{
constexpr int gateCount = 12;
constexpr int velocityCount = 10;
constexpr const char* gateNames[gateCount] = {
    "Trance Four-on-the-Floor", "Trance Offbeat", "Melodic Techno 3-3-2",
    "House Syncopation", "UK Garage Skips", "Breakbeat Push",
    "Euclidean 5/16", "Euclidean 7/16", "Ambient Sparse",
    "Dotted Pulse", "Polyrhythm 5 against 4", "Acid Sixteenths"
};
constexpr const char* velocityNames[velocityCount] = {
    "Trance Pump", "House Backbeat", "Ambient Breathing", "Humanised Soft",
    "Techno Accents", "Broken Beat", "Rolling Triplets", "Odd Accent 5",
    "Crescendo", "Decrescendo"
};
inline std::uint32_t hash(std::uint32_t x) noexcept;
constexpr int panCount = 12;
constexpr int slideCount = 10;
constexpr const char* panNames[panCount] = {
    "Ambient - Slow Orbit", "Ambient - Gentle Sway", "Ambient - Return to Centre",
    "Ambient - Long L/R Drift", "Trance - Alternating Bars", "Progressive - Crescent",
    "House - Offbeat Pan", "Garage - Synco-Pan", "Techno - 5-Step Orbit",
    "Experimental - Spiral", "Arpeggio - Ascending Stereo", "Dream - Tidal Breathing"
};
constexpr const char* slideNames[slideCount] = {
    "Instant (0%)", "Soft 10% Approach", "Ambient 25% Approach",
    "Half-Beat Swoop", "Full-Gate Portamento", "Trance Short Slides",
    "Syncopated Approaches", "Odd-Polyrhythm Glide", "Crescendo Slide",
    "Breathing Rubato"
};
// A pan recipe returns both its stereo position (CC10 0..127, 64 centre)
// and optional tempo-relative glide 0..100%.  Stereo pan is NOT binaural 3D.
inline void pans(std::array<int,128>& positions, std::array<int,128>& glide,
                 int length, int choice, std::uint32_t seed) noexcept
{
    if (length < 1 || length > 128 || choice < 0 || choice >= panCount) return;
    constexpr double pi = 3.14159265358979323846;
    for (int i = 0; i < length; ++i)
    {
        const double x = static_cast<double>(i);
        const double bar = static_cast<double>(i%16);
        double motion = 0.0;
        int glidePct = 45;
        switch (choice)
        {
          case 0: motion = std::sin(x * 2*pi / 64); glidePct=90; break;
          case 1: motion = 0.34*std::sin(x*2*pi/32); glidePct=80; break;
          case 2: motion = std::sin(x*2*pi/32) * std::abs(std::sin(x*pi/32)); glidePct=85; break;
          case 3: motion = std::sin(x*2*pi/128); glidePct=95; break;
          case 4: motion = ((i/16)%2==0 ? -0.82 : 0.82); glidePct=30; break;
          case 5: motion = std::sin(x*2*pi/48)*0.78; glidePct=65; break;
          case 6: motion = (i%4 == 2 ? 0.55 : i%4 == 0 ? -0.55 : 0.0); glidePct=28; break;
          case 7: motion = (i%6==0 ? -0.85 : i%6==3 ? 0.85 : 0.0); glidePct=32; break;
          case 8: motion = std::sin(x*2*pi/5)*0.75; glidePct=52; break;
          case 9: motion = std::sin(x*2*pi/11)*0.85; glidePct=60; break;
          case 10: motion = -0.9 + 1.8 * bar/15.0; glidePct=70; break;
          case 11: motion = 0.56*std::sin(x*2*pi/32) + 0.18*std::sin(x*2*pi/7); glidePct=75; break;
        }
        const int variation = static_cast<int>(hash(seed + static_cast<std::uint32_t>(i/16+17))%7)-3;
        const auto index = static_cast<std::size_t>(i);
        positions[index] = std::max(0,std::min(127,static_cast<int>(std::lround(64+motion*58))+variation));
        glide[index] = glidePct;
    }
}
inline void slides(std::array<int,128>& values, int length, int choice,
                   std::uint32_t seed) noexcept
{
    if (length < 1 || length > 128 || choice < 0 || choice >= slideCount) return;
    for (int i=0; i<length; ++i)
    {
        const int pulse = i%16;
        int v=0;
        switch (choice)
        {
          case 0: v=0; break;
          case 1: v=10; break;
          case 2: v=25; break;
          case 3: v=50; break;
          case 4: v=100; break;
          case 5: v=(pulse%4==3 ? 45 : 0); break;
          case 6: v=(pulse%3==0 ? 65 : 8); break;
          case 7: v=(i%5==0 ? 75 : 22); break;
          case 8: v=10+pulse*5; break;
          case 9: v=static_cast<int>(std::lround(38+32*std::sin(i*2*3.141592653589793/24.0))); break;
        }
        if (choice > 4) v += static_cast<int>(hash(seed + static_cast<std::uint32_t>(i/16+13))%7)-3;
        values[static_cast<std::size_t>(i)] = std::max(0,std::min(100,v));
    }
}
constexpr std::array<std::uint16_t, gateCount> gateMasks {{
    0x1111u, 0x2222u, 0x9249u, 0x2525u, 0xA48Au, 0xD292u,
    0x9249u, 0xB56Au, 0x8082u, 0x4444u, 0x8421u, 0xFFFFu
}};
inline std::uint32_t hash(std::uint32_t x) noexcept
{
    x ^= x << 13; x ^= x >> 17; x ^= x << 5; return x;
}
inline void gates(std::array<bool,128>& out, int length, int choice,
                  std::uint32_t seed) noexcept
{
    if (length < 1 || length > 128 || choice < 0 || choice >= gateCount) return;
    for (int i = 0; i < length; ++i)
    {
        const auto base = ((gateMasks[static_cast<std::size_t>(choice)] >> (i % 16)) & 1u) != 0;
        const auto noise = hash(seed + static_cast<std::uint32_t>(i * 1637 + 17));
        // Ambient Sparse adds infrequent variations across bars. Every other
        // groove repeats exactly by design until the user chooses another.
        out[static_cast<std::size_t>(i)] = choice == 8 && (i / 16) > 0
            ? (base && (noise % 5 != 0)) || ((noise % 23) == 0) : base;
    }
}
inline void velocities(std::array<int,128>& out, int length, int choice,
                       std::uint32_t seed) noexcept
{
    if (length < 1 || length > 128 || choice < 0 || choice >= velocityCount) return;
    for (int i = 0; i < length; ++i)
    {
        const int beat = i % 16;
        const int jitter = static_cast<int>(hash(seed + static_cast<std::uint32_t>(i*7717+13)) % 13) - 6;
        int value = 68;
        switch (choice)
        {
            case 0: value = (beat%4 == 0 ? 96 : beat%2 == 0 ? 72 : 47); break;
            case 1: value = (beat == 4 || beat == 12 ? 94 : beat%4 == 0 ? 83 : 54); break;
            case 2: value = 50 + (beat < 8 ? beat*4 : (15-beat)*4); break;
            case 3: value = 62 + jitter*2; break;
            case 4: value = (beat%4 == 0 ? 100 : beat%4 == 3 ? 78 : 59); break;
            case 5: value = (beat%3 == 0 ? 88 : beat%5 == 0 ? 75 : 48); break;
            case 6: value = (beat%3 == 0 ? 95 : beat%3 == 1 ? 62 : 53); break;
            case 7: value = (i%5 == 0 ? 96 : i%5 == 3 ? 79 : 50); break;
            case 8: value = 30 + beat*4; break;
            case 9: value = 94 - beat*4; break;
        }
        if (choice != 3 && choice != 8 && choice != 9) value += jitter;
        out[static_cast<std::size_t>(i)] = value < 1 ? 1 : value > 100 ? 100 : value;
    }
}

constexpr int aftertouchCount = 8;
constexpr const char* aftertouchNames[aftertouchCount] {
    "Breathing swell", "Offbeat accents", "Rising pressure",
    "Falling pressure", "Alternating pulses", "Sine waves",
    "Sparse gestures", "Organic drift"
};
inline void aftertouch(std::array<int, 128>& out, int length, int choice,
                       std::uint32_t seed) noexcept
{
    if (length < 1 || length > 128 || choice < 0 || choice >= aftertouchCount) return;
    for (int i = 0; i < length; ++i)
    {
        const int group = i % 16;
        const auto noise = hash(seed + static_cast<std::uint32_t>(i * 8677 + 31));
        int v = 0;
        switch (choice)
        {
            case 0: v = static_cast<int>(40 + 38 * (0.5 - 0.5 * std::cos(i * 0.39))); break;
            case 1: v = group % 4 == 2 ? 94 : group % 4 == 0 ? 55 : 8; break;
            case 2: v = 8 + group * 6; break;
            case 3: v = 98 - group * 6; break;
            case 4: v = (i % 2 == 0) ? 86 : 10; break;
            case 5: v = static_cast<int>(50 + 44 * std::sin(i * 0.25)); break;
            case 6: v = (noise % 7 == 0) ? 75 + static_cast<int>(noise % 26) : 0; break;
            case 7: v = static_cast<int>(45 + 23 * std::sin(i * 0.45)
                      + static_cast<int>(noise % 33) - 16); break;
        }
        out[static_cast<std::size_t>(i)] = std::max(0, std::min(100, v));
    }
}

}
