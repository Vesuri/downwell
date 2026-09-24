#ifndef DOWNWELL_CORE_FROG_BEHAVIOR_H
#define DOWNWELL_CORE_FROG_BEHAVIOR_H

namespace downwell {

// Source alarm decisions, independent of either runtime's storage format.
// A negative preparation value means leave it unchanged (failed Alarm 1).
struct FrogAlarmEffect {
    int preparation = -1;
    bool setX = false, setY = false, setJumpSpeed = false;
    long xQ16 = 0, yFifthsQ8 = 0;
    int jumpSpeed = 0;
    unsigned alarm = 1;
    int ticks = 60;
    int sound = -1;
};

inline FrogAlarmEffect frogPreparationAlarm(int direction) {
    FrogAlarmEffect result;
    if (direction != 0) {
        result.preparation = 1;
        result.setX = result.setJumpSpeed = true;
        result.xQ16 = direction * 65536L;
        result.jumpSpeed = direction * 2;
        result.alarm = 0;
        result.sound = 113;
    }
    return result;
}

inline FrogAlarmEffect frogJumpAlarm(bool grounded, int jumpSpeed) {
    FrogAlarmEffect result;
    result.preparation = 0;
    if (grounded) {
        result.setX = result.setY = true;
        result.xQ16 = jumpSpeed * 65536L;
        result.yFifthsQ8 = -4 * 1280L; // enmFrog Create: jumpsp=4
        result.sound = 116;
    }
    // The source alarm does not clear grounded; Step recomputes support.
    return result;
}

inline bool frogLandingEdge(bool grounded, bool& landed) {
    const bool justLanded = grounded && !landed;
    landed = grounded;
    return justLanded;
}

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
