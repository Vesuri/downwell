#ifndef DOWNWELL_CORE_PLAYER_MOTION_H
#define DOWNWELL_CORE_PLAYER_MOTION_H

#include "core/Input.h"

namespace downwell { class StateDigest; }

// Portable default-style player state/update, extracted from the native port.
// Terrain is queried through callbacks; rendering and hardware are not owned
// here. This is an adaptation, not a complete implementation of scrWallCol:
// full original mask/event ordering remains migration work.

struct PlayerMotionDelta {
    int deltaX;
    int deltaY;
};

class PlayerMotion {
public:
    typedef int (*LandingResolver)(void* context, int x, int minimumY);
    typedef bool (*SolidResolver)(void* context, int x, int y);
    typedef void (*ComboCompletion)(void* context);
    void configureComboCompletion(ComboCompletion callback, void* context, bool timeStopped = false) {
        comboCompletion_ = callback; comboContext_ = context; comboTimeStopped_ = timeStopped;
    }
    void configureMovementBoundary(ComboCompletion callback, void* context) {
        movementBoundary_ = callback; movementBoundaryContext_ = context;
    }
    // Coordinates use fifths of Q8, matching FixedCollisionMath.
    typedef bool (*BreakBoxResolver)(void* context, long x, long y);
    void configureBreakBoxResolver(BreakBoxResolver resolver, void* context) {
        breakBoxResolver_ = resolver;
        breakBoxContext_ = context;
    }

    static constexpr int landingProfileLeft = -60;
    static constexpr unsigned landingProfileSize = 121;

    explicit PlayerMotion(bool startsGrounded = true,
                         int landingOffset = 0);
    PlayerMotionDelta step(const downwell::Input& input, bool controlsLocked = false,
                           bool dead = false);
    void initializeEntryInput(bool fireHeld) {
        priorFire_ = fireHeld;
        jumpShootLocked_ = fireHeld;
        shotDelay_ = 30; // scrPlayerInit alarm[2]
    }
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
    // scrPlMovement publishes global.eplx/eply before position integration.
    long frameStartXFixed() const { return frameStartX_; }
    long frameStartYFixed() const { return frameStartY_; }
    long speedXFixed() const { return speedX_; }
    long speedYFixed() const { return speedY_; }
    bool jumpShootLocked() const { return jumpShootLocked_; }
    bool shotDelayed() const { return shotDelay_ != 0; }
    bool airborneShot() const { return airborneShot_; }
    bool spinJumping() const { return spinJumping_; }
    int ammo() const { return ammo_; }
    int ammoCapacity() const { return ammoCapacity_; }
    // comboRewardText Create increases global.ammo and refills global.stammo.
    void increaseAmmoCapacity(unsigned amount) {
        ammoCapacity_ += static_cast<int>(amount);
        ammo_ = ammoCapacity_;
    }
    bool grounded() const { return grounded_; }
    int facing() const { return facing_; }
    int spriteId(bool dead = false) const;
    unsigned spriteFrame(bool dead = false, int deathAnimation = 1) const;
    unsigned jumps() const { return jumps_; }
    unsigned shots() const { return shots_; }
    unsigned recharges() const { return recharges_; }
    void stompBounce();
    void contactKnockback(int enemyX, int enemyY);
    void appendStateDigest(downwell::StateDigest& digest) const;

private:
    static int fixedToPixel(long value);
    static unsigned integerSquareRoot(unsigned value);
    void beginSpin();
    void recharge();
    unsigned recharges_ = 0; // Successful scrRecharge calls, not capacity grants.

    long x_;
    long y_;
    long frameStartX_ = 0;
    long frameStartY_ = 0;
    long speedX_;
    long speedY_;
    int gravityRemainder_; // signed fifths of a Q8 velocity unit
    int verticalPositionRemainder_; // signed fifths of a Q8 position unit
    long landingY_;
    int landingProfile_[landingProfileSize];
    LandingResolver landingResolver_;
    void* landingResolverContext_;
    SolidResolver solidResolver_;
    BreakBoxResolver breakBoxResolver_ = nullptr;
    void* breakBoxContext_ = nullptr;
    void* solidResolverContext_;
    int horizontalLeftLimit_;
    int horizontalRightLimit_;
    bool usesLandingProfile_;
    int pixelX_;
    int pixelY_;
    int ammo_;
    int ammoCapacity_ = 8;
    ComboCompletion comboCompletion_ = nullptr;
    void* comboContext_ = nullptr;
    bool comboTimeStopped_ = false;
    ComboCompletion movementBoundary_ = nullptr;
    void* movementBoundaryContext_ = nullptr;
    bool grounded_;
    bool priorFire_;
    bool jumpShootLocked_;
    bool airborneShot_;
    bool spinJumping_ = false;
    unsigned spinPhaseTwentieths_ = 0;
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
