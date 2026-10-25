<##
.SYNOPSIS
    Download the original Audiowide font's full SIL OFL licence notice.
.DESCRIPTION
    PNORDE already embeds font byte data. Redistribution requires the correct
    notice/licence. Download the upstream notice before public release.
##>
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$dir = Join-Path $root 'third_party_licenses'
New-Item -ItemType Directory -Force -Path $dir | Out-Null
$target = Join-Path $dir 'Audiowide-OFL.txt'
$url = 'https://raw.githubusercontent.com/google/fonts/main/ofl/audiowide/OFL.txt'
Invoke-WebRequest -Uri $url -OutFile $target
$content = Get-Content -LiteralPath $target -Raw
if ($content -notmatch 'SIL OPEN FONT LICENSE' -or $content -notmatch 'Copyright') {
    Remove-Item -LiteralPath $target -Force
    throw 'Downloaded licence notice did not look valid. Cancelled.'
}
Write-Host "Audiowide licence saved: $target" -ForegroundColor Green
Write-Host 'Compare your embedded font against the upstream version before publishing.' -ForegroundColor Yellow
