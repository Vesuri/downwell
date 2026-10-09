#ifndef DOWNWELL_TEST_SOURCE_BULLET_SCHEDULING_CHECKS_H
#define DOWNWELL_TEST_SOURCE_BULLET_SCHEDULING_CHECKS_H
#include "core/MachineGunSourceStep.h"
#include <cassert>
// Synthetic empty contact domain: tests scheduler/RNG/state ownership, not
// natural collision selection (covered by separate resource-owner fixtures).
template<class World> struct ScheduledEmptyBulletOps {
    World& world;downwell::InstanceHandle shot;unsigned writes=0;
    bool publishAngle(downwell::RunnerImageValue angle){return world.publishBulletAngle(shot,angle);}
    bool publishPosition(downwell::RunnerImageValue x,downwell::RunnerImageValue y){
        ++writes;return world.publishBulletCollisionPosition(shot,x,y);
    }
    bool line(int object,bool& hit){assert(object==59);hit=false;return true;}
    bool placeShootable(int object,bool& hit){assert(object==90);hit=false;return true;}
    bool selectWall(){assert(false);return false;}bool approachSolid(){assert(false);return false;}
    bool wallParent(int&){assert(false);return false;}
    bool wallHealth(downwell::MotionBinary64&){assert(false);return false;}
    bool wallHealthWrite(downwell::MotionBinary64){assert(false);return false;}
    bool wallHit(){assert(false);return false;}
    bool destroySelf(){assert(false);return false;}bool soundWall(){assert(false);return false;}
    bool explosion(int){assert(false);return false;}bool selectShootable(int){assert(false);return false;}
    bool thingHealth(downwell::MotionBinary64&){assert(false);return false;}
    bool thingHealthWrite(downwell::MotionBinary64){assert(false);return false;}
    bool thingHit(){assert(false);return false;}bool thingHitEffect(bool&){assert(false);return false;}
};
#endif
