#include "../Source/PnordePatternLibrary.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
int main()
{
    for (int length=1;length<=128;++length)
        for (int choice=0;choice<PnordePatternLibrary::aftertouchCount;++choice)
            for (std::uint32_t seed : {0u,1u,0xffffffffu,0xabcdeu})
            {
                std::array<int,128> out{};
                out.fill(-99);
                PnordePatternLibrary::aftertouch(out,length,choice,seed);
                for (int i=0;i<length;++i) assert(out[i]>=0 && out[i]<=100);
                for (int i=length;i<128;++i) assert(out[i]==-99);
                auto same=out;
                PnordePatternLibrary::aftertouch(out,length,choice,seed);
                assert(out==same);
            }
    std::array<int,128> out{};out.fill(7);
    PnordePatternLibrary::aftertouch(out,128,-1,0);
    for (int v:out)assert(v==7);
    PnordePatternLibrary::aftertouch(out,129,1,0);
    for (int v:out)assert(v==7);
    std::cout << "PASS 0.6.15: bounded, deterministic aftertouch recipes\n";
}
