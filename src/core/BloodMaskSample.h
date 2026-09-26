#ifndef DOWNWELL_CORE_BLOOD_MASK_SAMPLE_H
#define DOWNWELL_CORE_BLOOD_MASK_SAMPLE_H
#include "BloodEffect.h"
namespace downwell {
// Prepared unit-scale inverse transform for sprite 84's stored 4x3 mask.
// The precise collision path samples it even though its mode is AxisAlignedRect.
struct BloodMaskSample {
    RunnerImageValue x{},y{},cosine{},negativeSine{};
    bool prepare(const BloodEffect& effect){
        RunnerImageValue radians;MotionBinary64 c,s;BloodMaskSample next;
        if(!SmokeDirection::radians(effect.imageAngle,radians) ||
           !smokeTrig(radians.promoted(),c,s) || !next.cosine.assign(c) || !next.negativeSine.assign(s))return false;
        // sin(-a)=-sin(a), cos(-a)=cos(a); rounding symmetry retains signed zero.
        next.negativeSine.bits^=0x80000000u;next.x=effect.x;next.y=effect.y;
        *this=next;return true;
    }
    bool contains(int worldX,int worldY,bool& hit)const{
        RunnerImageValue dx,dy,xc,ys,yc,xs,sx,sy;
        RunnerImageValue px,py;
        RunnerImageValue negX=x,negY=y;negX.bits^=0x80000000u;negY.bits^=0x80000000u;
        if(!px.assign(MotionBinary64::fromInteger(worldX)) || !py.assign(MotionBinary64::fromInteger(worldY)) ||
           !sum(px.promoted(),negX.promoted(),dx) ||
           !sum(py.promoted(),negY.promoted(),dy) ||
           !product(dx,cosine,xc) || !product(dy,negativeSine,ys) ||
           !product(dy,cosine,yc) || !product(dx,negativeSine,xs))return false;
        xs.bits^=0x80000000u;
        if(!sum(ys.promoted(),xc.promoted(),sx) || !sum(yc.promoted(),xs.promoted(),sy) ||
           !sum(sx.promoted(),MotionBinary64::fromInteger(12),sx) ||
           !sum(sy.promoted(),MotionBinary64::fromInteger(12),sy))return false;
        // Nonnegative mask coordinates truncate toward zero. The source mask
        // has exactly columns 10..13 and rows 11..13, all other pixels zero.
        hit=sx.bits>=0x41200000u && sx.bits<0x41600000u &&
            sy.bits>=0x41300000u && sy.bits<0x41600000u;
        return true;
    }
private:
    static bool sum(MotionBinary64 a,MotionBinary64 b,RunnerImageValue& out){return a.add(b)&&out.assign(a);}
    static bool product(RunnerImageValue a,RunnerImageValue b,RunnerImageValue& out){
        MotionBinary64 value;return multiplyBinary64(a.promoted(),b.promoted(),value)&&out.assign(value);
    }
};
}
#endif
