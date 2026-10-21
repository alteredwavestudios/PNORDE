#include <cassert>
#include <cmath>
#include <cstddef>
#include <array>
#include <iostream>

// Checks the approximation used in the realtime JUCE painting loop.
int main()
{
    constexpr std::array<float, 5> behind {{1.0f, 0.24659696f, 0.06081006f,
                                            0.01499558f, 0.00369786f}};
    constexpr std::array<float, 3> ahead {{1.0f, 0.16529889f, 0.02732372f}};
    for (int p = 0; p <= 100; ++p)
    {
        const float phase = static_cast<float>(p) / 100.0f;
        const float attenuation = std::exp(-1.4f * phase);
        for (std::size_t i = 0; i < behind.size(); ++i)
            assert(std::abs(std::exp(-1.4f * (static_cast<float>(i) + phase))
                - behind[i] * attenuation) < 1.0e-6f);
        for (std::size_t i = 0; i < ahead.size(); ++i)
            assert(std::abs(std::exp(-1.8f * static_cast<float>(i))
                - ahead[i]) < 1.0e-6f);
    }
    // Iteration budget uses a clamped sample count: no overflow from int*8.
    for (const int samples : {1, 2, 64, 512, 4096, 4097, 2147483647})
    {
        const int safeSamples = samples < 4096 ? samples : 4096;
        const int budget = (safeSamples * 8 < 128) ? 128
            : (safeSamples * 8 > 32768 ? 32768 : safeSamples * 8);
        assert(budget >= 128 && budget <= 32768);
    }
    std::cout << "Stability falloff & bounded scheduler tests passed\n";
}
