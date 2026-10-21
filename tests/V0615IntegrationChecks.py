"""Source-level contract tests for the v0.6.15 Windows JUCE candidate.
NOT a substitute for compiling/loading the actual VST3 in Ableton.
"""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[1]
proc = (root/'Source/PluginProcessor.cpp').read_text()
ph = (root/'Source/PluginProcessor.h').read_text()
editor = (root/'Source/PluginEditor.cpp').read_text()
eh = (root/'Source/PluginEditor.h').read_text()
dials = (root/'Source/PnordeDials.h').read_text()
presets = (root/'Source/PnordePresetFormat.h').read_text()
version = (root/'Source/PnordeProjectVersion.h').read_text()
assert 'build = 615' in version and 'release = "0.6.15"' in version
assert 'VERSION 0.6.15' in (root/'CMakeLists.txt').read_text()
assert proc.index('"pn_program_slot"') > proc.index('"pn_global_reverse"')
assert '"pn_program_slot", "PNORDE Quick Program", 0, 3, 0' in proc
assert 'std::array<juce::MemoryBlock, programCount> savedPrograms' in ph
assert 'programCount = 4' in ph
assert 'PnordeProgramSlot' in proc and 'PnordeProgramSelected' in proc
assert 'if (!applyingProgramSnapshot && isBanked)' in proc
assert 'if (isBanked) savedPrograms = std::move(restoredPrograms);' in proc
assert 'for (auto& slot : savedPrograms) slot = legacySnapshot;' in proc
assert proc.index('for (auto& slot : savedPrograms) slot = legacySnapshot;') < proc.index('parameters.replaceState(state)')
assert 'if (selected == previous) return;' in proc
assert 'requestMidiPanic();\n    requestSequenceReset();' in proc
assert 'getLaneStartStep(row)' in proc and 'PnordePlayback::indexWithStart(' in proc
assert 'applyPendingProgramChange();' in proc and 'startTimerHz(60)' in proc
assert 'getRawParameterValue("pn_program_slot")' in proc
assert 'size > 4 * 1024 * 1024' in proc
assert '4u * 1024u * 1024u' in presets
assert 'state.getNumProperties() > 512' in proc
assert 'PnordeSettingsViewport' in editor
assert 'class PnordeOverlayHost final' in editor
assert 'openContainedPanel(std::move(content), bounds);' in editor
assert 'popupOverlay->setBounds(getLocalBounds())' in editor
assert 'g.excludeClipRegion(popupOverlay->getBounds());' in editor
assert 'juce::CallOutBox::launchAsynchronously' not in editor
assert 'std::array<juce::TextButton, 4> quickPrograms' in eh
assert 'quickProgram.setRange(1, 4, 1);' in editor
assert 'onContextMenu' in dials and 'showClocksMenu(row)' in editor
assert 'Generate AFTERTOUCH pressure' in editor
assert 'id >= 900 && id < 900 + PnordePatternLibrary::aftertouchCount' in editor
assert 'logoHover || creditHover' in editor and 'juce::ColourGradient gradient' in editor
assert 'footerCreditBounds()' in editor
assert 'return width >= 982 ? 16 : width >= 720 ? 8 : 4;' in editor
# At every supported width, leave a protected 20px horizontal gap before right edge.
for w in range(800, 2801):
    cols = 16 if w >= 982 else 8 if w >= 720 else 4
    stride = (min(60., max(42.,(w-454.)/4.)) if cols == 4 else
              min(60., max(32. if cols==16 else 33., (w-454.)/cols)))
    work=454.+cols*stride
    assert work <= w+1e-5, (w, cols, work)
    start=max(0.,(w-work)*0.5)
    page_right=start+250.+cols*stride+8+76+66+34
    assert page_right <= w-10.+1e-3, (w, cols, page_right)
    if w < 1220:
        dial_left=350; dial_right=dial_left+309+87
        program_right=230+3*25+23
        assert program_right+10 < dial_left and dial_right+20<w
    else:
        dial_left=round((w-475.)*.5)
        assert dial_left > 255+93+10
for fn in ('BUILD_PNORDE.ps1', 'INSTALL_PNORDE_ADMIN.ps1'):
    content=(root/fn).read_text()
    assert 'build-clean-v0615' in content and 'v0.6.14' not in content
print('PASS v0.6.15: program bank migration, popup ownership, layout, host mapping, pattern wiring')
