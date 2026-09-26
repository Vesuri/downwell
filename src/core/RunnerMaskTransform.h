#ifndef DOWNWELL_CORE_RUNNER_MASK_TRANSFORM_H
#define DOWNWELL_CORE_RUNNER_MASK_TRANSFORM_H
#include "RunnerSpatialSplit.h"
#include "DivideBinary64.h"
#include "SpriteMaskPack.h"
namespace downwell {
// Scaled and rotated precise branches 0x4b9227..0x4b968f. The unit/near-zero
// fast path has different position rounding and must be dispatched separately.
class RunnerMaskTransform {
    using Math=RunnerSpatialSplit;
public:
    using Real=RunnerImageValue;
    bool prepare(Real x,Real y,Real scaleX,Real scaleY,Real inverseCosine,Real inverseSine,int originX,int originY){
        valid_=false;axisAligned_=false;
        const Real values[]={x,y,scaleX,scaleY,inverseCosine,inverseSine};
        for(auto value:values)if(!Math::valid(value))return false;
        Real rx,ry;MotionBinary64 quotient;
        const Real one{0x3f800000};
        if(Math::equal(scaleX,one))rx=one;
        else if(!divideBinary64(one.promoted(),scaleX.promoted(),quotient)||!rx.assign(quotient))return false;
        if(Math::equal(scaleY,one))ry=one;
        else if(!divideBinary64(one.promoted(),scaleY.promoted(),quotient)||!ry.assign(quotient))return false;
        if((Math::less(scaleX,Real{})&&!Math::sub(x,one,x))||
           (Math::less(scaleY,Real{})&&!Math::sub(y,one,y)))return false;
        if(!originX_.assign(MotionBinary64::fromInteger(originX))||!originY_.assign(MotionBinary64::fromInteger(originY)))return false;
        x_=x;y_=y;rx_=rx;ry_=ry;c_=inverseCosine;s_=inverseSine;valid_=true;return true;
    }
    // Dispatch here only when BOTH pair angles pass the runner's epsilon
    // test and its unit-scale fast path was not selected.
    bool prepareAxisAligned(Real x,Real y,Real scaleX,Real scaleY,int originX,int originY){
        if(!prepare(x,y,scaleX,scaleY,{0x3f800000},{},originX,originY))return false;
        axisAligned_=true;return true;
    }
    bool sample(int worldX,int worldY,Real& sampleX,Real& sampleY)const{
        if(!valid_)return false;
        Real dx,dy,xc,ys,yc,xs,sx,sy;
        if(!dx.assign(MotionBinary64::fromInteger(worldX))||!dy.assign(MotionBinary64::fromInteger(worldY))||
           !Math::sub(dx,x_,dx)||!Math::sub(dy,y_,dy))return false;
        if(axisAligned_){
            if(!mul(dx,rx_,sx)||!mul(dy,ry_,sy)||
               !Math::sub(sx,Math::negate(originX_),sx)||!Math::sub(sy,Math::negate(originY_),sy))return false;
            sampleX=sx;sampleY=sy;return true;
        }
        if(
           !mul(dx,c_,xc)||!mul(dy,s_,ys)||!mul(dy,c_,yc)||!mul(dx,s_,xs)||
           !Math::sub(ys,Math::negate(xc),sx)||!Math::sub(yc,xs,sy)||
           (!Math::equal(rx_,Real{0x3f800000})&&!mul(sx,rx_,sx))||
           (!Math::equal(ry_,Real{0x3f800000})&&!mul(sy,ry_,sy))||
           !Math::sub(sx,Math::negate(originX_),sx)||!Math::sub(sy,Math::negate(originY_),sy))return false;
        sampleX=sx;sampleY=sy;return true;
    }
    bool contains(const SpriteMaskPack::Mask& mask,int worldX,int worldY,bool& hit)const{
        if(!mask.bytes||!mask.width||!mask.height||mask.width>0x7fffffffu||mask.height>0x7fffffffu)return false;
        Real x,y,w,h;
        if(!sample(worldX,worldY,x,y)||!w.assign(MotionBinary64::fromInteger(mask.width))||
           !h.assign(MotionBinary64::fromInteger(mask.height)))return false;
        if(axisAligned_){
            // cvttss2si precedes the canvas test. In particular (-1,0)
            // truncates to zero rather than being rejected as off-canvas.
            int px,py;
            if(!truncateSigned(x,px)||!truncateSigned(y,py)){hit=false;return true;}
            hit=mask.contains(px,py);return true;
        }
        if(Math::less(x,Real{})||Math::less(y,Real{})||!Math::less(x,w)||!Math::less(y,h)){hit=false;return true;}
        hit=mask.contains(truncatePositive(x),truncatePositive(y));return true;
    }
private:
    static bool truncateSigned(Real value,int& out){
        if(int((value.bits>>23)&255)-127>=31)return false; // Source indefinite is outside every admitted canvas.
        const int magnitude=truncatePositive(Real{value.bits&0x7fffffffu});
        out=(value.bits&0x80000000u)?-magnitude:magnitude;return true;
    }
    static int truncatePositive(Real value){
        const int exponent=int((value.bits>>23)&255)-127;
        if(exponent<0)return 0;
        const auto mantissa=(value.bits&0x7fffffu)|0x800000u;
        return int(exponent<23?mantissa>>unsigned(23-exponent):mantissa<<unsigned(exponent-23));
    }
    static bool mul(Real a,Real b,Real& out){MotionBinary64 n;return multiplyBinary64(a.promoted(),b.promoted(),n)&&out.assign(n);}
    Real x_{},y_{},rx_{},ry_{},c_{},s_{},originX_{},originY_{};bool valid_=false,axisAligned_=false;
};
}
#endif
