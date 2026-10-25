<##
.SYNOPSIS
    Fetch the public JUCE sources into this project's third_party/JUCE folder.
.DESCRIPTION
    Works wherever PNORDE has been unzipped or cloned. Requires Git.
    Downloads a tagged upstream version, not a moving 'master' branch.
    The JUCE licence is independent: consult third_party/JUCE/LICENSE.md.
    A repo WITH a JUCE submodule should instead run `git submodule update --init`.
##>
param([string]$Version = '9.0.4')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$destination = Join-Path $root 'third_party\JUCE'
if (Test-Path -LiteralPath (Join-Path $destination 'modules\juce_core')) {
    Write-Host "JUCE is already present: $destination" -ForegroundColor Green
    exit 0
}
if (-not (Get-Command git -ErrorAction SilentlyContinue)) { throw 'Git not found. Install Git first.' }
# Prefer the exact dependency recorded by an existing GitHub submodule.
if (Test-Path -LiteralPath (Join-Path $root '.gitmodules')) {
    Push-Location $root
    try { git submodule update --init --recursive; if ($LASTEXITCODE -ne 0) { throw 'Submodule fetch failed' } }
    finally { Pop-Location }
} else {
    if (Test-Path -LiteralPath $destination) {
        throw "Folder exists but lacks JUCE modules: $destination. Inspect it rather than overwriting it."
    }
    git clone --depth 1 --branch $Version https://github.com/juce-framework/JUCE.git $destination
    if ($LASTEXITCODE -ne 0) { throw 'JUCE download failed' }
}
if (-not (Test-Path -LiteralPath (Join-Path $destination 'modules\juce_core'))) {
    throw 'Downloaded folder is not a valid JUCE source tree.'
}
Write-Host "JUCE ready at $destination. Check JUCE licensing before distributing binaries." -ForegroundColor Green
