#ifndef DOWNWELL_RUNNER_INTEGER_VALUE_VERIFICATION_H
#define DOWNWELL_RUNNER_INTEGER_VALUE_VERIFICATION_H
#include "RunnerImageValue.h"
namespace downwell {
struct RunnerIntegerValueVerification {
    static bool checkPixel(std::uint32_t bits){
        const RunnerImageValue value{bits};
        std::int32_t direct=1234567,reference=1234567;
        return value.roundPixel(direct)==value.promoted().roundPixel(reference)&&direct==reference;
    }
    static bool check(std::int32_t n){
        RunnerImageValue direct,reference;
        return direct.assignInteger(n)&&reference.assign(MotionBinary64::fromInteger(n))&&direct.bits==reference.bits;
    }
    static bool run(){
        const std::uint32_t mantissas[]={0u,1u,0x3fffffu,0x400000u,0x400001u,0x7fffffu};
        for(unsigned exponent=0;exponent<256;++exponent)for(unsigned sign=0;sign<2;++sign)
            for(auto mantissa:mantissas)
                if(!checkPixel((sign<<31)|(exponent<<23)|mantissa))return false;
        for(unsigned exponent=126;exponent<150;++exponent){
            const unsigned half=1u<<(149-exponent);
            for(unsigned parity=0;parity<2;++parity)for(int offset=-1;offset<=1;++offset){
                const auto bits=(exponent<<23)+half+(parity<<(150-exponent))+offset;
                if(!checkPixel(bits)||!checkPixel(bits|0x80000000u))return false;
            }
        }
        if(!check(0)||!check(-2147483647-1)||!check(2147483647))return false;
        for(unsigned power=0;power<31;++power){
            const std::int32_t center=std::int32_t(1u<<power);
            for(int offset=-2;offset<=2;++offset){
                const auto n=center+offset;
                if(!check(n)||!check(-n))return false;
            }
        }
        // Both parities at every discarded-bit width, including carry to 2^31.
        for(unsigned shift=1;shift<=7;++shift)for(unsigned parity=0;parity<2;++parity){
            const auto center=std::int32_t(((0x800000u+parity)<<shift)+(1u<<(shift-1)));
            for(int offset=-1;offset<=1;++offset)
                if(!check(center+offset)||!check(-center-offset))return false;
        }
        return check(2147483583)&&check(2147483584)&&check(2147483585);
    }
};
}
#endif
