// Random binary32 pairs (wide exponents, room-scale differences, integers
// and halves): narrowAtanCandidate must equal portableAtan's narrowing, and
// its Q62 value must stay within a quarter of margin of portableAtan's.
#include "core/NarrowAtan.h"
#include <atomic>
#include <cstdio>
#include <cstring>
#include <random>
#include <thread>
#include <vector>
using namespace downwell;
static std::uint32_t fb(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
int main(int argc,char**argv){
    const unsigned long perThread=argc>1?std::strtoul(argv[1],0,10):200000;
    std::atomic<unsigned long> admitted{0},rejected{0},failures{0},skipped{0};
    std::atomic<std::uint64_t> worst{0};
    const unsigned threads=std::thread::hardware_concurrency();
    std::vector<std::thread> pool;
    for(unsigned t=0;t<threads;++t)pool.emplace_back([&,t]{
        std::mt19937_64 rng(1234+t);
        unsigned long a=0,r=0,f=0,sk=0;std::uint64_t w=0;
        for(unsigned long i=0;i<perThread;++i){
            RunnerImageValue x,y;
            switch(i%3){
            case 0:{ // random bits, exponents within 40
                const std::uint32_t e=90+rng()%60;
                x.bits=std::uint32_t(rng()&0x807fffffu)|(e<<23);
                y.bits=std::uint32_t(rng()&0x807fffffu)|((e-20+rng()%41)<<23);
                break;}
            case 1:{ // room-scale differences with fractional speeds
                std::uniform_real_distribution<double> u(-480,480);
                x.bits=fb(float(float(u(rng))-float(u(rng))));
                y.bits=fb(float(float(u(rng))-float(u(rng))));
                break;}
            default:{ // integers and halves
                x.bits=fb(float(int(rng()%961)-480)/(1+rng()%2));
                y.bits=fb(float(int(rng()%961)-480)/(1+rng()%2));
            }}
            if(!(x.bits&0x7fffffffu)){++sk;continue;}
            MotionBinary64 exact;RunnerImageValue expected;
            if(!portableAtan(y.promoted(),x.promoted(),exact)||!expected.assign(exact)){++sk;continue;}
            narrow_atan_detail::U64 q;
            if(narrow_atan_detail::evaluate(y,x,q)){
                // portable magnitude in Q62 from its double bits
                const std::uint64_t bits=(std::uint64_t(exact.high&0x7fffffffu)<<32)|exact.low;
                const int e=int(bits>>52)-1023; const std::uint64_t m=(bits&((1ull<<52)-1))|(1ull<<52);
                const int shift=e+62-52;
                if(e>-24&&shift<=11){
                    const std::uint64_t scaled=shift>=0?m<<shift:m>>-shift;
                    const auto u=(scaled>q?scaled-q:q-scaled)+1; if(u>w)w=u;
                }
            }
            RunnerImageValue fast;
            if(!narrowAtanCandidate(y,x,fast)){++r;continue;}
            ++a;
            if(fast.bits!=expected.bits){if(f<5)std::fprintf(stderr,"mismatch y=%08x x=%08x %08x expected %08x\n",y.bits,x.bits,fast.bits,expected.bits);++f;}
        }
        admitted+=a;rejected+=r;failures+=f;skipped+=sk;
        for(auto c=worst.load();w>c&&!worst.compare_exchange_weak(c,w););
    });
    for(auto&th:pool)th.join();
    std::printf("narrow atan: %lu admitted, %lu rejected, %lu skipped, %lu failures, worst %llu of margin %llu Q62 units\n",
        admitted.load(),rejected.load(),skipped.load(),failures.load(),(unsigned long long)worst.load(),(unsigned long long)narrow_atan_detail::margin);
    // Outside the domain the candidate declines; narrowAtan still answers.
    RunnerImageValue unused;MotionBinary64 angle;
    if(narrowAtanCandidate({0},{0x3f800000u},unused)||narrowAtanCandidate({0x3f800000u},{0x00400000u},unused)||
        narrowAtanCandidate({0x3f800000u},{0x30000000u},unused)||
        !narrowAtan(RunnerImageValue{0}.promoted(),RunnerImageValue{0xbf800000u}.promoted(),angle)||angle.high!=0x400921fbu)++failures;
    return failures||worst.load()*4>narrow_atan_detail::margin?1:0;
}
