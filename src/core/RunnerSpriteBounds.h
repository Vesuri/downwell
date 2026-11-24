#ifndef DOWNWELL_CORE_RUNNER_SPRITE_BOUNDS_H
#define DOWNWELL_CORE_RUNNER_SPRITE_BOUNDS_H
#include "RunnerSpatialSplit.h"
namespace downwell {
// Ordinary sprite bounds: 0x46616d..0x46669c. Not the skeletal-sprite
// branch. Each arithmetic operation narrows to binary32. Compatibility mode
// takes an explicit x87 integer-rounding policy; runtime FP mode is not assumed.
struct RunnerSpriteBounds {
    using Math=RunnerSpatialSplit;
    using Real=Math::Real;
    using Box=Math::Box;
    // Accept full artwork metadata or the compact collision-only descriptor.
    template<class Resource,class Round> static bool fromSprite(const Resource& sprite,Real x,Real y,
        Real scaleX,Real scaleY,bool hasRotation,Real cosine,Real sine,
        bool compatibility,Round round,Box& result){
        std::int32_t px,py;
        if(!hasRotation&&scaleX.bits==0x3f800000u&&scaleY.bits==0x3f800000u&&
           unitPixel(x,px)&&unitPixel(y,py)&&
           small(sprite.marginLeft)&&small(sprite.marginRight)&&small(sprite.marginTop)&&
           small(sprite.marginBottom)&&small(sprite.originX)&&small(sprite.originY)){
            Box next;
            if(!unitAxis(sprite.marginLeft,sprite.marginRight,sprite.originX,px,compatibility,round,next.left,next.right)||
               !unitAxis(sprite.marginTop,sprite.marginBottom,sprite.originY,py,compatibility,round,next.top,next.bottom))return false;
            result=next;return true;
        }
        return fromSpriteBinary32(sprite,x,y,scaleX,scaleY,hasRotation,cosine,sine,compatibility,round,result);
    }
    template<class Resource,class Round> static bool fromSpriteBinary32(const Resource& sprite,Real x,Real y,
        Real scaleX,Real scaleY,bool hasRotation,Real cosine,Real sine,
        bool compatibility,Round round,Box& result){
        Box margins;
        if(!margins.left.assignInteger(sprite.marginLeft)||
           !margins.top.assignInteger(sprite.marginTop)||
           !margins.right.assignInteger(sprite.marginRight)||
           !margins.bottom.assignInteger(sprite.marginBottom))return false;
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
    // Unit scale with integer margins: low-origin, high-low+1 and both
    // products by one are exact small integers, so only the two additions
    // of the position round. Integer arithmetic gives those operands; the
    // additions keep the reference's operand order. axisBinary32 is the oracle.
    template<class Round> static bool axis(Real low,Real high,int origin,Real position,
        Real scale,bool compatibility,Round& round,Real& near,Real& far){
        std::int32_t l,h;
        // A -0 low edge keeps its sign through low-origin; unitReal makes +0.
        if(scale.bits!=0x3f800000u||low.bits==0x80000000u||!small(origin)||
           !unitPixel(low,l)||!unitPixel(high,h))
            return axisBinary32(low,high,origin,position,scale,compatibility,round,near,far);
        if(!add(unitReal(l-origin),position,near)||!add(unitReal(h-l+1),near,far))return false;
        return finishAxis(compatibility,round,near,far);
    }
    template<class Round> static bool axisBinary32(Real low,Real high,int origin,Real position,
        Real scale,bool compatibility,Round& round,Real& near,Real& far){
        Real width,offset;const Real one{0x3f800000};
        if(!offset.assignInteger(origin)||
           !Math::sub(high,low,width)||!add(width,one,width)||
           !Math::sub(low,offset,near)||!mul(near,scale,near)||
           !mul(scale,width,far)||!add(near,position,near)||!add(far,near,far))return false;
        return finishAxis(compatibility,round,near,far);
    }
private:
    template<class Round> static bool finishAxis(bool compatibility,Round& round,Real& near,Real& far){
        const Real one{0x3f800000};
        if(compatibility){
            std::int32_t a,b;
            if(!round(near,a)||!round(far,b)||
               !near.assignInteger(a)||!far.assignInteger(b))return false;
        }
        if(Math::less(far,near)){const auto saved=near;near=far;far=saved;}
        // Subtract AFTER rounding and sorting, including zero-scale bounds.
        return !compatibility || Math::sub(far,one,far);
    }
    static bool small(std::int32_t value){return value>=-32768&&value<=32767;}
    static bool unitPixel(Real value,std::int32_t& out){
        if(!(value.bits&0x7fffffffu)){out=0;return true;}
        const unsigned exponent=(value.bits>>23)&255;
        if(exponent<127||exponent>147)return false;
        const unsigned shift=150-exponent,mantissa=(value.bits&0x7fffffu)|0x800000u;
        if(mantissa&((1u<<shift)-1))return false;
        const auto magnitude=std::int32_t(mantissa>>shift);
        out=value.bits&0x80000000u?-magnitude:magnitude;return true;
    }
    // The gated domain keeps every intermediate strictly inside +/-2^22,
    // so every add/subtract and multiplication by +1 is exact in binary32.
    static Real unitReal(std::int32_t value){
        if(!value)return {};
        const auto magnitude=value<0?0u-std::uint32_t(value):std::uint32_t(value);
        const unsigned top=31-MotionBinary64::Words::leadingZeros32(magnitude);
        return {(value<0?0x80000000u:0u)|((top+127u)<<23)|((magnitude<<(23-top))&0x7fffffu)};
    }
    template<class Round> static bool unitAxis(int low,int high,int origin,std::int32_t position,
        bool compatibility,Round& round,Real& near,Real& far){
        std::int32_t a=low-origin+position,b=a+(high-low)+1;
        near=unitReal(a);far=unitReal(b);
        if(compatibility){
            std::int32_t roundedA,roundedB;
            // Preserve callback order, failures and nonstandard policies.
            if(!round(near,roundedA)||!round(far,roundedB))return false;
            if(roundedA!=a||roundedB!=b){
                if(!near.assignInteger(roundedA)||
                   !far.assignInteger(roundedB))return false;
                sort(near,far);return Math::sub(far,Real{0x3f800000u},far);
            }
        }
        // The unchanged integer edges are already encoded above. Preserve
        // those encodings through sorting; only the compatibility far-edge
        // decrement needs a new conversion.
        if(a>b){const auto saved=a;a=b;b=saved;const auto edge=near;near=far;far=edge;}
        if(compatibility)far=unitReal(b-1);
        return true;
    }
    static void sort(Real& a,Real& b){if(Math::less(b,a)){const auto saved=a;a=b;b=saved;}}
    static bool local(Real low,Real high,int origin,Real scale,Real& near,Real& far){
        Real offset;
        // Original compares serialized margins before origin subtraction.
        sort(low,high);
        return offset.assignInteger(origin)&&
            Math::sub(low,offset,near)&&Math::sub(high,offset,far)&&
            add(far,Real{0x3f800000},far)&&mul(near,scale,near)&&mul(far,scale,far);
    }
    template<class Round> static bool roundedEdge(Real& value,bool upper,Round& round){
        std::int32_t pixel;
        if(!round(value,pixel)||(upper&&pixel==(-2147483647-1)))return false;
        // Rotated path decrements the INTEGER before converting to binary32.
        return value.assignInteger(upper?pixel-1:pixel);
    }
    static bool add(Real a,Real b,Real& out){return Math::sub(a,Math::negate(b),out);}
    static bool mul(Real a,Real b,Real& out){
        return multiplyBinary32(a,b,out);
    }
};
}
#endif
