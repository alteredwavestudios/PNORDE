<##
.SYNOPSIS
    Replace the README template with your actual GitHub repository link.
.EXAMPLE
    .\scripts\Set-Release-Link.ps1 -Repository 'your-name/PNORDE'
##>
param([Parameter(Mandatory=$true)][string]$Repository)
$ErrorActionPreference = 'Stop'
if ($Repository -notmatch '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$') { throw 'Expected owner/repository, e.g. myname/PNORDE' }
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$readme = Join-Path $root 'README.md'
$text = Get-Content -LiteralPath $readme -Raw
$text = $text.Replace('https://github.com/YOUR_USERNAME/YOUR_REPOSITORY/releases/latest/download/', "https://github.com/$Repository/releases/latest/download/")
Set-Content -LiteralPath $readme -Value $text -Encoding utf8
Write-Host 'README release link updated. Create/upload the release asset before clicking it.' -ForegroundColor Green
