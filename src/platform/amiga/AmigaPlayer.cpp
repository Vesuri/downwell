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
      landingY_(landingOffset * fixedOne), pixelX_(0), pixelY_(0),
      ammo_(maximumAmmo),
      grounded_(startsGrounded), priorFire_(false),
      jumpShootLocked_(false), shotDelay_(startsGrounded ? 0u : 1u),
      jumps_(0), shots_(0)
{
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
        shotDelay_ = 7;
    }

    if (!grounded_) {
        y_ += speedY_;
        if (y_ >= landingY_) {
            y_ = landingY_;
            speedY_ = 0;
            grounded_ = true;
            ammo_ = maximumAmmo;
        }
    }

    priorFire_ = input.fire;
    pixelX_ = fixedToPixel(x_);
    pixelY_ = fixedToPixel(y_);
    AmigaPlayerMotion motion = {pixelX_ - oldPixelX, pixelY_ - oldPixelY};
    return motion;
}
