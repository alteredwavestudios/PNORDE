#include "../Source/PnordeSafeCC.h"
#include "../Source/PnordeLane.h"
#include <cassert>
#include <iostream>
#include <set>

static_assert(PnordeSafeCC::allowed(20), "Start of CC whitelist");
static_assert(PnordeSafeCC::allowed(31), "End of CC whitelist");
static_assert(!PnordeSafeCC::allowed(0), "Bank select excluded");
static_assert(!PnordeSafeCC::allowed(6), "Data entry excluded");
static_assert(!PnordeSafeCC::allowed(10), "Pan is a dedicated lane");
static_assert(!PnordeSafeCC::allowed(64), "Sustain is a dedicated lane");
static_assert(!PnordeSafeCC::allowed(98), "NRPN excluded");
static_assert(!PnordeSafeCC::allowed(123), "Channel mode excluded");
static_assert(PnordeSafeCC::sanitise(-1) == 20, "Invalid CC sanitised");
static_assert(PnordeSafeCC::sanitise(127) == 20, "Invalid CC sanitised");
static_assert(PnordeSafeCC::sanitise(27) == 27, "Allowed CC unchanged");
static_assert(PnordeLane::firstRow == 7 && PnordeLane::rowCount == 14, "Slot layout unchanged");

int main()
{
    int count = 0;
    for (int n = -512; n <= 512; ++n)
    {
        const bool safe = n >= 20 && n <= 31;
        assert(PnordeSafeCC::allowed(n) == safe);
        assert(PnordeSafeCC::allowed(PnordeSafeCC::sanitise(n)));
        count += PnordeSafeCC::allowed(n);
    }
    assert(count == 12);
    std::set<int> dedicatedDestinations;
    for (int i = 0; i < 14; ++i)
    {
        const auto row = PnordeLane::firstRow + i;
        const auto kind = PnordeLane::defaultType(row);
        if (i < 8) assert(kind == PnordeLane::Type::ccMacro);
        if (i >= 8) assert(kind != PnordeLane::Type::ccMacro);
        if (PnordeLane::unique(kind))
        {
            const auto control = PnordeLane::cc(kind);
            if (control >= 0) assert(dedicatedDestinations.insert(control).second);
        }
    }
    std::cout << "PASS: safe CC range, legacy fallbacks, and stable lane metadata\n";
}
