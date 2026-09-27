#ifndef DOWNWELL_CORE_NON_LOOP_SPRITE_ANIMATION_H
#define DOWNWELL_CORE_NON_LOOP_SPRITE_ANIMATION_H
#include "RunnerSpriteChange.h"
#include "UnitSequenceFrames.h"
namespace downwell {
// Shared parentNoloopFx animation for ordinary consecutive unit-key sprites.
// Parent births start unsprited; callers install a verified nonempty sprite
// before advancing this admitted path. Owners capture the global animation
// cutoff and publish/retire states at event boundaries.
struct NonLoopSpriteAnimation {
    RunnerImageValue timeline{},imageIndex{},imageSpeed{0x3f800000u};
    int spriteId=-1;
    unsigned frames=0;
    bool valid=true,destroyRequested=false;
    template<class Publish> bool setSprite(int id,unsigned count,Publish publish){
        if(!valid)return false;
        RunnerSpriteChange::Result change;
        // Resource validation is the caller's responsibility. Empty assigned
        // resources remain outside this admitted non-loop animation path.
        if(!count||!RunnerSpriteChange::plan(spriteId,id,imageIndex,count,change)){
            valid=false;return false;
        }
        if(!change.changed)return true;
        spriteId=id;frames=count;
        if(change.resetIndex)imageIndex={};
        if(change.resetTimeline)timeline={};
        valid=publish(*this);return valid;
    }
    bool advanceImage(){
        if(!valid)return false;
        if(destroyRequested)return true;
        bool crossed=false;
        valid=UnitSequenceFrames::advance(timeline,imageSpeed,frames,imageIndex,crossed);
        if(valid&&crossed)destroyRequested=true;
        return valid;
    }
    unsigned drawFrame()const{
        const unsigned exponent=(imageIndex.bits>>23)&255u;
        return exponent<127?0:((imageIndex.bits&0x7fffffu)|0x800000u)>>(150u-exponent);
    }
};
}
#endif
