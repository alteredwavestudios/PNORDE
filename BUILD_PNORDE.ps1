$ErrorActionPreference = 'Stop'
$project = $PSScriptRoot
$build = Join-Path $project 'build-clean-v059'
Write-Host 'PNORDE v0.5.9: configuring a separate CLEAN Visual Studio 2026 build...' -ForegroundColor Cyan
if (Test-Path -LiteralPath $build) {
    Remove-Item -LiteralPath $build -Recurse -Force -ErrorAction Stop
}
& cmake -S "$project" -B "$build" -G 'Visual Studio 18 2026' -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed. Do NOT install.' }
& cmake --build "$build" --config Release --target PnordeDreamSequencer_VST3 --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed. Do NOT install.' }
$bundle = Join-Path $build 'PnordeDreamSequencer_artefacts\Release\VST3\PNORDE - Dream Sequencer.vst3'
$binary = Join-Path $bundle 'Contents\x86_64-win\PNORDE - Dream Sequencer.vst3'
if (-not (Test-Path -LiteralPath $binary -PathType Leaf)) {
    throw "VST3 built binary not found: $binary"
}
Write-Host 'PNORDE v0.5.9 CLEAN BUILD SUCCESSFUL! :3' -ForegroundColor Green
Write-Host 'Close Ableton and run INSTALL_PNORDE_ADMIN.ps1 as Administrator.'
