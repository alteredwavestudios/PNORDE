#include "../Source/PnordeProgramLane.h"
#include <cassert>
#include <array>
#include <iostream>
int main()
{
    using namespace PnordeProgramLane;
    assert(boundedLength(-5)==1 && boundedLength(1000)==128);
    assert(boundedSlot(-1)==0 && boundedSlot(5)==4);
    for (int length=1; length<=128; ++length)
    for (int start=0; start<length; ++start)
    {
        assert(activeIndex(0,length,start)==start);
        for (std::int64_t tick=-256;tick<=256;++tick)
        {
            const int active=activeIndex(tick,length,start);
            assert(active>=0 && active<length);
            const int completed=completedIndex(tick+1,length,start);
            assert(completed==active);
            assert(activeIndex(tick+length,length,start)==active);
        }
    }
    // 8-cell lane starts at 3 (one-based), then 4,5,...1,2.
    std::array<int,8> expected {2,3,4,5,6,7,0,1};
    for (int i=0;i<8;++i) assert(activeIndex(i,8,2)==expected[i]);
    // Trigger at end of step, not at its beginning.
    assert(completedIndex(1,8,2)==2);
    assert(completedIndex(2,8,2)==3);
    std::cout << "PASS v0.6.16 PROGRAM lane boundaries, wrap, 128-step limits\n";
}
