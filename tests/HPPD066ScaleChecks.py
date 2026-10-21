from pathlib import Path
r=Path(__file__).resolve().parents[1]
e=(r/'Source/PluginEditor.cpp').read_text()
p=(r/'Source/PluginProcessor.cpp').read_text()
v=(r/'Source/PnordeVibes.h').read_text()
assert 'addInt("pn_scale", "Scale", 0, 13, PnordeProjectVersion::newInstanceMajorScale);' in p
assert '"NovaScale", PnordeProjectVersion::newInstanceMajorScale' in p
assert 'state.getProperty("NovaScale", getScale())' not in p
assert 'state.setProperty("NovaScale", getScale(), nullptr);' in p
assert 'processor.setLaneValue(1, step,' not in e[e.index('scaleDial.onChange'):e.index('addAndMakeVisible(scaleDial)')]
assert 'processor.setLaneValue(1, i,' not in e[e.index('scaleSelect.onChange'):e.index('for (int i = 0; i < 5; ++i) globalClockSelect')]
assert 'UNCANNY CHESS' in e
assert 'const float ripple = ' in v and 'const float skew = ' in v
print('v0.6.7 scale and chess source checks passed')
