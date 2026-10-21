# PNORDE v0.6.1 — Vibes, retained transport, independent lane restart

This ZIP is a **source candidate**, based directly on the supplied `PNORDE_v060_Factory_Patterns_Transport_Directions_Source_Candidate(1).zip`.
Do not install until it builds successfully on Windows. There is no compiled `.vst3` in this ZIP.

## Changes

- Existing Play/Pause, Reset All, settings and macros preserved. **Play resumes every lane from its retained musical phase**. DAW Stop and PNORDE Pause silence notes but preserve phase; Restart All (the existing RESET button) restores all origins to step 1. Restart timing is audio-buffer-level, not sample-accurate automation.
- New **Restart Lane 1..21 Trigger** automatable parameters appended after every v0.6.0 parameter. In Ableton, change the trigger parameter's **integer value** (for example 0->1, 1->2, and so on) at the desired beat. Use *stepped*, not ramped, automation, because **each parameter value change triggers another restart**. Parameters are not momentary Boolean switches.
- From the lane's direction menu, choose **RESTART THIS LANE (automatable)** to restart just that lane. Previously, clicking the row label opens the direction menu; the additional item is added there. Resetting SEQ also ends its held note.
- **Vibes** (Settings) replaces the eight faint wash gradients with a cached static mandala and 6-fold, phase-derived colourful positions. Vibes also has a separate **Vibes Main-Area Bleed** parameter/slider (0 to 50%). It intentionally keeps overlay alpha under 10% and is disabled by default due to flashing-light sensitivity considerations. The previous Intensity control remains 0-40.
- No cache allocations or drawing occur inside `processBlock`. The Vibes cache is at most 672x432 ARGB (about 1.1 MiB) and rebuilds only when the editor dimensions change. The existing 24/30 Hz editor timer drives effects; no second animation timer.
- Fixed a pre-existing editor source typo: a `std::array` declared to hold five buttons was initialised with seven.
- Kept preset decoding limits, fixed-capacity MIDI queue, VST3 identity, and **append-only** host automation ID ordering.

## Build (Windows PowerShell, from the extracted v0.6.1 folder)

Open PowerShell in this folder. Execute the project-provided build script (after inspecting it):

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\BUILD_PNORDE.ps1"
```

If you prefer using CMake directly:

```powershell
cmake -S . -B build-clean-v061 -G "Visual Studio 18 2026" -A x64
cmake --build build-clean-v061 --config Release --target PnordeDreamSequencer_VST3 --parallel 4
```

The generator is the Visual Studio **2026** generator used in the supplied v0.6.0 source. If your system uses Visual Studio 2022 instead, change `Visual Studio 18 2026` to `Visual Studio 17 2022` in the script or CMake command. Set `-DJUCE_DIR=...` if your JUCE isn't in one of the existing auto-detection paths.

## Install only after successful compile

1. **Close Ableton Live completely.**
2. Inspect `INSTALL_PNORDE_ADMIN.ps1`. It stages, SHA256-checks and backs up the installed PNORDE plug-in before installing; it runs only as administrator.
3. From an **administrator** PowerShell in this directory, run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\INSTALL_PNORDE_ADMIN.ps1"
```

4. Open Ableton. VST3 is under `C:\Program Files\Common Files\VST3` and keeps the same PNORDE plugin identity.
5. Create a **new test set** first and compare 30-second playback with Vibes OFF/ON, editor closed/open. Only then test previous Live sets/presets.

**Important:** A temporary `-ExecutionPolicy Bypass` does not permanently alter machine policy. Use it only for scripts you have inspected and trust. Do not run scripts from a ZIP as administrator without checking them.

## Acceptance checklist

- [ ] VST3 builds, appears in Live, produces MIDI.
- [ ] Sequence of different lane lengths (for example 5,7,9) pauses via PNORDE PAUSE and resumes on the same phase; note-offs at pause do not stick.
- [ ] Ableton STOP/PLAY also preserves phase (the first new note can occur on the next step boundary, rather than retriggering mid-step).
- [ ] RESET ALL sets all lanes to first step; individual lane resets leave other lanes unchanged.
- [ ] In Live, both global and individual Restart Trigger automation function using stepped integer values.
- [ ] Vibes off/on, 0%/50% bleed, resize, settings popup visible; no severe CPU increase or flickering.
- [ ] Test preset loads and save/reload. Test 17/19/22/31 EDO, MPE mode, external MIDI if used.
- [ ] Compare Task Manager CPU and memory over several minutes. Opening/closing plugin should release the cached image, and memory should not grow steadily.

## Verification performed in this environment

`bash tests/RUN_STATIC_TESTS.sh` passed, including existing C++ sanitizer tests and new retained-clock tests. However, **JUCE and the Windows VST3 toolchain are not installed in this environment**; the final Windows plugin compile and Ableton validation could not be performed here. Report any Visual Studio compiler error with its file and line for the next correction build.

## Security notes

- Vibes is deterministic GUI drawing, with no external network, file access or shader downloads.
- Cache sizes and row counts are bounded; no dynamic memory allocation added to the real-time MIDI processor.
- ASLR means JUCE components don't need or retain specific absolute memory addresses.
- Existing bounded parser and fixed MIDI queue remain intact; this does not constitute a security audit of every code path.
- If debugging memory safety on Windows, use `BUILD_ASAN_STANDALONE_WINDOWS.ps1` to compile the standalone **test** build; never install ASan as a production VST3.
