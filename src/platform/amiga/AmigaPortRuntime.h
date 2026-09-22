#ifndef DOWNWELL_AMIGA_PORT_RUNTIME_H
#define DOWNWELL_AMIGA_PORT_RUNTIME_H

#include "AmigaPlayer.h"
#include "core/StateDigest.h"

#include <cstdint>

enum {
    amigaPortBulletCapacity = 6,
    amigaPortEnemyCapacity = 3,
    amigaPortGemCapacity = 12
};

struct AmigaPortFrame {
    AmigaPlayerMotion playerMotion;
    bool ammoChanged;
    bool healthChanged;
    bool currencyChanged;
    bool bulletChanged;
    bool scrolled;
    int scrollDelta;
    bool cameraXChanged;
    int cameraXDelta;
    unsigned destroyedBlockCount;
    int blockX[amigaPortBulletCapacity];
    int blockY[amigaPortBulletCapacity];
};

class AmigaPortRuntime {
public:
    static constexpr unsigned ringHeight = 320;
    static constexpr unsigned bulletCapacity = amigaPortBulletCapacity;
    static constexpr unsigned enemyCapacity = amigaPortEnemyCapacity;
    static constexpr unsigned gemCapacity = amigaPortGemCapacity;
    static constexpr int bulletBottom = 284;
    typedef bool (*SolidResolver)(void* context, int x, int y);

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
        bool active;
        bool awake;
    };
    struct GemState {
        int x;
        int y;
        int amount;
        unsigned age;
        long preciseX;
        long preciseY;
        long speedX;
        long speedY;
        bool large;
        bool active;
    };

    explicit AmigaPortRuntime(unsigned initialRingOrigin,
                              bool startsGrounded = true,
                              int landingOffset = 0,
                              bool followCamera = false);
    AmigaPortFrame step(std::uint8_t packedInput);
    void configureLandingProfile(const int* offsets, unsigned count) {
        player_.configureLandingProfile(offsets, count);
    }
    void configureLandingResolver(AmigaPlayer::LandingResolver resolver,
                                  void* context) {
        player_.configureLandingResolver(resolver, context);
    }
    void configureBreakables(const int* x, const int* y, unsigned count);
    void configureSolidResolver(SolidResolver resolver, void* context) {
        solidResolver_ = resolver;
        solidResolverContext_ = context;
        player_.configureSolidResolver(resolver, context, -236, 236);
    }
    bool configureEnemy(unsigned index, int objectId, int spriteId,
                        int x, int y, int speedX, int speedY,
                        int health, int reward);

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
    unsigned currency() const { return currency_; }
    int health() const { return health_; }
    unsigned invulnerability() const { return invulnerability_; }
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
    struct Breakable {
        int x;
        int y;
        bool active;
    };
    static constexpr unsigned breakableCapacity = 96;

    AmigaPlayer player_;
    unsigned tick_;
    unsigned ringOrigin_;
    bool followCamera_;
    long cameraY_;
    int cameraPixelY_;
    int cameraPixelX_;
    BulletState bullets_[bulletCapacity];
    EnemyState enemies_[enemyCapacity];
    GemState gems_[gemCapacity];
    unsigned enemyCount_;
    unsigned enemiesDefeated_;
    int health_;
    unsigned invulnerability_;
    unsigned currency_;
    SolidResolver solidResolver_;
    void* solidResolverContext_;
    Breakable breakables_[breakableCapacity];
    unsigned breakableCount_;
    unsigned blocksDestroyed_;
    std::uint64_t checkpoint_;
    downwell::StateDigest trajectory_;

    void spawnReward(const EnemyState& enemy);
    static unsigned integerSquareRoot(unsigned value);
};

#endif
