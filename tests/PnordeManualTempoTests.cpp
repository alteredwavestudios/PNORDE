#include "../Source/PnordeManualTempo.h"
#include <cassert>
#include <cmath>
#include <iostream>
using PnordeManualTempo::Input;
int main()
{
    Input input{0.0, 120.0, 90.0, 48000.0, 480, false,false,false,0,0};
    auto previous=PnordeManualTempo::advance(input);
    assert(std::abs(previous.end-0.02)<1e-9); // host BPM 120
    input.hostPpq=previous.hostEnd;
    input.previousHostEnd=previous.hostEnd;
    input.previousManualEnd=previous.end;
    input.previousPlaying=true;
    input.manualEnabled=true;
    auto switched=PnordeManualTempo::advance(input);
    assert(std::abs(switched.start-previous.hostEnd)<1e-9);
    assert(std::abs(switched.end-switched.start-0.015)<1e-9);
    input.hostPpq=switched.hostEnd;
    input.previousHostEnd=switched.hostEnd;
    input.previousManualEnd=switched.end;
    input.previousManual=true;
    auto running=PnordeManualTempo::advance(input);
    assert(std::abs(running.start-switched.end)<1e-9);
    input.hostPpq=8.0;
    auto seek=PnordeManualTempo::advance(input);
    assert(seek.reset && seek.start==8.0);
    input.hostPpq=0.1;
    input.previousHostEnd=0.1;
    input.previousManualEnd=0.1;
    input.previousPlaying=false;
    auto restart=PnordeManualTempo::advance(input);
    assert(restart.reset && restart.start==0.1);
    input.requestedBpm=1.0;
    assert(PnordeManualTempo::advance(input).bpm==20.0);
    input.requestedBpm=9999.0;
    assert(PnordeManualTempo::advance(input).bpm==300.0);
    input.manualEnabled=false;
    assert(PnordeManualTempo::advance(input).bpm==120.0);
    // Stress toggling, seeking and long runs. Pure arithmetic, no dynamic memory.
    for (int i=0;i<100000;++i)
    {
        input.manualEnabled=(i%17)!=0;
        input.previousPlaying=(i%113)!=0;
        input.hostPpq=i*0.01;
        input.previousHostEnd=(i-1)*0.01;
        const auto out=PnordeManualTempo::advance(input);
        assert(std::isfinite(out.start) && std::isfinite(out.end));
        assert(out.end>=out.start);
    }
    std::cout << "Manual-tempo stress tests passed\n";
}
