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
    std::puts("20000 divisions and 10000 level-clear motion updates match binary64");
    std::puts("20000 signed carousel updates and option coordinates match binary64");
    std::puts("315000 prepared/fallback motion updates match binary64, including stable tails and mutated inputs");
}
