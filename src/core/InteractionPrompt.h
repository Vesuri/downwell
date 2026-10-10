#ifndef DOWNWELL_CORE_INTERACTION_PROMPT_H
#define DOWNWELL_CORE_INTERACTION_PROMPT_H
#include "HudReal.h"
#include "RunnerRealComparison.h"
namespace downwell {
// scrCheckThisSignStep uses binary64 arithmetic, not a decimal tenths counter.
struct InteractionPrompt {
    HudReal phase;
    bool step(bool eligible) {
        const auto two = HudReal::integer(2);
        int order;
        if (eligible) {
            if(!compare(two,order))return false;
            if(order<0)phase=two;
            const HudReal limit{0x14666666666666ULL, 2}; // 5.1
            if(!compare(limit,order))return false;
            if(order<=0)phase=phase.add({0x13333333333333ULL, -2}); // .3
        } else {
            phase = phase.add({0x1999999999999aULL, -4}); // .1
            if(!compare(two,order))return false;
            if(order>0)phase=phase.subtract(two); // once, not modulo
        }
        return true;
    }
    unsigned frame() const { return phase.floor(); }
private:
    static bool real(HudReal value,MotionBinary64& out){
        if(!value.mantissa){out={};return true;}
        if(value.exponent < -1022||value.exponent>1023||value.mantissa<0x10000000000000ULL||
           value.mantissa>=0x20000000000000ULL)return false;
        out.high=(unsigned(value.exponent+1023)<<20)|(std::uint32_t(value.mantissa>>32)&0xfffffu);
        out.low=std::uint32_t(value.mantissa);return true;
    }
    bool compare(HudReal other,int& order)const{
        MotionBinary64 a,b;return real(phase,a)&&real(other,b)&&compareRunnerReal(a,b,order);
    }
};
}
#endif
