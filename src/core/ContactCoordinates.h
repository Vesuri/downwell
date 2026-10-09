#ifndef DOWNWELL_CONTACT_COORDINATES_H
#define DOWNWELL_CONTACT_COORDINATES_H
#include "RunnerImageValue.h"
namespace downwell {
// World coordinates in [-65536,65536] on the Q8 lattice are exact binary32.
// In that domain subtracting narrowed arguments is exactly integer subtraction
// followed by ONE binary32 rounding. No origin cancellation is used outside it.
struct ContactCoordinates {
    // scrPlMovement publishes global.plx/ply before integrating movement.
    // Preserve the portable player's captured Q8 values as binary64 globals,
    // not the rounded builtin position or the post-movement drawing cache.
    static bool playerGlobals(std::int32_t x,std::int32_t y,int ox,int oy,
        MotionBinary64& px,MotionBinary64& py){
        auto ax=MotionBinary64::fromInteger(x),ay=MotionBinary64::fromInteger(y);
        if(x)ax.high-=8u<<20;if(y)ay.high-=8u<<20;
        if(!ax.add(MotionBinary64::fromInteger(ox))||!ay.add(MotionBinary64::fromInteger(oy)))return false;
        px=ax;py=ay;return true;
    }
    MotionBinary64 playerX{},playerY{};
    RunnerImageValue dx{},dy{};
    static RunnerImageValue q8(std::int32_t value){
        RunnerImageValue out;out.assignInteger(value);
        if(value)out.bits-=8u<<23;
        return out;
    }
    bool prepare(std::int32_t px,std::int32_t py,int cx,int cy,int ox,int oy){
        const std::int64_t wx=std::int64_t(px)+std::int64_t(ox)*256,
            wy=std::int64_t(py)+std::int64_t(oy)*256,
            callerX=(std::int64_t(cx)+ox)*256,callerY=(std::int64_t(cy)+oy)*256;
        constexpr std::int64_t bound=16777216;
        if(wx < -bound||wx > bound||wy < -bound||wy > bound||
           callerX < -bound||callerX > bound||callerY < -bound||callerY > bound)return false;
        playerX=q8(std::int32_t(wx)).promoted();playerY=q8(std::int32_t(wy)).promoted();
        dx=q8(std::int32_t(wx-callerX));dy=q8(std::int32_t(wy-callerY));
        return true;
    }
};
}
#endif
