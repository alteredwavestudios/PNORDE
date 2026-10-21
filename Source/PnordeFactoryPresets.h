#pragma once
#include "PnordePatternLibrary.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Pure fixed-size factory recipes. No file access, heap use, or callbacks.
// The editor applies the recipe on the message thread; host saves full state.
namespace PnordeFactoryPresets
{
struct Preset
{
    const char* category;
    const char* name;
    int gateRecipe, velocityRecipe, panRecipe, slideRecipe;
    int pitchSpan, clockDivision, panDivision;
};
constexpr std::array<Preset, 12> presets {{
    {"Ambient", "Still Water",       8,2,0,0, 3,4,5},
    {"Ambient", "Orbiting Memory",   8,3,1,2, 5,3,6},
    {"Ambient", "Distant Lighthouse", 9,2,2,1, 4,5,6},
    {"Ambient", "Breathing Clouds",  8,3,3,3, 3,4,5},
    {"Dance", "Trance Horizon",      0,0,4,5, 7,2,3},
    {"Dance", "Progressive Motion",  2,4,5,4, 6,2,3},
    {"Dance", "House Sidechain",     3,1,6,5, 5,2,3},
    {"Dance", "Garage Rebound",      4,5,7,6, 7,2,2},
    {"Experimental", "Polymetric Mirrors", 10,7,8,7, 9,3,4},
    {"Experimental", "Impossible Spiral",  6,6,9,8, 9,1,2},
    {"Generative", "Bittersweet Drift", 8,2,10,9, 6,4,5},
    {"Generative", "Nocturne Steps",  9,8,11,2, 5,3,4}
}};
inline void pitch(std::array<int,128>& out, int length, int span, std::uint32_t seed) noexcept
{
    length=std::max(1,std::min(128,length));
    const int motif[] = {0,2,5,3,7,5,2,-2,0,4,7,5,9,7,4,2};
    for (int i=0; i<length; ++i)
    {
        const int jitter = static_cast<int>(PnordePatternLibrary::hash(seed+static_cast<std::uint32_t>(i/16+29))%3)-1;
        out[static_cast<std::size_t>(i)] = std::max(-120,std::min(120,
            (motif[i%16]*span/7+jitter)*10));
    }
}
}
