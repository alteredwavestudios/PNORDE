# PNORDE — Dream Sequencer v0.5.9 (Windows v1.0 candidate)

A colourful polymetric/polyrhythmic JUCE C++ MIDI VST3 instrument by **Altered Wave Studios**, created by **LANTERNS (Nova O'Reilly)**.

This is a **source candidate, not a compiled or certified Windows 1.0 release**. Compile and test this candidate in Ableton before promoting it to 1.0.

## What's changed from v0.5.6.1

- **Tempo-relative glide (0–100% of the lane clock duration)** for Aftertouch, Mod Wheel (CC1), Expression (CC11), Vibrato (CC76 and CC77), and Pan (CC10). Sustain (CC64) remains instantaneous. CC Macro glide remains tempo-relative and restricted to safe CC20–31.
- Aftertouch values from v0.5.5–0.5.6.1 that were explicitly saved in **seconds** are migrated losslessly into a legacy internal range (101–201). Newly edited glide is tempo-relative. The original automatic pressure ramp (`-1`) is preserved.
- New **Global Octave** dial, **-2 to +2 octaves**, with final MIDI note clamping (0–127); the seven base lanes and existing Octave lane remain independent.
- **Key, Scale and Global Clock** use compact dial controls. Pitch Bend Range remains a dropdown. **Theme selection is in Settings only**.
- The **MIDI OUT** selector and refresh control are in the left footer, beside existing footer controls without overlap.
- Each lane has **separate compact circular dials**: division and S/T/D timing. Drag up/down or scroll each independently; double-click resets that dial.
- The **Delete X glows warning red on hover**, the sequence-length marker remains neon bright, active step numbers become brighter, and cell backgrounds subtly brighten over four columns then repeat.
- Preserves v0.5.6.1 experimental darkness, inline Macro labels, rail drag reordering and per-lane tooltips.

### Editing glide

- **Aftertouch, Mod Wheel, Expression, CC Macro:** vertical drag sets the parameter and horizontal drag sets glide percentage.
- **Vibrato:** normal horizontal drag sets Depth; **Shift+horizontal drag** sets glide for Rate and Depth. Vertical sets Rate.
- **Pan:** horizontal drag changes Pan (left/centre/right); vertical drag changes tempo-relative Glide. Shift offers fine adjustment.
- **Sustain and MIDI Channel Select:** no glide option.

Glide starts from the current interpolated value when the lane steps change. With a 1-beat lane division, 50% glide completes over half a beat. It follows tempo changes on subsequent step transitions. For the first occurrence on a route, the initial value is sent immediately rather than inventing a previous value.

## Clean Windows build

Back up your working v0.5.6.1 source, extract this archive to `H:\Music Library\PNORDE\dev\PnordeDreamSequencer`, then run in **ordinary PowerShell** (no script execution-policy changes required):

```powershell
$project = 'H:\Music Library\PNORDE\dev\PnordeDreamSequencer'
cmake -S "$project" -B "$project\build-clean-v059" -G 'Visual Studio 18 2026' -A x64
if ($LASTEXITCODE -ne 0) { throw 'Configuration failed; do not install.' }
cmake --build "$project\build-clean-v059" --config Release --target PnordeDreamSequencer_VST3 --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed; do not install.' }
Write-Host 'PNORDE v0.5.9 BUILD SUCCESSFUL' -ForegroundColor Green
```

After a successful build, close Ableton, open **PowerShell as Administrator** and run the included `INSTALL_PNORDE_ADMIN.ps1` (script policy might require a temporary process-scoped policy change). Installer checks build output, copies to a staged directory, SHA-256 verifies, backs up your old PNORDE VST3 and replaces only PNORDE. It does **not** alter `Polyrhythm MIDI.vst3`.

## Safety and backwards compatibility

- Plugin identity remains unchanged: `AlWS`, `PnDr`, `com.alteredwavestudios.pnorde`.
- Existing projects **without** new glides load those optional glides as 0 (instant, as before). Existing saved Aftertouch glide seconds are preserved. New global octave defaults to 0.
- The old CC20–31 restriction remains. Older macros using different CCs may be remapped to CC20 on loading, as in v0.5.6.1. Keep project backups.
- Standalone C++17 ASan and UBSan checks plus source regression checks are run here; full JUCE Windows compilation, actual Ableton interaction, and a full audio-thread/leak test are **not** run in this environment.
- Before a v1.0 tag, follow `TESTING_AND_COMPATIBILITY.md`, try multiple instances, save/reload presets, test all dials and automation, and compare previously made music in copies of Ableton sets.

## v0.5.8 stabilisation candidate (not Windows 1.0 yet)

- Removed the un-dimmed square around each lane dial. Independent **Division** and **Timing** circular dials (S/T/D) fit between the step grid and page controls. Rings remain bright over experimental darkness; circular dials have no rectangular fill.
- Pan gestures: **left/right = pan CC10, up/down = glide percentage**, Shift slows both. Values are clamped to CC10 0..127 and Glide 0..100.
- Page navigation has a steady, lane-coloured neon outline; Remove Page turns warning red on hover. Dark-theme tooltips are assigned fully opaque backgrounds and contrasting foreground text.
- Experimental animation is capped at **30 Hz** instead of 45 Hz to reduce editor repaint requests. Distant cells skip unnecessary exponential trail calculations; audio/MIDI scheduling is unchanged.
- There are **20 additional 12-TET/quarter-tone scale choices** plus **13 curated EDO modes** (47 choices total). The first 14 retain their old indices and original MIDI automation range. New scales are in a separately appended parameter to avoid old normalised-scale automation changing value. Click the Scale dial to choose from the list, or drag/scroll.
- The exact semitone pattern **0, 1, 4, 5, 7, 8, 10** is labelled `Phrygian Dominant (Hijaz 12-TET)`. Arabic Rast and Bayati are explicitly labelled *quarter-tone approximations*, not full culturally/contextually accurate maqamat.
- Optional **MANUAL BPM** (20.0–300.0) controls the plugin's musical beat progression while using the Ableton transport for play/stop and seeking. Disable it for normal Ableton-host tempo. This does not change Ableton's own tempo or make PNORDE play while transport is stopped.
- State v12 saves the new controls and the EDO choice. Legacy v10 and older projects default to HOST tempo. Existing preset CC limitations still apply; always test copies of Ableton projects first.

## Experimental EDO melodies and generators

- Click **Scale** to open the list: the **13 new EDO modes** are at the bottom. They cover 17, 19, 22, 24 and 31 equally divided octaves. The first 34 choices and their automation ranges stay intact; EDO modes use a new appended `pn_scale_edo` automation parameter.
- In **RANDOMISE → Generate microtonal melody (PITCH)**, choose **Longing / unresolved**, **Bittersweet arpeggio**, **Gentle dissonance** or **Homeward resolution**. The submenu is enabled only for an EDO mode. It changes **PITCH only**, leaving gates, velocity, lengths and other lanes intact.
- Pattern generation uses the selected EDO's approximate 6/5 (minor third), 5/4 (major third), 3/2 (fifth), 7/4 (harmonic seventh), and 11/9-like neutral third as melodic anchors, with gentle deterministic variations. These are musical *starting points*, not a prediction of how all listeners will perceive sadness or pleasantness.
- The existing pitch lane stores 0.1-semitone increments, but **actual MIDI playback snaps to exact EDO frequencies** via fractional-semitone channel pitch bend. Microtonal scales force an effective +/-2-semitone bend range; set the receiving synth to +/-2 too. The original bend preference returns for conventional scales. The SLIDE lane stays active, with 0–100% controlling tempo-relative approach to each microtonal pitch. Output is monophonic; experimental lower-zone MPE sends the single sequencer voice on member channel 2, while a separate Poly Aftertouch toggle is disabled by default. Neither option creates polyphonic voices. A non-pitch-bend-capable receiver will play normal 12-TET notes.
- Opening an old project should not change scale indices, but the extra EDO parameter and exact microtonal pitch delivery still need tested in Windows/Ableton before calling this release-ready. See `MICROTONAL_EDO_AND_GENERATORS.md` for sources, cents comparisons and examples.

### Known validation limits

Standalone Scale quantisation plus original helper tests can run with AddressSanitizer/UBSan. Full plugin Windows compilation, memory profiling and Ableton runtime / host automation testing **have not been performed here**. Do not release this as Windows 1.0 before those tests.

## v0.5.9 — Microtonal pitch approach and experimental output

- Selecting **17/19/22/24/31-EDO**, **Rast**, **Bayati**, or **Free (0.1)** now forces an **effective +/-2 semitone bend range** in PNORDE's MIDI math. The header and Settings show +/-2 and lock that setting while the microtonal scale is active. On returning to standard scales, the previous stored bend preference is restored without rewriting the project's `pn_bend` automation. **Set the receiving synth's pitch bend range to +/-2 too**; PNORDE cannot guarantee that the DAW forwards the bend or the synth honours an RPN.
- **SLIDE remains enabled with all original per-step data intact** in microtonal scales. The value now serves as *pitch approach time*: 0% = instant, 100% = arrives at the exact target when the current note's gate ends. The time is measured in beats, so it follows Ableton or manual BPM. Because a microtonal pitch wheel at +/-2 spans only +/-2 semitones around the active MIDI note, jumps larger than this can only approach from the nearest representable starting pitch.
- Settings has two output routes: **Standard Pitch Bend** (default) and **Experimental single-member MPE** (channel 2, sends an RPN request for +/-2 bend). **Experimental Poly Aftertouch** is a *separate* toggle, OFF by default. MPE channel pressure and MIDI polyphonic key pressure are different messages. No polyphonic note generation is claimed.
- For reliable tests, set PITCH values that produce clear microtonal intervals, select SLIDE **0%** at first, and use MIDI Monitor before the synth to verify host-delivered Pitch Bend messages. Standalone sanitizer tests don't verify VST3 MIDI message forwarding.
