// ============================================================================
// PnordeScales.h — Scale interval definitions
// ============================================================================
// Pitch-class masks and non-12-tone scale data; degree and key calculations must match the UI and generator.
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
#include <cmath>
#include <cstdint>
#include "PnordeEDO.h"

// Keep the first 14 indices unchanged for v0.5.x presets and automation.
// Quarter-tone modes are *approximations*, not full maqam intonation systems.
namespace PnordeScales
{
// The original 13 keyboard scales are defined here ONCE, so PITCH editing
// and MIDI playback can never accidentally use different allowed notes.
// Each bit corresponds to a note within the octave: bit 0=C, bit 1=C#,
// bit 2=D and so on. Example: Major contains 0,2,4,5,7,9,11.
// IMPORTANT: do not reorder indices; old presets refer to them by number.
constexpr std::array<int, 13> legacyMasks {{
    0xfff, 0xab5, 0x5ad, 0x6ad, 0x5ab, 0x56b, 0x4e9,
    0x29d, 0xebd, 0x9ad, 0x18b, 0xaad, 0x6b5
}};
// STAGE 3 — One authoritative definition of each legacy scale.
// A bit mask has one bit per semitone. For example, the historic Pelog mask
// 0x18b has FIVE active notes: C, C#, Eb, G and Ab.
//
// The composition generator previously kept its own handwritten arrays of
// scale degrees. Two of them (Bebop and Pelog) differed from these masks,
// so some newly generated notes were silently snapped to OTHER notes during
// actual MIDI playback. These tiny constexpr helpers let the generator read
// the original masks directly, without adding any runtime memory allocations.
//
// constexpr means the compiler can work this out during compilation; it
// doesn't mean a calculation must run each time Ableton plays a note.
constexpr int legacyDegreeCount(int scaleIndex) noexcept
{
    if (scaleIndex < 0 || scaleIndex >= static_cast<int>(legacyMasks.size()))
        return 0;
    int count = 0;
    for (int note = 0; note < 12; ++note)
        if ((legacyMasks[static_cast<std::size_t>(scaleIndex)] & (1 << note)) != 0)
            ++count;
    return count;
}

constexpr int legacyDegreeSemitone(int scaleIndex, int degreeIndex) noexcept
{
    if (scaleIndex < 0 || scaleIndex >= static_cast<int>(legacyMasks.size())
        || degreeIndex < 0)
        return 0;
    for (int note = 0, found = 0; note < 12; ++note)
    {
        if ((legacyMasks[static_cast<std::size_t>(scaleIndex)] & (1 << note)) == 0)
            continue;
        if (found == degreeIndex) return note;
        ++found;
    }
    return 0; // Invalid degree: safe fallback, never index outside an array.
}

static_assert(legacyDegreeCount(8) == 9, "Historic Bebop mask has nine notes");
static_assert(legacyDegreeCount(10) == 5, "Historic Pelog mask has five notes");

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
