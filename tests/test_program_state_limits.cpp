// PNORDE STAGE 1 - TESTS THAT DO NOT NEED JUCE / ABLETON
//
// A "test" is a tiny program which deliberately checks that our code returns
// the expected answers. A nonzero exit code means something is wrong.
//
// Compile on Windows (Developer PowerShell) or with any C++17 compiler.
// These tests do NOT prove a whole VST3 can load: use the optional JUCE
// integration test and the Ableton checklist too.

#include "../Source/PnordeStateLimits.h"
#include "../Source/PnordeProgramRequests.h"
#include <cassert>
#include <cstdint>
#include <iostream>

int main()
{
    using namespace PnordeStateLimits;

    // 151 is the current v0.6.25 number of host-visible parameters.
    assert(allowedChildren(151) == 183);
    assert(acceptsChildren(151, 151));
    assert(acceptsChildren(160, 151)); // preserve the old valid maximum
    assert(!acceptsChildren(184, 151));

    // THE REGRESSION CASE. v0.6.26 introduced 22 parameters (173 total).
    // Its saved state could not fit the old fixed 160-child validator.
    assert(allowedChildren(173) == 205);
    assert(acceptsChildren(173, 173));
    assert(allowedXmlTags(173) >= 2 * 173 + 2);
    assert(!acceptsChildren(206, 173));

    // Adding more parameters must not require hand-editing a magic 160.
    assert(acceptsChildren(250, 250));
    assert(allowedXmlTags(250) >= 2 * 250 + 2);

    // We still reject invalid counts and keep a strict absolute ceiling.
    assert(!acceptsChildren(-1, 151));
    assert(!acceptsChildren(10000, 151));
    assert(!acceptsChildren(absoluteChildMaximum + 1, 9999));
    assert(hasExpansionRoom(151));
    assert(!hasExpansionRoom(400));

    // A PROGRAM step sends its requested scene number (1..4) in a tiny
    // integer "mailbox". The epoch prevents an OLD request overriding a
    // newer manual action. These checks protect its existing semantics.
    int target = -1;
    const std::uint32_t epoch = 42;
    for (int oneBased = 1; oneBased <= 4; ++oneBased)
    {
        const std::uint64_t packet = PnordeProgramRequests::pack(epoch, oneBased);
        assert(packet != 0);
        assert(PnordeProgramRequests::unpack(packet, epoch, target));
        assert(target == oneBased - 1);
        assert(!PnordeProgramRequests::unpack(packet, epoch + 1, target));
    }
    assert(PnordeProgramRequests::pack(epoch, 0) == 0);
    assert(PnordeProgramRequests::pack(epoch, 5) == 0);
    assert(!PnordeProgramRequests::unpack(0, epoch, target));

    std::cout << "PASS: dynamic state limits, v0.6.26 regression scenario, "
                 "four PROGRAM request slots and stale-request rejection\n";
    return 0;
}
