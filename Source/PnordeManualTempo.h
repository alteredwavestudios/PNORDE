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
    return {start, start + v.sampleCount*bpm/(60.0*sampleRate), bpm,
            v.hostPpq+advanceHost, v.manualEnabled, reset};
}
}
