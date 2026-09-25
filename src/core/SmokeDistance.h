#ifndef DOWNWELL_CORE_SMOKE_DISTANCE_H
#define DOWNWELL_CORE_SMOKE_DISTANCE_H
#include "MultiplyBinary64.h"
#include "RunnerImageValue.h"
namespace downwell {
// fxSmoke's point_distance(xstart,ystart,x,y) > 16, not a generic sqrt.
// Inputs are instance builtin binary32 coordinates, promoted to VM reals.
// Runner 0x53d5b0 rounds differences, both squares, their sum and sqrt to float.
// With real-comparison epsilon 1e-5, the first accepted sqrt float is
// 16+6*2^-19. Its lower rounding midpoint squared is strictly between the
// squared-distance floats 0x4380000b and 0x4380000c. Thus no sqrt is needed.
inline bool smokeEscapedDistance(MotionBinary64 sx,MotionBinary64 sy,
                                 MotionBinary64 x,MotionBinary64 y,bool& escaped) {
    // Validate the callback contract instead of silently narrowing arbitrary
    // VM reals differently from the runner's mixed getter/subtract sequence.
    const MotionBinary64 coordinates[]={sx,sy,x,y};
    for(auto value:coordinates) {
        RunnerImageValue narrowed;
        if(!narrowed.assign(value))return false;
        const auto promoted=narrowed.promoted();
        if(promoted.high!=value.high || promoted.low!=value.low)return false;
    }
    sx.high^=0x80000000u;sy.high^=0x80000000u;
    RunnerImageValue dx,dy,xx,yy,sum;
    MotionBinary64 squareX,squareY;
    if(!x.add(sx) || !y.add(sy) || !dx.assign(x) || !dy.assign(y) ||
       !multiplyBinary64(dx.promoted(),dx.promoted(),squareX) ||
       !multiplyBinary64(dy.promoted(),dy.promoted(),squareY) ||
       !xx.assign(squareX) || !yy.assign(squareY)) return false;
    auto total=xx.promoted();
    if(!total.add(yy.promoted()) || !sum.assign(total))return false;
    escaped=sum.bits>=0x4380000cu;
    return true;
}
}
#endif
