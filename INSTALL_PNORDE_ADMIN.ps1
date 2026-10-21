# PNORDE v0.6.21 isolated installer. Close Ableton Live before running.
# Requires Windows PowerShell as Administrator and successful BUILD_PNORDE.ps1.
$ErrorActionPreference = 'Stop'
$build = 'H:\PNORDE-build-v0621'
$bundle = 'PNORDE - Dream Sequencer.vst3'
$relativeBinary = 'Contents\x86_64-win\PNORDE - Dream Sequencer.vst3'
$relativeMetadata = 'Contents\Resources\moduleinfo.json'
$source = Join-Path $build ('PnordeDreamSequencer_artefacts\Release\VST3\' + $bundle)
$installed = Join-Path 'C:\Program Files\Common Files\VST3' $bundle
$backupRoot = 'H:\Music Library\PNORDE\dev\PnordeDreamSequencer\VST3-backups'

$admin = [Security.Principal.WindowsPrincipal]::new(
    [Security.Principal.WindowsIdentity]::GetCurrent()
).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $admin) { throw 'Open PowerShell as Administrator before running the installer.' }
if (Get-Process | Where-Object { ($_.ProcessName -match 'Ableton|REAPER|Cubase|Bitwig|Studio One|FL Studio') -and ($_.ProcessName -ne 'AbletonAudioCpl') }) {
    throw 'Close Ableton and all DAWs/plugin hosts first; then retry.'
}
$sourceBinary = Join-Path $source $relativeBinary
$sourceMetadata = Join-Path $source $relativeMetadata
if (-not (Test-Path -LiteralPath $sourceBinary -PathType Leaf) -or
    -not (Test-Path -LiteralPath $sourceMetadata -PathType Leaf)) {
    throw 'No complete Release v0.6.21 VST3 build found. Installation CANCELLED.'
}
$versionData = Get-Content -LiteralPath $sourceMetadata -Raw
if ($versionData -notmatch '"Version"\s*:\s*"0\.6\.21"') {
    throw 'Release bundle does not identify as v0.6.21. Installation CANCELLED.'
}
$sourceHash = (Get-FileHash -LiteralPath $sourceBinary -Algorithm SHA256).Hash
$stageRoot = Join-Path $build ('PNORDE-stage-' + [guid]::NewGuid().ToString('N'))
$stageBundle = Join-Path $stageRoot $bundle
$parked = $null
$backup = $null
$installedNew = $false
try {
    New-Item -ItemType Directory -Path $stageRoot -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $stageBundle -Recurse -ErrorAction Stop
    if ((Get-FileHash -LiteralPath (Join-Path $stageBundle $relativeBinary) -Algorithm SHA256).Hash -ne $sourceHash) {
        throw 'Staged VST3 hash mismatch. Installation CANCELLED.'
    }
    New-Item -ItemType Directory -Path $backupRoot -Force | Out-Null
    if (Test-Path -LiteralPath $installed) {
        $backup = Join-Path $backupRoot ('PNORDE-before-v0621-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '.vst3')
        Copy-Item -LiteralPath $installed -Destination $backup -Recurse -ErrorAction Stop
        $installedHash = (Get-FileHash -LiteralPath (Join-Path $installed $relativeBinary) -Algorithm SHA256).Hash
        $backupHash = (Get-FileHash -LiteralPath (Join-Path $backup $relativeBinary) -Algorithm SHA256).Hash
        if ($installedHash -ne $backupHash) { throw 'Backup verification failed; installed plugin untouched.' }
        Write-Host "Verified backup: $backup" -ForegroundColor Cyan
        # Rename within the same drive; prevents Windows copying through locked files.
        # A failure here leaves existing v0.6.20 untouched.
        $parked = Join-Path (Split-Path $installed) ('PNORDE-PREVIOUS-' + [guid]::NewGuid().ToString('N') + '.vst3-disabled')
        Rename-Item -LiteralPath $installed -NewName (Split-Path $parked -Leaf) -ErrorAction Stop
    }
    Copy-Item -LiteralPath $stageBundle -Destination $installed -Recurse -ErrorAction Stop
    $installedNew = $true
    $verifiedHash = (Get-FileHash -LiteralPath (Join-Path $installed $relativeBinary) -Algorithm SHA256).Hash
    if ($verifiedHash -ne $sourceHash) { throw 'Installed VST3 hash mismatch.' }
    $installedMeta = Get-Content -LiteralPath (Join-Path $installed $relativeMetadata) -Raw
    if ($installedMeta -notmatch '"Version"\s*:\s*"0\.6\.21"') { throw 'Installed VST3 metadata mismatch.' }
    Write-Host ''
    Write-Host 'PNORDE v0.6.21 INSTALLED AND VERIFIED!' -ForegroundColor Green
    Write-Host "SHA256: $verifiedHash"
    Write-Host "Backup: $backup"
    Write-Host "Parked previous plugin: $parked"
}
catch {
    Write-Host "INSTALLATION STOPPED: $($_.Exception.Message)" -ForegroundColor Red
    if ($parked -and (Test-Path -LiteralPath $parked)) {
        try {
            if (Test-Path -LiteralPath $installed) {
                Remove-Item -LiteralPath $installed -Recurse -Force -ErrorAction Stop
            }
            Rename-Item -LiteralPath $parked -NewName $bundle -ErrorAction Stop
            Write-Host 'Previous PNORDE restored.' -ForegroundColor Yellow
        } catch {
            Write-Host "Could not automatically restore. Keep backup: $backup" -ForegroundColor Red
            Write-Host "Previous version parked at: $parked" -ForegroundColor Red
        }
    }
    throw
}
finally {
    if (Test-Path -LiteralPath $stageRoot) {
        Remove-Item -LiteralPath $stageRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}

