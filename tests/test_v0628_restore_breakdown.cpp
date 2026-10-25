// PNORDE v0.6.28: developer timing breakdown test (no JUCE required).
#include "PnordeDeveloperStats.h"
#include <iostream>
#include <cstdlib>
#define CHECK(x) do { if (!(x)) { std::cerr << "FAIL line " << __LINE__ << ": " << #x << "\n"; return EXIT_FAILURE; } } while (false)
int main()
{
    PnordeDeveloperStats::Counters c;
    c.recordRestoreBreakdown(5000, 3000, 9000); // disabled => no counting
    CHECK(c.snapshot().lastCaptureUs == 0);
    c.setEnabled(true);
    c.recordRestoreBreakdown(2000, 3100, 7000);
    c.recordParameterReplace(1800);
    c.recordSwitch(0, 2, 1, 4400, 12400, true);
    auto s = c.snapshot();
    CHECK(s.lastCaptureUs == 2000 && s.lastPrepareUs == 3100 && s.lastLoadUs == 7000);
    CHECK(s.lastRestoreUs == 12400 && s.lastQueueUs == 4400);
    CHECK(s.lastParameterReplaceUs == 1800);
    CHECK(s.programSwitches == 1);
    c.reset();
    s = c.snapshot();
    CHECK(s.lastCaptureUs == 0 && s.lastPrepareUs == 0 && s.lastLoadUs == 0);
    CHECK(s.lastParameterReplaceUs == 0);
    CHECK(s.programSwitches == 0);
    c.setEnabled(false);
    c.recordRestoreBreakdown(700, 800, 900);
    CHECK(c.snapshot().lastCaptureUs == 0);
    std::cout << "PASS: detailed restore phase counters, disabled mode, reset\n";
}
