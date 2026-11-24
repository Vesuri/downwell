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
// Signed a with |a| <= 65535 (so +32768 too) times unsigned 16-bit b, as
// sign and magnitude: one mulu.w. The product must fit 32 signed bits.
inline std::int32_t mulsu16(std::int32_t a,std::uint16_t b){
    return a<0?-std::int32_t(mulu16(std::uint16_t(-a),b)):std::int32_t(mulu16(std::uint16_t(a),b));
}
// Where a 16-bit bound rests on what callers pass rather than on the line
// itself, test it: the .w form when it holds, the original 32-bit operation
// otherwise. A few cycles, and exact for every input.
inline bool fitsS16(long v){return v>=-32768&&v<=32767;}
inline bool fitsU16(unsigned long v){return v<=65535u;}
}
#endif
