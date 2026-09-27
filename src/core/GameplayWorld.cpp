#include "GameplayWorld.h"
#include "PlayerBoxContact.h"
#include "GameplayPlayerStep.h"
#include "core/MainRoomLifetime.h"
#include "core/MainRoomActivation.h"
#include "Profile.h"

#include "core/PortCheckpoint.h"
#include "core/GameMakerRounding.h"
#include "core/EnemyPresentation.h"
#include "core/EnemyCreateProfile.h"
#include "core/EnemyStepPresentation.h"
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

downwell::ResultsDestination GameplayWorld::restartFromResults(
        downwell::DeathMenuSequence::Action action, std::uint32_t seed, unsigned initialRingOrigin) {
    const auto destination = downwell::resultsDestination(action);
    if (destination == downwell::ResultsDestination::none) return destination;
    if (!run_.effectsSucceeded || !clearEnemyOwnership() || !clearBreakableOwnership() || !retirePlayerOwnership()) return downwell::ResultsDestination::none;
    if(terrainInstances_&&!terrainInstances_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    const auto terrainInstances=terrainInstances_;
    const auto phaseBoundary=phaseBoundary_;
    const auto enemyOrderCapture=enemyOrderCapture_;
    void* const enemyOrderContext=enemyOrderContext_;
    void* const phaseBoundaryContext=phaseBoundaryContext_;
    const auto playerLifecycle=playerLifecycle_;
    const auto nextPlayerIdentity=nextPlayerLifetimeId_;
    const auto enemyLifecycle=enemyLifecycle_;
    const auto breakableLifecycle=breakableLifecycle_;
    const auto nextBreakableIdentity=nextBreakableLifetimeId_;
    const auto records = run_.records();
    const auto nextBulletIdentity=nextBulletLifetimeId_;
    const auto nextEnemyIdentity=nextEnemyLifetimeId_;
    const auto nextGemIdentity=nextGemLifetimeId_;
    const auto fieldBirths=run_.timeFields.birthSequence();
    const auto numberBirths=run_.damageNumbers.birthSequence();
    const auto expiryBirths=run_.corpseExpiryEffects.birthSequence();
    const auto smokeBirths=run_.smokeEffects.birthSequence();
    const auto bloodBirths=run_.bloodEffects.birthSequence();
    const auto flashBirths=run_.flashEffects.birthSequence();
    const auto foregroundBirth=run_.nextForegroundBirth;
    this->~GameplayWorld();
    new (this) GameplayWorld(initialRingOrigin, false, 1201 - 176, true, enemyLifecycle,breakableLifecycle,{},terrainInstances);
    nextBreakableLifetimeId_=nextBreakableIdentity;
    configurePhaseBoundary(phaseBoundary,phaseBoundaryContext);
    configureEnemyStepOrder(enemyOrderCapture,enemyOrderContext);
    nextBulletLifetimeId_=nextBulletIdentity;
    nextEnemyLifetimeId_=nextEnemyIdentity;
    nextGemLifetimeId_=nextGemIdentity;
    // Construction above has no bound player owner. Recreate its lifetime
    // only after restoring the run and the preserved monotonic key sequence.
    playerLifecycle_=playerLifecycle;
    playerInstance_.lifetimeId=0;
    nextPlayerLifetimeId_=nextPlayerIdentity;
    run_.restart(action, records);
    if(!run_.timeFields.restoreEmptyBirthSequence(fieldBirths))run_.effectsSucceeded=false;
    if(!run_.smokeEffects.restoreEmptyBirthSequence(smokeBirths)||
       !run_.bloodEffects.restoreEmptyBirthSequence(bloodBirths)||
       !run_.flashEffects.restoreEmptyBirthSequence(flashBirths))run_.effectsSucceeded=false;
    if(!run_.damageNumbers.restoreEmptyBirthSequence(numberBirths)||
       !run_.corpseExpiryEffects.restoreEmptyBirthSequence(expiryBirths))run_.effectsSucceeded=false;
    run_.nextForegroundBirth=foregroundBirth;
    enemyRandom_.setSeed(seed);
    if(!birthPlayerOwnership())return downwell::ResultsDestination::none;
    return destination;
}

bool GameplayWorld::prepareNextAreaOneLevel(const downwell::Input& input, unsigned initialRingOrigin) {
    if (!run_.levelClearActive || !run_.nextLevelRequested ||
        run_.stage.area != 1 || run_.stage.level >= 3) return false;
    if (!run_.effectsSucceeded || !clearEnemyOwnership() || !clearBreakableOwnership() || !retirePlayerOwnership()) return false;
    if(terrainInstances_&&!terrainInstances_->clear()){run_.effectsSucceeded=false;return false;}
    roomTerrainLoaded_=false;
    if (!run_.advanceAfterLevelClear()) return false;
    // Do not reconstruct this large runtime or copy RunSession onto the task
    // stack: its run globals remain in place while old world ownership retires.
    run_.shops.reset();
    run_.flashEffects.clear();
    run_.smokeEffects.clear();
    run_.bloodEffects.clear();
    run_.clearCorpseExpiry();
    run_.clearDamageNumbers();
    run_.timeFields.clear();
    run_.sideCameraLockers.clear();
    player_.enterRoom(input.fire, 1201 - 176);
    emitter_.reset(run_.upgrades.levels[12] != 0);
    run_.ammoHud.previousShots = run_.ammoHud.previousRecharges = 0;
    currencyAccount_.playerCreated();
    death_ = {};
    invulnerability_ = 0;
    enemyCount_ = breakableCount_ = blocksDestroyed_ = 0;
    bulletSlotEnd_ = gemSlotEnd_ = 0;
    for (unsigned i = 0; i < bulletCapacity; ++i) bullets_[i].active = false;
    for (unsigned i = 0; i < enemyCapacity; ++i) {
        enemies_[i].active = false;
        enemies_[i].identitySlot = 0xffffu;
        enemyIdentitySlots_[i] = false;
    }
    for (unsigned i = 0; i < gemCapacity; ++i) gems_[i].active = false;
    generationStep_ = nullptr; generationContext_ = nullptr;
    generationSucceeded_ = true;
    solidResolver_ = nullptr; solidResolverContext_ = nullptr; wallContactResolver_ = nullptr;
    segmentResolver_ = nullptr;
    maskResolver_ = nullptr;
    landingResolver_ = nullptr; landingResolverContext_ = nullptr;
    breakableIndexReady_ = false;
    followCamera_ = true;
    cameraMotion_.reset(240, 176);
    cameraY_ = cameraPixelY_ = cameraPixelX_ = activationY_ = 0;
    ringOrigin_ = initialRingOrigin % ringHeight;
    return birthPlayerOwnership();
}

GameplayWorld::GameplayWorld(unsigned initialRingOrigin,
                                   bool startsGrounded,
                                   int landingOffset,
                                   bool followCamera, const EnemyLifecycleBinding& enemyLifecycle,
                                   const BreakableLifecycleBinding& breakableLifecycle,
                                   const PlayerLifecycleBinding& playerLifecycle,
                                   downwell::GameplayTerrainInstances* terrainInstances)
    : enemyLifecycle_(enemyLifecycle), breakableLifecycle_(breakableLifecycle), playerLifecycle_(playerLifecycle), terrainInstances_(terrainInstances), player_(startsGrounded, landingOffset), tick_(0),
      ringOrigin_(initialRingOrigin % ringHeight),
      followCamera_(followCamera), cameraY_(0), cameraPixelY_(0),
      cameraPixelX_(0), activationY_(0),
      enemyCount_(0), enemyRandom_(2),
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
    birthPlayerOwnership();
}

bool GameplayWorld::birthPlayerOwnership() {
    if(!run_.effectsSucceeded||!nextPlayerLifetimeId_) {
        run_.effectsSucceeded=false;return false;
    }
    playerInstance_={240+int(downwell::roundGameMakerQ8(player_.offsetXFixed())),
                     176+int(downwell::roundGameMakerQ8(player_.offsetYFixed())),
                     true,nextPlayerLifetimeId_++};
    if(!playerLifecycle_.birth(playerHandle(),playerInstance_)) {
        run_.effectsSucceeded=false;return false;
    }
    return true;
}

bool GameplayWorld::retirePlayerOwnership() {
    if(!playerInstance_.lifetimeId)return true;
    if(!playerLifecycle_.retire(playerHandle(),playerInstance_)) {
        run_.effectsSucceeded=false;return false;
    }
    playerInstance_.lifetimeId=0;playerInstance_.active=false;return true;
}

bool GameplayWorld::publishPlayerPosition(int x,int y) {
    if(!playerInstance_.lifetimeId||!playerLifecycle_.integerPosition(playerHandle(),playerInstance_,x,y)) {
        run_.effectsSucceeded=false;return false;
    }
    playerInstance_.x=x;playerInstance_.y=y;return true;
}

void GameplayWorld::spawnReward(const EnemyState& enemy, bool attracted)
{
    if(!run_.effectsSucceeded)return;
    unsigned next = downwell::compactCurrencySlots(gems_, gemSlotEnd_);
    unsigned available = gemCapacity - next;
    // Exhausted keys cannot alias a retired lifetime. Bound births before
    // Create consumes RNG, just as for exhausted storage capacity.
    const std::uint32_t identities = nextGemLifetimeId_ ?
        0xffffffffu - nextGemLifetimeId_ + 1u : 0u;
    if (identities < available) available = identities;
    const auto reward=downwell::spawnCurrencyReward(enemy.reward, enemy.x, enemy.y, attracted,
        available, enemyRandom_, [this, &enemy, attracted, &next](const downwell::CurrencyBirth& birth) {
        GemState& gem = gems_[next++];
        gem.lifetimeId = nextGemLifetimeId_++;
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
    if(!reward.complete)run_.effectsSucceeded=false;
}

unsigned GameplayWorld::gemCount() const
{
    unsigned count = 0;
    for (unsigned index = 0; index < gemSlotEnd_; ++index)
        if (gems_[index].active)
            ++count;
    return count;
}

const GameplayWorld::GemState&
GameplayWorld::gemAt(unsigned index) const
{
    return gems_[index < gemCapacity ? index : 0];
}

bool GameplayWorld::publishEnemyTransform(const EnemyState& enemy,downwell::RunnerTransformField field,int value) {
    if(enemyLifecycle_.integerTransform({enemy.identitySlot,enemy.lifetimeId},enemy,field,value,
                                        {0x3f800000u},0xffffffu))return true;
    run_.effectsSucceeded=false;return false;
}

bool GameplayWorld::publishEnemyImage(const EnemyState& enemy,unsigned phase) {
    if(enemyLifecycle_.phaseIndex({enemy.identitySlot,enemy.lifetimeId},enemy,phase))return true;
    run_.effectsSucceeded=false;return false;
}

bool GameplayWorld::publishEnemyActivation(const EnemyState& enemy, bool active) {
    if (enemyLifecycle_.activation({enemy.identitySlot,enemy.lifetimeId},enemy,active)) return true;
    run_.effectsSucceeded=false;
    return false;
}

bool GameplayWorld::retireEnemyOwnership(EnemyState& enemy) {
    if (enemy.identitySlot>=enemyCapacity) return true;
    if (!enemyLifecycle_.retire({enemy.identitySlot,enemy.lifetimeId},enemy)) {
        run_.effectsSucceeded=false;
        return false;
    }
    enemyIdentitySlots_[enemy.identitySlot]=false;
    enemy.identitySlot=0xffffu;
    return true;
}

bool GameplayWorld::clearEnemyOwnership() {
    for(unsigned i=0;i<enemyCount_;++i)
        if(!retireEnemyOwnership(enemies_[i])) return false;
    return true;
}

bool GameplayWorld::configureEnemy(unsigned index, int objectId,
                                      int spriteId, int x, int y,
                                      int speedX, int speedY,
                                      int health, int reward,
                                      int motionState, int auxiliaryState,
                                      int alarm0, int alarm1,
                                      int direction, int moveSpeed,
                                      bool active, bool grounded, bool awake)
{
    if (index >= enemyCapacity || !run_.effectsSucceeded)
        return false;
    if (!nextEnemyLifetimeId_) {
        run_.effectsSucceeded = false;
        return false;
    }
    EnemyState& enemy = enemies_[index];
    unsigned slot=enemy.identitySlot;
    if(index<enemyCount_ && !retireEnemyOwnership(enemy))return false;
    if(index>=enemyCount_ || slot>=enemyCapacity){
        slot=0;
        while(slot<enemyCapacity&&enemyIdentitySlots_[slot])++slot;
    }
    if(slot==enemyCapacity){run_.effectsSucceeded=false;return false;}
    enemyIdentitySlots_[slot]=true;
    enemy.identitySlot=static_cast<unsigned short>(slot);
    enemy.lifetimeId = nextEnemyLifetimeId_++;
    enemy.objectId = objectId;
    enemy.spriteId = spriteId;
    enemy.maskId=-1;
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
    enemy.imageSpeed = downwell::enemyCreateProfile(objectId).animationRate;
    enemy.imageFrame = 0;
    enemy.active = active;
    enemy.destroyRequested = false;
    enemy.awake = awake;
    enemy.grounded = grounded;
    enemy.frogLanded = true;
    enemy.frogPresentation = {};
    enemy.activationPending = !active;
    if (enemyCount_ <= index)
        enemyCount_ = index + 1;
    if(!enemyLifecycle_.birth({enemy.identitySlot,enemy.lifetimeId},enemy)) {
        run_.effectsSucceeded=false;
        return false;
    }
    return true;
}

const GameplayWorld::EnemyState&
GameplayWorld::enemyAt(unsigned index) const
{
    return enemies_[index < enemyCount_ ? index : 0];
}

unsigned GameplayWorld::bulletCount() const
{
    unsigned count = 0;
    unsigned index;
    for (index = 0; index < bulletSlotEnd_; ++index)
        if (bullets_[index].active)
            ++count;
    return count;
}

const GameplayWorld::BulletState&
GameplayWorld::bulletAt(unsigned index) const
{
    return bullets_[index < bulletCapacity ? index : 0];
}

int GameplayWorld::bulletX() const
{
    unsigned index;
    for (index = 0; index < bulletSlotEnd_; ++index)
        if (bullets_[index].active)
            return bullets_[index].x;
    return 0;
}

int GameplayWorld::bulletY() const
{
    unsigned index;
    for (index = 0; index < bulletSlotEnd_; ++index)
        if (bullets_[index].active)
            return bullets_[index].y;
    return 0;
}

downwell::LaserSightRay GameplayWorld::traceLaserSight(
    downwell::LaserSightRay::Coordinate x, downwell::LaserSightRay::Coordinate y,
    downwell::LaserSightRay::Coordinate bottom, std::uint32_t speedTurns,
    std::uint32_t aimTurns) const {
    using downwell::ProjectileSegment;
    const auto local = [](const ProjectileSegment& path) {
        auto x = path.x, y = path.y;
        x.pixel -= 240; y.pixel -= 176;
        return ProjectileSegment(x, y, path.dx, path.dy);
    };
    return downwell::LaserSightRay::trace(x, y, bottom, speedTurns,
        [&](const ProjectileSegment& roomPath) {
            if (run_.shops.rayWallIntersects(roomPath)) return true;
            const auto path = local(roomPath);
            for (unsigned i = 0; i < breakableCount_; ++i) {
                const auto& block = breakables_[i];
                if (block.active && path.intersects(block.x-8, block.x+8, block.y-8, block.y+8))
                    return true;
            }
            if (segmentResolver_) return segmentResolver_(solidResolverContext_, path);
            if (!solidResolver_) return false;
            const int left = path.x.pixel < path.endX.pixel ? path.x.pixel : path.endX.pixel;
            const int right = path.x.pixel > path.endX.pixel ? path.x.pixel : path.endX.pixel;
            const int top = path.y.pixel < path.endY.pixel ? path.y.pixel : path.endY.pixel;
            const int bottom = path.y.pixel > path.endY.pixel ? path.y.pixel : path.endY.pixel;
            for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x)
                if (solidResolver_(solidResolverContext_, x, y) && path.intersects(x,x+1,y,y+1))
                    return true;
            return false;
        }, [&](const ProjectileSegment& roomPath) {
            const auto path = local(roomPath);
            for (unsigned i = 0; i < enemyCount_; ++i) {
                const auto& enemy = enemies_[i];
                if (!enemy.active) continue;
                const downwell::ContactActor contact{enemy.x*256L, enemy.y*256L,
                    enemy.objectId == 117 ? enemy.motionState : enemy.facing,
                    enemy.imageScaleY(), downwell::enemyContactMask(enemy.objectId, enemy.spriteId,enemy.maskId)};
                if (path.intersects(contact)) return true;
            }
            return false;
        }, aimTurns);
}

void GameplayWorld::configureBreakables(const int* x, const int* y,
                                            unsigned count)
{
    if(!run_.effectsSucceeded)return;
    if (x == 0 || y == 0)
        return;
    if (count > breakableCapacity) {run_.effectsSucceeded=false;return;}
    if(!clearBreakableOwnership())return;
    breakableCount_ = 0;
    breakableIndexReady_ = true;
    for (auto& cell : breakableIndex_) cell = 0;
    blocksDestroyed_ = 0;
    if(!appendBreakables(x, y, count))run_.effectsSucceeded=false;
}

bool GameplayWorld::inactivateBreakable(unsigned index) {
    auto& box=breakables_[index];
    if(!breakableLifecycle_.activation({index,box.lifetimeId},box,false)) {
        run_.effectsSucceeded=false;return false;
    }
    box.active=false;return true;
}

bool GameplayWorld::retireBreakable(unsigned index) {
    auto& box=breakables_[index];
    if(!box.lifetimeId)return true;
    if(!breakableLifecycle_.retire({index,box.lifetimeId},box)) {
        run_.effectsSucceeded=false;return false;
    }
    box.lifetimeId=0;box.active=false;return true;
}

bool GameplayWorld::markEnemyDestroyed(downwell::InstanceHandle handle) {
    auto* enemy=const_cast<EnemyState*>(resolveEnemyLifetime(handle));
    if(!run_.effectsSucceeded||!enemy){run_.effectsSucceeded=false;return false;}
    if(enemy->destroyRequested)return true;
    if(!enemyLifecycle_.markDestroyed(handle,*enemy)){run_.effectsSucceeded=false;return false;}
    enemy->destroyRequested=true;return true;
}

bool GameplayWorld::markBreakableDestroyed(downwell::InstanceHandle handle) {
    auto* box=const_cast<Breakable*>(findBreakable(handle));
    if(!run_.effectsSucceeded||!box){run_.effectsSucceeded=false;return false;}
    if(box->destroyRequested)return true;
    if(!breakableLifecycle_.markDestroyed(handle,*box)){run_.effectsSucceeded=false;return false;}
    box->destroyRequested=true;return true;
}

bool GameplayWorld::retireMarkedEnemy(downwell::InstanceHandle handle) {
    auto* enemy=const_cast<EnemyState*>(resolveEnemyLifetime(handle));
    if(!run_.effectsSucceeded||!enemy||!enemy->destroyRequested){run_.effectsSucceeded=false;return false;}
    const unsigned index=unsigned(enemy-enemies_);
    if(!retireEnemyOwnership(*enemy))return false;
    for(unsigned next=index+1;next<enemyCount_;++next)enemies_[next-1]=enemies_[next];
    --enemyCount_;
    enemies_[enemyCount_].identitySlot=0xffffu;
    enemies_[enemyCount_].lifetimeId=0;
    enemies_[enemyCount_].active=false;
    return true;
}

bool GameplayWorld::retireMarkedBreakable(downwell::InstanceHandle handle) {
    const auto* box=findBreakable(handle);
    if(!run_.effectsSucceeded||!box||!box->destroyRequested){run_.effectsSucceeded=false;return false;}
    return retireBreakable(handle.slot);
}

bool GameplayWorld::clearBreakableOwnership() {
    for(unsigned i=0;i<breakableCount_;++i)
        if(!retireBreakable(i))return false;
    return true;
}

bool GameplayWorld::appendBreakables(const int* x, const int* y, unsigned count)
{
    if(!run_.effectsSucceeded)return false;
    if (count > breakableCapacity - breakableCount_ || (count && (!x || !y)))
        return false;
    if(count&&(!nextBreakableLifetimeId_||count-1>0xffffffffu-nextBreakableLifetimeId_))return false;
    for (unsigned source = 0; source < count; ++source) {
        const unsigned index = breakableCount_++;
        breakables_[index].x = x[source];
        breakables_[index].y = y[source];
        breakables_[index].active = true;
        breakables_[index].destroyRequested = false;
        breakables_[index].lifetimeId = nextBreakableLifetimeId_++;
        if(!breakableLifecycle_.birth({index,breakables_[index].lifetimeId},breakables_[index])) {
            run_.effectsSucceeded=false;return false;
        }
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

bool GameplayWorld::breakableActiveAt(int x, int y) const
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

int GameplayWorld::indexedBreakable(int x, int y) const {
    x += 512 + 8;
    y += 8;
    if (x < 0 || x >= 1024 || y < 0 || y >= 8192) return -1;
    return (int)breakableIndex_[(y >> 4) * 64 + (x >> 4)] - 1;
}

void GameplayWorld::applyFrogAlarm(EnemyState& enemy, const downwell::FrogAlarmEffect& effect)
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

bool GameplayWorld::captureEnemyStepOrder(){
    enemyStepOrder_.clear();
    if(enemyOrderCapture_){
        if(!enemyOrderCapture_(enemyOrderContext_,enemyStepOrder_))return run_.effectsSucceeded=false;
    }else{
        // Unbound component worlds have no external registration owner.
        for(unsigned i=0;i<enemyCount_;++i){const auto& e=enemies_[i];
            if(e.active&&!enemyStepOrder_.append(e.identitySlot,e.lifetimeId,e.objectId))return run_.effectsSucceeded=false;
        }
    }
    enemyStepOrder_.sort();return true;
}

void GameplayWorld::advanceEnemyAlarms()
{
    DW_PROFILE_PHASE(profileEnemyAlarms);
    if(!captureEnemyStepOrder())return;
    for (unsigned enemyIndex = 0; enemyIndex < enemyCount_; ++enemyIndex) {
        EnemyState& enemy = enemies_[enemyIndex];
        if (!enemy.active)
            continue;
        if (enemy.objectId == 113) enemy.frogHit.advanceFrame();
        if (enemy.objectId == 117) enemy.snailHit.advanceFrame();
        if (enemy.objectId == 116) enemy.batHit.advanceFrame();
    }
    // Match the runner's global alarm-index order. Dispatch each event before
    // counting down later slots: frog Alarm 0 writes Alarm 1, for example.
    for (unsigned alarm = 0; alarm < 2; ++alarm) {
      for (unsigned i=0;i<enemyStepOrder_.size();++i) {
        auto* live=const_cast<EnemyState*>(resolveEnemyLifetime(enemyStepOrder_.handle(i)));
        if(!live)continue;
        EnemyState& enemy = *live;
        if (!enemy.active) continue;
        const unsigned events = downwell::enemyAlarmEvents(enemy.objectId);
        const bool alarm0Fired = alarm == 0 && downwell::AlarmBank::advanceValue(enemy.alarm0, (events & 1) != 0);
        const bool alarm1Fired = alarm == 1 && downwell::AlarmBank::advanceValue(enemy.alarm1, (events & 2) != 0);

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
            updateCrawler(enemy, true, cameraPixelX_, viewCameraYQ8());
            enemy.alarm0 = 2;
        }
      }
    }
}

void GameplayWorld::updateCrawler(EnemyState& enemy, bool alarm, int cameraX, long cameraYQ8)
{
    const long cx = cameraX * 4096L;
    const long cy = cameraYQ8 * 16;
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

GameplayFrame GameplayWorld::step(std::uint8_t packedInput)
{
    if (run_.levelClearActive || !run_.effectsSucceeded || !generationSucceeded_) return {}; // Result owner uses stepLevelClear.
    // Only valid unit-sequence state is admitted through the effect owner.
    run_.advanceFlashAnimation();
    run_.advanceSmokeAnimation();
    run_.advanceCorpseExpiryAnimation();
    if(!run_.effectsSucceeded)return {};
    // Runner advances existing instances' animation before Begin Step/alarms.
    // In particular, an Alarm/Step frame reset must not then be advanced again.
    for (unsigned i=0;i<enemyCount_;++i) if (enemies_[i].active) {
        downwell::advanceEnemyImageQ8(enemies_[i].imagePhase,enemies_[i].imageRemainder,
                                     enemies_[i].imageSpeed);
        if(!publishEnemyImage(enemies_[i],enemies_[i].imagePhase))return {};
    }
    DW_PROFILE_PHASE(profilePlayer);
    // Wrappers own a runtime pointer; rebind after any host-side runtime copy.
    if (solidResolver_) configureSolidResolver(solidResolver_, solidResolverContext_, segmentResolver_, maskResolver_,wallContactResolver_);
    if (landingResolver_) configureLandingResolver(landingResolver_, landingResolverContext_);
    // camera_get_view_y reads the runner's published cache, not camMain.y.
    // Normal following refreshes before Draw; post-Step refresh is dirty-only.
    const long generationCameraY = viewCameraYQ8();
    const int publishedCameraX = cameraPixelX_;
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
    // objPlayer_n Begin Step advances activatey once, before gravity/motion.
    // Native camera offsets map to world view bottom 318 + cameraY.
    const auto activation = downwell::advanceMainRoomActivationChecked(activationY_,
        318L * 256L + viewCameraYQ8(), enemyCount_,
        [this](unsigned i) {
            const auto& enemy = enemies_[i];
            if (enemy.active || !enemy.activationPending)
                return downwell::ActivationInstance{enemy.objectId, {}, enemy.active, enemy.activationPending};
            const auto bounds = downwell::contactWorldBounds({(enemy.x + 240L) * 256, (enemy.y + 176L) * 256,
                enemy.objectId == 117 ? enemy.motionState : enemy.facing,
                enemy.imageScaleY(), downwell::enemyContactMask(enemy.objectId, enemy.spriteId,enemy.maskId)});
            return downwell::ActivationInstance{enemy.objectId, bounds,
                                               enemy.active, enemy.activationPending};
        },
        [this](unsigned i) {
            if (!publishEnemyActivation(enemies_[i],true)) return false;
            enemies_[i].active = true;
            enemies_[i].activationPending = false;
            return true;
        });
    if(!activation.succeeded){run_.effectsSucceeded=false;return {};}
    // scrPlayerShootN captures instance x/y before this Begin Step moves xx/yy.
    const int muzzleX = (int)downwell::roundGameMakerQ8(player_.offsetXFixed());
    const int muzzleY = (int)downwell::roundGameMakerQ8(player_.offsetYFixed()) + 4;
    struct HeadBreakContext {
        GameplayWorld* runtime;
        int index;
    } headBreak = {this, -1};
    player_.configureBreakBoxResolver([](void* opaque, long x, long y, bool& found) {
        auto& context = *static_cast<HeadBreakContext*>(opaque);
        auto& runtime = *context.runtime;
        downwell::PlayerBoxContact::Bounds bounds;
        if(!downwell::PlayerBoxContact::prepare(x,y,240,176,bounds))return false;
        for (unsigned i = 0; i < runtime.breakableCount_; ++i) {
            Breakable& box = runtime.breakables_[i];
            if(!box.active)continue;
            bool hit=false;
            if(!downwell::PlayerBoxContact::testPrepared(bounds,box.x+240,box.y+176,hit))return false;
            if (hit) {
                if(!runtime.inactivateBreakable(i))return false;
                ++runtime.blocksDestroyed_;
                context.index = (int)i;
                found=true;
                return true;
            }
        }
        found=false;return true;
    }, &headBreak);
    PlayerMotionDelta motion = downwell::stepGameplayPlayer(player_, death_, run_,
        currencyAccount_, health_, enemyRandom_, input, 240, 176,
        cameraMotion_.pixelX() - 80, cameraMotion_.pixelY() - 142,false,
        [this](int x,int y){return publishPlayerPosition(x,y);},
        [this](int value){
            if(!playerLifecycle_.integerTransform(playerHandle(),playerInstance_,downwell::RunnerTransformField::scaleX,
                    value,{0x3f800000u},0xffffffu))return false;
            playerInstance_.scaleX=value;return true;
        },[this](downwell::RunSession& run,downwell::MotionBinary64 x,downwell::MotionBinary64 y,int facing,bool& hit){
            if(!playerLifecycle_.hasPlaceMeeting())return run.playerGoalContact(x,y,facing,hit);
            return playerInstance_.lifetimeId&&playerLifecycle_.placeMeeting(playerHandle(),playerInstance_,255,
                downwell::MotionBinary64::fromInteger(playerInstance_.x),
                downwell::MotionBinary64::fromInteger(playerInstance_.y),hit);
        });
    player_.configureBreakBoxResolver(nullptr, nullptr);
    if(!run_.effectsSucceeded)return {};
    if (headBreak.index >= 0) {
        const Breakable& box = breakables_[headBreak.index];
        destroyedBlockX[destroyedBlockCount] = box.x;
        destroyedBlockY[destroyedBlockCount++] = box.y;
    }
    if (player_.shots() != oldShots) {
        const int muzzleOffset = emitter_.nextOffset();
        const unsigned index = downwell::compactInstanceSlots(bullets_, bulletSlotEnd_);
        bulletSlotEnd_ = index;
        if (index >= bulletCapacity || !nextBulletLifetimeId_) {
            run_.effectsSucceeded = false;
            return {};
        }
        BulletState& bullet = bullets_[index];
        bullet.lifetimeId=nextBulletLifetimeId_++;
        bullet.active = true;
        bulletSlotEnd_ = index + 1;
        bullet.x = muzzleX + muzzleOffset;
        bullet.y = muzzleY;
        bullet.gemHigh = run_.gemHigh;
        bullet.animation = {};
        bullet.trajectory.reset(bullet.x, bullet.y, run_.upgrades.levels[12] != 0);
        bulletChanged = true;
    }

    // Alarms follow Begin Step movement, before builder Step/Create events.
    // Runner 0x50dc89 reconciles pending room membership after the complete
    // Begin Step dispatcher, not after each individual activation request.
    if(!reconcilePhase(PhaseBoundary::beginStep))return {};
    const auto damageNumberAlarmPhase=run_.damageNumbers.beginPhase();
    // Object 221 Alarm 0 only destroys itself: no RNG or cross-family effects.
    run_.advanceDamageNumberAlarm(damageNumberAlarmPhase,0);
    advanceEnemyAlarms();
    if(!reconcilePhase(PhaseBoundary::alarm))return {};
    const auto smokeStepPhase=run_.smokeEffects.beginPhase();
    const auto damageNumberStepPhase=run_.damageNumbers.beginPhase();
    DW_PROFILE_PHASE(profilePlayer);
    int scrollDelta = 0;
    int cameraXDelta = 0;
    if (followCamera_) {
        const int oldCameraPixelX = cameraPixelX_;
        DW_PROFILE_SCOPE(profileCamera);
        const int oldCameraPixelY = cameraPixelY_;
        cameraMotion_.step(240L * 256 + player_.offsetXFixed(),
                           176L * 256 + player_.offsetYFixed(), dead(),
                           run_.sideCameraLockers.overlaps(cameraMotion_));
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

    if(!captureEnemyStepOrder())return {};
    if (generationStep_ && generationSucceeded_) {
        DW_PROFILE_SCOPE(profileGeneration);
        const long bottom = generationCameraY + 318L * 256L;
        const int bottomCeiling = bottom >= 0 ? (int)((bottom + 255) / 256)
                                              : -(int)((-bottom) / 256);
        generationSucceeded_ = generationStep_(generationContext_, *this,
                                               enemyRandom_, bottomCeiling);
        if (!generationSucceeded_) return {};
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
            const long cameraCentreQ12 = 176L * 4096 + generationCameraY * 16;
            bool alive=false;
            if(!downwell::stepMachineGunProjectileChecked(bullet, enemyRandom_,
                [this, &bullet](const downwell::ProjectileSegment& path) {
                    for (unsigned enemyIndex = 0; enemyIndex < enemyCount_; ++enemyIndex) {
                        EnemyState& enemy = enemies_[enemyIndex];
                        const downwell::ContactActor contact{
                            enemy.x * 256L, enemy.y * 256L,
                            enemy.objectId == 117 ? enemy.motionState : enemy.facing,
                            enemy.imageScaleY(),
                            downwell::enemyContactMask(enemy.objectId, enemy.spriteId,enemy.maskId)};
                        if (!enemy.active || enemy.health <= 0 ||
                            !path.intersects(contact))
                            continue;
                        const auto impact = downwell::applyStandardBulletImpact(
                            enemy, run_.kills, enemyRandom_,
                            [this, &enemy] { run_.combo.state.enemyDefeated(); spawnReward(enemy, false); return run_.effectsSucceeded; },
                            bullet.gemHigh, bullet.trajectory.direction,
                            [this](const EnemyState& record){return publishEnemyActivation(record,false);});
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
                        const unsigned columns = ((xs[0]+8) >> 4) == ((xs[1]+8) >> 4) ? 1 : 2;
                        const unsigned rows = ((ys[0]+8) >> 4) == ((ys[1]+8) >> 4) ? 1 : 2;
                        for (unsigned column = 0; column < columns; ++column)
                          for (unsigned row = 0; row < rows; ++row) {
                            const int next = indexedBreakable(xs[column], ys[row]);
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
                        // Indexed selection already proved this exact hit.
                        if (block.active && (breakableIndexReady_ || path.intersects(block.x - 8, block.x + 8,
                                                           block.y - 8, block.y + 8))) {
                            if(!inactivateBreakable(blockIndex))return true;
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
                        if (segmentResolver_) return segmentResolver_(solidResolverContext_, path);
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
                },alive,[this]{return run_.effectsSucceeded;},176,cameraCentreQ12-142L*4096))return {};
            bullet.active=alive;
        }
    }


    DW_PROFILE_PHASE(profileEnemyMotion);
    for (unsigned i=0;i<enemyStepOrder_.size();++i) {
        auto* live=const_cast<EnemyState*>(resolveEnemyLifetime(enemyStepOrder_.handle(i)));
        if(!live)continue;
        EnemyState& enemy = *live;
        if (!enemy.active)
            continue;
        if (enemy.objectId == 113)
            enemy.imageSpeed = enemy.frogPresentation.beginStepRate();
        if (!enemy.awake && enemy.y - cameraPixelY_ <= 254)
            enemy.awake = true;
        if ((!enemy.awake && enemy.objectId != 113 && enemy.objectId != 111 && enemy.objectId != 112 && enemy.objectId != 116) || solidResolver_ == 0)
            continue;
        if (enemy.objectId == 111 || enemy.objectId == 112) {
            const bool turtle = enemy.objectId == 112;
            const long cx = publishedCameraX * 4096L;
            const long cy = generationCameraY * 16;
            const bool visible = downwell::CrawlerMotion::inView(enemy.x, enemy.y,
                cx - 80L * 4096, cy - 142L * 4096, cx + 80L * 4096, cy + 142L * 4096,
                turtle ? 64 : 32);
            if(!enemy.walker.step(turtle, downwell::groundTurtleWalking(enemy.auxiliaryState), visible,
                [this, &enemy, turtle](long x, long y) {
                    return enemy.walker.meetsTiles(x, y, turtle, [this](int px, int py) {
                        return solidResolver_(solidResolverContext_, px, py);
                    });
                },[this,&enemy](int sprite,int scale){
                    enemy.grounded=enemy.walker.grounded;
                    if(!enemyLifecycle_.sprite({enemy.identitySlot,enemy.lifetimeId},enemy,sprite)){
                        run_.effectsSucceeded=false;return false;
                    }
                    enemy.spriteId=sprite;
                    if(scale){
                        if(!publishEnemyTransform(enemy,downwell::RunnerTransformField::scaleX,scale))return false;
                        enemy.facing=scale;
                    }
                    return true;
                }))return {};
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
            bat.collisionMask=downwell::enemyContactMask(enemy.objectId,enemy.spriteId,enemy.maskId);
            const auto effects = downwell::stepBatBehavior(bat, enemy.batHit, enemy.batActivity,
                enemy.x, enemy.y, bat.collisionMask, player_.frameStartXFixed(),
                player_.frameStartYFixed(), enemyRandom_,
                [this](int x, int y) {
                    return solidResolver_(solidResolverContext_, x, y);
                },[this,&enemy](int scale){
                    if(!publishEnemyTransform(enemy,downwell::RunnerTransformField::scaleX,scale))return false;
                    enemy.facing=scale;return true;
                });
            enemy.preciseX = bat.x;
            enemy.preciseY = bat.y;
            enemy.speedX = bat.speedX;
            enemy.speedY = bat.speedY;
            enemy.direction = bat.direction;
            enemy.batDirectionFraction = bat.directionFraction;
            enemy.facing = bat.facing;
            enemy.moveSpeed = (bat.speedFortieths * 256 + 20) / 40;
            if(!effects.succeeded)return {};
            if (effects.hit) {
                enemy.alarm0 = 3;
                if(!publishEnemyImage(enemy,0))return {};
                enemy.imagePhase = 0;
                enemy.imageRemainder = 0;
            }
        } else if (enemy.objectId == 113) {
            const bool integrate = downwell::frogMotionVisible(enemy.x * 256L, enemy.y * 256L,
                (publishedCameraX - 80) * 256L, generationCameraY - 142 * 256L,
                (publishedCameraX + 80) * 256L, generationCameraY + 142 * 256L);
            downwell::FrogVerticalMotion vertical{
                enemy.preciseY * 5 + enemy.gravityPositionRemainder,
                enemy.speedY * 5 + enemy.gravityRemainder, enemy.grounded};
            const auto effects = downwell::stepFrogMotion(enemy.frogHorizontal, vertical,
                enemy.frogHit, integrate, [this](int x, int y) {
                    return solidResolver_(solidResolverContext_, x, y);
                },[this,&enemy]{
                enemy.auxiliaryState = 0;
                enemy.alarm0 = 0;
                enemy.alarm1 = 60;
                enemy.speedX = downwell::FrogHorizontalMotion::toQ8(enemy.frogHorizontal.velocity);
                if(!publishEnemyImage(enemy,0))return false;
                enemy.imagePhase = 0;
                enemy.imageRemainder = 0;
                return true;
            },[this,&enemy](int scale){
                if(!publishEnemyTransform(enemy,downwell::RunnerTransformField::scaleX,scale))return false;
                enemy.facing=scale;return true;
            },[&enemy](bool grounded){
                if(downwell::frogLandingEdge(grounded,enemy.frogLanded))enemy.alarm1=60;
            });
            enemy.preciseY = downwell::FrogVerticalMotion::floorDivide(vertical.position, 5);
            enemy.gravityPositionRemainder = static_cast<unsigned char>(vertical.position - enemy.preciseY * 5);
            enemy.speedY = downwell::FrogVerticalMotion::floorDivide(vertical.velocity, 5);
            enemy.gravityRemainder = static_cast<unsigned char>(vertical.velocity - enemy.speedY * 5);
            enemy.grounded = vertical.grounded;
            enemy.preciseX = downwell::FrogHorizontalMotion::toQ8(enemy.frogHorizontal.position);
            enemy.speedX = downwell::FrogHorizontalMotion::toQ8(enemy.frogHorizontal.velocity);
            if(!effects.succeeded)return {};
        } else if (enemy.objectId == 117) {
            const int hitAlarm = enemy.snailHit.consume(enemyRandom_);
            if (hitAlarm >= 0) {
                if(!publishEnemyImage(enemy,0))return {};
                enemy.imagePhase = 0; enemy.imageRemainder = 0;
                enemy.alarm0 = 0; enemy.alarm1 = hitAlarm;
            }
            const bool alive = enemy.snail.step([this](int x, int y) {
                return solidResolver_(solidResolverContext_, x, y);
            },false,[this,&enemy](downwell::RunnerTransformField field,int value){return publishEnemyTransform(enemy,field,value);});
            if(!run_.effectsSucceeded)return {};
            if (!alive && !publishEnemyActivation(enemy,false)) return {};
            enemy.active = alive;
            if (!enemy.active)
                continue;
            enemy.preciseX = enemy.snail.x / 5;
            enemy.preciseY = enemy.snail.y / 5;
            enemy.speedY = enemy.snail.vy / 5;
            enemy.motionState = enemy.facing = enemy.snail.facing;
        } else if (enemy.objectId == 119) {
            updateCrawler(enemy, false, publishedCameraX, generationCameraY);
        }
        // World origins (240,176) are even integers, so ties-to-even is
        // unchanged by storing these positions relative to the origin.
        int publishedX = (int)downwell::roundGameMakerQ8(enemy.preciseX);
        int publishedY = (int)downwell::roundGameMakerQ8(enemy.preciseY);
        if (enemy.objectId == 111 || enemy.objectId == 112) {
            publishedX = enemy.walker.publishedX;
            publishedY = enemy.walker.publishedY;
        }
        if (enemy.objectId == 119) {
            publishedX = enemy.crawler.pixelX();
            publishedY = enemy.crawler.pixelY();
        }
        if (enemy.objectId == 117) {
            publishedX = static_cast<int>(downwell::FixedCollisionMath::round(enemy.snail.x) / 1280);
            publishedY = static_cast<int>(downwell::FixedCollisionMath::round(enemy.snail.y) / 1280);
        }
        if(!enemyLifecycle_.integerPosition({enemy.identitySlot,enemy.lifetimeId},enemy,
                                           publishedX+240,publishedY+176)) {
            run_.effectsSucceeded=false;return {};
        }
        enemy.x=publishedX;enemy.y=publishedY;
        if(enemy.objectId==119 &&
           !publishEnemyTransform(enemy,downwell::RunnerTransformField::angle,enemy.crawler.imageDirection*90))return {};
        if (enemy.objectId!=116 && enemy.objectId!=113 && enemy.speedX != 0)
            enemy.facing = enemy.speedX < 0 ? -1 : 1;
        const auto nextSprite = downwell::finishEnemyPresentation(
            {enemy.objectId, enemy.auxiliaryState, enemy.grounded, enemy.speedY < 0,
             enemy.frogHit.remaining != 0, enemy.snailHit.stunned},
            enemy.frogPresentation, enemy.batActivity, enemy.batHit,
            enemy.imagePhase, enemy.imageRemainder);
        if(enemy.objectId!=111 && enemy.objectId!=112 &&
           !enemyLifecycle_.sprite({enemy.identitySlot,enemy.lifetimeId},enemy,nextSprite)) {
            run_.effectsSucceeded=false;return {};
        }
        enemy.spriteId=nextSprite;
        if(enemy.objectId==116 && !downwell::finishBatFields(enemy.batActivity,nextSprite,
            [this,&enemy](int mask){
                if(!enemyLifecycle_.mask({enemy.identitySlot,enemy.lifetimeId},enemy,mask)){
                    run_.effectsSucceeded=false;return false;
                }
                enemy.maskId=mask;return true;
            },[this,&enemy](int scale){
                if(!publishEnemyTransform(enemy,downwell::RunnerTransformField::scaleX,scale))return false;
                enemy.facing=scale;return true;
            },[&enemy]{enemy.direction=270;enemy.batDirectionFraction=0;enemy.moveSpeed=512;}))return {};
        if(!publishEnemyImage(enemy,enemy.imagePhase))return {};
        enemy.imageFrame = enemy.imagePhase >> 8;
    }

    DW_PROFILE_PHASE(profileEnemyContact);
    run_.stepSmoke(smokeStepPhase,enemyRandom_);
    run_.stepDamageNumbers(damageNumberStepPhase);
    run_.collideSmokeWalls([&](const downwell::ContactBounds& bounds,auto emit){
        unsigned count=0;
        if(wallContactResolver_ && !wallContactResolver_(solidResolverContext_,bounds,count))return false;
        for(unsigned i=0;i<count;++i)if(!emit())return false;
        return true;
    });
    if(!run_.effectsSucceeded)return {};
    if (!downwell::stepEnemyContacts(enemyCount_, player_, health_, invulnerability_,
        run_.kills, run_.combo.state, run_.upgrades.levels[7] != 0,
        [this](unsigned i) -> EnemyState& { return enemies_[i]; },
        [](const EnemyState& enemy) {
            return downwell::EnemyContactLocation{{
            enemy.preciseX, enemy.preciseY,
            enemy.objectId == 117 ? enemy.motionState : enemy.facing,
            enemy.imageScaleY(),
            downwell::enemyContactMask(enemy.objectId, enemy.spriteId,enemy.maskId)}, enemy.x, enemy.y};
        },
        [this](EnemyState& enemy) { spawnReward(enemy, true); return run_.effectsSucceeded; },
        [](downwell::EnemyContact) {},
        [this](int x,int y) {
            return run_.contactDamageNumber(player_.frameStartXFixed(),player_.frameStartYFixed(),
                x,y,240,176);
        },[this](const EnemyState& enemy){return publishEnemyActivation(enemy,false);})) return {};

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
    run_.exitDither.step(34 + downwell::floorQ8Coordinate(generationCameraY), 284);
    run_.collideLevelExit(240 + downwell::roundGameMakerQ8(player_.offsetXFixed()),
        176 + downwell::roundGameMakerQ8(player_.offsetYFixed()), death_.active);
    // Moving the followed instance does not dirty the camera cache. End Step
    // still reads the previous Draw's view, including after a Collision snap.
    const long viewTop = 34L * 4096 + generationCameraY * 16;
    if(!reconcilePhase(PhaseBoundary::step))return {};
    if (followCamera_ && run_.sideCameraLockers.count) {
        run_.sideCameraLockers.collide(cameraMotion_);
        const int collisionScroll = cameraMotion_.pixelY() - 176 - cameraPixelY_;
        cameraY_ = (cameraMotion_.y - 176L * 4096) / 16;
        cameraPixelY_ += collisionScroll;
        scrollDelta += collisionScroll;
        motion.deltaY -= collisionScroll;
        if (collisionScroll) {
            int origin = (int(ringOrigin_) + collisionScroll) % int(ringHeight);
            if (origin < 0) origin += int(ringHeight);
            ringOrigin_ = unsigned(origin);
        }
    }
    if(!reconcilePhase(PhaseBoundary::collision))return {};
    unsigned retiredEnemies = 0;
    const bool enemiesRetired=downwell::retireMainRoomObjectsChecked(enemyCount_, 11, 1, viewTop,
        [this, &retiredEnemies](unsigned i) {
            const auto& enemy = enemies_[i - retiredEnemies];
            return downwell::LifetimeInstance{enemy.objectId, enemy.y + 176, enemy.active};
        },
        [this, &retiredEnemies](unsigned i) {
            const unsigned index = i - retiredEnemies;
            if(!retireEnemyOwnership(enemies_[index]))return false;
            for (unsigned next = index + 1; next < enemyCount_; ++next)
                enemies_[next - 1] = enemies_[next];
            --enemyCount_;
            enemies_[enemyCount_].identitySlot=0xffffu;
            ++retiredEnemies;
            return true;
        });
    if(!enemiesRetired){run_.effectsSucceeded=false;return {};}
    const bool boxesRetired=downwell::retireMainRoomObjectsChecked(breakableCount_, 11, 1, viewTop,
        [this](unsigned i) {
            const auto& box = breakables_[i];
            return downwell::LifetimeInstance{92, box.y + 176, box.active};
        },
        [this](unsigned i) { return retireBreakable(i); });
    if(!boxesRetired){run_.effectsSucceeded=false;return {};}

#if !defined(DOWNWELL_PLATFORM_AMIGA) || defined(DOWNWELL_REPLAY_VERIFY) || defined(DOWNWELL_SCENE_VERIFY)
    const bool firstBulletActive = bulletActive();
    checkpoint_ = downwell::portCheckpointDigest(
        tick_, packedInput, player_, ringOrigin_, firstBulletActive,
        bulletX(), bulletY());
    if (run_.sideCameraLockers.count || cameraMotion_.sideLocked) {
        downwell::StateDigest extended;
        extended.addU64(checkpoint_);
        extended.addU32(0x534c4f43);
        extended.addU64(sideCameraDigest());
        checkpoint_ = extended.value();
    }
    trajectory_.addU64(checkpoint_);
#endif

    run_.shops.endStep(34 + downwell::floorQ8Coordinate(generationCameraY));
    GameplayFrame frame = {};
    run_.gameplayPresentationStep(enemyRandom_);
    if(!reconcilePhase(PhaseBoundary::endStep))return {};
    frame.playerMotion = motion;
    DW_PROFILE_PHASE(profileHudState);
    const bool ammoMotionChanged = run_.ammoHud.step(player_.shots(), player_.recharges());
    run_.ammoGauge.step(player_.ammo(), player_.ammoCapacity());
    frame.ammoChanged = player_.ammo() != oldAmmo || player_.ammoCapacity() != oldAmmoCapacity || ammoMotionChanged || run_.ammoGauge.visuallyChanged();
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
