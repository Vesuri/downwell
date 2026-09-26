#ifndef DOWNWELL_CORE_PORTABLE_ATAN_H
#define DOWNWELL_CORE_PORTABLE_ATAN_H
#include "DivideBinary64.h"
#include "MultiplyBinary64.h"
namespace downwell {
// Candidate for the finite atan2 result consumed by RunnerPointDirection.
// Mathematical range reduction + alternating series, NOT an x87 emulation.
// Admission requires binary32 comparison with original-runner evidence.
inline bool portableAtan(MotionBinary64 y,MotionBinary64 x,MotionBinary64& out){
    if(!x.valid() || !y.valid())return false;
    const unsigned sx=x.high&0x80000000u,sy=y.high&0x80000000u;
    x.high&=0x7fffffffu;y.high&=0x7fffffffu;
    constexpr MotionBinary64 pi{0x400921fbu,0x54442d18u},halfPi{0x3ff921fbu,0x54442d18u},quarterPi{0x3fe921fbu,0x54442d18u};
    if(y.zero()){out=sx?pi:MotionBinary64{};out.high|=sy;return true;}
    if(x.zero()){out=halfPi;out.high|=sy;return true;}
    const bool swap=y.high>x.high || (y.high==x.high && y.low>x.low);
    MotionBinary64 z;
    if(!divideBinary64(swap?x:y,swap?y:x,z))return false;
    const bool reduce=z.high>=0x3fe00000u; // z>=1/2; reduced |z|<=1/3
    if(reduce){
        auto denominator=z;
        if(!denominator.add(MotionBinary64::fromInteger(1)) || !z.add(MotionBinary64::fromInteger(-1)) ||
           !divideBinary64(z,denominator,z))return false;
    }
    auto angle=z,term=z;MotionBinary64 square;
    if(!z.zero()){
        if(!multiplyBinary64(z,z,square))return false;
        for(unsigned i=1;i<=32;++i){
            // Further terms cannot affect binary32 once below 2^-60 relative
            // to the leading term. Avoid underflow for very small arguments.
            if(int((term.high>>20)&2047)+int((square.high>>20)&2047)-1023 < int((z.high>>20)&2047)-60)break;
            if(!multiplyBinary64(term,square,term))return false;
            term.high^=0x80000000u;
            MotionBinary64 part;
            if(!divideBinary64(term,MotionBinary64::fromInteger(int(2*i+1)),part) || !angle.add(part))return false;
        }
    }
    if(reduce && !angle.add(quarterPi))return false;
    if(swap){angle.high^=0x80000000u;if(!angle.add(halfPi))return false;}
    if(sx){angle.high^=0x80000000u;if(!angle.add(pi))return false;}
    angle.high^=sy;out=angle;return true;
}
}
#endif
