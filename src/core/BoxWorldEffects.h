#ifndef DOWNWELL_CORE_BOX_WORLD_EFFECTS_H
#define DOWNWELL_CORE_BOX_WORLD_EFFECTS_H
#include "InstanceHandle.h"
#include "FixedPoint.h"
#include "SoundPlayScript.h"
namespace downwell {
// Concrete shared emissions for BoxDestructionScript. Controls supplies the
// source globals, chain queries/setters, destruction, rewards and time
// query. Those dependencies cannot be replaced with successful no-op defaults.
template<class World,class Controls,class Frames,class Audio>
struct BoxWorldEffects:Controls {
    using Handle=InstanceHandle;
    World& world;Handle box;Frames frames;
    Audio& audio;AudioLevel volume;bool inWater;
    BoxWorldEffects(World& w,Handle h,Controls controls,Frames lookup,Audio& backend,AudioLevel gain,bool water)
        :Controls(controls),world(w),box(h),frames(lookup),audio(backend),volume(gain),inWater(water){}
    bool sound(int id,int priority,int loop,int cut){
        return id==342&&priority==50&&loop==0&&cut==1&&world.playBoxChainSound(box,volume,inWater,audio);
    }
    bool soundOverlap(int sound,int priority,int loop,int cut,const char* channel){
        return priority==50&&loop==0&&cut==1&&SoundPlayScript::channel(channel)==7&&
            world.playBoxBreakSound(box,sound,volume,audio);
    }
    bool alarmStop(int alarm){
        return alarm==0&&world.stopBoxAlarm0(box,[&](bool& allowed){return this->timeAllowed(allowed);});
    }
    bool smoke(MotionBinary64 x,MotionBinary64 y,int count,int vary){
        Fixed16 qx,qy;
        return count==1&&vary==0&&Fixed16::fromBinary64(x,qx)&&Fixed16::fromBinary64(y,qy)&&world.emitBoxSmoke(qx,qy);
    }
    bool flash(MotionBinary64 x,MotionBinary64 y,int count,int vary,int radius){
        Fixed16 qx,qy;
        return count==1&&vary==0&&radius==2&&Fixed16::fromBinary64(x,qx)&&Fixed16::fromBinary64(y,qy)&&world.emitBoxFlash(qx,qy);
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
template<class World,class Controls,class Frames,class Audio>
auto boxWorldEffects(World& world,InstanceHandle box,Controls controls,Frames frames,Audio& audio,AudioLevel volume,bool inWater){
    return BoxWorldEffects<World,Controls,Frames,Audio>(world,box,controls,frames,audio,volume,inWater);
}
}
#endif
