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
      enemyCount_(0), enemiesDefeated_(0), health_(4), invulnerability_(0),
      solidResolver_(0),
      solidResolverContext_(0),
      breakableCount_(0), blocksDestroyed_(0), checkpoint_(0)
{
    unsigned index;
    for (index = 0; index < bulletCapacity; ++index) {
        bullets_[index].x = 0;
        bullets_[index].y = 0;
        bullets_[index].active = false;
    }
    for (index = 0; index < enemyCapacity; ++index) {
        enemies_[index].objectId = -1;
        enemies_[index].spriteId = -1;
        enemies_[index].x = 0;
        enemies_[index].y = 0;
        enemies_[index].health = 0;
        enemies_[index].reward = 0;
        enemies_[index].facing = 1;
        enemies_[index].preciseX = 0;
        enemies_[index].preciseY = 0;
        enemies_[index].speedX = 0;
        enemies_[index].speedY = 0;
        enemies_[index].active = false;
        enemies_[index].awake = false;
    }
}

bool AmigaPortRuntime::configureEnemy(unsigned index, int objectId,
                                      int spriteId, int x, int y,
                                      int speedX, int speedY,
                                      int health, int reward)
{
    if (index >= enemyCapacity)
        return false;
    EnemyState& enemy = enemies_[index];
    enemy.objectId = objectId;
    enemy.spriteId = spriteId;
    enemy.x = x;
    enemy.y = y;
    enemy.health = health;
    enemy.reward = reward;
    enemy.facing = speedX < 0 ? -1 : 1;
    enemy.preciseX = (long)x * 256L;
    enemy.preciseY = (long)y * 256L;
    enemy.speedX = speedX;
    enemy.speedY = speedY;
    enemy.active = true;
    enemy.awake = false;
    if (enemyCount_ <= index)
        enemyCount_ = index + 1;
    return true;
}

const AmigaPortRuntime::EnemyState&
AmigaPortRuntime::enemyAt(unsigned index) const
{
    return enemies_[index < enemyCount_ ? index : 0];
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
    const int oldHealth = health_;
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
            for (unsigned enemyIndex = 0;
                 enemyIndex < enemyCount_ && bullet.active; ++enemyIndex) {
                EnemyState& enemy = enemies_[enemyIndex];
                if (!enemy.active || !enemy.awake || enemy.health <= 0 ||
                    bullet.x < enemy.x - 8 || bullet.x > enemy.x + 7 ||
                    priorBulletY > enemy.y + 7 || bullet.y < enemy.y - 8)
                    continue;
                if (enemy.objectId != 112) {
                    enemy.health -= 10;
                    if (enemy.health <= 0) {
                        enemy.active = false;
                        ++enemiesDefeated_;
                    }
                }
                bullet.active = false;
            }
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

    for (unsigned enemyIndex = 0; enemyIndex < enemyCount_; ++enemyIndex) {
        EnemyState& enemy = enemies_[enemyIndex];
        if (!enemy.active)
            continue;
        if (!enemy.awake && enemy.y - cameraPixelY_ <= 254)
            enemy.awake = true;
        if (!enemy.awake || solidResolver_ == 0)
            continue;
        if (enemy.objectId == 111) {
            const int direction = enemy.speedX < 0 ? -1 : 1;
            const bool grounded = solidResolver_(
                solidResolverContext_, enemy.x, enemy.y + 6);
            if (grounded) {
                enemy.speedY = 0;
                if (!solidResolver_(solidResolverContext_,
                                    enemy.x + direction * 7,
                                    enemy.y + 6))
                    enemy.speedX = -enemy.speedX;
            } else {
                enemy.speedY += 51;
                if (enemy.speedY > 7 * 256)
                    enemy.speedY = 7 * 256;
            }
            const int nextX = (int)((enemy.preciseX + enemy.speedX) / 256L);
            const int edge = nextX + (enemy.speedX < 0 ? -6 : 6);
            if (solidResolver_(solidResolverContext_, edge, enemy.y))
                enemy.speedX = -enemy.speedX;
            else
                enemy.preciseX += enemy.speedX;
            if (!grounded) {
                const int nextY =
                    (int)((enemy.preciseY + enemy.speedY) / 256L);
                if (enemy.speedY > 0 && solidResolver_(
                        solidResolverContext_, enemy.x, nextY + 5)) {
                    while (!solidResolver_(solidResolverContext_, enemy.x,
                                           enemy.y + 6)) {
                        ++enemy.y;
                        enemy.preciseY = (long)enemy.y * 256L;
                    }
                    enemy.speedY = 0;
                } else {
                    enemy.preciseY += enemy.speedY;
                }
            }
        } else if (enemy.objectId == 117) {
            const int vertical = enemy.speedY < 0 ? -1 : 1;
            if (solidResolver_(solidResolverContext_, enemy.x,
                               enemy.y + vertical * 8) ||
                !solidResolver_(solidResolverContext_, enemy.x + 16,
                                enemy.y + vertical * 8))
                enemy.speedY = -enemy.speedY;
            enemy.preciseY += enemy.speedY;
        }
        enemy.x = (int)(enemy.preciseX / 256L);
        enemy.y = (int)(enemy.preciseY / 256L);
        if (enemy.speedX != 0)
            enemy.facing = enemy.speedX < 0 ? -1 : 1;
    }

    if (invulnerability_ > 0)
        --invulnerability_;
    for (unsigned enemyIndex = 0; enemyIndex < enemyCount_; ++enemyIndex) {
        EnemyState& enemy = enemies_[enemyIndex];
        if (!enemy.active || !enemy.awake || enemy.health <= 0)
            continue;
        const int dx = player_.offsetX() - enemy.x;
        const int dy = player_.offsetY() - enemy.y;
        if (dx < -10 || dx > 10 || dy < -11 || dy > 11)
            continue;
        if (enemy.objectId == 111 && enemy.y > player_.offsetY() + 2) {
            enemy.health = 0;
            enemy.active = false;
            ++enemiesDefeated_;
            player_.stompBounce();
            continue;
        }
        if (invulnerability_ == 0 && health_ > 0) {
            --health_;
            invulnerability_ = 90;
            player_.contactKnockback(enemy.x);
        }
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
    frame.healthChanged = health_ != oldHealth;
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
