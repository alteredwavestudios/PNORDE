from pathlib import Path
root = Path(__file__).resolve().parents[1]
processor = (root/'Source/PluginProcessor.cpp').read_text()
header = (root/'Source/PluginProcessor.h').read_text()
editor = (root/'Source/PluginEditor.cpp').read_text()
editor_h = (root/'Source/PluginEditor.h').read_text()
version = (root/'Source/PnordeProjectVersion.h').read_text()
assert 'build = 616' in version and 'release = "0.6.16"' in version
assert 'VERSION 0.6.16' in (root/'CMakeLists.txt').read_text()
assert processor.index('addInt("pn_program_slot"') > processor.index('addInt("pn_global_reverse"')
assert 'setProperty("pn_program_slot", pinnedProgram' in processor
assert 'restoreProgramSnapshot(snapshot)' in processor
assert 'void PolyrhythmMIDIProcessor::timerCallback()' in processor
assert 'pendingSequencedProgram.exchange(-1' in processor
assert 'programLaneBeat = progEnd;' in processor
assert 'PnordeProgramLane::completedIndex' in processor
assert 'getProgramLaneValue(outgoing)' in processor
assert 'programLaneCells' in header
assert 'PnordeProgramLaneValues' in processor
assert 'PnordeProgramLaneStart' in processor
assert 'copyProgram(int source, int destination)' in processor
assert 'resetProgram(int index)' in processor
assert 'duplicateLanePattern(int row)' in processor
assert 'if (length < 1 || length > maxSteps / 2)' in processor
assert 'showProgramMenu(int index)' in editor
assert 'showProgramLaneMenu()' in editor
assert 'programLanePage' in editor_h
assert 'paintProgramLane(g);' in editor
assert 'paintProgramLane(juce::Graphics& g)' in editor
assert 'i % 4' in editor
assert 'setOpaque(true)' in editor
for file in ['BUILD_PNORDE.ps1', 'INSTALL_PNORDE_ADMIN.ps1']:
    script=(root/file).read_text()
    assert 'build-clean-v0616' in script
    assert 'v0.6.16' in script
print('PASS v0.6.16 source contracts: bank, PROGRAM lane, theme, UI, scripts')
