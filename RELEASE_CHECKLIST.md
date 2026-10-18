# Public alpha release checklist

- [ ] Replace the short `LICENSE` notice with the **full AGPLv3 text** downloaded from https://www.gnu.org/licenses/agpl-3.0.txt.
- [ ] Confirm and document redistribution permission and licensing for `Source/AudiowideEmbedded.h` (embedded font data). If in doubt, remove or replace it before publishing.
- [ ] Verify the rights and licences for any other bundled code/assets; JUCE's open-source/commercial licensing depends on the installed JUCE version and applicable terms.
- [ ] Check `CMakeLists.txt` for local absolute fallback paths; no secrets were identified, but use the public `-DJUCE_DIR` build instructions.
- [ ] Compile the exact public source and ensure it matches the uploaded plugin bundle.
- [ ] Scan the binary and repository for secrets and unwanted generated files.
- [ ] Tag `v0.5.2-alpha.1` and mark the GitHub Release **Pre-release**.
- [ ] Test download, extraction, DAW plugin detection and preset saving in a fresh Windows profile.

The binary archive is supplied separately from the repository source.
