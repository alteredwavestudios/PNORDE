#include "../Source/PnordePatternLibrary.h"
#include <cassert>
int main()
{
    std::array<bool,128> gate{};
    std::array<int,128> velocity{};
    for (int c=0;c<PnordePatternLibrary::gateCount;++c)
    {
        PnordePatternLibrary::gates(gate,128,c,12345);
        bool hasOn=false;
        for (bool g:gate) hasOn |= g;
        assert(hasOn);
    }
    for (int c=0;c<PnordePatternLibrary::velocityCount;++c)
    {
        PnordePatternLibrary::velocities(velocity,128,c,12345);
        for (int v:velocity) assert(v>=1 && v<=100);
    }
}
