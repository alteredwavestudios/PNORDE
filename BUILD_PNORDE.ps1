$ErrorActionPreference = 'Stop'
$project = $PSScriptRoot
$build = 'H:\PNORDE-build-v0621'
if (-not (Test-Path -LiteralPath (Join-Path $project 'CMakeLists.txt') -PathType Leaf)) {
    throw 'PNORDE v0.6.21 CMakeLists.txt was not found.'
}
Write-Host 'PNORDE v0.6.21: configuring isolated Visual Studio build...' -ForegroundColor Cyan
& cmake -S $project -B $build -G 'Visual Studio 18 2026' -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed. DO NOT INSTALL.' }
& cmake --build $build --config Release --target PnordeDreamSequencer_VST3 --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed. DO NOT INSTALL.' }
$binary = Join-Path $build 'PnordeDreamSequencer_artefacts\Release\VST3\PNORDE - Dream Sequencer.vst3\Contents\x86_64-win\PNORDE - Dream Sequencer.vst3'
if (-not (Test-Path -LiteralPath $binary -PathType Leaf)) {
    throw 'No Release VST3 binary produced. DO NOT INSTALL.'
}
Write-Host 'PNORDE v0.6.21 BUILD SUCCESSFUL!' -ForegroundColor Green
Write-Host 'Verify Release files, then close Ableton and run INSTALL_PNORDE_ADMIN.ps1 in Administrator PowerShell.'
