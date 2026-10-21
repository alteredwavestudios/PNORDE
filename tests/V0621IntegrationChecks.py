from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=(root/'Source/PluginProcessor.cpp').read_text()
e=(root/'Source/PluginEditor.cpp').read_text()
v=(root/'Source/PnordeProjectVersion.h').read_text()
c=(root/'CMakeLists.txt').read_text()
assert 'build = 621' in v and 'release = "0.6.21"' in v
assert 'VERSION 0.6.21' in c
assert 'pnordeFindSavedParameter' in p
assert 'child.getProperty("id").toString() == parameterId' in p
assert 'pnordeSavedParameterExists(state, "pn_global_reverse")' in p
assert 'if (!state.hasProperty("pn_global_reverse"))' not in p
assert 'pnordeKeepLiveParameter(scene, liveParameters, key)' in p
assert '"pn_program_slot"})' in p
assert '"pn_hypnosis", "pn_program_slot"' in p
assert 'pnordeSetAutomatable(parameters, "pn_program_slot", committed)' in p
assert 'const auto inkForLight = shade.interpolatedWith(juce::Colours::black, 0.46f)' in e
assert 'const auto inkForDark = shade.interpolatedWith(juce::Colours::white, 0.44f)' in e
assert 'state.setProperty("PnordeProgramLaneValues", programValues, nullptr)' in p
print('PASS v0.6.21 APVTS child-state, Global Reverse, selection and light-label source checks')

g=(root/'Source/PnordeCompositionEngine.h').read_text()
assert 'constexpr int melodyCount = 7;' in g
assert 'constexpr int arpCount = 6;' in g
assert 'inline void makeSerial' in g
assert 'constexpr std::array<Composition, 16>' in g
assert 'const float top = stacked ? h - height - 6.0f' in e
assert 'const int selectX = juce::roundToInt(gridStartX())' in e
assert 'Generate Composition (changes lane lengths)' in e
assert 'PnordeCompositionEngine::fill(phrase,scale' in e
assert 'pnordeKeepLiveParameter(scene, liveParameters, key)' in p
assert 'Source/PnordeCompositionEngine.h' in c
assert (root/'Source/PluginEditor.h').read_text().count('"GENERATE"') == 1
print('PASS v0.6.21 generator integration, protected v0.6.20 state and layout checks')
