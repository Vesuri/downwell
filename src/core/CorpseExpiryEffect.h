#ifndef DOWNWELL_CORE_CORPSE_EXPIRY_EFFECT_H
#define DOWNWELL_CORE_CORPSE_EXPIRY_EFFECT_H
#include "RunnerSpriteChange.h"
#include "UnitSequenceFrames.h"
namespace downwell {
// Object 242 (fxJobutsu), parent 219 (parentNoloopFx). No Step event,
// TimeStopBound, movement or Create RNG. Ordinary sprite drawing, not drawFx.
struct CorpseExpiryEffect {
    RunnerImageValue x{},y{},timeline{},imageIndex{},imageSpeed{0x3e99999au};
    int spriteId=102;
    unsigned frames=12;
    bool valid=true,destroyRequested=false;
    bool create(MotionBinary64 px,MotionBinary64 py){
        CorpseExpiryEffect next;
        next.valid=next.x.assign(px)&&next.y.assign(py);
        *this=next;return valid;
    }
    // Caller supplies verified normal, consecutive unit-key sprite metadata.
    // Preserve source setter semantics and publish changed collision bounds.
    template<class Publish> bool setSprite(int id,unsigned count,Publish publish){
        if(!valid)return false;
        RunnerSpriteChange::Result change;
        if(!count || !RunnerSpriteChange::plan(spriteId,id,imageIndex,count,change)){
            valid=false;return false;
        }
        if(!change.changed)return true;
        spriteId=id;frames=count;
        if(change.resetIndex)imageIndex={};
        if(change.resetTimeline)timeline={};
        valid=publish(*this);return valid;
    }
    // Animation is scheduled before Begin Step. Parent Other 7 kills the
    // instance on the first wrap; no separate age counter or freeze branch.
    bool advanceImage(){
        if(!valid)return false;
        if(destroyRequested)return true;
        bool crossed=false;
        valid=UnitSequenceFrames::advance(timeline,imageSpeed,frames,imageIndex,crossed);
        if(valid&&crossed)destroyRequested=true;
        return valid;
    }
};
}
#endif
