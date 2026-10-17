#ifndef DOWNWELL_CORE_PIXEL_COLLISION_MATH_H
#define DOWNWELL_CORE_PIXEL_COLLISION_MATH_H
#include <cstdint>

namespace downwell {

// A FixedCollisionMath value (fifths of Q8, 1280 per pixel) held as a whole
// pixel and its 1280ths, so that rounding, the pixel a solid query needs and
// the Q8 fields the enemy records publish take no division.
struct PixelFraction {
    std::int32_t pixel = 0;
    std::int32_t fraction = 0; // [0,1280)
};
constexpr bool operator==(PixelFraction a, PixelFraction b) { return a.pixel == b.pixel && a.fraction == b.fraction; }
constexpr bool operator!=(PixelFraction a, PixelFraction b) { return !(a == b); }
constexpr bool operator<(PixelFraction a, PixelFraction b) {
    return a.pixel < b.pixel || (a.pixel == b.pixel && a.fraction < b.fraction);
}
constexpr bool operator>(PixelFraction a, PixelFraction b) { return b < a; }
constexpr bool operator<=(PixelFraction a, PixelFraction b) { return !(b < a); }
constexpr bool operator>=(PixelFraction a, PixelFraction b) { return !(a < b); }
constexpr PixelFraction operator-(PixelFraction a) {
    return a.fraction ? PixelFraction{-a.pixel - 1, 1280 - a.fraction} : PixelFraction{-a.pixel, 0};
}
constexpr PixelFraction operator+(PixelFraction a, PixelFraction b) {
    std::int32_t pixel = a.pixel + b.pixel, fraction = a.fraction + b.fraction;
    if (fraction >= 1280) { fraction -= 1280; ++pixel; }
    return {pixel, fraction};
}
constexpr PixelFraction operator-(PixelFraction a, PixelFraction b) {
    std::int32_t pixel = a.pixel - b.pixel, fraction = a.fraction - b.fraction;
    if (fraction < 0) { fraction += 1280; --pixel; }
    return {pixel, fraction};
}
inline PixelFraction& operator+=(PixelFraction& a, PixelFraction b) { return a = a + b; }
inline PixelFraction& operator-=(PixelFraction& a, PixelFraction b) { return a = a - b; }
// The motion scripts scale only by a direction sign.
constexpr PixelFraction operator*(int sign, PixelFraction a) {
    return sign > 0 ? a : sign < 0 ? -a : PixelFraction{};
}

// FixedCollisionMath's operations, and the conversions the enemy motion
// records make, on PixelFraction. Keep FixedCollisionMath as the oracle.
struct PixelCollisionMath {
    using Number = PixelFraction;
    static constexpr Number unit() { return {1, 0}; }
    static constexpr Number pixels(int count) { return {count, 0}; }
    static Number fixed(long value) {
        long pixel = value / 1280, fraction = value % 1280;
        if (fraction < 0) { fraction += 1280; --pixel; }
        return {std::int32_t(pixel), std::int32_t(fraction)};
    }
    static long fixedValue(Number value) { return value.pixel * 1280L + value.fraction; }
    // Nearest pixel, ties to even, as FixedCollisionMath::round on the magnitude.
    static int roundPixel(Number value) {
        return value.fraction < 640 ? value.pixel : value.fraction > 640 ? value.pixel + 1 :
            value.pixel + (value.pixel & 1);
    }
    static Number round(Number value) { return {roundPixel(value), 0}; }
    static Number ceilAbs(Number value) {
        if (value.pixel < 0) value = -value;
        return {value.pixel + (value.fraction != 0), 0};
    }
    static int floorPixel(Number value) { return value.pixel; }
    // value/5 (Q8) truncated toward zero, as long division does.
    static long truncatedQ8(Number value) {
        if (value.pixel >= 0) return value.pixel * 256L + fifth(value.fraction);
        return -truncatedQ8(-value);
    }
    // floor(value/5) and the fifths that remain, in [0,5).
    static long floorQ8(Number value) { return value.pixel * 256L + fifth(value.fraction); }
    static int floorQ8Remainder(Number value) { return int(value.fraction - 5 * fifth(value.fraction)); }
private:
    // Exact for every fraction below 1280: one 16x16 multiply, no divide.
    static std::int32_t fifth(std::int32_t fraction) {
        return std::int32_t((std::uint32_t(std::uint16_t(fraction)) * 0xcccdu) >> 18);
    }
};

} // namespace downwell
#endif
