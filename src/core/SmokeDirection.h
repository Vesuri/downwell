#ifndef DOWNWELL_CORE_SMOKE_DIRECTION_H
#define DOWNWELL_CORE_SMOKE_DIRECTION_H
#include "MultiplyBinary64.h"
#include "RunnerImageValue.h"
namespace downwell {
// The lengthdir wrappers' normal/zero arithmetic for smoke and shard bursts:
// speed [0,8], angle [-585,585] (also usual-gun projectiles). The original trig implementation is an input;
// using host sin/cos here does NOT establish original-runner trig identity.
struct SmokeDirection {
    template<unsigned MaximumAngleBits=0x44124000u>
    static bool radians(RunnerImageValue angle,RunnerImageValue& out) {
        static_assert(MaximumAngleBits<=0x44340000u,"verified degree wrapper domain through 720");
        const auto magnitude=angle.bits&0x7fffffffu;
        if(magnitude>MaximumAngleBits || ((magnitude&0x7f800000u)==0 && magnitude))return false;
        MotionBinary64 product;
        RunnerImageValue numerator;
        if(!multiplyBinary64(angle.promoted(),RunnerImageValue{0x40490fdbu}.promoted(),product) ||
           !numerator.assign(product))return false;
        if(!(numerator.bits&0x7fffffffu)){out=numerator;return true;}
        const auto m=(numerator.bits&0x7fffffu)|0x800000u;
        // Normalize the integer significand quotient before nearest-even
        // rounding; m<<8 fits in 32 bits. No general soft-float division.
        const unsigned shift=m>=11796480u?7u:8u;
        const auto scaled=m<<shift;
        auto quotient=scaled/180u;
        const auto remainder=scaled%180u;
        if(remainder>90u || (remainder==90u && (quotient&1u)))++quotient;
        int e=int((numerator.bits>>23)&255u)-int(shift);
        if(quotient&0x1000000u){quotient>>=1;++e;}
        if(e<=0 || e>=255)return false;
        out.bits=(numerator.bits&0x80000000u)|(unsigned(e)<<23)|(quotient&0x7fffffu);
        return true;
    }
    template<unsigned MaximumSpeedBits=0x41000000u>
    static bool component(RunnerImageValue speed,MotionBinary64 trig,bool negate,MotionBinary64& out) {
        static_assert(MaximumSpeedBits<=0x41100000u,"component integer extraction verified through speed 9");
        if(speed.bits>MaximumSpeedBits || ((speed.bits&0x7f800000u)==0 && speed.bits))return false;
        RunnerImageValue narrowed,value;
        MotionBinary64 product;
        if(!narrowed.assign(trig) || (narrowed.bits&0x7fffffffu)>0x3f800000u ||
           !multiplyBinary64(speed.promoted(),narrowed.promoted(),product) || !value.assign(product))return false;
        if(negate)value.bits^=0x80000000u;
        const unsigned exponent=(value.bits>>23)&255u;
        const unsigned integer=exponent<127?0:((value.bits&0x7fffffu)|0x800000u)>>(150u-exponent);
        auto integral=MotionBinary64::fromInteger(integer);
        integral.high|=value.bits&0x80000000u;
        auto fraction=value.promoted(),subtract=integral;subtract.high^=0x80000000u;
        RunnerImageValue roundedFraction;
        if(!fraction.add(subtract) || !roundedFraction.assign(fraction))return false;
        // modf truncates toward zero. Snap only tiny fractional parts, not
        // numbers just below the next integer. Keep signed zero for negatives.
        out=(roundedFraction.bits&0x7fffffffu)<0x38d1b717u?integral:value.promoted();
        return true;
    }
    // trig(radians, cosine, sine) supplies double-valued results.
    template<class Trig> static bool vector(MotionBinary64 length,MotionBinary64 degrees,
        MotionBinary64& x,MotionBinary64& y,Trig trig) {
        RunnerImageValue speed,angle,radian;
        if(!length.valid() || !degrees.valid() || !speed.assign(length) || !angle.assign(degrees) ||
           (speed.bits&0x80000000u) || speed.bits>0x41000000u || !radians(angle,radian))return false;
        MotionBinary64 cosine,sine,nx,ny;
        if(!trig(radian.promoted(),cosine,sine) || !component(speed,cosine,false,nx) ||
           !component(speed,sine,true,ny))return false;
        x=nx;y=ny;return true;
    }
};
}
#endif
