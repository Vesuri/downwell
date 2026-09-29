#ifndef DOWNWELL_AMIGA_MEMORY_COPY_VERIFICATION_H
#define DOWNWELL_AMIGA_MEMORY_COPY_VERIFICATION_H
#include <cstddef>
// Small target-side corpus; the host test exhausts every length through 1024.
inline unsigned verifyAmigaMemoryCopy(void* (*copy)(void*,const void*,std::size_t)){
    alignas(4) static unsigned char source[288],actual[288],expected[288];
    const unsigned lengths[]={0,1,2,3,4,5,7,8,9,15,16,17,31,32,33,63,64,65,127,128,129,255,256,257};
    unsigned cases=0;
    for(unsigned from=0;from<4;++from)for(unsigned to=0;to<4;++to)for(unsigned length:lengths){
        // Volatile byte oracle must not lower to the memcpy under test.
        volatile unsigned char* reference=expected;
        for(unsigned i=0;i<288;++i){source[i]=static_cast<unsigned char>(i*73+19);actual[i]=0xa5;reference[i]=0xa5;}
        for(unsigned i=0;i<length;++i)reference[to+i]=source[from+i];
        if(copy(actual+to,source+from,length)!=actual+to)return 0;
        for(unsigned i=0;i<288;++i)
            if(actual[i]!=expected[i]||source[i]!=static_cast<unsigned char>(i*73+19))return 0;
        ++cases;
    }
    return cases;
}
#endif
