# PNORDE v0.6.21 — build / install safety

This ZIP is SOURCE, not a compiled VST3. Linux CI cannot compile the Windows JUCE target.

Close Ableton before replacing the plugin. Build with `./BUILD_PNORDE.ps1` from PowerShell.
The isolated build is `H:\PNORDE-build-v0621`; source can remain deeply nested.
Verify the produced VST3 binary and its version before installation.

**Important:** After a successful Windows build, run `INSTALL_PNORDE_ADMIN.ps1` from Administrator PowerShell with Ableton closed. The installer stages on H:, verifies binary hashes and version metadata, and backs up/parks the previous VST3. Keep all v0.6.20 backups. An older v0.6.20 installer is explicitly renamed `DO_NOT_RUN_FOR_v0621`.

New commands in the GENERATE menu deliberately rewrite PITCH, OCTAVE, VELOCITY, GATE LENGTH, SLIDE, AFTERTOUCH and SEQ across the active Quick Program. Generated compositions can set 48/64/96/128 step lane lengths and align core lane divisions. Optional CC lanes and Quick Programs 2–4 are not altered.

This is a source candidate awaiting compilation and Ableton auditory testing. No claim of click-free switching is made: investigate instrument envelope and MIDI note-off/on ordering before adding risky audio-thread lookahead.
