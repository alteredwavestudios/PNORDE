#include "../Source/PnordeGlobalReverse.h"
#include "../Source/PnordePlayback.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
int main()
{
    using PnordeGlobalReverse::Phase;
    Phase p;
    std::array<std::int64_t,Phase::maxLanes> ticks{};
    for(int direction=0; direction<=2; ++direction)
      for(int length=1;length<=128;++length)
        for(int start=0;start<length;++start)
          for(std::int64_t at=0;at<300;++at)
          {
              p.reset(false);
              ticks.fill(at);
              auto original=PnordePlayback::indexWithStart(p.position(0,at),length,direction,start);
              p.update(true,ticks);
              auto flipped=PnordePlayback::indexWithStart(p.position(0,at),length,direction,start);
              assert(original==flipped); // never jumps on toggle
              // Flipping back at a later tick is also continuous.
              ticks.fill(at+7);
              const auto before=PnordePlayback::indexWithStart(p.position(0,at+7),length,direction,start);
              p.update(false,ticks);
              const auto after=PnordePlayback::indexWithStart(p.position(0,at+7),length,direction,start);
              assert(before==after);
              // All positions valid at all times.
              const auto position=PnordePlayback::indexWithStart(p.position(0,at+8),length,direction,start);
              assert(position>=0 && position<length);
          }
    // Reverse direction from current cell for forward/backward modes.
    p.reset(false); ticks.fill(5);
    p.update(true,ticks);
    assert(p.position(0,5)==5);
    assert(p.position(0,6)==4);
    assert(p.position(0,7)==3);
    // Distinct lanes preserve their own phase coordinates.
    p.reset(false); ticks.fill(5); ticks[1]=19;
    p.update(true,ticks);
    assert(p.position(0,5)==5);
    assert(p.position(1,19)==19);
    assert(p.position(0,6)==4);
    assert(p.position(1,20)==18);
    p.resetLane(1);
    assert(p.position(1,0)==0);
    std::cout<<"Global reverse transition tests passed\n";
}
