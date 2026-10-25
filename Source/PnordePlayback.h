// ============================================================================
// PnordePlayback.h — Which step plays next
// ============================================================================
// Pure calculations for sequence direction, length, random ordering and starting positions.
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
#include <cstdint>

// This file decides WHICH cell a lane reads on each of its clock ticks.
// It NEVER rearranges, edits, or saves the cell values themselves.
//
// The direction numbers 0, 1 and 2 are intentionally unchanged because old
// Ableton sets and older presets already use those numbers. RANDOM is number 3.
namespace PnordePlayback
{
enum Direction : int { forward = 0, backward = 1, pingPong = 2, randomStep = 3 };

// Convert a whole-number lane tick into a well-mixed, repeatable pseudo-random
// 64-bit number. "Pseudo-random" means it LOOKS random but the same input always
// gives the same output. That is useful for music: if Ableton asks us to render
// the same moment twice, we MUST pick the same cell both times.
//
// This uses a mathematical mixing function, NOT JUCE's shared Random object.
// No memory is allocated, no locks are taken, and the audio thread keeps no
// extra random state. unsigned arithmetic wraps around safely by C++ rules.
inline std::uint64_t mixedRandom(std::int64_t tick, int lane) noexcept
{
    std::uint64_t number = static_cast<std::uint64_t>(tick);
    // Give every physical lane its own distinct random pattern.
    number += 0x9e3779b97f4a7c15ULL
            * (static_cast<std::uint64_t>(lane < 0 ? 0 : lane) + 1ULL);
    number = (number ^ (number >> 30)) * 0xbf58476d1ce4e5b9ULL;
    number = (number ^ (number >> 27)) * 0x94d049bb133111ebULL;
    return number ^ (number >> 31);
}

// tick: which clock step the lane has reached (NOT an audio sample count).
// length: the number of active cells (between 1 and 128).
// direction: 0=forward, 1=backward, 2=ping-pong, 3=random.
// lane: each lane's permanent row number; prevents identical random streams.
inline int index(std::int64_t tick, int length, int direction, int lane = 0) noexcept
{
    length = std::max(1, std::min(128, length));
    if (length == 1) return 0;
    if (direction == randomStep)
        return static_cast<int>(mixedRandom(tick, lane)
                                % static_cast<std::uint64_t>(length));
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

// startStep shifts where the selected cell appears within the lane's pattern.
// For RANDOM this shifts the result but never changes the original cell data.
// Repeated calls with the same tick, lane and settings select the SAME cell;
// this matters when MIDI notes and 200-Hz modulation share the same lane.
inline int indexWithStart(std::int64_t tick, int length, int direction,
                          int startStep, int lane = 0) noexcept
{
    length = std::max(1, std::min(128, length));
    startStep = std::max(0, std::min(startStep, length - 1));
    if (length == 1) return 0;
    const int base = index(tick, length, direction, lane);
    if (direction == backward)
        return (startStep - static_cast<int>((tick % length + length) % length) + length) % length;
    return (base + startStep) % length;
}
}
