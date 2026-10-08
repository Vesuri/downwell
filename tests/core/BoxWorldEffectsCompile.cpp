#include "core/GameplayWorld.h"
#include "core/BoxWorldEffects.h"
struct Controls {
    int area();
    bool create(int,downwell::MotionBinary64,downwell::MotionBinary64,downwell::InstanceHandle&);
};
bool nativeBoxWorldEmissions(GameplayWorld& world,downwell::InstanceHandle box,Controls controls,
    bool (*frames)(int,unsigned&),downwell::InstanceHandle& child){
    auto effects=downwell::boxWorldEffects(world,box,controls,frames);
    const auto x=downwell::MotionBinary64::fromInteger(240),y=downwell::MotionBinary64::fromInteger(512);
    return effects.smoke(x,y,1,0)&&effects.flash(x,y,1,0,2)&&
        effects.effect(x,y,603,{0x3fe00000u,0},downwell::MotionBinary64::fromInteger(270),-60000)&&
        effects.create(370,x,y,child);
}
