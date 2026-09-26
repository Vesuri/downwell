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
// smoke/shard float radians in [-11,11]. Tiny/general arguments and alternate FP
// environment fallbacks are not admitted. Source zero has explicit results.
inline bool smokeTrig(MotionBinary64 radians,MotionBinary64& cosine,MotionBinary64& sine){
    if(!radians.valid())return false;
    if(radians.zero()){cosine=MotionBinary64::fromInteger(1);sine=radians;return true;}
    const auto magnitude=radians.high&0x7fffffffu;
    // Original dispatch strips the sign before checking the exponent range.
    // Feed signed input to the polynomial; do not substitute trig symmetry.
    if(magnitude<0x30300000u || magnitude>0x40260000u ||
       (magnitude==0x40260000u && radians.low))return false;
    smoke_trig_detail::IntegerReal c,s,input{radians};
    if(!smoke_trig_detail::smokeTrigProgram(input,true,c) ||
       !smoke_trig_detail::smokeTrigProgram(input,false,s))return false;
    cosine=c.value;sine=s.value;return true;
}
}
#endif
