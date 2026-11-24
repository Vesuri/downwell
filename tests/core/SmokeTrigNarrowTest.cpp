// Every binary32 radian smokeTrigNarrowCandidate admits, both functions:
// its result must equal the binary32 narrowing of the recovered SSE program
// (evaluated with host binary64 arithmetic, which the soft path matches; a
// sample re-checks that through smokeTrigNarrow's own fallback).
#include "core/SmokeTrigNarrow.h"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
using downwell::MotionBinary64;
using downwell::RunnerImageValue;
static double real(MotionBinary64 x){const std::uint64_t b=(std::uint64_t(x.high)<<32)|x.low;double d;std::memcpy(&d,&b,8);return d;}
struct Scalar {
    double value=0;
    static Scalar constant(std::uint32_t h,std::uint32_t l){return {real({h,l})};}
    bool add(Scalar b){value+=b.value;return true;}
    bool subtract(Scalar b){value-=b.value;return true;}
    bool multiply(Scalar b){value*=b.value;return true;}
    bool integer(int& out)const{out=int(std::nearbyint(value));return true;}
};
static float bitsFloat(std::uint32_t b){float f;std::memcpy(&f,&b,4);return f;}
static std::uint32_t floatBits(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
int main(){
    constexpr std::uint32_t first=117u<<23,last=130u<<23;
    std::atomic<unsigned long> admitted{0},rejected{0},failures{0};
    std::atomic<std::uint64_t> worst{0};
    const unsigned threads=std::max(1u,std::thread::hardware_concurrency());
    std::vector<std::thread> pool;
    for(unsigned t=0;t<threads;++t)pool.emplace_back([&,t]{
        unsigned long localAdmitted=0,localRejected=0,localFailures=0;std::uint64_t localWorst=0;
        for(std::uint32_t bits=first+t;bits<last;bits+=threads)for(int cosine=0;cosine<2;++cosine){
            Scalar exact;
            if(!downwell::smoke_trig_detail::smokeTrigProgram(Scalar{bitsFloat(bits)},cosine!=0,exact)){
                ++localFailures;continue;
            }
            downwell::smoke_trig_narrow_detail::U64 value;bool negative;
            if(downwell::smoke_trig_narrow_detail::evaluate({bits},cosine!=0,value,negative)){
                // Error of the Q64 evaluation against the program's double.
                const double magnitude=std::fabs(exact.value);
                if((exact.value<0)!=negative)++localFailures;
                else{
                    // magnitude*2^64 in integers: 53-bit significand, truncated below a unit.
                    int e;const double m=std::frexp(magnitude,&e);
                    const auto significand=std::uint64_t(std::ldexp(m,53));
                    const int shift=e+11; // magnitude = significand*2^(e-53), Q64 shift e+11
                    if(magnitude>=std::ldexp(1.0,-24)&&shift<=11){
                        const std::uint64_t scaled=shift>=0?significand<<shift:significand>>-shift;
                        const auto units=(scaled>value?scaled-value:value-scaled)+1;
                        if(units>localWorst)localWorst=units;
                    }
                }
            }
            RunnerImageValue fast;
            if(!downwell::smokeTrigNarrowCandidate({bits},cosine!=0,fast)){++localRejected;continue;}
            ++localAdmitted;
            if(fast.bits!=floatBits(float(exact.value))){
                if(localFailures<10)std::fprintf(stderr,"mismatch %08x %s: %08x expected %08x\n",
                    bits,cosine?"cos":"sin",fast.bits,floatBits(float(exact.value)));
                ++localFailures;
            }
            // The shipping soft path on a sample, through the fallback.
            if(!((bits>>5)%4099u)){
                RunnerImageValue soft;
                if(!downwell::smokeTrigNarrowFallback({bits},cosine!=0,soft)||soft.bits!=fast.bits)++localFailures;
            }
        }
        admitted+=localAdmitted;rejected+=localRejected;failures+=localFailures;
        for(auto w=worst.load();localWorst>w&&!worst.compare_exchange_weak(w,localWorst););
    });
    for(auto& thread:pool)thread.join();
    // Outside the domain the candidate declines.
    RunnerImageValue unused;
    for(std::uint32_t bits:{first-1,last,0x80000000u|first,0x3f800000u|0x80000000u,0u})
        if(downwell::smokeTrigNarrowCandidate({bits},false,unused))++failures;
    const auto margin=downwell::smoke_trig_narrow_detail::margin;
    std::printf("Smoke trig narrow: %lu admitted, %lu to the program (%.4f%%), worst error %llu of margin %llu Q64 units\n",
        admitted.load(),rejected.load(),100.0*double(rejected)/double(admitted+rejected),
        (unsigned long long)worst.load(),(unsigned long long)margin);
    if(failures||worst.load()*4>margin){std::fprintf(stderr,"Smoke trig narrow failed: %lu\n",failures.load());return 1;}
    return 0;
}
