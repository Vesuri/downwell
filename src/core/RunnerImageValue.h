#ifndef DOWNWELL_CORE_RUNNER_IMAGE_VALUE_H
#define DOWNWELL_CORE_RUNNER_IMAGE_VALUE_H
#include "MotionBinary64.h"
#include "NarrowArithmetic.h"
namespace downwell {
// Windows image_index/image_speed setters narrow VM doubles to binary32.
// Integer-only normal/zero subset for native animation fields. Unsupported
// subnormal/nonfinite/overflow values fail without changing the destination.
struct RunnerImageValue {
    std::uint32_t bits = 0;
    // Nearest binary32 of a fixed-point magnitude (2^32 <= magnitude, value
    // magnitude/2^fractionBits) known within margin < 2^24 units: false when
    // a rounding midpoint lies within margin, since the value's rounding is
    // then undecided. 32-bit word operations only.
    bool assignFixed(std::uint64_t magnitude,unsigned fractionBits,bool negative,std::uint64_t margin){
        const auto top=std::uint32_t(magnitude>>32);
        if(!top)return false;
        const unsigned zeros=MotionBinary64::Words::leadingZeros32(top);
        const auto normal=shiftLeft64(magnitude,zeros); // top bit 63
        auto mantissa=std::uint32_t(normal>>40);
        const std::uint64_t remainder=normal&((std::uint64_t(1)<<40)-1),half=std::uint64_t(1)<<39;
        if((remainder>half?remainder-half:half-remainder)<=shiftLeft64(margin,zeros))return false;
        int exponent=127+63-int(zeros)-int(fractionBits);
        if(remainder>half&&++mantissa==0x1000000u){mantissa>>=1;++exponent;}
        if(exponent<=0||exponent>=255)return false;
        bits=(negative?0x80000000u:0u)|(std::uint32_t(exponent)<<23)|(mantissa&0x7fffffu);
        return true;
    }
    // Same nearest-even pixel conversion as promoted().roundPixel(), without
    // building a split binary64 mantissa. Preserve that adapter's range (even
    // -2^31 rejects) and its exponent-zero promotion to signed zero. Callers
    // remain responsible for normal/zero admission before geometry operations.
    bool roundPixel(std::int32_t& result)const{
        const unsigned exponent=(bits>>23)&255u;
        if(exponent<126){result=0;return true;}
        if(exponent>157)return false;
        const std::uint32_t mantissa=(bits&0x7fffffu)|0x800000u;
        std::uint32_t value;
        if(exponent>=150)value=mantissa<<(exponent-150);
        else{
            const unsigned shift=150-exponent;
            value=mantissa>>shift;
            const unsigned half=1u<<(shift-1),tail=mantissa&((1u<<shift)-1);
            if(tail>half||(tail==half&&(value&1u)))++value;
        }
        result=bits&0x80000000u?-static_cast<std::int32_t>(value):static_cast<std::int32_t>(value);
        return true;
    }
    // Every int32 is exactly representable in binary64. Direct nearest-even
    // narrowing therefore matches assign(MotionBinary64::fromInteger(value)).
    bool assignInteger(std::int32_t value) {
        if(!value){bits=0;return true;}
        const std::uint32_t sign=value<0?0x80000000u:0u;
        const std::uint32_t magnitude=value<0?0u-std::uint32_t(value):std::uint32_t(value);
        unsigned top=31-MotionBinary64::Words::leadingZeros32(magnitude);
        std::uint32_t mantissa;
        if(top<=23)mantissa=magnitude<<(23-top);
        else {
            const unsigned shift=top-23;
            mantissa=magnitude>>shift;
            const unsigned half=1u<<(shift-1),remainder=magnitude&((1u<<shift)-1);
            if(remainder>half||(remainder==half&&(mantissa&1u)))++mantissa;
            if(mantissa==0x1000000u){mantissa>>=1;++top;}
        }
        bits=sign|((top+127u)<<23)|(mantissa&0x7fffffu);return true;
    }
    bool assign(MotionBinary64 value) {
        const auto sign = value.high & 0x80000000u;
        const auto exponent = (value.high >> 20) & 2047u;
        if (!exponent) {
            if ((value.high & 0xfffffu) || value.low) return false;
            bits = sign; return true;
        }
        int e = int(exponent) - 1023 + 127;
        if (e <= 0 || e >= 255) return false;
        std::uint32_t mantissa = ((value.high & 0xfffffu) | 0x100000u) << 3;
        mantissa |= value.low >> 29;
        const auto remainder = value.low & 0x1fffffffu;
        if (remainder > 0x10000000u ||
            (remainder == 0x10000000u && (mantissa & 1))) ++mantissa;
        if (mantissa == 0x1000000u) { mantissa >>= 1; ++e; }
        if (e >= 255) return false;
        bits = sign | (std::uint32_t(e) << 23) | (mantissa & 0x7fffffu);
        return true;
    }
    MotionBinary64 promoted() const {
        const auto sign = bits & 0x80000000u;
        const auto e = (bits >> 23) & 255u;
        if (!e) return {sign, 0}; // Only normal/zero values are admitted.
        return {sign | ((e + 896u) << 20) | ((bits & 0x7fffffu) >> 3),
                (bits & 7u) << 29};
    }
};
}
#endif
