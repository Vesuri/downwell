#ifndef DOWNWELL_CORE_BLOOD_MASK_SAMPLE_H
#define DOWNWELL_CORE_BLOOD_MASK_SAMPLE_H
#include "BloodEffect.h"
#include "RunnerMaskTransform.h"
#include "RunnerMaskDispatch.h"
namespace downwell {
// Prepared unit-scale inverse transform for sprite 84's stored 4x3 mask.
// The precise collision path samples it even though its mode is AxisAlignedRect.
struct BloodMaskSample {
    RunnerMaskTransform transform;
    RunnerMaskDispatch::Path path=RunnerMaskDispatch::Path::unsupported;
    std::int32_t roundedX=0,roundedY=0;
    bool prepare(const BloodEffect& effect,RunnerMaskDispatch::Path selected=RunnerMaskDispatch::Path::rotated){
        if(!RunnerSpatialSplit::valid(effect.x)||!RunnerSpatialSplit::valid(effect.y)||
           !RunnerSpatialSplit::valid(effect.imageAngle))return false;
        RunnerImageValue radians,cosine,negativeSine;MotionBinary64 c,s;BloodMaskSample next;
        next.path=selected;
        if(selected==RunnerMaskDispatch::Path::roundedPosition){
            if(!effect.x.promoted().roundPixel(next.roundedX)||!effect.y.promoted().roundPixel(next.roundedY))return false;
            *this=next;return true;
        }
        if(selected==RunnerMaskDispatch::Path::scaled){
            if(!next.transform.prepareAxisAligned(effect.x,effect.y,{0x3f800000},{0x3f800000},12,12))return false;
            *this=next;return true;
        }
        if(selected!=RunnerMaskDispatch::Path::rotated)return false;
        // 0x4b93f6 negates the stored angle BEFORE float pi/division and
        // trigonometry. Do not assume symmetry of the runner's polynomial.
        auto inverseAngle=effect.imageAngle;
        inverseAngle.bits^=0x80000000u;
        if(!SmokeDirection::radians(inverseAngle,radians) ||
           !smokeTrig(radians.promoted(),c,s) || !cosine.assign(c) || !negativeSine.assign(s))return false;
        if(!next.transform.prepare(effect.x,effect.y,{0x3f800000},{0x3f800000},cosine,negativeSine,12,12))return false;
        *this=next;return true;
    }
    bool contains(int worldX,int worldY,bool& hit)const{
        if(path==RunnerMaskDispatch::Path::roundedPosition){
            const auto sx=12u-std::uint32_t(roundedX)+std::uint32_t(worldX);
            const auto sy=12u-std::uint32_t(roundedY)+std::uint32_t(worldY);
            hit=sx>=10&&sx<14&&sy>=11&&sy<14;return true;
        }
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
