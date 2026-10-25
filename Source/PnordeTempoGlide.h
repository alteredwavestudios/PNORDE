// ============================================================================
// PnordeTempoGlide.h — PNORDE TempoGlide
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
