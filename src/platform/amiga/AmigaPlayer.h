#ifndef DOWNWELL_AMIGA_PLAYER_H
#define DOWNWELL_AMIGA_PLAYER_H

#include "core/Input.h"

struct AmigaPlayerMotion {
    int deltaX;
    int deltaY;
};

class AmigaPlayer {
public:
    explicit AmigaPlayer(bool startsGrounded = true,
                         int landingOffset = 0);
    AmigaPlayerMotion step(const downwell::Input& input);

    int offsetX() const { return pixelX_; }
    int offsetY() const { return pixelY_; }
    long offsetYFixed() const { return y_; }
    int ammo() const { return ammo_; }
    bool grounded() const { return grounded_; }
    unsigned jumps() const { return jumps_; }
    unsigned shots() const { return shots_; }

private:
    static int fixedToPixel(long value);

    long x_;
    long y_;
    long speedX_;
    long speedY_;
    long landingY_;
    int pixelX_;
    int pixelY_;
    int ammo_;
    bool grounded_;
    bool priorFire_;
    bool jumpShootLocked_;
    unsigned shotDelay_;
    unsigned jumps_;
    unsigned shots_;
};

#endif
