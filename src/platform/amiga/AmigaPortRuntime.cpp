#include "AmigaPortRuntime.h"
#include "core/MainRoomLifetime.h"
#include "core/MainRoomActivation.h"
#include "AmigaProfile.h"

#include "core/PortCheckpoint.h"
#include "core/GameMakerRounding.h"
#include "core/EnemyPresentation.h"
#include "core/EnemyContact.h"
#include "core/EnemyContactResponse.h"
#include "core/CurrencyReward.h"
#include "core/CurrencyStep.h"
#include "core/EnemyAlarmEvents.h"
#include "core/AlarmBank.h"
#include "core/FrogBehavior.h"
#include "core/FrogMotion.h"
#include "core/StandardBulletStep.h"
#include "core/StandardBulletImpact.h"
#include "core/PlayerLifeRules.h"
#include "core/StableInstanceSlots.h"
#include "core/FrogVerticalMotion.h"
#include "core/BatMotion.h"
#include "core/Replay.h"
#include "core/SolidCollision.h"
#ifdef DOWNWELL_FREESTANDING
inline void* operator new(__SIZE_TYPE__, void* address) noexcept { return address; }
#else
#include <new>
#endif

downwell::ResultsDestination AmigaPortRuntime::restartFromResults(
        downwell::DeathMenuSequence::Action action, std::uint32_t seed, unsigned initialRingOrigin) {
    const auto destination = downwell::resultsDestination(action);
    if (destination == downwell::ResultsDestination::none) return destination;
    const auto records = run_.records();
    this->~AmigaPortRuntime();
    new (this) AmigaPortRuntime(initialRingOrigin, false, 1201 - 176, true);
    run_.restart(action, records);
    enemyRandom_.setSeed(seed);
    return destination;
}

bool AmigaPortRuntime::prepareNextAreaOneLevel(const downwell::Input& input, unsigned initialRingOrigin) {
    if (!run_.levelClearActive || !run_.nextLevelRequested ||
        run_.stage.area != 1 || run_.stage.level >= 3) return false;
    if (!run_.advanceAfterLevelClear()) return false;
    // Do not reconstruct this large runtime or copy RunSession onto the task
    // stack: its run globals remain in place while old world ownership retires.
    run_.shops = {};
    player_.enterRoom(input.fire, 1201 - 176);
    emitter_.reset(run_.upgrades.levels[12] != 0);
    run_.ammoHud.previousShots = run_.ammoHud.previousRecharges = 0;
    currencyAccount_.playerCreated();
    death_ = {};
    invulnerability_ = 0;
    enemyCount_ = enemiesDefeated_ = breakableCount_ = blocksDestroyed_ = 0;
    bulletSlotEnd_ = gemSlotEnd_ = 0;
    for (unsigned i = 0; i < bulletCapacity; ++i) bullets_[i].active = false;
    for (unsigned i = 0; i < enemyCapacity; ++i) enemies_[i].active = false;
    for (unsigned i = 0; i < gemCapacity; ++i) gems_[i].active = false;
    generationStep_ = nullptr; generationContext_ = nullptr;
    generationSucceeded_ = true;
    solidResolver_ = nullptr; solidResolverContext_ = nullptr;
    landingResolver_ = nullptr; landingResolverContext_ = nullptr;
    breakableIndexReady_ = false;
    followCamera_ = true;
    cameraMotion_.reset(240, 176);
    cameraY_ = cameraPixelY_ = cameraPixelX_ = activationY_ = 0;
    ringOrigin_ = initialRingOrigin % ringHeight;
    return true;
}

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
      solidResolver_(0),
      solidResolverContext_(0),
      breakableCount_(0), blocksDestroyed_(0), checkpoint_(0)
{
    unsigned index;
    run_.reset();
    run_.stage.advance(1); // This runtime starts at the confirmed scrNextLevel(1) entry.
    currencyAccount_.playerCreated();
    if (followCamera_) {
        /*
         * camMain receives one Step on rmMain's creation frame, before the
         * player receives movement. Its recovered +24 target and /5 easing
         * therefore establish the first visible camera/ring position here.
         */
        cameraMotion_.step(240 * 256L, 176 * 256L);
        cameraY_ = (cameraMotion_.y - 176L * 4096) / 16;
        cameraPixelY_ = cameraMotion_.pixelY() - 176;
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
    unsigned next = downwell::compactCurrencySlots(gems_, gemSlotEnd_);
    downwell::spawnCurrencyReward(enemy.reward, enemy.x, enemy.y, attracted,
        gemCapacity - next, enemyRandom_, [this, &enemy, attracted, &next](const downwell::CurrencyBirth& birth) {
        GemState& gem = gems_[next++];
        gem.large = birth.large;
        gem.amount = birth.amount;
        gem.age = 0;
        gem.x = enemy.x;
        gem.y = enemy.y;
        gem.preciseX = (long)enemy.x * 256L;
        gem.preciseY = (long)enemy.y * 256L;
        gem.motion = birth.motion;
        gem.speedX = downwell::GemMotion::q8(gem.motion.vx);
        gem.speedY = downwell::GemMotion::q8(gem.motion.vy);
        gem.attracted = attracted;
        gem.active = true;
    }, run_.upgrades.levels[10] != 0);
    gemSlotEnd_ = next;
}

unsigned AmigaPortRuntime::gemCount() const
{
    unsigned count = 0;
    for (unsigned index = 0; index < gemSlotEnd_; ++index)
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
    enemy.crawler.initialize(x, y);
    enemy.crawler.latched = grounded;
    enemy.snail.initialize(x, y, speedY >= 0);
    enemy.snailHit = {};
    enemy.batHit = {};
    enemy.batDirectionFraction = 0;
    enemy.batActivity = {};
    enemy.walker.initialize(x, y, speedX * 5L, speedY * 5L, grounded);
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
    enemy.frogPresentation = {};
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
    for (index = 0; index < bulletSlotEnd_; ++index)
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
    for (index = 0; index < bulletSlotEnd_; ++index)
        if (bullets_[index].active)
            return bullets_[index].x;
    return 0;
}

int AmigaPortRuntime::bulletY() const
{
    unsigned index;
    for (index = 0; index < bulletSlotEnd_; ++index)
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
    DW_PROFILE_COUNT(counterBreakableFallback);
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
        if (enemy.objectId == 117) enemy.snailHit.advanceFrame();
        if (enemy.objectId == 116) enemy.batHit.advanceFrame();
        const unsigned events = downwell::enemyAlarmEvents(enemy.objectId);
        const bool alarm0Fired = downwell::AlarmBank::advanceValue(enemy.alarm0, (events & 1) != 0);
        const bool alarm1Fired = downwell::AlarmBank::advanceValue(enemy.alarm1, (events & 2) != 0);

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
                    viewCameraYQ8() - 142 * 256L, (cameraPixelX_ + 80) * 256L,
                    viewCameraYQ8() + 142 * 256L);
                applyFrogAlarm(enemy, downwell::frogPreparationAlarm(direction));
            }
        } else if (enemy.objectId == 116 && alarm0Fired) {
            enemy.batHit.alarm();
        } else if (enemy.objectId == 119 && alarm0Fired) {
            updateCrawler(enemy, true);
            enemy.alarm0 = 2;
        }
    }
}

void AmigaPortRuntime::updateCrawler(EnemyState& enemy, bool alarm)
{
    const long cx = cameraPixelX_ * 4096L;
    const long cy = viewCameraYQ8() * 16;
    const bool visible = downwell::CrawlerMotion::inView(enemy.x, enemy.y,
        cx - 80L * 4096, cy - 142L * 4096, cx + 80L * 4096, cy + 142L * 4096,
        alarm ? 0 : 64);
    const auto meeting = [this](long x, long y) {
        return downwell::CrawlerMotion::meetsTiles(x, y, [this](int px, int py) {
            return solidResolver_ && solidResolver_(solidResolverContext_, px, py);
        });
    };
    if (alarm) enemy.crawler.alarm(enemy.motionState > 0, visible, meeting);
    else enemy.crawler.step(enemy.motionState > 0, visible, meeting);
    enemy.preciseX = enemy.crawler.x / 5;
    enemy.preciseY = enemy.crawler.y / 5;
    enemy.speedX = enemy.crawler.vx / 5;
    enemy.speedY = enemy.crawler.vy / 5;
    enemy.grounded = enemy.crawler.latched;
}

AmigaPortFrame AmigaPortRuntime::step(std::uint8_t packedInput)
{
    if (run_.levelClearActive) return {}; // Result owner uses stepLevelClear.
    DW_PROFILE_PHASE(profilePlayer);
    // Wrappers own a runtime pointer; rebind after any host-side runtime copy.
    if (solidResolver_) configureSolidResolver(solidResolver_, solidResolverContext_);
    if (landingResolver_) configureLandingResolver(landingResolver_, landingResolverContext_);
    // The reference dispatches the builder before the camera in rmMain's
    // ordinary Step instance order. Generation observes the previous camera,
    // not the camera position being prepared for this frame's display.
    const long generationCameraY = viewCameraYQ8();
    const int oldAmmo = player_.ammo();
    const int oldAmmoCapacity = player_.ammoCapacity();
    const int oldHealth = health_;
    const int oldHealthMaximum = healthMaximum();
    const unsigned oldHeartPieces = healthReserve().pieces;
    const unsigned oldCurrency = currencyAccount_.currency;
    const unsigned oldGemStreak = currencyAccount_.streak;
    const unsigned oldShots = player_.shots();
    bool bulletChanged = false;
    unsigned destroyedBlockCount = 0;
    int destroyedBlockX[destroyedCapacity] = {0};
    int destroyedBlockY[destroyedCapacity] = {0};

    ++tick_;
    currencyAccount_.beginStep();
    run_.selectWeaponLevel(currencyAccount_);
    run_.consumables.beginStep(player_, health_, run_.combo.healthReserve);
    downwell::Input input = downwell::unpackReplayInput(packedInput);
    run_.results.beginStep(input);
    death_.beginStep(input.fire);
    run_.combo.beginStep();
    // objPlayer_n Begin Step advances activatey once, before gravity/motion.
    // Native camera offsets map to world view bottom 318 + cameraY.
    downwell::advanceMainRoomActivation(activationY_,
        318L * 256L + viewCameraYQ8(), enemyCount_,
        [this](unsigned i) {
            const auto& enemy = enemies_[i];
            return downwell::ActivationInstance{enemy.objectId, (enemy.y + 176L) * 256,
                                               enemy.active, enemy.activationPending};
        },
        [this](unsigned i) {
            enemies_[i].active = true;
            enemies_[i].activationPending = false;
        });
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
    player_.configureComboCompletion([](void* context) {
        auto& runtime = *static_cast<AmigaPortRuntime*>(context);
        runtime.run_.combo.finish(runtime.player_, runtime.currencyAccount_, runtime.health_,
                                  runtime.run_.upgrades.levels[20] != 0);
    }, this);
    player_.configureMovementBoundary([](void* context) {
        auto& runtime = *static_cast<AmigaPortRuntime*>(context);
        runtime.run_.gems.observeDeathMovement(runtime.death_.movementDead(), runtime.currencyAccount_.gameGem);
    }, this);
    AmigaPlayerMotion motion = player_.step(input, run_.shops.noControl || run_.exitControlsLocked,
        death_.movementDead(), run_.shops.interactable, run_.goalStopped, run_.gemHigh);
    const int shopPlayerX = 240 + downwell::roundGameMakerQ8(player_.offsetXFixed());
    const int shopPlayerY = 176 + downwell::roundGameMakerQ8(player_.offsetYFixed());
    run_.shops.observePlayer(shopPlayerX, shopPlayerY, player_.grounded(), player_.speedXFixed());
    int shopFacing = player_.facing();
    const auto shopRandom = [this](unsigned lo, unsigned hi) { return lo + enemyRandom_.irandomSmall(hi - lo); };
    run_.shops.keeperAlarms(shopRandom);
    run_.shops.step(input, shopPlayerX, shopPlayerY, false, death_.active,
        shopFacing, currencyAccount_, [this] { return 1u + enemyRandom_.irandomSmall(2); },
        [this](unsigned id) { grantConsumable(id); }, [](auto&, auto) {});
    player_.face(shopFacing);
    run_.shops.advanceKeepers(shopPlayerX, false, run_.stage.area, shopRandom);
    run_.shops.advancePresentation();
    player_.configureComboCompletion(nullptr, nullptr);
    player_.configureMovementBoundary(nullptr, nullptr);
    // Goal-field overlap and boss completion remain separate source guards.
    run_.clock.advance(true, death_.active, run_.goalStopped);
    run_.advanceLevelExitAlarms(cameraMotion_.pixelX() - 80, cameraMotion_.pixelY() - 142);
    death_.publishHealth(health_);
    death_.advanceAlarms(player_.grounded());
    run_.results.open(death_.menuReady, run_.gems.lifetime, currencyAccount_.gameGem);
    player_.configureBreakBoxResolver(nullptr, nullptr);
    if (headBreak.index >= 0) {
        const Breakable& box = breakables_[headBreak.index];
        destroyedBlockX[destroyedBlockCount] = box.x;
        destroyedBlockY[destroyedBlockCount++] = box.y;
    }
    if (player_.shots() != oldShots) {
        const int muzzleOffset = emitter_.nextOffset();
        const unsigned index = downwell::compactInstanceSlots(bullets_, bulletSlotEnd_);
        bulletSlotEnd_ = index;
        if (index < bulletCapacity) {
            BulletState& bullet = bullets_[index];
            bullet.active = true;
            bulletSlotEnd_ = index + 1;
            bullet.x = muzzleX + muzzleOffset;
            bullet.y = muzzleY;
            bullet.gemHigh = run_.gemHigh;
            bullet.animation = {};
            bullet.trajectory.reset(bullet.x, bullet.y, run_.upgrades.levels[12] != 0);
            bulletChanged = true;
        }
    }

    // Alarms follow Begin Step movement, before builder Step/Create events.
    advanceEnemyAlarms();
    DW_PROFILE_PHASE(profilePlayer);
    int scrollDelta = 0;
    int cameraXDelta = 0;
    if (followCamera_) {
        const int oldCameraPixelX = cameraPixelX_;
        DW_PROFILE_SCOPE(profileCamera);
        const int oldCameraPixelY = cameraPixelY_;
        cameraMotion_.step(240L * 256 + player_.offsetXFixed(),
                           176L * 256 + player_.offsetYFixed(), dead());
        cameraPixelX_ = cameraMotion_.pixelX() - 240;
        cameraXDelta = cameraPixelX_ - oldCameraPixelX;
        cameraY_ = (cameraMotion_.y - 176L * 4096) / 16;
        cameraPixelY_ = cameraMotion_.pixelY() - 176;
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
        DW_PROFILE_SCOPE(profileGeneration);
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
        for (bulletIndex = 0; bulletIndex < bulletSlotEnd_; ++bulletIndex) {
            BulletState& bullet = bullets_[bulletIndex];
            if (!bullet.active)
                continue;
            bulletChanged = true;
            bullet.animation.advance();
            bullet.active = downwell::stepMachineGunBullet(bullet.trajectory, enemyRandom_,
                [this, &bullet](const downwell::ProjectileSegment& path) {
                    for (unsigned enemyIndex = 0; enemyIndex < enemyCount_; ++enemyIndex) {
                        EnemyState& enemy = enemies_[enemyIndex];
                        const downwell::ContactActor contact{
                            enemy.x * 256L, enemy.y * 256L,
                            enemy.objectId == 117 ? enemy.motionState : enemy.facing,
                            enemy.imageScaleY(),
                            downwell::enemyContactMask(enemy.objectId, enemy.spriteId)};
                        if (!enemy.active || enemy.health <= 0 ||
                            !path.intersects(contact))
                            continue;
                        const auto impact = downwell::applyStandardBulletImpact(
                            enemy, enemiesDefeated_, enemyRandom_,
                            [this, &enemy] { run_.combo.state.enemyDefeated(); spawnReward(enemy, false); },
                            bullet.gemHigh, bullet.trajectory.direction);
                        return impact.kind != downwell::BulletImpactKind::ignored;
                    }
                    return false;
                }, [this, &destroyedBlockCount, &destroyedBlockX, &destroyedBlockY]
                   (const downwell::ProjectileSegment& path) {
                    unsigned firstBlock = 0, endBlock = breakableCount_;
                    if (breakableIndexReady_) {
                        int candidate = -1;
                        // An eight-pixel segment can touch at most four
                        // 16-pixel tiles, including a corner-adjacent tile
                        // containing neither endpoint. Test all candidates.
                        const int xs[] = {path.x.pixel, path.endX.pixel};
                        const int ys[] = {path.y.pixel, path.endY.pixel};
                        for (int x : xs) for (int y : ys) {
                            const int next = indexedBreakable(x, y);
                            if (next < 0) continue;
                            const auto& block = breakables_[next];
                            if (block.active && path.intersects(block.x-8, block.x+8, block.y-8, block.y+8) &&
                                    (candidate < 0 || next < candidate)) candidate = next;
                        }
                        firstBlock = candidate < 0 ? 0 : (unsigned)candidate;
                        endBlock = candidate < 0 ? 0 : firstBlock + 1;
                    }
                    for (unsigned blockIndex = firstBlock; blockIndex < endBlock; ++blockIndex) {
                        Breakable& block = breakables_[blockIndex];
                        if (block.active && path.intersects(block.x - 8, block.x + 8,
                                                           block.y - 8, block.y + 8)) {
                            block.active = false;
                            if (destroyedBlockCount < destroyedCapacity) {
                                destroyedBlockX[destroyedBlockCount] = block.x;
                                destroyedBlockY[destroyedBlockCount] = block.y;
                                ++destroyedBlockCount;
                            }
                            ++blocksDestroyed_;
                            return true;
                        }
                    }
                    // An ordinary, non-wave bullet also stops at unbreakable walls.
                    if (solidResolver_ != 0) {
                        const int left = path.x.pixel < path.endX.pixel ? path.x.pixel : path.endX.pixel;
                        const int right = path.x.pixel > path.endX.pixel ? path.x.pixel : path.endX.pixel;
                        const int top = path.y.pixel < path.endY.pixel ? path.y.pixel : path.endY.pixel;
                        const int bottom = path.y.pixel > path.endY.pixel ? path.y.pixel : path.endY.pixel;
                        for (int y = top; y <= bottom; ++y)
                            for (int x = left; x <= right; ++x)
                                if (solidResolver_(solidResolverContext_, x, y) && path.intersects(x,x+1,y,y+1))
                                    return true;
                    }
                    return false;
                });
            bullet.x = bullet.trajectory.x.pixel;
            bullet.y = bullet.trajectory.y.pixel;
            const long cameraCentreQ12 = 176L * 4096 + viewCameraYQ8() * 16;
            if (downwell::fractionalBulletOutsideView(bullet.trajectory.y, 176,
                                                    cameraCentreQ12 - 142L * 4096))
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
        if (enemy.objectId == 113)
            enemy.imageSpeed = enemy.frogPresentation.beginStepRate();
        if (!enemy.awake && enemy.y - cameraPixelY_ <= 254)
            enemy.awake = true;
        if ((!enemy.awake && enemy.objectId != 113 && enemy.objectId != 111 && enemy.objectId != 112 && enemy.objectId != 116) || solidResolver_ == 0)
            continue;
        if (enemy.objectId == 111 || enemy.objectId == 112) {
            const bool turtle = enemy.objectId == 112;
            const long cx = cameraPixelX_ * 4096L;
            const long cy = viewCameraYQ8() * 16;
            const bool visible = downwell::CrawlerMotion::inView(enemy.x, enemy.y,
                cx - 80L * 4096, cy - 142L * 4096, cx + 80L * 4096, cy + 142L * 4096,
                turtle ? 64 : 32);
            enemy.walker.step(turtle, downwell::groundTurtleWalking(enemy.auxiliaryState), visible,
                [this, &enemy, turtle](long x, long y) {
                    return enemy.walker.meetsTiles(x, y, turtle, [this](int px, int py) {
                        return solidResolver_(solidResolverContext_, px, py);
                    });
                });
            enemy.preciseX = downwell::FrogVerticalMotion::floorDivide(enemy.walker.x, 5);
            enemy.preciseY = downwell::FrogVerticalMotion::floorDivide(enemy.walker.y, 5);
            enemy.speedX = downwell::FrogVerticalMotion::floorDivide(enemy.walker.vx, 5);
            enemy.speedY = downwell::FrogVerticalMotion::floorDivide(enemy.walker.vy, 5);
            enemy.gravityPositionRemainder = enemy.walker.y - enemy.preciseY * 5;
            enemy.gravityRemainder = enemy.walker.vy - enemy.speedY * 5;
            enemy.grounded = enemy.walker.grounded;
        } else if (enemy.objectId == 116) {
            downwell::BatMotion bat{enemy.preciseX, enemy.preciseY, enemy.speedX, enemy.speedY,
                enemy.direction, (enemy.moveSpeed * 40 + 128) / 256, enemy.facing,
                enemy.batDirectionFraction};
            const auto effects = downwell::stepBatBehavior(bat, enemy.batHit, enemy.batActivity,
                enemy.x, enemy.y, enemy.spriteId, player_.frameStartXFixed(),
                player_.frameStartYFixed(), enemyRandom_,
                [this](int x, int y) {
                    return solidResolver_(solidResolverContext_, x, y);
                });
            if (effects.hit) {
                enemy.alarm0 = 3;
                enemy.imagePhase = 0;
                enemy.imageRemainder = 0;
            }
            enemy.preciseX = bat.x;
            enemy.preciseY = bat.y;
            enemy.speedX = bat.speedX;
            enemy.speedY = bat.speedY;
            enemy.direction = bat.direction;
            enemy.batDirectionFraction = bat.directionFraction;
            enemy.facing = bat.facing;
            enemy.moveSpeed = (bat.speedFortieths * 256 + 20) / 40;
        } else if (enemy.objectId == 113) {
            const bool integrate = downwell::frogMotionVisible(enemy.x * 256L, enemy.y * 256L,
                (cameraPixelX_ - 80) * 256L, viewCameraYQ8() - 142 * 256L,
                (cameraPixelX_ + 80) * 256L, viewCameraYQ8() + 142 * 256L);
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
            const int hitAlarm = enemy.snailHit.consume(enemyRandom_);
            if (hitAlarm >= 0) {
                enemy.imagePhase = 0; enemy.imageRemainder = 0;
                enemy.alarm0 = 0; enemy.alarm1 = hitAlarm;
            }
            enemy.active = enemy.snail.step([this](int x, int y) {
                return solidResolver_(solidResolverContext_, x, y);
            });
            if (!enemy.active)
                continue;
            enemy.preciseX = enemy.snail.x / 5;
            enemy.preciseY = enemy.snail.y / 5;
            enemy.speedY = enemy.snail.vy / 5;
            enemy.motionState = enemy.facing = enemy.snail.facing;
        } else if (enemy.objectId == 119) {
            updateCrawler(enemy, false);
        }
        // World origins (240,176) are even integers, so ties-to-even is
        // unchanged by storing these positions relative to the origin.
        enemy.x = (int)downwell::roundGameMakerQ8(enemy.preciseX);
        enemy.y = (int)downwell::roundGameMakerQ8(enemy.preciseY);
        if (enemy.objectId == 111 || enemy.objectId == 112) {
            enemy.x = enemy.walker.publishedX;
            enemy.y = enemy.walker.publishedY;
        }
        if (enemy.objectId == 119) {
            enemy.x = enemy.crawler.pixelX();
            enemy.y = enemy.crawler.pixelY();
        }
        if (enemy.objectId == 117) {
            enemy.x = static_cast<int>(downwell::FixedCollisionMath::round(enemy.snail.x) / 1280);
            enemy.y = static_cast<int>(downwell::FixedCollisionMath::round(enemy.snail.y) / 1280);
        }
        if (enemy.speedX != 0)
            enemy.facing = enemy.speedX < 0 ? -1 : 1;
        enemy.spriteId = downwell::enemyPresentationSprite(enemy.objectId,
                                            enemy.grounded, enemy.auxiliaryState);
        if (enemy.objectId == 117 && enemy.snailHit.stunned) enemy.spriteId = 178;
        if (enemy.objectId == 116)
            enemy.spriteId = downwell::batPresentation(enemy.batActivity, enemy.batHit);
        if (enemy.objectId == 113) {
            const auto visual = enemy.frogPresentation.finishStep(
                enemy.frogHit.remaining != 0, enemy.grounded,
                enemy.auxiliaryState != 0, enemy.speedY < 0);
            enemy.spriteId = visual.sprite;
            if (visual.forcedFrame >= 0) {
                enemy.imagePhase = static_cast<unsigned>(visual.forcedFrame) * 256u;
                enemy.imageRemainder = 0;
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
        if (!enemy.active || enemy.health <= 0)
            continue;
        const downwell::ContactActor enemyContact{
            enemy.preciseX, enemy.preciseY,
            enemy.objectId == 117 ? enemy.motionState : enemy.facing,
            enemy.imageScaleY(),
            downwell::enemyContactMask(enemy.objectId, enemy.spriteId)};
        downwell::applyEnemyContact(enemy.objectId, enemyContact,
            enemy.x, enemy.y, enemy.active, enemy.health, player_, health_, invulnerability_, enemiesDefeated_,
            [this, &enemy] { run_.combo.state.enemyDefeated(); spawnReward(enemy, true); },
            run_.upgrades.levels[7] != 0);
    }

    DW_PROFILE_PHASE(profileGems);
    for (unsigned gemIndex = 0; gemIndex < gemSlotEnd_; ++gemIndex) {
        GemState& gem = gems_[gemIndex];
        if (!gem.active)
            continue;
        const auto result = run_.collectCurrency(gem.motion, gem.large, static_cast<unsigned>(gem.amount),
            player_, currencyAccount_,
            [this](int x, int y) { return solidResolver_ && solidResolver_(solidResolverContext_, x, y); }, enemyRandom_);
        if (result != downwell::CurrencyStepResult::alive) {
            gem.active = false;
            continue;
        }
        gem.age = gem.motion.lifetime.age;
        gem.preciseX = downwell::GemMotion::q8(gem.motion.x);
        gem.preciseY = downwell::GemMotion::q8(gem.motion.y);
        gem.speedX = downwell::GemMotion::q8(gem.motion.vx);
        gem.speedY = downwell::GemMotion::q8(gem.motion.vy);
        gem.attracted = gem.motion.attracted;
        gem.x = downwell::GemMotion::rounded(gem.motion.x);
        gem.y = downwell::GemMotion::rounded(gem.motion.y);
    }

    DW_PROFILE_PHASE(profileLifecycle);
    // Only discard inactive tails; holes stay in creation order until the
    // existing stable compaction at the next birth. Never reorder live slots.
    while (bulletSlotEnd_ && !bullets_[bulletSlotEnd_-1].active) --bulletSlotEnd_;
    while (gemSlotEnd_ && !gems_[gemSlotEnd_-1].active) --gemSlotEnd_;
    run_.exitDither.step(cameraMotion_.pixelY() - 142, 284);
    run_.collideLevelExit(240 + downwell::roundGameMakerQ8(player_.offsetXFixed()),
        176 + downwell::roundGameMakerQ8(player_.offsetYFixed()), death_.active);
    const long viewTop = 34L * 4096 + viewCameraYQ8() * 16;
    downwell::retireMainRoomObjects(enemyCount_, 11, 1, viewTop,
        [this](unsigned i) {
            const auto& enemy = enemies_[i];
            return downwell::LifetimeInstance{enemy.objectId, enemy.y + 176, enemy.active};
        },
        [this](unsigned i) {
            auto& enemy = enemies_[i];
            enemy.active = false;
            enemy.awake = false;
            enemy.activationPending = false;
        });
    downwell::retireMainRoomObjects(breakableCount_, 11, 1, viewTop,
        [this](unsigned i) {
            const auto& box = breakables_[i];
            return downwell::LifetimeInstance{92, box.y + 176, box.active};
        },
        [this](unsigned i) { breakables_[i].active = false; });

#if !defined(DOWNWELL_PLATFORM_AMIGA) || defined(DOWNWELL_REPLAY_VERIFY) || defined(DOWNWELL_SCENE_VERIFY)
    const bool firstBulletActive = bulletActive();
    checkpoint_ = downwell::portCheckpointDigest(
        tick_, packedInput, player_, ringOrigin_, firstBulletActive,
        bulletX(), bulletY());
    trajectory_.addU64(checkpoint_);
#endif

    run_.shops.endStep(34 + downwell::floorQ8Coordinate(viewCameraYQ8()));
    AmigaPortFrame frame = {};
    run_.results.presentationStep();
    frame.playerMotion = motion;
    DW_PROFILE_PHASE(profileHudState);
    const bool ammoMotionChanged = run_.ammoHud.step(player_.shots(), player_.recharges());
    const bool ammoRowsChanged = run_.ammoGauge.step(player_.ammo(), player_.ammoCapacity());
    frame.ammoChanged = player_.ammo() != oldAmmo || player_.ammoCapacity() != oldAmmoCapacity || ammoMotionChanged || ammoRowsChanged;
    frame.healthChanged = health_ != oldHealth || healthMaximum() != oldHealthMaximum ||
                          healthReserve().pieces != oldHeartPieces;
    frame.currencyChanged = currencyAccount_.currency != oldCurrency;
    frame.streakChanged = currencyAccount_.streak != oldGemStreak;
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
