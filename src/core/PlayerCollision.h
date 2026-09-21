#ifndef DOWNWELL_CORE_PLAYER_COLLISION_H
#define DOWNWELL_CORE_PLAYER_COLLISION_H

#include <cmath>

namespace downwell {

// sprPlayerIdle's recovered mask spans x -4..+3 and y -4..+7.
// sprThinPlatform's mask spans x -8..+7 and only y -8..-7. GameMaker rests
// the player one pixel above that mask and detects support at yy + 1.
inline bool resolveDownwardThinPlatform(float playerX,
                                        float currentPlayerY,
                                        float nextPlayerY,
                                        float platformX,
                                        float platformY,
                                        float& resolvedPlayerY) {
    if (nextPlayerY <= currentPlayerY)
        return false;
    const float playerLeft = playerX - 4.0F;
    const float playerRight = playerX + 3.0F;
    const float platformLeft = platformX - 8.0F;
    const float platformRight = platformX + 7.0F;
    if (playerRight < platformLeft || playerLeft > platformRight)
        return false;

    const float platformTop = platformY - 8.0F;
    const float currentBottom = currentPlayerY + 7.0F;
    // scrPlayerPlatformCollision probes round(yy) plus ceil(abs(ysp)), so even
    // subpixel downward speed keeps an already-standing player supported.
    const float probedBottom = currentBottom +
        std::ceil(nextPlayerY - currentPlayerY);
    if (currentBottom >= platformTop || probedBottom < platformTop)
        return false;
    resolvedPlayerY = platformTop - 8.0F;
    return true;
}

} // namespace downwell

#endif
