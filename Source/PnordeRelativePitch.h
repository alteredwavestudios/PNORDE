// ============================================================================
// PnordeRelativePitch.h — Relative pitch quantisation
// ============================================================================
// Converts stored pitch offsets into musically useful positions while respecting key and scale.
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

#include "PnordeScales.h"
#include <cmath>
#include <limits>

// ============================================================================
// HOW PNORDE HANDLES KEY AND SCALE (BEGINNER EXPLANATION)
// ============================================================================
// A PITCH cell stores a number such as 40 (meaning 4.0 semitones above the
// selected key). THAT NUMBER BELONGS TO THE USER AND MUST NEVER BE OVERWRITTEN
// WHEN THE KEY OR SCALE CHANGES.
//
// There are three separate stages:
//   1. Read the original PITCH cell: 40 means +4 semitones.
//   2. Find the nearest note in the chosen scale. In minor, +4 becomes +3.
//   3. Transpose by the selected key. In D minor, +3 becomes +5 above C.
//
// For example, changing C major -> C minor -> C major produces
//       +4 -> +3 -> +4
// because each calculation starts with the ORIGINAL +4, not the previous
// answer. Similarly C major -> D major adds +2 to each final MIDI note.
//
// This file does calculations ONLY. No GUI, memory allocation, MIDI sending,
// file saving or locks happen here. That makes it safe in JUCE's audio callback.
namespace PnordeRelativePitch
{
    // C++ 'inline' allows this small function to be shared by multiple .cpp
    // files without creating duplicate definitions at link time.
    inline double scaleRelativeSemitones(int storedTenths, int scaleIndex) noexcept
    {
        // Original cells are measured in TENTHS of a semitone: 15 = 1.5.
        const int bounded = storedTenths < -120 ? -120 : storedTenths > 120 ? 120 : storedTenths;
        const double original = bounded / 10.0;

        // Chromatic and Free (0.1) do not restrict the allowed pitches.
        if (scaleIndex == 0 || scaleIndex == 13)
            return original;

        // All scale calculations below are relative to the key, where 0 means
        // the tonic. We add the chosen key only AFTER finding the scale degree.
        // Thus moving C -> D cannot accidentally quantise the melody back to C.
        double best = original;
        double closest = std::numeric_limits<double>::infinity();

        // Consider notes in neighbouring octaves, because -1 might be closer
        // to the lower tonic (-12) than to the upper tonic (0).
        const auto consider = [&](double candidate)
        {
            const double distance = std::abs(candidate - original);
            // Equal-distance ties favour the LOWER note, deterministically.
            // Example: B natural becomes Bb rather than C in C minor.
            if (distance < closest - 1.0e-9 ||
                (std::abs(distance - closest) <= 1.0e-9 && candidate < best))
            {
                best = candidate;
                closest = distance;
            }
        };

        if (PnordeEDO::isMode(scaleIndex))
        {
            // EDO = Equal Divisions of the Octave. Some degrees are BETWEEN
            // piano-key semitones, so we keep fractional precision here and
            // let the existing MIDI pitch-bend engine play the exact result.
            const auto& mode = PnordeEDO::mode(scaleIndex);
            for (int octave = -2; octave <= 2; ++octave)
                for (int degree : mode.degrees)
                    consider(12.0 * octave + 12.0 * degree / mode.edo);
        }
        else if (scaleIndex >= PnordeScales::legacyCount &&
                 scaleIndex < PnordeEDO::firstScaleIndex)
        {
            // Extra scales include pentatonics and quarter-tone approximations.
            // Their stored offsets are also measured in tenths of a semitone.
            const auto& scale = PnordeScales::extras[
                static_cast<std::size_t>(scaleIndex - PnordeScales::legacyCount)];
            for (int octave = -2; octave <= 2; ++octave)
                for (int degree = 0; degree < scale.size; ++degree)
                    consider(12.0 * octave +
                             scale.offsetsTenths[static_cast<std::size_t>(degree)] / 10.0);
        }
        else
        {
            // The first 13 scale modes were part of the original PNORDE.
            // A bit mask simply says which of the twelve piano notes are
            // allowed. '1 << semitone' tests whether one note is allowed.
            if (scaleIndex < 0 || scaleIndex >= 13)
                return original; // Unknown index: do not unexpectedly change pitch.
            for (int octave = -2; octave <= 2; ++octave)
                for (int note = 0; note < 12; ++note)
                    if ((PnordeScales::legacyMasks[static_cast<std::size_t>(scaleIndex)] & (1 << note)) != 0)
                        consider(12.0 * octave + note);
        }
        return best;
    }

    // This is what a PITCH cell should DISPLAY. The scale can move the
    // number to the nearest allowed scale degree, but the key does not.
    //
    // Example (raw PITCH +4.0):
    //   C major => display +4.0; D major => STILL display +4.0.
    //   C minor => display +3.0; D minor => STILL display +3.0.
    //
    // The original number is stored elsewhere and never overwritten by this
    // preview. Note that 'displaySemitones' has NO key argument on purpose.
    // Later, a MIDI note can select the key without changing the PITCH grid.
    inline double displaySemitones(int storedTenths, int scale) noexcept
    {
        return scaleRelativeSemitones(storedTenths, scale);
    }

    // This is what the sequencer should PLAY. It starts with exactly the
    // same scale-adjusted relative interval that the grid displays, then
    // shifts the sounding note by the selected key (C=0, D=2, etc.).
    // The separate OCTAVE lane is added later in processBlock().
    inline double outputSemitones(int storedTenths, int key, int scale) noexcept
    {
        const int safeKey = key < 0 ? 0 : key > 11 ? 11 : key;
        return safeKey + displaySemitones(storedTenths, scale);
    }
}
