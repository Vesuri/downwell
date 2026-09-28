#ifndef DOWNWELL_CORE_RUNNER_MOVE_SNAP_H
#define DOWNWELL_CORE_RUNNER_MOVE_SNAP_H
#include "RunnerImageValue.h"
namespace downwell {
struct RunnerMoveSnap {
    static bool coordinate16(RunnerImageValue coordinate,RunnerImageValue& out){
        if(!coordinate.promoted().valid())return false;
        const auto exponent=(coordinate.bits>>23)&255u;
        if((coordinate.bits&0x7fffffffu)&&exponent<=4)return false;
        if(coordinate.bits&0x7fffffffu)coordinate.bits-=4u<<23;
        std::int32_t grid;
        if(!coordinate.promoted().roundPixel(grid)||grid>134217727||grid< -134217728)return false;
        return out.assign(MotionBinary64::fromInteger(grid*16));
    }
};
}
#endif
