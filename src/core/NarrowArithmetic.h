#ifndef DOWNWELL_CORE_NARROW_ARITHMETIC_H
#define DOWNWELL_CORE_NARROW_ARITHMETIC_H
#include <cstdint>
namespace downwell {
// The 68000 multiplies 16 x 16 -> 32 bits in one instruction (mulu.w,
// muls.w). GCC emits it only when both operands are visibly 16-bit; a
// 32 x 32 product calls libgcc's __mulsi3 (three mulu.w, shifts, adds and
// a call, about 100 cycles on the A1200). Use these where both operands
// fit; the result is the full product, identical on every target.
inline std::uint32_t mulu16(std::uint16_t a,std::uint16_t b){
    return std::uint32_t(a)*std::uint32_t(b);
}
inline std::int32_t muls16(std::int16_t a,std::int16_t b){
    return std::int32_t(a)*std::int32_t(b);
}
}
#endif
