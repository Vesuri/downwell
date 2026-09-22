#include "AmigaPlayer.h"

namespace {
enum {
    fixedOne = 256,
    horizontalLimit = 60 * fixedOne,
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
      landingY_(landingOffset * fixedOne), usesLandingProfile_(false),
      pixelX_(0), pixelY_(0), ammo_(maximumAmmo),
      grounded_(startsGrounded), priorFire_(false),
      jumpShootLocked_(false), airborneShot_(false),
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

AmigaPlayerMotion AmigaPlayer::step(const downwell::Input& input)
{
    const int oldPixelX = pixelX_;
    const int oldPixelY = pixelY_;
    const bool firePressed = input.fire && !priorFire_;
    const bool fireReleased = !input.fire && priorFire_;

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
    x_ += speedX_;
    if (x_ < -horizontalLimit) { x_ = -horizontalLimit; speedX_ = 0; }
    if (x_ > horizontalLimit) { x_ = horizontalLimit; speedX_ = 0; }
    if (usesLandingProfile_) {
        int profileX = fixedToPixel(x_) - landingProfileLeft;
        if (profileX < 0) profileX = 0;
        if (profileX >= (int)landingProfileSize)
            profileX = (int)landingProfileSize - 1;
        landingY_ = landingProfile_[profileX] * fixedOne;
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

    priorFire_ = input.fire;
    pixelX_ = fixedToPixel(x_);
    pixelY_ = fixedToPixel(y_);
    AmigaPlayerMotion motion = {pixelX_ - oldPixelX, pixelY_ - oldPixelY};
    return motion;
}

int AmigaPlayer::spriteId() const
{
    if (grounded_) return 31;
    return airborneShot_ ? 3 : 37;
}

unsigned AmigaPlayer::spriteFrame() const
{
    if (grounded_ || airborneShot_) return 0;
    if (speedY_ < -2 * fixedOne) return 0;
    if (speedY_ < 0) return 1;
    if (speedY_ < fixedOne) return 2;
    if (speedY_ < 2 * fixedOne) return 3;
    return 4;
}
