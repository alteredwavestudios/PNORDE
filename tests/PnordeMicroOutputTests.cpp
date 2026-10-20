#include "../Source/PnordeMicroOutput.h"
#include <cassert>
#include <cmath>
int main()
{
    using namespace PnordeMicroOutput;
    for (int index=0;index<47;++index)
    {
        const bool expected = index==13 || index==32 || index==33 ||
                              (index>=34 && index<47);
        assert(isMicrotonalScale(index)==expected);
        assert(effectiveBendRangeIndex(expected,2)==(expected?0:2));
        for (int mode=0;mode<=2;++mode)
        {
            assert(bendRange(expected,mode,24)==(expected?2:24));
            assert(mpeEnabled(expected,mode)==(expected && mode==1));
            assert(noteChannel(expected,mode,14)==(expected && mode==1?2:14));
        }
    }
    for (int amount=0;amount<=100;++amount)
    {
        const auto duration=slideApproachBeats(amount,0.5,80);
        assert(duration>=0.0 && duration<=0.4);
        assert(std::abs(duration-0.4*amount/100.0)<1.e-9);
    }
    assert(slideApproachBeats(0,1.0,100)==0.0);
    assert(slideApproachBeats(100,1.0,100)==1.0);
    assert(slideApproachBeats(50,1.0,50)==0.25);
    assert(slideApproachBeats(150,1.0,150)==1.0);
    assert(slideApproachBeats(-10,1.0,100)==0.0);
    assert(slideApproachBeats(80,0.0,100)==0.0);
}
