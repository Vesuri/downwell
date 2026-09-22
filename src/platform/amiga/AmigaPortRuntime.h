#ifndef DOWNWELL_AMIGA_PORT_RUNTIME_H
#define DOWNWELL_AMIGA_PORT_RUNTIME_H

#include "AmigaPlayer.h"
#include "core/StateDigest.h"

#include <cstdint>

enum { amigaPortBulletCapacity = 6 };

struct AmigaPortFrame {
    AmigaPlayerMotion playerMotion;
    bool ammoChanged;
    bool bulletChanged;
    bool scrolled;
    int scrollDelta;
    unsigned destroyedBlockCount;
    int blockX[amigaPortBulletCapacity];
    int blockY[amigaPortBulletCapacity];
};

class AmigaPortRuntime {
public:
    static constexpr unsigned ringHeight = 320;
    static constexpr unsigned bulletCapacity = amigaPortBulletCapacity;
    static constexpr int bulletBottom = 284;

    struct BulletState {
        int x;
        int y;
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
    void configureBreakables(const int* x, const int* y, unsigned count);

    const AmigaPlayer& player() const { return player_; }
    unsigned tick() const { return tick_; }
    unsigned ringOrigin() const { return ringOrigin_; }
    unsigned bulletCount() const;
    const BulletState& bulletAt(unsigned index) const;
    bool bulletActive() const { return bulletCount() != 0; }
    int bulletX() const;
    int bulletY() const;
    int cameraOffsetY() const { return cameraPixelY_; }
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
    static constexpr unsigned breakableCapacity = 16;

    AmigaPlayer player_;
    unsigned tick_;
    unsigned ringOrigin_;
    bool followCamera_;
    long cameraY_;
    int cameraPixelY_;
    BulletState bullets_[bulletCapacity];
    Breakable breakables_[breakableCapacity];
    unsigned breakableCount_;
    unsigned blocksDestroyed_;
    std::uint64_t checkpoint_;
    downwell::StateDigest trajectory_;
};

#endif
