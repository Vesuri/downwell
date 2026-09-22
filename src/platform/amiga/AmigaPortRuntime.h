#ifndef DOWNWELL_AMIGA_PORT_RUNTIME_H
#define DOWNWELL_AMIGA_PORT_RUNTIME_H

#include "AmigaPlayer.h"
#include "core/StateDigest.h"

#include <cstdint>

struct AmigaPortFrame {
    AmigaPlayerMotion playerMotion;
    bool ammoChanged;
    bool bulletChanged;
    bool scrolled;
    int scrollDelta;
};

class AmigaPortRuntime {
public:
    static constexpr unsigned ringHeight = 320;
    static constexpr int bulletBottom = 284;

    explicit AmigaPortRuntime(unsigned initialRingOrigin,
                              bool startsGrounded = true,
                              int landingOffset = 0,
                              bool followCamera = false);
    AmigaPortFrame step(std::uint8_t packedInput);
    void configureLandingProfile(const int* offsets, unsigned count) {
        player_.configureLandingProfile(offsets, count);
    }

    const AmigaPlayer& player() const { return player_; }
    unsigned tick() const { return tick_; }
    unsigned ringOrigin() const { return ringOrigin_; }
    bool bulletActive() const { return bulletActive_; }
    int bulletX() const { return bulletX_; }
    int bulletY() const { return bulletY_; }
    int cameraOffsetY() const { return cameraPixelY_; }
    std::uint64_t checkpoint() const { return checkpoint_; }
    std::uint64_t trajectoryDigest() const { return trajectory_.value(); }

private:
    AmigaPlayer player_;
    unsigned tick_;
    unsigned ringOrigin_;
    bool followCamera_;
    long cameraY_;
    int cameraPixelY_;
    bool bulletActive_;
    int bulletX_;
    int bulletY_;
    std::uint64_t checkpoint_;
    downwell::StateDigest trajectory_;
};

#endif
