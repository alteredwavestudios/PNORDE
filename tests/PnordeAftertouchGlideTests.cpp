#include "../Source/PnordeAftertouchGlide.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main()
{
    using P = PnordeAftertouchGlide;
    assert(P::clampSetting(-200) == -1);
    assert(P::clampSetting(250) == 201);
    assert(P::durationBeats(0, 120, 0.5) == 0.0);
    assert(std::abs(P::durationBeats(15, 120, 0.5) - 0.075) < 1e-9); // 1.5 seconds
    assert(std::abs(P::durationBeats(100, 120, 0.5) - 0.5) < 1e-9);
    assert(std::abs(P::durationBeats(-1, 120, 0.5) - 0.5) < 1e-9); // legacy
    assert(std::abs(P::durationBeats(-1, 120, 25) - 20.0) < 1e-9); // old 10s max
    assert(P::durationBeats(100, 0.0, 0.5) == 0.0);
    assert(P::durationBeats(100, std::numeric_limits<double>::quiet_NaN(), 0.5) == 0.0);
    for (int bpm = 1; bpm < 1000; ++bpm)
        for (int glide = -1; glide <= 201; ++glide)
            assert(std::isfinite(P::durationBeats(glide, bpm, 0.125)));
    std::cout << "PASS: tempo-relative aftertouch plus bounded prior-seconds format\n";
}
