#ifndef DOWNWELL_CORE_EFFECT_SEQUENCE_ANIMATION_H
#define DOWNWELL_CORE_EFFECT_SEQUENCE_ANIMATION_H
#include "MultiplyBinary64.h"
#include "UnitSequenceFrames.h"
namespace downwell {
// Four/six-key debris and shard sequences with initial random(7) index writes.
struct EffectSequenceAnimation {
    // Restricted to nonnegative [0,7] writes and the verified unit-key tracks.
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
    static bool advance(RunnerImageValue& timeline,RunnerImageValue imageSpeed,unsigned frames,RunnerImageValue& imageIndex){
        if(frames!=4&&frames!=6)return false;
        RunnerImageValue evaluated;bool reconcile=false,crossed=false;
        if(!UnitSequenceFrames::evaluate(timeline,frames,evaluated) ||
           !UnitSequenceFrames::needsReconcile(imageIndex,evaluated,reconcile))return false;
        auto candidate=timeline;
        if(reconcile){
            if(!inverse(imageIndex,frames,candidate))return false;
            // set-sequence-position (0x50f9e0) normalizes before the ordinary
            // speed addition. Combining both wraps changes float rounding.
            const unsigned bound=frames==4?0x40800000u:0x40c00000u;
            if(candidate.bits>=bound && !SequenceTimeline::wrap(candidate,frames,candidate))return false;
        }
        if(!UnitSequenceFrames::advance(candidate,imageSpeed,frames,evaluated,crossed))return false;
        timeline=candidate;imageIndex=evaluated;
        return true;
    }
};
}
#endif
