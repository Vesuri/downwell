#include "core/RandomRangeBinary32.h"
#include "core/RandomBinary64.h"
#include <cstdio>
#include <cstdlib>
// The shipped random_range image speeds (bulletExplosion1's 0.25..0.4,
// bulletRanged's 0.4..0.6) against randomBinary64, the binary64 add and the
// binary32 builtin narrowing. Every 2^32 word matched when this was written;
// the suite samples 2^20 low and high words and 2^24 scattered ones.
static void checkAt(bool ok,int line){if(!ok){std::fprintf(stderr,"Random range check failed at line %d\n",line);std::abort();}}
#define check(ok) checkAt((ok),__LINE__)
using R=downwell::MotionBinary64;
namespace {
struct Range{R width,low;std::uint64_t widthM,lowM;const char* name;};
std::uint32_t reference(const Range& r,std::uint32_t word){
    R value;downwell::RunnerImageValue out;
    check(downwell::randomBinary64(word,r.width,value)&&value.add(r.low)&&out.assign(value));
    return out.bits;
}
}
int main(){
    const Range ranges[]{
        {{0x3fc33333u,0x33333334u},{0x3fd00000u,0},0x13333333333334u,0x10000000000000u,"0.25..0.4"},
        {{0x3fc99999u,0x99999998u},{0x3fd99999u,0x9999999au},0x19999999999998u,0x1999999999999au,"0.4..0.6"},
    };
    for(const auto& r:ranges){
        unsigned long long words=0,differ=0;
        const auto test=[&](std::uint32_t w){++words;differ+=downwell::RandomRangeBinary32::value(w,r.widthM,r.lowM).bits!=reference(r,w);};
        for(std::uint32_t w=0;w<(1u<<20);++w){test(w);test(~w);}
        std::uint32_t x=0x9e3779b9u;
        for(unsigned i=0;i<(1u<<24);++i){x^=x<<13;x^=x>>17;x^=x<<5;test(x);}
        std::printf("random_range(%s) binary32: %llu RNG words, %llu differ from the binary64 route\n",r.name,words,differ);
        check(differ==0);
    }
}
