#ifndef DOWNWELL_CORE_DIVIDE_BINARY64_H
#define DOWNWELL_CORE_DIVIDE_BINARY64_H
#include "MotionBinary64.h"
namespace downwell {
// Normal/zero nearest-even division using 32-bit limbs. Unsupported nonfinite,
// subnormal, zero-divisor and under/overflow cases fail without changing out.
inline bool divideBinary64(MotionBinary64 a,MotionBinary64 b,MotionBinary64& out){
    if(!a.valid() || !b.valid() || b.zero())return false;
    const unsigned sign=(a.high^b.high)&0x80000000u;
    if(a.zero()){out={sign,0};return true;}
    using Words=MotionBinary64::Words;
    Words remainder{(a.high&0xfffffu)|0x100000u,a.low};
    const Words divisor{(b.high&0xfffffu)|0x100000u,b.low};
    auto less=[](Words x,Words y){return x.high<y.high || (x.high==y.high && x.low<y.low);};
    int exponent=int((a.high>>20)&2047)-int((b.high>>20)&2047)+1023;
    if(less(remainder,divisor)){remainder.leftOne();--exponent;}
    remainder.subtract(divisor);Words quotient{0,1};
    for(unsigned i=0;i<52;++i){
        remainder.leftOne();quotient.leftOne();
        if(!less(remainder,divisor)){remainder.subtract(divisor);quotient.low|=1;}
    }
    remainder.leftOne();
    const bool equal=remainder.high==divisor.high && remainder.low==divisor.low;
    if(!less(remainder,divisor) && (!equal || (quotient.low&1)))quotient.add({0,1});
    if(quotient.high&0x200000u){quotient={quotient.high>>1,(quotient.low>>1)|(quotient.high<<31)};++exponent;}
    if(exponent<=0 || exponent>=2047)return false;
    out={sign|(unsigned(exponent)<<20)|(quotient.high&0xfffffu),quotient.low};return true;
}
}
#endif
