#ifndef DOWNWELL_CORE_RED_BALL_FIXED_STEP_H
#define DOWNWELL_CORE_RED_BALL_FIXED_STEP_H
#include "SteeringFixedState.h"
namespace downwell {
// enmRedBall 118 and neonRedBall 173 (set neon before create) in Q16 fixed
// point for the compact world (SteeringFixedState). RedBallBirth and
// RedBallStep are the VM forms and the oracle. Ops as WhiteJellyFixedStep's,
// plus inView(visible), xscale(sign), effectAlarmReset(ticks) and
// movingFxQ16 for the effect alarm; no smoke.
struct RedBallFixedStep : SteeringFixedState, CheckCollisionFixed {
    // movesp constants in thousandths: 0.05, 0.25 and 0.2.
    static constexpr std::int32_t accelerationMilli=50,decelerationMilli=250,slowMilli=200,oneMilli=1000;
    static constexpr std::int32_t rotate16=2*65536,angle16=1474560; // 2 and 22.5 degrees
    static constexpr std::uint32_t trailRateBits=0x3dcccccdu; // 0.1 as binary32
    static constexpr int emitTicks=10;
    bool neon=false;
    int eye=0,eyeX=0,shotDirection=0;
    bool eyeInitialized=false;

    template<class Random,class Ops> bool create(std::int32_t px,std::int32_t py,Random& random,Ops& ops){
        if(!inheritedCreate(px,py,ops))return false;
        health=neon?40:30;money=neon?10:20;
        random.nextU32(); // velocity = random_range(1, 1.5), never read
        moveSpeedMilli=neon?1000:100;maxSpeedMilli=neon?1500:1200;cone=90;
        if(!aimAtPlayer(ops)||!ops.alarm(0,emitTicks)||!ops.alarm(2,30)||
           !publishImageSpeed(imageRateBits,ops))return false;
        initialized=true;return true;
    }
    // Draw's eye: the 22.5-degree sector of playerDir. The last sector ends
    // at 359, not 360: a first Draw in (359, 360) has no eye.
    bool updateEye(){
        if(!initialized)return false;
        static constexpr int sprites[]={182,184,185,184,182,183,181,183,182};
        static constexpr int mirrors[]={1,1,1,-1,-1,-1,-1,1,1};
        static constexpr int shots[]={0,45,90,135,180,225,270,315,0};
        for(int row=0;row<9;++row){
            const std::int32_t lower=row?angle16*(row*2-1):0;
            const std::int32_t upper=row==8?angle16*16-65536:angle16*(row*2+1);
            if(playerDirection16<lower)continue;
            if(playerDirection16<upper){
                eye=sprites[row];eyeX=mirrors[row];shotDirection=shots[row];eyeInitialized=true;break;
            }
        }
        return eyeInitialized;
    }
    template<class Random,class Ops> bool enemyContact(Random& random,Ops&){
        if(!initialized)return false;
        setDirection(direction16+Fixed16::random(random.nextU32(),Fixed16::fromInteger(40)).v-20*65536);
        return true;
    }
    bool hitStunAlarm(){if(!initialized)return false;hitStun=false;return true;}
    template<class Random,class Ops> bool effectAlarm(bool stopped,Random& random,Ops& ops){
        if(!initialized)return false;
        if(!stopped){
            const Fixed16 angle=Fixed16::raw(Fixed16::random(random.nextU32(),Fixed16::fromInteger(40)).v+70*65536);
            RunnerImageValue rate;rate.bits=trailRateBits;
            if(!ops.movingFxQ16(Fixed16::raw(x16),Fixed16::raw(y16),704,rate,angle,Fixed16::raw(32768)))return false;
        }
        // Read after the synchronous child; stopped alarms still reset.
        return ops.effectAlarmReset(emitTicks);
    }
    template<class Random,class Ops> bool step(bool stopped,Random& random,Ops& ops){
        if(!initialized)return false;
        if(!stopped){
            if(imageSpeed.bits!=imageRateBits&&!publishImageSpeed(imageRateBits,ops))return false;
            if(scriptActive){
                if(hit){
                    hit=false;setDirection(hitDirection16);
                    moveSpeedMilli=hitMilli;
                    if(!ops.imageIndex(0)||!ops.sound(126,50,0,1,"enemymove"))return false;
                    hitStun=true;if(!ops.alarm(1,5))return false;
                }
                // Unlike the jelly, hit stun does not bypass red-ball steering.
                if(!steer(ops))return false;
            }
            if(!publishPosition(ops))return false;
        }else if(!publishImageSpeed(0,ops))return false;
        if(health<=0)alive=false;
        if(!alive){
            if(!ops.enemyDeath()||!deathBurst(random,ops)||
               !ops.flashQ16(Fixed16::fromInteger(pixelX),Fixed16::fromInteger(pixelY))||!ops.currency(money)||
               !ops.sound(125,50,0,1,"enemymove")||!ops.destroy())return false;
        }
        return ops.sprite(hitStun?703:neon?352:702); // Also executes after destruction.
    }
private:
    // direction += 2 * delta, or -= when subtract: the source's reflections.
    void reflect(std::int32_t delta,bool subtract){setDirection(direction16+(subtract?-2*delta:2*delta));}
    void reflectWalls(){
        if(xcollision){
            xsp16=0;
            const bool above=direction16>180*65536;
            if(xcollision==1)reflect(above?direction16-270*65536:direction16+90*65536,above);
            else reflect(above?270*65536-direction16:direction16-90*65536,!above);
        }
        if(ycollision){
            ysp16=0;
            // Observe the X reflection before Y.
            const bool inside=direction16>90*65536&&direction16<=270*65536;
            if(ycollision==1)reflect(inside?direction16-180*65536:360*65536-direction16,inside);
            else reflect(inside?180*65536-direction16:direction16,!inside);
        }
    }
    template<class Ops> bool steer(Ops& ops){
        std::int32_t targetX,targetY;
        if(!ops.enemyPlayerQ16(true,targetY)||!ops.enemyPlayerQ16(false,targetX))return false;
        playerDirection16=FixedTrig::pointDirection(targetX-x16,targetY-y16);
        std::int32_t difference=direction16-playerDirection16;
        if(difference>180*65536)difference-=full;
        else if(difference< -180*65536)difference+=full;
        if(magnitude(difference)<cone*65536){
            if(moveSpeedMilli<maxSpeedMilli){
                // scrInView(0,0,0) is queried only below the cap.
                bool visible;if(!ops.inView(visible))return false;
                if(visible)moveSpeedMilli+=accelerationMilli;
            }
        }else if(moveSpeedMilli>oneMilli)moveSpeedMilli-=decelerationMilli;
        if(moveSpeedMilli>maxSpeedMilli)moveSpeedMilli-=slowMilli;
        if(difference)setDirection(direction16+(difference>0?-rotate16:rotate16));
        if(moveSpeedMilli<0||moveSpeedMilli>hitMilli)return false;
        const std::int32_t length=speed16();
        xsp16=FixedTrig::lengthdir(false,length,direction16);
        ysp16=FixedTrig::lengthdir(true,length,direction16);
        if(!CheckCollisionFixed::run(x16,y16,xsp16,ysp16,[&](std::int32_t px,std::int32_t py,bool& found){
            return ops.meetingQ16(px,py,57,found);
        }))return false;
        reflectWalls();
        if(xsp16&&!ops.xscale(xsp16<0?-1:1))return false;
        x16+=xsp16;y16+=ysp16;
        return true;
    }
};
}
#endif
