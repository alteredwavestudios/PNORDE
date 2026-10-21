#include "../Source/PnordeProgramRequests.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
int main()
{
    int selected=-1;
    for (std::uint32_t epoch : {0u,1u,19u,100000u,UINT32_MAX})
        for (int program=1;program<=4;++program)
        {
            const auto packet=PnordeProgramRequests::pack(epoch,program);
            assert(packet != 0);
            assert(PnordeProgramRequests::unpack(packet,epoch,selected));
            assert(selected == program-1);
            if (epoch != UINT32_MAX)
                assert(!PnordeProgramRequests::unpack(packet,epoch+1,selected));
        }
    assert(PnordeProgramRequests::pack(0,0) == 0);
    assert(PnordeProgramRequests::pack(0,5) == 0);
    assert(!PnordeProgramRequests::unpack(0,0,selected));
    for (int slot=5;slot<=7;++slot)
        assert(!PnordeProgramRequests::unpack(static_cast<std::uint64_t>(slot),0,selected));
    // Manual click on program 4 increments epoch, invalidating a pending
    // automatic jump to program 2 from the PREVIOUS epoch.
    const auto stale=PnordeProgramRequests::pack(73,2);
    assert(!PnordeProgramRequests::unpack(stale,74,selected));
    const auto fresh=PnordeProgramRequests::pack(74,4);
    assert(PnordeProgramRequests::unpack(fresh,74,selected) && selected==3);
    std::cout << "PASS v0.6.17 PROGRAM request arbitration\n";
}
