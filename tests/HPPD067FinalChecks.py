from pathlib import Path
root = Path(__file__).resolve().parents[1]
p = (root/'Source/PluginProcessor.cpp').read_text()
e = (root/'Source/PluginEditor.cpp').read_text()
l = (root/'Source/PnordeLights.h').read_text()
d = (root/'Source/PnordeDials.h').read_text()
assert 'PnordeStateSchema' in p and 'PnordeSavedBuild' in p
assert 'state.setProperty("PnordeSavedPluginVersion"' in p
assert 'setScale(PnordeProjectVersion::restoredScale(hasSchema, savedScale));' in p
assert 'parameters.replaceState(state);' in p
assert p.index('const bool hasSchema') < p.index('parameters.replaceState(state);')
assert 'addInt("pn_hppd_fps", "HPPD Frames Per Second", 1, 60, 12);' in p
assert p.index('addInt("pn_hppd_fps"') > p.index('"pn_lane_restart_"')
assert 'frameIntervalMs = 1000.0 / requestedFps' in e
assert 'requestedFps * 2' in e and 'juce::jmin(60' in e
assert 'hppdFps.setRange(1, 60, 1)' in e
assert 'setHppdFps(static_cast<int>(state.getProperty("PnordeHppdFps",12)))' in p
assert 'laneLabelFont' in e and '"Orbitron"' in e
assert 'withMultipliedSaturation' in l
assert 'if (description == "SCALE") update(1)' in d
assert 'vibes.update(getWidth(), getHeight(), processor)' in e
print('v0.6.7 migration, HPPD scheduling, typography and lighting source checks passed')
