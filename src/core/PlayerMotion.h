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
    // scrPlayerInit room-local reset, not ResetPlayer/new-run initialization.
    // Retains global ammo capacity/facing and refills charge. Room-owned
    // collision callbacks must be rebound after replacing the old world.
    void enterRoom(bool fireHeld, int landingOffset = 0);
    PlayerMotionDelta step(const downwell::Input& input, bool controlsLocked = false,
                           bool dead = false, bool interactable = false,
                           bool goalStopped = false, bool gemHigh = false);
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
                                int leftLimit, int rightLimit, SolidResolver mask = nullptr) {
        solidResolver_ = resolver;
        solidMaskResolver_ = mask;
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
    // scrRechargeGem: round(getAmount / 2), capped at capacity. This is not
    // scrRecharge and must not publish its full-recharge counter/effects.
    bool rechargeFromGem(unsigned amount) {
        if (ammo_ >= ammoCapacity_) return false;
        unsigned charge = amount / 2;
        if ((amount & 1u) && (charge & 1u)) ++charge; // ties to even
        const unsigned missing = static_cast<unsigned>(ammoCapacity_ - ammo_);
        ammo_ += static_cast<int>(charge < missing ? charge : missing);
        return true; // Source emits effects even if rounding produces zero.
    }
    // comboRewardText Create increases global.ammo and refills global.stammo.
    void increaseAmmoCapacity(unsigned amount) {
        ammoCapacity_ += static_cast<int>(amount);
        ammo_ = ammoCapacity_;
    }
    bool grounded() const { return grounded_; }
    int facing() const { return facing_; }
    void face(int direction) { facing_ = direction < 0 ? -1 : 1; }
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
    // Optional pure backend query for exactly the four corners below.
    SolidResolver solidMaskResolver_ = nullptr;
    bool solidMaskAt(int x, int y) const {
        if (solidMaskResolver_) return solidMaskResolver_(solidResolverContext_, x, y);
        return solidResolver_(solidResolverContext_, x-4, y-4) ||
            solidResolver_(solidResolverContext_, x+3, y-4) ||
            solidResolver_(solidResolverContext_, x-4, y+7) ||
            solidResolver_(solidResolverContext_, x+3, y+7);
    }
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
