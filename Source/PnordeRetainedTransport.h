#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

// Pure realtime helpers for the independently advancing PNORDE beat clock.
// Audio thread owns beat/origin values; no allocation, locks, or host interactions.
namespace PnordeRetainedTransport
{
inline double endOfBlock(double start, double bpm, int samples, double sampleRate) noexcept
{
    return start + static_cast<double>(samples) * bpm / (60.0 * sampleRate);
}
inline std::int64_t tickAt(double at, double origin, double stepDuration) noexcept
{
    return static_cast<std::int64_t>(std::floor((at - origin) / stepDuration + 1.0e-9));
}
inline std::int64_t nextBoundaryTick(double start, double origin,
                                     double stepDuration) noexcept
{
    return static_cast<std::int64_t>(std::ceil((start - origin) / stepDuration - 1.0e-9));
}
inline double boundaryAt(std::int64_t tick, double origin, double stepDuration) noexcept
{
    return origin + static_cast<double>(tick) * stepDuration;
}
}
