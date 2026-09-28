#ifndef DOWNWELL_CORE_MULTIPLY_BINARY32_H
#define DOWNWELL_CORE_MULTIPLY_BINARY32_H
#include "RunnerImageValue.h"
namespace downwell {
// Exact 24x24 significand product followed by the same nearest-even narrowing
// as promoted binary64 multiplication + RunnerImageValue::assign. A binary64
// significand holds the entire 48-bit product, so there is no intermediate
// rounding to reproduce. Normal/zero inputs only; failure leaves out untouched.
inline bool multiplyBinary32(RunnerImageValue a,RunnerImageValue b,RunnerImageValue& out){
    const unsigned ae=(a.bits>>23)&255u,be=(b.bits>>23)&255u;
    if(ae==255||be==255||(!ae&&(a.bits&0x7fffffffu))||(!be&&(b.bits&0x7fffffffu)))return false;
    const auto sign=(a.bits^b.bits)&0x80000000u;
    if(!ae||!be){out.bits=sign;return true;}
    const auto am=(a.bits&0x7fffffu)|0x800000u,bm=(b.bits&0x7fffffu)|0x800000u;
    const auto al=am&65535u,ah=am>>16,bl=bm&65535u,bh=bm>>16;
    const auto bottom=al*bl,middle=ah*bl+al*bh;
    const auto low=bottom+(middle<<16);
    const auto high=ah*bh+(middle>>16)+(low<bottom?1u:0u);
    const unsigned extra=(high>>15)&1u,shift=23+extra;
    int exponent=int(ae)+int(be)-127+int(extra);
    // assign rejects a subnormal exponent before rounding, even if rounding
    // could have promoted the result to the smallest normal value.
    if(exponent<=0||exponent>=255)return false;
    auto mantissa=(high<<(32-shift))|(low>>shift);
    const std::uint32_t half=1u<<(shift-1),remainder=low&((1u<<shift)-1);
    if(remainder>half||(remainder==half&&(mantissa&1u)))++mantissa;
    if(mantissa==0x1000000u){mantissa>>=1;++exponent;}
    if(exponent>=255)return false;
    out.bits=sign|(unsigned(exponent)<<23)|(mantissa&0x7fffffu);return true;
}
}
#endif
