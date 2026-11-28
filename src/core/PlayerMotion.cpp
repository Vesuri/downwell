#include "PlayerMotion.h"
#include "core/PlayerControls.h"
#include "core/PlayerLifeRules.h"
#include "core/SolidCollision.h"
#include "core/StateDigest.h"
#include "core/MachineGunProfile.h"
#include "core/PlayerBoxContact.h"
#include "core/GameMakerRounding.h"
#include <cstdint>

namespace {
enum {
    fixedOne = 256,
    jumpSpeed = -1126,
    gravity = 51,
    fallMaximum = 7 * fixedOne,
    maximumAmmo = 8
};
}

PlayerMotion::PlayerMotion(bool startsGrounded, int landingOffset)
    : x_(0), y_(0), speedX_(0), speedY_(0),
      gravityRemainder_(0), verticalPositionRemainder_(0),
      landingY_(landingOffset * fixedOne), landingResolver_(0),
      landingResolverContext_(0), solidResolver_(0),
      solidResolverContext_(0),
      horizontalLeftLimit_(-60), horizontalRightLimit_(60),
      usesLandingProfile_(false),
      pixelX_(0), pixelY_(0), ammo_(maximumAmmo),
      grounded_(startsGrounded), priorFire_(false),
      jumpShootLocked_(false), airborneShot_(false),
      running_(false), runPhaseTenths_(0),
      idling_(startsGrounded), idlePhaseTenths_(0),
      balancing_(false), balancePhaseQuarters_(0),
      shotPhaseQuarters_(0),
      shotDelay_(startsGrounded ? 0u : 1u), facing_(1),
      jumps_(0), shots_(0)
{
    unsigned index;
    for (index = 0; index < landingProfileSize; ++index)
        landingProfile_[index] = landingOffset;
    airstatus_ = !startsGrounded; // scrPlayerInit's airborne entry pose
}

void PlayerMotion::enterRoom(bool fireHeld, int landingOffset)
{
    const int capacity = ammoCapacity_;
    const int facing = facing_;
    const bool rules = gunRules_, delayKill = gunDelayKill_;
    const int consumption = gunConsumption_, rof = gunRof_, recoil = gunRecoilFifths_;
    const int style = playStyle_;
    const bool rocket = rocketJump_;
    *this = PlayerMotion(false, landingOffset);
    playStyle_ = style; rocketJump_ = rocket;
    // scrPlayerInit clears nonAuto; the selected gun is global state.
    gunRules_ = rules; gunDelayKill_ = delayKill;
    gunConsumption_ = consumption; gunRof_ = rof; gunRecoilFifths_ = recoil;
    ammoCapacity_ = capacity;
    ammo_ = capacity;
    facing_ = facing;
    initializeEntryInput(fireHeld);
}

void PlayerMotion::configureLandingProfile(const int* offsets, unsigned count)
{
    unsigned index;
    if (offsets == 0 || count != landingProfileSize)
        return;
    for (index = 0; index < landingProfileSize; ++index)
        landingProfile_[index] = offsets[index];
    usesLandingProfile_ = true;
}

int PlayerMotion::fixedToPixel(long value)
{
    return value >= 0 ? (int)(value >> 8) : -(int)((-value) >> 8);
}

unsigned PlayerMotion::integerSquareRoot(unsigned value)
{
    unsigned result = 0;
    unsigned bit = 1u << 30;
    while (bit > value)
        bit >>= 2;
    while (bit != 0) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    return result;
}

void PlayerMotion::beginSpin()
{
    if (!spinJumping_) {
        // scrFjump does not reset image_index. Preserve fractional phase
        // from the currently selected animation when switching to spin.
        spinPhaseTwentieths_ = grounded_
            ? running_ ? runPhaseTenths_ * 2
              : balancing_ ? balancePhaseQuarters_ * 5 : idlePhaseTenths_ * 2
            : airborneShot_ ? shotPhaseQuarters_ * 5 : spriteFrame() * 20;
    }
    spinJumping_ = true;
}

void PlayerMotion::stompBounce(int playStyle)
{
    if (playStyle < 0) playStyle = playStyle_;
    beginSpin();
    // scrEnemyStomped: Boulder (2) uses scrFjump(0,1.5); others use 2.5.
    speedY_ = playStyle == 2 ? -384 : -640;
    gravityRemainder_ = 0;
    grounded_ = false;
    recharge();
}

void PlayerMotion::recharge()
{
    if (ammo_ < ammoCapacity_) {
        ammo_ = ammoCapacity_;
        ++recharges_;
        ++pendingRecharges_; // soundPlay(27, 90, 0, 1)
    }
}

// scrJump's dry, rocketless branch.
void PlayerMotion::jump(const downwell::Input& input, int x, int y)
{
    if (rocketJump_) {
        // scrPlayerInit: pugRocket's jumpsp 5, then style 3's * 0.85 (4.25).
        setVerticalSpeedFifths(playStyle_ == 3 ? -5440 : -6400);
        rocketX_ = long(x) * fixedOne; rocketY_ = long(y) * fixedOne;
        addSound(SoundEvent::rocket);
    } else {
        speedY_ = jumpSpeed;
        gravityRemainder_ = -2; // -4.4 * 256 = -1126 - 2/5
        // scrPlayerInit: style 3's jumpsp *= 0.85 (3.74 = 4787.2 fifths).
        if (playStyle_ == 3) setVerticalSpeedFifths(-4787);
    }
    addSound(SoundEvent::jump); // soundPlay(choose(86), 80, 0, 1)
    // objJumpFx at the instance x/y.
    addDust(true, long(x) * fixedOne, long(y) * fixedOne, 0);
    jumpShootLocked_ = true;
    grounded_ = false;
    hardLandJump_ = false;
    if (input.left || input.right) spinJumping_ = true;
    spinPhaseTwentieths_ = 0; // scrJump explicitly resets image_index
    ++jumps_;
}

// place_meeting(xx, yy + pixels, sParentSolid) below the player: the landing
// surface is within reach when standing on it is less than that far down.
bool PlayerMotion::solidBelowWithin(int pixels) const
{
    const int px = fixedToPixel(x_), py = fixedToPixel(y_);
    long landing = landingY_ / fixedOne;
    if (landingResolver_ != 0) landing = landingResolver_(landingResolverContext_, px, py);
    else if (usesLandingProfile_) {
        int profileX = px - landingProfileLeft;
        if (profileX < 0) profileX = 0;
        if (profileX >= (int)landingProfileSize) profileX = (int)landingProfileSize - 1;
        landing = landingProfile_[profileX];
    }
    return landing - py < pixels;
}

void PlayerMotion::setFacing(int value)
{
    if (facing_ == value) return;
    if (facingPublication_ && !facingPublication_(facingPublicationContext_, value)) { motionSucceeded_ = false; return; }
    facing_ = value;
}

void PlayerMotion::contactKnockback(int enemyX, int enemyY)
{
    // scrTypicalDamage aims from enemy x/y to global.plx/ply, published
    // before integration. Keep subpixel precision until normalization.
    const long dx = frameStartX_ - (long)enemyX * fixedOne;
    const long dy = frameStartY_ - (long)enemyY * fixedOne;
    const unsigned distance = integerSquareRoot(
        (unsigned)(dx * dx + dy * dy));
    const long impulseX = distance == 0
                  ? (dx == 0 && dy == 0 ? 3L * fixedOne : 0)
                  : (long)dx * (3L * fixedOne) / (long)distance;
    // scrFjump(impulse, 2) in ForcedPlayerJump's order: spin, recharge,
    // airborne, xsp only for a nonzero impulse, then ysp = -2. Both arguments
    // are finite here, so its validity checks always pass.
    // Coincident points have direction zero, hence lengthdir_x(3,0)=+3.
    beginSpin();
    recharge();
    grounded_=false;
    if(impulseX)speedX_=impulseX;
    speedY_=-2*fixedOne;gravityRemainder_=0;
}

PlayerMotionDelta PlayerMotion::step(const downwell::Input& rawInput, bool controlsLocked, bool dead,
                                   bool interactable, bool goalStopped, bool gemHigh)
{
    if(!motionSucceeded_)return {};
    const auto input = downwell::playerGameplayInput(rawInput, dead ? 0 : 1);
    frameStartX_ = x_;
    frameStartY_ = y_;
    const int oldPixelX = pixelX_;
    const int oldPixelY = pixelY_;
    const bool firePressed = input.fire && !priorFire_;
    const bool fireReleased = !input.fire && priorFire_;
    bool shotFired = false;
    bool horizontalCollision = false;
    dustCount_ = 0;
    soundEventCount_ = 0;
    // scrPlMovement reads grounded as the previous scrWallCol left it.
    const bool groundedAtEntry = grounded_;

    if (shotDelay_ > 0)
        --shotDelay_;
    // Gravity precedes horizontal movement/support checks in the recovered
    // player Step. Walking off an edge must start falling this same frame.
    if (playStyle_ == 3) {
        // scrGravity_n: global.grav / 1.5 = 512/3 fifths of Q8 per Step.
        setVerticalSpeedFifths(speedYFifths() + (gravityThirds_ < 2 ? 171 : 170));
        gravityThirds_ = gravityThirds_ < 2 ? gravityThirds_ + 1 : 0;
    } else {
        speedY_ += gravity;
        if (++gravityRemainder_ >= 5) {
            ++speedY_;
            gravityRemainder_ -= 5;
        }
    }
    // scrGravity_n: a held Heart Balloon (global.ballooning) lowers maxgrav by 1.
    const long fallLimit = ballooning_ ? fallMaximum - fixedOne : fallMaximum;
    if (speedY_ >= fallLimit) {
        speedY_ = fallLimit;
        gravityRemainder_ = 0;
    }
    // scrPlMovement calls scrUpButtonFunctions only when noControl is false.
    // Input edges still advance globally while locked; do not replay a release
    // later or manufacture one by replacing held input with neutral input.
    if (fireReleased && !dead && !controlsLocked) {
        if (speedY_ < 0) {
            const long halved = (speedY_ * 5 + gravityRemainder_) / 2;
            speedY_ = halved / 5;
            gravityRemainder_ = (int)(halved % 5);
        }
        jumpShootLocked_ = false;
        // global.pBulDelayKill == 1 only; nonAuto always clears.
        if (!gunRules_ || gunDelayKill_) shotDelay_ = 0;
        nonAuto_ = false;
        jetpacking_ = false;
    }

    const bool wasSpinning = spinJumping_;
    if (dead) {
        if (controlsLocked) speedX_ = 0;
        // scrPlMovement death branch bypasses living input/friction. Like
        // living acceleration, 0.08 is rounded to the existing Q8 storage.
        const long drag = downwell::FixedPlayerMath::ratio(2, 25);
        speedX_ -= speedX_ > 0 ? drag : speedX_ < 0 ? -drag : 0;
        // playerSpriteControl follows and snaps abs(xsp)<0.5 to zero.
        if (speedX_ > -fixedOne / 2 && speedX_ < fixedOne / 2) speedX_ = 0;
        if (speedY_ * 5 + gravityRemainder_ < 0) {
            const long halved = (speedY_ * 5 + gravityRemainder_) / 2;
            speedY_ = halved / 5;
            gravityRemainder_ = static_cast<int>(halved % 5);
        }
    } else {
        // scrPlMovement: a grounded press from rest or against the current
        // motion, without wall contact, kicks dust behind the new direction.
        if (!controlsLocked && grounded_ && !wallColliding_ && input.left != input.right &&
            (input.left ? speedX_ >= 0 : speedX_ <= 0))
            addDust(false, x_, y_, input.left ? -1 : 1);
        if(!downwell::applyPlayerHorizontalInput<downwell::FixedPlayerMath>(
            speedX_, facing_, grounded_, input.left, input.right, controlsLocked,!spinJumping_,
            [this](int value){return !facingPublication_||facingPublication_(facingPublicationContext_,value);})){
            motionSucceeded_=false;return {};
        }
    }
    // scrUpButtonFunctions' dUp block, before scrWallCol integrates the new
    // speeds: jump, the hard-landing grace jump, the wall kick and the tiny
    // jump threshold's grounded shot. Instance x/y are the previous Step's
    // published position.
    bool groundedBlock = groundedAtEntry, tinyShot = false;
    const auto publishedX = [this] { return fixedToPixel(frameStartX_); };
    const auto publishedY = [this] { return fixedToPixel(frameStartY_); };
    if (!controlsLocked && !dead && firePressed) {
        if (!interactable) {
            if (groundedAtEntry) {
                if (tinyJumpThreshold_ && !(solidResolver_ && solidMaskAt(publishedX(), publishedY() - 16)))
                    tinyJumpThreshold_ = false;
                if (tinyJumpThreshold_ && !hardLandJump_) {
                    tinyShot = true; // scrPlayerShootN while grounded
                    tinyJumpThreshold_ = false;
                } else {
                    jump(input, oldPixelX, oldPixelY);
                    groundedBlock = false;
                }
            } else if (hardLandJump_) {
                if (solidBelowWithin(8)) jump(input, oldPixelX, oldPixelY);
                else hardLandJump_ = false;
            } else if (spinJumping_ && solidResolver_) {
                int kick = 0; // xsp = -maxsp toward the side away from the wall
                if (input.left) { if (solidMaskAt(publishedX() + 3, publishedY())) kick = -1; }
                else if (input.right) { if (solidMaskAt(publishedX() - 3, publishedY())) kick = 1; }
                if (kick) {
                    // objJumpFx at x/y (image_angle 90 or 270), choose(86).
                    addDust(true, long(publishedX()) * fixedOne, long(publishedY()) * fixedOne, kick);
                    addSound(SoundEvent::wallKick);
                    jumpShootLocked_ = true;
                    grounded_ = false;
                    shotPhaseQuarters_ = 0; // image_index = 0
                    tinyJumpAlarm_ = 30;
                    tinyJumpThreshold_ = true;
                    hardLandJump_ = false;
                    speedY_ = -4 * fixedOne; // wallkicksp
                    gravityRemainder_ = 0;
                    speedX_ = kick * 2 * fixedOne; // maxsp
                    spinJumping_ = false;
                }
            }
        }
        if (!jumpShootLocked_) spinJumping_ = false;
    }
    if (solidResolver_ != 0) {
        long remaining = speedX_;
        while (remaining != 0) {
            long movement = remaining;
            if (movement < -fixedOne) movement = -fixedOne;
            if (movement > fixedOne) movement = fixedOne;
            const long nextX = x_ + movement;
            // place_meeting rounds the moved mask like the upward probe below
            // (opt-in; the gameplay adapter still truncates).
            const int nextPixelX = roundedHorizontal_
                ? static_cast<int>(downwell::roundGameMakerQ8(nextX + collisionOriginX_ * 256L) - collisionOriginX_)
                : fixedToPixel(nextX);
            if (solidMaskAt(nextPixelX, pixelY_)) {
                speedX_ = 0;
                horizontalCollision = true;
                break;
            }
            x_ = nextX;
            remaining -= movement;
        }
    } else {
        x_ += speedX_;
    }
    if (x_ < (long)horizontalLeftLimit_ * fixedOne) {
        x_ = (long)horizontalLeftLimit_ * fixedOne;
        speedX_ = 0;
        horizontalCollision = true;
    }
    if (x_ > (long)horizontalRightLimit_ * fixedOne) {
        x_ = (long)horizontalRightLimit_ * fixedOne;
        speedX_ = 0;
        horizontalCollision = true;
    }
    // objPlayer_n Begin Step still calls scrWallCol when goalStop is set.
    // Retain collision correction, but skip ordinary xx += xsp integration.
    if (goalStopped && !horizontalCollision) x_ = frameStartX_;
    if (landingResolver_ != 0) {
        landingY_ = landingResolver_(landingResolverContext_,
                                    fixedToPixel(x_), fixedToPixel(y_)) *
                    fixedOne;
        if (grounded_ && y_ < landingY_)
            grounded_ = false;
    } else if (usesLandingProfile_) {
        int profileX = fixedToPixel(x_) - landingProfileLeft;
        if (profileX < 0) profileX = 0;
        if (profileX >= (int)landingProfileSize)
            profileX = (int)landingProfileSize - 1;
        landingY_ = landingProfile_[profileX] * fixedOne;
        if (grounded_ && y_ < landingY_)
            grounded_ = false;
    }

    // scrUpButtonFunctions' dUpHeld block: scrPlayerShootN clears the spin
    // whenever it is called, whether or not a charge remains.
    // grounded is still the Begin Step's (an edge walk-off is scrWallCol's).
    const bool shootCall = tinyShot || (!controlsLocked && input.fire && !groundedBlock && shotDelay_ == 0 &&
        !jumpShootLocked_ && !hardLandJump_ && !nonAuto_);
    if (shootCall) spinJumping_ = false;
    if (shootCall && ammo_ == 0) {
        // scrPlayerShootN's empty-charge branch still brakes a fast fall.
        // It consumes no charge and creates no bullet or shooting animation.
        if (speedY_ > 3 * fixedOne ||
            (speedY_ == 3 * fixedOne && gravityRemainder_ > 0)) {
            speedY_ = 3 * fixedOne;
            gravityRemainder_ = 0;
        }
        ++emptyShots_;
        if (!noChargeMessage_) { noChargeMessage_ = true; ++emptyMessages_; }
        shotDelay_ = 16;
    } else if (shootCall && ammo_ > 0) {
        if (gunRules_) {
            // if (ysp > global.pBulRecoil) ysp = global.pBulRecoil
            if (speedY_ * 5 + gravityRemainder_ > gunRecoilFifths_)
                setVerticalSpeedFifths(gunRecoilFifths_);
            ammo_ -= gunConsumption_;
            shotDelay_ = static_cast<unsigned>(gunRof_ < 0 ? -gunRof_ : gunRof_);
            if (gunRof_ < 0) nonAuto_ = true;
            // stammo <= 0 after the shot: stammo = 0 and nonAuto = 1.
            if (ammo_ <= 0) { ammo_ = 0; nonAuto_ = true; }
        } else {
            if (speedY_ > 0 || (speedY_ == 0 && gravityRemainder_ > 0)) {
                speedY_ = 0;
                gravityRemainder_ = 0;
            }
            ammo_ -= downwell::machineGunProfile(gemHigh).consumption;
            shotDelay_ = downwell::machineGunProfile(gemHigh).interval;
        }
        ++shots_;
        if (!groundedBlock) airborneShot_ = true; // scrPlMovement's !grounded pFired branch
        shotFired = true;
    }
    // scrUpButtonFunctions' pugJet block (Safety Jetpack): airborne with Up
    // held, not nonAuto and out of charge, jetFrame (halves here) climbs by
    // 0.5 to jetMax 90 and caps the fall at jetPower = 3 * jetFrame / 90,
    // jetHalves / 60 pixels: 64/3 fifths per half, floored to the lattice.
    if (jetpackOwned_ && !controlsLocked && !dead && input.fire && !groundedBlock && !nonAuto_) {
        if (jetHalves_ < 180 && ammo_ <= 0) {
            jetpacking_ = true;
            ++jetHalves_;
            const long limit = static_cast<long>(jetHalves_) * 64 / 3;
            if (speedYFifths() > limit) setVerticalSpeedFifths(limit);
        } else {
            jetpacking_ = false;
        }
    }

    // scrPlMovement's grounded block, after the jump decisions: the landing
    // block (the hard-landing hop or soundLand), comboDone and the footstep.
    bool hopped = false;
    if (groundedBlock) {
        if (airstatus_) {
            airstatus_ = false;
            if (hardLand_ && (rawInput.left || rawInput.right) && playStyle_ != 3) {
                // scrFjump(0, 1): scrRecharge, grounded = 0, ysp = -1. Its
                // spinJumping = 1 is cleared again by this block.
                recharge();
                grounded_ = false;
                speedY_ = -fixedOne;
                gravityRemainder_ = 0;
                hardLandJump_ = true;
                hardLandAlarm_ = 15;
                addSound(SoundEvent::hardLandTurn);
                hopped = true;
            } else {
                addSound(SoundEvent::land);
            }
            hardLand_ = false;
            noChargeMessage_ = false;
            spinJumping_ = false;
            airborneShot_ = false;
        }
        if (comboCompletion_ && !comboTimeStopped_) comboCompletion_(comboContext_);
        jetpacking_ = false; jetHalves_ = 0;
        // sprite_index == spriteRun (imgsprun 0.3) entering frame 1 or 5.
        if (running_ && playStyle_ != 2 && playStyle_ != 3 &&
            ((runPhaseTenths_ >= 10 && runPhaseTenths_ < 13) || (runPhaseTenths_ >= 50 && runPhaseTenths_ < 53)))
            addSound(SoundEvent::footstep);
    }
    // playerSpriteControl: a spin beside a wall shows the kick pose.
    wallPose_ = false;
    if (!(groundedBlock && !hopped) && spinJumping_ && solidResolver_ && !dead) {
        if (input.left) { if (solidMaskAt(publishedX() + 3, publishedY())) { wallPose_ = true; setFacing(-1); } }
        else if (input.right) { if (solidMaskAt(publishedX() - 3, publishedY())) { wallPose_ = true; setFacing(1); } }
        if (!motionSucceeded_) return {};
    }
    if (!grounded_) airstatus_ = true;
    // scrPlMovement's death/saveStats tail precedes scrWallCol and its
    // landing combo/recharge. Keep that observable boundary shared.
    if (movementBoundary_) movementBoundary_(movementBoundaryContext_);
    // scrWallCol's head: hardLand holds through a fall of at least
    // hardLandSp (1.2 = 1536 fifths) and toggles otherwise.
    if (!hardLand_ && !hardLandJump_) hardLand_ = true;
    else if (hardLand_ && speedY_ * 5 + gravityRemainder_ < 1536) hardLand_ = false;
    if (!grounded_) {
        if (solidResolver_ != 0 && speedY_ < 0) {
            long collisionX = x_ * 5;
            long collisionY = y_ * 5 + verticalPositionRemainder_;
            int hit=0;
            const bool checked = downwell::resolveVerticalSolidCollisionChecked<downwell::FixedCollisionMath>(
                collisionX, collisionY, speedY_ * 5 + gravityRemainder_,
                [this](long px, long py,bool& found) {
                    int x,y;
                    if(!downwell::PlayerBoxContact::terrainCentre(px,py,collisionOriginX_,collisionOriginY_,x,y))return false;
                    found=solidMaskAt(x,y);return true;
                },hit);
            if(!checked){
                y_=collisionY/5;
                verticalPositionRemainder_=static_cast<int>(collisionY%5);
                motionSucceeded_=false;return {};
            }
            if (hit < 0) {
                y_ = collisionY / 5;
                verticalPositionRemainder_ = (int)(collisionY % 5);
                // scrWallCol writes ysp=0 before the synchronous box query.
                speedY_=0;gravityRemainder_=0;
                const long response = downwell::ceilingResponse<downwell::FixedCollisionMath>(
                    collisionX, collisionY, [this](long x, long y) {
                        bool found=false;
                        if(breakBoxResolver_ && !breakBoxResolver_(breakBoxContext_,x,y,found))
                            motionSucceeded_=false;
                        return found;
                    });
                if(!motionSucceeded_)return {};
                speedY_ = std::int16_t(response) / 5; // 0 or unit() 1280
                gravityRemainder_ = 0;
            }
        }
        const long displacement = speedY_ * 5 + gravityRemainder_ +
                                  verticalPositionRemainder_;
        const long nextY = y_ + displacement / 5;
        if (!goalStopped) {
            y_ = nextY;
            verticalPositionRemainder_ = (int)(displacement % 5);
        }
        // A predicted landing is scrWallCol correction, not yy += ysp.
        if (nextY >= landingY_) {
            y_ = landingY_;
            speedY_ = 0;
            gravityRemainder_ = 0;
            verticalPositionRemainder_ = 0;
            grounded_ = true;
            // scrWallCol: airborne landing completes only without xcollision,
            // before scrRecharge. A simultaneous wall hit defers completion.
            if (!horizontalCollision && comboCompletion_ && !comboTimeStopped_)
                comboCompletion_(comboContext_);
            ++landings_;
            // Landing dust at the corrected position, emitTo = sign(xsp).
            addDust(false, x_, y_, speedX_ > 0 ? 1 : speedX_ < 0 ? -1 : 0);
            recharge();
            airborneShot_ = false;
            noChargeMessage_ = false;
            spinJumping_ = false;
        }
    }

    if (grounded_) {
        spinJumping_ = false;
        speedY_ = 0;
        gravityRemainder_ = 0;
    }
    pixelX_ = fixedToPixel(x_);
    pixelY_ = fixedToPixel(y_);

    bool nowBalancing = grounded_ && speedX_ == 0 &&
        !input.left && !input.right && solidResolver_ != 0;
    if (nowBalancing) {
        const int probeX = pixelX_ + 5 * facing_;
        for (int probeY = pixelY_; probeY <= pixelY_ + 16; ++probeY) {
            if (solidResolver_(solidResolverContext_, probeX, probeY)) {
                nowBalancing = false;
                break;
            }
        }
    }
    if (nowBalancing) {
        if (!balancing_)
            balancePhaseQuarters_ = (std::uint16_t(idlePhaseTenths_) / 10) * 4;
        else
            balancePhaseQuarters_ = std::uint16_t(balancePhaseQuarters_ + 1) % 20;
    } else {
        balancePhaseQuarters_ = 0;
    }
    balancing_ = nowBalancing;

    const bool nowRunning = grounded_ && speedX_ != 0;
    if (nowRunning) {
        if (!running_)
            runPhaseTenths_ = 0;
        else
            // imgsprun: 0.3, or 0.25 for style 2 (alternate 2 and 3 tenths;
            // the floor of every exact quarter step is the same frame).
            runPhaseTenths_ = (runPhaseTenths_ + (playStyle_ != 2 ? 3 :
                (runPhaseTenths_ % 5 == 0 ? 2 : 3))) % 80;
    } else {
        runPhaseTenths_ = 0;
    }
    running_ = nowRunning;

    const bool nowIdling = grounded_ && speedX_ == 0 && !balancing_;
    if (nowIdling) {
        if (!idling_)
            idlePhaseTenths_ = 0;
        else
            idlePhaseTenths_ = std::uint16_t(idlePhaseTenths_ + (playStyle_ == 2 ? 2 : 1)) % 40;
    } else {
        idlePhaseTenths_ = 0;
    }
    idling_ = nowIdling;

    if (!airborneShot_) {
        shotPhaseQuarters_ = 0;
    } else if (shotFired) {
        shotPhaseQuarters_ = 0;
    } else if (shotPhaseQuarters_ < 13) {
        ++shotPhaseQuarters_;
    }

    if (spinJumping_ && wasSpinning)
        spinPhaseTwentieths_ = std::uint16_t(spinPhaseTwentieths_ + 6) % 160;
    if (!spinJumping_) spinPhaseTwentieths_ = 0;
    // alarm[6] clears hardLandJump, alarm[10] tinyJumpThreshold.
    if (hardLandAlarm_ && --hardLandAlarm_ == 0) hardLandJump_ = false;
    if (tinyJumpAlarm_ && --tinyJumpAlarm_ == 0) tinyJumpThreshold_ = false;
    wallColliding_ = horizontalCollision;
    priorFire_ = input.fire;
    PlayerMotionDelta motion = {pixelX_ - oldPixelX, pixelY_ - oldPixelY};
    return motion;
}

void PlayerMotion::appendStateDigest(downwell::StateDigest& digest) const
{
    // Serialize authoritative state, not float presentation mirrors or
    // callback addresses. Timer/fraction differences affect future behavior.
    digest.addI32(x_); digest.addI32(y_);
    digest.addI32(frameStartX_); digest.addI32(frameStartY_);
    digest.addI32(speedX_); digest.addI32(speedY_);
    digest.addI32(gravityRemainder_);
    digest.addI32(verticalPositionRemainder_);
    digest.addI32(landingY_);
    digest.addI32(horizontalLeftLimit_); digest.addI32(horizontalRightLimit_);
    digest.addBool(usesLandingProfile_);
    if (usesLandingProfile_)
        for (unsigned i = 0; i < landingProfileSize; ++i) digest.addI32(landingProfile_[i]);
    digest.addI32(pixelX_); digest.addI32(pixelY_);
    digest.addI32(ammo_); digest.addBool(grounded_);
    digest.addBool(priorFire_); digest.addBool(jumpShootLocked_);
    digest.addBool(airborneShot_); digest.addBool(running_);
    digest.addBool(spinJumping_); digest.addU32(spinPhaseTwentieths_);
    digest.addU32(runPhaseTenths_); digest.addBool(idling_);
    digest.addU32(idlePhaseTenths_); digest.addBool(balancing_);
    digest.addU32(balancePhaseQuarters_); digest.addU32(shotPhaseQuarters_);
    digest.addU32(shotDelay_); digest.addI32(facing_);
    digest.addU32(jumps_); digest.addU32(shots_);
    // Preserve the default-state replay stream; nondefault capacity is an
    // explicit extension and must differ even when current charge is equal.
    if (ammoCapacity_ != maximumAmmo) {
        digest.addU32(0x414d4d4f); digest.addI32(ammoCapacity_);
    }
    if (nonAuto_) digest.addU32(0x4e4f4e41);
    if (playStyle_) { digest.addU32(0x5354594c); digest.addI32(playStyle_); digest.addU32(gravityThirds_); }
    digest.addU32(0x4c414e44); // W2.3 landing, hop and wall-kick state
    digest.addBool(airstatus_); digest.addBool(hardLand_); digest.addBool(hardLandJump_);
    digest.addBool(tinyJumpThreshold_); digest.addU32(hardLandAlarm_); digest.addU32(tinyJumpAlarm_);
    digest.addBool(wallPose_);
    if(!motionSucceeded_)digest.addU32(0x4641494c);
}

int PlayerMotion::spriteId(bool dead) const
{
    if (dead) return 0; // playerSpriteControl: sprPlDead.
    // styleSet: spriteIdle, spriteRun and spriteAir by global.playStyle.
    static const int idle[5] = {31, 33, 34, 35, 32}, run[5] = {27, 25, 26, 28, 29},
        air[5] = {37, 38, 40, 28, 42};
    if (grounded_) return running_ ? run[playStyle_] : balancing_ ? 15 : idle[playStyle_];
    if (spinJumping_) return wallPose_ ? 20 : 1;
    return airborneShot_ ? 3 : air[playStyle_];
}

unsigned PlayerMotion::spriteFrame(bool dead, int deathAnimation) const
{
    // The phases stay below 160 (every writer is a modulus or a small
    // multiple of another phase), so 16-bit division is exact.
    // Source death branch is not an animated living/spinning pose. Its second
    // frame requires grounded, abs(xsp)<0.5 and deathani==0, all together.
    if (dead) return grounded_ && speedX_ > -128 && speedX_ < 128 && deathAnimation == 0 ? 1 : 0;
    if (grounded_)
        return running_ ? std::uint16_t(runPhaseTenths_) / 10
                        : balancing_ ? balancePhaseQuarters_ / 4
                                     : std::uint16_t(idlePhaseTenths_) / 10;
    if (spinJumping_) return wallPose_ ? 0 : std::uint16_t(spinPhaseTwentieths_) / 20;
    if (airborneShot_)
        return shotPhaseQuarters_ / 4 < 3 ? shotPhaseQuarters_ / 4 : 3;
    return downwell::playerAirFrame(speedYFifths());
}
