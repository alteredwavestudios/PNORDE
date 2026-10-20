#include "../Source/PnordeVisualOrder.h"
#include <cassert>
#include <random>
#include <iostream>

int main()
{
    auto rows = PnordeVisualOrder::identity();
    assert(PnordeVisualOrder::valid(rows));
    auto bad = rows;
    bad[2] = 21;
    assert(!PnordeVisualOrder::valid(bad));
    bad = rows; bad[3] = bad[4];
    assert(!PnordeVisualOrder::valid(bad));
    assert(!PnordeVisualOrder::move(bad, 7, 20));
    std::mt19937 rng(20260505);
    for (int run = 0; run < 1000; ++run)
    {
        rows = PnordeVisualOrder::identity();
        for (int i = 0; i < 1000; ++i)
        {
            const int from = 7 + int(rng() % 14), to = 7 + int(rng() % 14);
            assert(PnordeVisualOrder::move(rows, from, to));
            assert(PnordeVisualOrder::valid(rows));
        }
        // Deleting a macro compacts physical rows 7..14, preserving valid IDs.
        const int removedRow = 7 + int(rng() % 8);
        assert(PnordeVisualOrder::removeCompacted(rows, removedRow, 14));
        assert(PnordeVisualOrder::valid(rows));
    }
    std::cout << "PASS: visual permutation bounds, drag and compact-delete stress tests\n";
}
