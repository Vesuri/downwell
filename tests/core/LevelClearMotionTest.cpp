#include "core/LevelClearMotion.h"
#include "core/LevelClearCarousel.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <initializer_list>
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
static double asDouble(downwell::HudReal real) {
    std::uint64_t bits = real.mantissa ?
        (std::uint64_t(real.exponent + 1023) << 52) | (real.mantissa & 0xfffffffffffffULL) : 0;
    double result; std::memcpy(&result, &bits, sizeof result); return result;
}
int main() {
    using downwell::HudReal;
    std::uint32_t state = 1234;
    for (unsigned test = 0; test < 20000; ++test) {
        state = state * 1664525u + 1013904223u;
        const auto high = state & 0xfffffu;
        state = state * 1664525u + 1013904223u;
        HudReal value{(std::uint64_t{1} << 52) | (std::uint64_t(high) << 32) | state,
            static_cast<int>(test % 41) - 20};
        const unsigned divisor = 1 + test % 32;
        require(asDouble(value.divideSmall(divisor)) == asDouble(value) / divisor,
            "integer-only small division matches correctly rounded binary64");
        require(asDouble(value.timesSmall(divisor)) == asDouble(value) * divisor,
            "integer-only small multiplication matches correctly rounded binary64");
    }
    downwell::LevelClearMotion motion;
    double player = 0, target = 160, camera = 142, cameraTarget = 142, acceleration = 0;
    unsigned divisor = 20;
    for (unsigned tick = 1; tick <= 10000; ++tick) {
        if (tick == 60) {
            motion.showChoices(); target = 302; cameraTarget = 340; divisor = 10;
        }
        const bool departing = tick >= 92;
        motion.step(departing);
        if (departing) { target += acceleration; acceleration += 0.2; }
        player += (target - player) / divisor;
        camera += (cameraTarget - camera) / 10;
        require(asDouble(motion.playerY) == player && asDouble(motion.playerTarget) == target &&
            asDouble(motion.cameraY) == camera && asDouble(motion.acceleration) == acceleration,
            "Result player/camera easing and acceleration match binary64 on every update");
        require(motion.viewY() == static_cast<int>(std::floor(camera + 0.5)) - 142,
            "Result view derives from rounded camera instance");
    }
    require(asDouble(downwell::LevelClearMotion::approach(HudReal::integer(300), HudReal::integer(100), 10)) == 280,
        "downward target correction retains signed source arithmetic");
    // Exercise the complete stable tail, early/late choice entry, departure
    // from every part of the sequence, and mutation of each admitted input.
    for(unsigned shown : {1u,59u,60u,61u,120u})
      for(unsigned departed : {1u,59u,60u,92u,400u,1400u})
        for(unsigned mutation=0;mutation<7;++mutation){
            downwell::LevelClearMotion candidate;
            double py=0,cy=142,pt=160,ct=142,ac=0;unsigned div=20;
            for(unsigned tick=1;tick<=1500;++tick){
                if(tick==shown){candidate.showChoices();pt=302;ct=340;div=10;}
                if(tick==150){
                    switch(mutation){
                    case 1:candidate.playerY=HudReal::integer(200);py=200;break;
                    case 2:candidate.cameraY=HudReal::integer(200);cy=200;break;
                    case 3:candidate.playerTarget=HudReal::integer(100);pt=100;break;
                    case 4:candidate.cameraTarget=HudReal::integer(100);ct=100;break;
                    case 5:candidate.playerDivisor=7;div=7;break;
                    case 6:candidate.acceleration=HudReal::integer(1);ac=1;break;
                    }
                }
                const bool leaving=tick>=departed;
                candidate.step(leaving);
                if(leaving){pt+=ac;ac+=0.2;}
                py+=(pt-py)/div;cy+=(ct-cy)/10;
                require(asDouble(candidate.playerY)==py&&asDouble(candidate.cameraY)==cy&&
                    asDouble(candidate.playerTarget)==pt&&asDouble(candidate.acceleration)==ac,
                    "prepared motion and all fallback boundaries remain binary64 exact");
            }
        }
    downwell::LevelClearCarousel carousel;
    carousel.create(1);
    double x = 42;
    for (unsigned tick = 0; tick < 20000; ++tick) {
        const unsigned cursor = (tick / 127) % 4;
        carousel.step(cursor);
        x += ((80 - 38 * int(cursor)) - x) / 5;
        const auto real = carousel.position;
        require((real.negative ? -1 : 1) * asDouble(real.magnitude) == x,
            "signed carousel easing matches binary64 across zero and reversals");
        for (unsigned index = 0; index < 4; ++index) {
            const auto icon = carousel.optionX(index);
            require((icon.negative ? -1 : 1) * asDouble(icon.magnitude) == x + 38 * index,
                "option coordinates retain source addition rounding");
        }
    }
    {
        // HudReal::roundEven against nearbyint for every exponent in reach of
        // int32, with ties, neighbours and random mantissas.
        std::uint64_t state = 0x9e3779b97f4a7c15ULL; unsigned checked = 0;
        for (int exponent = -60; exponent <= 34; ++exponent)
            for (unsigned n = 0; n < 4000; ++n) {
                state ^= state << 13; state ^= state >> 7; state ^= state << 17;
                std::uint64_t m = (state & 0xfffffffffffffULL) | (1ULL << 52);
                const int shift = 52 - exponent;
                if (n < 64 && shift > 0 && shift < 53) {
                    const std::uint64_t half = 1ULL << (shift - 1), ulp = 1ULL << shift;
                    m = ((m & ~(ulp - 1)) | half) + (std::uint64_t(n & 3) - 1) * (n & 4 ? 1 : 0);
                    if (m < (1ULL << 52) || m >= (1ULL << 53)) continue;
                }
                const downwell::HudReal value{m, exponent};
                const double real = std::ldexp(double(m), exponent - 52);
                const double rounded = std::nearbyint(real);
                std::int32_t y = 7; const bool ok = value.roundEven(y);
                const bool fits = rounded <= 2147483647.0;
                require(ok == fits && (!fits || y == std::int32_t(rounded)),
                    "HudReal round matches binary64 round ties-to-even");
                ++checked;
            }
        std::int32_t y = 7;
        require(downwell::HudReal{}.roundEven(y) && y == 0, "zero rounds to zero");
        std::printf("%u HudReal rounds match binary64 nearbyint\n", checked);
    }
    std::puts("20000 divisions and 10000 level-clear motion updates match binary64");
    std::puts("20000 signed carousel updates and option coordinates match binary64");
    std::puts("315000 prepared/fallback motion updates match binary64, including stable tails and mutated inputs");
}
