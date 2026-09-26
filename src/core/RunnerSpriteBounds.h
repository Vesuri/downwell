#ifndef DOWNWELL_CORE_RUNNER_SPRITE_BOUNDS_H
#define DOWNWELL_CORE_RUNNER_SPRITE_BOUNDS_H
#include "RunnerSpatialSplit.h"
#include "SpritePack.h"
namespace downwell {
// Ordinary sprite bounds: 0x46616d..0x46669c. Not the skeletal-sprite
// branch. Each arithmetic operation narrows to binary32. Compatibility mode
// takes an explicit x87 integer-rounding policy; runtime FP mode is not assumed.
struct RunnerSpriteBounds {
    using Math=RunnerSpatialSplit;
    using Real=Math::Real;
    using Box=Math::Box;
    template<class Round> static bool fromSprite(const PackedSprite& sprite,Real x,Real y,
        Real scaleX,Real scaleY,bool hasRotation,Real cosine,Real sine,
        bool compatibility,Round round,Box& result){
        Box margins;
        if(!margins.left.assign(MotionBinary64::fromInteger(sprite.marginLeft))||
           !margins.top.assign(MotionBinary64::fromInteger(sprite.marginTop))||
           !margins.right.assign(MotionBinary64::fromInteger(sprite.marginRight))||
           !margins.bottom.assign(MotionBinary64::fromInteger(sprite.marginBottom)))return false;
        return hasRotation?rotated(margins,sprite.originX,sprite.originY,x,y,scaleX,scaleY,
                                   cosine,sine,compatibility,round,result):
                           unrotated(margins,sprite.originX,sprite.originY,x,y,scaleX,scaleY,
                                     compatibility,round,result);
    }
    // Nonzero-angle branch. Caller supplies the source-narrowed cosine/sine;
    // angle conversion and trigonometry are separate from this geometry.
    template<class Round> static bool rotated(Box margins,int originX,int originY,
        Real x,Real y,Real scaleX,Real scaleY,Real cosine,Real sine,
        bool compatibility,Round round,Box& result){
        const Real values[]={margins.left,margins.top,margins.right,margins.bottom,
                             x,y,scaleX,scaleY,cosine,sine};
        for(Real value:values)if(!Math::valid(value))return false;
        Real l,r,t,b;
        if(!local(margins.left,margins.right,originX,scaleX,l,r)||
           !local(margins.top,margins.bottom,originY,scaleY,t,b))return false;
        Real cl,cr,st,sb,ct,cb,sl,sr;
        if(!mul(cosine,l,cl)||!mul(cosine,r,cr)||!mul(sine,t,st)||!mul(sine,b,sb)||
           !mul(cosine,t,ct)||!mul(cosine,b,cb)||!mul(sine,l,sl)||!mul(sine,r,sr))return false;
        sort(cl,cr);sort(st,sb);sort(ct,cb);sort(sl,sr);
        Box next;
        if(!add(x,cl,next.left)||!add(next.left,st,next.left)||
           !add(x,cr,next.right)||!add(next.right,sb,next.right)||
           !add(y,ct,next.top)||!Math::sub(next.top,sr,next.top)||
           !add(y,cb,next.bottom)||!Math::sub(next.bottom,sl,next.bottom))return false;
        if(compatibility && (!roundedEdge(next.left,false,round)||!roundedEdge(next.right,true,round)||
                            !roundedEdge(next.top,false,round)||!roundedEdge(next.bottom,true,round)))return false;
        result=next;return true;
    }
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
    static void sort(Real& a,Real& b){if(Math::less(b,a)){const auto saved=a;a=b;b=saved;}}
    static bool local(Real low,Real high,int origin,Real scale,Real& near,Real& far){
        Real offset;
        // Original compares serialized margins before origin subtraction.
        sort(low,high);
        return offset.assign(MotionBinary64::fromInteger(origin))&&
            Math::sub(low,offset,near)&&Math::sub(high,offset,far)&&
            add(far,Real{0x3f800000},far)&&mul(near,scale,near)&&mul(far,scale,far);
    }
    template<class Round> static bool roundedEdge(Real& value,bool upper,Round& round){
        std::int32_t pixel;
        if(!round(value,pixel)||(upper&&pixel==(-2147483647-1)))return false;
        // Rotated path decrements the INTEGER before converting to binary32.
        return value.assign(MotionBinary64::fromInteger(upper?pixel-1:pixel));
    }
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
