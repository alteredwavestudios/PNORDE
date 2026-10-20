# v0.5.8 — Performance and MIDI-safety notes

- Source is based on PNORDE v0.5.6.1; the selective repaint loop and optional heavy lighting remain unchanged.
- New UI dials are simple vector Components, using mouse-driven repaint, and do not cache full-screen bloom buffers.
- All new optional-glide and Aftertouch settings are fixed-capacity atomic arrays. Audio-thread interpolation uses stack arithmetic; no new allocations, locks, filesystem or UI calls were added to processBlock for the new features.
- Dedicated Mod Wheel, Expression, Vibrato Rate+Depth and Pan are sampled on the sequencer's existing 200 Hz modulation loop and suppress duplicate CC values.
- Sustain retains discrete CC64 on/off and does not interpolate.
- Global octave transposition clamps output to MIDI note 0–127, even when summed with the existing per-step OCTAVE lane.
- Four-column visual shading adds one lightweight translucent fill per visible cell. It does not change border colours; measure GUI CPU before v1.0.
- Source-level and standalone helpers were checked using AddressSanitizer and UndefinedBehaviorSanitizer; these do not prove full JUCE plugin memory safety or confirm host-side CPU improvements.

## v0.5.8 changes and trade-offs

- Experimental lighting requests 30 Hz, reduced from 45 Hz in v0.5.7 (33% fewer requested timer ticks). Non-experimental stays 24 Hz. This does **not** prove proportional CPU improvement.
- An active-step halo uses a bounded radius: cells more than four steps behind or two steps ahead skip exponential tail calculation. Constant idle backlight still uses cheap fills.
- Circular dial outlines/text can be drawn above darkness without excluding an undimmed rectangular region. The existing JUCE UI is not framebuffer-cached, and additional foreground overlays may incur drawing cost; performance must be measured in Ableton.
- New scale tables and manual tempo arithmetic use fixed arrays and stack-only arithmetic inside the audio processing path. Saved-state/XML and popup-menu allocations remain on UI/state threads, not per MIDI step.
- Added standalone sanitized stress coverage for 34-scale quantization (including quarter-tone approximations) and manual-beat-accumulator transport seeking/restarting.
- No Windows JUCE build, host profiler output, or full-plugin heap tracing has been collected here; "no leaks" cannot yet be guaranteed.

## Curated EDO generation (v0.5.8 candidate)

- The 13 EDO mode tables, melody pattern generator and quantisers are pure C++17, fixed-size and allocation-free. Generation is called on the **UI thread** only, on an explicit RANDOMISE-menu choice.
- Playback quantises one pitch event to the nearest EDO scale degree using a bounded search (five octaves × seven degrees); output uses the existing pitch-bend sampler and therefore allocates no new buffers in `processBlock`.
- The extra EDO parameter is **appended**, not inserted or expanded from the legacy scale ranges, protecting normalised host automation for existing choices.
- Tested in standalone C++17 AddressSanitizer/UBSan loops. Full JUCE plugin build, Ableton runtime performance and memory-leak checks remain unverified.

## v0.5.9 microtonal glide audit

- Reuses the original audio-thread SLIDE interpolation and 200Hz modulation scheduler; no new per-cell animations, framebuffers, dynamic allocations or locks in the audio callback.
- Effective bend-range selection uses constant comparisons and an unchanged atomic parameter. Old non-microtonal bend preferences are retained.
- Separate output mode / Poly AT settings are written on UI thread only; MIDI events use the existing bounded host MidiBuffer. Diagnostic values measure constructed MIDI, not guaranteed delivery.
- Standalone sanitizer tests exercise bend mode policy, EDO quantization and tempo-relative interpolation; full Windows JUCE/Live runtime profiling is still necessary.
