#ifndef DOWNWELL_CORE_TIME_SHARD_ANIMATION_H
#define DOWNWELL_CORE_TIME_SHARD_ANIMATION_H
#include "TimeShardMotion.h"
#include "EffectSequenceAnimation.h"
namespace downwell {
struct TimeShardAnimation : TimeShardMotion {
    RunnerImageValue timeline{};
    template<class Random> bool create(MotionBinary64 px,MotionBinary64 py,bool lowSpec,Random& random){
        if(!TimeShardMotion::create(px,py,lowSpec,random))return false;
        timeline={};return true;
    }
    static bool inverse(RunnerImageValue index,RunnerImageValue& result){
        return EffectSequenceAnimation::inverse(index,6,result);
    }
    bool advanceImage(){
        // No Animation End handler for object 238.
        return destroyRequested||EffectSequenceAnimation::advance(timeline,imageSpeed,6,imageIndex);
    }
};
}
#endif
