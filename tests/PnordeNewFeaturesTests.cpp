#include "../Source/PnordePlayback.h"
#include "../Source/PnordePatternLibrary.h"
#include "../Source/PnordeFactoryPresets.h"
#include <cassert>
#include <array>
#include <set>
int main()
{
    using PnordePlayback::index;
    for (int len=1;len<=128;++len)
      for(int dir=0;dir<3;++dir)
        for (std::int64_t tick=-2000;tick<=2000;tick+=7)
            assert(index(tick,len,dir)>=0 && index(tick,len,dir)<len);
    assert(index(0,8,0)==0 && index(7,8,0)==7);
    assert(index(0,8,1)==7 && index(7,8,1)==0);
    assert(index(0,4,2)==0 && index(1,4,2)==1 && index(2,4,2)==2 &&
           index(3,4,2)==3 && index(4,4,2)==2 && index(5,4,2)==1 && index(6,4,2)==0);
    std::array<int,128> pan{},glide{},slide{},pitch{};
    for (int type=0;type<PnordePatternLibrary::panCount;++type)
    {
        PnordePatternLibrary::pans(pan,glide,128,type,14);
        for (int i=0;i<128;++i) {assert(pan[i]>=0 && pan[i]<=127);assert(glide[i]>=0 && glide[i]<=100);}
    }
    for (int type=0;type<PnordePatternLibrary::slideCount;++type)
    {
        PnordePatternLibrary::slides(slide,128,type,14);
        for (int v:slide) assert(v>=0 && v<=100);
    }
    assert(PnordeFactoryPresets::presets.size()>=8);
    for (const auto& factory : PnordeFactoryPresets::presets)
    {
        assert(factory.gateRecipe>=0 && factory.gateRecipe<PnordePatternLibrary::gateCount);
        assert(factory.velocityRecipe>=0 && factory.velocityRecipe<PnordePatternLibrary::velocityCount);
        assert(factory.panRecipe>=0 && factory.panRecipe<PnordePatternLibrary::panCount);
        assert(factory.slideRecipe>=0 && factory.slideRecipe<PnordePatternLibrary::slideCount);
        PnordeFactoryPresets::pitch(pitch,128,factory.pitchSpan,7);
        for (int v:pitch) assert(v>=-120 && v<=120);
    }
    return 0;
}
