# PNORDE v0.6.20 — targeted restoration fix

This is a narrow maintenance candidate based on v0.6.19. No new synthesis,
sequencing, automation IDs, PROGRAM-lane cells or preset format changes.

## Root causes addressed

1. v0.6.19 checked the `pn_global_reverse` parameter as an APVTS root
   property, but JUCE stores APVTS parameters as child nodes with `id`
   and `value`. Therefore every quick-program snapshot load incorrectly set
   Global Reverse to OFF. The loader now detects both child and historic root
   representations, defaulting OFF only if truly absent.
2. v0.6.19 pinned Quick Program at the state root, but a saved snapshot
   could replace its actual APVTS parameter child with a stale selection.
   Scene restore now preserves the live `pn_program_slot` child and commits
   the selected destination through the existing host parameter API.
3. The same child/root confusion prevented preservation of global visual
   APVTS parameters. Visual parameter children now follow the current live
   appearance when loading musical programs. Global Reverse remains per-scene.
4. Light-theme lane titles use a deeper accent ink and reduce the glow halo
   until darkness is enabled. Dark theme lane emissions remain bright.

## Backward compatibility

Existing PNORDE v0.6.19 and earlier .pnorde/Live states are not migrated or
rewritten until saved. All old host parameter IDs, order, plugin identity,
and four-program slot data remain unchanged. Keep source/Live Set backups.

## Testing required in Ableton

Test Global Reverse ON in Program 1 and OFF in Program 2, repeatedly switch
1→2→1 and confirm each setting persists. Do this first with PROGRAM AUTO
disarmed and without Live automation. Then test program triggers and host
mapping separately. Also test saving and reopening a four-program Live Set.

This is a candidate until the Windows JUCE build and Ableton runtime checks
have passed. The internal PROGRAM lane still uses message-thread snapshot
commit; it's not sample-accurate. If a program still reverts with AUTO disarmed,
inspect Live's automation lane/mapped controller or collect a precise repro.
