#include "../Source/PnordeScaleDegreeEdit.h"
#include <cassert>
#include <iostream>

namespace E = PnordeScaleDegreeEdit;
int main()
{
    // C Major: C, E, G -> D, F, A after +1 scale degree.
    auto major = E::buildAllowedPitches(1);
    assert(major.count > 0);
    int base[] { E::nearestIndex(major, 0), E::nearestIndex(major, 40),
                 E::nearestIndex(major, 70) };
    auto move = E::boundedGroupDelta(base, 3, 1, 0, major.count - 1);
    assert(move == 1);
    assert(major.values[base[0] + move] == 20);
    assert(major.values[base[1] + move] == 50);
    assert(major.values[base[2] + move] == 90);
    // An over-large up/down drag never crosses the outer permitted notes.
    assert(E::boundedGroupDelta(base, 3, 100000, 0, major.count - 1) == major.count - 1 - base[2]);
    assert(E::boundedGroupDelta(base, 3, -100000, 0, major.count - 1) == -base[0]);
    // Numeric groups stop together at the first bound. The distances stay.
    int numeric[] {20, 60, 97};
    assert(E::boundedGroupDelta(numeric, 3, 50, 1, 100) == 3);
    assert(E::boundedGroupDelta(numeric, 3, -50, 1, 100) == -19);
    // Chromatic moves by one semitone, FREE by 0.1 semitone.
    auto chromatic = E::buildAllowedPitches(0);
    auto free = E::buildAllowedPitches(13);
    assert(chromatic.values[E::nearestIndex(chromatic, 20) + 1] == 30);
    assert(free.values[E::nearestIndex(free, 20) + 1] == 21);
    int octaves[] {-2, 0, 3};
    assert(E::boundedGroupDelta(octaves, 3, -100000, -4, 4) == -2);
    assert(E::boundedGroupDelta(octaves, 3, 100000, -4, 4) == 1);
    // For ALL scales (including EDO), accepted notes stay within +/-12.
    for (int scale = 0; scale < PnordeScales::count; ++scale)
    {
        auto allowed = E::buildAllowedPitches(scale);
        assert(allowed.count > 0 && allowed.count <= E::maximumEntries);
        for (int i = 0; i < allowed.count; ++i)
        {
            assert(allowed.values[i] >= E::lowestPitch);
            assert(allowed.values[i] <= E::highestPitch);
            if (i > 0) assert(allowed.values[i] > allowed.values[i-1]);
        }
        int indices[] { E::nearestIndex(allowed, -120), E::nearestIndex(allowed, -25),
                        E::nearestIndex(allowed, 120) };
        for (int proposed : {-1000, -1, 0, 1, 1000})
        {
            int d = E::boundedGroupDelta(indices, 3, proposed, 0, allowed.count-1);
            for (int value : indices)
            {
                int index = value + d;
                assert(index >= 0 && index < allowed.count);
            }
        }
    }
    std::cout << "CTRL scale-degree and hard bounds: all tests passed\n";
}
