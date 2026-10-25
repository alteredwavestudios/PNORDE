# Third-party notices

## JUCE

PNORDE is developed with the JUCE C++ framework, owned/licensed independently.
The recommended GitHub repository uses the official framework as a pinned
Git submodule. Neither the PNORDE source licence nor this README changes JUCE's
licence, and the source ZIP does **not** contain a vendored JUCE framework copy.

- Framework: https://github.com/juce-framework/JUCE
- Licence: https://github.com/juce-framework/JUCE/blob/master/LICENSE.md
- Pinned bootstrap version: 9.0.4 (must be Windows-tested with this plugin)

JUCE 9 modules are dual-licensed under AGPLv3 or a commercial JUCE agreement.
Check your eligibility/obligations when publishing and redistributing binaries.

## Audiowide font

`Source/AudiowideEmbedded.h` contains an embedded Audiowide font byte array
from the original working PNORDE source. It is distributed as part of code
and may constitute redistribution of the font software.

- Typeface: Audiowide, by Brian J. Bonislawsky for Astigmatic.
- Upstream source and SIL OFL 1.1 terms:
  https://github.com/google/fonts/tree/main/ofl/audiowide
- Licence text:
  https://github.com/google/fonts/blob/main/ofl/audiowide/OFL.txt
- FONTLOG: https://github.com/google/fonts/blob/main/ofl/audiowide/FONTLOG.txt

Before publishing, verify the embedded byte array corresponds to a properly
licensed upstream font and accompany its distribution with the applicable
copyright/licence text. A link alone may not satisfy every redistribution duty.
