#ifndef DOWNWELL_CORE_RUNNER_MASK_POINT_H
#define DOWNWELL_CORE_RUNNER_MASK_POINT_H
#include "RunnerSpatialSplit.h"
#include "DivideBinary64.h"
#include "SpriteMaskPack.h"
namespace downwell {
// Ordinary point-mask geometry, Windows 0x4ba8a8..0x4baa35. Caller supplies
// x87-rounded positions and the already-selected mask frame. Not pair geometry:
// half-pixel offset, direct division, strict angle threshold and floor sampling.
struct RunnerMaskPoint {
    using Real=RunnerImageValue;
    using Math=RunnerSpatialSplit;
    struct Shape {
        std::int32_t x=0,y=0,originX=0,originY=0;
        Real scaleX{0x3f800000},scaleY{0x3f800000},angle{};
    };
    // inverseTrig receives degrees and returns narrowed source cosine/sine
    // for -angle (float -angle*pi/180, then the runner's double trig routines).
    template<class Trig>
    static bool sample(const Shape& shape,std::int32_t pointX,std::int32_t pointY,
                       Trig inverseTrig,Real& outX,Real& outY){
        if(!Math::valid(shape.scaleX)||!Math::valid(shape.scaleY)||!Math::valid(shape.angle))return false;
        Real x,y,px,py,ox,oy,dx,dy;
        if(!integer(shape.x,x)||!integer(shape.y,y)||!integer(pointX,px)||!integer(pointY,py)||
           !integer(shape.originX,ox)||!integer(shape.originY,oy)||
           !Math::sub(x,Real{0x3f000000},x)||!Math::sub(y,Real{0x3f000000},y)||
           !Math::sub(px,x,dx)||!Math::sub(py,y,dy))return false;
        // 0x801618 is float 0.0001, not the pair kernel's epsilon.
        if(!Math::less(Real{shape.angle.bits&0x7fffffffu},Real{0x38d1b717})){
            Real c,s,ys,xc,yc,xs;
            if(!inverseTrig(shape.angle,c,s)||!Math::valid(c)||!Math::valid(s)||
               !multiply(dy,s,ys)||!multiply(dx,c,xc)||
               !multiply(dy,c,yc)||!multiply(dx,s,xs)||
               !Math::sub(ys,Math::negate(xc),dx)||!Math::sub(yc,xs,dy))return false;
        }
        if(!divide(dx,shape.scaleX,x)||!divide(dy,shape.scaleY,y)||
           !Math::sub(x,Math::negate(ox),x)||!Math::sub(y,Math::negate(oy),y))return false;
        outX=x;outY=y;return true;
    }
    template<class Trig>
    static bool contains(const Shape& shape,const SpriteMaskPack::Mask& mask,
                         std::int32_t pointX,std::int32_t pointY,Trig trig,bool& hit){
        if(!mask.bytes||!mask.width||!mask.height||mask.width>0x7fffffffu||
           mask.height>0x7fffffffu||mask.stride<(mask.width+7)/8)return false;
        Real x,y;
        if(!sample(shape,pointX,pointY,trig,x,y))return false;
        // floor followed by cvttss2si: negative and indefinite results always
        // fail the canvas test. Positive truncation is exactly floor here.
        int ix,iy;
        hit=pixel(x,ix)&&pixel(y,iy)&&mask.contains(ix,iy);return true;
    }
private:
    static bool integer(std::int32_t n,Real& out){return out.assign(MotionBinary64::fromInteger(n));}
    static bool multiply(Real a,Real b,Real& out){MotionBinary64 n;return multiplyBinary64(a.promoted(),b.promoted(),n)&&out.assign(n);}
    static bool divide(Real a,Real b,Real& out){MotionBinary64 n;return divideBinary64(a.promoted(),b.promoted(),n)&&out.assign(n);}
    static bool pixel(Real n,int& out){
        const auto magnitude=n.bits&0x7fffffffu;
        if((n.bits&0x80000000u)&&magnitude)return false;
        const int exponent=int((magnitude>>23)&255)-127;
        if(exponent>=31)return false;
        if(exponent<0){out=0;return true;}
        const auto mantissa=(magnitude&0x7fffffu)|0x800000u;
        out=int(exponent<23?mantissa>>unsigned(23-exponent):mantissa<<unsigned(exponent-23));return true;
    }
};
}
#endif
