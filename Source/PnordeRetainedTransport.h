// ============================================================================
// PnordeRetainedTransport.h — Transport timing helpers
// ============================================================================
// Converts host tempo and transport positions into safe scheduling boundaries.
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
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

// Pure realtime helpers for the independently advancing PNORDE beat clock.
// Audio thread owns beat/origin values; no allocation, locks, or host interactions.
namespace PnordeRetainedTransport
{
// Invalid transport inputs should NEVER lead to division by zero or an
// infinity that later becomes a MIDI step index. A zero-length block is a
// neutral/safe result; normal valid inputs use the ORIGINAL formula.
inline double endOfBlock(double start, double bpm, int samples, double sampleRate) noexcept
{
    if (!std::isfinite(start)) return 0.0;
    if (!std::isfinite(bpm) || bpm <= 0.0 ||
        !std::isfinite(sampleRate) || sampleRate <= 0.0 || samples <= 0)
        return start;
    const double result = start + static_cast<double>(samples) * bpm / (60.0 * sampleRate);
    return std::isfinite(result) ? result : start;
}

// Converting a NaN/infinity or an out-of-range double directly to int64_t
// is undefined or implementation-dependent. Keep a generous safety margin
// around the limits, because callers also add/subtract ticks while searching
// boundaries. Music at ordinary/long-session PPQ positions is unchanged.
inline std::int64_t safeTick(double position, bool ceiling) noexcept
{
    constexpr double safeMaximum = static_cast<double>(
        std::numeric_limits<std::int64_t>::max() / 4);
    if (!std::isfinite(position) || position < -safeMaximum || position > safeMaximum)
        return 0; // neutral tick for malformed clock information
    const double rounded = ceiling ? std::ceil(position) : std::floor(position);
    return static_cast<std::int64_t>(rounded);
}

inline std::int64_t tickAt(double at, double origin, double stepDuration) noexcept
{
    if (!std::isfinite(at) || !std::isfinite(origin) ||
        !std::isfinite(stepDuration) || stepDuration <= 0.0) return 0;
    return safeTick((at - origin) / stepDuration + 1.0e-9, false);
}
inline std::int64_t nextBoundaryTick(double start, double origin,
                                     double stepDuration) noexcept
{
    if (!std::isfinite(start) || !std::isfinite(origin) ||
        !std::isfinite(stepDuration) || stepDuration <= 0.0) return 0;
    return safeTick((start - origin) / stepDuration - 1.0e-9, true);
}
inline double boundaryAt(std::int64_t tick, double origin, double stepDuration) noexcept
{
    if (!std::isfinite(origin) || !std::isfinite(stepDuration) || stepDuration <= 0.0)
        return 0.0;
    const double result = origin + static_cast<double>(tick) * stepDuration;
    return std::isfinite(result) ? result : 0.0;
}
}
