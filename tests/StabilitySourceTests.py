from pathlib import Path
p = Path(__file__).resolve().parents[1]
processor = (p/'Source/PluginProcessor.cpp').read_text()
editor = (p/'Source/PluginEditor.cpp').read_text()
head = (p/'Source/PluginEditor.h').read_text()
profile = (p/'tools/PNORDE-Profile.ps1').read_text()
assert 'if (wasEmpty) notify();' in processor
assert 'if (r == write.load(std::memory_order_acquire)) { wait(50); continue; }' in processor
assert 'std::atomic<bool> outputReady' in processor
assert 'return outputReady.load(std::memory_order_acquire);' in processor
assert 'if (guard >= iterationBudget)' in processor
assert 'noteOff(numSamples - 1);' in processor
assert 'std::array<bool, lanes> rowDirty {}' in editor
assert 'if (!rowDirty[static_cast<size_t>(row)] || !rowVisible(row)) continue;' in editor
assert 'frameIntervalMs = 1000.0 / requestedFps' in editor and 'vibes.update(getWidth(), getHeight(), processor)' in editor
assert 'double nextVibesFrameMs = 0.0;' in head
assert 'behindWeights[behind] * phaseAttenuation' in editor
assert 'if (fullRepaint || !playing || !animate)' not in editor
assert 'sourceRoot' in profile and "[ValidatePattern('^[A-Za-z0-9_-]+$')]" in profile
assert 'build-clean-v069' in (p/'BUILD_PNORDE.ps1').read_text()
assert 'build-clean-v069' in (p/'INSTALL_PNORDE_ADMIN.ps1').read_text()
print('v0.6.3 targeted source stability checks passed')
