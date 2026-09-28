#ifndef DOWNWELL_CORE_DRONE_BULLET_SOURCE_STEP_H
#define DOWNWELL_CORE_DRONE_BULLET_SOURCE_STEP_H
#include "MachineGunSourceMotion.h"
#include "RunnerRealComparison.h"
namespace downwell {
// Object 266 (also spawned by Gunpowder Blocks), not bulletExplosion.
// Shared event arithmetic; room identity, inherited hit-memory storage,
// collision dispatch and animation scheduling belong to the world owner.
struct DroneBulletSourceStep {
    MachineGunSourceMotion motion;
    MotionBinary64 speed=MotionBinary64::fromInteger(7);
    MotionBinary64 damage=MotionBinary64::fromInteger(4),spread{};
    MotionBinary64 decline{0x3fe99999u,0x9999999au}; // source double 0.8
    RunnerImageValue imageSpeed{};
    bool decelerate=false,animation=false;
    int alarm0=16;
    static constexpr int objectId=266,spriteId=459,hitWallFx=101;
    bool reset(MotionBinary64 x,MotionBinary64 y){
        DroneBulletSourceStep next;
        if(!next.motion.reset(x,y))return false;
        *this=next;return true;
    }
    void alarmZero(){decelerate=true;}
    bool drawable()const{return motion.initialized;}
    template<class Random,class Ops>
    bool step(Random& random,Ops& ops,bool& good){
        if(!good)return false;
        auto check=[&](bool ok){good=good&&ok;return good;};
        auto vector=[&](){return SmokeDirection::vector(speed,motion.direction,motion.xsp,motion.ysp,smokeTrig);};
        auto angle=[&](){
            RunnerImageValue next;
            if(!next.assign(motion.direction)||!ops.publishAngle(next))return false;
            motion.imageAngle=next;return true;
        };
        if(!motion.initialized){
            // random_range(0,0) still consumes a word. The box overrides
            // bDir/bdmg AFTER synchronous Create and BEFORE this first Step.
            auto width=spread;auto negative=spread;negative.high^=0x80000000u;
            MotionBinary64 delta;
            if(!check(width.add(spread)&&randomBinary64(random.nextU32(),width,delta)&&
                delta.add(negative)&&motion.direction.add(delta))||!check(vector())||!check(angle()))return false;
            motion.initialized=true;
        }
        if(decelerate){
            MotionBinary64 next;
            if(!check(multiplyBinary64(speed,decline,next)))return false;
            speed=next;
        }
        if(!check(angle()))return false;
        int order=0;
        if(!check(compareRunnerReal(speed,MotionBinary64::fromInteger(3),order)))return false;
        if(order<0&&!animation){
            animation=true;
            const RunnerImageValue next{0x3f000000u};
            if(!check(ops.publishImageSpeed(next)))return false;
            imageSpeed=next;
        }
        if(!check(vector())||!check(ops.solid(*this)))return false;
        // Destruction requested by scrBulCheckSolid does not end this event.
        return check(motion.commitPosition([&](auto x,auto y){return ops.publishPosition(x,y);}));
    }
    template<class Ops> bool endStep(Ops& ops,bool& good){
        if(!good)return false;
        if(!ops.outOfView()||!good)return good=false;
        const bool ok=ops.bulletOutOfView();return good=good&&ok;
    }
    template<class Ops> bool animationEnd(Ops& ops,bool& good){
        if(!good)return false;
        const bool ok=ops.destroySelf();return good=good&&ok;
    }
};
}
#endif
