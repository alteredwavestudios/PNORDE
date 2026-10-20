$ErrorActionPreference = 'Stop'
$project = $PSScriptRoot
$source = Join-Path $project 'build-clean-v059\PnordeDreamSequencer_artefacts\Release\VST3\PNORDE - Dream Sequencer.vst3'
$destination = 'C:\Program Files\Common Files\VST3\PNORDE - Dream Sequencer.vst3'
$staging = "$destination.pending"
$binary = 'Contents\x86_64-win\PNORDE - Dream Sequencer.vst3'
$principal = [Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Start PowerShell as Administrator before installing.'
}
if (Get-Process -Name 'Ableton Live*' -ErrorAction SilentlyContinue) {
    throw 'Close Ableton Live before installing.'
}
if (-not (Test-Path -LiteralPath (Join-Path $source $binary) -PathType Leaf)) {
    throw "Fresh v0.5.9 binary not found; do NOT install: $source"
}
# Staging and SHA256 verification are performed BEFORE replacing the current plugin.
if (Test-Path -LiteralPath $staging) { Remove-Item -LiteralPath $staging -Recurse -Force }
Copy-Item -LiteralPath $source -Destination $staging -Recurse -Force -ErrorAction Stop
if (-not (Test-Path -LiteralPath (Join-Path $staging $binary) -PathType Leaf) -or
    (Get-FileHash -LiteralPath (Join-Path $staging $binary) -Algorithm SHA256).Hash -ne
    (Get-FileHash -LiteralPath (Join-Path $source $binary) -Algorithm SHA256).Hash) {
    Remove-Item -LiteralPath $staging -Recurse -Force
    throw 'Staged VST3 failed binary verification; installed PNORDE has not changed.'
}
$backup = $null
$previous = $null
if (Test-Path -LiteralPath $destination) {
    $backup = Join-Path $project ("Backups\PNORDE_Pre059_" + (Get-Date -Format 'yyyyMMdd_HHmmss'))
    New-Item -ItemType Directory -Path $backup -Force | Out-Null
    Copy-Item -LiteralPath $destination -Destination $backup -Recurse -Force -ErrorAction Stop
    $previous = Join-Path $backup 'PNORDE - Dream Sequencer.vst3'
    Write-Host "Existing PNORDE backed up to $backup" -ForegroundColor Cyan
}
try {
    if (Test-Path -LiteralPath $destination) {
        Remove-Item -LiteralPath $destination -Recurse -Force -ErrorAction Stop
    }
    Move-Item -LiteralPath $staging -Destination $destination -ErrorAction Stop
}
catch {
    Write-Warning 'Installation failed. Attempting to restore the previous VST3.'
    if (-not (Test-Path -LiteralPath $destination) -and $previous -and
        (Test-Path -LiteralPath $previous)) {
        Copy-Item -LiteralPath $previous -Destination $destination -Recurse -Force
    }
    throw
}
Write-Host 'PNORDE v0.5.9 INSTALLED! :3' -ForegroundColor Green
Write-Host 'Original Polyrhythm MIDI.vst3 was NOT touched.'
Write-Host 'Open Ableton and hold Alt while clicking Rescan under Plug-Ins.'
