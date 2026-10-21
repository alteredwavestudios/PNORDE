# PNORDE v0.6.18 — Program Controls and Optional Lane

- The PROGRAM lane is optional and **hidden on fresh instances**. Add it from + ADD LANE > PROGRAM. A formerly saved lane with actual trigger steps will remain visible on migration. Right-click the lane header > Remove to hide it while preserving its values.
- Program division/timing now use real PnordeClockDial and PnordeTimingDial components, with the same mouse drag, scroll, reset, theme, dimming and illumination as musical lanes.
- Scene restoration pins the host-visible program parameter to the **requested destination**, never to the previous state being replaced. The committed state is published to Ableton after successful restoration; manual/automatic switches no longer publish the destination before the scene is loaded.
- PROGRAM lane enablement is saved at bank level, not per musical scene; all old parameter IDs remain intact.
- Program changes still load on JUCE's message thread, not sample-accurately on the audio thread. Test in Ableton before relying on live performance.
- Preserve important Live projects and previously compiled VST3 backups.
