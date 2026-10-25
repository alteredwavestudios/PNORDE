// ============================================================================
// PnordeManualTempo.h — PNORDE ManualTempo
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
#include <algorithm>
#include <cmath>

// Pure, bounded, allocation-free musical beat clock for the audio callback.
// Ableton continues to control transport and seeking; Manual BPM overrides
// only PNORDE's beat progression while playing.
namespace PnordeManualTempo
{
struct Input
{
    double hostPpq, hostBpm, requestedBpm, sampleRate;
    int sampleCount;
    bool manualEnabled, previousPlaying, previousManual;
    double previousHostEnd, previousManualEnd;
};
struct Output
{
    double start, end, bpm, hostEnd;
    bool manualMode, reset;
};
inline Output advance(const Input& v) noexcept
{
    // The normal DAW/audio callback always supplies finite timing values.
    // But malformed host data must never reach the sample-rate division or
    // generate an invalid beat position. On bad input, keep a neutral block
    // and request a clock reset rather than inventing forward MIDI events.
    if (!std::isfinite(v.hostPpq) || !std::isfinite(v.sampleRate) ||
        v.sampleRate < 1.0 || v.sampleCount < 0 ||
        !std::isfinite(v.previousHostEnd) ||
        !std::isfinite(v.previousManualEnd))
    {
        const double safeStart = std::isfinite(v.hostPpq) ? v.hostPpq : 0.0;
        return {safeStart, safeStart, 120.0, safeStart, v.manualEnabled, true};
    }
    const double hostBpm = std::isfinite(v.hostBpm) ?
        std::max(1.0, std::min(1000.0, v.hostBpm)) : 120.0;
    const double bpm = v.manualEnabled && std::isfinite(v.requestedBpm)
        ? std::max(20.0, std::min(300.0, v.requestedBpm)) : hostBpm;
    const double sampleRate = std::max(1.0, v.sampleRate);
    const double advanceHost = v.sampleCount * hostBpm / (60.0 * sampleRate);
    const bool jump = v.previousPlaying &&
        std::abs(v.hostPpq - v.previousHostEnd) > std::max(0.05, advanceHost * 3.0);
    const bool reset = jump || !v.previousPlaying || !v.previousManual;
    const double start = v.manualEnabled
        ? (reset ? v.hostPpq : v.previousManualEnd) : v.hostPpq;
    const double end = start + v.sampleCount*bpm/(60.0*sampleRate);
    const double hostEnd = v.hostPpq + advanceHost;
    // Very small but positive rates can still overflow intermediate maths.
    // Do not hand an infinite clock to the MIDI engine if that happens.
    if (!std::isfinite(start) || !std::isfinite(end) || !std::isfinite(hostEnd) ||
        !std::isfinite(advanceHost))
        return {v.hostPpq, v.hostPpq, bpm, v.hostPpq, v.manualEnabled, true};
    return {start, end, bpm, hostEnd, v.manualEnabled, reset};
}
}
