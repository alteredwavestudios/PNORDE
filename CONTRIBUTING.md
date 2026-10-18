# Contributing

Thank you for experimenting with PNORDE! Please open an issue first for substantial changes, explain the musical or technical motivation, and submit focused pull requests.

- Build on Windows using JUCE and the instructions in `BUILDING.md`.
- Keep MIDI/audio thread work real-time safe; avoid allocations, blocking I/O or locks in `processBlock`.
- Validate file inputs and bounds, and keep new UI animations efficient.
- Include steps to reproduce bugs and describe your test environment.
- Preserve original creator attribution: **LANTERNS (Nova O'Reilly)**; credit later contributors as appropriate.
- Do not commit generated build files, private presets or proprietary third-party assets without proper licensing.

By contributing, you confirm you have the right to submit your work under the repository's licence.
