// ============================================================================
// PnordeMicroOutput.h — PNORDE MicroOutput
// ============================================================================
// This helper isolates one part of the sequencer so it can be understood and tested without reading the whole plugin.
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
#include "PnordeEDO.h"
// Host-safe, testable microtonal output policy. Never alter saved pn_bend when
// temporarily selecting a microtonal tuning: legacy projects regain their
// user-selected bend range immediately on returning to ordinary 12-TET.
namespace PnordeMicroOutput
{
constexpr int bendSafe = 0;
constexpr int experimentalMPE = 1;
constexpr int legacySlide = 2; // Saved-state alias: now identical to standard.
constexpr int microtonalBendSemitones = 2;
// Free 0.1 allows fractional semitones; Rast and Bayati have quarter-tones.
// All current EDO modes also use fractional semitone output.
constexpr bool isMicrotonalScale(int index) noexcept
{ return index == 13 || index == 32 || index == 33 || PnordeEDO::isMode(index); }
constexpr bool mpeEnabled(bool microtonal, int mode) noexcept
{ return microtonal && mode == experimentalMPE; }
constexpr int bendRange(bool microtonal, int /*mode*/, int traditionalRange) noexcept
{ return microtonal ? microtonalBendSemitones : traditionalRange; }
constexpr int effectiveBendRangeIndex(bool microtonal, int savedIndex) noexcept
{ return microtonal ? 0 : savedIndex; }
constexpr int noteChannel(bool microtonal, int mode, int requested) noexcept
{ return mpeEnabled(microtonal,mode) ? 2 : requested; }
// SLIDE never disappears: its per-step percentage is the length of the
// pitch-approach ramp, measured against the current sounding gate duration.
inline double slideApproachBeats(int amount, double sequenceBeats, int gatePercent) noexcept
{
    if (amount <= 0 || gatePercent <= 0 || sequenceBeats <= 0.0) return 0.0;
    if (amount > 100) amount = 100;
    if (gatePercent > 100) gatePercent = 100;
    return sequenceBeats * (static_cast<double>(gatePercent) / 100.0)
                         * (static_cast<double>(amount) / 100.0);
}
}
