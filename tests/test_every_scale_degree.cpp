// These tests deliberately do NOT use JUCE or Ableton.
// They check that the available notes in every scale are complete above and
// below the tonic, that no adjacent degree is duplicated or skipped, and that
// the playback quantiser agrees with the editor within 0.1-semitone precision.
#include "../Source/PnordeScaleDegreeEdit.h"
#include "../Source/PnordeRelativePitch.h"
#include <array>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace E = PnordeScaleDegreeEdit;
namespace R = PnordeRelativePitch;

// Explicit independent expectations for the original scales.
// These numbers are semitones UP from C within a single octave.
// Bebop preserves the 9-note interval selection of existing PNORDE projects;
// changing that would be a separate music/preset compatibility decision.
static const std::vector<std::vector<int>> legacyDegrees = {
    {0,1,2,3,4,5,6,7,8,9,10,11}, // chromatic
    {0,2,4,5,7,9,11}, // major
    {0,2,3,5,7,8,10}, // minor
    {0,2,3,5,7,9,10}, // dorian
    {0,1,3,5,7,8,10}, // phrygian
    {0,1,3,5,6,8,10}, // locrian
    {0,3,5,6,7,10}, // blues
    {0,2,3,4,7,9}, // major blues
    {0,2,3,4,5,7,9,10,11}, // existing bebop definition
    {0,2,3,5,7,8,11}, // harmonic minor
    {0,1,3,7,8}, // pelog approximation
    {0,2,3,5,7,9,11}, // ascending melodic minor
    {0,2,4,5,7,9,10} // mixolydian
};

int main()
{
    int total = 0;
    for (int scale = 0; scale < PnordeScales::count; ++scale)
    {
        const auto allowed = E::buildAllowedPitches(scale);
        assert(allowed.count > 0 && allowed.count <= E::maximumEntries);
        assert(allowed.values[0] == -120);
        assert(allowed.values[allowed.count - 1] == 120);
        std::vector<int> actual(allowed.values.begin(), allowed.values.begin() + allowed.count);
        for (int i = 0; i < allowed.count; ++i)
        {
            const int v = actual[static_cast<std::size_t>(i)];
            assert(v >= -120 && v <= 120);
            if (i) assert(actual[static_cast<std::size_t>(i-1)] < v);
            const double snapped = R::displaySemitones(v, scale);
            // EDO values may have been rounded to the closest 0.1 semitone.
            assert(std::abs(v / 10.0 - snapped) <= 0.050001);
            // Shifting by +/-12 semitones must be another legal note unless
            // the shift would leave the permitted pitch lane range.
            if (v + 120 <= 120)
                assert(std::binary_search(actual.begin(), actual.end(), v + 120));
            if (v - 120 >= -120)
                assert(std::binary_search(actual.begin(), actual.end(), v - 120));
        }
        if (scale <= 12)
        {
            std::vector<int> expected;
            for (int octave = -1; octave <= 1; ++octave)
                for (const int degree : legacyDegrees[static_cast<std::size_t>(scale)])
                {
                    const int raw = 10 * (12 * octave + degree);
                    if (raw >= -120 && raw <= 120) expected.push_back(raw);
                }
            std::sort(expected.begin(), expected.end());
            expected.erase(std::unique(expected.begin(), expected.end()), expected.end());
            assert(expected == actual);
        }
        if (scale >= 14 && scale < PnordeEDO::firstScaleIndex)
        {
            const auto& shape = PnordeScales::extras[static_cast<size_t>(scale - 14)];
            for (int octave = -1; octave <= 0; ++octave)
                for (int d = 0; d < shape.size; ++d)
                {
                    int v = 120 * octave + shape.offsetsTenths[static_cast<size_t>(d)];
                    assert(std::binary_search(actual.begin(), actual.end(), v));
                }
        }
        if (PnordeEDO::isMode(scale))
        {
            const auto& mode = PnordeEDO::mode(scale);
            for (int octave = -1; octave <= 0; ++octave)
                for (int degree : mode.degrees)
                {
                    int v = static_cast<int>(std::lround(120 * octave + 120.0 * degree / mode.edo));
                    assert(std::binary_search(actual.begin(), actual.end(), v));
                }
        }
        // Repeated key changes affect MIDI output, not the display list.
        for (int raw : {-120,-90,-40,0,40,90,120})
            assert(std::abs((R::outputSemitones(raw, 2, scale) -
                             R::outputSemitones(raw, 0, scale)) - 2.0) < 1e-9);
        ++total;
    }
    // THE REPORTED ISSUE: Major must include the sixth (+9=A), and every
    // negative note must belong to the same C-major scale, down an octave.
    auto major = E::buildAllowedPitches(1);
    const std::vector<int> aroundTonic {-120,-100,-80,-70,-50,-30,-10,0,20,40,50,70,90,110,120};
    assert(std::vector<int>(major.values.begin(), major.values.begin() + major.count) == aroundTonic);
    assert(major.values[static_cast<std::size_t>(E::nearestIndex(major, 70) + 1)] == 90);
    assert(major.values[static_cast<std::size_t>(E::nearestIndex(major, 0) - 1)] == -10);
    std::cout << "PASSED: all " << total << " scales; positive/negative octaves, "
              << "all degrees, sixth, keys, EDO and editable bounds.\n";
}
