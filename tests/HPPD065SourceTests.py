from pathlib import Path
root = Path(__file__).resolve().parents[1]
vibes = (root / 'Source/PnordeVibes.h').read_text()
editor = (root / 'Source/PluginEditor.cpp').read_text()
proc_h = (root / 'Source/PluginProcessor.h').read_text()
cmake = (root / 'CMakeLists.txt').read_text()
assert 'class KumikoChecker final : public Effect' in vibes
assert 'class PrismSpikes final : public Effect' in vibes
assert 'const int side = (row + sector) % 4;' in vibes
assert 'const float advance = 0.08f + 0.73f * cycle;' in vibes
assert 'const int bank = s.running' in vibes and '* 0.002' in vibes
assert 'l.enabled = (row % 3 == bank)' in vibes
assert vibes.count('g.strokePath(path, juce::PathStrokeType(width));') == 1
assert 'width * 6.0f' not in vibes
assert 'std::make_unique<KumikoChecker>()' in vibes
assert 'if (next == mode) return;' in vibes and 'effect.reset();' in vibes
assert 's.palette = palette;' in vibes and 'vibes.setPalette({' in editor
assert 'juce::jlimit(0, 4, mode)' in proc_h
assert 'vibesMode.getSelectedId() % 5 + 1' in editor
assert 'frameIntervalMs = 1000.0 / requestedFps' in editor
assert 'copyableUrl.setReadOnly(true)' in editor
assert 'copyableUrl.setText("l4nterns.bandcamp.com", false)' in editor
assert 'juce::HyperlinkButton updates' not in editor
assert 'github.com/alteredwavestudios/PNORDE' in editor
assert 'pn_vibes_bleed' in (root/'Source/PluginProcessor.cpp').read_text()
assert 'VERSION 0.6.9' in cmake
print('v0.6.7 HPPD source structure checks passed')
