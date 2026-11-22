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
    typedef bool (*FacingPublication)(void* context,int value);
    void configureFacingPublication(FacingPublication callback,void* context){
        facingPublication_=callback;facingPublicationContext_=context;
    }
    void configureComboCompletion(ComboCompletion callback, void* context, bool timeStopped = false) {
        comboCompletion_ = callback; comboContext_ = context; comboTimeStopped_ = timeStopped;
    }
    void configureMovementBoundary(ComboCompletion callback, void* context) {
        movementBoundary_ = callback; movementBoundaryContext_ = context;
    }
    // Coordinates use fifths of Q8, matching FixedCollisionMath.
    // Return success separately from the collision result. A query failure
    // stops the step without rolling back earlier movement/script effects.
    typedef bool (*BreakBoxResolver)(void* context, long x, long y, bool& found);
    bool motionSucceeded() const { return motionSucceeded_; }
    void configureCollisionOrigin(int x,int y){collisionOriginX_=x;collisionOriginY_=y;}
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
    long speedYFifths() const { return speedY_*5+gravityRemainder_; }
    bool jumpShootLocked() const { return jumpShootLocked_; }
    bool shotDelayed() const { return shotDelay_ != 0; }
    bool airborneShot() const { return airborneShot_; }
    bool spinJumping() const { return spinJumping_; }
    // global.ballooning as scrGravity_n reads it in this player's Step.
    void setBallooning(bool value) { ballooning_ = value; }
    // global.pugJet (Safety Jetpack), read at each Step; jetFrame resets with
    // the player (scrPlayerInit) and on the ground.
    void setJetpack(bool value) { jetpackOwned_ = value; }
    bool jetpacking() const { return jetpacking_; }
    unsigned jetHalves() const { return jetHalves_; }
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
    // Opt-in per-gun rules for scrPlayerShootN/scrUpButtonFunctions (the
    // compact world's seven guns). Unconfigured motion keeps the usual-gun
    // adapter: release always clears shotDelay and nonAuto is never set.
    void configureGun(int consumption, int rof, bool delayKill, int recoilFifths) {
        gunRules_ = true; gunConsumption_ = consumption; gunRof_ = rof;
        gunDelayKill_ = delayKill; gunRecoilFifths_ = recoilFifths;
    }
    bool nonAuto() const { return nonAuto_; }
    // scrBurstFire: ysp = global.pBulRecoil, unconditionally.
    void setVerticalSpeedFifths(long value) {
        speedY_ = value / 5; gravityRemainder_ = static_cast<int>(value % 5);
    }
    // GunModule battery: global.ammo += 2; global.stammo = global.ammo.
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
    // scrPlayerShootN's empty-charge branch: each call emits two smoke puffs;
    // the first since the last landing also raises "EMPTY!" (noChargeMsg).
    unsigned emptyShots() const { return emptyShots_; }
    // Cosmetic dust created during the last step, in creation order:
    // objJumpFx 95 at scrJump, objJumpSmallerFx 94 at a grounded turn start
    // and at landing. x/y are Q8 offsets; emitTo selects the drawn halves.
    struct Dust { bool jump; long x, y; int emitTo; };
    unsigned dustCount() const { return dustCount_; }
    const Dust& dust(unsigned index) const { return dust_[index]; }
    unsigned emptyMessages() const { return emptyMessages_; }
    // scrPlMovement's airborne landings (each calls levelBeginCue outside
    // ground rooms, before its landing dust).
    unsigned landings() const { return landings_; }
    unsigned shots() const { return shots_; }
    unsigned recharges() const { return recharges_; }
    // W2.3: sound scripts called inside this Begin Step, in source order
    // (scrUpButtonFunctions, the landing block, the footstep). Their RNG draws
    // belong to the owner, right after step() and before any later draw:
    // jump and wallKick are soundPlay(choose(86)); land is soundLand (gated
    // on sfxGunRecharge and death); footstep is soundFootstep; hardLandTurn is
    // the hard-landing hop's soundPlay(88). recharge is a successful
    // scrRecharge (sfxGunRecharge), also from stompBounce/contactKnockback;
    // the owner drains those with takeRechargeSounds().
    // rocket: scrJump's pugRocket block, which precedes the jump sound.
    enum class SoundEvent : unsigned char { jump, wallKick, land, footstep, hardLandTurn, rocket };
    // Rocket Jump (upgrade 18): scrPlayerInit's jumpsp 5, and scrJump's blast
    // at the instance x/y of the last jump (Q8, offset space).
    void setRocketJump(bool value) { rocketJump_ = value; }
    long rocketX() const { return rocketX_; }
    long rocketY() const { return rocketY_; }
    unsigned soundEventCount() const { return soundEventCount_; }
    SoundEvent soundEvent(unsigned index) const { return soundEvents_[index]; }
    unsigned takeRechargeSounds() { const unsigned n = pendingRecharges_; pendingRecharges_ = 0; return n; }
    bool hardLandJump() const { return hardLandJump_; }
    bool wallPose() const { return wallPose_; }
    // scrEnemyStomped and the Collision scripts: -1 uses the run's style.
    void stompBounce(int playStyle = -1);
    // global.playStyle (styleSet/scrPlayerInit/scrGravity_n): 3 (levitate)
    // has gravity / 1.5 and jumpsp * 0.85; 2 (boulder) animates its run at
    // 0.25 and idle at 0.2; each style has its own idle, run and air art.
    // Global, so it survives room changes like the gun.
    void setPlayStyle(int style) { playStyle_ = style < 0 || style > 4 ? 0 : style; }
    int playStyle() const { return playStyle_; }
    void contactKnockback(int enemyX, int enemyY);
    void appendStateDigest(downwell::StateDigest& digest) const;

private:
    bool motionSucceeded_ = true;
    bool rocketJump_ = false;
    long rocketX_ = 0, rocketY_ = 0;
    // World-owned coordinate adapter, rebound before each shared gameplay
    // step like the terrain callbacks; not an independently simulated field.
    int collisionOriginX_=0,collisionOriginY_=0;
    static int fixedToPixel(long value);
    static unsigned integerSquareRoot(unsigned value);
    void beginSpin();
    void recharge();
    void jump(const downwell::Input& input, int x, int y);
    bool solidBelowWithin(int pixels) const;
    void addSound(SoundEvent event) { if (soundEventCount_ < 4) soundEvents_[soundEventCount_++] = event; }
    void setFacing(int value);
    SoundEvent soundEvents_[4] = {};
    unsigned soundEventCount_ = 0, pendingRecharges_ = 0;
    // scrPlayerInit: airstatus = 1, hardLand = hardLandJump = 0,
    // tinyJumpThreshold = 0. The two counters are alarm[6] and alarm[10].
    bool airstatus_ = true, hardLand_ = false, hardLandJump_ = false, tinyJumpThreshold_ = false;
    unsigned hardLandAlarm_ = 0, tinyJumpAlarm_ = 0;
    bool wallPose_ = false; // playerSpriteControl's sprPlayerWall (20) frame 0
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
    bool gunRules_ = false, gunDelayKill_ = true, nonAuto_ = false;
    int gunConsumption_ = 1, gunRof_ = 7, gunRecoilFifths_ = 0;
    ComboCompletion comboCompletion_ = nullptr;
    void* comboContext_ = nullptr;
    bool comboTimeStopped_ = false;
    ComboCompletion movementBoundary_ = nullptr;
    FacingPublication facingPublication_=nullptr;
    void* facingPublicationContext_=nullptr;
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
    unsigned emptyShots_ = 0, emptyMessages_ = 0, landings_ = 0;
    Dust dust_[3] = {};
    unsigned dustCount_ = 0;
    int playStyle_ = 0;
    bool ballooning_ = false;
    bool jetpackOwned_ = false, jetpacking_ = false;
    unsigned jetHalves_ = 0; // jetFrame * 2
    unsigned gravityThirds_ = 0; // style 3's 512/3-fifth gravity, 171+171+170
    bool wallColliding_ = false; // scrWallCol's previous-Step xcollision
    void addDust(bool jump, long x, long y, int emitTo) {
        if (dustCount_ < 3) dust_[dustCount_++] = {jump, x, y, emitTo};
    }
    bool noChargeMessage_ = false;
    unsigned shots_;
};

#endif
