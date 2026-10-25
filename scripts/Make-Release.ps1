<##
.SYNOPSIS
    Bundle the compiled Windows VST3 folder into one GitHub Release ZIP.
.DESCRIPTION
    Run AFTER Build-And-Test.ps1; refuses stale or changed binaries.
    Upload the generated ZIP to GitHub Releases, not Git source history.
##>
param([string]$BuildFolder = 'build')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$build = Join-Path $root $BuildFolder
$bundle = Join-Path $build 'PnordeDreamSequencer_artefacts\Release\VST3\PNORDE - Dream Sequencer.vst3'
$binary = Join-Path $bundle 'Contents\x86_64-win\PNORDE - Dream Sequencer.vst3'
$marker = Join-Path $build 'V0629_WINDOWS_TESTS_PASSED.txt'
if (-not (Test-Path -LiteralPath $binary -PathType Leaf)) { throw 'Build the VST3 first.' }
if (-not (Test-Path -LiteralPath $marker -PathType Leaf)) { throw 'Run Build-And-Test.ps1 first.' }
$hash = (Get-FileHash -LiteralPath $binary -Algorithm SHA256).Hash
if ($hash -ne (Get-Content -LiteralPath $marker -Raw).Trim()) { throw 'Binary was changed since regression tests. Retest.' }
$dist = Join-Path $root 'dist'
New-Item -ItemType Directory -Path $dist -Force | Out-Null
$asset = Join-Path $dist 'PNORDE-Windows-VST3.zip'
$instructions = Join-Path $dist 'INSTALL_WINDOWS.txt'
@'
PNORDE - Dream Sequencer v0.6.29 (Windows x64 VST3)

1. Quit Ableton and other DAWs.
2. Extract the ZIP.
3. Copy the complete "PNORDE - Dream Sequencer.vst3" folder to
   C:\Program Files\Common Files\VST3\
   (Windows may request Administrator permission.)
4. Restart Ableton. Rescan VST3 plugins if necessary.

Keep the .vst3 FOLDER intact, not just its inner binary.
Use a low monitoring volume for first testing.
See README.md in the source repository for build instructions and third-party notices.
'@ | Set-Content -LiteralPath $instructions -Encoding utf8
if (Test-Path -LiteralPath $asset) { Remove-Item -LiteralPath $asset -Force }
# Include third-party notices with redistributable binaries, not just source.
$notice = Join-Path $root 'THIRD_PARTY_NOTICES.md'
$fontLicense = Join-Path $root 'third_party_licenses\Audiowide-OFL.txt'
if (-not (Test-Path -LiteralPath $fontLicense -PathType Leaf)) {
    throw 'Missing Audiowide licence. Run scripts/Get-Audiowide-License.ps1 first.'
}
$juceLicense = Join-Path $root 'third_party\JUCE\LICENSE.md'
if (-not (Test-Path -LiteralPath $juceLicense -PathType Leaf)) {
    throw 'Missing JUCE licence. Run Get-JUCE.ps1 or initialize the JUCE submodule.'
}
$juceNoticeCopy = Join-Path $dist 'JUCE-LICENSE.md'
Copy-Item -LiteralPath $juceLicense -Destination $juceNoticeCopy -Force
$assets = @($bundle, $instructions, $notice, $fontLicense, $juceNoticeCopy)
$appLicense = Join-Path $root 'LICENSE'
if (Test-Path -LiteralPath $appLicense -PathType Leaf) {
    $assets += $appLicense
} else {
    Write-Warning 'No PNORDE LICENSE file chosen yet. Resolve licensing before a public binary release.'
}
Compress-Archive -LiteralPath $assets -DestinationPath $asset -CompressionLevel Optimal
Write-Host "Release asset ready: $asset" -ForegroundColor Green
Write-Host "Plugin SHA256: $hash" -ForegroundColor Green
Write-Host 'Upload this ZIP to the v0.6.29 GitHub Release.' -ForegroundColor Cyan
