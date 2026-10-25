// ============================================================================
// PnordeScaleDegreeEdit.h — PNORDE ScaleDegreeEdit
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

#include "PnordeRelativePitch.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <limits>

// ============================================================================
// BEGINNER'S GUIDE: MOVING WHOLE PITCH PATTERNS WITHOUT LEAVING THE SCALE
// ============================================================================
// Imagine a staircase where each step is a note allowed by your chosen scale.
// CTRL+drag moves every PITCH cell the SAME number of staircase steps.
// In C major, going up one step moves C -> D and E -> F (different numbers
// of semitones, but the same number of SCALE DEGREES).
//
// We build the list of permitted pitches ONCE when the mouse is pressed.
// Nothing in this file changes the saved pattern, sends MIDI, takes a lock,
// or allocates memory. It just does predictable mathematical calculations.
namespace PnordeScaleDegreeEdit
{
    // PITCH is stored as tenths of a semitone. +/-120 means +/-12 semitones.
    constexpr int lowestPitch = -120;
    constexpr int highestPitch = 120;
    constexpr int maximumEntries = highestPitch - lowestPitch + 1;

    struct AllowedPitches
    {
        // A fixed-size array is a list that does NOT grow dynamically.
        // "count" tells us how much of that list we actually use.
        std::array<int, maximumEntries> values {};
        int count = 0;
    };

    // --------------------------------------------------------------------
    // Build a complete list of notes the musician is allowed to select.
    // --------------------------------------------------------------------
    // Earlier versions tried every 0.1-semitone number, asked the playback
    // quantiser to snap it, and removed repeated answers. That works for
    // most notes, but it is the WRONG SOURCE for an editor's staircase:
    // snapping/rounding can hide degrees or appear to jump over one.
    //
    // Instead, read the ACTUAL list of notes defining each scale and copy
    // those notes across neighbouring octaves (-24 to +24 semitones).
    // Filter to the pitch lane's legal range (-12..+12 semitones).
    // This is identical on both sides of 0, in the musical sense: each
    // octave repeats the same pitch classes. In C major, the note BELOW
    // C is B (-1), NOT Bb (-2), since Bb is not in C major.
    //
    // This runs ONCE at mouse-down in the editor, NOT during audio playback.
    // The fixed-size array does not allocate memory or interfere with MIDI.
    inline AllowedPitches buildAllowedPitches(int scale) noexcept
    {
        AllowedPitches result;
        const auto addNote = [&](double semitones)
        {
            // 0.1 semitone = PNORDE's smallest stored PITCH unit.
            const int value = static_cast<int>(std::lround(semitones * 10.0));
            if (value < lowestPitch || value > highestPitch) return;
            if (result.count < maximumEntries)
                result.values[static_cast<std::size_t>(result.count++)] = value;
        };

        // CHROMATIC: piano keys in one-semitone jumps.
        // FREE: every tenth of a semitone is permitted.
        if (scale == 0 || scale == 13)
        {
            const int stride = scale == 13 ? 1 : 10;
            for (int pitch = lowestPitch; pitch <= highestPitch; pitch += stride)
                result.values[static_cast<std::size_t>(result.count++)] = pitch;
            return result;
        }

        // The EDO modes use a fixed number of divisions per octave,
        // possibly falling BETWEEN the keys of a piano.
        if (PnordeEDO::isMode(scale))
        {
            const auto& mode = PnordeEDO::mode(scale);
            for (int octave = -2; octave <= 2; ++octave)
                for (int degree : mode.degrees)
                    addNote(12.0 * octave + 12.0 * degree / mode.edo);
        }
        // The extra named scales store semitones multiplied by ten.
        else if (scale >= PnordeScales::legacyCount &&
                 scale < PnordeEDO::firstScaleIndex)
        {
            const auto& mode = PnordeScales::extras[
                static_cast<std::size_t>(scale - PnordeScales::legacyCount)];
            for (int octave = -2; octave <= 2; ++octave)
                for (int degree = 0; degree < mode.size; ++degree)
                    addNote(12.0 * octave +
                        mode.offsetsTenths[static_cast<std::size_t>(degree)] / 10.0);
        }
        // Older (12-TET) scales are kept in the same masks as playback.
        else if (scale >= 1 && scale <= 12)
        {
            const int mask = PnordeScales::legacyMasks[
                static_cast<std::size_t>(scale)];
            for (int octave = -2; octave <= 2; ++octave)
                for (int semitone = 0; semitone < 12; ++semitone)
                    if ((mask & (1 << semitone)) != 0)
                        addNote(12.0 * octave + semitone);
        }
        else
        {
            // Invalid mode: do not leave the UI with an empty staircase.
            return buildAllowedPitches(0);
        }

        // Sorting puts the most negative pitch first. unique() removes a
        // duplicate if two musical notes happen to round to the same 0.1.
        // std::sort/unique work on the editor thread, NEVER the audio thread.
        auto end = result.values.begin() + result.count;
        std::sort(result.values.begin(), end);
        result.count = static_cast<int>(std::unique(result.values.begin(), end)
            - result.values.begin());
        return result;
    }

    inline int nearestIndex(const AllowedPitches& allowed, int storedPitch) noexcept
    {
        // The stored number may not be in the current scale (e.g. +4 in
        // minor). Find the closest *allowed* step. Equal ties go lower.
        if (allowed.count == 0) return 0;
        int bestIndex = 0;
        int bestDistance = std::numeric_limits<int>::max();
        for (int i = 0; i < allowed.count; ++i)
        {
            const int distance = std::abs(allowed.values[static_cast<std::size_t>(i)] - storedPitch);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = i;
            }
        }
        return bestIndex;
    }

    // Each cell has a limit. Choose ONE permitted movement for the group,
    // stopping when the FIRST cell meets a boundary. This keeps the musical
    // distances between cells intact instead of squashing them independently.
    inline int boundedGroupDelta(const int* original, int count,
                                 int proposed, int lowest, int highest) noexcept
    {
        if (count <= 0) return 0;
        int minimumDelta = std::numeric_limits<int>::min();
        int maximumDelta = std::numeric_limits<int>::max();
        for (int i = 0; i < count; ++i)
        {
            if (lowest - original[i] > minimumDelta)
                minimumDelta = lowest - original[i];
            if (highest - original[i] < maximumDelta)
                maximumDelta = highest - original[i];
        }
        if (minimumDelta > maximumDelta) return 0;
        return proposed < minimumDelta ? minimumDelta
             : proposed > maximumDelta ? maximumDelta : proposed;
    }
}
