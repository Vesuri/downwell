#ifndef DOWNWELL_CORE_SMOKE_PARTICLE_H
#define DOWNWELL_CORE_SMOKE_PARTICLE_H
#include "SmokeBirth.h"
#include "FixedPoint.h"
#include "UnitSequenceFrames.h"
namespace downwell {
// The compact smoke owner's fxSmoke (object 225, sprite 84) with Q16
// position and velocity (W3 fixed point): within a pixel of SmokeEffect,
// its VM form and oracle, which the registered room path still runs.
// Create's RNG samples, speed, direction and the animation rate stay the
// VM's binary64/binary32 values: Step reads speed and direction once, to
// set the velocity, and the wall response scales speed only.
struct SmokeParticle {
    MotionBinary64 speed{},direction{};
    RunnerImageValue imageSpeed{},timeline{},imageIndex{};
    std::int32_t xx=0,yy=0,xsp=0,ysp=0,xstart=0,ystart=0; // Q16
    std::int32_t x=0,y=0; // published whole pixels
    bool noCollision=true,allSet=false,destroyRequested=false;

    // GameMaker round (halves to even) of a Q16 value.
    static std::int32_t round(std::int32_t q16){
        std::int32_t whole=(q16+0x8000)>>16;
        if((q16&0xffff)==0x8000&&(whole&1))--whole;
        return whole;
    }
    template<class Random> bool create(MotionBinary64 px,MotionBinary64 py,Random& random){
        SmokeBirth birth;
        Fixed16 fx,fy;
        if(!birth.create(px,py,random)||!Fixed16::fromBinary64(birth.xx,fx)||
           !Fixed16::fromBinary64(birth.yy,fy))return false;
        SmokeParticle next;
        next.speed=birth.speed;next.direction=birth.direction;next.imageSpeed=birth.imageSpeed;
        next.xx=next.xstart=fx.v;next.yy=next.ystart=fy.v;
        next.x=round(fx.v);next.y=round(fy.v);
        *this=next;return true;
    }
    // Animation runs before Begin Step, as SmokeEffect::advanceImage.
    bool advanceImage(){
        bool crossed=false;
        if(!UnitSequenceFrames::advance(timeline,imageSpeed,9,imageIndex,crossed))return false;
        if(crossed)destroyRequested=true;
        return true;
    }
    unsigned drawFrame()const{
        const unsigned exponent=(imageIndex.bits>>23)&255u;
        return exponent<127?0:((imageIndex.bits&0x7fffffu)|0x800000u)>>(150u-exponent);
    }
    template<class Random> bool step(Random& random){
        if(!allSet){
            // lengthdir(smokesp, smokedir), then Step's second rate sample.
            Fixed16 length,angle;
            if(!Fixed16::fromBinary64(speed,length)||length.v<0||length.v>8*65536||
               !Fixed16::fromBinary64(direction,angle))return false;
            xsp=FixedTrig::lengthdir(false,length.v,angle.v);
            ysp=FixedTrig::lengthdir(true,length.v,angle.v);
            SmokeBirth rate;rate.speed=speed;
            if(!rate.sampleImageSpeed(random))return false;
            imageSpeed=rate.imageSpeed;allSet=true;
        }
        // Integrate the old velocity, publish whole pixels, then damp by 0.9.
        xx+=xsp;yy+=ysp;
        x=round(xx);y=round(yy);
        xsp=damp(xsp);ysp=damp(ysp);
        if(noCollision&&escaped())noCollision=false;
        return true;
    }
    void alarm0(){noCollision=false;}
    // The source scales smokesp, not the velocity or image_speed.
    bool collideWall(){
        if(noCollision)return true;
        MotionBinary64 scaled;
        if(!multiplyBinary64(speed,{0x3fc99999u,0x9999999au},scaled))return false;
        speed=scaled;return true;
    }
private:
    static std::int32_t damp(std::int32_t q16){
        return std::int32_t((std::int64_t(q16)*58982+0x8000)>>16); // 0.9 in Q16
    }
    // point_distance(xstart, ystart, x, y) > 16.
    bool escaped()const{
        const std::int64_t dx=std::int64_t(x)*65536-xstart,dy=std::int64_t(y)*65536-ystart;
        constexpr std::int64_t far=std::int64_t(1)<<28; // squares stay below 2^62
        if(dx>=far||dx<=-far||dy>=far||dy<=-far)return true;
        return dx*dx+dy*dy>(std::int64_t(1)<<40);
    }
};
}
#endif
