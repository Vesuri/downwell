#ifndef DOWNWELL_CORE_TIME_SHARD_ANIMATION_H
#define DOWNWELL_CORE_TIME_SHARD_ANIMATION_H
#include "TimeShardMotion.h"
#include "UnitSequenceFrames.h"
namespace downwell {
// Sprite 91 has six consecutive unit keys. Create nevertheless writes random(7).
struct TimeShardAnimation : TimeShardMotion {
    RunnerImageValue timeline{};
    template<class Random> bool create(MotionBinary64 px,MotionBinary64 py,bool lowSpec,Random& random){
        if(!TimeShardMotion::create(px,py,lowSpec,random))return false;
        timeline={};return true;
    }
    // Restricted to this object's nonnegative [0,7] writes. Source inverse
    // mapping: float(index/6), fractional cycle * 6, near-key snap, key time.
    static bool inverse(RunnerImageValue index,RunnerImageValue& result){
        if(!SequenceTimeline::valid(index) || (index.bits&0x80000000u) || index.bits>0x40e00000u)return false;
        if(!index.bits){result={};return true;}
        unsigned exponent=(index.bits>>23)&255u;
        const unsigned mantissa=(index.bits&0x7fffffu)|0x800000u;
        const unsigned shift=mantissa<0xc00000u?3:2;
        if(exponent<=shift)return false;
        const unsigned numerator=mantissa<<shift;
        unsigned quotient=numerator/6, remainder=numerator%6;
        if(remainder>3 || (remainder==3 && (quotient&1)))++quotient;
        exponent-=shift;
        if(quotient==0x1000000u){quotient>>=1;++exponent;}
        RunnerImageValue cycles{(exponent<<23)|(quotient&0x7fffffu)};
        const bool whole=cycles.bits>=0x3f800000u;
        auto fractional=cycles.promoted();
        if(whole && !fractional.add(MotionBinary64::fromInteger(-1)))return false;
        MotionBinary64 product;RunnerImageValue position;
        if(!multiplyBinary64(fractional,MotionBinary64::fromInteger(6),product) || !position.assign(product))return false;
        unsigned key=0;
        const unsigned e=(position.bits>>23)&255u;
        if(e>=127)key=((position.bits&0x7fffffu)|0x800000u)>>(150-e);
        auto fraction=position.promoted();
        if(!fraction.add(MotionBinary64::fromInteger(-int(key))))return false;
        auto near=fraction;
        if(!near.add({0x3ee4f8b5u,0x88e368f1u}))return false;
        if(near.high>=0x3ff00000u){++key;fraction={};}
        if(key>5)key=5;
        if(!fraction.add(MotionBinary64::fromInteger(int(key)+(whole?6:0))))return false;
        return result.assign(fraction);
    }
    bool advanceImage(){
        if(destroyRequested)return true;
        RunnerImageValue evaluated;bool reconcile=false,crossed=false;
        if(!UnitSequenceFrames::evaluate(timeline,6,evaluated) ||
           !UnitSequenceFrames::needsReconcile(imageIndex,evaluated,reconcile))return false;
        auto candidate=timeline;
        if(reconcile){
            if(!inverse(imageIndex,candidate))return false;
            // set-sequence-position (0x50f9e0) normalizes before the ordinary
            // speed addition. Combining both wraps changes float rounding.
            if(candidate.bits>=0x40c00000u && !SequenceTimeline::wrap(candidate,6,candidate))return false;
        }
        if(!UnitSequenceFrames::advance(candidate,imageSpeed,6,evaluated,crossed))return false;
        timeline=candidate;imageIndex=evaluated;
        // No Animation End event: crossing a boundary does not retire a shard.
        return true;
    }
};
}
#endif
