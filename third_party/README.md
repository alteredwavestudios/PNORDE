# JUCE dependency (source arrives here)

This folder is where **JUCE** belongs. It is not included in the tiny source ZIP.
To download and keep JUCE alongside PNORDE locally, run:

```powershell
.\scripts\Get-JUCE.ps1
```

For GitHub, the recommended way to **include JUCE in the repository** is a
pinned **Git submodule**, created with `scripts/Setup-JUCE-Submodule.ps1`.
Users cloning the repository with `--recurse-submodules` get JUCE too.
The official JUCE sources remain unmodified and under their own licence.
See [JUCE licensing](https://github.com/juce-framework/JUCE/blob/master/LICENSE.md).
