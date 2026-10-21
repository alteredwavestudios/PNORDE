#include "../Source/PnordeProjectVersion.h"
#include <cassert>
int main()
{
    using namespace PnordeProjectVersion;
    static_assert(schema == 3 && build == 621);
    assert(restoredScale(false, 1) == legacyFreeScale);
    assert(restoredScale(false, 13) == legacyFreeScale);
    assert(restoredScale(true, 1) == newInstanceMajorScale);
    assert(restoredScale(true, 13) == legacyFreeScale);
    assert(restoredScale(true, 39) == 39);
}
