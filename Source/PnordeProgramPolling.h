// ============================================================================
// PnordeProgramPolling.h — Quick Program polling cadence
// ============================================================================
// A named constant for the 120 Hz polling mechanism. More frequent polling is not the same as sample-accurate restoration.
//
// Beginner notes:
//   - A header (.h) usually DECLARES names, types and helper functions.
//   - A source file (.cpp) usually IMPLEMENTS behaviour.
//   - `noexcept` promises an operation will not throw C++ exceptions.
//   - `const` generally indicates a value cannot be modified through that use.
//   - Avoid changing audio-thread behaviour merely to add documentation.
// See docs/SOURCE_GUIDE.md for terms, architecture and call-flow diagrams.
// ============================================================================

#pragma once

// PNORDE v0.6.26: EXPERIMENTAL faster checking of queued PROGRAM requests.
// This constant is used ONLY by the processor's JUCE message-thread timer.
//
// IMPORTANT DIFFERENCE:
// - PROGRAM lane and host automation: put a pending request in a mailbox;
//   this timer checks for it. 120 Hz = nominal 8.33 ms between checks.
// - Manual selection in PNORDE's interface: normally switches immediately
//   on the message thread, so this rate does NOT make manual clicks faster.
// - MIDI scheduling / Macro CC output / HPPD frame rate: unchanged.
//
// JUCE timers are *not* real-time clocks. When Ableton's UI is busy, a timer
// callback may arrive late. A faster timer also means more checks per second.
// We therefore TEST responsiveness and CPU usage rather than assuming it helps.
namespace PnordeProgramPolling
{
    constexpr int rateHz = 120; // Stage 5 used 60 Hz.
    static_assert(rateHz > 0 && rateHz <= 120,
                  "Do not raise the message-thread polling rate without profiling");
}
