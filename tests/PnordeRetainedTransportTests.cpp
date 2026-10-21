#include "../Source/PnordeRetainedTransport.h"
#include "../Source/PnordePlayback.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    using namespace PnordeRetainedTransport;
    double clock = 0.0;
    const double dur = 0.25;
    for (int i = 0; i < 100; ++i)
        clock = endOfBlock(clock, 120.0, 240, 48000.0);
    assert(std::abs(clock - 1.0) < 1e-10);
    // Paused: no audio blocks advance the retained beat clock.
    const double paused = clock;
    assert(tickAt(paused, 0.0, dur) == 4);
    // Resume at 90 BPM; phase must remain at the same clock position.
    clock = endOfBlock(clock, 90.0, 480, 48000.0);
    assert(std::abs(clock - paused - 0.015) < 1e-10);
    // Lane 1 restart does not affect lane 2, with a different length.
    const double lane1Origin = clock;
    const double lane2Origin = 0.0;
    assert(tickAt(clock, lane1Origin, dur) == 0);
    assert(tickAt(clock, lane2Origin, dur) == 4);
    assert(PnordePlayback::index(tickAt(clock, lane1Origin, dur), 5, 0) == 0);
    assert(PnordePlayback::index(tickAt(clock, lane2Origin, dur), 7, 0) == 4);
    // SEQ restart uses a boundary at precisely the restarted local origin.
    const auto nextTick = nextBoundaryTick(clock, lane1Origin, dur);
    assert(nextTick == 0);
    assert(std::abs(boundaryAt(nextTick, lane1Origin, dur) - clock) < 1e-12);
    // Global restart resets every origin together; no negative/overflow indices.
    clock = 0.0;
    for (int len = 1; len <= 128; ++len)
    {
        for (int tick = 0; tick < 10000; ++tick)
        {
            const int idx = PnordePlayback::index(tickAt(static_cast<double>(tick), 0.0, dur), len, 0);
            assert(idx >= 0 && idx < len);
        }
    }
    std::cout << "Retained pause/resume and 21-lane phase helpers passed\n";
}
