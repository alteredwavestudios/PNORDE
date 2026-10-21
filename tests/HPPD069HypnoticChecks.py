from pathlib import Path
r=Path(__file__).resolve().parents[1]
p=(r/'Source/PluginProcessor.cpp').read_text()
h=(r/'Source/PluginProcessor.h').read_text()
e=(r/'Source/PluginEditor.cpp').read_text()
v=(r/'Source/PnordeVibes.h').read_text()
l=(r/'Source/PnordeLights.h').read_text()
assert 'build = 609' in (r/'Source/PnordeProjectVersion.h').read_text()
assert 'VERSION 0.6.9' in (r/'CMakeLists.txt').read_text()
assert e.count('Shibuya Neon') == e.count('Matcha Vapor') == e.count('Asakusa Night') == e.count('Koi Sunset') == 1
assert e.index('Ume Shadow') < e.index('Shibuya Neon')
assert 'addInt("pn_theme", "Colour Theme", 0, 7, 0);' in p
assert p.index('pn_theme_extension') > p.index('pn_hppd_fps')
assert 'pn_hypnosis' in h and 'PnordeHypnosis' in p
assert 'vibesMode.getSelectedId() % 5 + 1' in e
assert 'class ChromaticDrift final : public Effect' in v
assert 'std::make_unique<ChromaticDrift>()' in v
assert 'const float rMax = std::sqrt(float(w*w + h*h)) * 0.64f' in v
assert 'const int bank = s.running' in v
assert 'cellHalo' in l and 'interactionFlash[128]' in (r/'Source/PluginEditor.h').read_text()
assert 'l.activeStep != l.lastLedStep' in e and 'const float activity = previous * visualDecay' in e
assert 'hoverLift = l.hoverStep == i' in e
assert 'juce::HyperlinkButton' not in e
assert 'build-clean-v069' in (r/'BUILD_PNORDE.ps1').read_text()
assert 'build-clean-v069' in (r/'INSTALL_PNORDE_ADMIN.ps1').read_text()
print('v0.6.9 visual, MIDI-isolation, backwards-compatibility and budget checks passed')
