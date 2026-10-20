#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include "PnordeEDO.h"

// Keep the first 14 indices unchanged for v0.5.x presets and automation.
// Quarter-tone modes are *approximations*, not full maqam intonation systems.
namespace PnordeScales
{
constexpr int legacyCount = 14;
constexpr int count = 34 + PnordeEDO::modeCount;
static constexpr const char* names[count] = {
    "Chromatic",
    "Major",
    "Minor",
    "Dorian",
    "Phrygian",
    "Locrian",
    "Blues",
    "Major Blues",
    "Bebop",
    "Harmonic Minor",
    "Pelog",
    "Melodic Minor",
    "Mixolydian",
    "Free (0.1)",
    "Major Pentatonic",
    "Minor Pentatonic",
    "Egyptian Pentatonic",
    "Hirajoshi",
    "Insen",
    "Iwato",
    "Kumoi",
    "Yo",
    "Lydian",
    "Aeolian",
    "Mixolydian b6",
    "Lydian Dominant",
    "Altered (Super Locrian)",
    "Half-Whole Diminished",
    "Whole-Half Diminished",
    "Whole Tone",
    "Phrygian Dominant (Hijaz 12-TET)",
    "Double Harmonic Major",
    "Rast (quarter-tone approximation)",
    "Bayati (quarter-tone approximation)",
    "17-EDO / Subminor Twilight",
    "17-EDO / Neutral Nocturne",
    "19-EDO / Velvet Minor",
    "19-EDO / Bittersweet Major 7",
    "22-EDO / Porcupine 7",
    "22-EDO / Pajara-inspired",
    "22-EDO / Unresolved Cloud",
    "24-EDO / Neutral Minor",
    "24-EDO / Quartertone Haze",
    "24-EDO / Hijaz-inspired",
    "31-EDO / Meantone Minor",
    "31-EDO / Septimal Longing",
    "31-EDO / Neutral Dream",
};
struct Scale { std::array<int, 8> offsetsTenths; int size; };
static constexpr Scale extras[PnordeEDO::firstScaleIndex-legacyCount] = {
    {{0, 20, 40, 70, 90, 0, 0, 0}, 5}, // Major Pentatonic,
    {{0, 30, 50, 70, 100, 0, 0, 0}, 5}, // Minor Pentatonic,
    {{0, 20, 50, 70, 100, 0, 0, 0}, 5}, // Egyptian Pentatonic,
    {{0, 20, 30, 70, 80, 0, 0, 0}, 5}, // Hirajoshi,
    {{0, 10, 50, 70, 100, 0, 0, 0}, 5}, // Insen,
    {{0, 10, 50, 60, 100, 0, 0, 0}, 5}, // Iwato,
    {{0, 20, 30, 70, 90, 0, 0, 0}, 5}, // Kumoi,
    {{0, 20, 50, 70, 90, 0, 0, 0}, 5}, // Yo,
    {{0, 20, 40, 60, 70, 90, 110, 0}, 7}, // Lydian,
    {{0, 20, 30, 50, 70, 80, 100, 0}, 7}, // Aeolian,
    {{0, 20, 40, 50, 70, 80, 100, 0}, 7}, // Mixolydian b6,
    {{0, 20, 40, 60, 70, 90, 100, 0}, 7}, // Lydian Dominant,
    {{0, 10, 30, 40, 60, 80, 100, 0}, 7}, // Altered (Super Locrian),
    {{0, 10, 30, 40, 60, 70, 90, 100}, 8}, // Half-Whole Diminished,
    {{0, 20, 30, 50, 60, 80, 90, 110}, 8}, // Whole-Half Diminished,
    {{0, 20, 40, 60, 80, 100, 0, 0}, 6}, // Whole Tone,
    {{0, 10, 40, 50, 70, 80, 100, 0}, 7}, // Phrygian Dominant (Hijaz 12-TET),
    {{0, 10, 40, 50, 70, 80, 110, 0}, 7}, // Double Harmonic Major,
    {{0, 20, 35, 50, 70, 90, 105, 0}, 7}, // Rast (quarter-tone approximation),
    {{0, 15, 30, 50, 70, 80, 100, 0}, 7}, // Bayati (quarter-tone approximation)
};
// Quantise signed, tenths-of-a-semitone offsets around middle C.
inline int quantiseExtra(int valueTenths, int octave, int keyPc, int scaleIndex) noexcept
{
    if (PnordeEDO::isMode(scaleIndex))
        return PnordeEDO::quantiseTenths(valueTenths,octave,keyPc,scaleIndex);
    if (scaleIndex < legacyCount || scaleIndex >= PnordeEDO::firstScaleIndex) return valueTenths;
    const auto& shape = extras[static_cast<size_t>(scaleIndex-legacyCount)];
    const int baseTenths = (60 + octave*12)*10;
    const int target = baseTenths + valueTenths;
    const int oct = static_cast<int>(std::floor((target-keyPc*10)/120.0));
    int best = target;
    int distance = 0x7fffffff;
    for (int o=oct-2; o<=oct+2; ++o)
        for (int n=0; n<shape.size; ++n)
        {
            const int candidate = (o*12+keyPc)*10 + shape.offsetsTenths[static_cast<size_t>(n)];
            if (candidate < 0 || candidate > 1270) continue;
            const int d = std::abs(candidate-target);
            if (d < distance) { distance=d; best=candidate; }
        }
    const int shift = best-baseTenths;
    return shift < -120 ? -120 : shift > 120 ? 120 : shift;
}
}
