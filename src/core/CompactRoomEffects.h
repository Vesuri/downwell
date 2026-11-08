#ifndef DOWNWELL_CORE_COMPACT_ROOM_EFFECTS_H
#define DOWNWELL_CORE_COMPACT_ROOM_EFFECTS_H
#include "MovingFx.h"
#include "ScriptSpawnEffect.h"
#include "WhiteJellyStep.h"
namespace downwell {
// Compact owners for the native world's script-spawned effects:
// parentNoloopFx (219, scrEffectSpawn) and parentMovingFx (220,
// emitMovingFx). Neither Create draws RNG. Records stay in birth order and
// the world captures one cutoff per global phase, as for smoke and flash.
// Only the reviewed sprites below are admitted; anything else fails.
template<unsigned NoLoopCapacity,unsigned MovingCapacity>
class CompactRoomEffects {
    static_assert(NoLoopCapacity>0&&MovingCapacity>0,"compact effect storage must be nonempty");
public:
    using Real=MotionBinary64;
    struct NoLoop {std::uint32_t handle=0;ScriptSpawnEffect effect;};
    struct Moving {std::uint32_t handle=0;MovingFx effect;};
    struct Phase {std::uint32_t newest;};
    static bool frames(int sprite,unsigned& count){
        switch(sprite){
            case 84:count=9;return true;   // sprSmoke (TorchFire's puffs)
            case 118:count=8;return true;  // sprSuperFlash (opened gem chunk)
            case 571:count=5;return true;  // sprChunkSparkle1
            case 572:count=4;return true;  // sprChunkSparkle2
            case 111:count=8;return true;  // sprFxAmmoCharge (GunModule pickup)
            case 601:count=12;return true; // sprFxExMuzzleSmoke (gem-high laser muzzle)
            case 602:count=1;return true;  // sprFxBoingRed (gem-high muzzle)
            case 603:count=1;return true;  // sprFxBoingball (box breaks, usual muzzle)
            case 605:count=3;return true;  // sprFxImpactSplosion (shootable hits)
            case 704:count=6;return true;  // sprFxRedSplash (steering deaths)
            default:count=0;return false;
        }
    }
    bool valid()const{return good_;}
    Phase beginPhase()const{return {next_-1};}
    unsigned size()const{return noLoopCount_+movingCount_;}
    unsigned noLoopCount()const{return noLoopCount_;}
    unsigned movingCount()const{return movingCount_;}
    const NoLoop& noLoop(unsigned index)const{return noLoop_[index];}
    const Moving& moving(unsigned index)const{return moving_[index];}
    void clear(){noLoopCount_=movingCount_=0;good_=true;} // Never rewind handles.
    // scrEffectSpawn: Create, then sprite, image_speed, image_angle, depth.
    bool spawnNoLoop(Real x,Real y,int sprite,Real speed,Real angle,Real depth){
        unsigned count;
        if(!good_||noLoopCount_==NoLoopCapacity||!frames(sprite,count))return fail();
        NoLoop record;record.handle=next_;
        auto& e=record.effect;
        const auto accept=[](const auto&){return true;};
        if(!e.create(x,y)||!e.setSprite(sprite,count,accept)||
           !e.setValue(e.imageSpeed,speed,accept)||!e.setValue(e.angle,angle,accept)||
           !e.setValue(e.depth,depth,accept))return fail();
        noLoop_[noLoopCount_++]=record;++next_;return true;
    }
    // emitMovingFx: Create, sprite, image_speed, image_angle, then the
    // lengthdir velocity components.
    bool spawnMoving(Real x,Real y,int sprite,Real imageSpeed,Real angle,Real speed){
        unsigned count;Moving record;record.handle=next_;
        auto& e=record.effect;RunnerImageValue px,py,rate,turn;Real component;
        if(!good_||movingCount_==MovingCapacity||!frames(sprite,count)||
           !px.assign(x)||!py.assign(y)||!e.create(px,py)||
           !e.setSprite(sprite,count,[](const auto&){return true;})||
           !rate.assign(imageSpeed)||!turn.assign(angle))return fail();
        e.imageSpeed=rate;e.angle=turn;
        if(!WhiteJellyStep::movementComponent(false,speed,angle,component))return fail();
        e.xsp=component;
        if(!WhiteJellyStep::movementComponent(true,speed,angle,component))return fail();
        e.ysp=component;
        moving_[movingCount_++]=record;++next_;return true;
    }
    // Runner animation precedes Begin Step. Animation End destroys both.
    bool advanceAnimation(Phase phase){
        if(!good_)return false;
        for(unsigned i=0;i<noLoopCount_;++i){auto& r=noLoop_[i];
            if(r.handle<=phase.newest&&!r.effect.advanceImage())return fail();
        }
        for(unsigned i=0;i<movingCount_;++i){auto& r=moving_[i];
            if(r.handle<=phase.newest&&!r.effect.advanceImage())return fail();
        }
        retire();return true;
    }
    // parentMovingFx Step. bound(effect,moving) answers TimeStopBound.
    template<class Bound> bool stepMoving(Phase phase,Bound bound){
        if(!good_)return false;
        struct Ops {
            Bound& bound;const MovingFx* effect;
            bool timeStopBound(bool& moving){return bound(*effect,moving);}
            bool imageSpeed(RunnerImageValue value,RunnerImageValue& accepted){accepted=value;return true;}
            bool position(bool,RunnerImageValue value,RunnerImageValue& accepted){accepted=value;return true;}
        } ops{bound,nullptr};
        for(unsigned i=0;i<movingCount_;++i){auto& r=moving_[i];
            ops.effect=&r.effect;
            if(r.handle<=phase.newest&&!r.effect.destroyRequested&&!r.effect.step(ops))return fail();
        }
        return true;
    }
    bool stepMoving(Phase phase){
        return stepMoving(phase,[](const MovingFx&,bool& moving){moving=true;return true;});
    }
private:
    bool fail(){good_=false;return false;}
    void retire(){
        unsigned kept=0;
        for(unsigned i=0;i<noLoopCount_;++i)if(!noLoop_[i].effect.destroyRequested)noLoop_[kept++]=noLoop_[i];
        noLoopCount_=kept;kept=0;
        for(unsigned i=0;i<movingCount_;++i)if(!moving_[i].effect.destroyRequested)moving_[kept++]=moving_[i];
        movingCount_=kept;
    }
    NoLoop noLoop_[NoLoopCapacity];
    Moving moving_[MovingCapacity];
    unsigned noLoopCount_=0,movingCount_=0;
    std::uint32_t next_=1;
    bool good_=true;
};
}
#endif
