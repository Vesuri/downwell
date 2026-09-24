#ifndef DOWNWELL_CORE_LEVEL_CLEAR_RASTER_H
#define DOWNWELL_CORE_LEVEL_CLEAR_RASTER_H
#include "LevelClearCarousel.h"
namespace downwell {
// Narrow the shared normalized binary64 coordinate to binary32 (ties to even),
// then floor it. Matches the host fixture's float-coordinate raster policy;
// Windows GPU pixel-center behavior still needs external comparison. No FPU.
struct LevelClearRaster {
    static int floorBinary32(LevelClearCoordinate coordinate) {
        const auto value = coordinate.magnitude;
        if (!value.mantissa || value.exponent < -150) return 0;
        if (value.exponent == -150) {
            // Exactly half the smallest float subnormal rounds to signed zero.
            return coordinate.negative && value.mantissa > (std::uint64_t{1} << 52) ? -1 : 0;
        }
        if (value.exponent < -1) return coordinate.negative ? -1 : 0;
        // Out-of-range coordinates remain far outside the viewport instead of
        // invoking undefined float-to-int overflow. Ordinary scene values fit.
        if (value.exponent >= 31) return coordinate.negative ? (-2147483647 - 1) : 2147483647;
        std::uint32_t mantissa = std::uint32_t(value.mantissa >> 29);
        const unsigned remainder = unsigned(value.mantissa & ((1u << 29) - 1));
        if (remainder > (1u << 28) || (remainder == (1u << 28) && (mantissa & 1))) ++mantissa;
        int exponent = value.exponent;
        if (mantissa == (1u << 24)) { mantissa >>= 1; ++exponent; }
        if (exponent >= 31) return coordinate.negative ? (-2147483647 - 1) : 2147483647;
        unsigned whole;
        bool fraction = false;
        if (exponent >= 23) whole = mantissa << (exponent - 23);
        else {
            const unsigned bits = 23 - exponent;
            whole = mantissa >> bits;
            fraction = (mantissa & ((1u << bits) - 1)) != 0;
        }
        return coordinate.negative ? -int(whole) - int(fraction) : int(whole);
    }
};
}
#endif
