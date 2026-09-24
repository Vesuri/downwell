#include "core/AmmoFill.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
static double real(downwell::HudReal value) {
    return std::ldexp(static_cast<double>(value.mantissa), value.exponent - 52);
}
static void check(unsigned ammo, unsigned cap) {
    // VM stores stammoMeter before the later neg/add instructions. Do not
    // let the host compiler contract those operations into a fused multiply-add.
    volatile double meter = cap ? 184.0 * (static_cast<double>(ammo) / cap) : 0;
    const double scale = ammo && cap ? 5.0 - meter : 0;
    const double top = 232.0 + (scale < 0 ? scale : 0);
    const double height = std::abs(scale);
    const auto fill = downwell::AmmoFill::geometry(ammo, cap);
    if (real(fill.top) != top || real(fill.height) != height) {
        std::fprintf(stderr, "fill geometry mismatch %u/%u top %.17g/%.17g height %.17g/%.17g\n", ammo, cap,
                     real(fill.top), top, real(fill.height), height);
        std::abort();
    }
    unsigned calls = 0;
    downwell::drawAmmoFill(ammo, cap, 289,
        [&](int x, unsigned y, unsigned w, unsigned h) {
            ++calls;
            if (x != 289 || y != std::floor(top) || w != 10 || h != std::floor(height))
                std::abort();
        });
    if (calls != (height >= 1 ? 1u : 0u)) std::abort();
}
int main() {
    std::uint32_t random = 1;
    for (unsigned i = 0; i < 100000; ++i) {
        random = random * 1664525u + 1013904223u;
        const auto integer = downwell::HudReal::integer(random);
        if (real(integer) != double(random) || integer.floor() != random) std::abort();
        const unsigned denominator = 1u << (i%32);
        if (real(downwell::HudReal::ratio(random, denominator)) != double(random)/denominator)
            std::abort();
    }
    check(0, 0);
    for (unsigned cap = 1; cap <= 512; ++cap)
        for (unsigned ammo = 0; ammo <= cap; ++ammo) check(ammo, cap);
    // Large capacities, reversed positive scale, exact zero scale, full charge.
    for (unsigned cap : {1000u, 0x7fffffffu, 0xffffffffu}) {
        check(1, cap); check(cap / 2, cap); check(cap, cap);
    }
    check(5, 184);
    std::puts("Ammo fill geometry and integer rectangle policy passed");
}
