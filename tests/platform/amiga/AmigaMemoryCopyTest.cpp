#include "platform/amiga/AmigaMemoryCopy.h"
#include "platform/amiga/AmigaMemoryCopyVerification.h"
#include <array>
#include <cassert>
#include <cstdio>
int main(){
    assert(verifyAmigaMemoryCopy(downwellMemoryCopy)==384);
    alignas(4) std::array<unsigned char,1056> source{},actual{},expected{};
    for(unsigned i=0;i<source.size();++i)source[i]=static_cast<unsigned char>(i*73+19);
    const auto original=source;
    for(unsigned from=0;from<16;++from)for(unsigned to=0;to<16;++to)
    for(unsigned count=0;count<=1024;++count){
        actual.fill(0xa5);expected.fill(0xa5);
        for(unsigned i=0;i<count;++i)expected[to+i]=source[from+i];
        assert(downwellMemoryCopy(actual.data()+to,source.data()+from,count)==actual.data()+to);
        assert(actual==expected&&source==original);
    }
    assert(downwellMemoryCopy(nullptr,nullptr,0)==nullptr);
    for(unsigned to=0;to<16;++to)for(unsigned count=0;count<=1024;++count)for(int value:{0,0x5a,-1,0x1ff}){
        actual.fill(0xa5);expected.fill(0xa5);
        for(unsigned i=0;i<count;++i)expected[to+i]=static_cast<unsigned char>(value);
        assert(downwellMemoryFill(actual.data()+to,value,count)==actual.data()+to);
        assert(actual==expected);
    }
    assert(downwellMemoryFill(nullptr,0,0)==nullptr);
    std::puts("Memory copy: 262400 alignment/length cases, guards and source preservation passed; "
              "fill: 65600 cases passed");
}
