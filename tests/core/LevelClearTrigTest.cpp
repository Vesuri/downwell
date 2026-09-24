#include "core/LevelClearTrig.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
static double real(downwell::HudReal value) {
    return std::ldexp(double(value.mantissa), value.exponent - 52);
}
int main() {
    using T = downwell::LevelClearTrig;
    std::uint32_t random = 17;
    double maxSine = 0, maxCosine = 0;
    unsigned pixelDifferences = 0;
    for (unsigned sample = 0; sample < 1000000; ++sample) {
        random = random * 1664525u + 1013904223u;
        const std::uint32_t clock = sample < 10000 ? sample : random;
        const std::uint32_t multiplier = random ^ 0xac478f13u;
        require(T::productHigh(clock, multiplier) == ((std::uint64_t(clock) * multiplier) >> 32),
            "split product matches full-width integer oracle");
        const std::uint64_t phase = std::uint64_t(clock) * 0xc760d9ddu;
        require(T::radianPhase(clock) == std::uint32_t((phase >> 32) + clock * 0x020984d7u),
            "radian phase does not accumulate per-frame rounding");
        const double s = real(T::absoluteSine(clock)), c = real(T::absoluteCosine(clock));
        require(s >= 0 && s <= 1 && c >= 0 && c <= 1, "amplitudes remain bounded");
        const double expectedSine = std::fabs(std::sin(clock / 20.0));
        const double expectedCosine = std::fabs(std::cos(clock * (3.14159265358979323846 / 180.0)));
        maxSine = std::max(maxSine, std::fabs(s - expectedSine));
        maxCosine = std::max(maxCosine, std::fabs(c - expectedCosine));
        // SDL first narrows layout coordinates to float. This is a sampled
        // floor-raster check, not a claim about the Windows GPU's rasterizer.
        for (unsigned pass = 0; pass <= 6; ++pass)
            pixelDifferences += std::floor(float(80 - pass * c)) != std::floor(float(80 - pass * expectedCosine));
        pixelDifferences += std::floor(float(354 - 6 * s)) != std::floor(float(354 - 6 * expectedSine));
    }
    require(maxSine < 8e-8 && maxCosine < 8e-8, "integer sampler meets measured absolute-error limit");
    for (unsigned degree = 0; degree < 360; ++degree)
        require(T::degreePhase(degree) == std::uint32_t((std::uint64_t(degree) << 32) / 360),
            "degree phase uses exact rational quotient");
    require(real(T::absoluteSine(0)) == 0 && real(T::absoluteCosine(0)) == 1 &&
        real(T::absoluteCosine(90)) == 0 && real(T::absoluteCosine(180)) == 1,
        "exact axes survive conversion to shared arithmetic");
    std::printf("1,000,000 animation samples; max sine error %.9g, cosine %.9g; %u floor-pixel differences\n",
        maxSine, maxCosine, pixelDifferences);
    require(pixelDifferences == 0, "sampled float-coordinate heading/bob pixels match host libm");
}
