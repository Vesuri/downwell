#ifndef DOWNWELL_CORE_TIME_SHARD_PARTICLE_H
#define DOWNWELL_CORE_TIME_SHARD_PARTICLE_H
#include "TimeShardBirth.h"
#include "EffectSequenceAnimation.h"
#include "FixedPoint.h"
#include "Binary32Timeline.h"
#include "AlarmBank.h"
namespace downwell {
// A published builtin that only ever holds whole pixels (the compact
// shards' x and y): readers call roundPixel as they do on RunnerImageValue.
struct WholePixel {
    std::int32_t v=0;
    bool roundPixel(std::int32_t& out)const{out=v;return true;}
};
// The compact world's fxTimeShard 238 with Q16 position and velocity (W3
// fixed point): TimeShardAnimation, its VM form and oracle, still runs on the
// registered room paths. Create's RNG samples stay the VM's (converted once);
// the sParentSolid queries take Q16 positions (Meeting(x, y, hit) with
// std::int32_t coordinates) and every RNG draw of a hit is kept, so a hit
// moved by a Q16 rounding is the only way this can shift the RNG stream.
// x and y are the published builtins: whole pixels, as the compact bursts
// are born at the player's published position.
struct TimeShardParticle {
    static constexpr bool fixedPoint=true;
    WholePixel x,y;
    std::int32_t x16=0,y16=0,xsp=0,ysp=0; // Q16
    RunnerImageValue imageSpeed{},imageIndex{},timeline{},savedSpeed{};
    std::int32_t frame27=0; // the timeline once timed (Q27)
    bool timed=false;
    int alarm0=0,alarm1=40,alarm2=30,shardLimit=20,dflash=-1;
    bool disappearing=false,noTouch=true,destroyRequested=false;
    // Q16 constants: gravity 0.08 and 0.1, the -1 threshold, wall bounce 0.7,
    // drag 0.9 and the 0.05 minimum speed.
    static constexpr std::int32_t gravity=5243,hardGravity=6554,fall=-65536;
    static constexpr std::uint32_t bounce=45875,drag=58982;
    static constexpr std::int32_t creep=3277;

    // TimeShardEffects' slot position before Create; whole pixels only.
    bool place(MotionBinary64 px,MotionBinary64 py){
        std::int32_t ix,iy;
        if(!px.roundPixel(ix)||!py.roundPixel(iy)||!(MotionBinary64::fromInteger(ix).high==px.high&&
           MotionBinary64::fromInteger(ix).low==px.low)||!(MotionBinary64::fromInteger(iy).high==py.high&&
           MotionBinary64::fromInteger(iy).low==py.low))return false;
        x.v=ix;y.v=iy;return true;
    }
    template<class Random> bool create(MotionBinary64 px,MotionBinary64 py,bool lowSpec,Random& random){
        TimeShardBirth birth;
        if(!place(px,py)||!birth.create(px,py,lowSpec,random)||
           x.v< -(1<<14)||x.v>=(1<<14)||y.v< -(1<<14)||y.v>=(1<<14))return false;
        Fixed16 vx,vy;
        if(!Fixed16::fromBinary64(birth.xsp,vx)||!Fixed16::fromBinary64(birth.ysp,vy)||
           !savedSpeed.assign(birth.savedSpeed))return false;
        x16=x.v*65536;y16=y.v*65536;xsp=vx.v;ysp=vy.v;
        imageSpeed=birth.imageSpeed;imageIndex=birth.imageIndex;timeline={};timed=false;frame27=0;
        alarm0=birth.alarm0;alarm1=birth.alarm1;alarm2=birth.alarm2;
        shardLimit=birth.shardLimit;dflash=birth.dflash;
        disappearing=birth.disappearing;noTouch=birth.noTouch;destroyRequested=false;
        return true;
    }
    // TimeShardBirth::create on whole pixels with no binary64: the same RNG
    // draws in order (irandom(3) - 2, choose(7), choose(5), random(7) and the
    // alarm), image_speed as the binary32 the table narrows to, and random(7)
    // narrowed once to binary32 (7 * word / 2^32 is exact in binary64).
    template<class Random> bool createWhole(std::int32_t px,std::int32_t py,bool lowSpec,Random& random){
        if(px< -(1<<14)||px>=(1<<14)||py< -(1<<14)||py>=(1<<14))return false;
        x.v=px;y.v=py;x16=px*65536;y16=py*65536;
        xsp=(std::int32_t(random.irandomSmall(4))-2)*65536;
        static constexpr std::int32_t vertical[]={0,-1,-1,-1,-2,-2,-3};
        ysp=vertical[random.chooseIndexSmall(7)]*65536;
        shardLimit=lowSpec?3:20;
        // -0.5, -0.3, 0, 0.3, 0.5 narrowed to binary32.
        static constexpr std::uint32_t rates[]={0xbf000000u,0xbe99999au,0,0x3e99999au,0x3f000000u};
        savedSpeed=RunnerImageValue{rates[random.chooseIndexSmall(5)]};imageSpeed=savedSpeed;
        imageIndex=RunnerImageValue{seventhsBinary32(random.nextU32())};
        alarm0=90+int(random.randomIntegerSmall(70));alarm1=40;alarm2=30;dflash=-1;
        timeline={};timed=false;frame27=0;
        disappearing=false;noTouch=true;destroyRequested=false;
        return true;
    }
    // Round-to-nearest-even binary32 of 7 * word / 2^32, on 32-bit words
    // (the product has at most three bits above the low word).
    static std::uint32_t seventhsBinary32(std::uint32_t word){
        if(!word)return 0;
        const std::uint32_t low=word*7u;
        const std::uint32_t high=((word>>16)*7u+(((word&0xffffu)*7u)>>16))>>16;
        int top;std::uint32_t mantissa,rest=0,half=0;
        if(high){
            top=high>=4?34:high>=2?33:32;
            const int shift=top-23; // 9..11
            mantissa=(high<<(32-shift))|(low>>shift);
            rest=low&((1u<<shift)-1u);half=1u<<(shift-1);
        }else{
            top=31;while(!(low>>top))--top;
            if(top<=23)mantissa=low<<(23-top);
            else{
                const int shift=top-23; // 1..8
                mantissa=low>>shift;rest=low&((1u<<shift)-1u);half=1u<<(shift-1);
            }
        }
        if(half&&(rest>half||(rest==half&&(mantissa&1u))))++mantissa;
        if(mantissa>>24){mantissa>>=1;++top;}
        return (std::uint32_t(top-32+127)<<23)|(mantissa&0x7fffffu);
    }
    // scrTimeShard's lengthdir velocity writes, after Create.
    void setVelocity(std::int32_t horizontal,std::int32_t vertical){xsp=horizontal;ysp=vertical;}
    // TimeShardAnimation::advanceImage: the first advance reconciles
    // Create's random(7) image_index through EffectSequenceAnimation; later
    // advances add image_speed to the binary32 timeline in exact Q27.
    bool advanceImage(){
        if(destroyRequested)return true;
        constexpr std::int32_t frames=6;
        std::int32_t rate27;bool exact=false;
        if(timed&&(!Binary32Timeline::fromBits(imageSpeed.bits,rate27,exact)||!exact))timed=false;
        if(!timed){
            if(!EffectSequenceAnimation::advance(timeline,imageSpeed,frames,imageIndex))return false;
            timed=Binary32Timeline::fromBits(timeline.bits,frame27,exact)&&exact&&frame27>=0&&
                imageIndex.bits==(timeline.bits&0x7fffffffu);
            return true;
        }
        if(rate27>frames*Binary32Timeline::one||rate27< -frames*Binary32Timeline::one)return false;
        Binary32Timeline::advance(frame27,rate27,frames*Binary32Timeline::one);
        timeline.bits=imageIndex.bits=Binary32Timeline::toBits(frame27);
        return true;
    }
    // TimeShardLifecycle's alarms and contact.
    void advanceAlarm(unsigned index){
        if(destroyRequested)return;
        switch(index){
        case 0: if(AlarmBank::advanceValue(alarm0))fireAlarm(index);break;
        case 1: if(AlarmBank::advanceValue(alarm1))fireAlarm(index);break;
        case 2: if(AlarmBank::advanceValue(alarm2))fireAlarm(index);break;
        default: break;
        }
    }
    void fireAlarm(unsigned index){
        if(destroyRequested)return;
        if(index==0)destroyRequested=true;
        if(index==1)disappearing=true;
        if(index==2)noTouch=false;
    }
    // sign(player speed) * random(2) per axis; a stationary axis still draws.
    template<class Random> bool playerContact(MotionBinary64 playerXsp,MotionBinary64 playerYsp,Random& random){
        if(destroyRequested||noTouch)return true;
        if(!playerXsp.valid()||!playerYsp.valid())return false;
        xsp=contactVelocity(playerXsp,random.nextU32());
        ysp=contactVelocity(playerYsp,random.nextU32());
        noTouch=true;alarm2=5;return true;
    }
    // TimeShardMotion::step on Q16 values. Count returns the live shard
    // population (negative on failure); Outside(effect, retire) tests the
    // OLD published y; Destroy runs once, at the first destroy request.
    template<class Random,class Meeting,class Count,class Outside,class Destroy>
    bool step(bool stopped,Random& random,Meeting meeting,Count count,Outside outside,Destroy destroy){
        if(destroyRequested)return true;
        auto requestDestroy=[&](){
            if(destroyRequested)return true;
            destroyRequested=true;return destroy();
        };
        constexpr std::int32_t range=std::int32_t(1)<<29,fast=16*65536;
        if(x16<=-range||x16>=range||y16<=-range||y16>=range||
           xsp< -fast||xsp>fast||ysp< -fast||ysp>fast)return false;
        if(!stopped){
            ysp+=ysp<fall?hardGravity:gravity;
            imageSpeed=savedSpeed;
        }else{
            imageSpeed={};
            if(alarm0>0)++alarm0;
            if(alarm1>0)++alarm1;
        }
        bool hit=false;
        const std::int32_t firstX=x16+xsp,firstY=y16;
        if(!meeting(firstX,firstY,hit))return false;
        const bool firstHit=hit;
        if(hit)xsp=-Fixed16::scale(xsp,bounce);
        if(!meeting(x16,y16+ysp,hit))return false;
        if(hit){
            if(ysp<=0)ysp=0;
            // Both fractional bounds truncate to zero; the draws still happen.
            else ysp=std::int32_t(random.irandomSmall(0))*65536;
            xsp=Fixed16::scale(xsp,bounce);
            static const std::uint32_t rates[]={0xbe99999au,0xbdcccccdu,0,0x3dcccccdu,0x3e99999au};
            imageSpeed=RunnerImageValue{rates[random.chooseIndexSmall(5)]};
        }
        if(xsp>-creep&&xsp<creep)xsp=xsp>0?creep:xsp<0?-creep:0;
        x16+=xsp;y16+=ysp;
        xsp=Fixed16::scale(xsp,drag);ysp=Fixed16::scale(ysp,drag);
        if(xsp>-creep&&xsp<creep)xsp=0;
        if(ysp>-creep&&ysp<creep)ysp=0;
        // A resting shard ends where the first probe was: nothing in between
        // moves solids, so that query's answer stands.
        if(x16==firstX&&y16==firstY)hit=firstHit;
        else if(!meeting(x16,y16,hit))return false;
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
            if(hit&&!requestDestroy())return false;
        }
        // A destroy request does not abort the remaining instructions.
        dflash=disappearing?-dflash:-1;
        const int population=count();
        if(population<0)return false;
        if(population>shardLimit&&!requestDestroy())return false;
        bool retire=false;
        if(!outside(*this,retire))return false;
        if(retire&&!requestDestroy())return false;
        x.v=Fixed16::roundPixel(x16);y.v=Fixed16::roundPixel(y16);
        return true;
    }
private:
    // random(2) in Q16 with the player's sign; zero speed keeps zero.
    static std::int32_t contactVelocity(MotionBinary64 velocity,std::uint32_t word){
        if(velocity.zero())return 0;
        const std::int32_t magnitude=std::int32_t(word>>16)*2;
        return (velocity.high&0x80000000u)?-magnitude:magnitude;
    }
};
}
#endif
