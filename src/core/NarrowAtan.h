#ifndef DOWNWELL_CORE_NARROW_ATAN_H
#define DOWNWELL_CORE_NARROW_ATAN_H
#include "PortableAtan.h"
#include "RunnerImageValue.h"
#include "NarrowArithmetic.h"
namespace downwell {
namespace narrow_atan_detail {
using U64=std::uint64_t;
#include "../../generated/narrow-atan-q64.inc"
inline U64 high(U64 a,U64 b){return mulhi64(a,b);}
// floor(n*2^64/d) for 64n < d < 2^63: restoring division; the first six
// quotient bits are zero.
inline U64 quotient(U64 n,U64 d){
    U64 q=0;n<<=6;
    for(unsigned bit=6;bit<64;++bit){
        n<<=1;q<<=1;
        if(n>=d){n-=d;q|=1u;}
    }
    return q;
}
constexpr U64 margin=U64(1)<<14; // Q62 units
// The Q62 magnitude (at most pi) of atan2(y, x) for normal binary32
// operands whose exponents differ by at most 30.
inline bool evaluate(RunnerImageValue y,RunnerImageValue x,U64& angle){
    const std::uint32_t ax=x.bits&0x7fffffffu,ay=y.bits&0x7fffffffu;
    if(ax<0x00800000u||ay<0x00800000u||ax>=0x7f800000u||ay>=0x7f800000u)return false;
    const bool swap=ay>ax;
    const auto small=swap?ax:ay,large=swap?ay:ax;
    const unsigned delta=(large>>23)-(small>>23);
    if(delta>30)return false;
    // S/L = z <= 1 exactly scaled. With k = floor(64 z):
    // atan z = atan(k/64) + atan(t), t = (64S - kL)/(64L + kS) in [0, 2^-6).
    const std::uint32_t s=(small&0x7fffffu)|0x800000u;
    const U64 l=shiftLeft64((large&0x7fffffu)|0x800000u,delta);
    U64 n=s;unsigned k=n>=l; // z = 1 only when S = L
    if(k)n=0;
    for(unsigned bit=0;bit<6;++bit){n<<=1;k<<=1;if(n>=l){n-=l;k|=1u;}}
    // n = 64S - kL < L; kS needs one mulu.w per significand half.
    const U64 d=(l<<6)+(U64(mulu16(std::uint16_t(k),std::uint16_t(s>>16)))<<16)+mulu16(std::uint16_t(k),std::uint16_t(s));
    // Terms at the precision their size needs for a 2^-56 total: T = 64t in
    // [0, 1); t^3/3 in Q64 products, t^5/5 in 32 x 32, t^7/7 in 16 x 16.
    const U64 t=quotient(n,d),T=t<<6,T2=high(T,T),T3=high(T2,T);
    const auto Tw=std::uint32_t(T>>32),T2w=std::uint32_t(T2>>32);
    const auto T4w=std::uint32_t(mulu32(T2w,T2w)>>32),T5w=std::uint32_t(mulu32(T4w,Tw)>>32);
    const auto T6h=mulu16(std::uint16_t(T4w>>16),std::uint16_t(T2w>>16)),T7h=mulu16(std::uint16_t(T6h>>16),std::uint16_t(Tw>>16));
    U64 value=(arctangents[k]+t-(high(T3,inverse3)>>18)+(mulu32(T5w,std::uint32_t(inverse5w))>>30)-
        (mulu16(std::uint16_t(T7h>>16),std::uint16_t(inverse7h))>>12))>>2;
    if(swap)value=halfPi-value;
    if(x.bits>>31)value=pi-value;
    angle=value;return true;
}
// Nearest binary32 of a Q62 magnitude, unless a midpoint is within margin.
inline bool narrow(U64 magnitude,bool negative,RunnerImageValue& out){
    return magnitude>=(U64(1)<<38)&&out.assignFixed(magnitude,62,negative,margin); // below 2^-24
}
}
// The binary32 that RunnerPointDirection narrows portableAtan's double to,
// for binary32 operands: an integer evaluation (error below 2^-56) accepted
// only when no rounding midpoint lies within margin (2^-48), so that it and
// portableAtan (within about 2^-50, verify-narrow-atan) narrow alike.
inline bool narrowAtanCandidate(RunnerImageValue y,RunnerImageValue x,RunnerImageValue& output){
    narrow_atan_detail::U64 angle;
    return narrow_atan_detail::evaluate(y,x,angle)&&narrow_atan_detail::narrow(angle,y.bits>>31,output);
}
// An atan for RunnerPointDirection only: its result is narrowed at once, so
// the binary32 stands in for the double. Other operands go to portableAtan.
inline bool narrowAtan(MotionBinary64 y,MotionBinary64 x,MotionBinary64& out){
    RunnerImageValue fy,fx,angle;
    if(fy.assign(y)&&fx.assign(x)&&fy.promoted().high==y.high&&fy.promoted().low==y.low&&
        fx.promoted().high==x.high&&fx.promoted().low==x.low&&narrowAtanCandidate(fy,fx,angle)){
        out=angle.promoted();return true;
    }
    return portableAtan(y,x,out);
}
}
#endif
