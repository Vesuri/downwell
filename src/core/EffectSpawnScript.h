#ifndef DOWNWELL_CORE_EFFECT_SPAWN_SCRIPT_H
#define DOWNWELL_CORE_EFFECT_SPAWN_SCRIPT_H
#include "MotionBinary64.h"
namespace downwell {
// scrEffectSpawn: instance_create is synchronous. Preserve the caller's myFx
// assignment and four subsequent setters; do not combine them into birth
// defaults. Ops owns identity, source setters, failure latching and cleanup.
// Completed operations survive a later failure, just like other VM adapters.
struct EffectSpawnScript {
    // scrFxNol leaves the child's constructor angle/depth untouched. Writing
    // explicit zeroes through the longer script would add observable setters.
    template<class Ops> static bool runNol(Ops& ops,MotionBinary64 x,MotionBinary64 y,
        int sprite,MotionBinary64 speed){
        typename Ops::Handle child{};
        return ops.create(219,x,y,child)&&ops.myFx(child)&&
            ops.sprite(child,sprite)&&ops.imageSpeed(child,speed);
    }
    template<class Ops> static bool run(Ops& ops,MotionBinary64 x,MotionBinary64 y,
        int sprite,MotionBinary64 speed,MotionBinary64 angle,MotionBinary64 depth){
        typename Ops::Handle child{};
        return ops.create(219,x,y,child)&&ops.myFx(child)&&
            ops.sprite(child,sprite)&&ops.imageSpeed(child,speed)&&
            ops.imageAngle(child,angle)&&ops.depth(child,depth);
    }
};
}
#endif
