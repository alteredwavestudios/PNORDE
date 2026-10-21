#pragma once
// Saved in the host's plugin state (and exported .pnorde presets), not in
// per-machine mutable storage. This header has no JUCE or audio-thread dependencies.
namespace PnordeProjectVersion
{
constexpr int schema = 3;
constexpr int build = 621;  // v0.6.21
constexpr const char* release = "0.6.21";
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
