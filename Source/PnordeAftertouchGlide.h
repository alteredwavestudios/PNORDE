#pragma once
#include <algorithm>
#include <cmath>
#include "PnordeTempoGlide.h"
// -1 = old automatic one-step ramp; 0..100 = percent of lane duration.
// 101..201 are v0.5.5-6.1 values in tenths of seconds, kept for project recall.
struct PnordeAftertouchGlide final
{
    static constexpr int legacyAuto = -1;
    static int clampSetting(int v) noexcept { return std::max(-1, std::min(201, v)); }
    static double durationBeats(int setting, double bpm, double laneBeats) noexcept
    {
        if (!std::isfinite(bpm) || bpm <= 0.0 || !std::isfinite(laneBeats) || laneBeats <= 0.0) return 0.0;
        if (setting < 0) return std::min(10.0 * bpm / 60.0, laneBeats);
        if (setting > 100) return (setting - 101) * 0.1 * bpm / 60.0;
        return PnordeTempoGlide::durationBeats(setting, laneBeats);
    }
};
