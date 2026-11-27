#ifndef DOWNWELL_CORE_RANDOM_RANGE_BINARY32_H
#define DOWNWELL_CORE_RANDOM_RANGE_BINARY32_H
#include "RunnerImageValue.h"
#include <cstdint>
namespace downwell {
// A binary32 builtin assigned random_range(low, low + width), i.e.
// random(width) + low, with the VM's three roundings reproduced in integers
// (W3.20): (word / 2^32) * width to 53 significant bits, the sum to its
// binary64 ulp, then to binary32, each to nearest even. Round words often
// land exactly on a binary32 tie after the first two roundings, so a single
// rounding would differ. The domain is the shipped one: width (the binary64
// significand widthM * 2^-55) in [0.125, 0.25) and low (lowM * 2^-54) in
// [0.25, 0.5), so the sum lies in [0.25, 0.75).
struct RandomRangeBinary32 {
    static std::uint64_t mask(unsigned bits) { return (std::uint64_t(1) << bits) - 1; }
    static std::uint64_t nearestEven(std::uint64_t q, std::uint64_t rest, unsigned shift) {
        if (!shift) return q;
        const std::uint64_t half = std::uint64_t(1) << (shift - 1);
        return q + (rest > half || (rest == half && (q & 1)));
    }
    static RunnerImageValue value(std::uint32_t word, std::uint64_t widthM, std::uint64_t lowM) {
        // N = word * widthM = top * 2^32 + bottom, the product in 2^-87 units.
        const std::uint64_t lo = std::uint64_t(word) * std::uint32_t(widthM);
        const std::uint64_t top = std::uint64_t(word) * std::uint32_t(widthM >> 32) + (lo >> 32);
        const std::uint32_t bottom = std::uint32_t(lo);
        unsigned length = 0;
        while (length < 64 && (top >> length)) ++length;
        std::uint64_t product; unsigned k = 0; // the rounded product is product * 2^k
        if (length <= 21) product = (top << 32) | bottom; // at most 53 bits: exact
        else {
            k = length - 21; // 1..32
            product = nearestEven((top << (32 - k)) | (std::uint64_t(bottom) >> k), std::uint64_t(bottom) & mask(k), k);
        }
        // The exact sum product * 2^k + lowM * 2^33 (2^-87 units) rounded at
        // shift s: 33 (ulp 2^-54) below 0.5, 34 (2^-53) from it.
        const auto sum = [&](unsigned s) {
            const std::uint64_t rest = ((product & mask(s - k)) << k) + ((lowM & mask(s - 33)) << 33);
            const std::uint64_t q = (product >> (s - k)) + (lowM >> (s - 33)) + (rest >> s);
            return nearestEven(q, rest & mask(s), s);
        };
        std::uint64_t s = sum(33);
        unsigned exponent = 125;
        if (s >= (std::uint64_t(1) << 53)) {
            s = sum(34); exponent = 126;
        }
        std::uint64_t mantissa = nearestEven(s >> 29, s & mask(29), 29);
        if (mantissa == (std::uint64_t(1) << 24)) { mantissa >>= 1; ++exponent; }
        return {(exponent << 23) | (std::uint32_t(mantissa) - 0x800000u)};
    }
};
}
#endif
