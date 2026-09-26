#ifndef DOWNWELL_CORE_RUNNER_SPRITE_BOUNDS_H
#define DOWNWELL_CORE_RUNNER_SPRITE_BOUNDS_H
#include "RunnerSpatialSplit.h"
namespace downwell {
// Ordinary sprite, zero angle: 0x46616d..0x466328. Not the skeletal-sprite
// branch. Each arithmetic operation narrows to binary32. Compatibility mode
// takes an explicit x87 integer-rounding policy; runtime FP mode is not assumed.
struct RunnerSpriteBounds {
    using Math=RunnerSpatialSplit;
    using Real=Math::Real;
    using Box=Math::Box;
    template<class Round> static bool unrotated(Box margins,int originX,int originY,
        Real x,Real y,Real scaleX,Real scaleY,bool compatibility,Round round,Box& result){
        if(!Math::valid(margins.left)||!Math::valid(margins.right)||
           !Math::valid(margins.top)||!Math::valid(margins.bottom)||
           !Math::valid(x)||!Math::valid(y)||!Math::valid(scaleX)||!Math::valid(scaleY))return false;
        Box next;
        if(!axis(margins.left,margins.right,originX,x,scaleX,compatibility,round,next.left,next.right)||
           !axis(margins.top,margins.bottom,originY,y,scaleY,compatibility,round,next.top,next.bottom))return false;
        result=next;return true;
    }
private:
    static bool add(Real a,Real b,Real& out){return Math::sub(a,Math::negate(b),out);}
    static bool mul(Real a,Real b,Real& out){
        MotionBinary64 product;
        return multiplyBinary64(a.promoted(),b.promoted(),product)&&out.assign(product);
    }
    template<class Round> static bool axis(Real low,Real high,int origin,Real position,
        Real scale,bool compatibility,Round& round,Real& near,Real& far){
        Real width,offset;const Real one{0x3f800000};
        if(!offset.assign(MotionBinary64::fromInteger(origin))||
           !Math::sub(high,low,width)||!add(width,one,width)||
           !Math::sub(low,offset,near)||!mul(near,scale,near)||
           !mul(scale,width,far)||!add(near,position,near)||!add(far,near,far))return false;
        if(compatibility){
            std::int32_t a,b;
            if(!round(near,a)||!round(far,b)||
               !near.assign(MotionBinary64::fromInteger(a))||!far.assign(MotionBinary64::fromInteger(b)))return false;
        }
        if(Math::less(far,near)){const auto saved=near;near=far;far=saved;}
        // Subtract AFTER rounding and sorting, including zero-scale bounds.
        return !compatibility || Math::sub(far,one,far);
    }
};
}
#endif
