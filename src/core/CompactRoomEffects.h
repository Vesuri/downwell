#ifndef DOWNWELL_CORE_COMPACT_ROOM_EFFECTS_H
#define DOWNWELL_CORE_COMPACT_ROOM_EFFECTS_H
#include "MovingFx.h"
#include "ScriptSpawnEffect.h"
#include "WhiteJellyStep.h"
#include "FixedPoint.h"
#include "Binary32Timeline.h"
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
    // One compact record for both families (W3 fixed point): position and
    // velocity in Q16, the binary32 animation timeline in Q27
    // (Binary32Timeline); x/y are the published whole pixels and
    // turn the nearest quarter turn of image_angle, fixed at spawn. The
    // registered owners keep ScriptSpawnEffect and MovingFx, their oracle.
    struct Effect {
        std::int32_t x16=0,y16=0,xsp16=0,ysp16=0;
        std::int32_t frame27=0,rate27=0,fullRate27=0,frames27=0;
        std::int32_t x=0,y=0;
        int spriteId=-1;
        unsigned turn=0;
        bool valid=true,destroyRequested=false;
        unsigned drawFrame()const{return unsigned(frame27>>Binary32Timeline::fraction);}
        // Runner animation: one image_speed step; reaching the last frame's
        // end wraps and requests Animation End.
        void advanceImage(){
            if(Binary32Timeline::advance(frame27,rate27,frames27))destroyRequested=true;
        }
    };
    struct NoLoop {std::uint32_t handle=0;Effect effect;};
    struct Moving {std::uint32_t handle=0;Effect effect;};
    struct Phase {std::uint32_t newest;};
    static bool frames(int sprite,unsigned& count){
        switch(sprite){
            case 84:count=9;return true;   // sprSmoke (TorchFire's puffs)
            case 118:count=8;return true;  // sprSuperFlash (opened gem chunk)
            case 571:count=5;return true;  // sprChunkSparkle1
            case 572:count=4;return true;  // sprChunkSparkle2
            case 111:count=8;return true;  // sprFxAmmoCharge (GunModule pickup)
            case 112:count=5;return true;  // sprFxImmuneSpark (bullet-immune hits)
            case 96:count=3;return true;   // sprFlashBall (laser hitWallFx)
            case 101:count=8;return true;  // sprFxBulletHitWall (parentPBullet hitWallFx)
            case 94:case 62094:count=4;return true; // sprJumpSmallerFx (landing/turn dust, both halves)
            case 95:count=4;return true;   // sprJumpFx (scrJump dust)
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
    bool noLoopFull()const{return noLoopCount_==NoLoopCapacity;}
    unsigned dropped()const{return dropped_;} // effects a full store discarded
    unsigned movingCount()const{return movingCount_;}
    const NoLoop& noLoop(unsigned index)const{return noLoop_[index];}
    const Moving& moving(unsigned index)const{return moving_[index];}
    void clear(){noLoopCount_=movingCount_=0;good_=true;} // Never rewind handles.
    // scrEffectSpawn: Create, then sprite, image_speed (binary32) and
    // image_angle; depth has no compact reader. Positions are Q16.
    bool spawnNoLoop(Fixed16 x,Fixed16 y,int sprite,RunnerImageValue speed,Fixed16 angle){
        unsigned count;
        if(!good_||!frames(sprite,count))return fail();
        // Both stores are cosmetic (no RNG, no gameplay reads): a full store
        // drops the new effect instead of ending the game.
        if(noLoopCount_==NoLoopCapacity){++dropped_;return true;}
        NoLoop record;record.handle=next_;
        auto& e=record.effect;
        e.x=Fixed16::roundQ16(x.v);e.y=Fixed16::roundQ16(y.v);
        if(!begin(e,sprite,count,speed,angle))return fail();
        noLoop_[noLoopCount_++]=record;++next_;return true;
    }
    // emitMovingFx: Create, sprite, image_speed, image_angle, then the
    // lengthdir velocity components (Q16, as WhiteJellyStep's).
    bool spawnMoving(Fixed16 x,Fixed16 y,int sprite,RunnerImageValue imageSpeed,Fixed16 angle,Fixed16 speed){
        unsigned count;Moving record;record.handle=next_;
        auto& e=record.effect;
        if(!good_||!frames(sprite,count))return fail();
        if(movingCount_==MovingCapacity){++dropped_;return true;}
        if(!begin(e,sprite,count,imageSpeed,angle)||
           speed.v<0||speed.v>3*65536||angle.v<0||angle.v>360*65536)return fail();
        e.x16=x.v;e.y16=y.v;e.x=Fixed16::roundQ16(x.v);e.y=Fixed16::roundQ16(y.v);
        e.xsp16=FixedTrig::lengthdir(false,speed.v,angle.v);
        e.ysp16=FixedTrig::lengthdir(true,speed.v,angle.v);
        moving_[movingCount_++]=record;++next_;return true;
    }
    // Runner animation precedes Begin Step. Animation End destroys both.
    bool advanceAnimation(Phase phase){
        if(!good_)return false;
        for(unsigned i=0;i<noLoopCount_;++i){auto& r=noLoop_[i];
            if(r.handle<=phase.newest&&!r.effect.destroyRequested)r.effect.advanceImage();
        }
        for(unsigned i=0;i<movingCount_;++i){auto& r=moving_[i];
            if(r.handle<=phase.newest&&!r.effect.destroyRequested)r.effect.advanceImage();
        }
        retire();return true;
    }
    // parentMovingFx Step. bound(effect,moving) answers TimeStopBound; a
    // stopped effect's image_speed is 0 until it moves again. roundPosition
    // republishes x/y even while paused.
    template<class Bound> bool stepMoving(Phase phase,Bound bound){
        if(!good_)return false;
        constexpr std::int32_t range=std::int32_t(1)<<30;
        for(unsigned i=0;i<movingCount_;++i){auto& r=moving_[i];auto& e=r.effect;
            if(r.handle>phase.newest||e.destroyRequested)continue;
            bool moving;if(!bound(static_cast<const Effect&>(e),moving))return fail();
            e.rate27=moving?e.fullRate27:0;
            if(moving){
                e.x16+=e.xsp16;e.y16+=e.ysp16;
                if(e.x16>=range||e.x16<=-range||e.y16>=range||e.y16<=-range)return fail();
            }
            e.y=Fixed16::roundQ16(e.y16);e.x=Fixed16::roundQ16(e.x16);
        }
        return true;
    }
    bool stepMoving(Phase phase){
        return stepMoving(phase,[](const Effect&,bool& moving){moving=true;return true;});
    }
private:
    bool fail(){good_=false;return false;}
    // Sprite, binary32 image_speed and the angle's nearest quarter turn.
    static bool begin(Effect& e,int sprite,unsigned count,RunnerImageValue rate,Fixed16 angle){
        if(count>Binary32Timeline::maximumFrames)return false;
        e.spriteId=sprite;e.frames27=std::int32_t(count)*Binary32Timeline::one;
        if(!Binary32Timeline::rate(rate.bits,e.rate27)||e.rate27>e.frames27)return false;
        std::int32_t degrees=Fixed16::roundQ16(angle.v);
        e.fullRate27=e.rate27;
        degrees%=360;if(degrees<0)degrees+=360;
        e.turn=unsigned(std::uint16_t(degrees+45)/90)&3u;
        return true;
    }
    unsigned dropped_=0;
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
