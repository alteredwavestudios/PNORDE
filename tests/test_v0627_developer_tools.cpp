// Standalone test (no JUCE) for Developer Tools counters.
// CHECK does not disappear in a Windows Release build (unlike assert()).
#include "PnordeDeveloperStats.h"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "FAILED " << __LINE__ << ": " << #x << '\n'; return EXIT_FAILURE; } } while (false)
int main()
{
    PnordeDeveloperStats::Counters counters;
    counters.recordAudioBlock(100, 3);
    counters.recordRequest(false);
    CHECK(counters.snapshot().audioBlocks == 0); // OFF means zero counted events
    counters.setEnabled(true);
    counters.recordAudioBlock(100, 3);
    counters.recordAudioBlock(250, 5);
    counters.recordRequest(false);
    counters.recordRequest(true);
    counters.recordSwitch(0, 2, 1, 8000, 2400, true);
    counters.recordSwitch(2, 1, 2, 0, 1100, false);
    const auto s = counters.snapshot();
    CHECK(s.audioBlocks == 2 && s.audioTotalUs == 350 && s.audioPeakUs == 250);
    CHECK(s.midiMessages == 8 && s.programRequests == 2);
    CHECK(s.overwrittenRequests == 1 && s.programSwitches == 1);
    CHECK(s.programRestoreFailures == 1 && s.lastFrom == 2 && s.lastTo == 1);
    CHECK(s.lastQueueUs == 0 && s.lastRestoreUs == 1100 && s.lastSource == 2);
    counters.reset();
    CHECK(counters.snapshot().programSwitches == 0);
    counters.setEnabled(false);
    counters.recordAudioBlock(150, 2);
    CHECK(counters.snapshot().audioBlocks == 0);
    std::cout << "PASS: opt-in counters, MIDI counts, switch durations, reset, disabled mode\n";
    return EXIT_SUCCESS;
}
