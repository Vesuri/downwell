#ifndef DOWNWELL_CORE_RUNNER_POINT_DIRECTION_H
#define DOWNWELL_CORE_RUNNER_POINT_DIRECTION_H
#include "RunnerImageValue.h"
#include "MultiplyBinary64.h"
namespace downwell {
// Windows wrapper 0x53d490: narrow each argument BEFORE subtraction, narrow
// differences, atan2(dy,dx), narrow result, float *180 /float(pi), negate/wrap.
// Atan is a separate original-runner dependency, not an approximate fallback.
struct RunnerPointDirection {
    static bool dividePi(RunnerImageValue input,RunnerImageValue& result){
        if(!input.promoted().valid())return false;
        const auto sign=input.bits&0x80000000u;
        if(!(input.bits&0x7fffffffu)){result=input;return true;}
        unsigned remainder=(input.bits&0x7fffffu)|0x800000u;
        constexpr unsigned divisor=0xc90fdbu; // binary32 pi significand
        int exponent=int((input.bits>>23)&255u)-1;
        if(remainder<divisor){remainder<<=1;--exponent;}
        remainder-=divisor;
        unsigned quotient=1;
        for(unsigned bit=0;bit<23;++bit){
            remainder<<=1;quotient<<=1;
            if(remainder>=divisor){remainder-=divisor;quotient|=1;}
        }
        if(remainder*2>divisor || (remainder*2==divisor && (quotient&1)))++quotient;
        if(quotient==0x1000000u){quotient>>=1;++exponent;}
        if(exponent<=0 || exponent>=255)return false;
        result.bits=sign|(unsigned(exponent)<<23)|(quotient&0x7fffffu);return true;
    }
    template<class Atan> static bool evaluate(MotionBinary64 ax,MotionBinary64 ay,
        MotionBinary64 bx,MotionBinary64 by,MotionBinary64& output,Atan atan){
        RunnerImageValue x1,y1,x2,y2,dx,dy;
        if(!x1.assign(ax) || !y1.assign(ay) || !x2.assign(bx) || !y2.assign(by))return false;
        auto x=x2.promoted(),y=y2.promoted(),nx=x1.promoted(),ny=y1.promoted();
        nx.high^=0x80000000u;ny.high^=0x80000000u;
        if(!x.add(nx) || !y.add(ny) || !dx.assign(x) || !dy.assign(y))return false;
        return fromDifferences(dx,dy,output,atan);
    }
    // Already narrowed differences, including their signs, from a proved
    // coordinate domain. Keep every subsequent runner rounding boundary.
    template<class Atan> static bool fromDifferences(RunnerImageValue dx,RunnerImageValue dy,
        MotionBinary64& output,Atan atan){
        if(!(dx.bits&0x7fffffffu)){
            output=MotionBinary64::fromInteger(!(dy.bits&0x7fffffffu)?0:(dy.bits>>31?90:270));
            return true;
        }
        MotionBinary64 radians,product;RunnerImageValue angle,multiplied,degrees;
        if(!atan(dy.promoted(),dx.promoted(),radians) || !angle.assign(radians) ||
           !multiplyBinary64(angle.promoted(),MotionBinary64::fromInteger(180),product) ||
           !multiplied.assign(product) || !dividePi(multiplied,degrees))return false;
        if((degrees.bits>>31) || !(degrees.bits&0x7fffffffu)){
            degrees.bits^=0x80000000u;output=degrees.promoted();return true;
        }
        auto result=MotionBinary64::fromInteger(360),subtract=degrees.promoted();subtract.high^=0x80000000u;
        if(!result.add(subtract) || !degrees.assign(result))return false;
        output=degrees.promoted();return true;
    }
};
}
#endif
