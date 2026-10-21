# PNORDE v0.6.4 — Experimental Neon & Prism Spikes

- Three separate selectable effects: Neon Nodal, Phase Weave and Prism Spikes.
- One active effect instance and one downscaled ARGB cache. Inactive renderers are destroyed.
- Heavy geometry updates capped at **12 Hz**, using the existing editor timer; no audio-thread work.
- Changed the Vibes overlay opacity limit to 82% at maximum bleed/intensity. Neon sits above the darkness pass, but can obscure controls at extreme settings: reduce Bleed to regain legibility.
- Prism Spikes generates triangular shards from all enabled lane snapshots.
- Default Vibes options and existing MIDI/preset state unchanged; third mode is appended.

## Build

Run from the project directory containing CMakeLists.txt:

```powershell
cmake -S . -B build-clean-v064 -G "Visual Studio 18 2026" -A x64
cmake --build build-clean-v064 --config Release --target PnordeDreamSequencer_VST3 --parallel 4
```

The JUCE VST3 must be compiled and performance-tested in Windows/Ableton. This candidate is not guaranteed faster than v0.6.3 at 3 Hz. Change `84.0` back to `334.0` in PluginEditor.cpp if needed. Preserve a backup of the working VST3.

Photosensitivity caution: avoid sustained viewing at high brightness, particularly if sensitive to flashing or high-contrast patterns.
