// Stage 4 GUI-view rules -- testable without JUCE or Ableton.
// CHECK is never disabled in Release (unlike assert()).
#include "../Source/PnordeUIViewport.h"
#include "../Source/PnordeProjectVersion.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#define CHECK(x) do { if (!(x)) { std::cerr << "FAIL line " << __LINE__ << ": " << #x << "\n"; return EXIT_FAILURE; } } while (false)

int main()
{
    using namespace PnordeUIViewport;
    CHECK(std::string(PnordeProjectVersion::displayVersion) == "0.6.29");
    CHECK(PnordeProjectVersion::appBuild == 629);
    CHECK(PnordeProjectVersion::build == 625); // Persisted compatibility stays intact.
    CHECK(PnordeProjectVersion::schema == 3);
    CHECK(clampedPage(5, 128, 16) == 5);
    CHECK(clampedPage(5, 16, 16) == 0);
    CHECK(clampedPage(5, 32, 16) == 1);
    CHECK(clampedPage(-4, 128, 16) == 0);
    CHECK(clampedPage(1, 1, 16) == 0);
    CHECK(clampedPage(1, 0, 16) == 0);
    CHECK(clampedPage(1, 16, 0) == 0);
    CHECK(clampedPage(7, 128, 32) == 3);
    CHECK(clampedScroll(450.0f, 900.0f) == 450.0f);
    CHECK(clampedScroll(1200.0f, 900.0f) == 900.0f);
    CHECK(clampedScroll(-100.0f, 900.0f) == 0.0f);
    CHECK(clampedScroll(20.0f, 0.0f) == 0.0f);
    CHECK(clampedScroll(std::numeric_limits<float>::quiet_NaN(), 900.0f) == 0.0f);
    CHECK(clampedScroll(std::numeric_limits<float>::infinity(), 900.0f) == 0.0f);
    CHECK(clampedScroll(20.0f, std::numeric_limits<float>::infinity()) == 0.0f);
    std::cout << "PASS: Stage 4 display version and editor viewport clamping\n";
}
