# PNORDE 0.6.5 — HPPD modular visuals (Windows build candidate)

HPPD = **Hallucinogenic Polyrhythm Pattern Discombobulator**. The plugin remains **PNORDE - Dream Sequencer** with exactly the same plugin identifiers and stable parameter IDs.

## What changed

- Four independent effects: **Neon Nodal**, **Phase Weave**, **Prism Spikes**, and new **Kumiko Checker**. Only one effect class is instantiated and one ARGB frame is retained at a time.
- **Prism Spikes now originates from each window edge and travels toward the centre**, rather than projecting shards from its centre. Its shards follow lane step, phase, division and value.
- **Soft path glow passes removed** from HPPD: one bright, narrow stroke per shape, with small crisp nodes. The **sequencer's regular LED lighting is preserved**. Ambient colour gradients remain intentionally subtle.
- Frames sample one rotating bank of up to 7 lanes, switching every **500 milliseconds**. All 21 lane indices can influence the visuals across a 1.5-second cycle; disabled optional lanes don't draw. This is visual sampling only and **does not affect MIDI processing**.
- HPPD retains **12 FPS maximum frame generation**, a single image capped at **768×512 ARGB** (1.5 MiB of raw pixels), and work on the UI thread rather than the audio callback. Whole-window compositing can still be costly.
- All modes use a shared saturated **theme-derived palette**, rather than cycling through unrelated fully primary RGB colours. The selected editor theme is respected.
- Click **Pnorde / Dream Sequencer** in the top-left header for an **About** callout, with version **0.6.5** and a clickable link to **https://github.com/alteredwavestudios/PNORDE** for updates.
- Click **LДNTERNS** in the bottom-right footer for a separate callout saying **Check out my bandcamp!!** with **l4nterns.bandcamp.com** in a **read-only, selectable and copyable TextEditor**, not a clickable URL. Both popups are JUCE CallOutBoxes which dismiss on click-away.
- Existing presets still load their visual mode 0, 1, 2 from previous versions. Mode 3 is the new fourth option. The previous `PnordeVibesMode` state key and `pn_vibes_bleed` automation ID are **preserved** for backwards compatibility.

## Build on your Windows machine

IMPORTANT: build with `-S` pointing to the **extracted `PNORDE_v065_HPPD` source folder**, not the main project's old source. This avoids repeating the previous wrong-version problem.

```powershell
$project = 'H:\Music Library\PNORDE\dev\PnordeDreamSequencer'
$source = Join-Path $project 'PNORDE_v065_HPPD'
$build = Join-Path $project 'build-clean-v065-hppd'
cmake -S $source -B $build -G 'Visual Studio 18 2026' -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }
cmake --build $build --config Release --target PnordeDreamSequencer_VST3 --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed. Do not install' }
```

The resulting bundle should be at `$build\PnordeDreamSequencer_artefacts\Release\VST3\PNORDE - Dream Sequencer.vst3`.

## Security and performance

No network calls were added to audio processing or the HPPD renderer. GitHub opens only through a deliberately user-clicked JUCE HyperlinkButton. Bandcamp is deliberately **not** a HyperlinkButton. The source suite includes regression tests and standalone sanitizer helper tests, but these do **not** replace a full Windows JUCE compilation, interactive testing in Ableton, or a Windows runtime memory/thread sanitizer run.

Measure Ableton's CPU / RAM with HPPD off vs. on, editor open vs. closed; also test resizing, stopping playback, mode switches, multiple instances, preset roundtrips and plugin closing. Keep a backup of your currently working v0.6.4 VST3.
