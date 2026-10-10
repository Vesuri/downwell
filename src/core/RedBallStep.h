#ifndef DOWNWELL_CORE_RED_BALL_STEP_H
#define DOWNWELL_CORE_RED_BALL_STEP_H
#include "RedBallBirth.h"
#include "CheckCollisionWithScript.h"
#include "RunnerRealComparison.h"
namespace downwell {
// Object 118's events. Publication/effects are synchronous owner boundaries;
// this state machine alone does not admit the family into a live scheduler.
struct RedBallStep : RedBallBirth, CheckCollisionWithScript {
    Real directionDifference{};
    int eye=0,eyeX=0,shotDirection=0;
    bool eyeInitialized=false;
    template<class Ops> bool draw(Ops& ops){
        if(!initialized||!ops.shade()||!ops.drawSelf())return false;
        static constexpr int sprites[]={182,184,185,184,182,183,181,183,182};
        static constexpr int mirrors[]={1,1,1,-1,-1,-1,-1,1,1};
        static constexpr int shots[]={0,45,90,135,180,225,270,315,0};
        for(unsigned row=0;row<9;++row){
            Real lower{},upper;int order;
            if(row&&!multiplyBinary64(angle16,Real::fromInteger(int(row*2-1)),lower))return false;
            if(!compareRunnerReal(playerDirection,lower,order))return false;
            if(order<0)continue;
            if(!multiplyBinary64(angle16,Real::fromInteger(row==8?16:int(row*2+1)),upper)||
                (row==8&&!upper.add(Real::fromInteger(-1)))||
                !compareRunnerReal(playerDirection,upper,order))return false;
            if(order<0){eye=sprites[row];eyeX=mirrors[row];shotDirection=shots[row];eyeInitialized=true;break;}
        }
        // The last interval ends at 359, not 360. No matching interval keeps
        // prior eye fields; do not invent them if this is the first Draw.
        if(!eyeInitialized)return false;
        const int mirror=eyeX;Real drawY=y.promoted();
        if(!drawY.add(Real::fromInteger(-5)))return false;
        const Real drawX=x.promoted();Real frame;
        if(!ops.imageIndex(frame))return false;
        return ops.drawSpriteExt(eye,frame,drawX,drawY,mirror,1,0,0xffffffu,1);
    }
    static Real negative(Real value){value.high^=0x80000000u;return value;}
    template<class Ops> bool setDirection(Real wanted,Ops& ops){
        Real accepted;if(!ops.direction(wanted,accepted))return false;direction=accepted;return true;
    }
    template<class Ops> bool setSpeed(Real wanted,Ops& ops){
        RunnerImageValue request,accepted;
        if(!request.assign(wanted)||!ops.imageSpeed(request,accepted))return false;imageSpeed=accepted;return true;
    }
    template<class Random,class Ops> bool enemyContact(Random& random,Ops& ops){
        if(!initialized)return false;
        Real wanted=direction,offset;
        return randomBinary64(random.nextU32(),Real::fromInteger(40),offset)&&
            offset.add(Real::fromInteger(-20))&&wanted.add(offset)&&setDirection(wanted,ops);
    }
    template<class Player> bool playerContact(Player& player){
        return initialized&&player.damage(1,3,2); // No stomp/height branch.
    }
    bool hitStunAlarm(){if(!initialized)return false;hitStun=false;return true;}
    template<class Random,class Ops> bool effectAlarm(bool stopped,Random& random,Ops& ops){
        if(!initialized)return false;
        if(!stopped){
            Real angle;
            if(!randomBinary64(random.nextU32(),Real::fromInteger(40),angle)||
                !angle.add(Real::fromInteger(-20))||!angle.add(Real::fromInteger(90))||
                !ops.movingFx(xx,yy,704,{0x3fb99999u,0x9999999au},angle,{0x3fe00000u,0}))return false;
        }
        // Read after the synchronous child; stopped alarms still reset.
        return ops.effectAlarmReset(emitTimer);
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
        int room;
        if(outside&&(!ops.room(room)||(room==11&&!ops.destroy())))return false;
        // scrOutofview returns even after instance_destroy: execute the
        // concrete event's additional upper bound and repeat the builtin call.
        if(!ops.viewY(boundary)||!boundary.add(Real::fromInteger(-124))||
            !compareRunnerReal(y.promoted(),boundary,order))return false;
        return order>=0||(ops.room(room)&&(room!=11||ops.destroy()));
    }
    template<class Ops> bool reflection(Real delta,bool subtract,Ops& ops){
        Real doubled,wanted=direction;
        return multiplyBinary64(delta,Real::fromInteger(2),doubled)&&
            wanted.add(subtract?negative(doubled):doubled)&&setDirection(wanted,ops);
    }
    template<class Ops> bool reflectWalls(Ops& ops){
        int order;Real delta;
        if(xcollision){
            xsp={};
            if(xcollision==1||xcollision==-1){
                if(!compareRunnerReal(direction,Real::fromInteger(180),order))return false;
                if(xcollision==1){
                    delta=order>0?direction:Real::fromInteger(90);
                    if(!delta.add(order>0?Real::fromInteger(-270):direction)||!reflection(delta,order>0,ops))return false;
                }else{
                    delta=order>0?Real::fromInteger(270):direction;
                    if(!delta.add(order>0?negative(direction):Real::fromInteger(-90))||!reflection(delta,order<=0,ops))return false;
                }
            }
        }
        if(ycollision){
            ysp={};
            if(ycollision==1||ycollision==-1){
                // Observe the already-published X reflection before Y.
                if(!compareRunnerReal(direction,Real::fromInteger(90),order))return false;
                bool inside=order>0;
                if(inside){if(!compareRunnerReal(direction,Real::fromInteger(270),order))return false;inside=order<=0;}
                if(ycollision==1){
                    delta=inside?direction:Real::fromInteger(360);
                    if(!delta.add(inside?Real::fromInteger(-180):negative(direction))||!reflection(delta,inside,ops))return false;
                }else{
                    delta=inside?Real::fromInteger(180):direction;
                    if(inside&&!delta.add(negative(direction)))return false;
                    if(!reflection(delta,!inside,ops))return false;
                }
            }
        }
        return true;
    }
    template<class Random,class Ops> bool step(bool stopped,Random& random,Ops& ops){
        if(!initialized)return false;int order;
        if(!stopped){
            if(!compareRunnerReal(imageSpeed.promoted(),imageRate,order)||(order&&!setSpeed(imageRate,ops)))return false;
            if(scriptActive){
                if(hit){
                    hit=false;if(!setDirection(hitDirection,ops))return false;
                    moveSpeed=Real::fromInteger(3);
                    if(!ops.imageIndex(0)||!ops.sound(126,50,0,1,"enemymove"))return false;
                    hitStun=true;if(!ops.alarm(1,5))return false;
                }
                // Unlike the jelly, hit stun does not bypass red-ball steering.
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
                    if(order<0){bool visible;if(!ops.inView(visible)||(visible&&!moveSpeed.add(acceleration)))return false;}
                }else{
                    if(!compareRunnerReal(moveSpeed,Real::fromInteger(1),order))return false;
                    if(order>0&&!moveSpeed.add(negative(deceleration)))return false;
                }
                if(!compareRunnerReal(moveSpeed,maxSpeed,order))return false;
                if(order>0&&!moveSpeed.add({0xbfc99999u,0x9999999au}))return false;
                if(!compareRunnerReal(directionDifference,{},order))return false;
                if(order){Real wanted=direction;
                    if(!wanted.add(order>0?negative(rotateSpeed):rotateSpeed)||!setDirection(wanted,ops))return false;
                }
                Real component;
                if(!ops.lengthdir(false,moveSpeed,direction,component))return false;xsp=component;
                if(!ops.lengthdir(true,moveSpeed,direction,component))return false;ysp=component;
                if(!CheckCollisionWithScript::run(57,xx,yy,xsp,ysp,[&](auto x,auto y,int object,bool& hit){
                    return ops.meeting(x,y,object,hit);
                })||!reflectWalls(ops))return false;
                if(!compareRunnerReal(xsp,{},order))return false;
                if(order&&!ops.xscale(xsp.high>>31?-1:1))return false;
                if(!xx.add(xsp)||!yy.add(ysp))return false;
            }
            std::int32_t pixel;RunnerImageValue wanted,accepted;
            if(!xx.roundPixel(pixel)||!wanted.assignInteger(pixel)||!ops.position(false,wanted,accepted))return false;x=accepted;
            if(!yy.roundPixel(pixel)||!wanted.assignInteger(pixel)||!ops.position(true,wanted,accepted))return false;y=accepted;
        }else if(!setSpeed({},ops))return false;
        if(health<=0)alive=false;
        if(!alive){
            if(!ops.enemyDeath())return false;
            for(unsigned i=0;i<4;++i){
                Real speed,angle,rate,span{0x3fd33333u,0x33333333u};
                if(!span.add({0xbfc99999u,0x9999999au})||
                    !randomBinary64(random.nextU32(),Real::fromInteger(1),speed)||!speed.add(Real::fromInteger(1))||
                    !randomBinary64(random.nextU32(),Real::fromInteger(359),angle)||
                    !randomBinary64(random.nextU32(),span,rate)||!rate.add({0x3fc99999u,0x9999999au})||
                    !ops.movingFx(xx,yy,704,rate,angle,speed))return false;
            }
            if(!ops.flash(x.promoted(),y.promoted(),1,0,0)||!ops.currency(money)||
                !ops.sound(125,50,0,1,"enemymove")||!ops.destroy())return false;
        }
        return ops.sprite(hitStun?703:702); // Also executes after destruction.
    }
};
}
#endif
