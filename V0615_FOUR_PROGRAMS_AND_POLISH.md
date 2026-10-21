# PNORDE v0.6.15 — Four Programs & Final Polish (Windows Build Candidate)

**This is a source candidate, not a verified Ableton release.** The C++ standalone tests and source-contract checks pass in the development environment. The complete JUCE VST3 must still be compiled and tested in Visual Studio 18 2026 / Ableton Live.

## Four quick-load programs

Four numbered header buttons select four independent program snapshots. The Settings panel also exposes a discrete Program 1–4 slider. The appended JUCE/host parameter ID is `pn_program_slot` (`PNORDE Quick Program`, integer 0–3 internally), automatable and MIDI-mappable through Ableton Live's Configure / MIDI Map features. This is a plugin automation parameter, **not** a MIDI Program Change implementation.

A scene stores the full existing plugin state: sequence/pitch/velocity/gate values; step patterns; per-lane lengths, clock divisions, timing, direction and start marker; scales/keys; CC assignments; visual configuration and other current settings. All four snapshots are embedded inside normal host state and PNORDE `.pnorde` presets. Selecting a *different* program releases active MIDI notes and invokes the lane-phase reset so all lanes start from their saved `start here` markers, without changing Ableton's transport position. Selecting the *same* program does not restart playback.

Older v0.6.14-or-earlier and unversioned files without the new four-slot metadata import into Program 1 (the other slots are initially copied from it). No existing parameter IDs were changed. Preset and state envelope limits were extended to allow complete four-slot saves, with per-slot bounds and structural checks. **Keep copies of old songs/presets; old PNORDE versions do not necessarily understand files resaved by v0.6.15.**

## Other changes

- Native JUCE CallOutBox popups have been replaced with editor-owned overlays that stay inside PNORDE's plugin rectangle; clicking outside dismisses the popup. Settings scrolls inside the panel on shorter editor windows.
- PNORDE and LДNTERNS names get animated, coloured luminous hover feedback, and the credit text is positioned next to the signature.
- Global dial labels use consistent positioning, and the header becomes two rows at narrower widths; 8 grid columns remain visible down to 800px.
- Right-click a specific lane's clock dial for one-lane division, timing (Straight/Triplet/Dotted), or combined randomisation.
- The existing aftertouch randomisation menu now offers eight pattern recipes, safely clamped to the existing MIDI expression range.
- **No swing engine in this release.**

## Build on Windows 11

Extract the entire ZIP so `CMakeLists.txt` is within `PNORDE_v0615_Four_Programs_Final_Polish`. From a normal PowerShell window:

```powershell
$project = 'H:\Music Library\PNORDE\dev\PnordeDreamSequencer\PNORDE_v0615_Four_Programs_Final_Polish'
& "$project\BUILD_PNORDE.ps1"
```

Or use manual commands from your known-good Visual Studio environment:

```powershell
cmake -S "$project" -B "$project\build-clean-v0615" -G 'Visual Studio 18 2026' -A x64
if ($LASTEXITCODE -ne 0) { throw 'Configuration failed!' }
cmake --build "$project\build-clean-v0615" --config Release --target PnordeDreamSequencer_VST3 --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Build failed!' }
```

Only **after a successful build**, close Ableton; then run `INSTALL_PNORDE_ADMIN.ps1` from Administrator PowerShell. It verifies the new VST3, stages it, creates a backed-up copy of the previous installation, verifies SHA256, and attempts restoration on failure.

## DAW acceptance checks — REQUIRED before calling stable

1. Make backup copies of pre-v0.6.15 Ableton sessions and PNORDE presets. Open one copy in v0.6.15; verify original pitch, gate, velocity, CC, scale and all lane start markers survive in Program 1.
2. Fill Programs 1–4 with intentionally different pitch/velocity patterns, scale/key, clock division and per-lane start positions. Return to each program and verify values are independently restored.
3. Play continuous MIDI notes; switch programs with the four buttons and while Live transport is running. Ensure no stuck notes, correct per-lane restart, and no host playhead restart. Re-select the same slot and check there is no gratuitous restart.
4. Use Ableton Configure + automation and an external MIDI controller to change `PNORDE Quick Program`. Make sure the currently selected button, Settings slider and playback agree.
5. Save a `.pnorde` file and an Ableton project with four distinct programs. Quit and reopen Live. Verify all four scenes are unchanged; repeat using program 2 or 4 active at save time.
6. At 800, 1000, 1220 and 1500px editor widths check 8/16 grid step visibility, no clipped globals/quick buttons/footer, and that Settings/direction/About panels remain entirely within the plugin edge.
7. Try one-lane clock randomisation and aftertouch recipes on short and long lanes. Verify other lane patterns and CC assignments do not change.
8. Check that project reopen restores saved state; **never overwrite the only copy of an older project while testing.**
