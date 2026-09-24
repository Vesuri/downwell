#ifndef DOWNWELL_AMIGA_PORT_RUNTIME_H
#define DOWNWELL_AMIGA_PORT_RUNTIME_H

#include "AmigaPlayer.h"
#include "core/GameMakerRandom.h"
#include "core/StateDigest.h"
#include "core/GameplayLimits.h"
#include "core/GemMotion.h"
#include "core/CurrencyAccount.h"
#include "core/MainCameraMotion.h"
#include "core/PlayerDeathSequence.h"
#include "core/RunGemStatistics.h"
#include "core/DeathResults.h"
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

#include <cstdint>

enum {
    amigaPortBulletCapacity = downwell::gameplayBulletCapacity,
    amigaPortDestroyedCapacity = amigaPortBulletCapacity + 1,
    amigaPortEnemyCapacity = 64,
    amigaPortGemCapacity = downwell::gameplayGemCapacity
};

struct AmigaPortFrame {
    AmigaPlayerMotion playerMotion;
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
    int blockX[amigaPortDestroyedCapacity];
    int blockY[amigaPortDestroyedCapacity];
};

class AmigaPortRuntime {
public:
    static constexpr unsigned ringHeight = 320;
    static constexpr unsigned bulletCapacity = amigaPortBulletCapacity;
    static constexpr unsigned destroyedCapacity = amigaPortDestroyedCapacity;
    static constexpr unsigned enemyCapacity = amigaPortEnemyCapacity;
    static constexpr unsigned gemCapacity = amigaPortGemCapacity;
    typedef bool (*SolidResolver)(void* context, int x, int y);
    // Called once after existing alarms and camera Step, before bullets and
    // enemy motion. Receives the shared RNG and ceil(world-space view bottom).
    // A false return latches generationSucceeded() until explicitly rebound.
    using GenerationStep = bool (*)(void*, AmigaPortRuntime&,
                                    downwell::GameMakerRandom&, int);
    void configureGenerationStep(GenerationStep step, void* context) {
        generationStep_ = step;
        generationContext_ = context;
        generationSucceeded_ = true;
    }
    bool generationSucceeded() const { return generationSucceeded_; }

    struct BulletState {
        int x;
        int y;
        bool active;
    };
    struct EnemyState {
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
        unsigned drawFrame() const {
            if (objectId == 119)
                return imageFrame + ((crawler.direction + (motionState > 0 ? 0u : 2u)) % 4u) * 256u;
            // Source enmSnail Step: image_yscale = -sign(ysp).
            // Frame 256+n is host-generated, for both planar and DMA assets.
            return imageFrame + (objectId == 117 && speedY > 0 ? 256u : 0u);
        }
    };
    struct GemState {
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

    explicit AmigaPortRuntime(unsigned initialRingOrigin,
                              bool startsGrounded = true,
                              int landingOffset = 0,
                              bool followCamera = false);
    AmigaPortFrame step(std::uint8_t packedInput);
    void initializeEntryInput(bool fireHeld) { player_.initializeEntryInput(fireHeld); }
    void configureLandingProfile(const int* offsets, unsigned count) {
        player_.configureLandingProfile(offsets, count);
    }
    void configureLandingResolver(AmigaPlayer::LandingResolver resolver,
                                  void* context) {
        player_.configureLandingResolver(resolver, context);
    }
    void configureBreakables(const int* x, const int* y, unsigned count);
    // Append a newly generated batch without reviving destroyed earlier blocks.
    // Reject capacity failure before changing any state.
    bool appendBreakables(const int* x, const int* y, unsigned count);
    void configureSolidResolver(SolidResolver resolver, void* context) {
        solidResolver_ = resolver;
        solidResolverContext_ = context;
        player_.configureSolidResolver(resolver, context, -236, 236);
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
    std::uint64_t randomDrawCount() const { return enemyRandom_.drawCount(); }

    const AmigaPlayer& player() const { return player_; }
    unsigned tick() const { return tick_; }
    unsigned ringOrigin() const { return ringOrigin_; }
    unsigned bulletCount() const;
    const BulletState& bulletAt(unsigned index) const;
    const EnemyState& enemyAt(unsigned index) const;
    unsigned enemyCount() const { return enemyCount_; }
    unsigned enemiesDefeated() const { return enemiesDefeated_; }
    unsigned gemCount() const;
    const GemState& gemAt(unsigned index) const;
    unsigned currency() const { return currencyAccount_.currency; }
    unsigned gameGemTotal() const { return currencyAccount_.gameGem; }
    unsigned gemStreak() const { return currencyAccount_.streak; }
    unsigned gemStreakTimer() const { return currencyAccount_.timer; }
    int health() const { return health_; }
    unsigned invulnerability() const { return invulnerability_; }
    bool dead() const { return death_.active; }
    unsigned deathFrames() const { return death_.frames; }
    bool deathMenuReady() const { return death_.menuReady; }
    const downwell::PlayerDeathSequence& deathSequence() const { return death_; }
    const downwell::RunGemStatistics& runGemStatistics() const { return runGemStatistics_; }
    const downwell::DeathResults& deathResults() const { return deathResults_; }
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

    AmigaPlayer player_;
    unsigned tick_;
    unsigned ringOrigin_;
    bool followCamera_;
    long cameraY_;
    downwell::MainCameraMotion cameraMotion_;
    int cameraPixelY_;
    int cameraPixelX_;
    int activationY_;
    BulletState bullets_[bulletCapacity];
    EnemyState enemies_[enemyCapacity];
    GemState gems_[gemCapacity];
    unsigned enemyCount_;
    unsigned enemiesDefeated_;
    downwell::GameMakerRandom enemyRandom_;
    int health_;
    unsigned invulnerability_;
    downwell::PlayerDeathSequence death_;
    downwell::RunGemStatistics runGemStatistics_;
    downwell::DeathResults deathResults_;
    downwell::CurrencyAccount currencyAccount_;
    SolidResolver solidResolver_;
    void* solidResolverContext_;
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
