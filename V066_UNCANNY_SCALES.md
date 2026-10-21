# PNORDE v0.6.6 — Uncanny HPPD and scale-switch fix

- On selecting Scale, no longer rewrites all 128 pitch-lane values; MIDI pitch is quantised non-destructively when played. This makes changing scale/key order reversible. Note currently sounding may not change until retriggered.
- `pn_scale` starts at 1 (Major) in **new instances**. Existing serialized states retain `NovaScale`, including Free 0.1 mode. When loading exceptionally old states with no `NovaScale` property, Major is the fallback.
- Explicitly clears stale base scale when selecting an extended scale.
- Fourth renderer becomes **UNCANNY CHESS**, a slowly distorted, perspective-skewed board with occasional diagonal cracks. The other three existing modes retain crisp theme-saturated shapes.
- No new UI animation timer or audio-thread graphics work; 12Hz renderer cap remains.

**Compatibility:** This changes newly created instances and the pitch behaviour when switching scales; verify existing projects and the interaction with EDO/microtonal modes. Full JUCE Windows compilation and DAW testing are still required.

Windows PowerShell from the main project folder:

```powershell
$project = "H:\Music Library\PNORDE\dev\PnordeDreamSequencer"
cmake -S "$project\PNORDE_v066_HPPD_Uncanny_Scales" -B "$project\build-clean-v066" -G "Visual Studio 18 2026" -A x64
cmake --build "$project\build-clean-v066" --config Release --target PnordeDreamSequencer_VST3 --parallel 4
```
