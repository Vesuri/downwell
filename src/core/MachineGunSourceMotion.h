#ifndef DOWNWELL_CORE_MACHINE_GUN_SOURCE_MOTION_H
#define DOWNWELL_CORE_MACHINE_GUN_SOURCE_MOTION_H
#include "RandomBinary64.h"
#include "SmokeDirection.h"
#include "SmokeTrig.h"
#include "MachineGunProfile.h"
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
    template<class Random> bool initialize(Random& random){
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
        initialized=true;return true;
    }
    bool endpoint(MotionBinary64& px,MotionBinary64& py)const{
        if(!initialized)return false;
        auto nx=x.promoted(),ny=y.promoted();
        if(!nx.add(xsp)||!ny.add(ysp))return false;
        px=nx;py=ny;return true;
    }
    bool commitPosition(){
        MotionBinary64 px,py;RunnerImageValue nx,ny;
        if(!endpoint(px,py)||!nx.assign(px)||!ny.assign(py))return false;
        x=nx;y=ny;return true;
    }
};
}
#endif
