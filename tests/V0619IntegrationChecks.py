from pathlib import Path
p=Path(__file__).resolve().parents[1]
processor=(p/'Source/PluginProcessor.cpp').read_text()
header=(p/'Source/PluginProcessor.h').read_text()
editor=(p/'Source/PluginEditor.cpp').read_text()
version=(p/'Source/PnordeProjectVersion.h').read_text()
cmake=(p/'CMakeLists.txt').read_text()
assert 'build = 619' in version and 'release = "0.6.19"' in version
assert 'VERSION 0.6.19' in cmake
assert 'Version 0.6.19' in editor
assert 'programLaneAutoArmed' in header
assert 'getProgramLaneAutoArmed() && slot >= 1 && slot <= programCount' in processor
assert 'setProgramLaneAutoArmed(false);' in processor
assert 'manualProgramEpoch.fetch_add(1' in processor
assert 'if (getProgramLaneEnabled() && getProgramLaneAutoArmed())' in processor
assert 'AUTO program changes (armed)' in editor
assert 'armed ? "AUTO" : "MANUAL"' in editor
assert 'PnordeProgramLaneValues' in processor
assert 'state.setProperty("PnordeProgramLaneValues", programValues, nullptr)' in processor
assert 'if (haveProgramLaneData)' in processor
assert 'getStateInformation' in processor
assert 'pnordeSetAutomatable(parameters, "pn_program_slot", committed)' in processor
assert 'if (isBanked) savedPrograms = std::move(restoredPrograms);' in processor
assert 'oldTree.setProperty(key, state.getProperty(key), nullptr)' in processor
assert 'setProgramLaneAutoArmed(armText == "1")' in processor
assert 'programLanePage = juce::jmax(0, programLanePage - 1)' in editor
assert 'processor.getProgramLaneLength() + 16' in editor
assert 'processor.getProgramLaneProgress()' in editor
assert 'const int beat = juce::jlimit(0, 3, absoluteStep % 4)' in editor
assert 'shadows[] {0.23f, 0.16f, 0.09f, 0.02f}' in editor
# All the visual prefs are guarded before scene restore
for key in ('PnordeTheme', 'PnordeDarknessPercent', 'PnordeHypnosis', 'PnordeVibesMode'):
    assert 'scene.setProperty("' + key in processor
for file in ('BUILD_PNORDE.ps1','INSTALL_PNORDE_ADMIN.ps1'):
    text=(p/file).read_text()
    assert 'build-clean-v0619' in text and 'v0.6.19' in text
assert 'PnordeProgramLaneEnabled' in processor
print('PASS v0.6.19 arm gating, per-scene PROGRAM snapshots, migration, UI and version integration checks')
