// ============================================================================
// PnordeProgramLane.h — PNORDE ProgramLane
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
#include <cstdint>

// JUCE-independent helpers. A PROGRAM lane cell is OFF (0) or scene 1..4.
// Global lane phase continues when the MUSICAL lanes restart on a bank change.
namespace PnordeProgramLane
{
inline int boundedLength(int n) noexcept { return n < 1 ? 1 : n > 128 ? 128 : n; }
inline int boundedSlot(int n) noexcept { return n < 0 ? 0 : n > 4 ? 4 : n; }
inline int activeIndex(std::int64_t tick, int length, int start) noexcept
{
    const int size = boundedLength(length);
    const int offset = start < 0 ? 0 : start >= size ? size - 1 : start;
    const auto position = (tick % size + size) % size;
    return static_cast<int>((position + offset) % size);
}
inline int completedIndex(std::int64_t boundaryTick, int length, int start) noexcept
{
    return activeIndex(boundaryTick - 1, length, start);
}
}
