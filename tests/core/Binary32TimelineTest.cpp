#include "core/Binary32Timeline.h"
#include "core/SequenceTimeline.h"
#include <cstdio>
#include <cstdlib>
// Binary32Timeline against SequenceTimeline, the runner's binary32 form:
// random signed rates of magnitude 1/16..frames from random binary32 starts,
// 1 to 15 frames. Positions, wraps and boundary ticks agree bit for bit.
static void checkAt(bool ok,int line){if(!ok){std::fprintf(stderr,"Binary32 timeline check failed at line %d\n",line);std::abort();}}
#define check(ok) checkAt((ok),__LINE__)
using downwell::Binary32Timeline;using downwell::RunnerImageValue;using downwell::SequenceTimeline;
static std::uint32_t state=12345;
static std::uint32_t next(){state=state*1664525u+1013904223u;return state;}
int main(){
    unsigned runs=0,steps=0,wraps=0,negative=0;
    for(unsigned run=0;run<200000;++run){
        const unsigned frames=1+next()%15;
        const std::int32_t length=std::int32_t(frames)*Binary32Timeline::one;
        // A rate: random binary32 bits with exponent in [2^-4, frames].
        std::uint32_t bits;std::int32_t rate;bool exact;
        do{bits=((123u+next()%6)<<23)|(next()&0x7fffffu)|(next()&0x80000000u);}
        while(!Binary32Timeline::fromBits(bits,rate,exact)||rate>length||rate< -length);
        check(exact);
        // A start: a random binary32 in [0, frames), exact in Q27.
        RunnerImageValue position{((119u+next()%8)<<23)|(next()&0x7fffffu)};
        std::int32_t q;
        if(!Binary32Timeline::fromBits(position.bits,q,exact)||!exact||q>=length)continue;
        ++runs;negative+=rate<0;
        for(unsigned tick=0;tick<64;++tick){
            bool crossed;
            check(SequenceTimeline::advance(position,RunnerImageValue{bits},frames,crossed));
            const bool fixed=Binary32Timeline::advance(q,rate,length);
            check(fixed==crossed&&Binary32Timeline::toBits(q)==(position.bits&0x7fffffffu)&&q>=0&&q<length);
            ++steps;wraps+=crossed;
        }
    }
    std::printf("Binary32 timeline: %u runs (%u negative rates), %u steps, %u wraps agree bit for bit\n",runs,negative,steps,wraps);
    check(runs>100000&&wraps>steps/20);
}
