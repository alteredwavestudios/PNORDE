# PNORDE v0.6.29 — Windows VST3

Public repository preparation of the existing v0.6.29 engine; **not** Chord Mode.

- Faster internal Quick Program restoration by removing a redundant XML conversion.
- Play/Pause state independent of P1–P4 Quick Program snapshots.
- Developer Tools: callback/restore/queue statistics.
- Optional output-side MIDI Black Box with transport-stop trace capture.
- Existing saved-project compatibility marker retained (625); application build 629.
- GitHub-ready portable CMake and PowerShell tooling, additional source explanations.

**Known limitations:** Extremely fast transport/clock combinations and certain
instruments have not been exhaustively stress-tested. The MIDI Black Box cannot
report downstream synth volume or internal voice allocation.

**Important:** The original plugin has passed Windows regressions and Ableton
checks on its development setup. The portable JUCE dependency setup must be
built and verified on Windows before publishing a corresponding release binary.
