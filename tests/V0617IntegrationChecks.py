from pathlib import Path
root=Path(__file__).resolve().parents[1]
processor=(root/'Source/PluginProcessor.cpp').read_text()
editor=(root/'Source/PluginEditor.cpp').read_text()
header=(root/'Source/PluginProcessor.h').read_text()
ver=(root/'Source/PnordeProjectVersion.h').read_text()
cmake=(root/'CMakeLists.txt').read_text()
assert 'build = 617' in ver and 'release = "0.6.17"' in ver
assert 'VERSION 0.6.17' in cmake
assert 'pn_global_reverse' in processor and 'PnordeLaneStart' in processor
assert 'captureCurrentProgram(current)' in processor
assert 'savedPrograms[static_cast<size_t>(previous)] = std::move(current)' in processor
assert 'restoreProgramSnapshot(savedPrograms[static_cast<size_t>(selected)])' in processor
assert 'state.getProperty("PnordeLaneStart"' in processor
assert 'getLaneDirection(row)' in processor
assert 'PnordeProgramRequests::pack' in processor
assert 'PnordeProgramRequests::unpack' in processor
assert 'manualProgramEpoch.fetch_add' in processor
assert 'observedHostProgram' in processor and 'std::atomic<std::uint64_t> pendingSequencedProgram' in header
assert 'if (currentHost != queuedIndex)' in processor
assert 'if (currentHost != index)' in processor
assert 'state.setProperty("pn_program_slot", pinnedProgram' in processor
for k in ['"pn_darkness"', '"pn_theme"', '"pn_theme_extension"',
          '"pn_hypnosis"', '"pn_hppd_fps"', '"pn_vibes_bleed"']:
    assert k in processor[processor.index('bool PolyrhythmMIDIProcessor::restoreProgramSnapshot('):
                          processor.index('int PolyrhythmMIDIProcessor::getProgramLaneValue(')]
assert 'paintProgramLane(g);' in editor
assert 'paintProgramLaneForeground(g);' in editor
assert editor.index('paintProgramLane(g);') < editor.index('void PolyrhythmMIDIEditor::paintOverChildren')
assert editor.index('paintProgramLaneForeground(g);') > editor.index('void PolyrhythmMIDIEditor::paintOverChildren')
assert 'pnordeStepColumnBackground(paper, colour, stepIndex)' in editor
assert 'pnordeStepColumnBackground(t.paper, colour, index)' in editor
assert '(absoluteStep & 1) == 0' in editor
assert 'background(juce::Colour::fromRGB(20, 27, 41))' in editor
assert 'copyableUrl.setOpaque(true)' in editor
assert 'Version 0.6.17' in editor
for name in ['BUILD_PNORDE.ps1', 'INSTALL_PNORDE_ADMIN.ps1']:
    script=(root/name).read_text()
    assert 'build-clean-v0617' in script
    assert 'v0.6.17' in script
print('PASS v0.6.17 source contracts, program ownership, visual persistence and renderer')
