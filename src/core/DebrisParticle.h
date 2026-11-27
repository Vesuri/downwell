#ifndef DOWNWELL_CORE_DEBRIS_PARTICLE_H
#define DOWNWELL_CORE_DEBRIS_PARTICLE_H
#include "AlarmBank.h"
#include "EffectSequenceAnimation.h"
#include "FixedPoint.h"
#include "Binary32Timeline.h"
namespace downwell {
// The compact debris owner's object 370 with Q16 position and velocity (W3
// fixed point): DebrisAnimation, its VM form and oracle, still runs on the
// registered room paths. Create's draws are the VM's, in order, sampled as
// the nearest Q16 (the VM's binary64 samples' nearest Q16 but for exact
// halves) and the binary32 frame and rate; the animation stays the VM's.
// The sParentSolid queries take Q16 positions (Meeting(x, y, hit)
// with std::int32_t coordinates) and every draw of a hit is kept, so a hit
// moved by a Q16 rounding is the only way this can shift the RNG stream.
struct DebrisParticle {
    using Position=Fixed16;
    std::int32_t x16=0,y16=0,xsp=0,ysp=0; // Q16
    RunnerImageValue imageSpeed{},imageIndex{},timeline{},rate{};
    std::int32_t frame27=0; // the timeline once timed (Q27)
    bool timed=false;
    AlarmBank alarms;
    int spriteId=91,flash=-1;
    bool disappearing=false,noTouch=true,destroyRequested=false;
    // Q16 constants: gravity 0.08 and 0.1, the -1 threshold, wall bounce 0.7
    // and the 0.05 minimum horizontal speed.
    static constexpr std::int32_t gravity=5243,hardGravity=6554,fall=-65536,bounce=45875,creep=3277;

    std::int32_t pixelX()const{return Fixed16::roundQ16(x16);}
    std::int32_t pixelY()const{return Fixed16::roundQ16(y16);}
    // DebrisBirth::create: xsp random(4) - 2, ysp random(1.5) - 0.5, the
    // area's sprite, image_speed choose(-0.5, -0.3, 0, 0.3, 0.5) and
    // image_index random(7), then the three alarms.
    template<class Random> bool create(Fixed16 px,Fixed16 py,int area,Random& random){
        DebrisParticle next;
        next.x16=px.v;next.y16=py.v;
        next.xsp=Fixed16::random(random.nextU32(),Fixed16::fromInteger(4)).v-2*65536;
        next.ysp=Fixed16::random(random.nextU32(),Fixed16::raw(98304)).v-32768;
        if(area==1||area==3||area==5)next.spriteId=115;
        else if(area==2)next.spriteId=116;
        // Area 4 explicitly assigns 91; other areas retain the resource default.
        static constexpr std::uint32_t rates[]={0xbf000000u,0xbe99999au,0,0x3e99999au,0x3f000000u};
        next.imageSpeed=next.rate=RunnerImageValue{rates[random.chooseIndexSmall(5)]};
        next.imageIndex=RunnerImageValue{RunnerImageValue::randomSevenths(random.nextU32())};
        next.alarms.set(0,30);next.alarms.set(1,15);next.alarms.set(2,7);
        *this=next;return true;
    }
    // The scenery's breakablesDebris.ysp write.
    bool setYSpeed(Fixed16 value){
        if(value.v< -8*65536||value.v>8*65536)return false;
        ysp=value.v;return true;
    }
    // The first advance reconciles Create's random(7) image_index write
    // through EffectSequenceAnimation; nothing writes image_index after
    // that, so later advances add image_speed to the binary32 timeline in
    // exact Q27 (Binary32Timeline) and publish it as image_index.
    bool advanceImage(){
        if(destroyRequested)return true;
        const unsigned frames=spriteId==91?6:spriteId==115||spriteId==116?4:0;
        std::int32_t rate27;bool exact=false;
        if(timed&&(!Binary32Timeline::fromBits(imageSpeed.bits,rate27,exact)||!exact))timed=false;
        if(!timed){
            if(!EffectSequenceAnimation::advance(timeline,imageSpeed,frames,imageIndex))return false;
            timed=Binary32Timeline::fromBits(timeline.bits,frame27,exact)&&exact&&frame27>=0&&
                imageIndex.bits==(timeline.bits&0x7fffffffu);
            return true;
        }
        if(!frames||rate27>std::int32_t(frames)*Binary32Timeline::one||rate27< -std::int32_t(frames)*Binary32Timeline::one)return false;
        Binary32Timeline::advance(frame27,rate27,std::int32_t(frames)*Binary32Timeline::one);
        timeline.bits=imageIndex.bits=Binary32Timeline::toBits(frame27);
        return true;
    }
    void advanceAlarm(unsigned index){
        if(destroyRequested||index>2||!alarms.advanceOne(index))return;
        alarmEvent(index);
    }
    void alarmEvent(unsigned index){
        if(destroyRequested||index>2)return;
        if(index==0)destroyRequested=true;
        else if(index==1)disappearing=true;
        else noTouch=false;
    }
    // DebrisMotion::step on Q16 values. The compact owner publishes nothing
    // per coordinate, so Publish is accepted and unused.
    template<class Random,class Meeting,class Publish>
    bool step(bool stopped,Random& random,Meeting meeting,Publish){
        if(destroyRequested)return true;
        if(stopped){
            for(unsigned i=0;i<3;++i){
                const auto n=alarms.get(i);
                if(n==0x7fffffff)return false;
                if(n>0)alarms.set(i,n+1);
            }
            imageSpeed={};return true;
        }
        constexpr std::int32_t range=std::int32_t(1)<<29,fast=16*65536;
        if(x16<=-range||x16>=range||y16<=-range||y16>=range||
           xsp< -fast||xsp>fast||ysp< -fast||ysp>fast)return false;
        imageSpeed=rate;
        ysp+=ysp<fall?hardGravity:gravity;
        bool hit=false;
        if(!meeting(x16+xsp,y16,hit))return false;
        if(hit)xsp=-scale(xsp);
        if(!meeting(x16,y16+ysp,hit))return false;
        if(hit){
            if(ysp<=0)ysp=0;
            else ysp=std::int32_t(random.irandomSmall(0))*65536;
            xsp=scale(xsp);
            static const std::uint32_t rates[]={0xbe99999au,0xbdcccccdu,0,0x3dcccccdu,0x3e99999au};
            imageSpeed=RunnerImageValue{rates[random.chooseIndexSmall(5)]};
        }
        if(xsp>-creep&&xsp<creep)xsp=xsp>0?creep:xsp<0?-creep:0;
        x16+=xsp;y16+=ysp;
        if(!meeting(x16,y16,hit))return false;
        if(hit){
            if(!meeting(x16+8*65536,y16,hit))return false;
            if(!hit)x16+=8*65536;
            else{
                if(!meeting(x16-8*65536,y16,hit))return false;
                if(!hit)x16-=8*65536;
            }
            if(!meeting(x16,y16-8*65536,hit))return false;
            if(!hit)y16-=8*65536;
            else{
                if(!meeting(x16,y16+8*65536,hit))return false;
                if(!hit)y16+=8*65536;
            }
            if(!meeting(x16,y16,hit))return false;
            if(hit)destroyRequested=true;
        }
        // A destroy request does not abort the remaining instructions.
        flash=disappearing?-flash:-1;
        return true;
    }
private:
    static std::int32_t scale(std::int32_t v){return Fixed16::scale(v,std::uint32_t(bounce));}
};
}
#endif
