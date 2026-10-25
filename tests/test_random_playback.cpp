#include "../Source/PnordePlayback.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

int main()
{
    using namespace PnordePlayback;
    // All three older playback modes must remain mathematically identical.
    const std::array<int, 8> forwardExpected {0,1,2,3,0,1,2,3};
    const std::array<int, 8> backwardExpected {3,2,1,0,3,2,1,0};
    const std::array<int, 8> pingPongExpected {0,1,2,3,2,1,0,1};
    for (int t = 0; t < 8; ++t)
    {
        assert(index(t, 4, forward) == forwardExpected[t]);
        assert(index(t, 4, backward) == backwardExpected[t]);
        assert(index(t, 4, pingPong) == pingPongExpected[t]);
    }
    // Nonzero lane start offsets keep their old meanings.
    assert(indexWithStart(0, 8, backward, 4) == 4);
    assert(indexWithStart(1, 8, backward, 4) == 3);
    assert(indexWithStart(1, 8, forward, 4) == 5);
    assert(indexWithStart(3, 4, pingPong, 1) == 0);

    // Fixed clock step = fixed RANDOM cell, even after 1000 re-reads.
    for (int lane = 0; lane < 21; ++lane)
        for (std::int64_t tick = -10; tick < 100; ++tick)
        {
            const int step = indexWithStart(tick, 13, randomStep, 2, lane);
            assert(step >= 0 && step < 13);
            for (int repeat = 0; repeat < 1000; ++repeat)
                assert(indexWithStart(tick, 13, randomStep, 2, lane) == step);
        }
    // Row 0 (SEQ) and row 1 (PITCH) should not share a randomStep stream.
    int differences = 0, repeats = 0;
    for (std::int64_t tick = 0; tick < 512; ++tick)
    {
        if (indexWithStart(tick, 16, randomStep, 0, 0) !=
            indexWithStart(tick, 16, randomStep, 0, 1)) ++differences;
        if (tick > 0 && index(tick, 4, randomStep, 0) == index(tick - 1, 4, randomStep, 0)) ++repeats;
        assert(index(tick, 1, randomStep, 0) == 0);
        assert(index(tick, 128, randomStep, 20) >= 0);
        assert(index(tick, 128, randomStep, 20) < 128);
    }
    assert(differences > 400);
    assert(repeats > 0); // Repeating the same cell twice is allowed.
    // Start marker offsets shift the selected cell, not the saved pattern.
    for (std::int64_t tick = 0; tick < 512; ++tick)
        assert(indexWithStart(tick, 16, randomStep, 3, 9) ==
               (indexWithStart(tick, 16, randomStep, 0, 9) + 3) % 16);
    std::cout << "Random/legacy playback tests passed\n";
}
