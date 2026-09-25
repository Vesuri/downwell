#ifndef DOWNWELL_CORE_MAIN_CAMERA_MOTION_H
#define DOWNWELL_CORE_MAIN_CAMERA_MOTION_H
#include "GameMakerRounding.h"
namespace downwell {
// camMain's default 480-pixel rmMain shaft/side-room rules. Q12 easing
// accommodates the serialized 300000-pixel room height in signed 32 bits;
// freeCam, focus, shake, autoscroll and ending are separate work.
// Side-locker primitives require collision-phase dispatch by the world owner.
struct MainCameraMotion {
    long x = 240L * 4096, y = 176L * 4096;
    long targetX = x, targetY = y;
    int slowdownX = 1, slowdownY = 5;
    int roomEnd = -1;
    bool sideLocked = false;
    void setLevelExit(int triggerY) { roomEnd = triggerY - 176; }
    // rmMain's serialized view follows camMain's rounded instance x/y.
    int pixelX() const { return static_cast<int>(roundGameMakerQ12(x)); }
    int pixelY() const { return static_cast<int>(roundGameMakerQ12(y)); }
    long viewLeftQ12() const { return (pixelX() - 80L) * 4096; }
    long viewRightQ12() const { return (pixelX() + 80L) * 4096; }
    long viewTopQ12() const { return (pixelY() - 142L) * 4096; }
    long viewBottomQ12() const { return (pixelY() + 142L) * 4096; }
    void reset(int px, int py) {
        x = targetX = px * 4096L; y = targetY = py * 4096L;
        slowdownX = 1; slowdownY = 5;
        roomEnd = -1;
        sideLocked = false;
    }
    // camMain Collision sideCamLocker. The outer unequal-target guard matters:
    // equal targets do not latch; later collisions change the target but do
    // not snap again while locked. No Step clamps run inside Collision.
    void collideSideLocker(int lockerY) {
        if (targetY == lockerY * 4096L) return;
        targetY = lockerY * 4096L;
        if (!sideLocked) {
            y = targetY;
            sideLocked = true;
        }
    }
    void step(long playerXQ8, long playerYQ8, bool dead = false,
              bool touchingSideLocker = false) {
        if (!dead) {
            if (playerXQ8 > 160L * 256 && playerXQ8 < 320L * 256) {
                targetX = 240L * 4096; // Both source horizontal clamps coincide.
                targetY = (playerYQ8 + 24L * 256) * 16;
                const bool cameraInShaft = x > 160L * 4096 && x < 320L * 4096;
                slowdownX = cameraInShaft ? 10 : 1;
                slowdownY = cameraInShaft ? 5 : 1;
            } else {
                slowdownX = 1;
                if (playerXQ8 < 160L * 256) targetX = 80L * 4096;
                else if (playerXQ8 > 320L * 256) targetX = 400L * 4096;
                // Exact boundary equality preserves the old target.
                // Side rooms preserve targetY and slowdownY.
            }
        }
        // Step blocks 56-62 run outside the !death branch and before the
        // room-end clamp. place_meeting uses the prior published instance.
        if (sideLocked && !touchingSideLocker) {
            y = targetY = (playerYQ8 + 24L * 256) * 16;
            x = targetX = 240L * 4096; // Both 480-wide shaft clamps coincide.
            slowdownX = slowdownY = 1;
            sideLocked = false;
        }
        // camMain clamps its target and (only in that branch) its current yy
        // before easing. The source's negative sentinel is not a true bool.
        if (roomEnd > 0 && targetY > roomEnd * 4096L) {
            targetY = roomEnd * 4096L;
            if (y > targetY) y = targetY;
        }
        x += (targetX - x) / slowdownX;
        y += (targetY - y) / slowdownY;
        if (y < 142L * 4096) y = 142L * 4096;
    }
};
} // namespace downwell
#endif
