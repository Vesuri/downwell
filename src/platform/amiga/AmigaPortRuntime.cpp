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
      breakableCount_(0), blocksDestroyed_(0), checkpoint_(0)
{
    unsigned index;
    for (index = 0; index < bulletCapacity; ++index) {
        bullets_[index].x = 0;
        bullets_[index].y = 0;
        bullets_[index].active = false;
    }
}

unsigned AmigaPortRuntime::bulletCount() const
{
    unsigned count = 0;
    unsigned index;
    for (index = 0; index < bulletCapacity; ++index)
        if (bullets_[index].active)
            ++count;
    return count;
}

const AmigaPortRuntime::BulletState&
AmigaPortRuntime::bulletAt(unsigned index) const
{
    return bullets_[index < bulletCapacity ? index : 0];
}

int AmigaPortRuntime::bulletX() const
{
    unsigned index;
    for (index = 0; index < bulletCapacity; ++index)
        if (bullets_[index].active)
            return bullets_[index].x;
    return 0;
}

int AmigaPortRuntime::bulletY() const
{
    unsigned index;
    for (index = 0; index < bulletCapacity; ++index)
        if (bullets_[index].active)
            return bullets_[index].y;
    return 0;
}

void AmigaPortRuntime::configureBreakables(const int* x, const int* y,
                                            unsigned count)
{
    unsigned index;
    if (x == 0 || y == 0)
        return;
    if (count > breakableCapacity)
        count = breakableCapacity;
    breakableCount_ = count;
    blocksDestroyed_ = 0;
    for (index = 0; index < count; ++index) {
        breakables_[index].x = x[index];
        breakables_[index].y = y[index];
        breakables_[index].active = true;
    }
}

bool AmigaPortRuntime::breakableActiveAt(int x, int y) const
{
    unsigned index;
    for (index = 0; index < breakableCount_; ++index)
        if (breakables_[index].x == x && breakables_[index].y == y)
            return breakables_[index].active;
    return false;
}

AmigaPortFrame AmigaPortRuntime::step(std::uint8_t packedInput)
{
    const int oldAmmo = player_.ammo();
    const unsigned oldShots = player_.shots();
    bool bulletChanged = false;
    unsigned destroyedBlockCount = 0;
    int destroyedBlockX[bulletCapacity] = {0};
    int destroyedBlockY[bulletCapacity] = {0};

    ++tick_;
    {
        unsigned bulletIndex;
        for (bulletIndex = 0; bulletIndex < bulletCapacity; ++bulletIndex) {
            BulletState& bullet = bullets_[bulletIndex];
            if (!bullet.active)
                continue;
            const int priorBulletY = bullet.y;
            unsigned blockIndex;
            bullet.y += 8;
            bulletChanged = true;
            for (blockIndex = 0;
                 blockIndex < breakableCount_ && bullet.active;
                 ++blockIndex) {
                Breakable& block = breakables_[blockIndex];
                if (block.active && bullet.x >= block.x - 8 &&
                    bullet.x <= block.x + 7 &&
                    priorBulletY <= block.y + 7 &&
                    bullet.y >= block.y - 8) {
                    block.active = false;
                    bullet.active = false;
                    if (destroyedBlockCount < bulletCapacity) {
                        destroyedBlockX[destroyedBlockCount] = block.x;
                        destroyedBlockY[destroyedBlockCount] = block.y;
                        ++destroyedBlockCount;
                    }
                    ++blocksDestroyed_;
                }
            }
            if (bullet.y >= bulletBottom)
                bullet.active = false;
        }
    }

    const downwell::Input input = downwell::unpackReplayInput(packedInput);
    AmigaPlayerMotion motion = player_.step(input);
    if (player_.shots() != oldShots) {
        unsigned index;
        for (index = 0; index < bulletCapacity; ++index) {
            BulletState& bullet = bullets_[index];
            if (!bullet.active) {
                bullet.active = true;
                bullet.x = player_.offsetX() - 1;
                bullet.y = player_.offsetY() + 8;
                bulletChanged = true;
                break;
            }
        }
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

    const bool firstBulletActive = bulletActive();
    checkpoint_ = downwell::portCheckpointDigest(
        tick_, packedInput, player_.offsetX(), player_.offsetY(),
        (std::uint32_t)player_.ammo(), player_.grounded(), player_.jumps(),
        player_.shots(), ringOrigin_, firstBulletActive,
        bulletX(), bulletY());
    trajectory_.addU64(checkpoint_);

    AmigaPortFrame frame = {};
    frame.playerMotion = motion;
    frame.ammoChanged = player_.ammo() != oldAmmo;
    frame.bulletChanged = bulletChanged;
    frame.scrolled = scrollDelta != 0;
    frame.scrollDelta = scrollDelta;
    frame.destroyedBlockCount = destroyedBlockCount;
    for (unsigned index = 0; index < destroyedBlockCount; ++index) {
        frame.blockX[index] = destroyedBlockX[index];
        frame.blockY[index] = destroyedBlockY[index];
    }
    return frame;
}
