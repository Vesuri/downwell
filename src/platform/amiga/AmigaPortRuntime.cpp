#include "AmigaPortRuntime.h"

#include "core/PortCheckpoint.h"
#include "core/Replay.h"

AmigaPortRuntime::AmigaPortRuntime(unsigned initialRingOrigin,
                                   bool startsGrounded,
                                   int landingOffset,
                                   bool followCamera)
    : player_(startsGrounded, landingOffset), tick_(0),
      ringOrigin_(initialRingOrigin % ringHeight),
      followCamera_(followCamera), cameraY_(0), cameraPixelY_(0),
      bulletActive_(false), bulletX_(0), bulletY_(0), checkpoint_(0)
{
}

AmigaPortFrame AmigaPortRuntime::step(std::uint8_t packedInput)
{
    const bool oldBulletActive = bulletActive_;
    const int oldBulletX = bulletX_;
    const int oldBulletY = bulletY_;
    const int oldAmmo = player_.ammo();
    const unsigned oldShots = player_.shots();

    ++tick_;
    if (bulletActive_) {
        bulletY_ += 6;
        if (bulletY_ >= bulletBottom)
            bulletActive_ = false;
    }

    const downwell::Input input = downwell::unpackReplayInput(packedInput);
    AmigaPlayerMotion motion = player_.step(input);
    if (player_.shots() != oldShots) {
        bulletActive_ = true;
        bulletX_ = player_.offsetX() - 1;
        bulletY_ = player_.offsetY() + 8;
    }

    int scrollDelta = 0;
    if (followCamera_) {
        const int oldCameraPixelY = cameraPixelY_;
        const long target = player_.offsetYFixed() + 24L * 256L;
        cameraY_ += (target - cameraY_) / 5L;
        cameraPixelY_ = cameraY_ >= 0
                            ? (int)(cameraY_ >> 8)
                            : -(int)((-cameraY_) >> 8);
        scrollDelta = cameraPixelY_ - oldCameraPixelY;
        motion.deltaY -= scrollDelta;
    } else if ((tick_ & 1u) == 0) {
        scrollDelta = 1;
    }
    if (scrollDelta != 0) {
        int origin = (int)ringOrigin_ + scrollDelta;
        origin %= (int)ringHeight;
        if (origin < 0) origin += (int)ringHeight;
        ringOrigin_ = (unsigned)origin;
    }

    checkpoint_ = downwell::portCheckpointDigest(
        tick_, packedInput, player_.offsetX(), player_.offsetY(),
        (std::uint32_t)player_.ammo(), player_.grounded(), player_.jumps(),
        player_.shots(), ringOrigin_, bulletActive_, bulletX_, bulletY_);
    trajectory_.addU64(checkpoint_);

    AmigaPortFrame frame = {
        motion,
        player_.ammo() != oldAmmo,
        bulletActive_ != oldBulletActive || bulletX_ != oldBulletX ||
            bulletY_ != oldBulletY,
        scrollDelta != 0,
        scrollDelta
    };
    return frame;
}
