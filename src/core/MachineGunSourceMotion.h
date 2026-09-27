#ifndef DOWNWELL_CORE_MACHINE_GUN_SOURCE_MOTION_H
#define DOWNWELL_CORE_MACHINE_GUN_SOURCE_MOTION_H
#include "RandomBinary64.h"
#include "SmokeDirection.h"
#include "SmokeTrig.h"
#include "MachineGunProfile.h"
#include "DivideBinary64.h"
namespace downwell {
// Arithmetic contract for bulletStandard Step, not its event scheduler.
// Always store ABSOLUTE room coordinates: translating before the builtin
// float setter changes rounding. Local drawing offsets are backend work.
// Collision queries use endpoint() BEFORE commitPosition() narrows x/y.
// The live Q20 adapter is not replaced until it can consume these endpoints
// and preserve synchronous muzzle Create / collision / destruction ordering.
class MachineGunSourceMotion {
public:
    RunnerImageValue x{},y{},imageAngle{};
    MotionBinary64 direction{},xsp{},ysp{};
    bool initialized=false,zeroSpread=false;
    static_assert(machineGunProfile(false).speed==8 && machineGunProfile(true).speed==8 &&
        machineGunProfile(false).accuracy==3 && machineGunProfile(true).accuracy==3,
        "source motion is restricted to the two usual-gun profiles");
    bool reset(MotionBinary64 px,MotionBinary64 py,bool laserSight=false){
        MachineGunSourceMotion next;
        if(!next.x.assign(px)||!next.y.assign(py))return false;
        next.zeroSpread=laserSight;*this=next;return true;
    }
    template<class Random,class PublishAngle,class Muzzle>
    bool initialize(Random& random,PublishAngle publishAngle,Muzzle muzzle){
        if(initialized)return true;
        // Equal random_range bounds still consume the original one draw.
        const auto word=random.nextU32();
        MotionBinary64 spread,angle,vx,vy;
        if(!randomBinary64(word,MotionBinary64::fromInteger(zeroSpread?0:6),spread) ||
           !spread.add(MotionBinary64::fromInteger(zeroSpread?0:-3)))return false;
        angle=MotionBinary64::fromInteger(270);
        RunnerImageValue publishedAngle;
        if(!angle.add(spread) ||
           !SmokeDirection::vector(MotionBinary64::fromInteger(8),angle,vx,vy,smokeTrig) ||
           !publishedAngle.assign(angle))return false;
        direction=angle;xsp=vx;ysp=vy;imageAngle=publishedAngle;
        // image_angle is visible to synchronous muzzle Create. allSet is
        // assigned only after that child and its caller-side overrides finish.
        if(!publishAngle(imageAngle)||!muzzle(*this))return false;
        initialized=true;return true;
    }
    template<class Random> bool initialize(Random& random){
        return initialize(random,[](RunnerImageValue){return true;},
            [](const MachineGunSourceMotion&){return true;});
    }
    bool endpoint(MotionBinary64& px,MotionBinary64& py)const{
        if(!initialized)return false;
        auto nx=x.promoted(),ny=y.promoted();
        if(!nx.add(xsp)||!ny.add(ysp))return false;
        px=nx;py=ny;return true;
    }
    // Publisher is invoked before committing the arithmetic owner's field,
    // matching the lifecycle adapters' checked write contract. A later failure
    // retains completed writes; the enclosing world must latch failure.
    template<class Publish> bool addX(MotionBinary64 delta,Publish publish){
        auto value=x.promoted();RunnerImageValue next;
        if(!value.add(delta)||!next.assign(value)||!publish(next,y))return false;
        x=next;return true;
    }
    template<class Publish> bool addY(MotionBinary64 delta,Publish publish){
        auto value=y.promoted();RunnerImageValue next;
        if(!value.add(delta)||!next.assign(value)||!publish(x,next))return false;
        y=next;return true;
    }
    template<class Publish> bool commitPosition(Publish publish){
        // bulletStandard Step continues even if scrBulCheckSolid requested
        // destruction. It does not commit X/Y as a simultaneous pair.
        return initialized&&addX(xsp,publish)&&addY(ysp,publish);
    }
    bool commitPosition(){
        return commitPosition([](RunnerImageValue,RunnerImageValue){return true;});
    }
    // scrBulCheckSolid's approach loop, called ONLY after the wall selector
    // succeeded and the enemy-line suppression check failed. Query provides
    // original collision_point(parentSolid=59, precise=false, notme=false).
    // This does not select thatWall, apply damage/effects, or request retirement.
    template<class Point,class Publish>
    bool approachSolid(Point point,Publish publish,unsigned& whileStop){
        if(!initialized)return false;
        whileStop=0;
        for(;;){
            bool hit=false;
            if(!point(x.promoted(),y.promoted(),hit))return false;
            if(hit)return true;
            MotionBinary64 dy,dx;
            // Original division by ten, not multiplication by rounded 0.1.
            // Compute X only after Y has been published, as the VM does.
            if(!divideBinary64(ysp,MotionBinary64::fromInteger(10),dy)||!addY(dy,publish)||
               !divideBinary64(xsp,MotionBinary64::fromInteger(10),dx)||!addX(dx,publish))return false;
            if(++whileStop>100)return true;
        }
    }
};
}
#endif
