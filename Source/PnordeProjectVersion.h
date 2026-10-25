// ============================================================================
// PnordeProjectVersion.h — Plugin and saved-project versions
// ============================================================================
// The displayed application/build version is separate from the saved-state compatibility marker. Do not bump the latter for comment-only changes.
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
// Saved in the host's plugin state (and exported .pnorde presets), not in
// per-machine mutable storage. This header has no JUCE or audio-thread dependencies.
namespace PnordeProjectVersion
{
constexpr int schema = 3;
constexpr int appBuild = 629; // Human-facing build. Increment each release.
constexpr int build = 625;  // Keep saved-state compatibility marker: NO schema change in v0.6.26
// Human-visible version. The About window and HPPD settings footer share
// this exact value so they cannot accidentally claim different releases.
// Do not confuse it with `build` (saved-project compatibility metadata).
// 0.6.29 fixes global transport state, avoids an internal XML round trip,
// and adds an opt-in MIDI flight recorder; saved schema is unchanged.
constexpr const char* displayVersion = "0.6.29";
constexpr const char* release = "0.6.29-test";
constexpr int legacyFreeScale = 13;
constexpr int newInstanceMajorScale = 1;
// Previously saved states have no PnordeStateSchema/PnordeSavedBuild pair.
// Their scale is intentionally mapped to Free (0.1), leaving their sequence
// and all other validated parameters unchanged.
constexpr int restoredScale(bool hasVersion, int savedScale) noexcept
{
    return hasVersion ? savedScale : legacyFreeScale;
}
}
