#pragma once
#include <array>
#include <cstdint>

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
}
