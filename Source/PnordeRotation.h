// ============================================================================
// PnordeRotation.h — PNORDE Rotation
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
// ===========================================================================
// PNORDE PATTERN ROTATION -- A tiny, JUCE-INDEPENDENT maths helper.
// ===========================================================================
// Imagine the eight stored cells as cards numbered 0, 1, 2, ... 7.
// Setting rotation to +2 DOES NOT move or rewrite any card. Instead, when
// the sequencer reaches visible position 2, it READS stored card 0.
// Every note, probability, CC glide and vibrato depth stays attached to its
// original card. Setting rotation back to zero reveals the exact originals.
//
// This helper makes positive rotations move the pattern to the RIGHT.
// All maths stays bounded: no heap allocation, no locks, no slow GUI work.
namespace PnordeRotation
{
    constexpr int wrap(int step, int length) noexcept
    {
        if (length <= 0) return 0;
        const int wrapped = step % length;
        return wrapped < 0 ? wrapped + length : wrapped;
    }

    // Position in the pattern -> the stored cell we should actually read.
    constexpr int sourceIndex(int visibleStep, int length, int rotation) noexcept
    {
        return wrap(visibleStep - rotation, length);
    }

    // Stored cell -> where it appears in the rotated on-screen pattern.
    constexpr int visibleIndex(int storedStep, int length, int rotation) noexcept
    {
        return wrap(storedStep + rotation, length);
    }
}
