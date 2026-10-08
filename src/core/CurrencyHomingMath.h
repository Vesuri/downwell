#ifndef DOWNWELL_CORE_CURRENCY_HOMING_MATH_H
#define DOWNWELL_CORE_CURRENCY_HOMING_MATH_H
#include "RunnerPointDirection.h"
#include "SmokeDirection.h"
#include "SmokeTrig.h"
namespace downwell {
struct CurrencyHomingMath {
    using Atan=bool(*)(MotionBinary64,MotionBinary64,MotionBinary64&);
    Atan atan;
    explicit CurrencyHomingMath(Atan implementation):atan(implementation){}
    bool pointDirection(MotionBinary64 ax,MotionBinary64 ay,MotionBinary64 bx,MotionBinary64 by,MotionBinary64& out){
        return atan&&RunnerPointDirection::evaluate(ax,ay,bx,by,out,atan);
    }
    static bool component(MotionBinary64 length,MotionBinary64 degrees,bool y,MotionBinary64& out){
        RunnerImageValue speed,angle,radians;MotionBinary64 trig;
        return speed.assign(length)&&angle.assign(degrees)&&SmokeDirection::radians(angle,radians)&&
            smokeTrigComponent(radians.promoted(),!y,trig)&&SmokeDirection::component<0x41100000u>(speed,trig,y,out);
    }
    bool lengthdirX(MotionBinary64 length,MotionBinary64 degrees,MotionBinary64& out){return component(length,degrees,false,out);}
    bool lengthdirY(MotionBinary64 length,MotionBinary64 degrees,MotionBinary64& out){return component(length,degrees,true,out);}
};
}
#endif
