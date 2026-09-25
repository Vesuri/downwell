#ifndef DOWNWELL_CORE_MULTIPLY_BINARY64_H
#define DOWNWELL_CORE_MULTIPLY_BINARY64_H
#include "MotionBinary64.h"
namespace downwell {
// Normal/zero binary64 product, rounded once to nearest-even. Like the motion
// adder, unsupported subnormal/nonfinite operands/results fail atomically.
// Base-65536 limbs keep every intermediate within 32 bits on a 68000.
inline bool multiplyBinary64(MotionBinary64 a, MotionBinary64 b, MotionBinary64& out) {
    if (!a.valid() || !b.valid()) return false;
    const auto sign=(a.high^b.high)&0x80000000u;
    if (a.zero() || b.zero()) {out={sign,0};return true;}
    const std::uint32_t av[4]={a.low&65535u,a.low>>16,a.high&65535u,((a.high>>16)&15u)|16u};
    const std::uint32_t bv[4]={b.low&65535u,b.low>>16,b.high&65535u,((b.high>>16)&15u)|16u};
    std::uint32_t product[8]={};
    for (unsigned i=0;i<4;++i) {
        std::uint32_t carry=0;
        for (unsigned j=0;j<4;++j) {
            const auto sum=av[i]*bv[j]+product[i+j]+carry;
            product[i+j]=sum&65535u;carry=sum>>16;
        }
        product[i+4]=carry;
    }
    const auto p0=product[0]|(product[1]<<16),p1=product[2]|(product[3]<<16);
    const auto p2=product[4]|(product[5]<<16),p3=product[6]|(product[7]<<16);
    const unsigned extra=(p3&512u)?1u:0u, shift=20+extra;
    int exponent=int((a.high>>20)&2047)+int((b.high>>20)&2047)-1023+int(extra);
    MotionBinary64::Words mantissa{(p3<<(32-shift))|(p2>>shift),(p2<<(32-shift))|(p1>>shift)};
    const auto guard=1u<<(shift-1);
    if ((p1&guard) && ((p1&(guard-1)) || p0 || (mantissa.low&1))) mantissa.add({0,1});
    if (mantissa.high&0x200000u) {
        mantissa={mantissa.high>>1,(mantissa.low>>1)|(mantissa.high<<31)};++exponent;
    }
    if (exponent<=0 || exponent>=2047) return false;
    out={sign|(unsigned(exponent)<<20)|(mantissa.high&0xfffffu),mantissa.low};
    return true;
}
}
#endif
