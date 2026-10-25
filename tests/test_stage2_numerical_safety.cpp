// Stage 2: numerical safety regression tests (no JUCE or Ableton required).
// This file is intentionally thorough and beginner-readable. Each assert()
// stops the executable if an expected result is false; success prints PASS.
#include "../Source/PnordeSafeAutomation.h"
#include "../Source/PnordeRetainedTransport.h"
#include "../Source/PnordeManualTempo.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>

int main()
{
    using namespace PnordeRetainedTransport;
    namespace A = PnordeSafeAutomation;
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const double dinf = std::numeric_limits<double>::infinity();
    const double dnan = std::numeric_limits<double>::quiet_NaN();

    // 1. Every malformed host float gets a documented, valid fallback.
    for (float bad : {nan, inf, -inf})
    {
        assert(A::truncate(bad, 0, 10, 2) == 2);
        assert(A::truncate(bad, 0, 2, 0) == 0);
        assert(A::round(bad, 50, 75, 50) == 50);
    }
    // Gigantic *finite* floats are also safely clamped BEFORE int conversion.
    assert(A::truncate(3.4e38f, 0, 10, 2) == 10);
    assert(A::truncate(-3.4e38f, 0, 10, 2) == 0);
    assert(A::round(3.4e38f, 50, 75, 50) == 75);

    // 2. Exhaustive small-range compatibility with the previous behaviour.
    for (int hundredths = -3000; hundredths <= 3000; ++hundredths)
    {
        float f = static_cast<float>(hundredths) / 100.0f;
        auto oldTruncate = [](float value, int lo, int hi) {
            const int casted = static_cast<int>(value); // safe: our inputs are small
            return std::max(lo, std::min(hi, casted));
        };
        auto oldRound = [](float value, int lo, int hi) {
            const int rounded = static_cast<int>(std::lround(value));
            return std::max(lo, std::min(hi, rounded));
        };
        assert(A::truncate(f, 0, 10, 2) == oldTruncate(f, 0, 10));
        assert(A::truncate(f, 0, 2, 0) == oldTruncate(f, 0, 2));
        assert(A::round(f, 50, 75, 50) == oldRound(f, 50, 75));
    }

    // 3. Malformed retained clock values never reach invalid int64 casts.
    assert(endOfBlock(4.0, 120.0, 512, 0.0) == 4.0);
    assert(endOfBlock(4.0, 120.0, 512, dinf) == 4.0);
    assert(endOfBlock(4.0, dnan, 512, 44100.0) == 4.0);
    assert(tickAt(4.0, 0.0, 0.0) == 0);
    assert(tickAt(dinf, 0.0, 0.5) == 0);
    assert(tickAt(1.0e300, 0.0, 1.0e-300) == 0);
    assert(nextBoundaryTick(dnan, 0.0, 0.5) == 0);
    assert(nextBoundaryTick(1.0e300, 0.0, 1.0e-300) == 0);
    assert(boundaryAt(4, dnan, 0.5) == 0.0);
    assert(boundaryAt(4, 0.0, 0.0) == 0.0);

    // 4. The original tick formulas must match for realistic music timing.
    // Thousands of clock positions, varied BPMs and durations. Do NOT modify
    // the 1e-9 tick tolerance without adjusting this compatibility test.
    std::mt19937 rng(20261025);
    std::uniform_real_distribution<double> position(-100000.0, 100000.0);
    for (int n = 0; n < 100000; ++n)
    {
        const double at = position(rng);
        const double origin = position(rng) / 10.0;
        const double step = (n % 5 == 0) ? 0.125 : (n % 5 == 1) ? 0.5
                          : (n % 5 == 2) ? 1.0 : (n % 5 == 3) ? 8.0 : 128.0;
        assert(tickAt(at, origin, step) ==
            static_cast<std::int64_t>(std::floor((at-origin)/step + 1.0e-9)));
        assert(nextBoundaryTick(at, origin, step) ==
            static_cast<std::int64_t>(std::ceil((at-origin)/step - 1.0e-9)));
        assert(boundaryAt(120, origin, step) == origin + 120.0 * step);
    }
    assert(endOfBlock(123.5, 125.0, 512, 48000.0) ==
           123.5 + 512.0 * 125.0 / (60.0 * 48000.0));

    // 5. Normal manual/host tempo calculations match the Stage 1 equations.
    // This compares both clock modes, seeks, stopped-to-playing transitions,
    // and normal sample rates. It specifically guards against changed grooves.
    for (int n = 0; n < 20000; ++n)
    {
        const double hostPpq = position(rng);
        const double hostBpm = 20.0 + static_cast<double>(n % 301);
        const double requestedBpm = 20.0 + static_cast<double>(n % 281);
        const double sampleRate = (n % 2 == 0) ? 44100.0 : 96000.0;
        const int sampleCount = 1 + (n % 2048);
        const bool manual = (n % 3 != 0);
        const bool wasPlaying = (n % 7 != 0);
        const bool wasManual = (n % 5 != 0);
        const double prevHostEnd = hostPpq + (n % 11 == 0 ? 100.0 : 0.0);
        const double prevManualEnd = hostPpq + 0.12;
        const PnordeManualTempo::Input v {hostPpq, hostBpm, requestedBpm,
            sampleRate, sampleCount, manual, wasPlaying, wasManual,
            prevHostEnd, prevManualEnd};
        const auto output = PnordeManualTempo::advance(v);
        const double advanceHost = sampleCount * hostBpm / (60.0 * sampleRate);
        const bool jump = wasPlaying && std::abs(hostPpq-prevHostEnd) >
            std::max(0.05, advanceHost * 3.0);
        const bool oldReset = jump || !wasPlaying || !wasManual;
        const double oldStart = manual ? (oldReset ? hostPpq : prevManualEnd) : hostPpq;
        const double oldBpm = manual ? requestedBpm : hostBpm;
        const double oldEnd = oldStart + sampleCount*oldBpm/(60.0*sampleRate);
        assert(output.start == oldStart && output.end == oldEnd);
        assert(output.bpm == oldBpm && output.reset == oldReset);
        assert(output.hostEnd == hostPpq + advanceHost);
    }

    // 6. Manual tempo: invalid rates/positions safely produce an empty block.
    PnordeManualTempo::Input input {12.0, 120.0, 130.0, 48000.0,
        512, true, true, true, 12.0, 12.0};
    const auto normal = PnordeManualTempo::advance(input);
    assert(normal.start == 12.0 && normal.end > normal.start);
    input.sampleRate = 0.0;
    const auto invalidRate = PnordeManualTempo::advance(input);
    assert(invalidRate.start == invalidRate.end && invalidRate.reset);
    input.sampleRate = 48000.0;
    input.hostPpq = dnan;
    const auto invalidPpq = PnordeManualTempo::advance(input);
    assert(std::isfinite(invalidPpq.start) && invalidPpq.start == invalidPpq.end);
    input.hostPpq = 12.0;
    input.sampleRate = 1.0e-300;
    const auto overflow = PnordeManualTempo::advance(input);
    assert(std::isfinite(overflow.end) && overflow.start == overflow.end);

    std::cout << "PASS: nonfinite host floats, legacy automation behaviour, "
                 "retained clock, sample timing and malformed manual tempo\n";
}
