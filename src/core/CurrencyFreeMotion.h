#ifndef DOWNWELL_CORE_CURRENCY_FREE_MOTION_H
#define DOWNWELL_CORE_CURRENCY_FREE_MOTION_H
#include "CurrencyCreate.h"
#include "RunnerRealComparison.h"
#include "MultiplyBinary64.h"
namespace downwell {
// objCurrency ordinary Step 19--50. Alarm 4 has different gravity and bounce
// gain semantics; it must not use this ordinary-Step implementation.
struct CurrencyFreeMotion {
    template<class Random,class Ops> static bool run(CurrencyCreate& s,MotionBinary64 playerX,MotionBinary64 playerY,
        Random& random,Ops& ops,bool& good){
        if(!good)return false;
        auto check=[&](bool ok){if(!ok)good=false;return ok;};
        auto multiply=[&](MotionBinary64& value,MotionBinary64 factor){MotionBinary64 out;
            if(!check(multiplyBinary64(value,factor,out)))return false;value=out;return true;};
        constexpr MotionBinary64 friction{0x3fe66666u,0x66666666u},minimum{0x3fa99999u,0x9999999au};
        int order=0;
        auto bounce=[&](){
            if(!check(compareRunnerReal(s.ysp,{},order)))return false;
            if(order<=0){if(!multiply(s.ysp,{}))return false;}
            // Both real arguments truncate before range ordering: [-1,0].
            else s.ysp=MotionBinary64::fromInteger(int(random.irandomSmall(1))-1);
            if(!multiply(s.xsp,friction))return false;
            MotionBinary64 top;
            if(!check(ops.viewTop(top)&&compareRunnerReal(s.yy,top,order)))return false;
            if(order>0){int voice;if(!check(ops.sound(s.bounceSound,30,false,true,voice)))return false;s.mySound=voice;}
            return true;
        };
        if(!check(compareRunnerReal(s.ysp,s.maximumGravity,order)))return false;
        if(order<0){if(!check(s.ysp.add(s.gravity)))return false;}else s.ysp=s.maximumGravity;
        if(!check(ops.collision(87,s)))return false;
        if(s.yCollision==1){
            bool hit=false;if(!check(ops.meeting(87,s.xx,s.yy,hit)))return false;
            if(!hit&&!bounce())return false;
        }
        if(!check(ops.collision(57,s)))return false;
        if(s.xCollision&&(!multiply(s.xsp,MotionBinary64::fromInteger(-1))||!multiply(s.xsp,friction)))return false;
        if(s.yCollision&&!bounce())return false;
        auto absolute=s.xsp;absolute.high&=0x7fffffffu;
        if(!check(compareRunnerReal(absolute,minimum,order)))return false;
        if(order<0){
            const int sign=s.xsp.zero()?0:(s.xsp.high>>31)?-1:1;
            MotionBinary64 out;if(!check(multiplyBinary64(MotionBinary64::fromInteger(sign),minimum,out)))return false;s.xsp=out;
        }
        if(!check(ops.pointDirection({}, {},s.xsp,s.ysp,s.direction)))return false;
        const auto radius=MotionBinary64::fromInteger(s.attractionRadius);
        auto negRadius=radius;negRadius.high^=0x80000000u;
        auto lowX=playerX,highX=playerX,lowY=playerY,highY=playerY;
        if(!check(lowX.add(negRadius)&&highX.add(radius)&&lowY.add(negRadius)&&highY.add(radius)))return false;
        bool inside=false;
        if(!check(compareRunnerReal(s.xx,lowX,order)))return false;
        if(order>0){
            if(!check(compareRunnerReal(s.xx,highX,order)))return false;
            if(order<0){
                if(!check(compareRunnerReal(s.yy,lowY,order)))return false;
                if(order>0){if(!check(compareRunnerReal(s.yy,highY,order)))return false;inside=order<0;}
            }
        }
        if(inside&&s.suckable){
            s.attracted=true;
            if(!check(ops.pointDirection(s.xx,s.yy,playerX,playerY,s.direction)))return false;
            s.gemSpeed=MotionBinary64::fromInteger(4);s.rotationSpeed=MotionBinary64::fromInteger(32);
        }
        bool water=false;if(!check(ops.water(s.x.promoted(),s.y.promoted(),water)))return false;
        if(water&&(!multiply(s.xsp,{0x3fefae14u,0x7ae147aeu})||!multiply(s.ysp,{0x3fefae14u,0x7ae147aeu})))return false;
        if(!check(s.xx.add(s.xsp)))return false;
        return check(s.yy.add(s.ysp));
    }
};
}
#endif
