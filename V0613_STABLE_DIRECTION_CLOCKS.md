# PNORDE v0.6.13 — Stable Direction + Clock Randomiser

## New behaviour
- Global reverse: header REV button; append-only, host-mappable `pn_global_reverse` 0–1.
- Per-lane direction popover: Ctrl/Cmd-click lane title to use a 3-way dial. Right-click legacy direction menus remain. Existing host parameters `pn_lane_direction_0..20` retained.
- Randomise button menu: clock divisions only (0..10), timing only (0..2), or both; visible active lanes only. No automatic/background randomisation.
- Global reverse preserves current step and reverses tick travel via bounded per-lane audio-thread phase offset. Cycling directions and lane starts retain their existing storage and parameter IDs.
- First-open defaults: darkness 0%, hypnosis 100%; old states preserve saved settings.
- No swing implementation in this release.

## Tests and risks
- Run `bash tests/RUN_STATIC_TESTS.sh`: pure-C++ ASan/UBSan checks, including exhaustive tick reflections.
- Complete Windows JUCE compilation, Ableton plugin rescan/automation/mapping, VST3 host resize and popup dismissal remain **unverified until tested on Windows**.
- Parameter changes are sampled once per processing block; hardware-MIDI remapping must be configured in Ableton's plugin parameter MIDI Map mode.
- When opening saved projects with v0.6.12 or earlier, missing global reverse defaults OFF. Older plugin versions may reject v0.6.13 project states. Test on a copy of important Ableton projects.

## Build
From a regular PowerShell, using the extracted folder as `$project`:

```powershell
$project = "H:\Music Library\PNORDE\dev\PnordeDreamSequencer\PNORDE_v0613_Stable_Direction_Clocks"
cmake -S "$project" -B "$project\build-clean-v0613" -G "Visual Studio 18 2026" -A x64
if ($LASTEXITCODE -ne 0) { throw 'Configure failed' }
cmake --build "$project\build-clean-v0613" --config Release --target PnordeDreamSequencer_VST3 --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Build failed - do not install' }
```

Run `INSTALL_PNORDE_ADMIN.ps1` in Administrator PowerShell only after a successful build and with Ableton closed. It stages/backs up/verifies VST3 SHA256.
