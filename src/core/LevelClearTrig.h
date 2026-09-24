#ifndef DOWNWELL_CORE_LEVEL_CLEAR_TRIG_H
#define DOWNWELL_CORE_LEVEL_CLEAR_TRIG_H
#include "HudReal.h"
namespace downwell {
// Integer presentation sampler for Result Draw's abs(cos(circle degrees)) and
// abs(sin(time/20 radians)). This is a bounded approximation, NOT binary64/libm
// identity. Keep the host calculation as an independent fidelity comparison.
// Angles use unsigned Q32 turns; amplitudes use Q30. No 64-bit multiply/divide.
struct LevelClearTrig {
    static std::uint32_t productHigh(std::uint32_t a, std::uint32_t b) {
        const std::uint32_t low = (a & 65535u) * (b & 65535u);
        const std::uint32_t middle = (a >> 16) * (b & 65535u) + (low >> 16);
        const std::uint32_t cross = (a & 65535u) * (b >> 16) + (middle & 65535u);
        return (a >> 16) * (b >> 16) + (middle >> 16) + (cross >> 16);
    }
    static std::uint32_t radianPhase(std::uint32_t time) {
        // floor(2^64/(40*pi)) = 0x020984d7c760d9dd. Multiplying the
        // absolute clock avoids accumulating a rounded per-frame phase step.
        return productHigh(time, 0xc760d9ddu) + time * 0x020984d7u;
    }
    static std::uint32_t degreePhase(std::uint32_t degrees) {
        unsigned remainder = degrees % 360u;
        std::uint32_t phase = 0;
        for (unsigned bit = 0; bit < 32; ++bit) {
            remainder <<= 1; phase <<= 1;
            if (remainder >= 360) { remainder -= 360; ++phase; }
        }
        return phase;
    }
    static std::int32_t shift(std::int32_t value, unsigned bits) {
        // Defined floor division even on hosts with a non-arithmetic >>.
        return value >= 0 ? std::int32_t(std::uint32_t(value) >> bits) :
            -std::int32_t((std::uint32_t(-value) + ((1u << bits) - 1)) >> bits);
    }
    static unsigned sineQ30(std::uint32_t phase) {
        const std::uint32_t quarter = 0x40000000u;
        std::uint32_t angle = phase & (quarter - 1);
        if (phase & quarter) angle = quarter - angle;
        if (!angle) return 0;
        if (angle == quarter) return quarter;
        // round(atan(2^-i)/(2*pi)*2^32), i=0..30.
        static const std::uint32_t angles[] = {
            0x20000000,0x12e4051e,0x09fb385b,0x051111d4,0x028b0d43,0x0145d7e1,
            0x00a2f61e,0x00517c55,0x0028be53,0x00145f2f,0x000a2f98,0x000517cc,
            0x00028be6,0x000145f3,0x0000a2fa,0x0000517d,0x000028be,0x0000145f,
            0x00000a30,0x00000518,0x0000028c,0x00000146,0x000000a3,0x00000051,
            0x00000029,0x00000014,0x0000000a,0x00000005,0x00000003,0x00000001,0x00000001
        };
        std::int32_t x = 652032874; // round(CORDIC inverse gain * 2^30)
        std::int32_t y = 0, z = angle;
        for (unsigned i = 0; i < 31; ++i) {
            const auto dx = shift(x, i), dy = shift(y, i);
            if (z >= 0) { x -= dy; y += dx; z -= std::int32_t(angles[i]); }
            else { x += dy; y -= dx; z += std::int32_t(angles[i]); }
        }
        return y <= 0 ? 0 : std::uint32_t(y) > quarter ? quarter : unsigned(y);
    }
    static HudReal amplitude(unsigned q30) {
        auto value = HudReal::integer(q30);
        if (value.mantissa) value.exponent -= 30;
        return value;
    }
    static HudReal absoluteCosine(std::uint32_t circle) {
        return amplitude(sineQ30(degreePhase(circle) + 0x40000000u));
    }
    static HudReal absoluteSine(std::uint32_t time) {
        return amplitude(sineQ30(radianPhase(time)));
    }
};
}
#endif
