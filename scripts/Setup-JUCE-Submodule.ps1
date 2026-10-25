<##
.SYNOPSIS
    Add the official JUCE repository as a PINNED Git submodule.
.DESCRIPTION
    Run ONCE in the local Git working tree before publishing PNORDE.
    Anyone can then clone with `git clone --recurse-submodules` to receive JUCE.
    Do not run on a folder where Get-JUCE.ps1 already cloned JUCE: this script
    deliberately refuses to delete files. Move that folder aside first.
##>
param([string]$Version = '9.0.4')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Push-Location $root
try {
    if (-not (Test-Path .git)) { throw 'Run git init (or clone an existing repository) first.' }
    if (Test-Path 'third_party\JUCE') { throw 'third_party/JUCE already exists. Back it up or remove it manually before adding a submodule.' }
    git submodule add --force https://github.com/juce-framework/JUCE.git third_party/JUCE
    if ($LASTEXITCODE -ne 0) { throw 'git submodule add failed' }
    git -C third_party/JUCE fetch origin tag $Version
    if ($LASTEXITCODE -ne 0) { throw 'Could not fetch pinned JUCE tag' }
    git -C third_party/JUCE checkout "tags/$Version"
    if ($LASTEXITCODE -ne 0) { throw 'Could not check out pinned JUCE tag' }
    git add .gitmodules third_party/JUCE
    if ($LASTEXITCODE -ne 0) { throw 'Could not stage JUCE submodule' }
    Write-Host "Git submodule pinned to JUCE $Version. Commit .gitmodules and the gitlink." -ForegroundColor Green
} finally { Pop-Location }

