#include "AmigaPlayer.h"

namespace {
enum {
    fixedOne = 256,
    horizontalAcceleration = 51,
    horizontalTurnBoost = fixedOne,
    horizontalFriction = 26,
    horizontalStopThreshold = 77,
    horizontalMaximum = 640,
    jumpSpeed = -1126,
    gravity = 51,
    fallMaximum = 7 * fixedOne,
    maximumAmmo = 8
};
}

AmigaPlayer::AmigaPlayer(bool startsGrounded, int landingOffset)
    : x_(0), y_(0), speedX_(0), speedY_(0),
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
      shotPhaseQuarters_(0),
      shotDelay_(startsGrounded ? 0u : 1u), facing_(1),
      jumps_(0), shots_(0)
{
    unsigned index;
    for (index = 0; index < landingProfileSize; ++index)
        landingProfile_[index] = landingOffset;
}

void AmigaPlayer::configureLandingProfile(const int* offsets, unsigned count)
{
    unsigned index;
    if (offsets == 0 || count != landingProfileSize)
        return;
    for (index = 0; index < landingProfileSize; ++index)
        landingProfile_[index] = offsets[index];
    usesLandingProfile_ = true;
}

int AmigaPlayer::fixedToPixel(long value)
{
    return value >= 0 ? (int)(value >> 8) : -(int)((-value) >> 8);
}

unsigned AmigaPlayer::integerSquareRoot(unsigned value)
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

void AmigaPlayer::stompBounce()
{
    speedY_ = -640;
    grounded_ = false;
    ammo_ = maximumAmmo;
}

void AmigaPlayer::contactKnockback(int enemyX, int enemyY)
{
    const int dx = pixelX_ - enemyX;
    const int dy = pixelY_ - enemyY;
    const unsigned distance = integerSquareRoot(
        (unsigned)(dx * dx + dy * dy));
    speedX_ = distance == 0
                  ? 0
                  : (long)dx * (3L * fixedOne) / (long)distance;
    speedY_ = -2 * fixedOne;
    grounded_ = false;
}

AmigaPlayerMotion AmigaPlayer::step(const downwell::Input& input)
{
    const int oldPixelX = pixelX_;
    const int oldPixelY = pixelY_;
    const bool firePressed = input.fire && !priorFire_;
    const bool fireReleased = !input.fire && priorFire_;
    bool shotFired = false;

    if (shotDelay_ > 0)
        --shotDelay_;
    if (!grounded_) {
        speedY_ += gravity;
        if (speedY_ > fallMaximum) speedY_ = fallMaximum;
    }
    if (fireReleased) {
        if (speedY_ < 0)
            speedY_ /= 2;
        jumpShootLocked_ = false;
        shotDelay_ = 0;
    }

    if (input.left != input.right) {
        if (input.left) {
            speedX_ -= horizontalAcceleration;
            if (speedX_ > 0) speedX_ -= horizontalTurnBoost;
        } else {
            speedX_ += horizontalAcceleration;
            if (speedX_ < 0) speedX_ += horizontalTurnBoost;
        }
        facing_ = input.left ? -1 : 1;
        if (speedX_ < -horizontalMaximum) speedX_ = -horizontalMaximum;
        if (speedX_ > horizontalMaximum) speedX_ = horizontalMaximum;
    } else if (speedX_ < 0) {
        speedX_ += horizontalFriction;
        if (speedX_ > -horizontalStopThreshold) speedX_ = 0;
    } else if (speedX_ > 0) {
        speedX_ -= horizontalFriction;
        if (speedX_ < horizontalStopThreshold) speedX_ = 0;
    }
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
        jumpShootLocked_ = true;
        ++jumps_;
    }
    if (input.fire && !grounded_ && shotDelay_ == 0 &&
        !jumpShootLocked_ && ammo_ > 0) {
        if (speedY_ > 0) speedY_ = 0;
        --ammo_;
        ++shots_;
        airborneShot_ = true;
        shotFired = true;
        shotDelay_ = 7;
    }

    if (!grounded_) {
        y_ += speedY_;
        if (y_ >= landingY_) {
            y_ = landingY_;
            speedY_ = 0;
            grounded_ = true;
            ammo_ = maximumAmmo;
            airborneShot_ = false;
        }
    }

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

    const bool nowIdling = grounded_ && speedX_ == 0;
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
    pixelX_ = fixedToPixel(x_);
    pixelY_ = fixedToPixel(y_);
    AmigaPlayerMotion motion = {pixelX_ - oldPixelX, pixelY_ - oldPixelY};
    return motion;
}

int AmigaPlayer::spriteId() const
{
    if (grounded_) return running_ ? 27 : 31;
    return airborneShot_ ? 3 : 37;
}

unsigned AmigaPlayer::spriteFrame() const
{
    if (grounded_)
        return running_ ? runPhaseTenths_ / 10 : idlePhaseTenths_ / 10;
    if (airborneShot_)
        return shotPhaseQuarters_ / 4 < 3 ? shotPhaseQuarters_ / 4 : 3;
    if (speedY_ < -2 * fixedOne) return 0;
    if (speedY_ < 0) return 1;
    if (speedY_ < fixedOne) return 2;
    if (speedY_ < 2 * fixedOne) return 3;
    return 4;
}
