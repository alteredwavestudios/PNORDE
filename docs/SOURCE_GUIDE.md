# PNORDE source code explained for beginners

> Version 0.6.29. These notes describe the existing stable engine; they are not instructions to change real-time MIDI behaviour.

## Four big pieces

1. **The host (Ableton)** starts the plugin, chooses tempo and buffer size, calls `processBlock`, and stores plugin state with the project.
2. **`PluginProcessor`** is PNORDE's MIDI/music engine. It reads timing, runs many independent lane clocks, makes MIDI notes and controller data, and tracks Quick Programs.
3. **`PluginEditor`** draws the user interface and handles human editing. It usually operates on JUCE's **message thread**.
4. **Utility headers** isolate the mathematical ideas (scales, swing, probability, rotations) so they can be tested without running Ableton.

## What happens to one note?

```text
Ableton host tempo/position
         |
         v
PluginProcessor::processBlock() -- called on audio thread
         |
         v
Lane clock and step index (PnordePlayback / PnordeSwing)
         |
         v
Gate + probability + pitch decision
         |
         v
JUCE MidiBuffer event at offset N samples inside current block
         |
         v
Ableton MIDI routing -> another instrument -> sound
```

The **sample offset** matters: events in one block can happen at different samples rather than all firing at its beginning. Writing debug text, opening files or waiting on locks here would risk audio dropouts.

## Quick Programs: why it is tricky

```text
PROGRAM lane makes request (audio thread)
                 |
                 v
small atomic mailbox -- no heavy state loading on audio thread
                 |
                 v
120 Hz timer notices request (message thread)
                 |
                 v
save outgoing program -> prepare incoming snapshot -> validate -> apply
                 |
                 v
message-thread/UI parameter updates complete
```

The total delay includes polling plus state work. Keeping state work off the audio thread helps avoid audio glitches. **Play/Pause is global**, not a saved setting within each Quick Program. In v0.6.29 the program-state compatibility build stays **625**, while the application build is **629**.

## MIDI black box: what it CAN and CANNOT know

- It can count emitted Note On/Off, CC64 sustain, CC120 All Sound Off, CC123 All Notes Off, pitch bend, and select important events.
- It can show a bounded pre-stop trace and approximate number of outstanding note keys.
- It **cannot see the instrument's audio** or know whether Arturia, Massive, or a hardware synth actually stopped a voice.
- CC120 may clear the recorder's note bookkeeping without emitting individual Note Offs. So equal counts are not a strict requirement for every situation.
- It is intentionally opt-in; inactive diagnostics shouldn't carry appreciable overhead.

## A quick C++ dictionary

| Syntax | Plain English |
|---|---|
| `.h` | A file announcing types, variables or helper functions |
| `.cpp` | A file implementing those declarations |
| `#include` | Bring declarations from another file into this translation unit |
| `class` | A blueprint containing data and functions |
| `struct` | Similar to a class; fields are public by default |
| `std::array<T, N>` | A fixed-size set of N values of type T |
| `std::atomic<T>` | A value designed for certain operations across threads without data races |
| `const` | Cannot change through this reference or object |
| `noexcept` | Promises the function will not throw an exception |
| `auto` | Ask C++ to infer the variable's type from its initializer |
| `::` | Member of a namespace or class |
| `&` | Often means a reference: avoid making a full copy of an object |
| `std::move` | Permit transferring ownership/resources instead of making a copy |
| `unique_ptr` | An owning smart pointer, automatically releases what it owns |
| `ValueTree` | JUCE's editable tree of saved/parameter state |
| `MidiBuffer` | JUCE's list of MIDI messages with sample offsets |
| `AudioProcessorValueTreeState` | JUCE's system for host-automatable parameters |

## File map

| Path | Main role |
|---|---|
| `Source/PluginProcessor.cpp/.h` | Transport, MIDI schedule, state and voice tracking |
| `Source/PluginEditor.cpp/.h` | Drawing, event handlers, pages, settings, HPPD |
| `Source/PnordeLane.h` | Lane categories and controller mappings |
| `Source/PnordeScales.h` | Musical scales and quantisation constants |
| `Source/PnordePlayback.h` | Step traversal and direction |
| `Source/PnordeProbability.h` | Per-cell probability |
| `Source/PnordeMidiBlackBox.h` | Debug MIDI event history |
| `Source/PnordeDeveloperStats.h` | Performance counters |
| `Source/PnordeProjectVersion.h` | Public version and saved-state compatibility |
| `tests/` | Console programs that exit nonzero on failed assumptions |
| `CMakeLists.txt` | Defines targets and where JUCE comes from |
| `scripts/` | Repeatable PowerShell build, installation and release |

## Why more comments, not more changes?

This public-release preparation adds explanations to the source without rewriting sequencing code. Comments are for readers; they have no runtime effect. The JUCE dependency and the build scripts have been made portable; those need a fresh Windows test before a public binary is attached.

## Next musical work

**Chord Mode** is next, then ratcheting. Chord Mode should be designed around voice allocation, note-off tracking, chord inversion and interaction with relative-pitch/scale behaviour. None of that is implemented in this 0.6.29 publication package.
