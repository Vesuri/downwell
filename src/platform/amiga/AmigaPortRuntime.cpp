#include "AmigaPortRuntime.h"

#include "core/PortCheckpoint.h"
#include "core/Replay.h"

AmigaPortRuntime::AmigaPortRuntime(unsigned initialRingOrigin,
                                   bool startsGrounded,
                                   int landingOffset)
    : player_(startsGrounded, landingOffset), tick_(0),
      ringOrigin_(initialRingOrigin % ringHeight),
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
    const AmigaPlayerMotion motion = player_.step(input);
    if (player_.shots() != oldShots) {
        bulletActive_ = true;
        bulletX_ = player_.offsetX() - 1;
        bulletY_ = player_.offsetY() + 8;
    }

    const bool scrolled = (tick_ & 1u) == 0;
    if (scrolled)
        ringOrigin_ = (ringOrigin_ + 1u) % ringHeight;

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
        scrolled
    };
    return frame;
}
