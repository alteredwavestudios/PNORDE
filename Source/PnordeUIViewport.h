// ============================================================================
// PnordeUIViewport.h — PNORDE UIViewport
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
// Pure (JUCE-free) view-position rules for restoring an editor after Undo.
// A "page" is the group of step cells currently visible in a lane.
// Musical note values are never modified by these helpers.
#include <cmath>

namespace PnordeUIViewport
{
constexpr int clampedPage(int requestedPage, int length, int columns) noexcept
{
    if (length < 1 || columns < 1) return 0; // Defensive against bad inputs.
    const int lastPage = (length - 1) / columns;
    return requestedPage < 0 ? 0 : (requestedPage > lastPage ? lastPage : requestedPage);
}

inline float clampedScroll(float requestedPixels, float maxPixels) noexcept
{
    // Avoid sending NaN/Infinity into JUCE's integer pixel positioning.
    if (!std::isfinite(requestedPixels) || !std::isfinite(maxPixels) || maxPixels <= 0.0f)
        return 0.0f;
    return requestedPixels < 0.0f ? 0.0f
        : (requestedPixels > maxPixels ? maxPixels : requestedPixels);
}
}
