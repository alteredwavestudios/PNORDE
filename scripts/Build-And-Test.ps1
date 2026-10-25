<##
.SYNOPSIS
    Build the PNORDE v0.6.29 VST3 and its existing regression executables.
.DESCRIPTION
    Run from anywhere; $PSScriptRoot locates this source tree, not a drive letter.
    Requires Windows, CMake, VS C++ build tools, and JUCE source.
    DOES NOT install the VST3 to Program Files.
##>
param(
    [string]$BuildFolder = 'build',
    [string]$JucePath = ''
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$build = Join-Path $root $BuildFolder
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { throw 'CMake is missing from PATH.' }
$flags = @('-DPNORDE_BUILD_PROGRAM_STATE_TESTS=ON', '-DPNORDE_BUILD_NUMERICAL_TESTS=ON',
    '-DPNORDE_BUILD_MUSICAL_TESTS=ON', '-DPNORDE_BUILD_UI_TESTS=ON',
    '-DPNORDE_BUILD_CC_MAPPING_TESTS=ON', '-DPNORDE_BUILD_POLLING_TESTS=ON',
    '-DPNORDE_BUILD_DEVELOPER_TESTS=ON', '-DPNORDE_BUILD_V0628_TESTS=ON',
    '-DPNORDE_BUILD_V0629_TESTS=ON')
if ($JucePath) { $flags += "-DPNORDE_JUCE_DIR=$JucePath" }
Write-Host '[1/3] Configure' -ForegroundColor Cyan
& cmake -S $root -B $build -A x64 @flags
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }
$targets = @('PnordeDreamSequencer_VST3', 'PnordeProgramStateTests',
    'PnordeStage2SafetyTests', 'PnordeStage3MusicalTests',
    'PnordeStage4UITests', 'PnordeStage5CCMappingTests',
    'PnordeStage56PollingTests', 'PnordeDeveloperToolsTests',
    'PnordeV0628ProfileTests', 'PnordeV0629BlackBoxTests')
Write-Host '[2/3] Build' -ForegroundColor Cyan
& cmake --build $build --config Release --target @targets --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
Write-Host '[3/3] Run tests' -ForegroundColor Cyan
foreach ($name in ($targets | Where-Object { $_ -ne 'PnordeDreamSequencer_VST3' })) {
    $found = @(Get-ChildItem -LiteralPath $build -Recurse -File -Filter "$name.exe" |
        Where-Object { $_.FullName -like '*\Release\*' })
    if ($found.Count -ne 1) { throw "Could not uniquely locate $name.exe" }
    & $found[0].FullName
    if ($LASTEXITCODE -ne 0) { throw "TEST FAILED: $name" }
    Write-Host "PASS: $name" -ForegroundColor Green
}
$binary = Join-Path $build 'PnordeDreamSequencer_artefacts\Release\VST3\PNORDE - Dream Sequencer.vst3\Contents\x86_64-win\PNORDE - Dream Sequencer.vst3'
if (-not (Test-Path -LiteralPath $binary -PathType Leaf)) { throw "Built VST3 is missing: $binary" }
$hash = (Get-FileHash -LiteralPath $binary -Algorithm SHA256).Hash
$marker = Join-Path $build 'V0629_WINDOWS_TESTS_PASSED.txt'
Set-Content -LiteralPath $marker -Encoding ascii -Value $hash
Write-Host "PNORDE v0.6.29 WINDOWS TESTS PASSED! SHA256=$hash" -ForegroundColor Green
Write-Host 'Plugin NOT installed; use Install-Local.ps1 separately.' -ForegroundColor Yellow
