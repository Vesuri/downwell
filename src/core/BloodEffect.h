#ifndef DOWNWELL_CORE_BLOOD_EFFECT_H
#define DOWNWELL_CORE_BLOOD_EFFECT_H
#include "SmokeDirection.h"
#include "SmokeTrig.h"
#include "RandomBinary64.h"
#include "UnitSequenceFrames.h"
namespace downwell {
// scrBloodfx's specialization of parentMovingFx (220), sprite 84. Not fxSmoke:
// constant velocity, TimeStopBound, and no random draws in child Create.
struct BloodEffect {
    MotionBinary64 xx{},yy{},xsp{},ysp{};
    RunnerImageValue x{},y{},imageAngle{},imageSpeed{0x3f000000u};
    RunnerImageValue savedSpeed{0xbf800000u},timeline{},imageIndex{};
    bool destroyRequested=false;
    bool create(MotionBinary64 px,MotionBinary64 py,MotionBinary64 angle){
        BloodEffect next;
        if(!next.x.assign(px) || !next.y.assign(py) || !next.imageAngle.assign(angle))return false;
        next.xx=next.x.promoted();next.yy=next.y.promoted();
        if(!SmokeDirection::vector(MotionBinary64::fromInteger(2),angle,next.xsp,next.ysp,smokeTrig))return false;
        *this=next;return true;
    }
    bool advanceImage(){
        bool crossed=false;
        if(!UnitSequenceFrames::advance(timeline,imageSpeed,9,imageIndex,crossed))return false;
        if(crossed)destroyRequested=true;
        return true;
    }
    // Owner evaluates TimeStopBound at the original event boundary. Animation
    // is a separate earlier phase, so freezing here does not undo that advance.
    bool step(bool timeAllowed){
        if(savedSpeed.bits==0xbf800000u)savedSpeed=imageSpeed;
        imageSpeed=timeAllowed?savedSpeed:RunnerImageValue{};
        auto nx=xx,ny=yy;
        if(timeAllowed && (!nx.add(xsp) || !ny.add(ysp)))return false;
        std::int32_t px,py;RunnerImageValue bx,by;
        if(!nx.roundPixel(px) || !ny.roundPixel(py) ||
           !bx.assign(MotionBinary64::fromInteger(px)) || !by.assign(MotionBinary64::fromInteger(py)))return false;
        xx=nx;yy=ny;x=bx;y=by;return true;
    }
    unsigned drawFrame()const{
        const unsigned exponent=(imageIndex.bits>>23)&255u;
        return exponent<127?0:((imageIndex.bits&0x7fffffu)|0x800000u)>>(150u-exponent);
    }
};
// The script ignores its supplied count arguments. Its two random(20) calls
// are separated by synchronous emitMovingFx/child Create, not batched upfront.
template<class Random,class Create>
bool bloodBurst(MotionBinary64 publishedX,MotionBinary64 publishedY,Random& random,Create create){
    for(unsigned i=0;i<2;++i){
        MotionBinary64 angle;
        if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(20),angle))return false;
        if(i){angle.high^=0x80000000u;if(!angle.add(MotionBinary64::fromInteger(180)))return false;}
        if(!create(publishedX,publishedY,angle))return false;
    }
    return true;
}
}
#endif
