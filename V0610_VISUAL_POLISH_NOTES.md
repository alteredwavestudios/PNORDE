# PNORDE v0.6.10 candidate - visual polish notes

## Implemented in this candidate
- Transparent lane-name / macro-label editors: text glows against the dark field rather than sitting in light boxes.
- Footer credit updated to **Designed by LДNTERNS** and grouped bottom-right with larger signature text.
- Bandcamp / GitHub copy fields now use dark text in light themes for readability.
- New **Grid glow / halos** Settings toggle disables halo bloom while keeping bright flash/fill feedback.
- New default HPPD mode for fresh sessions: **Chromatic Drift**.
- Chromatic Drift now has a subtle visual-snow / feedback veil.
- Neon Nodal geometry enlarged and strengthened.
- Responsive 16-step preservation relaxed: editor holds 16 visible columns down to a narrower width and compresses column spacing before dropping to 12.
- Dial faces darkened slightly; arc/fill accents brightened.
- Removed one redundant `atmosphere()` pass in the Kumiko/Uncanny Chess HPPD renderer.

## Still best treated as next-pass work
- Smooth sub-page side-scroll for 12/8/4-column modes (requires per-lane viewport offset state inside each visible 16-step page).
- Zippy animated page-to-page 'catapult' transition.
- External visual-engine data export.

## Audit / risk notes
- HPPD stays editor-only and message-thread-only, so it cannot break MIDI timing directly.
- The main reliability risk remains GUI repaint cost in some hosts if very high FPS + multiple glows are enabled. The new glow toggle helps here.
- VST3 caveat: host-side UI throttling / occlusion / resize behaviour can differ between DAWs, so animation smoothness may vary even when MIDI remains correct.
