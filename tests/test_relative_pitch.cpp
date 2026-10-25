// Standalone C++17 smoke tests for PNORDE's reversible pitch system.
// No JUCE/Ableton installation required to test these mathematical helpers.
#include "../Source/PnordeRelativePitch.h"
#include "../Source/PnordeProgramRequests.h"
#include <cassert>
#include <cmath>
#include <iostream>

static void near(double actual, double wanted)
{
    if (std::abs(actual - wanted) > 1.0e-7)
    {
        std::cerr << "Expected " << wanted << " but received " << actual << "\n";
        std::abort();
    }
}

int main()
{
    using namespace PnordeRelativePitch;
    constexpr int major = 1, minor = 2, freeScale = 13;
    constexpr int C = 0, D = 2;

    // Musical key must move the ROOT by exactly two semitones (C -> D).
    near(outputSemitones(0, C, major), 0.0);
    near(outputSemitones(0, D, major), 2.0);
    near(outputSemitones(40, C, major), 4.0); // E
    near(outputSemitones(40, D, major), 6.0); // F sharp

    // IMPORTANT: this is the number shown inside a PITCH lane cell.
    // KEY changes must not alter it; only SCALE can affect the display.
    // This stays true for EVERY available scale, including EDO scales.
    for (int mode = 0; mode < PnordeScales::count; ++mode)
        for (int pitch = -120; pitch <= 120; pitch += 15)
        {
            const double displayed = displaySemitones(pitch, mode);
            for (int key = 0; key < 12; ++key)
                near(outputSemitones(pitch, key, mode), key + displayed);
        }
    near(displaySemitones(40, major), 4.0); // shows +4.0 in C and D
    near(displaySemitones(40, minor), 3.0); // scale change shows +3.0
    near(displaySemitones(40, major), 4.0); // switching back restores +4.0
    near(displaySemitones(10, 0), 1.0);    // Chromatic: +1 is preserved

    // Quantisation is nearest-note, choosing LOWER on exact ties.
    near(outputSemitones(40, C, minor), 3.0); // E -> E flat
    near(outputSemitones(110, C, minor), 10.0); // B -> B flat
    near(outputSemitones(40, D, minor), 5.0); // F

    // Caller retains its original PITCH cell: switching scales back works.
    const int storedRaw = 40;
    near(outputSemitones(storedRaw, C, major), 4.0);
    near(outputSemitones(storedRaw, C, minor), 3.0);
    near(outputSemitones(storedRaw, C, major), 4.0);
    assert(storedRaw == 40);

    // Free and chromatic do not quantise, but still transpose with Key.
    near(outputSemitones(15, D, freeScale), 3.5);
    near(outputSemitones(-20, D, 0), 0.0);

    // All extended scales must return finite values; EDO preserves fractions.
    for (int mode = 0; mode < PnordeScales::count; ++mode)
        for (int pitch = -120; pitch <= 120; pitch += 15)
            assert(std::isfinite(outputSemitones(pitch, D, mode)));
    near(outputSemitones(0, D, PnordeEDO::firstScaleIndex), 2.0);

    // PROGRAM requests generated before a manual choice must be invalidated.
    int selected = -1;
    const auto req = PnordeProgramRequests::pack(17, 3);
    assert(PnordeProgramRequests::unpack(req, 17, selected) && selected == 2);
    assert(!PnordeProgramRequests::unpack(req, 18, selected));

    std::cout << "All reversible-pitch and program-request unit tests passed.\n";
}
