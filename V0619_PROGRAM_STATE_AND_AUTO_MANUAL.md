# v0.6.19 — PROGRAM lane ownership and manual override

- **Root cause isolated by user:** manual program button works while PROGRAM
  trigger cells are cleared. The automatic lane kept queuing future program
  changes after each manual selection, undoing it on a later cell boundary.
- New global AUTO/MANUAL arm flag: manual selection disarms without removing
  any patterns. Live MIDI/parameter changes also disarm; AUTO requests never
  disarm themselves.
- PROGRAM trigger cells, division, start, timing and length are stored inside
  each musical program. Scene copy/reset now handles them along with every
  other musical property; lane visibility and arming are global.
- Legacy four-program banks with the old common lane migrate that shared
  pattern to all four scenes. No user information is discarded.
- The PROGRAM visual uses its theme's lane accent, proper playhead outline,
  progress meter, + / - page length buttons and forward/back page controls.
- Column backgrounds use a subtle four-level progression per four steps
  instead of an alternating checkerboard.

## Acceptance test

1. Load a *copy* of a v0.6.18 project with PROGRAM triggers. Manual program
   selection should remain stable even while transport is playing.
2. Right-click PROGRAM and choose AUTO; automatic programmed transitions
   should resume. Manually select a program: status returns to MANUAL.
3. Give all four programs distinct trigger patterns and starting positions;
   save/reload and verify each scene's complete set of values.
4. Reset one scene: its PROGRAM pattern resets while the other three do not.
5. Check page add/remove and navigation, warning glow and cursor progress.

This candidate still requires Windows VST3 compilation and Ableton testing.
