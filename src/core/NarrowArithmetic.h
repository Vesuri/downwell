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
// 32 x 32 -> 64 bits in four mulu.w.
inline std::uint64_t mulu32(std::uint32_t a,std::uint32_t b){
    const std::uint16_t ah=std::uint16_t(a>>16),al=std::uint16_t(a),bh=std::uint16_t(b>>16),bl=std::uint16_t(b);
    const std::uint64_t middle=std::uint64_t(mulu16(ah,bl))+mulu16(al,bh);
    return (std::uint64_t(mulu16(ah,bh))<<32)+(middle<<16)+mulu16(al,bl);
}
// v << n for n < 64 on 32-bit words: a variable 64-bit shift is a libgcc
// call (__ashldi3), which the Amiga build does not link.
inline std::uint64_t shiftLeft64(std::uint64_t v,unsigned n){
    const auto high=std::uint32_t(v>>32),low=std::uint32_t(v);
    if(n>=32)return std::uint64_t(low<<(n-32))<<32;
    if(!n)return v;
    return (std::uint64_t((high<<n)|(low>>(32-n)))<<32)|std::uint32_t(low<<n);
}
// Q64 fractions: floor(a*b/2^64) less at most 3 (the dropped low product
// and two truncations), in three mulu32.
inline std::uint64_t mulhi64(std::uint64_t a,std::uint64_t b){
    const auto ah=std::uint32_t(a>>32),al=std::uint32_t(a),bh=std::uint32_t(b>>32),bl=std::uint32_t(b);
    return mulu32(ah,bh)+(mulu32(ah,bl)>>32)+(mulu32(al,bh)>>32);
}
}
#endif
