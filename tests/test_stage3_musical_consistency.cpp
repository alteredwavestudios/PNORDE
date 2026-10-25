// PNORDE Stage 3: musical consistency WITHOUT JUCE or Ableton.
//
// WHY THIS TEST EXISTS
// --------------------
// Originally the PITCH editor and playback used one set of masks while the
// composition generator used its own handwritten lists of musical degrees.
// These disagreed for Bebop and Pelog. That meant a newly composed melody
// might LOOK like one note but SOUND as a nearby different note after the
// playback scale restriction was applied.
//
// We check every legacy scale, including negative octave positions, not just
// two examples. CHECK() is active even in an optimised Release build; unlike
// standard assert(), it cannot disappear when NDEBUG is defined.
#include "../Source/PnordeCompositionEngine.h"
#include "../Source/PnordeRelativePitch.h"
#include <cmath>
#include <iostream>
#include <cstdlib>

#define CHECK(expr) do { if (!(expr)) { \
    std::cerr << "FAIL line " << __LINE__ << ": " << #expr << "\n"; \
    return EXIT_FAILURE; } } while (false)

int main()
{
    namespace S = PnordeScales;
    namespace C = PnordeCompositionEngine;
    namespace R = PnordeRelativePitch;

    // Preserve the 13 original scale masks exactly. These are saved by
    // index in old PNORDE projects; changing them changes existing songs.
    constexpr int historicalMasks[] = {
        0xfff, 0xab5, 0x5ad, 0x6ad, 0x5ab, 0x56b, 0x4e9,
        0x29d, 0xebd, 0x9ad, 0x18b, 0xaad, 0x6b5
    };
    constexpr int historicalCounts[] = {
        12, 7, 7, 7, 7, 7, 6, 6, 9, 7, 5, 7, 7
    };
    for (int scale = 0; scale < 13; ++scale)
    {
        CHECK(S::legacyMasks[static_cast<std::size_t>(scale)] == historicalMasks[scale]);
        CHECK(C::degreeCount(scale) == historicalCounts[scale]);

        // Every generator degree must be one the playback engine accepts.
        // Check negative degrees too: -1 means last note in lower octave.
        for (int degree = -50; degree <= 50; ++degree)
        {
            const auto generated = C::degreeSemis(scale, degree);
            const auto rounded = static_cast<int>(std::lround(generated));
            CHECK(std::abs(generated - rounded) < 1.0e-8);
            const int noteClass = ((rounded % 12) + 12) % 12;
            CHECK((historicalMasks[scale] & (1 << noteClass)) != 0);

            // Consecutive degrees must stay ascending, wrapping to the next
            // octave after the scale's actual (not guessed) note count.
            CHECK(C::degreeSemis(scale, degree + 1) > generated);
            CHECK(std::abs(C::degreeSemis(scale, degree + historicalCounts[scale])
                           - generated - 12.0) < 1.0e-8);
        }

        // Check the normal generator's resulting saved cells, not merely its
        // pitch lookup function. Key transposition happens only at playback.
        for (int recipe : {0, 2, 3, 4, 5, 6})
        {
            C::Phrase melody;
            C::fill(melody, scale, 2, 96, recipe, false, 12345);
            CHECK(melody.length == 96);
            for (int i = 0; i < melody.length; ++i)
            {
                const int stored = melody.pitch[static_cast<std::size_t>(i)];
                const double beforePlayback = stored / 10.0;
                const double afterScale = R::displaySemitones(stored, scale);
                CHECK(std::abs(beforePlayback - afterScale) < 1.0e-7);
                CHECK(std::abs(R::outputSemitones(stored, 2, scale) - (2 + afterScale)) < 1.0e-7);
            }
        }
        for (int recipe = 0; recipe < C::arpCount; ++recipe)
        {
            C::Phrase arp;
            C::fill(arp, scale, 7, 96, recipe, true, 12345);
            for (int i = 0; i < arp.length; ++i)
            {
                const int stored = arp.pitch[static_cast<std::size_t>(i)];
                CHECK(std::abs(R::displaySemitones(stored, scale) - stored / 10.0) < 1.0e-7);
            }
        }
    }

    // Precise targeted checks for the two previously inconsistent modes.
    CHECK(S::legacyDegreeSemitone(8, 2) == 3); // Bebop includes Eb.
    CHECK(S::legacyDegreeSemitone(8, 8) == 11);
    CHECK(S::legacyDegreeSemitone(10, 3) == 7); // Pelog: fourth note is G.
    CHECK(S::legacyDegreeSemitone(10, 4) == 8); // Pelog: fifth note is Ab.
    CHECK(C::degreeSemis(8, 9) == 12.0); // Nine degrees per Bebop octave.
    CHECK(C::degreeSemis(10, 5) == 12.0); // Five degrees per Pelog octave.

    // Non-legacy extra and EDO scale paths must retain their old meanings.
    CHECK(C::degreeCount(13) == 12);
    CHECK(C::degreeSemis(13, -1) == -1.0);
    CHECK(C::degreeCount(14) == 5);
    CHECK(C::degreeSemis(14, 2) == 4.0); // Major Pentatonic third.
    CHECK(C::degreeCount(29) == 6);
    CHECK(C::degreeSemis(29, 4) == 8.0); // Whole tone fifth degree.
    CHECK(C::degreeCount(33) == 7);
    CHECK(C::degreeSemis(33, 1) == 1.5); // Bayati quartertone approximation.

    std::cout << "PASS: 13 legacy generator/playback scales, 2 mismatches fixed, "
                 "negative degrees, 12 generation recipes, nonlegacy examples\n";
    return EXIT_SUCCESS;
}
