#ifndef DOWNWELL_TABLE_ATAN_H
#define DOWNWELL_TABLE_ATAN_H
#include "BitCount.h"
#include "PortableAtan.h"
#include "RunnerImageValue.h"
#include "NarrowArithmetic.h"
namespace downwell {
#ifdef DOWNWELL_COMPARE_CONTACT_ATAN
// Test-only selector: run the host oracle with the retained calculation while
// the paired native adapter uses the compact presentation calculation.
inline bool referenceContactAtan=false;
#endif
namespace table_atan_detail {
#include "../../generated/atan-q16.inc"
inline std::uint32_t rounded(std::uint32_t value){
    if(!value)return 0;
    const unsigned top=31-leadingZeros32(value);
    std::uint32_t mantissa;
    unsigned exponent=top+97; // unsigned Q30 -> binary32
    if(top<=23)mantissa=value<<(23-top);
    else{
        const unsigned shift=top-23;
        mantissa=value>>shift;
        const unsigned tail=value&((1u<<shift)-1),half=1u<<(shift-1);
        if(tail>half||(tail==half&&(mantissa&1u)))++mantissa;
        if(mantissa==0x1000000u){mantissa>>=1;++exponent;}
    }
    return (exponent<<23)|(mantissa&0x7fffffu);
}
}
// Presentation-only contact direction: bounded angular error, not bit-exact
// floating-point atan. Gameplay decisions must not use this approximation.
// The retained implementation handles unsupported inputs, not rounding ties.
inline bool tableAtanCandidate(MotionBinary64 y,MotionBinary64 x,MotionBinary64& out){
    RunnerImageValue fx,fy;
    if(!fx.assign(x)||!fy.assign(y)||fx.promoted().high!=x.high||fx.promoted().low!=x.low||
        fy.promoted().high!=y.high||fy.promoted().low!=y.low)return false;
    const auto ax=fx.bits&0x7fffffffu,ay=fy.bits&0x7fffffffu;
    if(ax<0x00800000u||ay<0x00800000u||ax>=0x7f800000u||ay>=0x7f800000u)return false;
    const bool swap=ay>ax;
    const auto small=swap?ax:ay,large=swap?ay:ax;
    const unsigned delta=(large>>23)-(small>>23);
    if(delta>8)return false;
    std::uint32_t remainder=(small&0x7fffffu)|0x800000u;
    const std::uint32_t divisor=((large&0x7fffffu)|0x800000u)<<delta;
    std::uint32_t angle;
    if(remainder==divisor)angle=table_atan_detail::samples[256];
    else{
        std::uint32_t ratio=0;
        for(unsigned bit=0;bit<24;++bit){
            ratio<<=1;
            const auto gap=divisor-remainder;
            if(remainder>=gap){remainder-=gap;ratio|=1u;}else remainder+=remainder;
        }
        const unsigned index=ratio>>16,fraction=ratio&65535u;
        const auto base=table_atan_detail::samples[index];
        const auto slope=table_atan_detail::samples[index+1]-base;
        angle=base+(mulu16(slope,fraction)>>16); // monotonic samples: slope <= 256
    }
    if(swap)angle=table_atan_detail::halfPi-angle;
    if(fx.bits>>31)angle=table_atan_detail::pi-angle;
    // Q16 error: endpoint <=.5, interpolation truncation <1, curvature
    // <=.125 (|atan''|<1), Q24 ratio truncation <1/256, constants <=1.
    // Total <2.629 Q16 radians (<.00230 degrees). The surrounding binary32
    // degree conversion still leaves ample margin below 1/256 degree.
    out=RunnerImageValue{table_atan_detail::rounded(angle<<14)|
        (fy.bits&0x80000000u)}.promoted();return true;
}
inline bool tableAtan(MotionBinary64 y,MotionBinary64 x,MotionBinary64& out){
#ifdef DOWNWELL_COMPARE_CONTACT_ATAN
    if(referenceContactAtan)return portableAtan(y,x,out);
#endif
    return tableAtanCandidate(y,x,out)||portableAtan(y,x,out);
}
}
#endif
