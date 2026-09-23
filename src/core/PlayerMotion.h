#ifndef DOWNWELL_CORE_PLAYER_MOTION_H
#define DOWNWELL_CORE_PLAYER_MOTION_H

#include "core/Input.h"

// Portable default-style player state/update, extracted from the native port.
// Terrain is queried through callbacks; rendering and hardware are not owned
// here. This is an adaptation, not a complete implementation of scrWallCol:
// ceiling contacts and original mask/event ordering remain migration work.

struct PlayerMotionDelta {
    int deltaX;
    int deltaY;
};

class PlayerMotion {
public:
    typedef int (*LandingResolver)(void* context, int x, int minimumY);
    typedef bool (*SolidResolver)(void* context, int x, int y);

    static constexpr int landingProfileLeft = -60;
    static constexpr unsigned landingProfileSize = 121;

    explicit PlayerMotion(bool startsGrounded = true,
                         int landingOffset = 0);
    PlayerMotionDelta step(const downwell::Input& input);
    void configureLandingProfile(const int* offsets, unsigned count);
    void configureLandingResolver(LandingResolver resolver, void* context) {
        landingResolver_ = resolver;
        landingResolverContext_ = context;
    }
    void configureSolidResolver(SolidResolver resolver, void* context,
                                int leftLimit, int rightLimit) {
        solidResolver_ = resolver;
        solidResolverContext_ = context;
        horizontalLeftLimit_ = leftLimit;
        horizontalRightLimit_ = rightLimit;
    }

    int offsetX() const { return pixelX_; }
    int offsetY() const { return pixelY_; }
    long offsetXFixed() const { return x_; }
    long offsetYFixed() const { return y_; }
    long speedXFixed() const { return speedX_; }
    int ammo() const { return ammo_; }
    bool grounded() const { return grounded_; }
    int facing() const { return facing_; }
    int spriteId() const;
    unsigned spriteFrame() const;
    unsigned jumps() const { return jumps_; }
    unsigned shots() const { return shots_; }
    void stompBounce();
    void contactKnockback(int enemyX, int enemyY);

private:
    static int fixedToPixel(long value);
    static unsigned integerSquareRoot(unsigned value);

    long x_;
    long y_;
    long speedX_;
    long speedY_;
    int gravityRemainder_; // signed fifths of a Q8 velocity unit
    int verticalPositionRemainder_; // signed fifths of a Q8 position unit
    long landingY_;
    int landingProfile_[landingProfileSize];
    LandingResolver landingResolver_;
    void* landingResolverContext_;
    SolidResolver solidResolver_;
    void* solidResolverContext_;
    int horizontalLeftLimit_;
    int horizontalRightLimit_;
    bool usesLandingProfile_;
    int pixelX_;
    int pixelY_;
    int ammo_;
    bool grounded_;
    bool priorFire_;
    bool jumpShootLocked_;
    bool airborneShot_;
    bool running_;
    unsigned runPhaseTenths_;
    bool idling_;
    unsigned idlePhaseTenths_;
    bool balancing_;
    unsigned balancePhaseQuarters_;
    unsigned shotPhaseQuarters_;
    unsigned shotDelay_;
    int facing_;
    unsigned jumps_;
    unsigned shots_;
};

#endif
