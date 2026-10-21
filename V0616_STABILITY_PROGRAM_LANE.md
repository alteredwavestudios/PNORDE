# PNORDE v0.6.16 — PROGRAM Lane & Stability Candidate

This is a **source candidate**, not a verified Windows VST3 or certified stable release. Compile with Visual Studio 18 2026 on Windows, test in a **copy** of an Ableton project, and retain v0.6.14/v0.6.15 backups.

## Included
- Dedicated PROGRAM lane with 0=Off or scene 1–4 cells, independent lane length, division, timing and saved start marker. A triggered cell requests a scene switch at the end of the cell.
- Four scene buttons: right-click to reset a slot or copy all its musical settings into another slot. Dialogs confirm replacement.
- Right-click lane header: duplicate the first half of the sequence into the second half, with bounds checks (only lengths <=64).
- Theme/brightness/HPPD presentation settings persist across program switches; musical values remain program-specific.
- Program selector remains exposed to Ableton; updated state-loading guards stop per-scene snapshot restore from reverting the selector.
- Four-step alternating background brightness, without recolouring buttons or sliders; title/logo hover and display tweaks.
- Previous pre-v0.6.15 states migrate as Program 1, preserving historical lane values.

## Important realtime limitation
The audio thread publishes a PROGRAM step's destination at the cell boundary; the actual state/XML restore occurs on a JUCE message-thread timer (~60 Hz). This design avoids parsing XML inside the audio callback but is **not sample-accurate**; an automatic switch can be late by approximately one timer interval or longer if the UI thread is busy. Test specifically under load and do not use for sample-tight performance transitions yet.

## Mandatory DAW checks
1. Confirm v0.6.14 historic projects load unchanged in Program 1, then save only an expendable copy.
2. Make four distinct slots; manually switch repeatedly and check the selected slot never jumps back.
3. Verify reset/copy do not mutate non-target slots and all four survive save/reopen.
4. Double a 32- and 64-step pattern; ensure the original half, derived half, and 128-step limit are correct.
5. Set PROGRAM lane triggers, watch end-of-cell transitions, test rapid events and Ableton automation conflicts, and confirm notes release safely.
6. Verify theme, darkness, logo text contrast, popup behaviour, and 4-step group shading in every theme and narrow/wide sizes.
7. Validate external controller mappings and old automation IDs after updating the VST3.

## Build and install
Run `BUILD_PNORDE.ps1` from ordinary PowerShell. Only after confirmed success, close Ableton, run `INSTALL_PNORDE_ADMIN.ps1` in elevated PowerShell. Installer backs up the previous VST3 and SHA256-checks the new binary. Neither has been executed in this Linux packaging environment.
