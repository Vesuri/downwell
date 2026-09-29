#ifndef DOWNWELL_CORE_WELL512_VERIFICATION_H
#define DOWNWELL_CORE_WELL512_VERIFICATION_H
#include "Well512.h"
namespace downwell {
inline unsigned verifyWell512(){
    unsigned failures=0;
    // One-hot basis covers every input bit at every ring index. The step is
    // linear over XOR; multi-step patterns below also check stores and wrap.
    for(unsigned start=0;start<16;++start)for(unsigned bit=0;bit<512;++bit){
        std::uint32_t actual[16]{},expected[16]{};
        actual[bit/32]=expected[bit/32]=1u<<(bit&31u);
        std::size_t a=start,b=start;
        failures+=Well512::step(actual,a)!=Well512::reference(expected,b);
        failures+=a!=b;
        for(unsigned i=0;i<16;++i)failures+=actual[i]!=expected[i];
    }
    for(unsigned start=0;start<16;++start)for(unsigned pattern=0;pattern<34;++pattern){
        std::uint32_t actual[16],expected[16];
        for(unsigned i=0;i<16;++i)
            actual[i]=expected[i]=pattern==0?0u:pattern==1?0xffffffffu:
                (1u<<((pattern-2+i)&31u));
        std::size_t a=start,b=start;
        for(unsigned step=0;step<64;++step){
            failures+=Well512::step(actual,a)!=Well512::reference(expected,b);
            failures+=a!=b;
            for(unsigned i=0;i<16;++i)failures+=actual[i]!=expected[i];
        }
    }
    return failures;
}
}
#endif
