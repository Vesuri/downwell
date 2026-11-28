#include "core/EffectSequenceAnimation.h"
#include "core/MultiplyBinary64.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
// EffectSequenceAnimation::inverse in binary32 against its binary64 form
// (the VM's arithmetic, kept here as the oracle). Every one of the
// 2,176,843,778 inputs (indices 0..7 for 4 and 6 frames) matched when this
// was written; the suite tests every 61st index and the edges.
static void checkAt(bool ok,int line){if(!ok){std::fprintf(stderr,"Sequence inverse check failed at line %d\n",line);std::abort();}}
#define check(ok) checkAt((ok),__LINE__)
namespace downwell {
struct Reference {
    static bool inverse(RunnerImageValue index,unsigned frames,RunnerImageValue& result){
        if((frames!=4&&frames!=6)||!SequenceTimeline::valid(index) || (index.bits&0x80000000u) || index.bits>0x40e00000u)return false;
        if(!index.bits){result={};return true;}
        unsigned exponent=(index.bits>>23)&255u;
        const unsigned mantissa=(index.bits&0x7fffffu)|0x800000u;
        const unsigned shift=(mantissa<<2)<frames*0x800000u?3:2;
        if(exponent<=shift)return false;
        const unsigned numerator=mantissa<<shift;
        unsigned quotient=numerator/frames, remainder=numerator%frames;
        if(remainder>frames/2 || (remainder==frames/2 && (quotient&1)))++quotient;
        exponent-=shift;
        if(quotient==0x1000000u){quotient>>=1;++exponent;}
        RunnerImageValue cycles{(exponent<<23)|(quotient&0x7fffffu)};
        const bool whole=cycles.bits>=0x3f800000u;
        auto fractional=cycles.promoted();
        if(whole && !fractional.add(MotionBinary64::fromInteger(-1)))return false;
        MotionBinary64 product;RunnerImageValue position;
        if(!multiplyBinary64(fractional,MotionBinary64::fromInteger(frames),product) || !position.assign(product))return false;
        unsigned key=0;
        const unsigned e=(position.bits>>23)&255u;
        if(e>=127)key=((position.bits&0x7fffffu)|0x800000u)>>(150-e);
        auto fraction=position.promoted();
        if(!fraction.add(MotionBinary64::fromInteger(-int(key))))return false;
        auto near=fraction;
        if(!near.add({0x3ee4f8b5u,0x88e368f1u}))return false;
        if(near.high>=0x3ff00000u){++key;fraction={};}
        if(key>=frames)key=frames-1;
        if(!fraction.add(MotionBinary64::fromInteger(int(key)+(whole?int(frames):0))))return false;
        return result.assign(fraction);
    }
};
}
int main(){
    unsigned long long tested=0,accepted=0;
    const auto test=[&](std::uint32_t bits){
        for(unsigned frames:{3u,4u,6u}){
            downwell::RunnerImageValue in{bits},a{0x12345678u},b{0x12345678u};
            const bool oa=downwell::Reference::inverse(in,frames,a),ob=downwell::EffectSequenceAnimation::inverse(in,frames,b);
            check(oa==ob&&(!oa||a.bits==b.bits));
            ++tested;accepted+=oa;
        }
    };
    for(std::uint32_t bits=0;bits<=0x40e00000u;bits+=61)test(bits);
    for(std::uint32_t bits:{0u,1u,0x007fffffu,0x00800000u,0x3f7fff58u,0x3f7fff59u,0x3f800000u,0x40800000u,
                             0x40c00000u,0x40dfffffu,0x40e00000u,0x40e00001u,0x80000000u,0xbf800000u,0x7f800000u})test(bits);
    std::printf("Sequence inverse: %llu inputs equal to the binary64 form (%llu accepted)\n",tested,accepted);
    check(accepted*2>tested);
}
