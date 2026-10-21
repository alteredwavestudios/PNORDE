# PNORDE v0.6.13 — Interface & Sequencer Refinement (Windows test candidate)

## Intended changes

- Dark mode fixed labels and optional-lane labels are drawn in a late, bright, text-only layer centred over the sequence-length buttons. No permanent pale boxes around names.
- Popups/tooltips are clipped out of late darkness and HPPD overlays for legibility.
- Previous/next and add/remove page buttons receive late neon light restoration; add is theme-green and remove theme-coral.
- Responsive grid uses actual reserved layout bounds: 16 steps above ~1160 px; 8 steps at 855–1159; 4 below 855. Native right-side controls retain a safety margin.
- Footer controls and Designed by / LДNTERNS share measured, non-overlapping groups; compact windows use two rows. The clickable Bandcamp region matches the drawn credit exactly.
- Brightness slider remains inside Settings; Ctrl/Cmd + wheel adjusts brightness anywhere over the editor; ordinary/side scrolling remains available.
- Click on a step number changes length; Ctrl/Cmd + click on a visible number (within lane length) changes that lane's start step. Tiny start triangle click resets to step 1.
- Forward, reverse and ping-pong lane starts now use one pure helper; SEQ note-generation uses the same helper as other lanes. Starts are state-saved and restored after lane lengths, defaulting to 0-based step 0 for older projects.
- Macro compaction shifts start markers with their physical lane data. Removal of a page adjusts subsequent markers.
- Existing host parameter IDs and ranges have not changed. Project state schema bumped to 3 to identify 0.6.12 saves; earlier schema 1/2 and unversioned states remain accepted through existing validators.

## Practical validation needed on Windows

1. Build Release VST3 in the version folder. Install only after a successful compile and verified backup. v0.6.11 remains available.
2. Load an old Ableton project and compare all MIDI note events, scale and preset data before placing start markers.
3. Set SEQ and PITCH starts independently, including beyond step 16, save/reopen and confirm exact starting steps.
4. Test reverse and ping-pong at lengths 1, 7, 16, 128; use an actual receiving synthesizer / MIDI monitor.
5. Resize 800/855/1160/1380/1920 px and at 16:10 and ultrawide; inspect footer, popup and control hitboxes.
6. Test enabled/disabled dark mode, 75% darkness, HPPD on/off, link selection, and Control/Command+wheel.

## Performance and VST3 caveats

No new allocations in audio callback. Lane-start offsets are atomic ints read by a bounded pure index helper. Bright page buttons and text effects are lightweight GUI primitives, not blur frame buffers. However, complete Windows JUCE compilation, DAW-host popup layering, plugin resizing, and cross-host MIDI delivery **cannot be verified by Linux standalone helper tests**. These remain critical release blockers before declaring 0.6.12 stable.
