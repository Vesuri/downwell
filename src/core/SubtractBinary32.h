#ifndef DOWNWELL_CORE_SUBTRACT_BINARY32_H
#define DOWNWELL_CORE_SUBTRACT_BINARY32_H
#include "RunnerImageValue.h"
namespace downwell {
// Nearest-even subtraction using a 24-bit significand and guard/round/sticky
// bits in one word. Match promoted binary64 addition followed by assign,
// including signed zero and rejection of a subnormal result BEFORE rounding.
inline bool subtractBinary32(RunnerImageValue left,RunnerImageValue right,RunnerImageValue& out){
    auto a=left.bits,b=right.bits^0x80000000u;
    const unsigned ae=(a>>23)&255u,be=(b>>23)&255u;
    if(ae==255||be==255||(!ae&&(a&0x7fffffu))||(!be&&(b&0x7fffffu))){
        // Keep existing behavior outside the normal/zero domain. In particular
        // promoted() is not a general IEEE conversion of NaNs or subnormals.
        auto wide=left.promoted();
        return wide.add(RunnerImageValue{b}.promoted())&&out.assign(wide);
    }
    if(!(a&0x7fffffffu)){
        out.bits=(b&0x7fffffffu)?b:(a&b);return true;
    }
    if(!(b&0x7fffffffu)){out.bits=a;return true;}
    if((a&0x7fffffffu)<(b&0x7fffffffu)){const auto tmp=a;a=b;b=tmp;}
    int exponent=int((a>>23)&255u);
    const unsigned gap=unsigned(exponent)-((b>>23)&255u);
    std::uint32_t major=((a&0x7fffffu)|0x800000u)<<3;
    std::uint32_t minor=((b&0x7fffffu)|0x800000u)<<3;
    if(gap){
        if(gap<32)minor=(minor>>gap)|((minor<<(32-gap))!=0?1u:0u);
        else minor=1;
    }
    if((a^b)&0x80000000u){
        major-=minor;
        if(!major){out.bits=0;return true;}
        const unsigned shift=MotionBinary64::Words::leadingZeros32(major)-5;
        major<<=shift;exponent-=int(shift);
    }else{
        major+=minor;
        if(major&0x08000000u){major=(major>>1)|(major&1u);++exponent;}
    }
    if(exponent<=0||exponent>=255)return false;
    const unsigned tail=major&7u;
    auto significand=major>>3;
    if(tail>4||(tail==4&&(significand&1u)))++significand;
    if(significand==0x1000000u){significand>>=1;++exponent;}
    if(exponent>=255)return false;
    out.bits=(a&0x80000000u)|(unsigned(exponent)<<23)|(significand&0x7fffffu);
    return true;
}
}
#endif
