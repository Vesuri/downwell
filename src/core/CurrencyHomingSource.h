#ifndef DOWNWELL_CORE_CURRENCY_HOMING_SOURCE_H
#define DOWNWELL_CORE_CURRENCY_HOMING_SOURCE_H
#include "CurrencyCreate.h"
#include "RunnerRealComparison.h"
namespace downwell {
// objCurrency Step 61--74 (also the attracted branch of Alarm 4).
// Point-direction and the two lengthdir calls remain explicit runner inputs.
struct CurrencyHomingSource {
    template<class Math> static bool run(CurrencyCreate& s,MotionBinary64 playerX,MotionBinary64 playerY,Math& math,bool& good){
        if(!good)return false;
        auto check=[&](bool ok){if(!ok)good=false;return ok;};
        auto negate=[](MotionBinary64 value){value.high^=0x80000000u;return value;};
        int order=0;s.stepRate=1;
        if(!check(math.pointDirection(s.xx,s.yy,playerX,playerY,s.playerDirection)))return false;
        s.directionDifference=s.direction;
        if(!check(s.directionDifference.add(negate(s.playerDirection))&&
            compareRunnerReal(s.directionDifference,MotionBinary64::fromInteger(180),order)))return false;
        if(order>0){if(!check(s.directionDifference.add(MotionBinary64::fromInteger(-360))))return false;}
        else{
            if(!check(compareRunnerReal(s.directionDifference,MotionBinary64::fromInteger(-180),order)))return false;
            if(order<0&&!check(s.directionDifference.add(MotionBinary64::fromInteger(360))))return false;
        }
        if(!check(compareRunnerReal(s.directionDifference,s.rotationSpeed,order)))return false;
        if(order>0){if(!check(s.direction.add(negate(s.rotationSpeed))))return false;}
        else{
            if(!check(compareRunnerReal(s.directionDifference,negate(s.rotationSpeed),order)))return false;
            if(order<0){if(!check(s.direction.add(s.rotationSpeed)))return false;}
            else s.direction=s.playerDirection;
        }
        if(!check(s.rotationSpeed.add({0x3fe33333u,0x33333333u})&&
            compareRunnerReal(s.rotationSpeed,MotionBinary64::fromInteger(32),order)))return false;
        if(order>0)s.rotationSpeed=MotionBinary64::fromInteger(32);
        if(!check(s.gemSpeed.add({0x3fc00000u,0})&&compareRunnerReal(s.gemSpeed,MotionBinary64::fromInteger(9),order)))return false;
        if(order>0)s.gemSpeed=MotionBinary64::fromInteger(9);
        if(!check(math.lengthdirX(s.gemSpeed,s.direction,s.xsp)))return false;
        if(!check(math.lengthdirY(s.gemSpeed,s.direction,s.ysp)))return false;
        if(!check(s.xx.add(s.xsp)))return false;
        return check(s.yy.add(s.ysp));
    }
};
}
#endif
