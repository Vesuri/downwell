#ifndef DOWNWELL_CORE_BLAST_ACTOR_H
#define DOWNWELL_CORE_BLAST_ACTOR_H
#include "ProjectileHitMemory.h"
#include "ScreenShakeCaller.h"
#include "SkeletonEffectBursts.h"
#include "SoundPlayScript.h"
#include "UnitSequenceFrames.h"
#include "DivideBinary64.h"
#include "RunnerRealComparison.h"
namespace downwell {
// Object 276 (parent 89), sprite 471. The owner publishes transforms and
// images and schedules mixed phases. No time-stop guard exists in these events.
template<unsigned HitCapacity> struct BlastActor {
    MotionBinary64 damage{},speed{},direction{},range{},xsp{},ysp{};
    bool allSet=false,bounce=false,consistent=false,wave=false,explode=false;
    int hitWallFx=101;
    ProjectileHitMemory<HitCapacity> hits;
    ScreenShakeCaller shake;
    SoundPlayState sound;
    SmokeBurstCaller flashCaller;
    std::uint32_t myFx=0; // scrEffectSpawn caller field, never a local pool handle.
    RunnerImageValue timeline{},imageIndex{},imageSpeed{0x3f800000u},xscale{0x3f800000u};
    bool damageEnd=false,destroyRequested=false,disregardWall=false,disregardWallWritten=false;
    MotionBinary64 damageEndFrame{};
    bool drawFrame(unsigned& frame)const{
        const auto magnitude=imageIndex.bits&0x7fffffffu;
        if(destroyRequested||magnitude>0x41580000u||((imageIndex.bits>>31)&&magnitude))return false;
        const unsigned exponent=magnitude>>23;
        frame=exponent<127?0:((magnitude&0x7fffffu)|0x800000u)>>(150-exponent);
        return frame<14;
    }
    template<class Events> bool create(Events& events,bool& good){
        if(!good)return false;
        // event_inherited: parentPBullet Create, followed by child assignments.
        allSet=false;hitWallFx=101;damage={};speed={};direction={};bounce=false;
        consistent=false;wave=false;range={};explode=false;hits.count=0;hits.ids[0]=-1;
        hits.hitIgnore=false;xsp={};ysp={};
        damage=MotionBinary64::fromInteger(50);direction=MotionBinary64::fromInteger(270);
        consistent=true;wave=true;allSet=true;damageEnd=false;damageEndFrame=MotionBinary64::fromInteger(3);
        if(!shake.execute(6,8,[&](int a,int d){return events.shake(a,d);})||!events.momentDelay())return good=false;
        imageIndex={};if(!events.imageIndex(imageIndex))return good=false;
        imageSpeed={0x3f000000u};if(!events.imageSpeed(imageSpeed))return good=false;
        unsigned choice=0;if(!events.choose(2,choice)||choice>1)return good=false;
        xscale={choice?0xbf800000u:0x3f800000u};
        return events.xscale(xscale)||(good=false);
    }
    template<class Events> bool advanceAnimation(Events& events,bool& good){
        if(!good)return false;bool crossed=false;
        if(!UnitSequenceFrames::advance(timeline,imageSpeed,14,imageIndex,crossed)||
           !events.imageIndex(imageIndex))return good=false;
        if(crossed){destroyRequested=true;if(!events.destroySelf())return good=false;}
        return true;
    }
    bool step(bool& good){
        if(!good)return false;int order;
        if(!compareRunnerReal(imageIndex.promoted(),damageEndFrame,order))return good=false;
        if(order>0)damageEnd=true;
        return true;
    }
    void alarm0(){disregardWall=false;disregardWallWritten=true;}
    bool playerContact(bool& good){return good;} // Source has no body after its IF.
    template<class Events> bool wallContact(Events& events,bool& good){
        if(!good)return false;if(damageEnd)return true;
        int parent=0;if(!events.directParent(parent))return good=false;
        if(parent!=84)return true;
        std::uint32_t id=0;if(!events.targetId(id)||!hits.check(id,good))return good=false;
        if(hits.hitIgnore)return true;
        MotionBinary64 hp;int order;
        if(!events.wallHealth(hp)||!compareRunnerReal(hp,{},order))return good=false;
        if(order<=0)return true;
        if(!events.wallHealth(hp)||!subtractDamage(hp)||!events.setWallHealth(hp))return good=false;
        return true;
    }
    template<class Events> bool shootableContact(Events& events,bool& good){
        if(!good)return false;if(damageEnd)return true;
        std::uint32_t id=0;if(!events.targetId(id)||!hits.check(id,good))return good=false;
        if(hits.hitIgnore)return true;
        MotionBinary64 hp;int order;
        if(!events.objectHealth(hp)||!compareRunnerReal(hp,{},order))return good=false;
        if(order<=0)return true;
        if(!events.objectHealth(hp)||!subtractDamage(hp)||!events.setObjectHealth(hp))return good=false;
        const auto y=events.callerY(),x=events.callerX();
        return events.flash(x,y,1,0,0)||(good=false);
    }
    template<class Events> bool enemyContact(Events& events,bool& good){
        if(!good)return false;if(damageEnd)return true;
        bool flag=false;if(!events.bulletThrough(flag))return good=false;if(flag)return true;
        if(!events.bulletImmune(flag))return good=false;if(flag)return true;
        MotionBinary64 hp;int order;
        if(!events.enemyHealth(hp)||!compareRunnerReal(hp,{},order))return good=false;
        if(order<=0)return true;
        hits.hitIgnore=false;
        if(consistent){std::uint32_t id=0;if(!events.targetId(id)||!hits.check(id,good))return good=false;}
        if(!consistent){
            if(explode){const auto y=events.callerY(),x=events.callerX();if(!events.createExplosion(272,x,y))return good=false;}
            destroyRequested=true;if(!events.destroySelf())return good=false;
        }
        if(hits.hitIgnore)return true;
        if(!events.setEnemyHit()||!events.setEnemyDirection(direction)||!events.enemyHealth(hp)||
           !subtractDamage(hp)||!events.setEnemyHealth(hp)||!events.setEnemyStun()||
           !events.momentDelay()||!events.hitSound(29,90,false,true))return good=false;
        MotionBinary64 impact;
        if(!divideBinary64(damage,MotionBinary64::fromInteger(5),impact)||!events.setEnemyImpact(impact))return good=false;
        const auto effectDirection=direction;
        const auto y=events.callerY(),x=events.callerX();
        if(!events.impactEffect(x,y,605,MotionBinary64{0x3fd99999u,0x9999999au},effectDirection,0)||
           !events.momentDelay())return good=false;
        return true;
    }
private:
    bool subtractDamage(MotionBinary64& value){auto negative=damage;negative.high^=0x80000000u;return value.add(negative);}
};
}
#endif
