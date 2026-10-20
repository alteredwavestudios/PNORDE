#pragma once
#include <algorithm>
#include <cmath>
// 0..100 percent of an individual lane's musical step duration. No allocations.
struct PnordeTempoGlide final
{
    static int clamp(int amount) noexcept { return std::max(0, std::min(100, amount)); }
    static double durationBeats(int amount, double laneBeats) noexcept
    {
        if (!std::isfinite(laneBeats) || laneBeats <= 0.0) return 0.0;
        return std::min(128.0, laneBeats * clamp(amount) / 100.0);
    }
};
