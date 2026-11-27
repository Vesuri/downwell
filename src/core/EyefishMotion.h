#ifndef DOWNWELL_CORE_EYEFISH_MOTION_H
#define DOWNWELL_CORE_EYEFISH_MOTION_H
#include "CheckCollisionUnits.h"
namespace downwell {
// enmEyefish 114 (hard-mode spawner sets): Create's xsp = choose(0.5, -0.5)
// and the unstopped part of its Step. scrCheckCollisionWith(parentWall) runs
// on its private xx/yy with ysp 0; a horizontal hit reverses xsp at movesp
// 0.5, then xx += xsp and x = round(xx). The |xsp| < 0.03 guard cannot fire:
// xsp only ever holds +-0.5. In Q16 (W3.20): every position is a whole or
// half pixel, which Q16 and binary64 both hold exactly, so this is the VM's
// motion. Query(x, y, hit) tests the eyefish's mask (sprEyefish 289) against
// parentWall at a Q16 position.
struct EyefishMotion {
    std::int32_t xx = 0, yy = 0, xsp = 0; // Q16
    void create(int x, int y, bool left) {
        xx = x * 65536; yy = y * 65536;
        xsp = left ? -32768 : 32768;
    }
    bool left() const { return xsp < 0; }
    // y is never reassigned: only the script's private yy can move.
    template<class Query> bool step(Query query, std::int32_t& x) {
        CheckCollisionUnits<65536, 65536> check;
        if (xx < -0x40000000 || xx > 0x40000000 || !check.run(xx, yy, xsp, 0, query)) return false;
        if (check.xcollision) xsp = check.xcollision > 0 ? -32768 : 32768;
        xx += xsp;
        x = CheckCollisionUnits<65536, 65536>::roundPixel<65536>(xx);
        return true;
    }
};
}
#endif
