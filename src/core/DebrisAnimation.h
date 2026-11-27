#ifndef DOWNWELL_CORE_DEBRIS_ANIMATION_H
#define DOWNWELL_CORE_DEBRIS_ANIMATION_H
#include "DebrisMotion.h"
#include "EffectSequenceAnimation.h"
namespace downwell {
struct DebrisAnimation : DebrisMotion {
    using Position=MotionBinary64;
    RunnerImageValue timeline{};
    template<class Random> bool create(MotionBinary64 x,MotionBinary64 py,int area,Random& random){
        if(!DebrisMotion::create(x,py,area,random))return false;
        timeline={};return true;
    }
    bool advanceImage(){
        if(destroyRequested)return true;
        const unsigned frames=spriteId==91?6:spriteId==115||spriteId==116?4:0;
        // No Animation End handler for object 370.
        return EffectSequenceAnimation::advance(timeline,imageSpeed,frames,imageIndex);
    }
};
}
#endif
