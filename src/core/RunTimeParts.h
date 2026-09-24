#ifndef DOWNWELL_CORE_RUN_TIME_PARTS_H
#define DOWNWELL_CORE_RUN_TIME_PARTS_H
#include <cstdint>
namespace downwell {
// timerNumber VM blocks 2/3: binary64 divide by 60, subtract floor, multiply
// by 100, then floor. Do not replace with (ticks % 60) * 100 / 60.
// Integer emulation avoids floating point and 64-bit multiply/divide helpers.
// This models round-to-nearest binary64 intermediates, not string/draw quirks.
struct RunTimeParts {
    std::uint32_t minutes, seconds, centiseconds;
    static RunTimeParts fromTicks(std::uint32_t ticks) {
        const std::uint32_t whole = ticks / 60;
        RunTimeParts result{whole / 60, whole % 60, 0};
        if (!ticks) return result;

        // Find the binade of ticks/60 without floating point or overflow.
        int exponent = 0;
        if (ticks >= 60) {
            std::uint32_t value = whole;
            while (value >= 2) { value >>= 1; ++exponent; }
        } else {
            std::uint32_t value = ticks;
            while (value < 60) { value <<= 1; --exponent; }
        }
        const unsigned fractionalBits = static_cast<unsigned>(52 - exponent);
        std::uint32_t remainder = ticks % 60;
        std::uint64_t fraction = 0;
        for (unsigned i = 0; i < fractionalBits; ++i) {
            remainder <<= 1;
            fraction <<= 1;
            if (remainder >= 60) { remainder -= 60; fraction |= 1; }
        }
        if (remainder > 30 || (remainder == 30 && (fraction & 1))) ++fraction;
        // Subtraction of the integer part is exact. Multiplication by 100
        // has its own binary64 rounding before floor is applied.
        std::uint64_t product = (fraction << 6) + (fraction << 5) + (fraction << 2);
        std::uint64_t significand = product;
        unsigned discarded = 0;
        while (significand >= (std::uint64_t(1) << 53)) {
            significand >>= 1;
            ++discarded;
        }
        if (discarded) {
            std::uint64_t half = 1;
            for (unsigned i = 1; i < discarded; ++i) half <<= 1;
            const std::uint64_t tail = product & ((half << 1) - 1);
            if (tail > half || (tail == half && (significand & 1))) ++significand;
            product = significand;
            for (unsigned i = 0; i < discarded; ++i) product <<= 1;
        }
        for (unsigned i = 0; i < fractionalBits; ++i) product >>= 1;
        result.centiseconds = static_cast<std::uint32_t>(product);
        return result;
    }
};
} // namespace downwell
#endif
