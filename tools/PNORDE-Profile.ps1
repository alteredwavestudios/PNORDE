<#
Read-only PNORDE performance sampler and C++ source scan.
Run in a regular PowerShell window, with Ableton Live open.
No administrator permission or plugin changes required.
Example:
  .\PNORDE-Profile.ps1 -SourceDir 'H:\Music Library\PNORDE\dev\PnordeDreamSequencer' -Seconds 30
#>
param(
    [string]$SourceDir = (Get-Location).Path,
    [ValidateRange(5,600)][int]$Seconds = 60,
    [ValidatePattern('^[A-Za-z0-9_-]+$')][string]$Label = 'baseline',
    [string]$OutputDir = (Join-Path (Get-Location).Path 'PNORDE-Diagnostics')
)
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
$target = Get-Process | Where-Object { $_.ProcessName -match '(?i)Ableton|Live' } |
    Sort-Object CPU -Descending | Select-Object -First 1
if (-not $target) {
    Write-Warning 'Ableton Live not found. Open Ableton, then re-run this script.'
    return
}
$processId = $target.Id
Write-Host "Sampling process $($target.ProcessName), PID $processId, for $Seconds seconds..."
Write-Host 'For a useful comparison, run once with PNORDE UI open and again with it closed.'
$samples = New-Object 'System.Collections.Generic.List[object]'
$logicalCores = [Environment]::ProcessorCount
$prev = Get-Process -Id $processId
$prevCpu = [double]$prev.CPU
$prevTime = [DateTime]::UtcNow
for ($i = 0; $i -lt ($Seconds * 2); $i++) {
    Start-Sleep -Milliseconds 500
    $p = Get-Process -Id $processId -ErrorAction SilentlyContinue
    if (-not $p) { Write-Warning 'Process exited during sampling.'; break }
    $now = [DateTime]::UtcNow
    $elapsed = [Math]::Max(0.001, ($now - $prevTime).TotalSeconds)
    $cpuPercent = 100.0 * ([double]$p.CPU - $prevCpu) / ($elapsed * $logicalCores)
    $samples.Add([pscustomobject]@{
        Time = $now.ToString('o')
        CpuPctOfMachine = [Math]::Round([Math]::Max(0,$cpuPercent),2)
        WorkingSetMB = [Math]::Round($p.WorkingSet64 / 1MB,2)
        PrivateMemoryMB = [Math]::Round($p.PrivateMemorySize64 / 1MB,2)
        Threads = $p.Threads.Count
        Handles = $p.HandleCount
    })
    $prevCpu = [double]$p.CPU
    $prevTime = $now
}
# Run-specific filenames preserve the OPEN and CLOSED comparisons.
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$runId = "$Label-$stamp"
$csv = Join-Path $OutputDir "ableton-$runId.csv"
$samples | Export-Csv $csv -NoTypeInformation -Encoding UTF8
$report = Join-Path $OutputDir "report-$runId.txt"
$meanCpu = ($samples | Measure-Object CpuPctOfMachine -Average).Average
$peakCpu = ($samples | Measure-Object CpuPctOfMachine -Maximum).Maximum
$meanRam = ($samples | Measure-Object WorkingSetMB -Average).Average
$startRam = if ($samples.Count -gt 0) { $samples[0].PrivateMemoryMB } else { 0 }
$endRam = if ($samples.Count -gt 0) { $samples[$samples.Count-1].PrivateMemoryMB } else { 0 }
$ramChange = $endRam - $startRam
@(
    'PNORDE performance baseline (host process only; not a plugin-specific CPU meter)'
    "Process: $($target.ProcessName), PID $processId"
    "Sample duration: $Seconds seconds"
    "Test label: $Label"
    ('Mean host CPU: {0:N2} % of total machine' -f $meanCpu)
    ('Peak host CPU: {0:N2} % of total machine' -f $peakCpu)
    ('Mean host working set: {0:N1} MB' -f $meanRam)
    ('Host private memory change: {0:+0.0;-0.0;0.0} MB' -f $ramChange)
    "SourceDir: $SourceDir"
    'Compare equivalent playback loads with plugin editor OPEN vs CLOSED.'
) | Set-Content -Path $report -Encoding UTF8
$patterns = @(
    @{ label='Timers'; regex='startTimerHz\s*\(' },
    @{ label='Repainters'; regex='\brepaint\s*\(' },
    @{ label='Cached images'; regex='setBufferedToImage|setCachedComponentImage|juce::Image' },
    @{ label='MIDI limits'; regex='reserve\s*\(|ensureSize\s*\(|juce::MidiBuffer' },
    @{ label='Realtime hazards (review context)'; regex='new\s+|malloc\s*\(|copyState\s*\(|replaceState\s*\(|callAsync\s*\(|MessageManagerLock' }
)
# Restrict static source scan to Source; skip enormous generated build trees.
$sourceRoot = Join-Path $SourceDir 'Source'
$sourceFiles = if (Test-Path -LiteralPath $sourceRoot) {
    Get-ChildItem -LiteralPath $sourceRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Extension -in '.cpp','.h','.hpp' }
} else { @() }
$hits = New-Object 'System.Collections.Generic.List[object]'
foreach ($f in $sourceFiles) {
    foreach ($pattern in $patterns) {
        Select-String -LiteralPath $f.FullName -Pattern $pattern.regex -AllMatches -ErrorAction SilentlyContinue |
            ForEach-Object { $hits.Add([pscustomobject]@{ Category=$pattern.label; File=$_.Path; Line=$_.LineNumber; Text=$_.Line.Trim() }) }
    }
}
$hits | Export-Csv (Join-Path $OutputDir "source-review-$runId.csv") -NoTypeInformation -Encoding UTF8
Write-Host ('CPU mean/peak: {0:N2}% / {1:N2}%; private memory change: {2:+0.0;-0.0;0.0} MB' -f $meanCpu, $peakCpu, $ramChange)
Write-Host "Done. Report: $report"
Write-Host "Samples: $csv"
Write-Host "Source review: $(Join-Path $OutputDir ('source-review-' + $runId + '.csv'))"
Write-Host 'Note: This does not prove a leak. Look for sustained growth across longer, repeated tests.'
