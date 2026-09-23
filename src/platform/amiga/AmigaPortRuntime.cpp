#include "AmigaPortRuntime.h"
#include "AmigaProfile.h"

#include "core/PortCheckpoint.h"
#include "core/Replay.h"

namespace {

const short headingX[36] = {
     256,  252,  241,  222,  196,  165,  128,   88,   44,
       0,  -44,  -88, -128, -165, -196, -222, -241, -252,
    -256, -252, -241, -222, -196, -165, -128,  -88,  -44,
       0,   44,   88,  128,  165,  196,  222,  241,  252
};

const short headingY[36] = {
       0,   44,   88,  128,  165,  196,  222,  241,  252,
     256,  252,  241,  222,  196,  165,  128,   88,   44,
       0,  -44,  -88, -128, -165, -196, -222, -241, -252,
    -256, -252, -241, -222, -196, -165, -128,  -88,  -44
};

int positiveDirection(int direction)
{
    if (direction >= 0 && direction < 360) return direction;
    if (direction >= -360 && direction < 0) return direction + 360;
    if (direction >= 360 && direction < 720) return direction - 360;
    direction %= 360;
    return direction < 0 ? direction + 360 : direction;
}

struct HeadingIndices { unsigned char values[360] = {}; };
constexpr HeadingIndices makeHeadingIndices() {
    HeadingIndices result;
    for (unsigned angle = 0; angle < 360; ++angle)
        result.values[angle] = ((angle + 5) / 10) % 36;
    return result;
}
constexpr auto headingIndices = makeHeadingIndices();

int absoluteInt(int value)
{
    return value < 0 ? -value : value;
}

unsigned enemySpriteFrameCount(int spriteId)
{
    switch (spriteId) {
        case 153: return 4; /* sprBat */
        case 163: return 4; /* sprSnake */
        case 164: return 1; /* sprSnakeAir */
        case 166: return 4; /* sprFrogIdle */
        case 168: return 2; /* sprFrogAir */
        case 170: return 4; /* sprFrogPrepare */
        case 177: return 4; /* sprEnmSnail */
        case 186: return 4; /* sprGroundedTurtle */
        case 187: return 1; /* sprGroundedTurtleFall */
        case 188: return 6; /* sprGroundedTurtleWalk */
        case 278: return 2; /* sprCrawler */
        default: return 1;
    }
}

} // namespace

AmigaPortRuntime::AmigaPortRuntime(unsigned initialRingOrigin,
                                   bool startsGrounded,
                                   int landingOffset,
                                   bool followCamera)
    : player_(startsGrounded, landingOffset), tick_(0),
      ringOrigin_(initialRingOrigin % ringHeight),
      followCamera_(followCamera), cameraY_(0), cameraPixelY_(0),
      cameraPixelX_(0),
      enemyCount_(0), enemiesDefeated_(0), enemyRandom_(2),
      health_(4), invulnerability_(0),
      dead_(false), deathFrames_(0),
      currency_(0), gemStreak_(0), gemStreakTimer_(0),
      solidResolver_(0),
      solidResolverContext_(0),
      breakableCount_(0), blocksDestroyed_(0), checkpoint_(0)
{
    unsigned index;
    if (followCamera_) {
        /*
         * camMain receives one Step on rmMain's creation frame, before the
         * player receives movement. Its recovered +24 target and /5 easing
         * therefore establish the first visible camera/ring position here.
         */
        cameraY_ = (24L * 256L) / 5L;
        cameraPixelY_ = (int)(cameraY_ >> 8);
        ringOrigin_ = (ringOrigin_ + (unsigned)cameraPixelY_) % ringHeight;
    }
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
        enemies_[index].motionState = 0;
        enemies_[index].auxiliaryState = 0;
        enemies_[index].alarm0 = -1;
        enemies_[index].alarm1 = -1;
        enemies_[index].direction = 0;
        enemies_[index].moveSpeed = 0;
        enemies_[index].imagePhase = 0;
        enemies_[index].imageSpeed = 0;
        enemies_[index].imageFrame = 0;
        enemies_[index].active = false;
        enemies_[index].awake = false;
        enemies_[index].grounded = false;
        enemies_[index].activationPending = false;
    }
    for (index = 0; index < gemCapacity; ++index) {
        gems_[index].x = 0;
        gems_[index].y = 0;
        gems_[index].amount = 0;
        gems_[index].age = 0;
        gems_[index].preciseX = 0;
        gems_[index].preciseY = 0;
        gems_[index].speedX = 0;
        gems_[index].speedY = 0;
        gems_[index].large = false;
        gems_[index].attracted = false;
        gems_[index].active = false;
    }
}

unsigned AmigaPortRuntime::integerSquareRoot(unsigned value)
{
    unsigned result = 0;
    unsigned bit = 1u << 30;
    while (bit > value)
        bit >>= 2;
    while (bit != 0) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    return result;
}

void AmigaPortRuntime::spawnReward(const EnemyState& enemy, bool attracted)
{
    int remaining = enemy.reward;
    for (unsigned index = 0; index < gemCapacity && remaining > 0; ++index) {
        GemState& gem = gems_[index];
        if (gem.active)
            continue;
        gem.large = remaining >= 10;
        gem.amount = gem.large ? 10 : 2;
        gem.age = 0;
        gem.x = enemy.x;
        gem.y = enemy.y;
        gem.preciseX = (long)enemy.x * 256L;
        gem.preciseY = (long)enemy.y * 256L;
        /* fixed8(random(4) - 2), preserving one runner draw per component */
        gem.speedX = (long)(enemyRandom_.nextU32() >> 22) - 512L;
        gem.speedY = (long)(enemyRandom_.nextU32() >> 22) - 512L;
        enemyRandom_.irandomSmall(3);  /* recovered imgSp choice */
        enemyRandom_.irandomSmall(70); /* first gemdir candidate */
        enemyRandom_.irandomSmall(70); /* second gemdir candidate */
        enemyRandom_.irandomSmall(1);  /* candidate choice */
        gem.attracted = attracted;
        gem.active = true;
        remaining -= gem.amount;
    }
}

unsigned AmigaPortRuntime::gemCount() const
{
    unsigned count = 0;
    for (unsigned index = 0; index < gemCapacity; ++index)
        if (gems_[index].active)
            ++count;
    return count;
}

const AmigaPortRuntime::GemState&
AmigaPortRuntime::gemAt(unsigned index) const
{
    return gems_[index < gemCapacity ? index : 0];
}

bool AmigaPortRuntime::configureEnemy(unsigned index, int objectId,
                                      int spriteId, int x, int y,
                                      int speedX, int speedY,
                                      int health, int reward,
                                      int motionState, int auxiliaryState,
                                      int alarm0, int alarm1,
                                      int direction, int moveSpeed,
                                      bool active)
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
    enemy.motionState = motionState;
    enemy.auxiliaryState = auxiliaryState;
    enemy.alarm0 = alarm0;
    enemy.alarm1 = alarm1;
    enemy.direction = direction;
    enemy.moveSpeed = moveSpeed;
    enemy.imagePhase = 0;
    enemy.imageSpeed = objectId == 111 ? 64u
                       : objectId == 112 ? 38u
                       : objectId == 116 ? 51u
                       : objectId == 117 || objectId == 119 ? 26u
                       : 0u;
    enemy.imageFrame = 0;
    enemy.active = active;
    enemy.awake = false;
    enemy.grounded = false;
    enemy.activationPending = !active;
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
    if (x == 0 || y == 0)
        return;
    if (count > breakableCapacity)
        count = breakableCapacity;
    breakableCount_ = 0;
    breakableIndexReady_ = true;
    for (unsigned cell = 0; cell < sizeof(breakableIndex_); ++cell) breakableIndex_[cell] = 0;
    blocksDestroyed_ = 0;
    appendBreakables(x, y, count);
}

bool AmigaPortRuntime::appendBreakables(const int* x, const int* y, unsigned count)
{
    if (count > breakableCapacity - breakableCount_ || (count && (!x || !y)))
        return false;
    for (unsigned source = 0; source < count; ++source) {
        const unsigned index = breakableCount_++;
        breakables_[index].x = x[source];
        breakables_[index].y = y[source];
        breakables_[index].active = true;
        if ((x[source] & 15) || (y[source] & 15) || x[source] < -512 ||
            x[source] >= 512 || y[source] < 0 || y[source] >= 8192) {
            breakableIndexReady_ = false;
        } else if (breakableIndexReady_) {
            auto& cell = breakableIndex_[(y[source] >> 4) * 64 + ((x[source] + 512) >> 4)];
            if (cell) breakableIndexReady_ = false;
            cell = index + 1;
        }
    }
    return true;
}

bool AmigaPortRuntime::breakableActiveAt(int x, int y) const
{
    if (breakableIndexReady_) {
        const int index = indexedBreakable(x, y);
        return index >= 0 && breakables_[index].x == x && breakables_[index].y == y &&
               breakables_[index].active;
    }
    unsigned index;
    for (index = 0; index < breakableCount_; ++index)
        if (breakables_[index].x == x && breakables_[index].y == y)
            return breakables_[index].active;
    return false;
}

int AmigaPortRuntime::indexedBreakable(int x, int y) const {
    x += 512 + 8;
    y += 8;
    if (x < 0 || x >= 1024 || y < 0 || y >= 8192) return -1;
    return (int)breakableIndex_[(y >> 4) * 64 + (x >> 4)] - 1;
}

AmigaPortFrame AmigaPortRuntime::step(std::uint8_t packedInput)
{
    DW_PROFILE_PHASE(profilePlayer);
    const int oldAmmo = player_.ammo();
    const int oldHealth = health_;
    const unsigned oldCurrency = currency_;
    const unsigned oldGemStreak = gemStreak_;
    const unsigned oldShots = player_.shots();
    bool bulletChanged = false;
    unsigned destroyedBlockCount = 0;
    int destroyedBlockX[bulletCapacity] = {0};
    int destroyedBlockY[bulletCapacity] = {0};

    ++tick_;
    if (dead_)
        ++deathFrames_;
    downwell::Input input = downwell::unpackReplayInput(packedInput);
    if (dead_)
        input = downwell::Input{};
    AmigaPlayerMotion motion = player_.step(input);
    if (player_.shots() != oldShots) {
        unsigned index;
        for (index = 0; index < bulletCapacity; ++index) {
            BulletState& bullet = bullets_[index];
            if (!bullet.active) {
                bullet.active = true;
                bullet.x = player_.offsetX() - 1;
                bullet.y = player_.offsetY() + 4;
                bulletChanged = true;
                break;
            }
        }
    }

    int scrollDelta = 0;
    int cameraXDelta = 0;
    if (followCamera_ && !dead_) {
        const int oldCameraPixelX = cameraPixelX_;
        const int oldCameraPixelY = cameraPixelY_;
        const int playerWorldX = 240 + player_.offsetX();
        cameraPixelX_ = playerWorldX < 160 ? -160
                        : playerWorldX > 320 ? 160 : 0;
        cameraXDelta = cameraPixelX_ - oldCameraPixelX;
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

    /*
     * objBullet_n receives its Step later in the same GameMaker phase in
     * which objPlayer_n creates it. Existing and newly-created shots therefore
     * advance together after player movement and firing.
     */
    {
        unsigned bulletIndex;
        DW_PROFILE_PHASE(profileBullets);
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
                        spawnReward(enemy, false);
                        enemy.active = false;
                        ++enemiesDefeated_;
                    }
                }
                bullet.active = false;
            }
            unsigned firstBlock = 0, endBlock = breakableCount_;
            if (breakableIndexReady_) {
                int candidate = indexedBreakable(bullet.x, priorBulletY);
                int next = indexedBreakable(bullet.x, bullet.y);
                if (candidate >= 0 && !breakables_[candidate].active) candidate = -1;
                if (next >= 0 && !breakables_[next].active) next = -1;
                if (candidate < 0 || (next >= 0 && next < candidate)) candidate = next;
                firstBlock = candidate < 0 ? 0 : (unsigned)candidate;
                endBlock = candidate < 0 ? 0 : firstBlock + 1;
            }
            for (blockIndex = firstBlock;
                 blockIndex < endBlock && bullet.active;
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
            if (bullet.y >= cameraPixelY_ + bulletBottom)
                bullet.active = false;
        }
    }

    DW_PROFILE_PHASE(profileEnemyAlarms);
    for (unsigned enemyIndex = 0; enemyIndex < enemyCount_; ++enemyIndex) {
        EnemyState& enemy = enemies_[enemyIndex];
        if (!enemy.active)
            continue;
        bool alarm0Fired = false, alarm1Fired = false;
        enemy.imagePhase += enemy.imageSpeed;
        if (enemy.alarm0 > 0 && --enemy.alarm0 == 0)
            alarm0Fired = true;
        if (enemy.alarm1 > 0 && --enemy.alarm1 == 0)
            alarm1Fired = true;

        if (enemy.objectId == 112 && alarm1Fired) {
            enemy.auxiliaryState = -enemy.auxiliaryState;
            enemy.alarm1 = 120 + (int)enemyRandom_.irandomSmall(180);
        } else if (enemy.objectId == 113) {
            if (alarm1Fired) {
                if (enemy.grounded) {
                    enemy.auxiliaryState = 1;
                    enemy.speedX =
                        player_.offsetX() < enemy.x ? -256 : 256;
                    enemy.motionState = enemy.speedX < 0 ? -2 : 2;
                    enemy.alarm0 = 60;
                } else {
                    enemy.alarm1 = 60;
                }
            }
            if (alarm0Fired) {
                if (enemy.grounded) {
                    enemy.speedY = -4 * 256;
                    enemy.speedX = enemy.motionState * 256;
                    enemy.grounded = false;
                }
                enemy.auxiliaryState = 0;
                enemy.alarm1 = 60;
            }
        } else if (enemy.objectId == 119 && alarm0Fired) {
            if (enemy.grounded) {
                const int crawl = enemy.motionState > 0 ? 1 : -1;
                int nextX = enemy.x;
                int nextY = enemy.y;
                if (solidResolver_ != 0 && solidResolver_(
                        solidResolverContext_, enemy.x + crawl, enemy.y)) {
                    enemy.speedX = 0;
                    enemy.speedY = -crawl * 256;
                    nextY -= crawl;
                } else {
                    enemy.speedX = crawl * 256;
                    enemy.speedY = 0;
                    nextX += crawl;
                }
                if (solidResolver_ == 0 || !solidResolver_(
                        solidResolverContext_, nextX, nextY)) {
                    enemy.preciseX = (long)nextX * 256L;
                    enemy.preciseY = (long)nextY * 256L;
                }
            }
            enemy.alarm0 = 2;
        }
    }

    DW_PROFILE_PHASE(profileEnemyMotion);
    for (unsigned enemyIndex = 0; enemyIndex < enemyCount_; ++enemyIndex) {
        EnemyState& enemy = enemies_[enemyIndex];
        if (!enemy.active)
            continue;
        if (!enemy.awake && enemy.y - cameraPixelY_ <= 254)
            enemy.awake = true;
        if (!enemy.awake || solidResolver_ == 0)
            continue;
        if (enemy.objectId == 111 || enemy.objectId == 112) {
            const int halfWidth = enemy.objectId == 112 ? 8 : 6;
            const int foot = enemy.objectId == 112 ? 8 : 5;
            const int direction = enemy.speedX < 0 ? -1 : 1;
            const bool grounded = solidResolver_(
                solidResolverContext_, enemy.x, enemy.y + foot + 1);
            enemy.grounded = grounded;
            if (grounded) {
                enemy.speedY = 0;
                if (!solidResolver_(solidResolverContext_,
                                    enemy.x + direction * (halfWidth + 1),
                                    enemy.y + foot + 1))
                    enemy.speedX = -enemy.speedX;
            } else {
                enemy.speedY += 51;
                if (enemy.speedY > 7 * 256)
                    enemy.speedY = 7 * 256;
            }
            const int nextX = (int)((enemy.preciseX + enemy.speedX) / 256L);
            const int edge = nextX +
                (enemy.speedX < 0 ? -halfWidth : halfWidth);
            if (solidResolver_(solidResolverContext_, edge, enemy.y))
                enemy.speedX = -enemy.speedX;
            else
                enemy.preciseX += enemy.speedX;
            if (!grounded) {
                const int nextY =
                    (int)((enemy.preciseY + enemy.speedY) / 256L);
                if (enemy.speedY > 0 && solidResolver_(
                        solidResolverContext_, enemy.x, nextY + foot)) {
                    while (!solidResolver_(solidResolverContext_, enemy.x,
                                           enemy.y + foot + 1)) {
                        ++enemy.y;
                        enemy.preciseY = (long)enemy.y * 256L;
                    }
                    enemy.speedY = 0;
                } else {
                    enemy.preciseY += enemy.speedY;
                }
            }
        } else if (enemy.objectId == 116) {
            const int dx = player_.offsetX() - enemy.x;
            const int dy = enemy.y - player_.offsetY();
            const int distance = absoluteInt(dx) + absoluteInt(dy);
            const int headingIndex = headingIndices.values[positiveDirection(enemy.direction)];
            const int currentX = headingX[headingIndex];
            const int currentY = headingY[headingIndex];
            const int jitter =
                (int)enemyRandom_.randomIntegerSmall(40) - 20;
            long cross = (long)currentX * dy - (long)currentY * dx;
            cross += (long)jitter * distance * 4L;
            const long dot = (long)currentX * dx + (long)currentY * dy;
            if (dot > 0 && (cross < 0 ? -cross : cross) <
                               (long)distance * 88L) {
                enemy.moveSpeed += 13;
                if (enemy.moveSpeed > 320)
                    enemy.moveSpeed = 320;
            } else {
                enemy.moveSpeed -= 6;
                if (enemy.moveSpeed < 256)
                    enemy.moveSpeed = 256;
            }
            enemy.direction = positiveDirection(
                enemy.direction + (cross > 0 ? 10 : -10));
            const int nextHeading = headingIndices.values[enemy.direction];
            enemy.speedX =
                (long)headingX[nextHeading] * enemy.moveSpeed / 256L;
            enemy.speedY =
                -(long)headingY[nextHeading] * enemy.moveSpeed / 256L;
            const long nextPreciseX = enemy.preciseX + enemy.speedX;
            const long nextPreciseY = enemy.preciseY + enemy.speedY;
            if (!solidResolver_(solidResolverContext_,
                                (int)(nextPreciseX / 256L), enemy.y))
                enemy.preciseX = nextPreciseX;
            if (!solidResolver_(solidResolverContext_,
                                (int)(enemy.preciseX / 256L),
                                (int)(nextPreciseY / 256L)))
                enemy.preciseY = nextPreciseY;
        } else if (enemy.objectId == 113) {
            enum { halfWidth = 7, foot = 8 };
            if (!enemy.grounded) {
                enemy.speedY += 38;
                if (enemy.speedY > 7 * 256)
                    enemy.speedY = 7 * 256;
                const int nextX =
                    (int)((enemy.preciseX + enemy.speedX) / 256L);
                const int edge = nextX +
                    (enemy.speedX < 0 ? -halfWidth : halfWidth);
                if (solidResolver_(solidResolverContext_, edge, enemy.y))
                    enemy.speedX = -(enemy.speedX * 77L) / 256L;
                else
                    enemy.preciseX += enemy.speedX;
                const int nextY =
                    (int)((enemy.preciseY + enemy.speedY) / 256L);
                if (enemy.speedY > 0 && solidResolver_(
                        solidResolverContext_, enemy.x, nextY + foot)) {
                    while (!solidResolver_(solidResolverContext_, enemy.x,
                                           enemy.y + foot + 1)) {
                        ++enemy.y;
                        enemy.preciseY = (long)enemy.y * 256L;
                    }
                    enemy.speedY = 0;
                    enemy.grounded = true;
                } else {
                    enemy.preciseY += enemy.speedY;
                }
            } else {
                enemy.speedY = 0;
            }
        } else if (enemy.objectId == 117) {
            if (enemy.motionState == 0) {
                if (solidResolver_(solidResolverContext_,
                                   enemy.x + 8, enemy.y))
                    enemy.motionState = 1;
                else if (solidResolver_(solidResolverContext_,
                                        enemy.x - 8, enemy.y))
                    enemy.motionState = -1;
                else
                    enemy.active = false;
                enemy.facing = enemy.motionState;
            }
            if (!enemy.active)
                continue;
            const int vertical = enemy.speedY < 0 ? -1 : 1;
            if (solidResolver_(solidResolverContext_, enemy.x,
                               enemy.y + vertical * 8) ||
                !solidResolver_(solidResolverContext_,
                                enemy.x + enemy.motionState * 16,
                                enemy.y + vertical * 8))
                enemy.speedY = -enemy.speedY;
            enemy.preciseY += enemy.speedY;
        } else if (enemy.objectId == 119) {
            enum { foot = 6 };
            if (!enemy.grounded) {
                enemy.speedY += 51;
                if (enemy.speedY > 7 * 256)
                    enemy.speedY = 7 * 256;
                const int nextY =
                    (int)((enemy.preciseY + enemy.speedY) / 256L);
                if (solidResolver_(solidResolverContext_,
                                   enemy.x, nextY + foot)) {
                    while (!solidResolver_(solidResolverContext_, enemy.x,
                                           enemy.y + foot + 1)) {
                        ++enemy.y;
                        enemy.preciseY = (long)enemy.y * 256L;
                    }
                    enemy.speedY = 0;
                    enemy.grounded = true;
                } else {
                    enemy.preciseY += enemy.speedY;
                }
            }
        }
        enemy.x = (int)(enemy.preciseX / 256L);
        enemy.y = (int)(enemy.preciseY / 256L);
        if (enemy.speedX != 0)
            enemy.facing = enemy.speedX < 0 ? -1 : 1;
        if (enemy.objectId == 111)
            enemy.spriteId = enemy.grounded ? 163 : 164;
        if (enemy.objectId == 112)
            enemy.spriteId = !enemy.grounded ? 187
                             : enemy.auxiliaryState != 0 ? 188 : 186;
        if (enemy.objectId == 113)
            enemy.spriteId = !enemy.grounded ? 168
                             : enemy.auxiliaryState != 0 ? 170 : 166;
        if (enemy.objectId == 116)
            enemy.spriteId = 153;
        if (enemy.objectId == 117)
            enemy.spriteId = 177;
        if (enemy.objectId == 119)
            enemy.spriteId = 278;
        if (enemy.objectId == 113) {
            if (enemy.spriteId == 168) {
                enemy.imageSpeed = 0;
                enemy.imagePhase = enemy.speedY < 0 ? 0u : 256u;
            } else {
                enemy.imageSpeed = 51u;
            }
        }
        const unsigned frameCount = enemySpriteFrameCount(enemy.spriteId);
        const unsigned cycle = frameCount * 256u;
        while (enemy.imagePhase >= cycle)
            enemy.imagePhase -= cycle;
        enemy.imageFrame = enemy.imagePhase >> 8;
    }

    DW_PROFILE_PHASE(profileEnemyContact);
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
        const bool stompable = enemy.objectId == 111 ||
                               enemy.objectId == 112 ||
                               enemy.objectId == 113 ||
                               enemy.objectId == 116;
        if (stompable && enemy.y > player_.offsetY() + 2) {
            enemy.health = 0;
            spawnReward(enemy, true);
            enemy.active = false;
            ++enemiesDefeated_;
            player_.stompBounce();
            continue;
        }
        if (invulnerability_ == 0 && health_ > 0) {
            --health_;
            invulnerability_ = 90;
            player_.contactKnockback(enemy.x, enemy.y);
            if (health_ == 0) {
                dead_ = true;
                deathFrames_ = 0;
            }
        }
    }

    if (gemStreakTimer_ > 0)
        --gemStreakTimer_;
    else
        gemStreak_ = 0;

    DW_PROFILE_PHASE(profileGems);
    for (unsigned gemIndex = 0; gemIndex < gemCapacity; ++gemIndex) {
        GemState& gem = gems_[gemIndex];
        if (!gem.active)
            continue;
        ++gem.age;
        const int dx = player_.offsetX() - gem.x;
        const int dy = player_.offsetY() - gem.y;
        if (!gem.attracted && gem.age >= 15 &&
            absoluteInt(dx) < 24 && absoluteInt(dy) < 24)
            gem.attracted = true;
        if (gem.attracted) {
            const unsigned distance = integerSquareRoot(
                (unsigned)(dx * dx + dy * dy));
            /*
             * The recovered uint16 age subtraction saturates the early
             * immediately-attracted stomp path at the nine-pixel cap.
             */
            long speed = gem.age < 15
                             ? 9L * 256L
                             : 4L * 256L + ((long)gem.age - 15L) * 32L;
            if (speed > 9L * 256L)
                speed = 9L * 256L;
            if (distance != 0) {
                gem.speedX = (long)dx * speed / (long)distance;
                gem.speedY = (long)dy * speed / (long)distance;
            }
            gem.preciseX += gem.speedX;
            gem.preciseY += gem.speedY;
        } else {
            gem.speedY += 9; /* recovered gravity 0.035 in fixed8 */
            if (gem.speedY > 4L * 256L)
                gem.speedY = 4L * 256L;
            const long nextX = gem.preciseX + gem.speedX;
            if (solidResolver_ != 0 && solidResolver_(
                    solidResolverContext_, (int)(nextX / 256L), gem.y))
                gem.speedX = -(gem.speedX * 179L) / 256L;
            else
                gem.preciseX = nextX;
            const long nextY = gem.preciseY + gem.speedY;
            if (gem.speedY > 0 && solidResolver_ != 0 && solidResolver_(
                    solidResolverContext_,
                    (int)(gem.preciseX / 256L),
                    (int)(nextY / 256L) + 4)) {
                const long bounce =
                    (long)enemyRandom_.randomIntegerSmall(180);
                gem.speedY = -128L - bounce;
                gem.speedX = (gem.speedX * 179L) / 256L;
            } else {
                gem.preciseY = nextY;
            }
            if (absoluteInt((int)gem.speedX) < 13)
                gem.speedX = gem.speedX < 0 ? -13 : 13;
        }
        gem.x = (int)(gem.preciseX / 256L);
        gem.y = (int)(gem.preciseY / 256L);
        const int collectX = player_.offsetX() - gem.x;
        const int collectY = player_.offsetY() - gem.y;
        if (gem.age >= 10 && collectX >= -6 && collectX <= 6 &&
            collectY >= -8 && collectY <= 8) {
            currency_ += (unsigned)gem.amount;
            gemStreak_ += (unsigned)gem.amount;
            gemStreakTimer_ = 300;
            gem.active = false;
        } else if (gem.age >= 310) {
            gem.active = false;
        }
    }

    for (unsigned enemyIndex = 0; enemyIndex < enemyCount_; ++enemyIndex) {
        EnemyState& enemy = enemies_[enemyIndex];
        if (!enemy.active && enemy.activationPending &&
            enemy.y - cameraPixelY_ <= 254) {
            enemy.active = true;
            enemy.activationPending = false;
        }
        // scrOutofview (rmMain): y < viewTop-160 or y > viewTop+3000.
        // Native coordinates are relative to player start y=176; viewTop
        // starts at 34, hence -302 / +2858. Future prebuilt enemies are not
        // yet live instances and must not be destroyed before activation.
        if (enemy.active && enemy.awake &&
            (enemy.y - cameraPixelY_ < -302 || enemy.y - cameraPixelY_ > 2858)) {
            enemy.active = false;
            enemy.awake = false;
            enemy.activationPending = false;
        }
    }

#if !defined(DOWNWELL_PLATFORM_AMIGA) || defined(DOWNWELL_REPLAY_VERIFY) || defined(DOWNWELL_SCENE_VERIFY)
    const bool firstBulletActive = bulletActive();
    checkpoint_ = downwell::portCheckpointDigest(
        tick_, packedInput, player_.offsetX(), player_.offsetY(),
        (std::uint32_t)player_.ammo(), player_.grounded(), player_.jumps(),
        player_.shots(), ringOrigin_, firstBulletActive,
        bulletX(), bulletY());
    trajectory_.addU64(checkpoint_);
#endif

    AmigaPortFrame frame = {};
    frame.playerMotion = motion;
    frame.ammoChanged = player_.ammo() != oldAmmo;
    frame.healthChanged = health_ != oldHealth;
    frame.currencyChanged = currency_ != oldCurrency;
    frame.streakChanged = gemStreak_ != oldGemStreak;
    frame.bulletChanged = bulletChanged;
    frame.scrolled = scrollDelta != 0;
    frame.scrollDelta = scrollDelta;
    frame.cameraXChanged = cameraXDelta != 0;
    frame.cameraXDelta = cameraXDelta;
    frame.destroyedBlockCount = destroyedBlockCount;
    for (unsigned index = 0; index < destroyedBlockCount; ++index) {
        frame.blockX[index] = destroyedBlockX[index];
        frame.blockY[index] = destroyedBlockY[index];
    }
    DW_PROFILE_PHASE(profileSimulation);
    return frame;
}
