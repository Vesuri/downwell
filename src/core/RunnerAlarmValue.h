#ifndef DOWNWELL_CORE_RUNNER_ALARM_VALUE_H
#define DOWNWELL_CORE_RUNNER_ALARM_VALUE_H
#include "MotionBinary64.h"
namespace downwell {
// The Windows setter uses cvttsd2si, not pixel rounding. Admit finite values
// whose truncated result fits int32; exceptional conversions remain explicit.
inline bool runnerAlarmValue(MotionBinary64 value,std::int32_t& out){
    if(((value.high>>20)&2047)==2047)return false;
    const int exponent=int((value.high>>20)&2047)-1023;
    if(exponent<0){out=0;return true;}
    if(exponent>31)return false;
    const unsigned significand=(value.high&0xfffffu)|0x100000u;
    const unsigned magnitude=exponent<=20?significand>>unsigned(20-exponent):
        (significand<<unsigned(exponent-20))|(value.low>>unsigned(52-exponent));
    const bool negative=(value.high>>31)!=0;
    if(magnitude>0x7fffffffu){
        if(!negative||magnitude!=0x80000000u)return false;
        out=(-2147483647-1);return true;
    }
    out=negative?-static_cast<std::int32_t>(magnitude):static_cast<std::int32_t>(magnitude);
    return true;
}
}
#endif
