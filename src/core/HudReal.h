#ifndef DOWNWELL_CORE_HUD_REAL_H
#define DOWNWELL_CORE_HUD_REAL_H
#include <cstdint>
namespace downwell {
// Positive, normal binary64 subset needed by the HUD's divider loops.
// No host floating point, 64-bit multiply/divide, or variable-width shifts.
// Subtraction saturates at zero: the source loop stops on any nonpositive result.
struct HudReal {
    std::uint64_t mantissa = 0;
    int exponent = 0;
    static std::uint64_t shiftJam(std::uint64_t value, unsigned count) {
        while (count--) value = (value >> 1) | (value & 1);
        return value;
    }
    static HudReal rounded(std::uint64_t extended, int exponent) {
        if (!extended) return {};
        while (extended >= (std::uint64_t{1} << 56)) {
            extended = shiftJam(extended, 1); ++exponent;
        }
        while (extended < (std::uint64_t{1} << 55)) {
            extended <<= 1; --exponent;
        }
        std::uint64_t m = extended >> 3;
        const unsigned tail = extended & 7;
        if (tail > 4 || (tail == 4 && (m & 1))) ++m;
        if (m == (std::uint64_t{1} << 53)) { m >>= 1; ++exponent; }
        return {m, exponent};
    }
    static HudReal integer(unsigned value) {
        return rounded(static_cast<std::uint64_t>(value) << 3, 52);
    }
    static HudReal ratio(unsigned numerator, unsigned denominator) {
        if (!numerator || !denominator) return {};
        std::uint64_t n = numerator, d = denominator;
        int exponent = 0;
        while (n < d) { n <<= 1; --exponent; }
        while (n >= (d << 1)) { d <<= 1; ++exponent; }
        std::uint64_t m = 1, remainder = n - d;
        for (unsigned bit = 0; bit < 52; ++bit) {
            remainder <<= 1; m <<= 1;
            if (remainder >= d) { remainder -= d; ++m; }
        }
        if (remainder > d - remainder || (remainder == d - remainder && (m & 1))) ++m;
        if (m == (std::uint64_t{1} << 53)) { m >>= 1; ++exponent; }
        return {m, exponent};
    }
    HudReal times184() const {
        const auto product = (mantissa << 7) + (mantissa << 5) +
                             (mantissa << 4) + (mantissa << 3);
        return rounded(product << 3, exponent);
    }
    HudReal timesSmall(unsigned factor) const {
        if (factor > 32) return {};
        std::uint64_t product = 0;
        for (unsigned i = 0; i < factor; ++i) product += mantissa;
        return rounded(product << 3, exponent);
    }
    // Small positive divisors used by Result's easing. Normalize before
    // division to retain three rounding bits plus a sticky remainder. The
    // bitwise quotient avoids a 64-bit divide helper on the 68000.
    HudReal divideSmall(unsigned divisor) const {
        if (!mantissa || !divisor || divisor > 32) return {};
        unsigned power = 0, ceiling = 1;
        while (ceiling < divisor) { ceiling <<= 1; ++power; }
        std::uint64_t dividend = mantissa << 3, mask = std::uint64_t{1} << 55;
        for (unsigned i = 0; i < power; ++i) { dividend <<= 1; mask <<= 1; }
        std::uint64_t quotient = 0;
        unsigned remainder = 0;
        for (unsigned i = 0; i < 56 + power; ++i) {
            remainder = (remainder << 1) | ((dividend & mask) ? 1u : 0u);
            dividend <<= 1; quotient <<= 1;
            if (remainder >= divisor) { remainder -= divisor; quotient |= 1; }
        }
        return rounded(quotient | (remainder ? 1u : 0u), exponent - static_cast<int>(power));
    }
    bool less(HudReal other) const {
        if (!mantissa) return other.mantissa != 0;
        if (!other.mantissa) return false;
        return exponent != other.exponent ? exponent < other.exponent : mantissa < other.mantissa;
    }
    bool equal(HudReal other) const {
        return mantissa == other.mantissa && (!mantissa || exponent == other.exponent);
    }
    HudReal subtract(HudReal other) const {
        if (!other.mantissa) return *this;
        if (less(other) || equal(other)) return {};
        return rounded((mantissa << 3) - shiftJam(other.mantissa << 3,
            static_cast<unsigned>(exponent - other.exponent)), exponent);
    }
    HudReal add(HudReal other) const {
        if (!mantissa) return other;
        if (!other.mantissa) return *this;
        if (exponent < other.exponent) return other.add(*this);
        return rounded((mantissa << 3) + shiftJam(other.mantissa << 3,
            static_cast<unsigned>(exponent - other.exponent)), exponent);
    }
    unsigned floor() const {
        if (!mantissa || exponent < 0) return 0;
        auto value = mantissa;
        for (int bit = exponent; bit < 52; ++bit) value >>= 1;
        return static_cast<unsigned>(value);
    }
};
}
#endif
