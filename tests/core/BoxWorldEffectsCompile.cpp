#include "core/GameplayWorld.h"
#include "core/BoxWorldEffects.h"
struct Controls {
    int area();
    bool timeAllowed(bool&);
    bool create(int,downwell::MotionBinary64,downwell::MotionBinary64,downwell::InstanceHandle&);
};
struct Audio {
    bool isPlaying(int,bool&);bool stop(int);bool play(int,int,bool,int&);
    bool getGain(int,downwell::MotionBinary64&);bool setGain(int,downwell::MotionBinary64,int);
    bool setPitch(int,downwell::MotionBinary64);
};
bool nativeBoxWorldEmissions(GameplayWorld& world,downwell::InstanceHandle box,Controls controls,
    bool (*frames)(int,unsigned&),Audio& audio,downwell::InstanceHandle& child){
    auto effects=downwell::boxWorldEffects(world,box,controls,frames,audio,downwell::MotionBinary64::fromInteger(1),false);
    const auto x=downwell::MotionBinary64::fromInteger(240),y=downwell::MotionBinary64::fromInteger(512);
    return effects.soundOverlap(171,50,0,1,"waterThings")&&effects.smoke(x,y,1,0)&&effects.flash(x,y,1,0,2)&&
        effects.effect(x,y,603,{0x3fe00000u,0},downwell::MotionBinary64::fromInteger(270),-60000)&&
        effects.create(370,x,y,child)&&effects.sound(342,50,0,1)&&effects.alarmStop(0);
}
