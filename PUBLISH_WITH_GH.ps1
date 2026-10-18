# Run only AFTER completing RELEASE_CHECKLIST.md and confirming you want the repo public.
$ErrorActionPreference = 'Stop'
$repo = 'PNORDE'   # Change to your desired repository name
$tag = 'v0.5.2-alpha.1'
$binary = Join-Path (Split-Path -Parent $PSScriptRoot) 'PNORDE-v0.5.2-alpha.1-Windows-x64-VST3.zip'
if (-not (Get-Command gh -ErrorAction SilentlyContinue)) { throw 'Install GitHub CLI from https://cli.github.com/' }
if (-not (Test-Path $binary)) { throw "Release ZIP not found: $binary" }
gh auth status
if ($LASTEXITCODE -ne 0) { throw 'Run gh auth login first' }
Set-Location $PSScriptRoot
git init
git add .
git commit -m 'Initial PNORDE public alpha source'
if ($LASTEXITCODE -ne 0) { throw 'Commit failed' }
git branch -M main
gh repo create $repo --public --source . --remote origin --push
if ($LASTEXITCODE -ne 0) { throw 'Repository creation failed' }
gh release create $tag $binary --title 'PNORDE v0.5.2 Alpha 1' --notes-file RELEASE_NOTES.md --prerelease
if ($LASTEXITCODE -ne 0) { throw 'Release creation failed' }
Write-Host 'GitHub repository and prerelease created.' -ForegroundColor Green
