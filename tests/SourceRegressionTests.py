"""Conservative source-structure checks. Not a substitute for a JUCE build."""
from pathlib import Path
import re

root = Path(__file__).resolve().parent.parent
cpp = (root/'Source/PluginEditor.cpp').read_text(encoding='utf-8')
proc = (root/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
header = (root/'Source/PluginEditor.h').read_text(encoding='utf-8')
cmake = (root/'CMakeLists.txt').read_text(encoding='utf-8')

def check(cond, message):
    assert cond, message
    print('PASS:', message)

check('getRootMenu()->addSubMenu' not in cpp, 'No duplicate Macro CC submenu')
check('for (int cc = PnordeSafeCC::first; cc <= PnordeSafeCC::last; ++cc)' in cpp,
      'CC macro selector uses centrally defined whitelist')
check('PnordeSafeCC::sanitise(number)' in proc, 'Processor enforces CC whitelist on imported state')
check('void PolyrhythmMIDIEditor::paintStaticLabels(' in cpp, 'Always-on typography path exists')
paint, overlay = cpp.split('void PolyrhythmMIDIEditor::paintOverChildren(',1)
check('paintStaticLabels(g);' in paint, 'Typography painted under native components')
check('paintStaticLabels(g);' not in overlay, 'No text duplicate in topmost light layer')
check('g.excludeClipRegion(settingsCallout->getBounds().reduced(3))' in overlay, 'Settings callout excluded from overlays')
check('setOpaque(true)' in cpp, 'Settings and Macro editor use opaque components')
check('settingsButton.getBounds(), this' in cpp, 'Settings popup anchor uses parent coordinates')
check('PNORDE - Dream Sequencer' in cmake and 'PLUGIN_CODE PnDr' in cmake and 'PLUGIN_MANUFACTURER_CODE AlWS' in cmake,
      'VST3 identity retained')
check('refreshMacroPlaceholders()' in cpp, 'Macro numbers are visible-order-based')
check('g.fillRect(0, juce::roundToInt(h * 0.110f), w,' in overlay and
      'const int gridX' not in overlay,
      'Experimental darkness spans the whole work area again')
check('protectControl(clockDials[row]);' not in overlay and
      'protectControl(laneTypeSelect[slot]);' in overlay and
      'protectControl(ccSelect[slot]);' in overlay,
      'Lane dropdowns protected; circular dials have no rectangular light holes')
check('protectControl(ccLabelEditors[slot]);' in overlay and
      'if (row >= 7 && isLaneType(row, PnordeLane::Type::ccMacro))' in overlay,
      'Native Macro text remains readable without duplicate title rendering')
# Liveness: the three compile failures from earlier v0.5.3 no longer present.
check('const auto fill = parameterFillBounds(row, bounds, f, processor.getOptionalLaneType(row));' in cpp,
      'Parameter-fill call has required fourth argument')
check('safe->isLaneType(row, PnordeLane::Type::pan)' in cpp,
      'Randomiser callback accesses editor through SafePointer')
print('Source regression checks complete.')

# v0.5.5 targeted regression checks (not a full C++ build).
check('PnordeAftertouchGlide::durationBeats(' in proc, 'Aftertouch uses bounded realtime ramp arithmetic')
check('#include "PnordeAftertouchGlide.h"' in proc, 'Aftertouch helper is compiled into processor')
check('state.setProperty("PnordeAftertouchGlide"' in proc, 'Aftertouch glide array is saved with presets')
check('PnordeAftertouchGlideUnits' in proc and 'glide[i].getIntValue() + 101' in proc,
      'Older seconds-based Aftertouch steps retain their original timing')
check('PnordeVisualOrder::move(' in proc and 'PnordeVisualOrder::valid(' in proc,
      'Optional lane reordering preserves a validated visual permutation')
check('getDisplayRowAt(i)' in cpp, 'Editor positions optional lanes by display order')
check('keyDial.onChange' in cpp and 'scaleDial.onChange' in cpp,
      'Key and scale use interactive dials')
check('"GLIDE TIME"' in cpp, 'Glide popup label is above the dial')
check('darkTheme ? (0.32f + 0.30f * activity) : (0.19f + 0.22f * activity)' in cpp,
      'Idle cells are brighter and active-cell backlight is intensified')
check('PnordeCogButton settingsButton;' in header,
      'Settings icon uses a vector cog instead of a system glyph')

# v0.5.6 UI and interaction checks
check('public juce::TooltipClient' in header and 'juce::String PolyrhythmMIDIEditor::getTooltip()' in cpp,
      'Drawn sequence values, lengths and rails provide contextual tooltips')
check('reorderHandle(row).contains(local)' in cpp and 'labelEditor.addMouseListener(this' not in cpp,
      'Reorder gesture uses enlarged rail handle, never CC name editor')
check('"SEQ LENGTH"' not in cpp, 'Crowded permanent sequence-length label removed')
check('Set Sequence Length:' in cpp, 'Length boxes show contextual click/drag help')
check('prev[row].setTooltip' in cpp and 'next[row].setTooltip' in cpp and
      'addPage[row].setTooltip' in cpp and 'removePage[row].setTooltip' in cpp,
      'Page controls all have explicit tooltips')
check('laneTypeSelect[slot].setTooltip' in cpp and 'ccSelect[slot].setTooltip' in cpp,
      'Both optional-lane dropdowns have tooltips')
check('t.cream.interpolatedWith(t.ink, 0.33f)' in cpp,
      'Dark-theme lane dropdowns have brighter static background')
check('Drag coloured lane rails to reorder' in cpp, 'Experimental settings label reflects new handle')

# v0.5.7 musical/visual regression contracts
check('PnordeTempoGlide::durationBeats(getOptionalGlide(row, step), laneDuration(row))' in proc,
      'Dedicated MIDI controls use tempo-relative glide')
check('type == 5 ? 0.0' in proc, 'Sustain never glides')
check('setGlobalOctave' in proc and 'getGlobalOctave())*12.0' in proc,
      'Global octave transposes with MIDI range protection')
check('state.setProperty("PnordeOptionalGlide"' in proc and 'PnordeAftertouchGlideUnits' in proc,
      'Glide format migration is explicit')
check('clockDials[row].setVisible(visible)' in cpp and 'timingDials[row].setVisible(visible)' in cpp and 'themeSelect.setBounds(' not in cpp,
      'Lane clock dial replaces two dropdowns and Theme is Settings-only')
check('footer(midiOutputSelect, 47, 138)' in cpp,
      'MIDI output relocated to left footer')
check('columnLight = 0.018f + (i % 4)' in cpp,
      'Subtle 4-column luminance cycle without changing outlines')
check('PnordeDeleteButton' in header, 'Delete X uses hover-warning renderer')

# v0.5.7 Pan axis independence
drag = cpp.split('void PolyrhythmMIDIEditor::updateDragValue(', 1)[1].split('void PolyrhythmMIDIEditor::mouseDrag(', 1)[0]
check('const bool editingGlide = !vibrato || e.mods.isShiftDown();' in drag,
      'Pan horizontal glide works without Shift; Vibrato still requires Shift')
check('const int amount = juce::roundToInt((pan ? dx : dy)' in drag,
      'Pan bipolar value is horizontal-only, independent of vertical glide')
check('const int glideDelta = pan ? dy : dx;' in drag,
      'Glide uses the vertical axis and fine Shift control for Pan')
check('Pan: left/right changes CC10' in cpp, 'Pan tooltip matches the new drag mapping')

# v0.5.8 regression contracts
check('PnordeScales::count' in cpp and 'PnordeScales::quantiseExtra' in proc,
      'Extended scales share single source of truth with quantiser')
check('addInt("pn_scale_ex", "Extended Scale", 0, 20, 0);' in proc,
      'Legacy pn_scale automation range preserved while extending scales')
check('PnordeManualTempo::advance({' in proc and 'getManualTempoBpm()' in proc,
      'Manual tempo uses a bounded opt-in BPM source')
check('previousHostBlockEndPpq' in proc and 'manualBlockEndPpq' in proc,
      'Manual clock handles host beat discontinuities')
check('state.setProperty("NovaLaneDataVersion", 13, nullptr);' in proc,
      'Saved-state migration version bumped')
check('timingDials[row].drawIllumination' in cpp and
      'clockDials[row].drawIllumination' in cpp,
      'Independent dial illumination, without rectangular mask holes')
check('const int glideDelta = pan ? dy : dx;' in drag and '(pan ? dx : dy)' in drag,
      'Pan left/right and Glide up/down are independent')
check('manualTempoToggle' in cpp and 'manualTempoInput' in cpp,
      'Manual BPM toggle and editable field have UI wiring')
print('v0.5.8 source regression checks complete')

# v0.5.8+ EDO additions: preserve both older automation parameter ranges.
check('addInt("pn_scale_ex", "Extended Scale", 0, 20, 0);' in proc,
      '34 existing scale indices retain automation range')
check('addInt("pn_scale_edo", "Microtonal EDO Mode", 0, PnordeEDO::modeCount, 0);' in proc,
      'Additional EDO scales have their own appended automatable parameter')
check('PnordeEDO::quantiseSemitones(rawPitch,octave,getKey(),getScale())' in proc,
      'EDO output uses precise fractional-semitone pitch bend')
check('PnordeEDO::generate(pitches,count,engine.getKey(),engine.getScale()' in cpp,
      'Four pattern generators update only the PITCH lane')

# v0.5.9 forced micro bend and tempo-relative SLIDE safety.
policy = (root/'Source/PnordeMicroOutput.h').read_text(encoding='utf-8')
check('isMicrotonalScale(int index)' in policy and 'index == 13 || index == 32 || index == 33' in policy,
      'EDO plus free microtonal and quarter-tone modes use the same output policy')
check('microtonal ? microtonalBendSemitones : traditionalRange' in policy and
      'effectiveBendRangeIndex(bool microtonal' in policy,
      'Microtonal bend is +/-2 while original conventional bend preference persists')
check('PnordeMicroOutput::slideApproachBeats(' in proc and
      'const int slide = getLaneValue(5, stepAt(5, boundary));' in proc,
      'SLIDE controls tempo-relative pitch approach and is not bypassed in EDO')
check('SLIDE BYPASSED' not in overlay and 'isEdoSlideBypassed()' not in cpp,
      'Microtonal SLIDE lane remains visible and editable')
check('juce::MidiMessage::controllerEvent(2, 6, 2)' in proc,
      'Experimental MPE requests a matching +/-2 bend range')
check('addInt("pn_poly_pressure", "Experimental Poly Aftertouch", 0, 1, 0);' in proc and
      'if (getExperimentalPolyPressure())' in proc and
      'if (mpe && getExperimentalPolyPressure())' not in proc,
      'Poly Aftertouch is separately enabled, defaults off and does not require MPE')
check('processor.getEffectiveBendRangeIndex()' in cpp and
      'bendRangeSelect.setEnabled(!processor.isMicrotonalScaleSelected());' in cpp,
      'Visible Bend Range selector follows the effective bend and disables in microtonal scales')
print('v0.5.9 microtonal/slide source regression checks complete')

# v0.6.0 candidate contracts
check('addInt("pn_play_enabled", "PNORDE Play", 0, 1, 1);' in proc and
      'addInt("pn_reset_counter", "PNORDE Reset Trigger", 0, 127, 0);' in proc,
      'Play/Reset are host-automatable with stable IDs')
check('pn_lane_direction_' in proc and 'PnordePlayback::index' in proc,
      'Lane directions affect MIDI step lookup, not just playhead display')
check('const auto stopIfNecessary' in proc and 'if (previousBlockWasPlaying || noteIsActive) reset();' in proc,
      'Host stop avoids redundant pressure resets on silent blocks')
check('PnordeFactoryPresets::presets' in cpp and 'applyFactoryPreset' in cpp,
      'Categorised factory presets are loaded into real sequencer state')
check('PnordePatternLibrary::pans' in cpp and 'PnordePatternLibrary::slides' in cpp,
      'Separate bounded Pan and SLIDE recipes')
check('class PnordeMacroNameEditor' in (root/'Source/PluginEditor.h').read_text(encoding='utf-8') and
      'labelEditor.onDirectionMenu' in cpp,
      'Editable Macro labels support independent right-click playback menu')
check('vibes.paintBackground(g,' in cpp and
      'processor.getPsychedelicEnabled()' in cpp and
      'vibes.paintOverlay(g,' in cpp,
      'Vibes renders only when opted in; geometry is cached and grid bleed is bounded')
check('"PnordePitchAnchorLow"' in proc and 'getLowPitchAnchor()?48.0:60.0' in proc,
      'C3 base pitch for new projects with legacy C4 compatibility')
print('v0.6.0 source regression checks complete')

# v0.6.1 transport and automation regression checks
check('retainedBeatPpq = end;' in proc and
      'const double start = retainedBeatPpq;' in proc and
      'laneCurrentSteps[row].store(-1' not in proc.split('void PolyrhythmMIDIProcessor::processBlock(',1)[1],
      'Pause and host stop retain musical phase and lane positions')
check('pn_lane_restart_' in proc and 'getLanePhaseOrigin(row)' in cpp and
      'RESTART THIS LANE (automatable)' in cpp,
      'Independent lane restart automation and UI menu are present')
check('pn_vibes_bleed' in proc and 'vibesBleed' in cpp,
      'Vibes grid bleed is a host-visible automation parameter')
check('PnordeVibes.h' in (root/'Source/PluginEditor.h').read_text(encoding='utf-8') and
      'juce::Image frame;' in (root/'Source/PnordeVibes.h').read_text(encoding='utf-8') and
      'std::unique_ptr<Effect> effect;' in (root/'Source/PnordeVibes.h').read_text(encoding='utf-8'),
      'Vibes holds one bounded frame and exactly one selected effect')
print('v0.6.1 source regression checks complete')

check('class NeonNodal final : public Effect' in (root/'Source/PnordeVibes.h').read_text(encoding='utf-8') and
      'class PhaseWeave final : public Effect' in (root/'Source/PnordeVibes.h').read_text(encoding='utf-8'),
      'Independent Neon Nodal and Phase Weave effect classes exist')
check('effect.reset();' in (root/'Source/PnordeVibes.h').read_text(encoding='utf-8') and
      'frame = {};' in (root/'Source/PnordeVibes.h').read_text(encoding='utf-8'),
      'Switching visual modes releases old effect and frame buffer')
check('frameIntervalMs = 1000.0 / requestedFps' in cpp and
      'nextVibesFrameMs' in cpp and
      'vibes.update(getWidth(), getHeight(), processor)' in cpp,
      'HPPD frame generation uses the requested 1-60 FPS without catch-up bursts')
check('PnordeVibesMode' in proc and 'vibesMode.addItem' in cpp,
      'Active effect mode is saved and selected in Settings')
print('v0.6.3 visual isolation regression checks complete')
