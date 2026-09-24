#ifndef DOWNWELL_CORE_FROG_BEHAVIOR_H
#define DOWNWELL_CORE_FROG_BEHAVIOR_H

namespace downwell {

inline bool frogInsideView(long x, long y, long left, long top, long right, long bottom) {
    return x > left && x < right && y > top && y < bottom;
}

// enmFrog Step: scrInView(0,0,32) gates integration, not gravity/collision.
inline bool frogMotionVisible(long x, long y, long left, long top, long right, long bottom) {
    return frogInsideView(x, y, left, top, right, bottom + 32 * 256L);
}

// enmFrog Alarm 1: grounded && scrInView(0,0,0), then global.plx > x.
// Coordinates are Q8; frog x/y must be the rounded instance coordinates,
// playerX is the pre-movement global.plx and view edges remain fractional.
// Return zero to retry alarm 1 in 60 ticks; +/-1 starts preparation and
// sets xsp to this sign, nsp to twice it, and alarm 0 to 60.
inline int frogPreparationDirection(bool grounded, long frogX, long frogY,
    long playerX, long viewLeft, long viewTop, long viewRight, long viewBottom) {
    if (!grounded || !frogInsideView(frogX, frogY, viewLeft, viewTop, viewRight, viewBottom)) return 0;
    return playerX > frogX ? 1 : -1;
}

} // namespace downwell
#endif
