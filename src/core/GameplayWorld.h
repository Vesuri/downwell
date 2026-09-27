#ifndef DOWNWELL_CORE_GAMEPLAY_WORLD_H
#define DOWNWELL_CORE_GAMEPLAY_WORLD_H

#include "PlayerMotion.h"
#include "Profile.h"
#include "core/GameMakerRandom.h"
#include "core/StateDigest.h"
#include "core/GameplayLimits.h"
#include "core/GemMotion.h"
#include "core/ProjectileAnimation.h"
#include "core/MachineGunTrajectory.h"
#include "core/ProjectileSegment.h"
#include "core/LaserSightRay.h"
#include "core/MachineGunEmitter.h"
#include "core/CurrencyAccount.h"
#include "core/MainCameraMotion.h"
#include "core/PlayerDeathSequence.h"
#include "core/RunSession.h"
#include "core/AreaOneBuilder.h"
#include "core/EnemyCreateProfile.h"
#include "core/ObjectPack.h"
#include "core/PlayerLifeRules.h"
#include "core/FrogHorizontalMotion.h"
#include "core/FrogHitState.h"
#include "core/FrogPresentation.h"
#include "core/FrogBehavior.h"
#include "core/CrawlerMotion.h"
#include "core/SnailMotion.h"
#include "core/SnailHitState.h"
#include "core/WalkingEnemyMotion.h"
#include "core/BatHitState.h"
#include "core/BatBehavior.h"
#include "core/InstanceHandle.h"
#include "core/InstanceLifecycle.h"

#include <cstdint>

enum {
    gameplayWorldBulletCapacity = downwell::gameplayBulletCapacity,
    gameplayWorldDestroyedCapacity = gameplayWorldBulletCapacity + 1,
    gameplayWorldEnemyCapacity = 64,
    gameplayWorldGemCapacity = downwell::gameplayGemCapacity
};

struct GameplayFrame {
    PlayerMotionDelta playerMotion;
    bool ammoChanged;
    bool healthChanged;
    bool currencyChanged;
    bool streakChanged;
    bool bulletChanged;
    bool scrolled;
    int scrollDelta;
    bool cameraXChanged;
    int cameraXDelta;
    unsigned destroyedBlockCount;
    int blockX[gameplayWorldDestroyedCapacity];
    int blockY[gameplayWorldDestroyedCapacity];
};

// Portable fixed-point world extracted from the native runtime. Terrain and
// generation are callbacks; no renderer, OS, DMA or platform types are owned.
// Ring-origin bookkeeping remains for compatibility during SDL migration.
class GameplayWorld {
public:
    static constexpr unsigned ringHeight = 320;
    static constexpr unsigned bulletCapacity = gameplayWorldBulletCapacity;
    static constexpr unsigned destroyedCapacity = gameplayWorldDestroyedCapacity;
    static constexpr unsigned enemyCapacity = gameplayWorldEnemyCapacity;
    static constexpr unsigned gemCapacity = gameplayWorldGemCapacity;
    typedef bool (*SolidResolver)(void* context, int x, int y);
    typedef bool (*SegmentResolver)(void* context, const downwell::ProjectileSegment& path);
    using WallContactResolver = bool (*)(void*,const downwell::ContactBounds&,unsigned&);
    // Called once after existing alarms and camera Step, before bullets and
    // enemy motion. Receives the shared RNG and ceil(world-space view bottom).
    // A false return stops this frame and future steps until explicitly rebound;
    // completed Create effects are retained, not rolled back.
    using GenerationStep = bool (*)(void*, GameplayWorld&,
                                    downwell::GameMakerRandom&, int);
    void configureGenerationStep(GenerationStep step, void* context) {
        generationStep_ = step;
        generationContext_ = context;
        generationSucceeded_ = true;
    }
    bool generationSucceeded() const { return generationSucceeded_; }
    // Recreate the implemented gameplay world without a large stack temporary.
    // Owner must rebuild terrain and rebind callbacks before stepping again.
    downwell::ResultsDestination restartFromResults(downwell::DeathMenuSequence::Action action,
        std::uint32_t seed, unsigned initialRingOrigin);
    void confirmFirstStage() { if (run_.stage.area == 0) run_.stage.advance(1); }

    struct BulletState {
        // Internal lifetime key, not a Windows instance/resource ID or slot.
        std::uint32_t lifetimeId = 0;
        int x;
        int y;
        bool active;
        bool gemHigh = false;
        downwell::ProjectileAnimation animation;
        downwell::MachineGunTrajectory trajectory;
    };
    struct EnemyState {
        // Family-local lifetime key; never a recovered Windows instance ID.
        std::uint32_t lifetimeId = 0;
        // Stable ownership slot, independent of the compacted traversal index.
        unsigned short identitySlot = 0xffffu;
        int objectId;
        int spriteId;
        int x;
        int y;
        int health;
        int reward;
        int facing;
        long preciseX;
        long preciseY;
        long speedX;
        long speedY;
        unsigned char gravityRemainder; // fifths of one Q8 velocity unit
        unsigned char gravityPositionRemainder; // fifth-Q8 integration residual
        downwell::FrogHorizontalMotion frogHorizontal;
        downwell::FrogHitState frogHit;
        downwell::FrogPresentation frogPresentation;
        downwell::CrawlerMotion crawler;
        downwell::SnailMotion snail;
        downwell::SnailHitState snailHit;
        downwell::BatHitState batHit;
        unsigned short batDirectionFraction;
        downwell::BatActivity batActivity;
        downwell::WalkingEnemyMotion walker;
        int motionState;
        int auxiliaryState;
        int alarm0;
        int alarm1;
        int direction;
        int moveSpeed;
        unsigned imagePhase;
        unsigned imageSpeed; // twentieths of a frame
        unsigned char imageRemainder; // fifths of one Q8 phase unit
        unsigned imageFrame;
        bool active;
        bool awake;
        bool grounded;
        bool frogLanded;
        bool activationPending;
        int imageScaleY() const { return objectId == 117 ? snail.verticalFlip : 1; }
        unsigned drawFrame() const {
            if (objectId == 119)
                return imageFrame + (crawler.imageDirection % 4u) * 256u;
            // Source enmSnail Step updates image_yscale only after nonzero
            // movement. Creation/time-stop/zero speed retain the stored flip.
            // Frame 256+n is host-generated, for both planar and DMA assets.
            return imageFrame + (imageScaleY() < 0 ? 256u : 0u);
        }
    };
    struct GemState {
        std::uint32_t lifetimeId = 0;
        downwell::GemMotion motion;
        int x;
        int y;
        int amount;
        unsigned age;
        long preciseX;
        long preciseY;
        long speedX;
        long speedY;
        bool large;
        bool attracted;
        bool active;
    };

    using EnemyLifecycleBinding = downwell::RecordLifecycleBinding<EnemyState>;
    explicit GameplayWorld(unsigned initialRingOrigin,
                              bool startsGrounded = true,
                              int landingOffset = 0,
                              bool followCamera = false,
                              EnemyLifecycleBinding enemyLifecycle = {});
    GameplayFrame step(std::uint8_t packedInput);
    void initializeEntryInput(bool fireHeld) { player_.initializeEntryInput(fireHeld); }
    void configureLandingProfile(const int* offsets, unsigned count) {
        player_.configureLandingProfile(offsets, count);
    }
    void configureLandingResolver(PlayerMotion::LandingResolver resolver,
                                  void* context) {
        landingResolver_ = resolver;
        landingResolverContext_ = context;
        player_.configureLandingResolver(resolver ? [](void* opaque, int x, int minimumY) {
            const auto& runtime = *static_cast<GameplayWorld*>(opaque);
            const int nearest = runtime.landingResolver_(runtime.landingResolverContext_, x, minimumY);
            return runtime.run_.shops.playerLanding(x + 240, minimumY + 176, nearest + 176) - 176;
        } : static_cast<PlayerMotion::LandingResolver>(nullptr), this);
    }
    void configureBreakables(const int* x, const int* y, unsigned count);
    // Append a newly generated batch without reviving destroyed earlier blocks.
    // Reject capacity failure before changing any state.
    bool appendBreakables(const int* x, const int* y, unsigned count);
    void configureSolidResolver(SolidResolver resolver, void* context, SegmentResolver segment = nullptr,
                                SolidResolver mask = nullptr,WallContactResolver walls = nullptr) {
        solidResolver_ = resolver;
        segmentResolver_ = segment;
        maskResolver_ = mask;
        wallContactResolver_ = walls;
        solidResolverContext_ = context;
        player_.configureSolidResolver(resolver ? [](void* opaque, int x, int y) {
            const auto& runtime = *static_cast<GameplayWorld*>(opaque);
            return (runtime.solidResolver_ && runtime.solidResolver_(runtime.solidResolverContext_, x, y)) ||
                runtime.run_.shops.playerSolidAt(x + 240, y + 176);
        } : static_cast<SolidResolver>(nullptr), this, -236, 236,
        resolver && mask ? [](void* opaque, int x, int y) {
            const auto& runtime = *static_cast<GameplayWorld*>(opaque);
            return runtime.maskResolver_(runtime.solidResolverContext_, x, y) ||
                runtime.run_.shops.playerSolidAt(x+236, y+172) ||
                runtime.run_.shops.playerSolidAt(x+243, y+172) ||
                runtime.run_.shops.playerSolidAt(x+236, y+183) ||
                runtime.run_.shops.playerSolidAt(x+243, y+183);
        } : static_cast<SolidResolver>(nullptr));
    }
    bool configureEnemy(unsigned index, int objectId, int spriteId,
                        int x, int y, int speedX, int speedY,
                        int health, int reward,
                        int motionState = 0, int auxiliaryState = 0,
                        int alarm0 = -1, int alarm1 = -1,
                        int direction = 0, int moveSpeed = 0,
                        bool active = true, bool grounded = false,
                        bool awake = false);
    void configureEnemyRandom(const downwell::GameMakerRandom& random) {
        enemyRandom_ = random;
    }
    // Synchronous builder handoff. Shop/exit Create effects must occur before
    // the next pattern cell draws RNG. The backend owns terrain representation;
    // this world owns breakable and actor registration and coordinate mapping.
    template<class Terrain>
    bool createGeneratedInstance(const downwell::DeferredCreateInstance& command,
                                 const downwell::ObjectPack& objects, Terrain terrain) {
        if (command.objectId == 277) {
            DW_GENERATION_SCOPE(profileGeneratedShop);
            return registerShop(command.x, command.y) != nullptr;
        }
        if (command.objectId == 306) registerLevelExit(command.y);
        if (command.objectId == 82)
            return run_.adoptTimeField(command.x,command.y,command.timeFieldRadiusWord());
        if (command.objectId == 62) return registerSideCameraLocker(command.x, command.y);
        if (!terrain(command)) return false;
        if (command.objectId == 92) {
            DW_GENERATION_SCOPE(profileGeneratedBreakable);
            const int x = command.x - 240, y = command.y - 176;
            return appendBreakables(&x, &y, 1);
        }
        const auto profile = downwell::enemyCreateProfile(command.objectId);
        if (!profile.health) return true;
        DW_GENERATION_SCOPE(profileGeneratedEnemy);
        downwell::PackedObject object;
        return objects.findObject(command.objectId, object) &&
            configureEnemy(enemyCount_, command.objectId, object.spriteId,
                command.x - 240, command.y - 176,
                command.integerVelocityX, command.integerVelocityY, profile.health, profile.reward,
                command.motionState, command.auxiliaryState,
                command.alarms.get(0), command.alarms.get(1),
                command.integerDirection, command.integerMoveSpeed,
                command.active, command.grounded, true);
    }
    std::uint64_t randomDrawCount() const { return enemyRandom_.drawCount(); }
    const downwell::GameMakerRandom& randomState() const { return enemyRandom_; }

    // Pure query of the implemented area-one snapshot, in room coordinates.
    downwell::LaserSightRay traceLaserSight(downwell::LaserSightRay::Coordinate x,
        downwell::LaserSightRay::Coordinate y, downwell::LaserSightRay::Coordinate bottom,
        std::uint32_t speedTurns, std::uint32_t aimTurns = 0) const;
    downwell::LaserSightRay laserSightEndpoint() const {
        using Ray = downwell::LaserSightRay;
        return traceLaserSight(Ray::fromQ8(player_.offsetXFixed(), 240),
            Ray::fromQ8(player_.offsetYFixed(), 176),
            Ray::fromQ8(viewCameraYQ8(), 176 + 142),
            Ray::turnsFromQ8Degrees(player_.speedXFixed()));
    }
    const PlayerMotion& player() const { return player_; }
    unsigned tick() const { return tick_; }
    unsigned ringOrigin() const { return ringOrigin_; }
    unsigned bulletCount() const;
    unsigned bulletSlotEnd() const { return bulletSlotEnd_; }
    unsigned gemSlotEnd() const { return gemSlotEnd_; }
    const BulletState& bulletAt(unsigned index) const;
    const BulletState* findBullet(std::uint32_t lifetimeId) const {
        if(lifetimeId)for(unsigned i=0;i<bulletSlotEnd_;++i)
            if(bullets_[i].active && bullets_[i].lifetimeId==lifetimeId)return &bullets_[i];
        return nullptr;
    }
    const EnemyState& enemyAt(unsigned index) const;
    const EnemyState* findEnemy(std::uint32_t lifetimeId) const {
        if(lifetimeId)for(unsigned i=0;i<enemyCount_;++i)
            if((enemies_[i].active || enemies_[i].activationPending) &&
               enemies_[i].lifetimeId==lifetimeId)return &enemies_[i];
        return nullptr;
    }
    downwell::InstanceHandle enemyHandle(std::uint32_t lifetimeId) const {
        const auto* enemy=findEnemy(lifetimeId);
        return enemy?downwell::InstanceHandle{enemy->identitySlot,enemy->lifetimeId}:downwell::InstanceHandle{};
    }
    const EnemyState* findEnemy(downwell::InstanceHandle handle) const {
        if(handle.slot>=enemyCapacity||!handle.generation||!enemyIdentitySlots_[handle.slot])return nullptr;
        const auto* enemy=findEnemy(handle.generation);
        return enemy&&enemy->identitySlot==handle.slot?enemy:nullptr;
    }
    const downwell::FlashBallEffects<32>& flashEffects() const {return run_.flashEffects;}
    const downwell::CorpseExpiryEffects<32>& corpseExpiryEffects()const{return run_.corpseExpiryEffects;}
    const downwell::PlayerDamageNumbers<8>& damageNumbers()const{return run_.damageNumbers;}
    template<class Expiry,class Number> bool visitForegroundEffects(Expiry expiry,Number number)const{
        return run_.visitForegroundEffects(expiry,number);
    }
    bool emitDamageNumber(downwell::MotionBinary64 x,downwell::MotionBinary64 y,
        downwell::MotionBinary64 amount,downwell::MotionBinary64 direction){
        return run_.createDamageNumber(x,y,amount,direction)!=0;
    }
    bool emitCorpseExpiry(downwell::MotionBinary64 x,downwell::MotionBinary64 y){return run_.emitCorpseExpiry(x,y);}
    const downwell::SmokeEffects<32>& smokeEffects() const {return run_.smokeEffects;}
    bool effectsSucceeded()const{return run_.effectsSucceeded;}
    bool emitSmokeBurst(downwell::MotionBinary64 x,downwell::MotionBinary64 y){
        return run_.emitSmokeBurst(x,y,enemyRandom_);
    }
    // Effect positions use original absolute room coordinates, not player offsets.
    bool emitFlashBurst(downwell::MotionBinary64 x,downwell::MotionBinary64 y) {
        return run_.emitFlashBurst(x,y,enemyRandom_);
    }
    unsigned enemyCount() const { return enemyCount_; }
    unsigned enemiesDefeated() const { return run_.kills; }
    unsigned gemCount() const;
    const GemState& gemAt(unsigned index) const;
    const GemState* findGem(std::uint32_t lifetimeId) const {
        if(lifetimeId)for(unsigned i=0;i<gemSlotEnd_;++i)
            if(gems_[i].active && gems_[i].lifetimeId==lifetimeId)return &gems_[i];
        return nullptr;
    }
    unsigned currency() const { return currencyAccount_.currency; }
    unsigned gameGemTotal() const { return currencyAccount_.gameGem; }
    unsigned gemStreak() const { return currencyAccount_.streak; }
    unsigned gemStreakTimer() const { return currencyAccount_.timer; }
    int health() const { return health_; }
    unsigned ammoHudOffset() const { return run_.ammoHud.offset; }
    unsigned ammoMeterOffset() const { return run_.ammoHud.meterOffset; }
    const downwell::AmmoGaugeRows& ammoGauge() const { return run_.ammoGauge; }
    unsigned invulnerability() const { return invulnerability_; }
    bool dead() const { return death_.active; }
    unsigned deathFrames() const { return death_.frames; }
    bool deathMenuReady() const { return death_.menuReady; }
    const downwell::PlayerDeathSequence& deathSequence() const { return death_; }
    const downwell::RunGemStatistics& runGemStatistics() const { return run_.gems; }
    std::uint32_t runTimeTicks() const { return run_.clock.ticks; }
    const downwell::RunCombo& runCombo() const { return run_.combo; }
    bool grantConsumable(unsigned id) { return run_.consumables.grant(id, currencyAccount_); }
    bool grantUpgrade(unsigned id) {
        return run_.upgrades.grant(id, run_.consumables, currencyAccount_,
            player_, health_, run_.combo.healthReserve);
    }
    const downwell::UpgradeInventory& upgrades() const { return run_.upgrades; }
    int nextMuzzleOffset() const { return emitter_.leg; }
    const downwell::Consumables& consumables() const { return run_.consumables; }
    downwell::ShopWorld::Instance* registerShop(int x, int y) {
        return run_.createShop(x, y, [this](unsigned lo, unsigned hi) {
            return lo + enemyRandom_.irandomSmall(hi - lo);
        }, [this] { return enemyRandom_.chooseIndexSmall(3); }, health_);
    }
    const downwell::ShopWorld& shops() const { return run_.shops; }
    bool registerSideCameraLocker(int x, int y) { return run_.sideCameraLockers.create(x, y); }
    const downwell::OrdinaryTimeFields<64>& timeFields() const {return run_.timeFields;}
    bool cameraSideLocked() const { return cameraMotion_.sideLocked; }
    std::uint64_t sideCameraDigest() const { return run_.sideCameraLockers.digest(cameraMotion_); }
    // Read-only rendering hint: entering a camera column may collide with a
    // locker at a different height. No RNG or live camera state is changed.
    int previewCameraViewY(int horizontalOffset) const {
        auto camera = cameraMotion_;
        camera.step((240L + horizontalOffset) * 256, 176L * 256 + player_.offsetYFixed(),
                    dead(), run_.sideCameraLockers.overlaps(camera));
        run_.sideCameraLockers.collide(camera);
        return camera.pixelY() - 142;
    }
    void registerLevelExit(int y) { run_.createLevelExit(480, y, enemyRandom_, cameraMotion_); }
    bool levelExitPresent() const { return run_.levelExitPresent; }
    const downwell::LevelExit& levelExit() const { return run_.levelExit; }
    const downwell::GoalTimeField& goalField() const { return run_.goalField; }
    unsigned goalFieldBirthBoundary() const { return run_.goalFieldBirthBoundary; }
    bool goalStopped() const { return run_.goalStopped; }
    bool exitDitherRequested() const { return run_.exitDitherRequested; }
    const downwell::ExitDither& exitDither() const { return run_.exitDither; }
    bool levelResultRequested() const { return run_.levelResultRequested; }
    // Scene owner must retire old DMA before entry; use this update path
    // instead of gameplay step until the next-room request is consumed.
    bool enterLevelClear(unsigned style, bool heartGoal) {
        return run_.enterLevelClear(style, heartGoal, health_, player_, enemyRandom_);
    }
    template<class Emit>
    bool stepLevelClear(bool& confirm, bool leftPressed, bool rightPressed, bool& heartGoal, Emit emit) {
        if (!run_.levelClearActive || run_.nextLevelRequested) return false;
        ++tick_;
        return run_.stepLevelClear(confirm, leftPressed, rightPressed, heartGoal,
            currencyAccount_, player_, health_, enemyRandom_, emit);
    }
    bool levelClearActive() const { return run_.levelClearActive; }
    bool nextLevelRequested() const { return run_.nextLevelRequested; }
    // Retire the old display first. This resets only room-local runtime state;
    // the owner must rebuild terrain and rebind generation/collision callbacks.
    bool prepareNextAreaOneLevel(const downwell::Input& input, unsigned initialRingOrigin);
    template<class Builder>
    bool initializeAreaOneBuilder(Builder& builder, int x, int y, unsigned style = 0) {
        if (run_.stage.area != 1 || run_.stage.level < 1 || run_.stage.level > 3 || style > 4) return false;
        downwell::AreaOneBuilder::Setup setup;
        setup.level = unsigned(run_.stage.level);
        setup.style = style;
        setup.member = run_.upgrades.levels[14] != 0;
        builder.resetRoom(x, y, enemyRandom_, setup);
        return true;
    }
    const downwell::LevelClearController& levelClear() const { return run_.levelClear; }
    const downwell::LevelClearPresentation::Frame& levelClearFrame() const { return run_.levelClearFrame; }
    const downwell::RunStage& runStage() const { return run_.stage; }
    int healthMaximum() const { return run_.combo.healthReserve.maximum; }
    const downwell::HealthReserve& healthReserve() const { return run_.combo.healthReserve; }
    const downwell::DeathResults& deathResults() const { return run_.results; }
    bool playerVisible() const { return downwell::playerDamageVisible(invulnerability_); }
    bool bulletActive() const { return bulletCount() != 0; }
    int bulletX() const;
    int bulletY() const;
    int cameraOffsetY() const { return cameraPixelY_; }
    int cameraOffsetX() const { return cameraPixelX_; }
    bool breakableActiveAt(int x, int y) const;
    unsigned blocksDestroyed() const { return blocksDestroyed_; }
    std::uint64_t checkpoint() const { return checkpoint_; }
    std::uint64_t trajectoryDigest() const { return trajectory_.value(); }

private:
    EnemyLifecycleBinding enemyLifecycle_;
    bool publishEnemyActivation(const EnemyState& enemy, bool active);
    bool publishEnemyImage(const EnemyState& enemy,unsigned phase);
    bool publishEnemyTransform(const EnemyState& enemy,downwell::RunnerTransformField field,int value);
    bool retireEnemyOwnership(EnemyState& enemy);
    bool clearEnemyOwnership();
    // Normal gameplay reads camMain's published pixel position. Preserve the
    // synthetic replay fixture's explicit camera track when following is off.
    long viewCameraYQ8() const { return followCamera_ ? cameraPixelY_ * 256L : cameraY_; }
    void applyFrogAlarm(EnemyState& enemy, const downwell::FrogAlarmEffect& effect);
    void advanceEnemyAlarms();
    void updateCrawler(EnemyState& enemy, bool alarm);
    GenerationStep generationStep_ = nullptr;
    void* generationContext_ = nullptr;
    bool generationSucceeded_ = true;
    struct Breakable {
        int x;
        int y;
        bool active;
    };
    static constexpr unsigned breakableCapacity = 512;

    PlayerMotion player_;
    unsigned tick_;
    unsigned ringOrigin_;
    bool followCamera_;
    long cameraY_;
    downwell::MainCameraMotion cameraMotion_;
    int cameraPixelY_;
    int cameraPixelX_;
    int activationY_;
    BulletState bullets_[bulletCapacity];
    std::uint32_t nextBulletLifetimeId_ = 1;
    std::uint32_t nextEnemyLifetimeId_ = 1, nextGemLifetimeId_ = 1;
    unsigned bulletSlotEnd_ = 0, gemSlotEnd_ = 0;
    EnemyState enemies_[enemyCapacity];
    bool enemyIdentitySlots_[enemyCapacity]{};
    GemState gems_[gemCapacity];
    unsigned enemyCount_;
    downwell::GameMakerRandom enemyRandom_;
    int health_;
    unsigned invulnerability_;
    downwell::PlayerDeathSequence death_;
    downwell::RunSession run_;
    downwell::MachineGunEmitter emitter_;
    downwell::CurrencyAccount currencyAccount_;
    SolidResolver solidResolver_;
    SegmentResolver segmentResolver_ = nullptr;
    SolidResolver maskResolver_ = nullptr;
    WallContactResolver wallContactResolver_ = nullptr;
    void* solidResolverContext_;
    PlayerMotion::LandingResolver landingResolver_ = nullptr;
    void* landingResolverContext_ = nullptr;
    Breakable breakables_[breakableCapacity];
    unsigned breakableCount_;
    bool breakableIndexReady_ = false;
    std::uint16_t breakableIndex_[64 * 512];
    int indexedBreakable(int x, int y) const;
    unsigned blocksDestroyed_;
    std::uint64_t checkpoint_;
    downwell::StateDigest trajectory_;

    void spawnReward(const EnemyState& enemy, bool attracted);
};

#endif
