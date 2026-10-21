v0.6.2: See V062_STABILITY_AUDIT_AND_BUILD.md for new audio-thread scheduling guards, external MIDI worker changes, repaint invalidation improvements and benchmarking instructions. Existing compatibility tests retained.

## v0.6.1 verification

See `V061_BUILD_AND_TEST.md` for retained-lane-clock, automation and Vibes-specific tests.

# PNORDE v0.6.0 — Pre-Windows-1.0 test checklist

Do **not** publish as 1.0 until the clean Windows VST3 build and these tests pass.

## Host/state stability

- [ ] Load in Ableton Live 12, run a fresh MIDI project, stop/start/seek transport; no stuck notes.
- [ ] Check audio/MIDI timing under fast clock divisions, slow clocks, triplets and dotted modes.
- [ ] Open a **copy** of a v0.5.6.1 Ableton project; compare notes, CC values, patterns, tempo, MIDI routing, mute/stop and preset names.
- [ ] Old Aftertouch: load AUTO glide (`-1`) and explicit prior seconds values, confirm audible time unchanged, save/reload and re-test.
- [ ] New Aftertouch percentage controls behave like existing CC Macro percentages.
- [ ] Add every optional type; confirm only one of each fixed role and max eight macros. CC Macro uses only CC20–31.
- [ ] Set glide on Mod Wheel, Expression, Vibrato Rate+Depth and Pan. On Pan, confirm vertical-only motion changes CC10 while horizontal-only motion changes glide **without changing pan**, including with Shift held. Confirm Sustain sends instant off/on.
- [ ] Test fast step reassignments, note retriggers, changing MIDI channel, glide while host BPM changes, and multiple instances.
- [ ] Verify old/new state save/reload, Undo in host where available, keyboard automation and protected preset import.

## GUI and dark-room regression

- [ ] Test 0%, 25%, 50%, 75% Experimental Darkness with Indigo Rain and Sakura Haze.
- [ ] Settings popup is opaque, readable, and its edges have no bright rectangular seam. Also verify settings can be dismissed.
- [ ] MIDI Out moved to footer without overlapping Save Preset or other controls, at minimum and maximum editor sizes.
- [ ] Global Octave is centred at 0 and ranges only -2..+2; notes always remain MIDI 0–127.
- [ ] Key/Scale/Global Clock dials drag and wheel responsively, with no jumping back mid-interaction.
- [ ] Per-lane dual clock dials: normal drag changes division; Shift drag changes timing; double-click resets. Test in dark mode and under scroll.
- [ ] Hover Delete X: turns warning red and returns to normal after hover.
- [ ] Cell step numbers brighten on active step; columns show subtle four-step shade variation; off-page steps remain legible.
- [ ] Editable CC Macro labels, rail drag reorder, sequence-length popup and all working-area tooltips remain functional.
- [ ] Ensure no visual flashes in the header/footer; only the vertical lane rail rhythmically blinks.

## Memory and performance

- [ ] Run Release VST3 and separate `BUILD_ASAN_STANDALONE_WINDOWS.ps1` Debug standalone tests (never install ASan debug in system plugin directory).
- [ ] Exercise repeated editor creation/destruction, switching themes, resizing, rapid preset imports, lane adds/deletes, long host transport and multiple plugin instances.
- [ ] Compare CPU/idle redraw with v0.5.6.1 and measure real-time audio underruns.

## Known design constraints

- VST3 MIDI delivery depends on host routing and connected instrument/controller response.
- Existing user-saved Macro CCs outside 20–31 retain previous remapping safety policy; they are not automatically preserved as an unsafe controller.
- An old Aftertouch explicit-seconds value only switches to tempo percentage when edited horizontally.
- Global +/-2 octaves is **additional** to the per-step Octave lane; final output is clamped to 0–127, so extreme combinations may saturate at a boundary.

## v0.5.8 Windows pre-1.0 regression matrix

1. **Pan direction and separation**: Start at CC10=64. Drag left to reach <64 and right >64 without changing Glide; drag up/down to change Glide without changing Pan, including with Shift.
2. **Two clock dials per lane**: Division cycles through 1/32..32, adjacent S/T/D cycles independent of division. Change while playing and rescan/reopen; no overlap with page controls or CC name editor.
3. **Dark experimental mask**: Inspect light theme with 75% darkness and dark Indigo Rain. No bright rectangular patches behind dials; circle/arcs and labels remain legible.
4. **Page controls and tooltips**: Previous/Next/Add Page have steady coloured outlines, Remove Page turns warning red when hovered. Tooltip background must be opaque in light and dark themes.
5. **47 scale choices:** All 14 legacy choices restore correctly from old presets; click Scale dial for full 47-entry menu. Verify Phrygian Dominant `0,1,4,5,7,8,10`; quarter-tone Rast/Bayati with a pitchbend-capable MIDI instrument.
6. **Manual BPM**: Disabled defaults to host BPM. Activate MANUAL and enter 20, 90, 120, 300 BPM; verify internal step rate, no output while Ableton stopped, no stuck notes when switching BPM/modes or seeking. Confirm manual mode/state persists when saving new projects; old projects should load with Manual OFF.
7. **Performance and memory**: Compare GUI-open/closed and Experimental ON/OFF in Ableton using the same sequence and plugin count against v0.5.6.1/v0.5.7. Repeat resize, themes, scrolling, opening settings, adding/deleting and reordering lanes. Observe Task Manager private bytes over 20 minutes and compare idle drift. Standalone ASan tests do **not** establish full plugin memory-leak freedom.
8. **Old projects**: Back up first. Verify host automation (especially original pn_scale), lane order, MIDI routing, Aftertouch legacy glide, macro CC assignments and presets in copies. Never overwrite original Ableton sets as part of this test.

## EDO regression checklist (source candidate)

- [ ] All 34 pre-existing scale names/indices restore unchanged, including v0.5.8 choices with the old `pn_scale_ex` range **0..20**; new `pn_scale_edo` range **0..13** is appended.
- [ ] Select each of the 13 new EDO modes; drag Pitch steps in increments that reach notes *between* semitones; verify their sound using a pitch-bend-capable receiving instrument and matching ±2/12/24/48 bend range.
- [ ] EDO pitch is exact to its division, not rounded to ten cents in playback. Test key C and non-C; octave offsets; global octave; MIDI 0..127; note off/pitch-bend reset.
- [ ] Generate each of the four EDO patterns from RANDOMISE; check melodic motion, 16- and 128-step sequences, and that other lanes/gates remain unchanged. The EDO menu is disabled for conventional scales.
- [ ] Save/reload a new EDO project and reopen copies of older v0.5.6.1/v0.5.7 projects. Test existing key/scale automation and multiple plugin instances.
- [ ] Check one receiving device at each bend range; test synths that ignore pitch bend to verify documented fallback behaviour. Stereo/MPE polyphonic retuning is **not** claimed.
- [ ] Stress test under Ableton with slow and fast divisions, rapid scale changes, stop/start/seek, and project recall. Compare CPU usage with the last verified build.

## Updated v0.5.9 microtonal/SLIDE compatibility checks

- [ ] EDO, Free (0.1), Rast and Bayati automatically show **+/-2** in both header and Settings; bend control is temporarily disabled. Old stored `pn_bend` is unchanged and restored when switching back to a conventional scale.
- [ ] SLIDE remains visible and editable as a pitch approach control in all microtonal modes: 0% immediate; 50% half of the current sounding gate length; 100% at note-off, all relative to host/manual BPM. No old SLIDE values are erased.
- [ ] With **standard pitch bend** or **experimental MPE**, note-on pitch bend and all interpolated bends use an effective +/-2 range. MPE zone RPN requests +/-2; check recipient synth's member channel configuration. Poly Aftertouch is independently opt-in (defaults OFF) and can be enabled whether MPE is selected or not.
- [ ] Test consecutive large (>2 semitone) leaps: new-note pitch glide cannot cover the full jump under +/-2 and must clamp rather than send illegal bend values or stuck notes.
- [ ] Confirm Ableton forwards Pitch Wheel events via MIDI Monitor; exact microtones cannot be guaranteed by code tests alone.

## v0.6.0 test matrix (required before tagging 1.0)

- [ ] Install only after a clean successful `build-clean-v061` Release VST3 build. Confirm plugin version and MIDI routing in a **new Ableton set**.
- [ ] Save a copy of each v0.5.9 project and verify loading leaves its earlier C4 pitch reference, CC routing, tempo, automation and EDO scales unchanged. In a new project, confirm C3 MIDI note 48 at global octave 0 and range -3..+3 clips safely at MIDI 0..127.
- [ ] Right-click ALL lane labels (including SEQ and editable Macro labels) and set Forward, Backward, Ping Pong; verify audible MIDI sequences, 1-step and 2-step ping-pong, page numbers and saved preset reload.
- [ ] In Ableton automation/MIDI mapping, check Global Clock, Key, Scale, Octave, per-lane Division/Timing/Direction, Manual BPM/On, PNORDE Play and Reset. Verify they are distinct parameters with no cross-mapping; only musical controls are claimed as mappable.
- [ ] PAUSE silences PNORDE without stopping Ableton. RESET restarts local sequence phase while Ableton continues; no stuck notes, repeated pressure floods or sudden tempo jumps. Stress stop/play/seek/reset with fast and slow clocks.
- [ ] In Load Preset, test every built-in Ambient, Dance, Experimental and Generative recipe. Existing MIDI route remains valid; save/load copy works. Confirm Pan is unique and no more than 8 Macro lanes are created.
- [ ] Under RANDOMISE test all 12 Gate, 10 Velocity, 12 Pan and 10 SLIDE recipes. Test Pan positions 0..127 and tempo-relative glide 0..100; no changes to unrelated existing lanes when using a single-lane recipe.
- [ ] Compare idle CPU with v0.5.9 (Editor closed/open). With psychedelic OFF, verify no extra periodic gradients; with ON, compare 0, 10, 20 and 40% at three GUI sizes and after repeated open/close. Ensure no screen-wide strobe or unexpected redraw of header/footer.
- [ ] Run Windows AddressSanitizer standalone (`BUILD_ASAN_STANDALONE_WINDOWS.ps1`) and a Release run with multiple PNORDE instances, preset import/export, JUCE component destruction/recreation, memory snapshots and transport edge cases. These are NOT covered by the standalone header tests.
- [ ] Check font licence, JUCE licence and third-party attribution before publishing the matching *source* as an open-source release. The binary prerelease is a separate artifact.
