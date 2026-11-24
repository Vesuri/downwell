#ifndef DOWNWELL_CORE_SMOKE_TRIG_NARROW_H
#define DOWNWELL_CORE_SMOKE_TRIG_NARROW_H
#include "SmokeTrig.h"
#include "RunnerImageValue.h"
#include "NarrowArithmetic.h"
namespace downwell {
namespace smoke_trig_narrow_detail {
using U64=std::uint64_t;
#include "../../generated/narrow-trig-q64.inc"
inline U64 high(U64 a,U64 b){return mulhi64(a,b);}
// Margin around a binary32 rounding midpoint, in Q64 units. The exhaustive
// host check (verify-smoke-trig-narrow) proves it covers the evaluation's
// error and the SSE program's own for every admitted input.
constexpr U64 margin=U64(1)<<14;
// Nearest binary32 of a Q64 magnitude, unless a midpoint is within margin.
inline bool narrow(U64 magnitude,bool negative,RunnerImageValue& out){
    return magnitude>=(U64(1)<<40)&&out.assignFixed(magnitude,64,negative,margin); // below 2^-24: midpoints too close
}
}
inline bool smokeTrigNarrowFallback(RunnerImageValue radians,bool cosine,RunnerImageValue& output){
    MotionBinary64 trig;RunnerImageValue narrowed;
    if(!smokeTrigComponent(radians.promoted(),cosine,trig)||!narrowed.assign(trig))return false;
    output=narrowed;return true;
}
namespace smoke_trig_narrow_detail {
// Q64 magnitude and sign of the function, abs error far below margin, for
// binary32 radians with magnitude in [2^-10, 16); false outside it.
inline bool evaluate(RunnerImageValue radians,bool cosine,U64& value,bool& negative){
    const unsigned exponent=(radians.bits>>23)&255u;
    if(exponent<117u||exponent>=131u)return false;
    // Exact Q60 magnitude: the significand shifted by exponent - 90 in [27, 40].
    U64 y=shiftLeft64((radians.bits&0x7fffffu)|0x800000u,exponent-90u);
    unsigned quadrant=0;
    while(y>=halfPi){y-=halfPi;++quadrant;}
    const bool reflect=y>quarterPi;
    if(reflect)y=halfPi-y;
    y<<=4; // Q64, at most pi/4
    const unsigned index=unsigned(y>>58);
    const U64 d=y&((U64(1)<<58)-1);
    // Each term at the precision its size needs for a 2^-56 total: D = 64d
    // in [0, 1); d^2 and d^3/6 in Q64 products, d^4/24 and d^5/120 in
    // 32 x 32, d^6/720 and d^7/5040 in 16 x 16.
    const U64 D=d<<6,D2=high(D,D),D3=high(D2,D);
    const auto Dw=std::uint32_t(D>>32),D2w=std::uint32_t(D2>>32);
    const auto D4w=std::uint32_t(mulu32(D2w,D2w)>>32),D5w=std::uint32_t(mulu32(D4w,Dw)>>32);
    const auto D6h=mulu16(std::uint16_t(D4w>>16),std::uint16_t(D2w>>16)),D7h=mulu16(std::uint16_t(D6h>>16),std::uint16_t(Dw>>16));
    const U64 sinD=d-(high(D3,inverse6)>>18)+(mulu32(D5w,std::uint32_t(inverse120w))>>30)-
        (mulu16(std::uint16_t(D7h>>16),std::uint16_t(inverse5040h))>>22);
    const U64 versine=(D2>>13)-(mulu32(D4w,std::uint32_t(inverse24w))>>24)+
        (mulu16(std::uint16_t(D6h>>16),std::uint16_t(inverse720h))>>13);
    const U64 s=sines[index],c=cosines[index];
    // Octant reflection swaps sine and cosine; odd quadrants swap them too.
    if((cosine==((quadrant&1u)!=0))!=reflect){
        value=s-high(s,versine);
        const U64 part=high(c,sinD);
        if(value+part<value)return false;
        value+=part;
    }else value=c-high(c,versine)-high(s,sinD);
    // The function of the magnitude, then sin(-r) = -sin r. The program
    // takes the signed input; the exhaustive check covers both signs.
    negative=((cosine?quadrant+1u:quadrant)&2u)!=0;
    if(!cosine&&(radians.bits>>31))negative=!negative;
    return true;
}
}
// The binary32 that SmokeDirection::component narrows smokeTrigComponent's
// double to, for binary32 radians of magnitude in [2^-10, 16): an integer Q64 evaluation
// (table every 2^-6 on [0, pi/4], Taylor terms through d^8) accepted only
// when it is clear of a rounding midpoint. Exact for every admitted input
// (verify-smoke-trig-narrow checks all of them against the program).
inline bool smokeTrigNarrowCandidate(RunnerImageValue radians,bool cosine,RunnerImageValue& output){
    smoke_trig_narrow_detail::U64 value;bool negative;
    return smoke_trig_narrow_detail::evaluate(radians,cosine,value,negative)&&
        smoke_trig_narrow_detail::narrow(value,negative,output);
}
// Elsewhere, and near midpoints, the original program decides.
inline bool smokeTrigNarrow(RunnerImageValue radians,bool cosine,RunnerImageValue& output){
    return smokeTrigNarrowCandidate(radians,cosine,output)||smokeTrigNarrowFallback(radians,cosine,output);
}
// smokeTrig for SmokeDirection::vector only: it narrows both results at
// once, so their binary32 stands in for the doubles. Radians that are not
// binary32 keep the program.
inline bool smokeTrigNarrowed(MotionBinary64 radians,MotionBinary64& cosine,MotionBinary64& sine){
    RunnerImageValue r,c,s;
    if(!r.assign(radians)||r.promoted().high!=radians.high||r.promoted().low!=radians.low)
        return smokeTrig(radians,cosine,sine);
    if(!smokeTrigNarrow(r,true,c)||!smokeTrigNarrow(r,false,s))return false;
    cosine=c.promoted();sine=s.promoted();return true;
}
}
#endif
