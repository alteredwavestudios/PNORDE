# v0.6.10 technical notes

- New theme indices are 8–11 and original indices remain 0–7. To protect the old `pn_theme` automation range, `pn_theme_extension` is appended with 0=legacy, 1–4=new.
- `pn_hypnosis` is appended; all legacy host parameter IDs remain in place. Stored custom properties mirror the new slider value. State schema stays 2 and build advances to 609.
- HPPD `ChromaticDrift` is a fifth `Effect` subclass; no effect is instantiated concurrently with another. One renderer ARGB buffer remains capped at 768×512.
- HPPD frame refresh is separate from MIDI. Seven rotating lane samples are used per heavy frame; the sampled lane phases and CC/value-derived measures drive colour bands and pools. No new audio-thread allocations.
- Active grid halos are one radial gradient around each recently triggered or interacted-with visible cell. Decay is tied to the GUI timer rate so release duration doesn't shorten radically when using 60 FPS.
- `rangeOpacity` is now triggered on a lane step transition rather than being reset to 1 on every timer tick during the entire step. This is important for slow clock divisions and avoids sustained peak brightness.
- The titles use an explicit JUCE `Rectangle<int>` call to avoid Visual Studio 2026 C2668.
- The UI remains responsive at 16/12/8/4 visible cells and respects the same max 128-step lane storage boundary. The About and Bandcamp addresses remain copyable, not clickable.
