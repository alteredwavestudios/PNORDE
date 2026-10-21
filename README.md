# PNORDE v0.6.21 — Neon Composition (source candidate)

Forked directly from stable v0.6.20. Adds `GENERATE` menu with 47 scale
submenus, coordinated melody/arpeggio generation, six specific chromatic jazz
chord-change recipes, 16 named compositions (48–128 steps), eight new gate
recipes, neon PROGRAM lane, grouped CC selectors and footer signature anchoring.

**IMPORTANT**: This is source, not a tested Windows VST3. No Windows/Ableton
compilation has taken place. Keep v0.6.20 backed up. This pass does NOT change
the real-time sequenced PROGRAM state scheduler or claim to fix synth clicks.

## Build from ordinary PowerShell on Windows 11

Extract ZIP anywhere on H:, `cd` into the inner project folder, then run:

```powershell
.\BUILD_PNORDE.ps1
```

After a successful build, close Ableton and run `INSTALL_PNORDE_ADMIN.ps1`
in Administrator PowerShell to back up and install v0.6.21.

The build script uses the short absolute build directory `H:\PNORDE-build-v0621`
to avoid MSBuild `FTK1011` path-length problems and prevents compiling an
unrelated v0.6.19 source. It expects JUCE to be installed at one of the
locations listed in `CMakeLists.txt`.

## Testing

On Linux/macOS with clang++: `bash tests/RUN_STATIC_TESTS.sh` for bounded
state and musical-theory checks (including sanitizers).
On Windows: compile, then test separately in a NEW Ableton Live Set before
replacing your stable setup. Audit notes: `V0621_COMPOSITION_AND_REGRESSION.md`.

## Intentional generation behaviour

Full melody, arpeggio and composition actions rewrite the current program's
seven primary musical lanes, align their divisions with SEQ, and set matching
lengths. This **replaces those values**; save your work first. Compositions
may choose up to 128 steps. The program bank and global reverse are untouched.
Legacy PITCH-only EDO patterns and single-lane randomisation still exist.
