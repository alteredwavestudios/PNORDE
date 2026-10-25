<##
.SYNOPSIS
    Install the just-tested VST3 to the normal Windows system VST3 folder.
.DESCRIPTION
    Administrator-only. Saves a verified backup to user's Documents, checks
    the SHA256 marker created by Build-And-Test.ps1 and attempts rollback.
    Users who prefer another VST3 directory can install manually instead.
##>
param([string]$BuildFolder = 'build')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$build = Join-Path $root $BuildFolder
$bundle = Join-Path $build 'PnordeDreamSequencer_artefacts\Release\VST3\PNORDE - Dream Sequencer.vst3'
$relativeBinary = 'Contents\x86_64-win\PNORDE - Dream Sequencer.vst3'
$binary = Join-Path $bundle $relativeBinary
$target = Join-Path $env:CommonProgramFiles 'VST3\PNORDE - Dream Sequencer.vst3'
$marker = Join-Path $build 'V0629_WINDOWS_TESTS_PASSED.txt'
$principal = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) { throw 'Please run this INSTALL script in an Administrator PowerShell window.' }
if (@(Get-Process | Where-Object { $_.ProcessName -match '^Ableton Live(\s|$)' }).Count -gt 0) { throw 'Close Ableton Live before installation.' }
if (-not (Test-Path -LiteralPath $binary -PathType Leaf)) { throw "Candidate plugin not found: $binary" }
if (-not (Test-Path -LiteralPath $marker -PathType Leaf)) { throw 'Run Build-And-Test.ps1 first.' }
$hash = (Get-FileHash -LiteralPath $binary -Algorithm SHA256).Hash
if ($hash -ne (Get-Content -LiteralPath $marker -Raw).Trim()) { throw 'Plugin has changed since testing; rebuild and retest.' }
$backupRoot = Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'PNORDE-Backups'
$backup = $null
if (Test-Path -LiteralPath $target) {
    New-Item -ItemType Directory -Path $backupRoot -Force | Out-Null
    $backup = Join-Path $backupRoot ('PNORDE_before_v0629_' + (Get-Date -Format yyyyMMdd_HHmmss) + '.vst3')
    Copy-Item -LiteralPath $target -Destination $backup -Recurse -Force
    if (-not (Test-Path -LiteralPath (Join-Path $backup $relativeBinary) -PathType Leaf)) { throw 'Backup missing VST3 binary. Cancelled.' }
    $oldHash = (Get-FileHash -LiteralPath (Join-Path $target $relativeBinary) -Algorithm SHA256).Hash
    if ((Get-FileHash -LiteralPath (Join-Path $backup $relativeBinary) -Algorithm SHA256).Hash -ne $oldHash) { throw 'Backup checksum mismatch. Cancelled.' }
    Write-Host "Backup: $backup" -ForegroundColor Green
}
try {
    if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Recurse -Force }
    Copy-Item -LiteralPath $bundle -Destination $target -Recurse -Force
    if ((Get-FileHash -LiteralPath (Join-Path $target $relativeBinary) -Algorithm SHA256).Hash -ne $hash) { throw 'Installed binary checksum mismatch' }
} catch {
    $errorText = $_
    if ($backup) {
        try {
            if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Recurse -Force }
            Copy-Item -LiteralPath $backup -Destination $target -Recurse -Force
            if ((Get-FileHash -LiteralPath (Join-Path $target $relativeBinary) -Algorithm SHA256).Hash -ne $oldHash) { throw 'Restored hash did not match backup' }
            throw "Install failed, old PNORDE restored: $errorText"
        } catch {
            Write-Warning "If needed, manually restore the backup located at: $backup"
            throw
        }
    }
    throw "First install failed; no previous installation to restore: $errorText"
}
Write-Host 'PNORDE v0.6.29 installed and verified.' -ForegroundColor Green
