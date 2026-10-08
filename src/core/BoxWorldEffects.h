#ifndef DOWNWELL_CORE_BOX_WORLD_EFFECTS_H
#define DOWNWELL_CORE_BOX_WORLD_EFFECTS_H
#include "InstanceHandle.h"
#include "MotionBinary64.h"
namespace downwell {
// Concrete shared emissions for BoxDestructionScript. Controls supplies the
// source audio, globals, chain queries/setters, destruction, rewards and time
// query. Those dependencies cannot be replaced with successful no-op defaults.
template<class World,class Controls,class Frames>
struct BoxWorldEffects:Controls {
    using Handle=InstanceHandle;
    World& world;Handle box;Frames frames;
    BoxWorldEffects(World& w,Handle h,Controls controls,Frames lookup)
        :Controls(controls),world(w),box(h),frames(lookup){}
    bool smoke(MotionBinary64 x,MotionBinary64 y,int count,int vary){
        return count==1&&vary==0&&world.emitBoxSmoke(x,y);
    }
    bool flash(MotionBinary64 x,MotionBinary64 y,int count,int vary,int radius){
        return count==1&&vary==0&&radius==2&&world.emitBoxFlash(x,y);
    }
    bool effect(MotionBinary64 x,MotionBinary64 y,int sprite,MotionBinary64 speed,
        MotionBinary64 angle,int depth){
        return world.spawnBoxScriptEffect(box,x,y,sprite,speed,angle,
            MotionBinary64::fromInteger(depth),frames);
    }
    bool create(int object,MotionBinary64 x,MotionBinary64 y,Handle& child){
        if(object==370)return world.spawnDebris(x,y,this->area(),child);
        return Controls::create(object,x,y,child);
    }
};
template<class World,class Controls,class Frames>
auto boxWorldEffects(World& world,InstanceHandle box,Controls controls,Frames frames){
    return BoxWorldEffects<World,Controls,Frames>(world,box,controls,frames);
}
}
#endif
