#include "core/LevelClearMotion.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
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
    std::puts("20000 divisions and 10000 level-clear motion updates match binary64");
}
