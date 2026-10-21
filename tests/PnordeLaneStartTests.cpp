#include "../Source/PnordePlayback.h"
#include <cassert>
#include <array>
#include <cstdint>
int main()
{
    using PnordePlayback::indexWithStart;
    // Note-on steps and UI read-out follow the very same calculation.
    for (int len = 1; len <= 128; ++len)
      for (int start = -2; start <= len + 2; ++start)
        for (int direction = 0; direction <= 2; ++direction)
        {
            const int clampedStart = start < 0 ? 0 : (start >= len ? len - 1 : start);
            assert(indexWithStart(0, len, direction, start) == clampedStart);
            for (std::int64_t tick = -300; tick <= 300; ++tick)
            {
                const int step = indexWithStart(tick, len, direction, start);
                assert(step >= 0 && step < len);
                if (direction == 0)
                {
                    const int next = indexWithStart(tick + 1, len, direction, start);
                    assert(next == (step + 1) % len);
                }
                if (direction == 1)
                {
                    const int next = indexWithStart(tick + 1, len, direction, start);
                    assert(next == (step + len - 1) % len);
                }
            }
        }
    assert(indexWithStart(0, 8, 0, 2) == 2);
    assert(indexWithStart(1, 8, 0, 2) == 3);
    assert(indexWithStart(7, 8, 0, 2) == 1);
    assert(indexWithStart(0, 8, 1, 2) == 2);
    assert(indexWithStart(1, 8, 1, 2) == 1);
    assert(indexWithStart(2, 8, 1, 2) == 0);
    assert(indexWithStart(3, 8, 1, 2) == 7);
    assert(indexWithStart(0, 8, 2, 2) == 2);
    assert(indexWithStart(7, 8, 2, 2) == 1);
    assert(indexWithStart(8, 8, 2, 2) == 0);
    assert(indexWithStart(0, 128, 0, 99) == 99);
    assert(indexWithStart(0, 3, 0, 99) == 2);
}
