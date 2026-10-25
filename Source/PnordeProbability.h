// ============================================================================
// PnordeProbability.h — Deterministic probability decisions
// ============================================================================
// Chooses whether a step passes its probability check without allocating, locking, or consulting unstable global random state.
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
#include <cstdint>

// ================================================================
// PER-CELL PROBABILITY -- plain C++, no JUCE required
// ================================================================
// Each lane has its OWN clock. Calling a random-number generator each
// time Live asks for a MIDI value would make a 50% cell flicker between
// accepted and rejected 200 times per second. Instead we derive a stable
// number from the lane number and clock tick. Every call for the same
// tick gets the SAME answer, so playback is reproducible and fast.
//
// 100 is the default: every cell behaves EXACTLY as in older PNORDE builds.
// Zero NEVER accepts its cell, and 50 accepts about half its visits.
//
// All calculations here use integers and don't allocate memory. This is
// safe for the time-critical audio processing thread.
namespace PnordeProbability
{
inline bool passes(int percentage, int row, std::int64_t tick) noexcept
{
    if (percentage >= 100) return true;
    if (percentage <= 0) return false;
    // A 64-bit mixing function scrambles sequential ticks into values
    // that behave randomly. Multiplication of unsigned integers can
    // wrap safely; that's normal and intended here.
    std::uint64_t x = static_cast<std::uint64_t>(tick) ^
        (0x9e3779b97f4a7c15ULL * static_cast<std::uint64_t>(row + 1));
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27; x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return static_cast<int>(x % 100ULL) < percentage;
}
}
