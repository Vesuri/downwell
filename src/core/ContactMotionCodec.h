#ifndef DOWNWELL_CONTACT_MOTION_CODEC_H
#define DOWNWELL_CONTACT_MOTION_CODEC_H
#include "SmokeDirection.h"
namespace downwell {
// Lossless decoding of an offline-certified finite domain, NOT a general
// cosine approximation. The generator checks every admitted input and stores
// exact exceptions. Callers must retain the original program outside that set.
struct ContactMotionCodec {
    struct Correction {std::uint32_t radians,speed;};
    static std::uint64_t wide(std::uint32_t a,std::uint32_t b){
        const std::uint32_t lo=(a&65535u)*(b&65535u),
            cross0=(a>>16)*(b&65535u),cross1=(a&65535u)*(b>>16),
            mid=(lo>>16)+(cross0&65535u)+(cross1&65535u),
            hi=(a>>16)*(b>>16)+(cross0>>16)+(cross1>>16)+(mid>>16);
        return (std::uint64_t(hi)<<32)|((mid<<16)|(lo&65535u));
    }
    static std::int64_t product(std::int64_t coefficient,std::uint32_t fraction){
        const bool negative=coefficient<0;
        const std::uint64_t magnitude=negative?-coefficient:coefficient;
        const auto value=(wide(std::uint32_t(magnitude>>32),fraction)<<9)+
            (wide(std::uint32_t(magnitude),fraction)>>23);
        return negative?-std::int64_t(value):std::int64_t(value);
    }
    static MotionBinary64 real(std::int64_t fixed){
        if(!fixed)return {};
        const bool negative=fixed<0;
        std::uint64_t magnitude=negative?-fixed:fixed;
        const unsigned hi=unsigned(magnitude>>32);
        const unsigned top=hi?63-MotionBinary64::Words::leadingZeros32(hi):
            31-MotionBinary64::Words::leadingZeros32(unsigned(magnitude));
        const auto shifted=MotionBinary64::Words{hi,unsigned(magnitude)}.left(52-top);
        return {(negative?0x80000000u:0u)|((1023+top-46)<<20)|
            (shifted.high&0xfffffu),shifted.low};
    }
    static bool decode(RunnerImageValue radians,const std::int64_t (*coefficients)[4],
        const Correction* corrections,unsigned count,MotionBinary64& speed){
        unsigned begin=0,end=count;
        while(begin<end){const unsigned mid=begin+(end-begin)/2;
            if(corrections[mid].radians<radians.bits)begin=mid+1;else end=mid;}
        if(begin<count&&corrections[begin].radians==radians.bits){
            speed=RunnerImageValue{corrections[begin].speed}.promoted();return true;
        }
        const unsigned magnitude=radians.bits&0x7fffffffu;
        if((radians.bits>>31)&&magnitude)return false;
        const unsigned exponent=magnitude>>23,mantissa=(magnitude&0x7fffffu)|0x800000u;
        if(exponent>129)return false;
        const unsigned fixed=exponent>=121?mantissa<<(exponent-121):
            exponent>89?mantissa>>(121-exponent):0;
        const unsigned segment=fixed>>23,fraction=fixed&0x7fffffu;
        if(segment>=403)return false;
        const auto* c=coefficients[segment];
        const auto value=c[0]+product(c[1]+product(c[2]+product(c[3],fraction),fraction),fraction);
        return SmokeDirection::component({0x40000000u},real(value),false,speed);
    }
};
}
#endif
