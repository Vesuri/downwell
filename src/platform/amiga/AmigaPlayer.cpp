#include "AmigaPlayer.h"

namespace {
enum {
    fixedOne = 256,
    horizontalLimit = 60 * fixedOne,
    horizontalAcceleration = 48,
    horizontalFriction = 32,
    horizontalMaximum = 384,
    jumpSpeed = -4 * fixedOne,
    releasedJumpSpeed = -384,
    gunbootRecoil = -384,
    gravity = 52,
    fallMaximum = 6 * fixedOne,
    maximumAmmo = 8
};
}

AmigaPlayer::AmigaPlayer(bool startsGrounded, int landingOffset)
    : x_(0), y_(0), speedX_(0), speedY_(0),
      landingY_(landingOffset * fixedOne), pixelX_(0), pixelY_(0),
      ammo_(maximumAmmo),
      grounded_(startsGrounded), priorFire_(false),
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

    if (input.left != input.right) {
        speedX_ += input.left ? -horizontalAcceleration
                              : horizontalAcceleration;
        if (speedX_ < -horizontalMaximum) speedX_ = -horizontalMaximum;
        if (speedX_ > horizontalMaximum) speedX_ = horizontalMaximum;
    } else if (speedX_ < 0) {
        speedX_ += horizontalFriction;
        if (speedX_ > 0) speedX_ = 0;
    } else if (speedX_ > 0) {
        speedX_ -= horizontalFriction;
        if (speedX_ < 0) speedX_ = 0;
    }
    x_ += speedX_;
    if (x_ < -horizontalLimit) { x_ = -horizontalLimit; speedX_ = 0; }
    if (x_ > horizontalLimit) { x_ = horizontalLimit; speedX_ = 0; }

    if (firePressed && grounded_) {
        grounded_ = false;
        speedY_ = jumpSpeed;
        ++jumps_;
    } else if (firePressed && ammo_ > 0) {
        speedY_ += gunbootRecoil;
        --ammo_;
        ++shots_;
    }
    if (!input.fire && speedY_ < releasedJumpSpeed)
        speedY_ = releasedJumpSpeed;

    if (!grounded_) {
        speedY_ += gravity;
        if (speedY_ > fallMaximum) speedY_ = fallMaximum;
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
