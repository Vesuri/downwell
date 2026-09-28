#ifndef DOWNWELL_FIXED_COLLISION_ROUNDING_VERIFICATION_H
#define DOWNWELL_FIXED_COLLISION_ROUNDING_VERIFICATION_H
#include "SolidCollision.h"
namespace downwell {
struct FixedCollisionRoundingVerification {
    // Build expected results from a known quotient/remainder, independently
    // of the implementation's division. Keep input and rounded output in the
    // signed 32-bit domain on both CPUs (host long may be wider).
    static bool check(long quotient,long remainder){
        const long input=quotient*1280+remainder;
        const long expected=(quotient+(remainder>640||(remainder==640&&(quotient&1))))*1280;
        return FixedCollisionMath::round(input)==expected&&FixedCollisionMath::round(-input)==-expected;
    }
    static bool run(){
        const std::uint32_t divisors[]={1,2,5,127,1280,32768,65535};
        const std::uint32_t values[]={0,1,65535,65536,0x7fffffffu,0x80000000u,0xffffffffu};
        auto division=[](std::uint32_t value,std::uint32_t divisor){
            const auto native=SmallUnsignedDivision::full(value,divisor);
            const auto portable=SmallUnsignedDivision::reference(value,divisor);
            return native.quotient==portable.quotient&&native.remainder==portable.remainder;
        };
        for(auto divisor:divisors){
            for(auto value:values)if(!division(value,divisor))return false;
            const auto boundary=divisor<<16;
            if(!division(boundary-1,divisor)||!division(boundary,divisor)||!division(boundary+1,divisor))return false;
        }
        const long quotients[]={0,1,2,127,128,32767,32768,131071,1677720};
        const long remainders[]={0,1,639,640,641,1278,1279};
        for(auto quotient:quotients)for(auto remainder:remainders)
            if(!check(quotient,remainder))return false;
        return true;
    }
};
}
#endif
