#ifndef DOWNWELL_CORE_RUNNER_DIRECTION_VALUE_H
#define DOWNWELL_CORE_RUNNER_DIRECTION_VALUE_H
#include "RunnerImageValue.h"
namespace downwell {
// Numeric portion of direction's setter: narrow, fmod(360), narrow, then
// binary32 +360 if negative. Engine velocity recomputation is a separate
// mandatory publication step; this helper is NOT the complete setter.
struct RunnerDirectionValue {
    static bool normalize(MotionBinary64 input,RunnerImageValue& output){
        RunnerImageValue value;if(!value.assign(input))return false;
        const unsigned sign=value.bits&0x80000000u;
        const unsigned magnitude=value.bits&0x7fffffffu;
        if(magnitude>=0x43b40000u){ // 360.0f
            const int exponent=int(magnitude>>23)-127;
            unsigned remainder=(magnitude&0x7fffffu)|0x800000u;
            unsigned shift=0;
            if(exponent<23){
                shift=unsigned(23-exponent); // At most 15 for inputs >=360.
                remainder%=360u<<shift;
            }else{
                remainder%=360u;
                for(int bit=23;bit<exponent;++bit){
                    remainder*=2;if(remainder>=360)remainder-=360;
                }
            }
            if(!value.assignInteger(int(remainder)))return false;
            if(remainder)value.bits-=shift<<23;
            value.bits|=sign; // fmod retains negative zero.
        }
        if(sign&&(value.bits&0x7fffffffu)){
            auto sum=value.promoted();
            if(!sum.add(MotionBinary64::fromInteger(360))||!value.assign(sum))return false;
        }
        output=value;return true;
    }
};
}
#endif
