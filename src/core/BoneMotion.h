#ifndef DOWNWELL_CORE_BONE_MOTION_H
#define DOWNWELL_CORE_BONE_MOTION_H
#include "MotionBinary64.h"
namespace downwell {
// Post-throw motion for object 269. SkeletonThrow has already run the full
// child Create and overridden its random velocities. Initialization consumes
// no RNG; animation, health and collision event ownership remain separate.
struct BoneMotion {
    MotionBinary64 xx, yy, xsp, ysp;
    std::int32_t x = 0, y = 0;
    static bool horizontal(int tenths, MotionBinary64& result) {
        const bool negative = tenths < 0;
        // Validate before negation (including INT_MIN).
        switch (tenths) {
        case 0: result = {}; return true;
        case -8: case 8: result = {0x3fe99999u,0x9999999au}; break;
        case -10: case 10: result = {0x3ff00000u,0}; break;
        case -12: case 12: result = {0x3ff33333u,0x33333333u}; break;
        default: return false;
        }
        if (negative) result.high |= 0x80000000u;
        return true;
    }
    bool launch(std::int32_t px, std::int32_t py, int vxTenths, int vyTenths) {
        MotionBinary64 vx, vy;
        if (!horizontal(vxTenths, vx)) return false;
        switch (vyTenths) {
        case -30: vy = {0xc0080000u,0}; break;
        case -31: vy = {0xc008ccccu,0xcccccccdu}; break;
        case -32: vy = {0xc0099999u,0x9999999au}; break;
        default: return false;
        }
        x = px; y = py; xx = MotionBinary64::fromInteger(px); yy = MotionBinary64::fromInteger(py);
        xsp = vx; ysp = vy;
        return true;
    }
    bool step(bool stopped) {
        if (stopped) return true;
        // Commit together so unsupported arithmetic cannot leave half a step.
        auto nextYsp = ysp, nextX = xx, nextY = yy;
        std::int32_t px, py;
        if (!nextYsp.add({0x3fb99999u,0x9999999au}) ||
            !nextX.add(xsp) || !nextY.add(nextYsp) ||
            !nextX.roundPixel(px) || !nextY.roundPixel(py)) return false;
        ysp = nextYsp; xx = nextX; yy = nextY; x = px; y = py;
        return true;
    }
};
}
#endif
