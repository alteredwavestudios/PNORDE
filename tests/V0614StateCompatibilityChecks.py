from pathlib import Path
import re
r = Path(__file__).resolve().parents[1]
p = (r / 'Source/PluginProcessor.cpp').read_text()
e = (r / 'Source/PluginEditor.cpp').read_text()
h = (r / 'Source/PluginEditor.h').read_text()
# Bound is permissive enough for all 21 serialized lanes, 14 optional rows
# and parameter metadata, yet rejects unusually large injected trees.
assert 'state.getNumProperties() > 512' in p
assert 'state.getNumProperties() > 256' not in p
assert 'state.getNumChildren() > 160' in p
assert 'getStateInformation(' in p and 'loadValidatedState(' in p
assert 'parameters.replaceState(state)' in p
assert p.index('parameters.replaceState(state)') > p.index('state.getNumProperties() > 512')
assert p.index('parameters.replaceState(state)') > p.index('PnordeLaneStart" + juce::String(row)')
assert 'state.getProperty(prefix + "Values")' in p
assert 'state.setProperty(prefix + "Values", values, nullptr)' in p
assert 'state.setProperty("NovaLaneDataVersion", 13, nullptr)' in p
assert 'setLaneValue(row, i,' in p and 'setStepEnabled(i,' in p
assert '"NovaScale", PnordeProjectVersion::newInstanceMajorScale' in p
assert 'setScale(PnordeProjectVersion::restoredScale(hasSchema, savedScale))' in p
assert 'setLaneName(row,' in p
assert 'getLocalBounds().reduced(10)' in e
assert 'class PnordeSettingsViewport final' in e
assert 'viewport.setViewedComponent(contents.release(), true)' in e
assert 'if (!hasKeyboardFocus(true)) return;' in h
assert 'editingTextColour : juce::Colours::transparentBlack' in h
print('PASS: v0.6.14 source/state/property-limit regressions')
