#ifndef DOWNWELL_CORE_CAMERA_CREATE_STATE_H
#define DOWNWELL_CORE_CAMERA_CREATE_STATE_H
#include "RunnerImageValue.h"
#include "DivideBinary64.h"
namespace downwell {
// camMain Create (997), before scrSpawnCamera's room-specific overrides.
// Public X/Y are binary32 setters; private locals retain binary64 values.
struct CameraCreateState {
    int roomw=0,sideLocked=0,camFocus=0,camShake=0,camShakeAmt=0;
    int endingCamera=0,creditSpawn=0,memorizey=0,camSlowdown=0,camSlowdownX=0;
    int autoScroll=0,daop=0,chasePlayer=0,centered=0,freeCam=0,xAhead=0,roomEnd=0;
    MotionBinary64 camPointy{},camPointx{},camAccly{},camAcclx{},xx{},yy{};
    bool initialized=false;
    template<class Publish>
    bool create(int width,bool ground,RunnerImageValue initialX,RunnerImageValue initialY,
        RunnerImageValue playerX,RunnerImageValue playerY,Publish publish){
        const auto finite=[](RunnerImageValue v){const unsigned e=v.bits&0x7f800000u;
            return e!=0x7f800000u&&(e||!(v.bits&0x7fffffu));};
        if(initialized||!finite(initialX)||!finite(initialY)||!finite(playerX)||!finite(playerY))return false;
        roomw=width;sideLocked=camFocus=camShake=camShakeAmt=endingCamera=creditSpawn=memorizey=0;
        camSlowdown=5;camSlowdownX=1;autoScroll=0;daop=ground?-32:24;
        camPointy=playerY.promoted();camPointx=playerX.promoted();
        auto y=camPointy,negativeY=initialY.promoted();negativeY.high^=0x80000000u;
        if(!y.add(MotionBinary64::fromInteger(daop))||!y.add(negativeY)||
            !divideBinary64(y,MotionBinary64::fromInteger(5),camAccly))return false;
        auto x=MotionBinary64::fromInteger(80),negativeX=initialX.promoted();negativeX.high^=0x80000000u;
        if(!x.add(negativeX)||!divideBinary64(x,MotionBinary64::fromInteger(10),camAcclx))return false;
        chasePlayer=centered=1;freeCam=0;xAhead=16;roomEnd=-1;
        // Callback returns the accepted builtin value. The runner may retain
        // old signed zero on a numerically equal setter.
        RunnerImageValue publishedX,publishedY;
        if(!publish(false,playerX,publishedX)||!publish(true,playerY,publishedY))return false;
        xx=publishedX.promoted();yy=publishedY.promoted();initialized=true;return true;
    }
};
}
#endif
