#pragma once
#include <algorithm>
#include <cstdint>

// Pure indexing for forward, backward, ping-pong play. Stable lane IDs: only
// the sampled step is remapped; sequencing state and automation are unchanged.
namespace PnordePlayback
{
enum Direction : int { forward = 0, backward = 1, pingPong = 2 };
inline int index(std::int64_t tick, int length, int direction) noexcept
{
    length = std::max(1, std::min(128, length));
    if (length == 1) return 0;
    const int fwd = static_cast<int>((tick % length + length) % length);
    if (direction == backward) return length - 1 - fwd;
    if (direction == pingPong)
    {
        const int period = 2 * (length - 1);
        const int phase = static_cast<int>((tick % period + period) % period);
        return phase < length ? phase : period - phase;
    }
    return fwd;
}
// Logical lane start is a stable step index, not an alteration of stored values.
// Reverse playback begins AT the marker, then travels backwards. Ping-pong
// uses a rotated triangle cycle. No audio-thread allocation or state writes.
inline int indexWithStart(std::int64_t tick, int length, int direction, int startStep) noexcept
{
    length = std::max(1, std::min(128, length));
    startStep = std::max(0, std::min(startStep, length - 1));
    if (length == 1) return 0;
    const int base = index(tick, length, direction);
    if (direction == backward)
        return (startStep - static_cast<int>((tick % length + length) % length) + length) % length;
    return (base + startStep) % length;
}
}
