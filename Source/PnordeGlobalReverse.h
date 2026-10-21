#pragma once
#include <array>
#include <cstdint>
#include <limits>

// Audio-thread-owned reverser: each toggle reflects local lane tick time
// about the CURRENT cell. Independent forward/backward/ping-pong settings
// remain unchanged (and host-automatable). All offsets reset on fresh start.
namespace PnordeGlobalReverse
{
class Phase
{
public:
    static constexpr int maxLanes = 21;
    void reset(bool reversed) noexcept
    {
        reverse = reversed;
        offsets.fill(0);
    }
    void update(bool next, const std::array<std::int64_t, maxLanes>& ticks) noexcept
    {
        if (reverse == next) return;
        // Next tick must equal previous tick at this instant:
        // oldSign*t + offset == newSign*t + newOffset.
        const int delta = reverse ? -2 : 2;
        for (int row = 0; row < maxLanes; ++row)
        {
            // Bound malformed/huge tick input to keep integer arithmetic safe.
            const auto tick = ticks[static_cast<std::size_t>(row)];
            if (tick > (std::numeric_limits<std::int64_t>::max() / 8) ||
                tick < (std::numeric_limits<std::int64_t>::min() / 8))
            { offsets[static_cast<std::size_t>(row)] = 0; continue; }
            offsets[static_cast<std::size_t>(row)] += delta * tick;
        }
        reverse = next;
    }
    std::int64_t position(int row, std::int64_t tick) const noexcept
    {
        if (row < 0 || row >= maxLanes) return tick;
        return (reverse ? -tick : tick) + offsets[static_cast<std::size_t>(row)];
    }
    void resetLane(int row) noexcept
    {
        if (row >= 0 && row < maxLanes) offsets[static_cast<std::size_t>(row)] = 0;
    }
private:
    bool reverse = false;
    std::array<std::int64_t, maxLanes> offsets {};
};
}
