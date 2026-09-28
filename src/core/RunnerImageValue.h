#ifndef DOWNWELL_CORE_RUNNER_IMAGE_VALUE_H
#define DOWNWELL_CORE_RUNNER_IMAGE_VALUE_H
#include "MotionBinary64.h"
namespace downwell {
// Windows image_index/image_speed setters narrow VM doubles to binary32.
// Integer-only normal/zero subset for native animation fields. Unsupported
// subnormal/nonfinite/overflow values fail without changing the destination.
struct RunnerImageValue {
    std::uint32_t bits = 0;
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
