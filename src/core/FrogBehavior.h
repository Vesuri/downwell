#ifndef DOWNWELL_CORE_FROG_BEHAVIOR_H
#define DOWNWELL_CORE_FROG_BEHAVIOR_H

namespace downwell {

// enmFrog Alarm 1: grounded && scrInView(0,0,0), then global.plx > x.
// Coordinates are Q8; frog x/y must be the rounded instance coordinates,
// playerX is the pre-movement global.plx and view edges remain fractional.
// Return zero to retry alarm 1 in 60 ticks; +/-1 starts preparation and
// sets xsp to this sign, nsp to twice it, and alarm 0 to 60.
inline int frogPreparationDirection(bool grounded, long frogX, long frogY,
    long playerX, long viewLeft, long viewTop, long viewRight, long viewBottom) {
    if (!grounded || frogX <= viewLeft || frogX >= viewRight ||
        frogY <= viewTop || frogY >= viewBottom) return 0;
    return playerX > frogX ? 1 : -1;
}

} // namespace downwell
#endif
