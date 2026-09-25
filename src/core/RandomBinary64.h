#ifndef DOWNWELL_CORE_RANDOM_BINARY64_H
#define DOWNWELL_CORE_RANDOM_BINARY64_H
#include "MotionBinary64.h"
namespace downwell {
// Exact round-to-nearest-even of (word / 2^32) * positiveScale. This is the
// Windows random() multiply, not a general binary64 multiplication routine.
// Three 32-bit limbs retain the complete 85-bit product before rounding.
inline MotionBinary64::Words randomMultiplyWords(std::uint32_t a, std::uint32_t b) {
    const std::uint32_t a0 = a & 65535, a1 = a >> 16, b0 = b & 65535, b1 = b >> 16;
    MotionBinary64::Words result{0,a0*b0};
    const auto ab = a0*b1, ba = a1*b0;
    result.add({ab >> 16, ab << 16}); result.add({ba >> 16, ba << 16});
    result.high += a1*b1;
    return result;
}
inline bool randomBinary64(std::uint32_t word, MotionBinary64 scale, MotionBinary64& result) {
    if (!scale.valid() || (scale.high & 0x80000000u)) return false;
    if (!word || scale.zero()) { result = {}; return true; }
    const auto bottom = randomMultiplyWords(word, scale.low);
    const auto top = randomMultiplyWords(word, (scale.high & 0xfffff) | 0x100000u);
    std::uint32_t low = bottom.low, middle = bottom.high + top.low;
    std::uint32_t high = top.high + unsigned(middle < bottom.high);
    int exponent = int((scale.high >> 20) & 2047) + 11;
    while (!(high & 0x80000000u)) {
        high = (high << 1) | (middle >> 31);
        middle = (middle << 1) | (low >> 31); low <<= 1;
        --exponent;
    }
    MotionBinary64::Words mantissa{high >> 11, (high << 21) | (middle >> 11)};
    const bool guard = (middle & 1024) != 0;
    if (guard && ((middle & 1023) || low || (mantissa.low & 1))) mantissa.add({0,1});
    if (mantissa.high & 0x200000u) {
        mantissa = {mantissa.high >> 1, (mantissa.low >> 1) | (mantissa.high << 31)};
        ++exponent;
    }
    if (exponent <= 0 || exponent >= 2047) return false;
    result = {(unsigned(exponent) << 20) | (mantissa.high & 0xfffff), mantissa.low};
    return true;
}
}
#endif
