# PNORDE v0.6.2 — Stability and low-level audit candidate

**Source-only build candidate** based on the working v0.6.1 Vibes Retained Transport release candidate. The UI, Vibes appearance, 21 lanes, MIDI routing, current presets, existing host parameter order and plugin identity are preserved. No new visuals have been added.

## Source review: observed issues and remediation

| Finding in v0.6.1 source | Change | Expected benefit / caveat |
|---|---|---|
| GUI called full-window repaint when paused, or when animations were disabled, even for small fades; when playing, repainted every visible row on most timer ticks. | Track exactly which visible rows changed; full repaint only for layout/theme/popup/transport transitions. | Reduces unnecessary redraws. With Experimental Lights enabled, every visible row still changes during playback and is therefore repainted. |
| Vibes marked repaint dirty at every 24/30 Hz tick even when it was the only animation. | Limit dedicated Vibes-driven redraw requests to ~12 FPS without adding a timer. | Cuts animation invalidations; moving visuals are a little less fluid. Normal step updates are still handled by lane redraws. |
| Experimental lights called `exp` on each step for fadeoff, many times per visible row. | Compute phase exponential once per row, use precalculated coefficients for step-distance exponential. | Same effect to float precision, fewer transcendental operations. |
| `processBlock` called `std::atomic_load` on a shared pointer to external MIDI port. | Audio thread checks a plain atomic `outputReady` flag. Port's `shared_ptr` remains on UI and worker threads. | Avoids potential shared_ptr atomic internal locks/refcount on audio thread. |
| MIDI producer called worker `notify()` for each event; idle worker polled every 2 ms. | Notify only on empty-to-nonempty queue transition; sleep up to 50 ms when empty and rely on notify for prompt wakeup. | Reduced message/thread wakeup overhead. Due-time polling still at <=5 ms when queued. |
| MIDI scheduler used a 200,000-iteration emergency guard. | Bounded, sample-count-dependent iteration budget; on exhaustion sends a note-off and resets modulation target, avoiding a trapped real-time callback. | Prevents extreme iteration runaway; unusual huge blocks can be truncated safely rather than blocking the host. |
| Profiler saved both runs to the same CSV, and scanned all generated CMake files recursively. | Added unique, labelled output files and source-only scan. | Proper OPEN/CLOSED and Vibes ON/OFF comparisons; avoids recursing build trees. |

## Not yet proved / residual risks

- Passing helper AddressSanitizer and UBSan tests **does not mean the full JUCE plugin is sanitizer-validated**. The VST3 must be built and tested on Windows.
- True CPU/GPU bottlenecks can only be confirmed from live metrics, e.g. Ableton CPU and render-thread profiling. The supplied PowerShell profiler measures the **host process**, not per-plugin CPU.
- Plugin performance can still be limited by native child controls, JUCE font drawing, transparency blending and experimental glow. The main renderer has not been replaced wholesale because that is a high-risk rewrite.
- External-MIDI queue overruns are counted via `getExternalMidiDroppedEvents()` but MIDI hardware can still get a dropped event if its worker cannot keep up. Test USB/virtual MIDI under dense CC changes. External MIDI device drivers may block shutdown; the worker's existing JUCE `stopThread(3000)` lifecycle deserves further on-machine stress tests.
- Do not hardcode component memory addresses; ASLR relocates plugin code and data. Memory ownership uses RAII and bounded arrays instead.
- User-controlled preset state continues through the existing bounded XML validation; this work is not a comprehensive formal security review.

## Build on Windows 11

Extract to a fresh source directory containing `CMakeLists.txt`. In PowerShell in that directory:

```powershell
cmake -S . -B build-clean-v062 -G "Visual Studio 18 2026" -A x64
cmake --build build-clean-v062 --config Release --target PnordeDreamSequencer_VST3 --parallel 4
```

If you use Visual Studio 2022 instead of 2026, substitute `Visual Studio 17 2022` in the configure command. JUCE path auto-detection is unchanged from v0.6.1. **Do not install if compilation fails.**

To install after a successful build: close Ableton, open *Administrator* PowerShell in the source directory, inspect `INSTALL_PNORDE_ADMIN.ps1`, and run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\INSTALL_PNORDE_ADMIN.ps1"
```

The install script stages and verifies the built VST3, backs up the current plugin under `Backups/PNORDE_Pre062_*`, and replaces only the matching PNORDE bundle. No other VST3s are changed.

## Measure v0.6.1 versus v0.6.2

In a **normal, non-administrator** PowerShell in the extracted source `tools` folder, use the same Live project, audio settings, loop and plugin instance count for every run. First do this with the old v0.6.1 plugin, then repeat after installing v0.6.2:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\PNORDE-Profile.ps1" -SourceDir "H:\Music Library\PNORDE\dev\PnordeDreamSequencer" -Seconds 60 -Label old-open
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\PNORDE-Profile.ps1" -SourceDir "H:\Music Library\PNORDE\dev\PnordeDreamSequencer" -Seconds 60 -Label old-closed
```

Then run the same test with `-Label new-open` and `-Label new-closed` after the update. To isolate Vibes, run tests with the UI open and Vibes off/on. Files go into `tools/PNORDE-Diagnostics` with **unique timestamps**, so results are not overwritten. Memory increases over 60 seconds do not by themselves prove a leak; use 3–5 minute runs and observe whether private memory keeps growing or reaches a plateau.

## Acceptance checklist

- [ ] VST3 compiles on Windows and loads in Ableton without crashes.
- [ ] Long loop (>5 minutes) with 5/7/9 polymetric lanes: pause/resume retains positions, global and individual restart automation still work.
- [ ] Editor open/closed and Vibes off/on comparison on the same Ableton project shows acceptable CPU use.
- [ ] Scroll and resize at native Windows display scaling and 125%/150% scaling without missing redraws.
- [ ] CC glides, microtonal/MPE, MIDI channel changes, and external MIDI routing behave as before.
- [ ] Closing/reopening the plugin does not produce sustained private-memory growth or held notes.
- [ ] Dense automation does not freeze the host. If output is unexpectedly quiet, revert to the backed-up v0.6.1 VST3 and report logs.

## Verification here

`bash tests/RUN_STATIC_TESTS.sh` completed successfully, including targeted source assertions and C++ ASan/UBSan helper tests. No Windows JUCE compiler, Windows audio driver or Ableton instance is available here, so this is a **source candidate**, not a tested Windows binary.
