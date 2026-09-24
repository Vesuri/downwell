#ifndef DOWNWELL_CORE_LEVEL_CLEAR_MOTION_H
#define DOWNWELL_CORE_LEVEL_CLEAR_MOTION_H
#include "HudReal.h"
namespace downwell {
// Result Create and scrResult's final player/camera easing. Positions remain
// positive in this path; the horizontally signed carousel is separate.
struct LevelClearMotion {
    HudReal cameraY = HudReal::integer(142), cameraTarget = cameraY;
    HudReal playerY, playerTarget = HudReal::integer(160), acceleration;
    unsigned playerDivisor = 20;
    void showChoices() {
        cameraTarget = HudReal::integer(340); // levely + 70
        playerTarget = HudReal::integer(302); // levely + 32
        playerDivisor = 10;
    }
    static HudReal approach(HudReal value, HudReal target, unsigned divisor) {
        return target.less(value) ? value.subtract(value.subtract(target).divideSmall(divisor)) :
            value.add(target.subtract(value).divideSmall(divisor));
    }
    void step(bool departureBranch) {
        if (departureBranch) {
            playerTarget = playerTarget.add(acceleration);
            acceleration = acceleration.add({0x1999999999999aULL, -3}); // binary64 0.2
        }
        playerY = approach(playerY, playerTarget, playerDivisor);
        cameraY = approach(cameraY, cameraTarget, 10);
    }
    int viewY() const { return static_cast<int>(cameraY.add({0x10000000000000ULL, -1}).floor()) - 142; }
};
}
#endif
