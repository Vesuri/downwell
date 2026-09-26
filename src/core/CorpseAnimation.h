#ifndef DOWNWELL_CORE_CORPSE_ANIMATION_H
#define DOWNWELL_CORE_CORPSE_ANIMATION_H
#include "CorpseMotion.h"
#include "RunnerSpriteChange.h"
#include "UnitSequenceFrames.h"
namespace downwell {
// Initial frog-dead sprite and skeleton-dead override: both are verified
// one-key, unit-rate tracks. Other corpse sprite families require admission.
struct CorpseAnimation : CorpseMotion {
    int spriteId=172;
    RunnerImageValue imageIndex{},timeline{},imageXscale{0x3f800000u};
    template<class Random,class Queries>
    bool create(MotionBinary64 px,MotionBinary64 py,bool rip,Random& random,Queries& queries){
        spriteId=172;imageIndex={};timeline={};imageXscale={0x3f800000u};
        return CorpseMotion::create(px,py,rip,random,queries);
    }
    // Publication callback updates the live sprite/collision cache. A same-ID
    // assignment is a no-op, not a dirty bound or timeline reset.
    template<class Publish> bool setSprite(int next,Publish publish){
        if(!valid)return false;
        if(next!=172&&next!=206){valid=false;return false;}
        RunnerSpriteChange::Result change;
        if(!RunnerSpriteChange::plan(spriteId,next,imageIndex,1,change)){valid=false;return false;}
        if(!change.changed)return true;
        spriteId=next;
        if(change.resetIndex)imageIndex={};
        if(change.resetTimeline)timeline={};
        if(!publish(*this)){valid=false;return false;}
        return true;
    }
    bool advanceImage(){
        if(!valid)return false;
        if(destroyRequested)return true;
        RunnerImageValue evaluated;
        bool reconcile=false,crossed=false;
        if(!UnitSequenceFrames::evaluate(timeline,1,evaluated)||
           !UnitSequenceFrames::needsReconcile(imageIndex,evaluated,reconcile)){valid=false;return false;}
        // Source Create writes no index. Admit explicit zero writes only;
        // nonzero external writes need a separately verified inverse mapping.
        auto candidate=timeline;
        if(reconcile){
            if(imageIndex.bits&0x7fffffffu){valid=false;return false;}
            candidate={};
        }
        if(!UnitSequenceFrames::advance(candidate,imageSpeed,1,evaluated,crossed)){valid=false;return false;}
        timeline=candidate;imageIndex=evaluated;
        return true; // No Animation End event; wrapping does not retire.
    }
};
}
#endif
