# PNORDE â€” Dream Sequencer ðŸŽ¹ðŸŒ€

**Experimental polyrhythmic MIDI sequencer for Ableton Live and other compatible VST3 hosts.**

**Current Windows version:** **v0.6.29** Â· **application build 629**  
**Saved-project compatibility marker:** **625** (unchanged deliberately)

## â¬‡ Download PNORDE for Windows

**[Download the latest compiled Windows VST3](https://github.com/alteredwavestudios/PNORDE/releases/latest/download/PNORDE-Windows-VST3.zip)**

This direct download becomes active when a matching ZIP asset has been uploaded
under **GitHub â†’ Releases**. To set your own repo URL, run:

```powershell
.\scripts\Set-Release-Link.ps1 -Repository 'YOUR_USERNAME/YOUR_REPOSITORY'
```

The ZIP contains the complete `.vst3` plugin folder. Close your DAW, unzip it,
and copy **the entire `PNORDE - Dream Sequencer.vst3` directory** into:

```text
C:\Program Files\Common Files\VST3\
```

Restart your DAW and rescan VST3 plugins if it isn't visible. You may need
Administrator permission to copy files into Program Files. Turn monitoring down
for initial tests. PNORDE **generates MIDI**, not sounds: route it to a software
instrument in Ableton.

## What can it do?

- Independent clocks, sequence lengths, direction and rotation for different lanes.
- Gate, pitch, probability, controller and other dedicated/optional lanes.
- Four Quick Programs, including an automatic PROGRAM lane.
- Relative pitch and scale-aware generative pattern options.
- Visual HPPD effects, appearance controls and resizable editor.
- Optional Developer Tools and MIDI Black Box for troubleshooting.

**Planned musical features:** **Chord Mode first**, then ratcheting. They are
not part of v0.6.29. The current version's existing gate and clock controls
already allow certain manual ratchet-like patterns.

## Build from source (Windows 11)

**Prerequisites:** Visual Studio 2022 Community or newer with **Desktop
development with C++**, CMake â‰¥ 3.22, Git and PowerShell.

### Get the sources and JUCE together

**Option A â€” GitHub clone (recommended):**

```powershell
git clone --recurse-submodules https://github.com/OWNER/REPO.git
cd REPO
```

`OWNER/REPO` is the actual published repository URL. After the maintainer adds
the pinned JUCE submodule, the clone command fetches JUCE **inside** the PNORDE
folder, under `third_party/JUCE`, without a separate manual SDK installation.

**Option B â€” Download ZIP:** Extract it anywhere (no special drive or folder).
In Windows File Explorer, navigate into that extracted folder, then
**right-click â†’ Open in Terminal** to start PowerShell there:

```powershell
.\scripts\Get-JUCE.ps1
```

The helper fetches an official tagged JUCE copy next to the plugin code.
You can also run `.\scripts\Get-Audiowide-License.ps1` before publishing.
JUCE has its own licence; see [third-party notices](THIRD_PARTY_NOTICES.md).

### Compile and test

From PowerShell opened in the PNORDE source folder (the one containing
`CMakeLists.txt`):

```powershell
.\scripts\Build-And-Test.ps1
```

If PowerShell refuses to run local unsigned scripts, you can allow the one
PowerShell session to run locally trusted scripts (review them first):

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force
.\scripts\Build-And-Test.ps1
```

This compiles the VST3 and nine regression executables. It does **not** modify
your installed copy. Run only on Windows with supported Visual Studio tools.

To choose an existing JUCE folder explicitly instead of downloading:

```powershell
.\scripts\Build-And-Test.ps1 -JucePath 'E:\Somewhere\JUCE'
```

The path is an example. Change it to **your own chosen folder**.

### Install your own build (optional)

After all tests pass and Ableton is closed, launch **Administrator PowerShell**
from the same source folder, and run:

```powershell
.\scripts\Install-Local.ps1
```

The script hashes the tested binary and backs up an existing install into
`Documents\PNORDE-Backups`. If you are simply downloading the release ZIP,
you do **not** need this developer installation script.

## For contributors / curious readers

- [Source code explained for beginners](docs/SOURCE_GUIDE.md) â€” threads, MIDI event timing, ValueTree, saved programs, and a C++ glossary.
- [Publishing and licence decisions](docs/PUBLISHING_AND_LICENSING.md)
- [Windows release notes](RELEASE_NOTES.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)

The source has explanatory headers and comments on important functions.
We welcome careful, bounded changes and regression tests, particularly in
real-time scheduling and state compatibility.

## Where can I download the plugin directly?

At the very top of this page: **Download the latest compiled Windows VST3**.
The file is stored as a **GitHub Release asset**, rather than committed to Git
source history. The asset must be published before the link will resolve.

## Licensing

Copyright (C) 2026 alteredwavestudios.

PNORDE's original application source code is licensed under the
**GNU Affero General Public License, Version 3 (AGPL-3.0-only)**.

See the full [LICENSE](LICENSE) for the terms.

Third-party components retain their own licences:

- **JUCE:** AGPLv3 or a separate commercial JUCE licence.
- **Audiowide:** SIL Open Font License 1.1.

See [Third-party notices](THIRD_PARTY_NOTICES.md) for details.


