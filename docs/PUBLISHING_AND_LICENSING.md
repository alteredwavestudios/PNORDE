# Publishing PNORDE

## Source licence: GNU AGPLv3

PNORDE's original application source code is released under
GNU Affero General Public License version 3 (AGPL-3.0-only).

The complete licence is available in the repository's LICENSE file.

JUCE retains its own licensing terms, and the embedded Audiowide font
retains its SIL Open Font License.

Check third-party licence compliance before distributing binaries.
## Audiowide embedded font

The original source already embeds Audiowide font bytes in
`Source/AudiowideEmbedded.h`. The font is published by Google Fonts under the
SIL Open Font License. Before public redistribution, check the exact embedded
font version and make sure the appropriate licence/copyright notice accompanies
it. The upstream notice and FONTLOG are linked in `THIRD_PARTY_NOTICES.md`.
No separate editable/downloadable font file is shipped in this package.

## Initial commit, pin JUCE and push

Run in a folder where PNORDE source is located, using Windows PowerShell:

```powershell
# Launch PowerShell from the folder holding CMakeLists.txt.
git init
.\scripts\Setup-JUCE-Submodule.ps1  # before Get-JUCE, requires empty third_party/JUCE
.\scripts\Set-Release-Link.ps1 -Repository 'YOUR_USERNAME/YOUR_REPOSITORY'
git add .
git status
# Inspect changed files and make sure build/, dist/, backup files aren't tracked.
git commit -m 'Publish PNORDE v0.6.29 with documented source and portable build'
# For an existing repository, set/check its existing remote first.
git remote -v
# Push to the intended branch/remote once the destination is confirmed.
```

For an **existing repository**, it's often easier to clone it first, then copy
this package's `Source`, `tests`, `scripts`, `docs`, README and CMakeLists into
that clone before making a commit. Do not run `git init` over a different
project or change remotes until you have checked where they point.

After pushing, anyone can use:

```powershell
git clone --recurse-submodules https://github.com/OWNER/REPO.git
```

The **green Code â†’ Download ZIP** option may not include JUCE submodule data;
ZIP users should run `scripts/Get-JUCE.ps1` after extraction.

## Build and publish the download

```powershell
.\scripts\Build-And-Test.ps1
.\scripts\Get-Audiowide-License.ps1
.\scripts\Make-Release.ps1
# Output: dist\PNORDE-Windows-VST3.zip
```

Open the GitHub repository â†’ **Releases** â†’ **Draft a new release**, tag
`v0.6.29`, select `Set as latest`, and upload that ZIP. The top-of-README
link then becomes a one-click download after its URL has been configured.

Alternatively, with GitHub CLI authenticated to the correct repository:

```powershell
gh release create v0.6.29 '.\dist\PNORDE-Windows-VST3.zip' --title 'PNORDE v0.6.29' --notes-file '.\RELEASE_NOTES.md' --latest
```

## Before publishing

- Confirm you have rights to publish your own code and the embedded font.
- Run `.\scripts\Get-Audiowide-License.ps1` and include its notice with source and release assets.
- Choose app licence; check compatibility with JUCE licensing.
- Run Windows build/regressions against the chosen JUCE checkout.
- Check that the README direct-download link has the correct repository name.
- Release asset must contain a real built `.vst3` folder (never source alone).
- Do not publish crash dumps, machine-specific paths or private presets.
- Start all initial audio testing at a low monitor volume.

