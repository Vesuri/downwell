#ifndef DOWNWELL_CORE_SMOKE_PARTICLE_H
#define DOWNWELL_CORE_SMOKE_PARTICLE_H
#include "FixedPoint.h"
namespace downwell {
// The compact smoke owner's fxSmoke (object 225, sprite 84) in Q16 (W3.20):
// within a pixel of SmokeEffect, its VM form and oracle, which the
// registered room path still runs. Create's RNG draws are the VM's, in
// order (random(2) + 2, the rate's random(0.08) + 0.05, random(359)), each
// sampled as the nearest Q16. Step reads speed and direction once, to set
// the velocity, and the wall response scales speed only. The animation
// position and rate are Q16 too (frame16, nine unit frames), so a smoke may
// end a tick before or after the VM's binary32 timeline.
struct SmokeParticle {
    Fixed16 speed{},direction{};
    std::int32_t frame16=0,rate16=0;
    std::int32_t xx=0,yy=0,xsp=0,ysp=0,xstart=0,ystart=0; // Q16
    std::int32_t x=0,y=0; // published whole pixels
    bool noCollision=true,allSet=false,destroyRequested=false;

    // GameMaker round (halves to even) of a Q16 value.
    static std::int32_t round(std::int32_t q16){
        std::int32_t whole=(q16+0x8000)>>16;
        if((q16&0xffff)==0x8000&&(whole&1))--whole;
        return whole;
    }
    template<class Random> bool create(Fixed16 px,Fixed16 py,Random& random){
        SmokeParticle next;
        next.speed=Fixed16::random(random.nextU32(),Fixed16::fromInteger(2))+Fixed16::fromInteger(2);
        if(!next.sampleRate(random))return false;
        next.direction=Fixed16::random(random.nextU32(),Fixed16::fromInteger(359));
        next.xx=next.xstart=px.v;next.yy=next.ystart=py.v;
        next.x=round(px.v);next.y=round(py.v);
        *this=next;return true;
    }
    // Animation runs before Begin Step, as SmokeEffect::advanceImage.
    // Reaching the ninth frame wraps the position and ends the smoke (its
    // Animation End).
    bool advanceImage(){
        frame16+=rate16;
        if(frame16>=frames16){frame16-=frames16;destroyRequested=true;}
        return frame16>=0&&frame16<frames16;
    }
    unsigned drawFrame()const{return unsigned(frame16>>16);}
    template<class Random> bool step(Random& random){
        if(!allSet){
            // lengthdir(smokesp, smokedir), then Step's second rate sample.
            if(speed.v<0||speed.v>8*65536)return false;
            xsp=FixedTrig::lengthdir(false,speed.v,direction.v);
            ysp=FixedTrig::lengthdir(true,speed.v,direction.v);
            if(!sampleRate(random))return false;
            allSet=true;
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
        speed=Fixed16::multiply(speed,Fixed16::raw(13107)); // 0.2
        return true;
    }
private:
    static constexpr std::int32_t frames16=9*65536;
    // image_speed = smokesp * (random(0.08) + 0.05).
    template<class Random> bool sampleRate(Random& random){
        const Fixed16 rate=Fixed16::multiply(speed,
            Fixed16::random(random.nextU32(),Fixed16::raw(5243))+Fixed16::raw(3277));
        if(rate.v<0||rate.v>=frames16)return false;
        rate16=rate.v;return true;
    }
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
