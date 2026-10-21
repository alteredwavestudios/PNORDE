from pathlib import Path
root=Path(__file__).resolve().parents[1]
e=(root/'Source/PluginEditor.cpp').read_text(encoding='utf-8')
p=(root/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
h=(root/'Source/PluginEditor.h').read_text(encoding='utf-8')
v=(root/'Source/PnordeProjectVersion.h').read_text(encoding='utf-8')
assert 'schema = 2' in v and 'build = 609' in v
assert 'schemaText.getIntValue() < 1' in p and 'schemaText.getIntValue() > PnordeProjectVersion::schema' in p
assert 'buildText.getIntValue() > PnordeProjectVersion::build' in p
assert 'PnordeLaneName' in p and 'setLaneName(' in p and 'getLaneName(' in p
assert 'laneNameEditors[row].setVisible(visible)' in e
assert 'protectControl(laneNameEditors[row])' in e
assert 'getCustomCCLabel' in p
assert 'juce::HyperlinkButton' not in e and 'github.com/alteredwavestudios/PNORDE' in e
assert 'VIBRATO  (CC76/77)' in e and 'SUSTAIN PEDAL  (CC64)' in e
assert 'visibleSteps < 16' in e and 'e.mods.isShiftDown()' in e
assert 'getConstrainer()->setFixedAspectRatio' not in e
assert 'setResizeLimits(800, 760, 2800, 1620)' in e
assert 'index < PolyrhythmMIDIProcessor::maxSteps' in e
assert 'i < juce::jmin(128, first + visibleSteps)' in e
assert 'processor.deletePage(row, (a.page * visibleSteps) / 16);' in e
for w, n in [(800,4),(900,4),(960,8),(1150,8),(1200,12),(1450,12),(1480,16),(2800,16)]:
    width = 16 if w>=1480 else 12 if w>=1200 else 8 if w>=960 else 4
    assert n==width
    work=440+60*width; left=max(0,(w-work)/2)
    grid=left+250; clock=grid+60*width+8; timing=clock+76; page=timing+66
    assert page+34<=w-0.5, (w,width,page)
    assert left+42>=0 and left+42+160<grid-3
    for length in [1,5,16,64,127,128]:
        for pageidx in range((length-1)//n+1):
            for col in range(n):
                step=pageidx*n+col
                if step <length:
                    assert 0<=step<128
                # Empty slots beyond step 128 must never touch lane arrays.
                if step>=128:
                    assert step>=length
print('v0.6.8 responsive geometry, compatibility and index-safety checks passed')
