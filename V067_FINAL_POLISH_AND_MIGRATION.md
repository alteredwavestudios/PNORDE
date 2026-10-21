# PNORDE v0.6.7 — Final polish and state migration

- **New instances:** Major default (pn_scale=1).
- **Unversioned projects**, including v0.6.6 and older: all validated lane/pattern/MIDI data restored as before, but Scale is deliberately set to Free (0.1) = 13. This policy was requested for backward compatibility. Even previously saved Major projects *without* a project-version stamp will initially switch to Free; the user may select Major afterward.
- **Versioned projects:** exact saved `NovaScale` is restored. `getStateInformation` writes `PnordeStateSchema`, `PnordeSavedBuild` and `PnordeSavedPluginVersion` plus `PnordeHppdFps` so the next save remembers this migration.
- **Future schema:** unknown or partially stamped states are refused safely rather than silently applying an incompatible format; other existing preset bounds checks still run.
- **Scale dial:** Shift-click Scale or choose RESET TO MAJOR from the menu to restore Major; double-click Key resets to C. Free ignores Key *by design*; its tooltip now explains this. Stored pitch step values remain untouched.
- **Lane typography:** Uses user-installed Google Fonts Orbitron when present, otherwise Windows Bahnschrift SemiCondensed. Font files are NOT distributed in this package; optionally install it from https://fonts.google.com/specimen/Orbitron on Windows and restart Ableton.
- **Dark-mode active cells:** higher alpha and saturation without increasing HPPD geometry passes or adding blurred layers.
- **HPPD frame rate:** 1–60 FPS, default 12; host-automatable parameter `pn_hppd_fps` appended after earlier parameter IDs. Frame scheduling is message-thread-only and never affects MIDI timing. 60 FPS requires a powerful computer and can be slower.
- **Performance:** No new high-resolution images or rendering in `processBlock`; idle GUI timer stays at its previous baseline.

## Build (Visual Studio 2026)

From your main PNORDE project folder:

```powershell
$project = "H:\Music Library\PNORDE\dev\PnordeDreamSequencer"
cmake -S "$project\PNORDE_v067_Final_Polish" -B "$project\build-clean-v067" -G "Visual Studio 18 2026" -A x64
cmake --build "$project\build-clean-v067" --config Release --target PnordeDreamSequencer_VST3 --parallel 4
```

Close Ableton and back up your installed VST3 before replacing it. Check build and installed binary SHA-256 hashes to confirm the correct version.
