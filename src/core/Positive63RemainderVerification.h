#ifndef DOWNWELL_CORE_POSITIVE63_REMAINDER_VERIFICATION_H
#define DOWNWELL_CORE_POSITIVE63_REMAINDER_VERIFICATION_H
#include "Positive63Remainder.h"
namespace downwell {
inline unsigned verifyPositive63Remainder() {
    using R=Positive63Remainder;
    unsigned failures=0;
    const auto check=[&](R::Words n,R::Words d) {
        const auto a=R::full(n,d),b=R::reference(n,d);
        failures+=a.high!=b.high || a.low!=b.low;
        failures+=!R::less(a,d);
#ifndef __m68k__
        // Independent host oracle, deliberately not linked into the native
        // executable, whose helper audit forbids generic 64-bit remainder.
        const auto expected=((std::uint64_t(n.high)<<32)|n.low)%
                            ((std::uint64_t(d.high)<<32)|d.low);
        failures+=a.high!=std::uint32_t(expected>>32) || a.low!=std::uint32_t(expected);
#endif
    };
    const R::Words boundaries[]={
        {0,0},{0,1},{0,2},{0,3},{0,0xffff},{0,0x10000},{0,0x10001},
        {0,0x7fffffff},{0,0x80000000},{0,0xffffffff},
        {1,0},{1,1},{1,0xffffffff},{0x7fffffff,0},
        {0x7fffffff,0xfffffffe},{0x7fffffff,0xffffffff},{0x80000000,0}
    };
    for(unsigned n=0;n<16;++n)for(unsigned d=1;d<17;++d)
        check(boundaries[n],boundaries[d]);
    std::uint32_t random=0x719a32b5;
    const auto next=[&]() { random^=random<<13;random^=random>>17;random^=random<<5;return random; };
    for(unsigned i=0;i<4096;++i) {
        R::Words n{next()&0x7fffffffu,next()},d{next()&0x7fffffffu,next()|1u};
        if(i%4==0) d.high=0;
        if(i%4==1) { d.high=0;d.low&=0xffffu; }
        check(n,d);
    }
    return failures;
}
}
#endif
