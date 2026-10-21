# PNORDE v0.6.3 — Vibes visual architecture / audit

## Strict ownership

`PluginEditor` owns one `PnordeVibes::Renderer`; the Renderer owns **either** a `NeonNodal` **or** a `PhaseWeave`, not both, and owns one `juce::Image` cache. Switching mode calls `effect.reset()` and clears the cache. All work is on JUCE's message thread, never the audio thread. No message-thread render touches MIDI scheduling state except atomic read-only snapshots through processor getters.

## Performance budget

- Heavy drawing: at most one frame every **334 ms** per editor (<=2.995 frames/sec).
- Cache: 768 x 512 x 4 bytes maximum = **1,572,864 bytes** (1.50 MiB), excluding JUCE image object bookkeeping.
- Per-update music snapshot: `std::array<LaneSample,21>` on stack. Optional lanes that aren't enabled are skipped; no vectors/heap allocation in snapshot generation.
- Neon glow is three bounded strokes per path, NOT a convolution bloom, shadow filter, or GPU FBO.
- Nodal shape draw: 8 rings + up to 21 lanes x 6 spokes with bounded iterations.
- Phase Weave draw: up to 21 paths of 63 points, each 3 glow strokes.
- Both effects reuse the same `glow`, `glowingNode`, and translucent `atmosphere` functions.
- Effects render to a cached 32-bit ARGB image; ordinary UI repaints only composite the finished image, never regenerate geometry.
- While paused, the current frame freezes after any required resize/mode update; clicking a new mode makes it visible on the next 3 Hz tick.

## Security/safety

No network requests, filesystem access, external shaders, untrusted asset loading or executable effect scripting. Input mode is clamped to known effect IDs, lane indices remain bounded, and MIDI generation was left untouched from v0.6.2. User can disable Vibes completely. Cap overlaid image alpha at 25% even with Intensity and Bleed at maximum; limit sudden convergence flashes.

## Limitations

This is a full JUCE source candidate, not a Windows-tested binary. It cannot prove render performance or GPU compatibility before you build it in Visual Studio and profile Ableton. Native editor controls can repaint faster than 3 Hz, but use only cached Vibes imagery. Visually busy designs may have readability tradeoffs on particular themes or scaled monitors; test dark/light themes.
