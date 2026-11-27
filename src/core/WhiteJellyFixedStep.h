#ifndef DOWNWELL_CORE_WHITE_JELLY_FIXED_STEP_H
#define DOWNWELL_CORE_WHITE_JELLY_FIXED_STEP_H
#include "SteeringFixedState.h"
namespace downwell {
// enmWhiteJelly's Create and events in Q16 fixed point for the compact world
// (SteeringFixedState). WhiteJellyBirth and WhiteJellyStep are the VM forms
// and the oracle. Ops: playerQ16 and enemyPlayerQ16(vertical, value),
// meetingQ16(x, y, object, hit), otherPixel(vertical, pixel) in collisions,
// smokeQ16, flashQ16 and movingFxQ16 with Fixed16 positions, and the builtin
// setters position, imageSpeed, alarm, imageIndex and sprite.
struct WhiteJellyFixedStep : SteeringFixedState, CheckCollisionFixed {
    static constexpr std::int32_t rotate16=98304; // 1.5 degrees
    // movesp constants in thousandths: 0.05, 0.001, 0.2, 0.8, 0.4 and 1.
    static constexpr std::int32_t accelerationMilli=50,decelerationMilli=1,slowMilli=200;
    static constexpr std::int32_t fastMilli=800,cruiseMilli=400,oneMilli=1000;

    template<class Random,class Ops> bool create(std::int32_t px,std::int32_t py,Random& random,Ops& ops){
        if(!inheritedCreate(px,py,ops))return false;
        health=25;money=6;
        // velocity = random_range(0.5, 1) is never read, but its draw stays:
        // removing it changes all following room RNG.
        random.nextU32();
        moveSpeedMilli=maxSpeedMilli=slowMilli;cone=10;
        if(!aimAtPlayer(ops)||!publishImageSpeed(imageRateBits,ops))return false;
        initialized=true;return true;
    }
    // Collision with subparentEnemy: away from the other, or a random heading
    // when x + y are equal. Both read the builtin (whole-pixel) x and y.
    template<class Random,class Ops> bool enemyContact(Random& random,Ops& ops){
        if(!initialized)return false;
        std::int32_t otherX,otherY;
        if(!ops.otherPixel(false,otherX)||!ops.otherPixel(true,otherY))return false;
        if(pixelX+pixelY==otherX+otherY){
            setDirection(std::int32_t(random.irandomSmall(359))*65536);
            return true;
        }
        setDirection(FixedTrig::pointDirection((pixelX-otherX)*65536,(pixelY-otherY)*65536));
        return true;
    }
    template<class Random,class Ops> bool step(bool timeStop,Random& random,Ops& ops){
        if(!initialized)return false;
        if(!timeStop){
            // image_speed differs from imgSp's binary64 every Step: set it.
            if(!publishImageSpeed(imageRateBits,ops))return false;
            if(scriptActive){
                if(hit){
                    hit=false;
                    setDirection(hitDirection16);
                    moveSpeedMilli=hitMilli;
                    if(!ops.imageIndex(0)||!ops.sound(118,50,0,1,"enemymove"))return false;
                    hitStun=true;
                    if(!ops.alarm(1,5))return false;
                }
                if(!hitStun&&!steer(ops))return false;
            }
            if(!publishPosition(ops))return false;
        }else if(!publishImageSpeed(0,ops))return false;
        // Death is outside the time-stop and script-active guards.
        if(health<=0)alive=false;
        if(!alive){
            if(!ops.enemyDeath()||!ops.smokeQ16(Fixed16::fromInteger(pixelX),Fixed16::fromInteger(pixelY))||
               !deathBurst(random,ops)||
               !ops.flashQ16(Fixed16::fromInteger(pixelX),Fixed16::fromInteger(pixelY))||!ops.currency(money)||
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
        playerDirection16=toward;
        std::int32_t difference=direction16-toward;
        if(difference>180*65536)difference-=full;
        else if(difference< -180*65536)difference+=full;
        if(magnitude(difference)<cone*65536){
            if(moveSpeedMilli<maxSpeedMilli)moveSpeedMilli+=accelerationMilli;
        }else if(moveSpeedMilli>oneMilli)moveSpeedMilli-=decelerationMilli;
        if(moveSpeedMilli>maxSpeedMilli)moveSpeedMilli-=slowMilli;
        if(difference)setDirection(direction16+(difference>0?-rotate16:rotate16));
        maxSpeedMilli=direction16>180*65536?fastMilli:cruiseMilli;
        if(moveSpeedMilli<0||moveSpeedMilli>hitMilli)return false;
        const std::int32_t length=speed16();
        xsp16=FixedTrig::lengthdir(false,length,direction16);
        ysp16=FixedTrig::lengthdir(true,length,direction16);
        if(!CheckCollisionFixed::run(x16,y16,xsp16,ysp16,[&](std::int32_t px,std::int32_t py,bool& found){
            return ops.meetingQ16(px,py,57,found);
        }))return false;
        if(xcollision)xsp16=0;
        if(ycollision){
            ysp16=0;
            if(ycollision==-1)setDirection(270*65536);
        }
        x16+=xsp16;y16+=ysp16;
        return true;
    }
};
}
#endif
