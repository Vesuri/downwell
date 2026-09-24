#include "AmigaPortRuntime.h"
#include "AmigaProfile.h"

#include "core/PortCheckpoint.h"
#include "core/GameMakerRounding.h"
#include "core/EnemyPresentation.h"
#include "core/EnemyContact.h"
#include "core/FrogBehavior.h"
#include "core/FrogMotion.h"
#include "core/FrogVerticalMotion.h"
#include "core/BatMotion.h"
#include "core/Replay.h"
#include "core/SolidCollision.h"

AmigaPortRuntime::AmigaPortRuntime(unsigned initialRingOrigin,
                                   bool startsGrounded,
                                   int landingOffset,
                                   bool followCamera)
    : player_(startsGrounded, landingOffset), tick_(0),
      ringOrigin_(initialRingOrigin % ringHeight),
      followCamera_(followCamera), cameraY_(0), cameraPixelY_(0),
      cameraPixelX_(0), activationY_(0),
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
        enemies_[index].gravityRemainder = 0;
        enemies_[index].gravityPositionRemainder = 0;
        enemies_[index].motionState = 0;
        enemies_[index].auxiliaryState = 0;
        enemies_[index].alarm0 = -1;
        enemies_[index].alarm1 = -1;
        enemies_[index].direction = 0;
        enemies_[index].moveSpeed = 0;
        enemies_[index].imagePhase = 0;
        enemies_[index].imageSpeed = 0;
        enemies_[index].imageRemainder = 0;
        enemies_[index].imageFrame = 0;
        enemies_[index].active = false;
        enemies_[index].awake = false;
        enemies_[index].grounded = false;
        enemies_[index].frogLanded = true;
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
        gem.motion.initialize(enemy.x, enemy.y, attracted, enemyRandom_);
        gem.speedX = downwell::GemMotion::q8(gem.motion.vx);
        gem.speedY = downwell::GemMotion::q8(gem.motion.vy);
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
                                      bool active, bool grounded, bool awake)
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
    enemy.frogHorizontal.position = (long)x * 65536L;
    enemy.frogHorizontal.velocity = (long)speedX * 256L;
    enemy.frogHit = {};
    enemy.preciseY = (long)y * 256L;
    enemy.speedX = speedX;
    enemy.speedY = speedY;
    enemy.gravityRemainder = 0;
    enemy.gravityPositionRemainder = 0;
    enemy.motionState = motionState;
    enemy.auxiliaryState = auxiliaryState;
    enemy.alarm0 = alarm0;
    enemy.alarm1 = alarm1;
    enemy.direction = direction;
    enemy.moveSpeed = moveSpeed;
    enemy.imagePhase = 0;
    enemy.imageRemainder = 0;
    enemy.imageSpeed = objectId == 111 ? 5u
                       : objectId == 112 ? 3u
                       : objectId == 116 ? 4u
                       : objectId == 117 || objectId == 119 ? 2u
                       : 0u;
    enemy.imageFrame = 0;
    enemy.active = active;
    enemy.awake = awake;
    enemy.grounded = grounded;
    enemy.frogLanded = true;
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
    for (auto& cell : breakableIndex_) cell = 0;
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

void AmigaPortRuntime::applyFrogAlarm(EnemyState& enemy, const downwell::FrogAlarmEffect& effect)
{
    if (effect.preparation >= 0) enemy.auxiliaryState = effect.preparation;
    if (effect.setJumpSpeed) enemy.motionState = effect.jumpSpeed;
    if (effect.setX) {
        enemy.frogHorizontal.velocity = effect.xQ16;
        enemy.speedX = downwell::FrogHorizontalMotion::toQ8(effect.xQ16);
    }
    if (effect.setY) {
        enemy.speedY = downwell::FrogVerticalMotion::floorDivide(effect.yFifthsQ8, 5);
        enemy.gravityRemainder = static_cast<unsigned char>(effect.yFifthsQ8 - enemy.speedY * 5);
    }
    if (effect.alarm == 0) enemy.alarm0 = effect.ticks;
    else enemy.alarm1 = effect.ticks;
    // Audio output is not yet implemented by this backend.
}

void AmigaPortRuntime::advanceEnemyAlarms()
{
    DW_PROFILE_PHASE(profileEnemyAlarms);
    for (unsigned enemyIndex = 0; enemyIndex < enemyCount_; ++enemyIndex) {
        EnemyState& enemy = enemies_[enemyIndex];
        if (!enemy.active)
            continue;
        if (enemy.objectId == 113) enemy.frogHit.advanceFrame();
        bool alarm0Fired = false, alarm1Fired = false;
        if (enemy.alarm0 > 0 && --enemy.alarm0 == 0)
            alarm0Fired = true;
        if (enemy.alarm1 > 0 && --enemy.alarm1 == 0)
            alarm1Fired = true;

        if (enemy.objectId == 112 && alarm1Fired) {
            enemy.auxiliaryState = -enemy.auxiliaryState;
            enemy.alarm1 = 120 + (int)enemyRandom_.irandomSmall(180);
        } else if (enemy.objectId == 113) {
            if (alarm0Fired)
                applyFrogAlarm(enemy, downwell::frogJumpAlarm(enemy.grounded, enemy.motionState));
            if (alarm1Fired) {
                const int direction = downwell::frogPreparationDirection(
                    enemy.grounded, enemy.x * 256L, enemy.y * 256L,
                    player_.frameStartXFixed(), (cameraPixelX_ - 80) * 256L,
                    cameraY_ - 142 * 256L, (cameraPixelX_ + 80) * 256L,
                    cameraY_ + 142 * 256L);
                applyFrogAlarm(enemy, downwell::frogPreparationAlarm(direction));
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
}

AmigaPortFrame AmigaPortRuntime::step(std::uint8_t packedInput)
{
    DW_PROFILE_PHASE(profilePlayer);
    // The reference dispatches the builder before the camera in rmMain's
    // ordinary Step instance order. Generation observes the previous camera,
    // not the camera position being prepared for this frame's display.
    const long generationCameraY = cameraY_;
    const int oldAmmo = player_.ammo();
    const int oldHealth = health_;
    const unsigned oldCurrency = currency_;
    const unsigned oldGemStreak = gemStreak_;
    const unsigned oldShots = player_.shots();
    bool bulletChanged = false;
    unsigned destroyedBlockCount = 0;
    int destroyedBlockX[destroyedCapacity] = {0};
    int destroyedBlockY[destroyedCapacity] = {0};

    ++tick_;
    if (dead_)
        ++deathFrames_;
    downwell::Input input = downwell::unpackReplayInput(packedInput);
    // objPlayer_n Begin Step advances activatey once, before gravity/motion.
    // Native camera offsets map to world view bottom 318 + cameraY.
    if (318L * 256L + cameraY_ > (long)(activationY_ - 112) * 256L) {
        activationY_ += 16;
        for (unsigned i = 0; i < enemyCount_; ++i) {
            EnemyState& enemy = enemies_[i];
            if (!enemy.active && enemy.activationPending &&
                enemy.y + 176 <= activationY_) {
                enemy.active = true;
                enemy.activationPending = false;
            }
        }
    }
    if (dead_)
        input = downwell::Input{};
    // scrPlayerShootN captures instance x/y before this Begin Step moves xx/yy.
    const int muzzleX = (int)downwell::roundGameMakerQ8(player_.offsetXFixed());
    const int muzzleY = (int)downwell::roundGameMakerQ8(player_.offsetYFixed()) + 4;
    struct HeadBreakContext {
        AmigaPortRuntime* runtime;
        int index;
    } headBreak = {this, -1};
    player_.configureBreakBoxResolver([](void* opaque, long x, long y) {
        auto& context = *static_cast<HeadBreakContext*>(opaque);
        auto& runtime = *context.runtime;
        for (unsigned i = 0; i < runtime.breakableCount_; ++i) {
            Breakable& box = runtime.breakables_[i];
            if (box.active && downwell::playerMaskMeetsBox(
                    x, y, (long)box.x * 1280, (long)box.y * 1280, 1280L)) {
                box.active = false;
                ++runtime.blocksDestroyed_;
                context.index = (int)i;
                return true;
            }
        }
        return false;
    }, &headBreak);
    AmigaPlayerMotion motion = player_.step(input);
    player_.configureBreakBoxResolver(nullptr, nullptr);
    if (headBreak.index >= 0) {
        const Breakable& box = breakables_[headBreak.index];
        destroyedBlockX[destroyedBlockCount] = box.x;
        destroyedBlockY[destroyedBlockCount++] = box.y;
    }
    if (player_.shots() != oldShots) {
        unsigned index;
        for (index = 0; index < bulletCapacity; ++index) {
            BulletState& bullet = bullets_[index];
            if (!bullet.active) {
                bullet.active = true;
                // Default scrPlayerInit starts shootLeg at -1;
                // scrPlayerEmitBullet negates it after every actual shot.
                bullet.x = muzzleX + ((oldShots & 1u) ? 1 : -1);
                bullet.y = muzzleY;
                bulletChanged = true;
                break;
            }
        }
    }

    // Alarms follow Begin Step movement, before builder Step/Create events.
    advanceEnemyAlarms();
    DW_PROFILE_PHASE(profilePlayer);
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

    if (generationStep_ && generationSucceeded_) {
        // Camera centre is 176 + cameraY_/256; view bottom is 142 below it.
        const long bottom = generationCameraY + 318L * 256L;
        const int bottomCeiling = bottom >= 0 ? (int)((bottom + 255) / 256)
                                              : -(int)((-bottom) / 256);
        generationSucceeded_ = generationStep_(generationContext_, *this,
                                               enemyRandom_, bottomCeiling);
    }

    /*
     * The adapted default bullet receives its Step later in the phase in
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
                    if (enemy.objectId == 113) enemy.frogHit.pending = true;
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
                    if (destroyedBlockCount < destroyedCapacity) {
                        destroyedBlockX[destroyedBlockCount] = block.x;
                        destroyedBlockY[destroyedBlockCount] = block.y;
                        ++destroyedBlockCount;
                    }
                    ++blocksDestroyed_;
                }
            }
            // scrBulCheckSolid also destroys an ordinary, non-wave bullet
            // on an unbreakable solid wall. Keep its enemy-line precedence:
            // an enemy hit above has already consumed the shot.
            if (bullet.active && solidResolver_ != 0) {
                for (int y = priorBulletY; y <= bullet.y; ++y) {
                    if (solidResolver_(solidResolverContext_, bullet.x, y)) {
                        bullet.active = false;
                        break;
                    }
                }
            }
            if (bullet.y >= cameraPixelY_ + bulletBottom)
                bullet.active = false;
        }
    }


    DW_PROFILE_PHASE(profileEnemyMotion);
    for (unsigned enemyIndex = 0; enemyIndex < enemyCount_; ++enemyIndex) {
        EnemyState& enemy = enemies_[enemyIndex];
        if (!enemy.active)
            continue;
        downwell::advanceEnemyImageQ8(enemy.imagePhase, enemy.imageRemainder,
                                     enemy.imageSpeed);
        if (!enemy.awake && enemy.y - cameraPixelY_ <= 254)
            enemy.awake = true;
        if ((!enemy.awake && enemy.objectId != 113) || solidResolver_ == 0)
            continue;
        if (enemy.objectId == 111 || enemy.objectId == 112) {
            const int halfWidth = enemy.objectId == 112 ? 8 : 6;
            const int foot = enemy.objectId == 112 ? 8 : 5;
            const int direction = enemy.speedX < 0 ? -1 : 1;
            const int probeX = (int)downwell::floorQ8Coordinate(enemy.preciseX);
            const int probeY = (int)downwell::floorQ8Coordinate(enemy.preciseY);
            const bool grounded = solidResolver_(
                solidResolverContext_, probeX, probeY + foot + 1);
            enemy.grounded = grounded;
            if (grounded) {
                enemy.speedY = 0;
                enemy.gravityRemainder = 0;
                if (!solidResolver_(solidResolverContext_,
                                    probeX + direction * (halfWidth + 1),
                                    probeY + foot + 1))
                    enemy.speedX = -enemy.speedX;
            } else {
                enemy.speedY += 51;
                if (++enemy.gravityRemainder == 5) {
                    ++enemy.speedY;
                    enemy.gravityRemainder = 0;
                }
                if (enemy.speedY >= 7 * 256) {
                    enemy.speedY = 7 * 256;
                    enemy.gravityRemainder = 0;
                }
            }
            const int nextX = (int)downwell::floorQ8Coordinate(enemy.preciseX + enemy.speedX);
            const int edge = nextX +
                (enemy.speedX < 0 ? -halfWidth : halfWidth);
            if (solidResolver_(solidResolverContext_, edge, probeY))
                enemy.speedX = -enemy.speedX;
            else
                enemy.preciseX += enemy.speedX;
            if (!grounded) {
                const unsigned residual = enemy.gravityPositionRemainder + enemy.gravityRemainder;
                const long nextPreciseY = enemy.preciseY + enemy.speedY + residual / 5u;
                const int nextY = (int)downwell::floorQ8Coordinate(nextPreciseY);
                const int movedX = (int)downwell::floorQ8Coordinate(enemy.preciseX);
                if (enemy.speedY > 0 && solidResolver_(
                        solidResolverContext_, movedX, nextY + foot)) {
                    while (!solidResolver_(solidResolverContext_, movedX,
                                (int)downwell::floorQ8Coordinate(enemy.preciseY) + foot + 1)) {
                        enemy.preciseY += 256L;
                    }
                    enemy.speedY = 0;
                    enemy.gravityRemainder = 0;
                    enemy.grounded = true;
                } else {
                    enemy.preciseY = nextPreciseY;
                    enemy.gravityPositionRemainder = static_cast<unsigned char>(residual % 5u);
                }
            }
        } else if (enemy.objectId == 116) {
            downwell::BatMotion bat{enemy.preciseX, enemy.preciseY, 0, 0,
                enemy.direction, (enemy.moveSpeed * 40 + 128) / 256};
            downwell::stepBatMotion(bat, player_.frameStartXFixed(),
                player_.frameStartYFixed(), enemyRandom_.nextU32(),
                [this](int x, int y) {
                    return solidResolver_(solidResolverContext_, x, y);
                });
            enemy.preciseX = bat.x;
            enemy.preciseY = bat.y;
            enemy.speedX = bat.speedX;
            enemy.speedY = bat.speedY;
            enemy.direction = bat.direction;
            enemy.moveSpeed = (bat.speedFortieths * 256 + 20) / 40;
        } else if (enemy.objectId == 113) {
            const bool integrate = downwell::frogMotionVisible(enemy.x * 256L, enemy.y * 256L,
                (cameraPixelX_ - 80) * 256L, cameraY_ - 142 * 256L,
                (cameraPixelX_ + 80) * 256L, cameraY_ + 142 * 256L);
            downwell::FrogVerticalMotion vertical{
                enemy.preciseY * 5 + enemy.gravityPositionRemainder,
                enemy.speedY * 5 + enemy.gravityRemainder, enemy.grounded};
            const bool hit = downwell::stepFrogMotion(enemy.frogHorizontal, vertical,
                enemy.frogHit, integrate, [this](int x, int y) {
                    return solidResolver_(solidResolverContext_, x, y);
                });
            if (hit) {
                enemy.auxiliaryState = 0;
                enemy.alarm0 = 0;
                enemy.alarm1 = 60;
                enemy.speedX = downwell::FrogHorizontalMotion::toQ8(enemy.frogHorizontal.velocity);
                enemy.imagePhase = 0;
                enemy.imageRemainder = 0;
            }
            enemy.preciseY = downwell::FrogVerticalMotion::floorDivide(vertical.position, 5);
            enemy.gravityPositionRemainder = static_cast<unsigned char>(vertical.position - enemy.preciseY * 5);
            enemy.speedY = downwell::FrogVerticalMotion::floorDivide(vertical.velocity, 5);
            enemy.gravityRemainder = static_cast<unsigned char>(vertical.velocity - enemy.speedY * 5);
            enemy.grounded = vertical.grounded;
            if (!enemy.grounded) {
                enemy.preciseX = downwell::FrogHorizontalMotion::toQ8(enemy.frogHorizontal.position);
                enemy.speedX = downwell::FrogHorizontalMotion::toQ8(enemy.frogHorizontal.velocity);
            }
            // enmFrog Step: restart preparation only on a landing edge.
            if (downwell::frogLandingEdge(enemy.grounded, enemy.frogLanded)) enemy.alarm1 = 60;
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
        // World origins (240,176) are even integers, so ties-to-even is
        // unchanged by storing these positions relative to the origin.
        enemy.x = (int)downwell::roundGameMakerQ8(enemy.preciseX);
        enemy.y = (int)downwell::roundGameMakerQ8(enemy.preciseY);
        if (enemy.speedX != 0)
            enemy.facing = enemy.speedX < 0 ? -1 : 1;
        enemy.spriteId = downwell::enemyPresentationSprite(enemy.objectId,
                                            enemy.grounded, enemy.auxiliaryState);
        if (enemy.objectId == 113) {
            if (enemy.frogHit.remaining != 0) {
                enemy.spriteId = 173;
            } else if (enemy.spriteId == 168) {
                enemy.imageSpeed = 0;
                enemy.imagePhase = enemy.speedY < 0 ? 0u : 256u;
                enemy.imageRemainder = 0;
            } else {
                enemy.imageSpeed = 4u;
            }
        }
        const unsigned frameCount = downwell::enemySpriteFrameCount(enemy.spriteId);
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
        const downwell::ContactActor playerContact{
            player_.offsetXFixed(), player_.offsetYFixed(), player_.facing(), 1, 31};
        const downwell::ContactActor enemyContact{
            enemy.preciseX, enemy.preciseY,
            enemy.objectId == 117 ? enemy.motionState : enemy.facing,
            enemy.objectId == 117 && enemy.speedY > 0 ? -1 : 1,
            downwell::enemyContactMask(enemy.objectId, enemy.spriteId)};
        const auto contact = downwell::classifyEnemyContact(
            playerContact, enemyContact, enemy.objectId, player_.frameStartYFixed());
        if (contact == downwell::EnemyContact::none) continue;
        if (contact == downwell::EnemyContact::stomp) {
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
        gem.motion.step(gem.age, player_.frameStartXFixed(), player_.frameStartYFixed(),
            [this](int x, int y) { return solidResolver_ && solidResolver_(solidResolverContext_, x, y); }, enemyRandom_);
        gem.preciseX = downwell::GemMotion::q8(gem.motion.x);
        gem.preciseY = downwell::GemMotion::q8(gem.motion.y);
        gem.speedX = downwell::GemMotion::q8(gem.motion.vx);
        gem.speedY = downwell::GemMotion::q8(gem.motion.vy);
        gem.attracted = gem.motion.attracted;
        gem.x = downwell::roundGameMakerQ8(gem.preciseX);
        gem.y = downwell::roundGameMakerQ8(gem.preciseY);
        if (gem.motion.collectible(gem.age, player_.offsetXFixed(), player_.offsetYFixed())) {
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
        tick_, packedInput, player_, ringOrigin_, firstBulletActive,
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
