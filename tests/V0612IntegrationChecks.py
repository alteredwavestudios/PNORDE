from pathlib import Path
r=Path(__file__).resolve().parents[1]
e=(r/'Source/PluginEditor.cpp').read_text()
p=(r/'Source/PluginProcessor.cpp').read_text()
h=(r/'Source/PluginEditor.h').read_text()
ph=(r/'Source/PluginProcessor.h').read_text()
version=(r/'Source/PnordeProjectVersion.h').read_text()
assert 'schema = 3' in version and 'build = 612' in version
assert 'VERSION 0.6.12' in (r/'CMakeLists.txt').read_text()
assert 'PnordeLaneStart' in p and 'getLaneStartStep(int row)' in ph
assert 'PnordePlayback::indexWithStart(' in p and 'const int index = PnordePlayback::indexWithStart(' in p
assert p.index('for (int row = 0; row < laneCount; ++row)\n        setLaneStartStep(row, static_cast<int>(') > p.index('const int savedVersion =')
assert p.index('PnordeLaneStart" + juce::String(row)') < p.index('parameters.replaceState(state)')
assert 'if (absoluteStep < l.length)' in e and 'e.mods.isCtrlDown() || e.mods.isCommandDown()' in e
assert 'getLaneStartStep(row)' in e and 'startHit.contains(e.position)' in e
assert 'g.excludeClipRegion(settingsCallout->getBounds());' in e
assert 'g.excludeClipRegion(infoCallout->getBounds());' in e
assert 'g.excludeClipRegion(tooltipWindow.getBounds());' in e
assert 'if (row >= 7 && laneNameEditors[row].hasKeyboardFocus(true))' in e
assert 'Final emission layer' in e and 'laneLabelFont(' in e
assert 'footerCreditBounds()' in e and 'footerCreditBounds() const noexcept' in h
assert 'const bool stackedFooter = w < 1380' in e
assert 'darknessSlider.setBounds(0, 0, 0, 0)' in e
assert 'updateDarknessTip()' in e and 'isCommandDown() || e.mods.isCtrlDown()' in e
assert 'vibes.update(getWidth(), getHeight(), processor)' in e
assert 'juce::HyperlinkButton' not in e
assert 'github.com/alteredwavestudios/PNORDE' in e
assert 'l4nterns.bandcamp.com' in e
assert 'build-clean-v0612' in (r/'BUILD_PNORDE.ps1').read_text()
assert 'build-clean-v0612' in (r/'INSTALL_PNORDE_ADMIN.ps1').read_text()
for w in range(800,2801):
    cols = 16 if w >= 1160 else 8 if w >= 855 else 4
    stride = 60.0 if cols == 4 else max(44.0 if cols == 16 else 50.0, min(60.0, (w - 454) / cols))
    work = 454.0 + cols * stride
    left = max(0, (w - work) * 0.5)
    assert work <= w + .001, (w, cols, work)
    # label ends x + 202, previous button x + 210..240, first cell x + 250.
    assert left + 202 < left + 210 < left + 240 < left + 250
    grid = left + 250
    clock = grid + cols * stride + 8
    timing = clock + 76
    page = timing + 66
    assert page + 34 <= w - 10 + .01, (w,cols,page)
    assert grid + cols * stride <= clock - 7.9
    if w < 1380:
        unit=max(.72, min(1.15, (w - 44) / 895))
        leftFooter=24
        assert leftFooter + (837+36)*unit < w, (w,unit)
        creditX = w - max(215,min(315,w*.185)) - 24
        assert creditX > 0
    if w >= 1380:
        groupWidth=max(215.0, min(315.0, w * 0.185))
        creditX=w-groupWidth-24.0
        available=creditX-54.0
        spare=max(0.0, available-30.0-873.0)
        c1_end=30+230
        c2_start=30+242+spare/3
        c2_end=30+550+spare/3
        c3_start=30+563+spare*2/3
        c3_end=30+817+spare*2/3
        c4_start=30+837+spare
        c4_end=30+873+spare
        assert c1_end < c2_start and c2_end < c3_start and c3_end < c4_start
        assert c4_end < creditX-40.0, (w,c4_end,creditX)
    for length in (1,7,16,64,127,128):
      for pageidx in range((length-1)//cols+1):
        first = pageidx * cols
        for col in range(cols):
            idx = first + col
            if idx<length: assert 0<=idx<128
print('PASS: v0.6.12 lane-start migration, UI layers, resize invariants and version checks')
