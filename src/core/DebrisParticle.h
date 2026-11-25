#ifndef DOWNWELL_CORE_DEBRIS_PARTICLE_H
#define DOWNWELL_CORE_DEBRIS_PARTICLE_H
#include "DebrisBirth.h"
#include "EffectSequenceAnimation.h"
#include "FixedPoint.h"
#include "Binary32Timeline.h"
namespace downwell {
// The compact debris owner's object 370 with Q16 position and velocity (W3
// fixed point): DebrisAnimation, its VM form and oracle, still runs on the
// registered room paths. Create's RNG samples and the animation stay the
// VM's; the sParentSolid queries take Q16 positions (Meeting(x, y, hit)
// with std::int32_t coordinates) and every draw of a hit is kept, so a hit
// moved by a Q16 rounding is the only way this can shift the RNG stream.
struct DebrisParticle {
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

    std::int32_t pixelX()const{return Fixed16::roundPixel(x16);}
    std::int32_t pixelY()const{return Fixed16::roundPixel(y16);}
    template<class Random> bool create(MotionBinary64 px,MotionBinary64 py,int area,Random& random){
        DebrisBirth birth;RunnerImageValue bx,by;Fixed16 fx,fy,vx,vy;
        // Builtin x/y narrow to binary32 first, as the VM form.
        if(!bx.assign(px)||!by.assign(py)||!birth.create(area,random)||
           !Fixed16::fromBinary64(bx.promoted(),fx)||!Fixed16::fromBinary64(by.promoted(),fy)||
           !Fixed16::fromBinary64(birth.xsp,vx)||!Fixed16::fromBinary64(birth.ysp,vy))return false;
        DebrisParticle next;
        next.x16=fx.v;next.y16=fy.v;next.xsp=vx.v;next.ysp=vy.v;
        next.imageSpeed=next.rate=birth.imageSpeed;next.imageIndex=birth.imageIndex;
        next.alarms=birth.alarms;next.spriteId=birth.spriteId;
        *this=next;return true;
    }
    // The scenery's breakablesDebris.ysp write.
    bool setYSpeed(MotionBinary64 value){
        Fixed16 v;if(!Fixed16::fromBinary64(value,v)||v.v< -8*65536||v.v>8*65536)return false;
        ysp=v.v;return true;
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
