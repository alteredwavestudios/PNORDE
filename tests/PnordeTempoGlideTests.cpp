#include "../Source/PnordeTempoGlide.h"
#include "../Source/PnordeAftertouchGlide.h"
#include <cassert>
#include <cmath>
int main() {
    assert(PnordeTempoGlide::durationBeats(0, 2.0) == 0.0);
    assert(PnordeTempoGlide::durationBeats(50, 2.0) == 1.0);
    assert(PnordeAftertouchGlide::durationBeats(-1, 120.0, 0.5) == 0.5);
    assert(PnordeAftertouchGlide::durationBeats(50, 120.0, 2.0) == 1.0);
    assert(std::abs(PnordeAftertouchGlide::durationBeats(116, 120.0, 0.5)-3.0)<1.e-9);
    for (int i=-1000;i<=1000;++i) {
        const auto b=PnordeAftertouchGlide::durationBeats(i, 120.0, 0.5);
        assert(std::isfinite(b) && b>=0);
    }
}
