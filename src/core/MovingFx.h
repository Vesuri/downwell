#ifndef DOWNWELL_CORE_MOVING_FX_H
#define DOWNWELL_CORE_MOVING_FX_H
#include "NonLoopSpriteAnimation.h"
#include "RunnerRealComparison.h"
namespace downwell {
// Object 220: no parent Create and no birth RNG. The caller subsequently
// installs artwork and velocity through emitMovingFx's separate writes.
struct MovingFx:NonLoopSpriteAnimation {
    using Real=MotionBinary64;
    RunnerImageValue x{},y{},angle{};
    Real xx{},yy{},xsp{},ysp{},imageRate=Real::fromInteger(-1);
    bool initialized=false;
    bool create(RunnerImageValue px,RunnerImageValue py){
        if(initialized||!px.promoted().valid()||!py.promoted().valid())return false;
        x=px;y=py;xx=x.promoted();yy=y.promoted();xsp={};ysp={};
        imageRate=Real::fromInteger(-1);initialized=true;return true;
    }
    template<class Ops> bool step(Ops& ops){
        if(!valid||!initialized)return false;
        int order;
        if(!compareRunnerReal(imageRate,Real::fromInteger(-1),order))return false;
        if(!order)imageRate=imageSpeed.promoted();
        // TimeStopBound includes spatial field membership, not merely the
        // global player's stopped flag. Read it after capturing image speed.
        bool moving;if(!ops.timeStopBound(moving))return false;
        const Real wanted=moving?imageRate:Real{};
        if(!compareRunnerReal(imageSpeed.promoted(),wanted,order))return false;
        if(order){
            RunnerImageValue value,accepted;
            if(!value.assign(wanted)||!ops.imageSpeed(value,accepted))return false;
            imageSpeed=accepted;
        }
        if(moving&&(!xx.add(xsp)||!yy.add(ysp)))return false;
        // roundPosition writes Y before X, even while paused.
        for(unsigned axis=0;axis<2;++axis){
            const bool vertical=axis==0;
            std::int32_t pixel;RunnerImageValue value,accepted;
            if(!(vertical?yy:xx).roundPixel(pixel)||!value.assignInteger(pixel)||
                !ops.position(vertical,value,accepted))return false;
            (vertical?y:x)=accepted;
        }
        return true;
    }
};
struct MovingFxSpawnScript {
    template<class Ops> static bool run(Ops& ops,MotionBinary64 x,MotionBinary64 y,
        int sprite,MotionBinary64 imageSpeed,MotionBinary64 angle,MotionBinary64 speed){
        typename Ops::Handle child{};MotionBinary64 component;
        return ops.create(220,x,y,child)&&ops.myFx(child)&&ops.sprite(child,sprite)&&
            ops.imageSpeed(child,imageSpeed)&&ops.imageAngle(child,angle)&&
            ops.lengthdir(false,speed,angle,component)&&ops.velocity(child,false,component)&&
            ops.lengthdir(true,speed,angle,component)&&ops.velocity(child,true,component);
    }
};
}
#endif
