#include "PlayerMotion.h"
#include "core/PlayerControls.h"
#include "core/SolidCollision.h"

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

void PlayerMotion::stompBounce()
{
    speedY_ = -640;
    gravityRemainder_ = 0;
    grounded_ = false;
    ammo_ = maximumAmmo;
}

void PlayerMotion::contactKnockback(int enemyX, int enemyY)
{
    const int dx = pixelX_ - enemyX;
    const int dy = pixelY_ - enemyY;
    const unsigned distance = integerSquareRoot(
        (unsigned)(dx * dx + dy * dy));
    speedX_ = distance == 0
                  ? 0
                  : (long)dx * (3L * fixedOne) / (long)distance;
    speedY_ = -2 * fixedOne;
    gravityRemainder_ = 0;
    grounded_ = false;
}

PlayerMotionDelta PlayerMotion::step(const downwell::Input& input)
{
    const int oldPixelX = pixelX_;
    const int oldPixelY = pixelY_;
    const bool firePressed = input.fire && !priorFire_;
    const bool fireReleased = !input.fire && priorFire_;
    bool shotFired = false;

    if (shotDelay_ > 0)
        --shotDelay_;
    // Gravity precedes horizontal movement/support checks in the recovered
    // player Step. Walking off an edge must start falling this same frame.
    speedY_ += gravity;
    if (++gravityRemainder_ >= 5) {
        ++speedY_;
        gravityRemainder_ -= 5;
    }
    if (speedY_ >= fallMaximum) {
        speedY_ = fallMaximum;
        gravityRemainder_ = 0;
    }
    if (fireReleased) {
        if (speedY_ < 0) {
            const long halved = (speedY_ * 5 + gravityRemainder_) / 2;
            speedY_ = halved / 5;
            gravityRemainder_ = (int)(halved % 5);
        }
        jumpShootLocked_ = false;
        shotDelay_ = 0;
    }

    downwell::applyPlayerHorizontalInput<downwell::FixedPlayerMath>(
        speedX_, facing_, grounded_, input.left, input.right);
    if (solidResolver_ != 0) {
        long remaining = speedX_;
        while (remaining != 0) {
            long movement = remaining;
            if (movement < -fixedOne) movement = -fixedOne;
            if (movement > fixedOne) movement = fixedOne;
            const long nextX = x_ + movement;
            const int nextPixelX = fixedToPixel(nextX);
            if (solidResolver_(solidResolverContext_, nextPixelX - 4,
                               pixelY_ - 4) ||
                solidResolver_(solidResolverContext_, nextPixelX + 3,
                               pixelY_ - 4) ||
                solidResolver_(solidResolverContext_, nextPixelX - 4,
                               pixelY_ + 7) ||
                solidResolver_(solidResolverContext_, nextPixelX + 3,
                               pixelY_ + 7)) {
                speedX_ = 0;
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
    }
    if (x_ > (long)horizontalRightLimit_ * fixedOne) {
        x_ = (long)horizontalRightLimit_ * fixedOne;
        speedX_ = 0;
    }
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

    if (firePressed && grounded_) {
        grounded_ = false;
        speedY_ = jumpSpeed;
        gravityRemainder_ = -2; // -4.4 * 256 = -1126 - 2/5
        jumpShootLocked_ = true;
        ++jumps_;
    }
    if (input.fire && !grounded_ && shotDelay_ == 0 &&
        !jumpShootLocked_ && ammo_ == 0) {
        // scrPlayerShootN's empty-charge branch still brakes a fast fall.
        // It consumes no charge and creates no bullet or shooting animation.
        if (speedY_ > 3 * fixedOne ||
            (speedY_ == 3 * fixedOne && gravityRemainder_ > 0)) {
            speedY_ = 3 * fixedOne;
            gravityRemainder_ = 0;
        }
        shotDelay_ = 16;
    } else if (input.fire && !grounded_ && shotDelay_ == 0 &&
               !jumpShootLocked_ && ammo_ > 0) {
        if (speedY_ > 0 || (speedY_ == 0 && gravityRemainder_ > 0)) {
            speedY_ = 0;
            gravityRemainder_ = 0;
        }
        --ammo_;
        ++shots_;
        airborneShot_ = true;
        shotFired = true;
        shotDelay_ = 7;
    }

    if (!grounded_) {
        if (solidResolver_ != 0 && speedY_ < 0) {
            long collisionX = x_ * 5;
            long collisionY = y_ * 5 + verticalPositionRemainder_;
            const int hit = downwell::resolveVerticalSolidCollision<downwell::FixedCollisionMath>(
                collisionX, collisionY, speedY_ * 5 + gravityRemainder_,
                [this](long px, long py) {
                    // Keep the existing solid adapter's pixel quantization;
                    // rounding X here alone would disagree with side contacts.
                    const int x = (int)(px / 1280);
                    const int y = (int)(py / 1280);
                    return solidResolver_(solidResolverContext_, x - 4, y - 4) ||
                           solidResolver_(solidResolverContext_, x + 3, y - 4) ||
                           solidResolver_(solidResolverContext_, x - 4, y + 7) ||
                           solidResolver_(solidResolverContext_, x + 3, y + 7);
                });
            if (hit < 0) {
                y_ = collisionY / 5;
                verticalPositionRemainder_ = (int)(collisionY % 5);
                speedY_ = 0;
                gravityRemainder_ = 0;
            }
        }
        const long displacement = speedY_ * 5 + gravityRemainder_ +
                                  verticalPositionRemainder_;
        y_ += displacement / 5;
        verticalPositionRemainder_ = (int)(displacement % 5);
        if (y_ >= landingY_) {
            y_ = landingY_;
            speedY_ = 0;
            gravityRemainder_ = 0;
            verticalPositionRemainder_ = 0;
            grounded_ = true;
            ammo_ = maximumAmmo;
            airborneShot_ = false;
        }
    }

    if (grounded_) {
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
            balancePhaseQuarters_ = (idlePhaseTenths_ / 10) * 4;
        else
            balancePhaseQuarters_ = (balancePhaseQuarters_ + 1) % 20;
    } else {
        balancePhaseQuarters_ = 0;
    }
    balancing_ = nowBalancing;

    const bool nowRunning = grounded_ && speedX_ != 0;
    if (nowRunning) {
        if (!running_)
            runPhaseTenths_ = 0;
        else
            runPhaseTenths_ = (runPhaseTenths_ + 3) % 80;
    } else {
        runPhaseTenths_ = 0;
    }
    running_ = nowRunning;

    const bool nowIdling = grounded_ && speedX_ == 0 && !balancing_;
    if (nowIdling) {
        if (!idling_)
            idlePhaseTenths_ = 0;
        else
            idlePhaseTenths_ = (idlePhaseTenths_ + 1) % 40;
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

    priorFire_ = input.fire;
    PlayerMotionDelta motion = {pixelX_ - oldPixelX, pixelY_ - oldPixelY};
    return motion;
}

int PlayerMotion::spriteId() const
{
    if (grounded_) return running_ ? 27 : balancing_ ? 15 : 31;
    return airborneShot_ ? 3 : 37;
}

unsigned PlayerMotion::spriteFrame() const
{
    if (grounded_)
        return running_ ? runPhaseTenths_ / 10
                        : balancing_ ? balancePhaseQuarters_ / 4
                                     : idlePhaseTenths_ / 10;
    if (airborneShot_)
        return shotPhaseQuarters_ / 4 < 3 ? shotPhaseQuarters_ / 4 : 3;
    if (speedY_ < -2 * fixedOne) return 0;
    if (speedY_ < 0) return 1;
    if (speedY_ < fixedOne) return 2;
    if (speedY_ < 2 * fixedOne) return 3;
    return 4;
}
