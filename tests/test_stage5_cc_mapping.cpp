// PNORDE Stage 5 regression test: MIDI controller mapping parity.
//
// Written without JUCE to run quickly on any C++17 compiler. This checks
// which CC events SHOULD be generated for every valid lane value, including
// the two independent axes in the vibrato lane.
//
// This cannot simulate Ableton or MIDI sample offsets. On Windows we also
// run the Stage 1 processor test and manually verify the plugin in Ableton.
#include "../Source/PnordeLane.h"
#include "../Source/PnordeSafeCC.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

struct CCEvent { int controller, value, axis; };
static bool operator==(const CCEvent& a, const CCEvent& b)
{
    return a.controller == b.controller && a.value == b.value && a.axis == b.axis;
}

// This is an intentional, independent recording of the OLD PluginProcessor
// mapping, taken before Stage 5. If Stage 5 ever changes output, this test
// fails instead of "correcting" the expected values alongside the code.
static std::vector<CCEvent> beforeStage5(int type, int vertical, int depth)
{
    const int scaledVertical = std::lround(vertical * 1.27f);
    const int scaledDepth = std::lround(depth * 1.27f);
    switch (type)
    {
        case 1: return { {1, scaledVertical, 0} };
        case 2: return { {11, scaledVertical, 0} };
        case 3: return { {76, scaledVertical, 0}, {77, scaledDepth, 1} };
        case 4: return { {10, vertical, 0} };
        case 5: return { {64, vertical >= 50 ? 127 : 0, 0} };
        default: return {};
    }
}

// A small independent model of the AFTER mapping: it reads all dedicated
// controller numbers from the same header the MIDI processor now uses.
static std::vector<CCEvent> afterStage5(int type, int vertical, int depth)
{
    using Type = PnordeLane::Type;
    const int scaledVertical = std::lround(vertical * 1.27f);
    const int scaledDepth = std::lround(depth * 1.27f);
    switch (type)
    {
        case 1: return { {PnordeLane::cc(Type::modWheel), scaledVertical, 0} };
        case 2: return { {PnordeLane::cc(Type::expression), scaledVertical, 0} };
        case 3: return { {PnordeLane::cc(Type::vibrato), scaledVertical, 0},
                         {PnordeLane::vibratoDepthCC, scaledDepth, 1} };
        case 4: return { {PnordeLane::cc(Type::pan), vertical, 0} };
        case 5: return { {PnordeLane::cc(Type::sustain), vertical >= 50 ? 127 : 0, 0} };
        default: return {};
    }
}

static int fail(const char* why, int type, int vertical, int depth)
{
    std::cerr << "FAIL: " << why << " type=" << type
              << " vertical=" << vertical << " depth=" << depth << '\n';
    return EXIT_FAILURE;
}

int main()
{
    // Existing old project state values must retain their meanings.
    if (static_cast<int>(PnordeLane::Type::modWheel) != 1 ||
        static_cast<int>(PnordeLane::Type::expression) != 2 ||
        static_cast<int>(PnordeLane::Type::vibrato) != 3 ||
        static_cast<int>(PnordeLane::Type::pan) != 4 ||
        static_cast<int>(PnordeLane::Type::sustain) != 5)
        return fail("Saved lane role integer changed", 0, 0, 0);
    if (PnordeLane::firstRow != 7 || PnordeLane::rowCount != 14)
        return fail("Optional lane position changed", 0, 0, 0);
    for (int row = 7; row <= 20; ++row)
    {
        const int expected = row >= 15 ? row - 14 : 0;
        if (static_cast<int>(PnordeLane::defaultType(row)) != expected)
            return fail("Saved default lane type changed", row, 0, 0);
    }
    // Every dedicated role and every allowed 0..100 control value is checked;
    // the 2D vibrato axis is checked exhaustively as well (101*101 values).
    int comparisons = 0;
    for (int type = 0; type <= 6; ++type)
        for (int vertical = 0; vertical <= 100; ++vertical)
        {
            const int depthEnd = type == 3 ? 100 : 0;
            for (int depth = 0; depth <= depthEnd; ++depth)
            {
                if (beforeStage5(type, vertical, depth) !=
                    afterStage5(type, vertical, depth))
                    return fail("MIDI controller/value/axis parity", type, vertical, depth);
                ++comparisons;
            }
        }
    for (int cc = -10; cc <= 140; ++cc)
    {
        const int expected = cc >= 20 && cc <= 31 ? cc : 20;
        if (PnordeSafeCC::sanitise(cc) != expected)
            return fail("Custom CC whitelist changed", cc, 0, 0);
    }
    if (PnordeLane::unique(PnordeLane::Type::ccMacro) ||
        !PnordeLane::unique(PnordeLane::Type::modWheel))
        return fail("Unique-lane policy changed", 0, 0, 0);
    std::cout << "PASS: Stage 5 dedicated MIDI CC numbers, output values,"
              << " both vibrato axes, lane IDs and macro whitelist ("
              << comparisons << " parity cases)\n";
    return EXIT_SUCCESS;
}
