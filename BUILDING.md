# Building PNORDE from source

## Windows 11 / Visual Studio 2026

Install CMake 3.22+, Visual Studio 2026 (Desktop development with C++), and JUCE 8 from its official upstream project. JUCE is not included in this repository.

From PowerShell in the repository root:

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DJUCE_DIR="C:/your/path/to/JUCE"
if ($LASTEXITCODE -ne 0) { throw "Configure failed" }
cmake --build build --config Release --target PnordeDreamSequencer_VST3 --parallel 4
if ($LASTEXITCODE -ne 0) { throw "Build failed" }
```

The plugin bundle will be under `build/PnordeDreamSequencer_artefacts/Release/VST3/`.

This project's CMakeLists.txt includes an older local JUCE-location fallback for the original developer's system. Public contributors should explicitly pass `-DJUCE_DIR=...`.

Keep the installed PNORDE VST3 separate from any legacy `Polyrhythm MIDI.vst3` plugin.
