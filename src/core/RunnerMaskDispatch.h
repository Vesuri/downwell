#ifndef DOWNWELL_CORE_RUNNER_MASK_DISPATCH_H
#define DOWNWELL_CORE_RUNNER_MASK_DISPATCH_H
#include "RunnerSpatialSplit.h"
namespace downwell {
// Pair-wide selection at 0x4b9000..0x4b909f. Defaults are the pinned
// executable's initial epsilon, not an assertion that it cannot be changed.
class RunnerMaskDispatch {
public:
    using Real=RunnerImageValue;
    enum class Path {unsupported,empty,roundedPosition,scaled,rotated};
    struct Shape {Real scaleX,scaleY,angle;};
    bool configure(MotionBinary64 epsilon){
        if(!epsilon.valid() || ((epsilon.high&0x80000000u)&&!epsilon.zero()))return false;
        if(epsilon.zero())epsilon.high=0;
        auto low=MotionBinary64::fromInteger(1),high=low,negative=epsilon;
        negative.high^=0x80000000u;
        Real lo,hi;
        if(!low.add(negative)||!high.add(epsilon)||!lo.assign(low)||!hi.assign(high))return false;
        epsilon_=epsilon;lower_=lo;upper_=hi;return true;
    }
    Path select(Shape a,Shape b)const{
        const Real values[]={a.scaleX,a.scaleY,a.angle,b.scaleX,b.scaleY,b.angle};
        for(auto value:values)if(!RunnerSpatialSplit::valid(value))return Path::unsupported;
        const Real scales[]={a.scaleX,a.scaleY,b.scaleX,b.scaleY};bool unit=true;
        for(auto value:scales){
            if(!(value.bits&0x7fffffffu))return Path::empty;
            if(RunnerSpatialSplit::less(value,lower_)||RunnerSpatialSplit::less(upper_,value))unit=false;
        }
        if(!nearZero(a.angle)||!nearZero(b.angle))return Path::rotated;
        return unit?Path::roundedPosition:Path::scaled;
    }
private:
    bool nearZero(Real angle)const{
        angle.bits&=0x7fffffffu;const auto v=angle.promoted();
        return v.high<epsilon_.high||(v.high==epsilon_.high&&v.low<=epsilon_.low);
    }
    MotionBinary64 epsilon_{0x3ee4f8b5u,0x88e368f1u};
    Real lower_{0x3f7fff58u},upper_{0x3f800054u};
};
}
#endif
