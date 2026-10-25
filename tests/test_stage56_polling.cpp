// Small safety check: keep the v0.6.29 display metadata and 120-Hz timer
// constant as intended, without touching previous saved project schemas.
#include "PnordeProgramPolling.h"
#include "PnordeProjectVersion.h"
#include <cassert>
#include <cstring>
#include <iostream>

int main()
{
    static_assert(PnordeProgramPolling::rateHz == 120);
    static_assert(PnordeProjectVersion::schema == 3);
    static_assert(PnordeProjectVersion::appBuild == 629);
    static_assert(PnordeProjectVersion::build == 625);
    assert(std::strcmp(PnordeProjectVersion::displayVersion, "0.6.29") == 0);
    assert(std::strcmp(PnordeProjectVersion::release, "0.6.29-test") == 0);
    std::cout << "PASS: v0.6.29 120-Hz poll constant, unchanged saved-state build/schema, version metadata\n";
}
