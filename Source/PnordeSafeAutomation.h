// ============================================================================
// PnordeSafeAutomation.h — PNORDE SafeAutomation
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
#include <cmath>

// These helpers turn a floating-point value from the DAW into a small,
// safe integer for a PNORDE control.
//
// Why is this necessary?  A float can represent NaN ("not a number") or
// infinity. C++ does NOT guarantee a useful result when either is converted
// directly to int. Checking the number FIRST avoids that unsafe conversion.
//
// All valid values keep the old behaviour: divisions and directions truncate
// fractional values toward zero; Swing rounds to the nearest integer.
// These are pure functions: no locks, allocations, JUCE or host calls.
namespace PnordeSafeAutomation
{
inline int truncate(float value, int minimum, int maximum, int fallback) noexcept
{
    if (!std::isfinite(value)) return fallback;
    // Compare BEFORE casting, so even a gigantic finite float is safe.
    if (value <= static_cast<float>(minimum)) return minimum;
    if (value >= static_cast<float>(maximum)) return maximum;
    const int integer = static_cast<int>(value); // safe: bounded + finite
    return integer < minimum ? minimum : (integer > maximum ? maximum : integer);
}

inline int round(float value, int minimum, int maximum, int fallback) noexcept
{
    if (!std::isfinite(value)) return fallback;
    if (value <= static_cast<float>(minimum)) return minimum;
    if (value >= static_cast<float>(maximum)) return maximum;
    // Safe because the range is small and value is finite.
    const int integer = static_cast<int>(std::lround(value));
    return integer < minimum ? minimum : (integer > maximum ? maximum : integer);
}
}
