#include "../Source/PnordeCompositionEngine.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <set>
int main()
{
    using namespace PnordeCompositionEngine;
    static_assert(PnordeScales::count==47,"47 scale modes must remain ABI stable");
    for(int scale=0;scale<PnordeScales::count;++scale)
    {
        const int count=degreeCount(scale);
        assert(count>=5 && count<=12);
        for(int k=-count*2;k<count*2;++k)
        {
            const double d=degreeSemis(scale,k);
            assert(std::isfinite(d));
            assert(d>=-26.0 && d<=26.0);
        }
        for(int length: {1,16,32,48,64,96,128})
            for(int recipe=0;recipe<melodyCount;++recipe)
            {
                Phrase a,b;
                fill(a,scale,0,length,recipe,false,12345);
                fill(b,scale,0,length,recipe,false,12345);
                assert(a.length==length && a.scale==scale);
                for(int i=0;i<length;++i)
                {
                    assert(a.gates[i]==b.gates[i]);
                    assert(a.pitch[i]==b.pitch[i]);
                    assert(a.pitch[i]>=-120 && a.pitch[i]<=120);
                    assert(a.octave[i]>=-4 && a.octave[i]<=4);
                    assert(a.velocity[i]>=1 && a.velocity[i]<=100);
                    assert(a.duration[i]>=1 && a.duration[i]<=100);
                    assert(a.slide[i]>=0 && a.slide[i]<=100);
                    assert(a.pressure[i]>=0 && a.pressure[i]<=100);
                }
            }
        for(int recipe=0;recipe<arpCount;++recipe)
        {
            Phrase a;
            fill(a,scale,5,128,recipe,true,17);
            assert(a.length==128);
        }
    }
    // Four serial forms must each use all 12 pitch classes once.
    Phrase serial;
    fill(serial,0,0,96,6,false,44);
    makeSerial(serial,false);
    for(int chunk=0;chunk<8;++chunk)
    {
        std::set<int> pcs;
        for(int i=chunk*12;i<(chunk+1)*12;++i)
        {
            const int semis=serial.octave[i]*12+serial.pitch[i]/10;
            pcs.insert((semis%12+12)%12);
        }
        assert(pcs.size()==12);
    }
    for(int style=0;style<chromaticJazzCount;++style)
    {
        Phrase p;
        makeChromaticJazz(p,0,128,style,117);
        assert(p.scale==0 && p.length==128);
        for(int i=0;i<128;++i)
            assert(p.pitch[i]>=-120 && p.pitch[i]<=120);
    }
    for(int recipe=0;recipe<addedGateCount;++recipe)
    {
        std::array<bool,128> gates{};
        makeGate(gates,128,recipe,123);
        const int hits=std::count(gates.begin(),gates.end(),true);
        assert(hits>1 && hits<128);
    }
    for(const auto& c:compositions)
    {
        Phrase p;
        fill(p,c.scale,3,c.length,c.recipe,c.arp,22);
        assert(p.length==c.length);
    }
    std::cout<<"All PNORDE v0.6.21 composition tests passed\\n";
}
