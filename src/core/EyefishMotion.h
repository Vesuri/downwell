#ifndef DOWNWELL_CORE_EYEFISH_MOTION_H
#define DOWNWELL_CORE_EYEFISH_MOTION_H
#include "CheckCollisionWithScript.h"
namespace downwell {
// enmEyefish 114 (hard-mode spawner sets): Create's xsp = choose(0.5, -0.5)
// and the unstopped part of its Step. scrCheckCollisionWith(parentWall) runs
// on its private xx/yy with ysp 0; a horizontal hit reverses xsp at movesp
// 0.5, then xx += xsp and x = round(xx). The |xsp| < 0.03 guard cannot fire:
// xsp only ever holds +-0.5. Query(x, y, target, hit) tests the eyefish's
// mask (sprEyefish 289) against parentWall at a binary64 position.
struct EyefishMotion {
    using Real = MotionBinary64;
    Real xx{}, yy{}, xsp{};
    void create(int x, int y, bool left) {
        xx = Real::fromInteger(x); yy = Real::fromInteger(y);
        xsp = {left ? 0xbfe00000u : 0x3fe00000u, 0};
    }
    bool left() const { return (xsp.high >> 31) != 0; }
    // y is never reassigned: only the script's private yy can move.
    template<class Query> bool step(Query query, std::int32_t& x) {
        CheckCollisionWithScript check;
        if (!check.run(57, xx, yy, xsp, Real{}, query)) return false;
        if (check.xcollision) xsp = {check.xcollision > 0 ? 0xbfe00000u : 0x3fe00000u, 0};
        return xx.add(xsp) && xx.roundPixel(x);
    }
};
}
#endif
