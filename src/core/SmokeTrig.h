#ifndef DOWNWELL_CORE_SMOKE_TRIG_H
#define DOWNWELL_CORE_SMOKE_TRIG_H
#include "MultiplyBinary64.h"
namespace downwell {
namespace smoke_trig_detail {
struct IntegerReal {
    MotionBinary64 value{};
    static constexpr IntegerReal constant(std::uint32_t high,std::uint32_t low){return {{high,low}};}
    bool add(IntegerReal other){return value.add(other.value);}
    bool subtract(IntegerReal other){other.value.high^=0x80000000u;return value.add(other.value);}
    bool multiply(IntegerReal other){MotionBinary64 out;if(!multiplyBinary64(value,other.value,out))return false;value=out;return true;}
    bool integer(int& out) const {std::int32_t rounded;if(!value.roundPixel(rounded))return false;out=rounded;return true;}
};
#include "../../generated/smoke-trig.inc"
}
// The pinned SSE straight-line paths under nearest-even rounding, bounded to
// admitted float radians in [-13,13], including unnormalized currency turns.
// Tiny/general arguments and alternate FP
// environment fallbacks are not admitted. Source zero has explicit results.
inline bool smokeTrigComponent(MotionBinary64 radians,bool cosine,MotionBinary64& output){
    if(!radians.valid())return false;
    if(radians.zero()){output=cosine?MotionBinary64::fromInteger(1):radians;return true;}
    const auto magnitude=radians.high&0x7fffffffu;
    // Original dispatch strips the sign before checking the exponent range.
    // Feed signed input to the polynomial; do not substitute trig symmetry.
    if(magnitude<0x30300000u || magnitude>0x402a0000u ||
       (magnitude==0x402a0000u && radians.low))return false;
    smoke_trig_detail::IntegerReal result,input{radians};
    if(!smoke_trig_detail::smokeTrigProgram(input,cosine,result))return false;
    output=result.value;return true;
}
// lengthdir_x consumers must not evaluate the unused sine program. Retain
// exactly the same recovered cosine operations and rounding boundaries.
inline bool smokeCosine(MotionBinary64 radians,MotionBinary64& cosine){
    return smokeTrigComponent(radians,true,cosine);
}
inline bool smokeTrig(MotionBinary64 radians,MotionBinary64& cosine,MotionBinary64& sine){
    MotionBinary64 c,s;
    if(!smokeTrigComponent(radians,true,c)||!smokeTrigComponent(radians,false,s))return false;
    cosine=c;sine=s;return true;
}
}
#endif
