#include "../Source/PnordeProbability.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <array>

int main()
{
    // These expectations protect backwards compatibility with old songs:
    // all default (100%) cells work, and zero-percent cells never trigger.
    for (int lane=0; lane<22; ++lane)
        for (std::int64_t tick=-256; tick<256; ++tick)
        {
            assert(PnordeProbability::passes(100,lane,tick));
            assert(!PnordeProbability::passes(0,lane,tick));
            assert(PnordeProbability::passes(50,lane,tick) ==
                   PnordeProbability::passes(50,lane,tick));
        }
    // Estimate how often each probability passes across 10,000 visits.
    // A 25% cell must not behave like a 5% or a 95% cell.
    for (int percentage : {10,25,50,75,90})
    {
        int accepted=0;
        for (int tick=0; tick<10000; ++tick)
            if (PnordeProbability::passes(percentage,3,tick)) ++accepted;
        assert(accepted > percentage*100-400);
        assert(accepted < percentage*100+400);
    }
    // Each lane rolls separately; differing lane numbers shouldn't make
    // all lanes choose the exact same result on every single clock tick.
    bool differ=false;
    for (int tick=0;tick<1000;++tick)
        differ |= PnordeProbability::passes(50,3,tick) !=
                  PnordeProbability::passes(50,4,tick);
    assert(differ);
    std::cout << "Probability: defaults, bounds, deterministic rolls, distribution passed\n";
}
