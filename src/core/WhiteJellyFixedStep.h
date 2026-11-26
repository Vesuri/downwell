#ifndef DOWNWELL_CORE_WHITE_JELLY_FIXED_STEP_H
#define DOWNWELL_CORE_WHITE_JELLY_FIXED_STEP_H
#include "WhiteJellyBirth.h"
#include "RunnerRealComparison.h"
#include "FixedPoint.h"
#include "CheckCollisionUnits.h"
namespace downwell {
// scrCheckCollisionWith on Q16 positions and velocities.
using CheckCollisionFixed=CheckCollisionUnits<65536,65536>;

// enmWhiteJelly Step with Q16 position, velocity and heading (W3 fixed
// point) for the compact world. movesp and maxsp are whole thousandths: every
// speed constant is one, so the speed cycle (0.2 rising by 0.05 to the 0.4
// or 0.8 cap) keeps the binary64 comparisons' outcomes, which Q16 constants
// would not (Q16 0.4 lies below Q16 0.2 + 4 * 0.05). WhiteJellyStep, its VM form and
// oracle, still runs on the registered owners. Create is WhiteJellyBirth's
// (binary64 RNG and birth heading), converted once; the binary64 xx, yy,
// xsp, ysp, moveSpeed, maxSpeed and direction fields are not maintained
// after it. The step needs ops.enemyPlayerQ16(vertical, value) and
// ops.meetingQ16(x, y, object, hit) besides WhiteJellyStep's operations.
struct WhiteJellyFixedStep : WhiteJellyBirth, CheckCollisionFixed {
    std::int32_t x16=0,y16=0,xsp16=0,ysp16=0,direction16=0;
    std::int32_t moveSpeedMilli=0,maxSpeedMilli=0;
    RunnerImageValue rate{}; // image_speed = imgSp (0.2) as the builtin stores it
    static constexpr std::int32_t full=360*65536;
    static constexpr std::int32_t rotate16=98304; // 1.5 degrees
    // movesp constants in thousandths: 0.05, 0.001, 0.2, 0.8, 0.4, 1 and 3.
    static constexpr std::int32_t accelerationMilli=50,decelerationMilli=1,slowMilli=200;
    static constexpr std::int32_t fastMilli=800,cruiseMilli=400,oneMilli=1000,hitMilli=3000;

    template<class Random,class Ops> bool create(RunnerImageValue px,RunnerImageValue py,Random& random,Ops& ops){
        if(!WhiteJellyBirth::create(px,py,random,ops))return false;
        Fixed16 fx,fy,heading;
        if(!Fixed16::fromBinary64(xx,fx)||!Fixed16::fromBinary64(yy,fy)||
           !Fixed16::fromBinary64(direction,heading)||!rate.assign(imageRate))return false;
        x16=fx.v;y16=fy.v;xsp16=ysp16=0;moveSpeedMilli=maxSpeedMilli=slowMilli;direction16=heading.v;
        return true;
    }
    // ceil(yy * 256): typStompable's yy > ply + 2 on the Q8 lattice.
    long stompCeilQ8()const{return (long(y16)+255)>>8;}
    // The builtin direction in [0, 360).
    void setDirection16(std::int32_t wanted){
        wanted%=full;if(wanted<0)wanted+=full;
        direction16=wanted;
    }
    // Rare events (hits, enemy contacts) keep the source's binary64 angle.
    template<class Ops> bool setDirection(MotionBinary64 wanted,Ops& ops){
        MotionBinary64 accepted;Fixed16 heading;
        if(!ops.direction(wanted,accepted)||!Fixed16::fromBinary64(accepted,heading))return false;
        direction=accepted;setDirection16(heading.v);return true;
    }
    template<class Random,class Ops> bool enemyContact(Random& random,Ops& ops){
        if(!initialized)return false;
        Real selfSum=x.promoted(),otherX,otherY;
        if(!selfSum.add(y.promoted())||!ops.otherX(otherX)||!ops.otherY(otherY)||!otherX.add(otherY))return false;
        int order;if(!compareRunnerReal(selfSum,otherX,order))return false;
        if(!order)return setDirection(Real::fromInteger(int(random.irandomSmall(359))),ops);
        const Real targetY=y.promoted(),targetX=x.promoted();Real angle;
        if(!ops.otherY(otherY)||!ops.otherX(otherX)||
            !ops.pointDirection(otherX,otherY,targetX,targetY,angle))return false;
        return setDirection(angle,ops);
    }
    template<class Random,class Ops> bool step(bool timeStop,Random& random,Ops& ops){
        if(!initialized)return false;
        if(!timeStop){
            // image_speed differs from imgSp's binary64 every Step: set it.
            RunnerImageValue accepted;
            if(!ops.imageSpeed(rate,accepted))return false;
            imageSpeed=accepted;
            if(scriptActive){
                if(hit){
                    hit=false;
                    if(!setDirection(hitDirection,ops))return false;
                    moveSpeedMilli=hitMilli;
                    if(!ops.imageIndex(0)||!ops.sound(118,50,0,1,"enemymove"))return false;
                    hitStun=true;
                    if(!ops.alarm(1,5))return false;
                }
                if(!hitStun&&!steer(ops))return false;
            }
            RunnerImageValue wanted,accepted2;
            if(!wanted.assignInteger(Fixed16::roundPixel(x16))||!ops.position(false,wanted,accepted2))return false;
            x=accepted2;
            if(!wanted.assignInteger(Fixed16::roundPixel(y16))||!ops.position(true,wanted,accepted2))return false;
            y=accepted2;
        }else{
            RunnerImageValue accepted;
            if(!ops.imageSpeed({},accepted))return false;
            imageSpeed=accepted;
        }
        // Death is outside the time-stop and script-active guards.
        if(health<=0)alive=false;
        if(!alive){
            const Real fx=Fixed16::raw(x16).binary64(),fy=Fixed16::raw(y16).binary64();
            if(!ops.enemyDeath()||!ops.smoke(x.promoted(),y.promoted(),1,0))return false;
            for(unsigned i=0;i<4;++i){
                Real speed,angle,rateValue,span{0x3fd33333u,0x33333333u};
                if(!span.add({0xbfc99999u,0x9999999au})||
                    !randomBinary64(random.nextU32(),Real::fromInteger(1),speed)||!speed.add(Real::fromInteger(1))||
                    !randomBinary64(random.nextU32(),Real::fromInteger(359),angle)||
                    !randomBinary64(random.nextU32(),span,rateValue)||!rateValue.add({0x3fc99999u,0x9999999au})||
                    !ops.movingFx(fx,fy,704,rateValue,angle,speed))return false;
            }
            if(!ops.flash(x.promoted(),y.promoted(),1,0,0)||!ops.currency(money)||
                !ops.sound(117,50,0,1,"enemymove")||!ops.destroy())return false;
        }
        // instance_destroy does not abort the current event.
        return ops.sprite(hitStun?175:176);
    }
private:
    // Turn toward eplx/eply, accelerate inside the cone, move and collide.
    template<class Ops> bool steer(Ops& ops){
        std::int32_t targetX,targetY;
        if(!ops.enemyPlayerQ16(true,targetY)||!ops.enemyPlayerQ16(false,targetX))return false;
        const std::int32_t toward=FixedTrig::pointDirection(targetX-x16,targetY-y16);
        std::int32_t difference=direction16-toward;
        if(difference>180*65536)difference-=full;
        else if(difference< -180*65536)difference+=full;
        if(magnitude(difference)<cone*65536){
            if(moveSpeedMilli<maxSpeedMilli)moveSpeedMilli+=accelerationMilli;
        }else if(moveSpeedMilli>oneMilli)moveSpeedMilli-=decelerationMilli;
        if(moveSpeedMilli>maxSpeedMilli)moveSpeedMilli-=slowMilli;
        if(difference)setDirection16(direction16+(difference>0?-rotate16:rotate16));
        maxSpeedMilli=direction16>180*65536?fastMilli:cruiseMilli;
        if(moveSpeedMilli<0||moveSpeedMilli>hitMilli)return false;
        // Nearest Q16 of the speed: 3000 * 65536 + 500 fits 32 bits.
        const std::int32_t length=std::int32_t((std::uint32_t(moveSpeedMilli)*65536u+500u)/1000u);
        xsp16=FixedTrig::lengthdir(false,length,direction16);
        ysp16=FixedTrig::lengthdir(true,length,direction16);
        if(!CheckCollisionFixed::run(x16,y16,xsp16,ysp16,[&](std::int32_t px,std::int32_t py,bool& found){
            return ops.meetingQ16(px,py,57,found);
        }))return false;
        if(xcollision)xsp16=0;
        if(ycollision){
            ysp16=0;
            if(ycollision==-1)setDirection16(270*65536);
        }
        x16+=xsp16;y16+=ysp16;
        return true;
    }
};
}
#endif
