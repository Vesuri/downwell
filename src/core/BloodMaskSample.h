#ifndef DOWNWELL_CORE_BLOOD_MASK_SAMPLE_H
#define DOWNWELL_CORE_BLOOD_MASK_SAMPLE_H
#include "BloodEffect.h"
#include "RunnerMaskTransform.h"
namespace downwell {
// Prepared unit-scale inverse transform for sprite 84's stored 4x3 mask.
// The precise collision path samples it even though its mode is AxisAlignedRect.
struct BloodMaskSample {
    RunnerMaskTransform transform;
    bool prepare(const BloodEffect& effect){
        RunnerImageValue radians,cosine,negativeSine;MotionBinary64 c,s;BloodMaskSample next;
        if(!SmokeDirection::radians(effect.imageAngle,radians) ||
           !smokeTrig(radians.promoted(),c,s) || !cosine.assign(c) || !negativeSine.assign(s))return false;
        // sin(-a)=-sin(a), cos(-a)=cos(a); rounding symmetry retains signed zero.
        negativeSine.bits^=0x80000000u;
        if(!next.transform.prepare(effect.x,effect.y,{0x3f800000},{0x3f800000},cosine,negativeSine,12,12))return false;
        *this=next;return true;
    }
    bool contains(int worldX,int worldY,bool& hit)const{
        RunnerImageValue sx,sy;
        if(!transform.sample(worldX,worldY,sx,sy))return false;
        // Nonnegative mask coordinates truncate toward zero. The source mask
        // has exactly columns 10..13 and rows 11..13, all other pixels zero.
        hit=sx.bits>=0x41200000u && sx.bits<0x41600000u &&
            sy.bits>=0x41300000u && sy.bits<0x41600000u;
        return true;
    }
};
}
#endif
