# PNORDE v0.6.14 — State Recovery and UI Containment (candidate)

A narrowly scoped compatibility/GUI maintenance patch based on v0.6.13.
No new MIDI timing features, swing engine, plugin IDs or host automation IDs.

## 1. Saved session recovery

The v0.6.13 `pnordeStateTreeIsSafe()` rejected states with more than **256** root
properties. PNORDE serialises metadata and 21 lane configurations into root
properties; genuine full saved states exceed this ceiling. A failed host load
left the default pattern visible, with pitch/velocity/etc apparently lost.
The upper bound is now **512** root properties, with the existing safety checks
for tree depth, per-property length, number of children, saved arrays, maximum
lane count and schema versions still in place.

**Important**: v0.6.14 can only restore information that is still present in
an original project/preset. If a user opened a project in v0.6.13 and saved
over the original while it showed default data, that overwritten file cannot
be reconstructed by the new plugin. Restore an older project backup.

Verification on an original (not v0.6.13-overwritten) Ableton project is
required, including pitch data, sequence gates, CC macros, all lane values,
clock choices, mode/timing, and save/reopen.

## 2. Double custom labels

The always-visible native `TextEditor` and the GUI's final luminous label pass
painted over one another. The native editor now keeps its text transparent
when it lacks keyboard focus; editing makes its native glyphs visible.
The final label pass continues to paint the luminous label once.
Ctrl/Cmd-click still accesses direction settings and ordinary click can edit
custom names.

## 3. Bounded CallOutBoxes

`Settings`, `Bandcamp/About` and lane direction now call JUCE's
`CallOutBox::updatePosition(anchor, editorBounds.reduced(10))` so position
selection is constrained within the host editor.

The Settings content was 914px high despite a 760px minimum host editor. It is
now displayed inside a vertical `juce::Viewport` whose height adapts to the
available window. Controls and fonts are not crammed smaller. Mouse wheel
scrolls the settings content; clicking away dismisses the callout.

## Validation

`bash tests/RUN_STATIC_TESTS.sh` passes under Clang/ASan/UBSan in the build
environment. **The full JUCE VST3 Windows build is not available here.**
Compile v0.6.14 on Windows and confirm both popup behaviour and old session
restoration in Ableton before replacing your stable version permanently.

## Recommended test sequence

1. Close Ableton; back up original .als projects and .pnorde presets.
2. Run BUILD_PNORDE.ps1 in a normal PowerShell window.
3. After successful compilation, close Ableton and run INSTALL_PNORDE_ADMIN.ps1
   in Administrator PowerShell (backs up installed bundle and verifies SHA256).
4. Open a *copy* of a known-good pre-v0.6.13 Ableton project (never save over
   the original). Confirm 21 lane state restoration and values.
5. Open custom lanes, test Ctrl/Cmd-click direction, click away, then test
   Settings, Bandcamp/About, window resize and the 760px minimum height.
6. Save/reopen a test copy and compare MIDI output with the source session.
