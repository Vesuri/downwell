#ifndef DOWNWELL_CORE_DRONE_BULLET_VERIFICATION_H
#define DOWNWELL_CORE_DRONE_BULLET_VERIFICATION_H
#include "DroneBulletSourceStep.h"
namespace downwell {
// Event tape, not room/renderer admission. Also executed on the 68000.
struct DroneBulletVerification {
    struct Random {unsigned draws=0;std::uint32_t nextU32(){++draws;return 0xdeadbeefu;}};
    struct Ops {
        unsigned calls=0,fail=0,angles=0,speeds=0,positions=0,solids=0,outCalls=0,bulletOutCalls=0,kills=0;
        bool destroyed=false;
        bool call(){return ++calls!=fail;}
        bool publishAngle(RunnerImageValue angle){++angles;return angle.bits==0x42b40000u&&call();}
        bool publishImageSpeed(RunnerImageValue speed){++speeds;return speed.bits==0x3f000000u&&call();}
        bool publishPosition(RunnerImageValue,RunnerImageValue){++positions;return call();}
        bool solid(DroneBulletSourceStep& shot){
            ++solids;destroyed=true;
            return shot.motion.initialized&&shot.damage.high==0x40340000u&&call();
        }
        bool outOfView(){++outCalls;return call();}
        bool bulletOutOfView(){++bulletOutCalls;return outCalls==1&&call();}
        bool destroySelf(){++kills;destroyed=true;return call();}
    };
    static bool lifetime(unsigned failure=0){
        struct LifetimeOps:Ops {
            unsigned images=0,failure=0;
            bool solid(DroneBulletSourceStep&){++solids;return call();}
            bool publishImageIndex(RunnerImageValue index){
                ++images;
                // Tick 31 wraps six frames at speed 0.5 back to zero.
                if(images==31&&index.bits)return false;
                return !(images==31&&failure==1);
            }
            bool destroySelf(){++kills;destroyed=true;return failure!=2;}
        } ops;
        ops.failure=failure;
        DroneBulletSourceStep shot;Random random;bool good=true;
        if(!shot.reset(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512)))return false;
        shot.motion.direction=MotionBinary64::fromInteger(90);shot.damage=MotionBinary64::fromInteger(20);
        // Synchronous birth after this frame's animation/alarm passes, then
        // object-266 Step in the same frame. No synthetic early alarm tick.
        if(!shot.step(random,ops,good)||shot.alarm0!=16||shot.drawFrame()!=0)return false;
        for(unsigned tick=1;tick<=31;++tick){
            const bool advanced=shot.advanceAnimation(ops,good);
            if(tick==31){
                if(advanced!=(failure==0)||good!=(failure==0)||shot.imageIndex.bits||shot.timeline.bits||
                    ops.kills!=(failure==1?0u:1u)||ops.solids!=31||random.draws!=1)return false;
                if(failure&&(shot.advanceAnimation(ops,good)||ops.images!=31||
                    shot.step(random,ops,good)||ops.solids!=31))return false;
                return true;
            }
            if(!advanced||!good||ops.destroyed)return false;
            const unsigned expectedFrame=tick<20?0:(tick-19)/2;
            if(shot.drawFrame()!=expectedFrame)return false;
            shot.advanceAlarm();
            if(shot.alarm0!=(tick<16?int(16-tick):tick==16?0:-1)||shot.decelerate!=(tick>=16)||
               !shot.step(random,ops,good)||shot.animation!=(tick>=19))return false;
        }
        return false;
    }
    static bool run(){
        if(!lifetime())return false;
        DroneBulletSourceStep shot;Random random;Ops ops;bool good=true;
        if(!shot.reset(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512))||
            shot.drawable()||shot.alarm0!=16||shot.imageSpeed.bits||shot.animation)return false;
        shot.motion.direction=MotionBinary64::fromInteger(90);shot.damage=MotionBinary64::fromInteger(20);
        if(!shot.step(random,ops,good)||!good||!shot.drawable()||random.draws!=1||
            ops.angles!=2||ops.positions!=2||ops.solids!=1||!ops.destroyed||
            shot.motion.y.bits!=0x43fc8000u)return false; // 512-7 = 505
        shot.alarmZero();
        for(unsigned i=0;i<4;++i){
            if(!shot.step(random,ops,good)||shot.animation!=(i==3))return false;
        }
        if(random.draws!=1||ops.speeds!=1||shot.imageSpeed.bits!=0x3f000000u||
            !shot.endStep(ops,good)||!shot.animationEnd(ops,good)||ops.outCalls!=1||ops.bulletOutCalls!=1||ops.kills!=1)return false;
        for(unsigned failure=1;failure<=5;++failure){
            DroneBulletSourceStep failed;Random rng;Ops rejected;bool okay=true;
            rejected.fail=failure;
            if(!failed.reset({},{}))return false;
            failed.motion.direction=MotionBinary64::fromInteger(90);failed.damage=MotionBinary64::fromInteger(20);
            if(failed.step(rng,rejected,okay)||okay||rejected.calls!=failure)return false;
            if(failed.step(rng,rejected,okay)||rejected.calls!=failure||rng.draws!=1)return false;
        }
        for(unsigned failure=1;failure<=2;++failure){
            Ops rejected;bool okay=true;rejected.fail=failure;
            if(shot.endStep(rejected,okay)||okay||rejected.calls!=failure||
                shot.endStep(rejected,okay)||rejected.calls!=failure)return false;
        }
        return true;
    }
};
}
#endif
