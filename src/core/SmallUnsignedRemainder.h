#ifndef DOWNWELL_CORE_SMALL_UNSIGNED_REMAINDER_H
#define DOWNWELL_CORE_SMALL_UNSIGNED_REMAINDER_H
#include <cassert>
#include <cstdint>
namespace downwell {
// Exact 32-bit remainder for a nonzero 16-bit divisor. The reduced high
// word guarantees a <=65535 quotient, avoiding 68000 DIVU overflow.
struct SmallUnsignedRemainder {
    static std::uint32_t append(std::uint32_t remainder, std::uint16_t word,
                                std::uint32_t divisor) {
        assert(divisor && divisor <= 0xffffu && remainder < divisor);
        std::uint32_t value = (remainder << 16) | word;
#ifdef __m68k__
        __asm__("divu.w %1,%0\n\tswap %0\n\tandi.l #0xffff,%0"
                : "+d"(value) : "d"(divisor) : "cc");
        return value;
#else
        return value % divisor;
#endif
    }
    static std::uint32_t full(std::uint32_t value, std::uint32_t divisor) {
        assert(divisor && divisor <= 0xffffu);
        std::uint32_t high = value >> 16;
        if (high >= divisor) high = append(0, static_cast<std::uint16_t>(high), divisor);
        return append(high, static_cast<std::uint16_t>(value), divisor);
    }
};
}
#endif
