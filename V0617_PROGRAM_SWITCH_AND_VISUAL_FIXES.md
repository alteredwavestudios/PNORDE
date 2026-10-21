# PNORDE v0.6.17 — Program Switching & Visual Stability

## Scope

This maintenance candidate changes no plugin IDs or existing Ableton automation
parameter IDs. It does not add swing. Historical states supported by the
existing validator are intended to remain supported.

- Manual quick-program selection now invalidates queued PROGRAM-lane requests
  from before the click. Queued requests are tagged with an epoch so an old
  request cannot snap selection back after the user has changed programs.
- A message-thread-only host parameter shadow detects genuine Live automation
  changes; an explicit button press does not need to wait for a subsequent
  parameter callback. Duplicate host notifications are avoided.
- Before loading each destination, the outgoing bank is captured, including
  `pn_global_reverse`, individual `pn_lane_direction_0..20` and
  `PnordeLaneStart0..20`. The destination is restarted from its own markers.
- Theme, darkness, Vibes, hypnosis/glow, visual frame rate, animation settings,
  viewport dimensions and visual lane order are transplanted into the
  incoming snapshot **before** the APVTS state is replaced. Live will retain
  its automation authority if it is actively automating a visual parameter.
- PROGRAM lane background/cells are now composed with the other lane bodies,
  *before* the darkening layer. Only text and markers use the final luminous
  foreground pass, eliminating the separately bright white strip.
- Column backgrounds are **dark, light, dark, light**, repeated by absolute
  step index for every lane. The dark panels are substantially darker. Buttons
  and value sliders are not tinted by the header grouping.
- Bandcamp and GitHub copyable URL fields use white text on a fixed dark slate
  popup/card in every theme. They remain selectable text, never automatic links.

## Mandatory Windows / Ableton acceptance checks

1. Back up v0.6.16. Load v0.6.17 in a **new** Live Set; verify clicking 1→2→3→4→1 never reverts while the PROGRAM lane is off.
2. Give each bank distinctive scale/key, Global Reverse, FWD/REV/PING lane directions and start markers (including >16). Cycle repeatedly and verify recall.
3. Enter PROGRAM-lane cells with target programmes, start transport, and inspect whether changes occur at the desired step end. **Note:** execution remains on the message thread; exact sample-boundary timing is not guaranteed.
4. Save, close Ableton, reopen the set, test all four bank data plus theme/darkness/Vibes staying unchanged across switches.
5. Open an untouched copy of a pre-v0.6.15 project, confirm Program 1 restores, don't overwrite the original.
6. Test all themes at 0 and 75% darkness, 4/8/16 visible grid columns, URL selection/copy, and PROGRAM lane legibility.

The Linux standalone tests compile pure C++ helpers and validate source wiring;
they **do not** compile JUCE/VST3 or prove runtime stability in Ableton.
