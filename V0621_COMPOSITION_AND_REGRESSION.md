# v0.6.21 — Neon Composition / verification notes

## Implemented in this source candidate

- `GENERATE` replaces the footer label `RANDOMISE` (legacy lane randomisers retained).
- Generate Melody: all existing 47 scales, 7 phrase recipes per scale; chromatic has a separate six-choice jazz-changes submenu (ii–V–I, tritone, turnaround, minor ii–V–i, major-third cycles, 12-bar blues).
- Generate Arpeggio: all existing 47 scales, 6 recipes per scale.
- Generate Composition: 16 multi-lane named pieces / 48, 64, 96, 128 step arrangements, grouped by musical inspiration. Includes true twelve-tone pitch-class row permutations. These are single-program arrangements, NOT automatic four-slot chains.
- LOAD PRESET: 16 new factory compositions alongside the existing legacy factory patterns. Factory-composition seeds are fixed so repeated loading of the same item produces identical results.
- Gate patterns: eight additional idiomatic/euclidean ideas; legacy 12 intact.
- Coherent PITCH, OCTAVE, VELOCITY, SEQ, GATE LENGTH, SLIDE and AFTERTOUCH, with intentional length and division alignment on the first seven lanes. No modifications to optional macro CC rows, per-lane direction, start markers, global reverse or program snapshot serialization.
- PROGRAM: length button and cells brighter neon lavender with stronger bloom and black text; high darkness receives final-pass reillumination.
- CC selectors: a grouped x-aligned pair flush with first grid cell, narrower and above the number header with a gap.
- `Designed by` + `LДNTERNS`: shared footer anchor and baseline at wide/narrow widths.
- Metadata / GUI version bumped to 0.6.21, source build script updated to short H: output path.

## Technical limitations

- Windows VST3 **not compiled or run** in this Linux test environment. The ZIP is a source candidate awaiting Visual Studio + Ableton validation.
- Named cultural traditions are musical sketches in PNORDE's current EDO/12-TET framework, NOT authentic maqam, raga, gamelan intonation or idiomatic performance synthesis.
- Each recipe is monophonic in the SEQ lane; polyphonic counterpoint and multi-instrument generation are not implemented.
- Generator applies directly to current program after selection; use SAVE PRESET beforehand. There is no preview/undo/random-seed UI yet. The factory LOAD PRESET compositions are deterministic; GENERATE variations use RNG.
- Sequenced PROGRAM's audio-thread request is still committed on the GUI timer; no speculative audio-thread snapshot restore or sample-accurate program commit is added. Audible clicks may come from synth envelopes/note retriggers and async scene commit. Evaluate Ableton MIDI event traces before an architecture change.

## Automated test scope

`bash tests/RUN_STATIC_TESTS.sh` runs source integration tests, legacy sanitizer tests, and new v0.6.21 C++ generation tests with clang address/UB sanitizers. New tests cover all 47 scales, lengths 1–128 (sampled), all melody and arpeggio algorithms, deterministic seeds, note/velocity/octave bounds, 12-tone row cardinality per transformation, 16 composition definitions, six jazz styles, eight gate patterns. No Windows JUCE compilation available here.

## Manual Ableton regression matrix (NOT YET VERIFIED)

1. Compile VST3 Release under `H:\PNORDE-build-v0621`; confirm plugin metadata says `0.6.21` and SHA256 matches installed binary after safe install.
2. In an empty Live Set, open in light and dark theme; resize through 4/8/16-step views, scroll to optional CC lanes and PROGRAM lane; inspect footer and dropdown group.
3. PROGRAM length and cells should remain legible and neon in light/dark palettes, high darkness and mouseover.
4. Quick Programs 1–4 still switch reliably; Global Reverse ON in slot 1 / OFF in slot 2 survives 1→2→1; test saved/reopened Live Set.
5. With PROGRAM MANUAL, verify manual selection takes precedence. With AUTO, program triggers still work; check MIDI click on note boundary using at least two instrument synths with different release envelopes.
6. Verify melody generation writes all seven lanes; choosing alternate scales changes scale; load composition sets the expected 48/64/96/128 length; factory compositions are reproducible.
7. Existing factory presets and EDO pitch-only patterns still available and do not change unrelated lanes; legacy randomise and `.pnorde` save/load still work.
8. Confirm optional CC macro names, selectors, numbers, settings/theme global state, and Program scene state still persist.

DO NOT overwrite important Live Sets until these checks pass. Keep the working v0.6.20 backed up.
