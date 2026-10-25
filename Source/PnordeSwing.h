// ============================================================================
// PnordeSwing.h — Swing timing math
// ============================================================================
// Computes step boundaries and groove timing. Floating point guards protect against malformed host values.
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
#include "PnordeRetainedTransport.h"

// Pure, allocation-free per-lane swing timing for PNORDE's retained beat clock.
// 50% = straight; 66.666...% = the classic 2:1 eighth-note swing ratio;
// 75% = a heavily swung 3:1 ratio. Two-step pairs last exactly 2 * duration.
// ONLY odd-position (weak) ticks move. Even-position (strong) ticks are
// unchanged, even with reverse pattern direction and nonzero start markers.
namespace PnordeSwing
{
constexpr int straight = 50;
constexpr int maximum = 75;

inline double weakDelay(int percent, double duration) noexcept
{
    percent = std::max(straight, std::min(maximum, percent));
    return duration * static_cast<double>(percent - straight) / 50.0;
}

inline bool isWeak(std::int64_t tick) noexcept
{
    return (tick % 2) != 0; // also works for negative tick numbers
}

inline double boundaryAt(std::int64_t tick, double origin,
                         double duration, int percent) noexcept
{
    const double base = PnordeRetainedTransport::boundaryAt(tick, origin, duration);
    return base + (isWeak(tick) ? weakDelay(percent, duration) : 0.0);
}

inline std::int64_t tickAt(double at, double origin,
                           double duration, int percent) noexcept
{
    auto tick = PnordeRetainedTransport::tickAt(at, origin, duration);
    // Within the delayed odd tick's slot, the previous (strong) cell still
    // owns the time. A nominal even tick is never delayed.
    if (isWeak(tick) && at + 1.0e-9 < boundaryAt(tick, origin, duration, percent))
        --tick;
    return tick;
}

inline std::int64_t nextBoundaryTick(double at, double origin,
                                     double duration, int percent) noexcept
{
    auto tick = PnordeRetainedTransport::nextBoundaryTick(at, origin, duration);
    // The nominal odd boundary may be BEFORE this audio block, even though
    // its delayed MIDI trigger is still inside it. Never skip that event.
    if (isWeak(tick - 1) &&
        boundaryAt(tick - 1, origin, duration, percent) >= at - 1.0e-9)
        --tick;
    return tick;
}

inline double stepSpan(std::int64_t tick, double origin,
                       double duration, int percent) noexcept
{
    return boundaryAt(tick + 1, origin, duration, percent) -
           boundaryAt(tick, origin, duration, percent);
}
}
