#ifndef DOWNWELL_CORE_WHITE_JELLY_STEP_H
#define DOWNWELL_CORE_WHITE_JELLY_STEP_H
#include "WhiteJellyBirth.h"
#include "RunnerRealComparison.h"
#include "CheckCollisionWithScript.h"
#include "EnemyStompScript.h"
#include "ContactDamageScripts.h"
namespace downwell {
// enmWhiteJelly Step_0. Operations are synchronous checked owner boundaries:
// meeting must query the caller's current mask against the selected target;
// child effects must complete Create before returning. This is not live admission.
struct WhiteJellyStep : WhiteJellyBirth, CheckCollisionWithScript {
    Real directionDifference{};
    ContactDamageScripts damageScripts;
    template<class Player> bool playerContact(Player& player){
        if(!initialized)return false;
        Real threshold;int order;
        if(!player.globalY(threshold)||!threshold.add(Real::fromInteger(2))||
            !compareRunnerReal(yy,threshold,order))return false;
        return order>0?player.stomp():player.damage(1,3,2);
    }
    template<class Events> bool stompPlayerChecked(Events& events,bool& good){
        return EnemyStompScript::run(health,hit,stomped,damageScripts.jump,damageScripts.shake,
            events,[&]{return events.createBlast(276,x.promoted(),y.promoted());},good);
    }
    template<class Query> bool wallCollision(Query query){
        return CheckCollisionWithScript::run(57,xx,yy,xsp,ysp,query);
    }
    static Real negative(Real v){v.high^=0x80000000u;return v;}
    template<class Ops> bool setDirection(Real wanted,Ops& ops){
        Real accepted;if(!ops.direction(wanted,accepted))return false;
        direction=accepted;return true;
    }
    template<class Ops> bool setSpeed(Real wanted,Ops& ops){
        RunnerImageValue requested,accepted;
        if(!requested.assign(wanted)||!ops.imageSpeed(requested,accepted))return false;
        imageSpeed=accepted;return true;
    }
    template<class Random,class Ops> bool enemyContact(Random& random,Ops& ops){
        if(!initialized)return false;
        Real selfSum=x.promoted(),otherX,otherY;
        if(!selfSum.add(y.promoted())||!ops.otherX(otherX)||!ops.otherY(otherY)||!otherX.add(otherY))return false;
        int order;if(!compareRunnerReal(selfSum,otherX,order))return false;
        if(!order)return setDirection(Real::fromInteger(int(random.irandomSmall(359))),ops);
        // Source evaluates the target (self) first, then other.y and other.x.
        const Real targetY=y.promoted(),targetX=x.promoted();Real angle;
        if(!ops.otherY(otherY)||!ops.otherX(otherX)||
            !ops.pointDirection(otherX,otherY,targetX,targetY,angle))return false;
        return setDirection(angle,ops);
    }
    template<class Random,class Ops> bool effectAlarm(bool timeStop,Random& random,Ops& ops){
        if(!initialized)return false;
        if(!timeStop){
            Real angle;
            if(!randomBinary64(random.nextU32(),Real::fromInteger(40),angle)||
                !angle.add(Real::fromInteger(-20))||!angle.add(Real::fromInteger(90))||
                !ops.movingFx(xx,yy,704,{0x3fb99999u,0x9999999au},angle,{0x3fe00000u,0}))return false;
        }
        // emitTimer is not assigned by Create: read it through the owner,
        // including its missing-variable failure, rather than invent a value.
        Real timer;return ops.emitTimer(timer)&&ops.effectAlarmReset(timer);
    }
    template<class Ops> bool endStep(Ops& ops){
        if(!initialized)return false;
        Real boundary;int order;
        if(!ops.viewY(boundary)||!boundary.add(Real::fromInteger(-160))||
            !compareRunnerReal(y.promoted(),boundary,order))return false;
        bool outside=order<0;
        if(!outside){
            if(!ops.viewY(boundary)||!boundary.add(Real::fromInteger(3000))||
                !compareRunnerReal(y.promoted(),boundary,order))return false;
            outside=order>0;
        }
        if(!outside)return true;
        int room;return ops.room(room)&&(room!=11||ops.destroy());
    }
    template<class Random,class Ops> bool step(bool timeStop,Random& random,Ops& ops){
        if(!initialized)return false;
        int order;
        if(!timeStop){
            if(!compareRunnerReal(imageSpeed.promoted(),imageRate,order))return false;
            if(order&&!setSpeed(imageRate,ops))return false;
            if(scriptActive){
                if(hit){
                    hit=false;
                    if(!setDirection(hitDirection,ops))return false;
                    moveSpeed=Real::fromInteger(3);
                    if(!ops.imageIndex(0)||!ops.sound(118,50,0,1,"enemymove"))return false;
                    hitStun=true;
                    if(!ops.alarm(1,5))return false;
                }
                if(!hitStun){
                    Real targetY,targetX,angle;
                    if(!ops.enemyPlayerY(targetY)||!ops.enemyPlayerX(targetX)||
                        !ops.pointDirection(xx,yy,targetX,targetY,angle))return false;
                    playerDirection=angle;directionDifference=direction;
                    if(!directionDifference.add(negative(playerDirection))||
                        !compareRunnerReal(directionDifference,Real::fromInteger(180),order))return false;
                    if(order>0){if(!directionDifference.add(Real::fromInteger(-360)))return false;}
                    else {
                        if(!compareRunnerReal(directionDifference,Real::fromInteger(-180),order))return false;
                        if(order<0&&!directionDifference.add(Real::fromInteger(360)))return false;
                    }
                    Real magnitude=directionDifference;magnitude.high&=0x7fffffffu;
                    if(!compareRunnerReal(magnitude,Real::fromInteger(cone),order))return false;
                    if(order<0){
                        if(!compareRunnerReal(moveSpeed,maxSpeed,order))return false;
                        if(order<0&&!moveSpeed.add(acceleration))return false;
                    }else{
                        if(!compareRunnerReal(moveSpeed,Real::fromInteger(1),order))return false;
                        if(order>0&&!moveSpeed.add(negative(deceleration)))return false;
                    }
                    if(!compareRunnerReal(moveSpeed,maxSpeed,order))return false;
                    if(order>0&&!moveSpeed.add({0xbfc99999u,0x9999999au}))return false;
                    if(!compareRunnerReal(directionDifference,{},order))return false;
                    if(order){
                        Real wanted=direction;
                        if(!wanted.add(order>0?negative(rotateSpeed):rotateSpeed)||!setDirection(wanted,ops))return false;
                    }
                    if(!compareRunnerReal(direction,Real::fromInteger(180),order))return false;
                    maxSpeed=order>0?Real{0x3fe99999u,0x9999999au}:Real{0x3fd99999u,0x9999999au};
                    Real component;
                    if(!ops.lengthdir(false,moveSpeed,direction,component))return false;
                    xsp=component;
                    if(!ops.lengthdir(true,moveSpeed,direction,component))return false;
                    ysp=component;
                    if(!wallCollision([&](Real px,Real py,int target,bool& hit){
                        return ops.meeting(px,py,target,hit);
                    }))return false;
                    if(xcollision)xsp={};
                    if(ycollision){
                        ysp={};
                        if(ycollision==-1&&!setDirection(Real::fromInteger(270),ops))return false;
                    }
                    if(!xx.add(xsp)||!yy.add(ysp))return false;
                }
            }
            std::int32_t pixel;RunnerImageValue wanted,accepted;
            if(!xx.roundPixel(pixel)||!wanted.assignInteger(pixel)||!ops.position(false,wanted,accepted))return false;
            x=accepted;
            if(!yy.roundPixel(pixel)||!wanted.assignInteger(pixel)||!ops.position(true,wanted,accepted))return false;
            y=accepted;
        }else if(!setSpeed({},ops))return false;
        // Death is outside the time-stop and script-active guards.
        if(health<=0)alive=false;
        if(!alive){
            if(!ops.enemyDeath()||!ops.smoke(x.promoted(),y.promoted(),1,0))return false;
            for(unsigned i=0;i<4;++i){
                Real speed,angle,rate,span{0x3fd33333u,0x33333333u};
                if(!span.add({0xbfc99999u,0x9999999au})||
                    !randomBinary64(random.nextU32(),Real::fromInteger(1),speed)||!speed.add(Real::fromInteger(1))||
                    !randomBinary64(random.nextU32(),Real::fromInteger(359),angle)||
                    !randomBinary64(random.nextU32(),span,rate)||!rate.add({0x3fc99999u,0x9999999au})||
                    !ops.movingFx(xx,yy,704,rate,angle,speed))return false;
            }
            if(!ops.flash(x.promoted(),y.promoted(),1,0,0)||!ops.currency(money)||
                !ops.sound(117,50,0,1,"enemymove")||!ops.destroy())return false;
        }
        // instance_destroy does not abort the current event.
        return ops.sprite(hitStun?175:176);
    }
};
}
#endif
