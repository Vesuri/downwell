#include "AmigaSceneRuntime.h"

namespace {
enum {
    fixedOne = 256,
    menuGroundY = 512 * fixedOne,
    menuWellLeft = 300 * fixedOne,
    menuWellRight = 356 * fixedOne,
    menuTeleportY = 625 * fixedOne
};
}

AmigaSceneRuntime::AmigaSceneRuntime()
    : scene_(AmigaScene::splash), tick_(0), priorLeft_(false),
      priorRight_(false), priorFire_(false), splashStage_(1),
      splashTimer_(10), menuTicks_(0), menuNapping_(true),
      menuGrounded_(true), menuTeleport_(false), menuTitleDelay_(-1),
      menuTitleImage_(0), menuTitleReadyTimer_(-1),
      menuTitleReady_(false), menuSequenceDelay_(-1),
      menuEnterActive_(false), menuEnterBlackDelay_(-1),
      menuFacing_(1), menuTitleFrame_(0), menuX_(112 * fixedOne),
      menuY_(menuGroundY), menuSpeedX_(0), menuSpeedY_(0),
      menuCameraX_(112 * fixedOne), menuCameraY_(512 * fixedOne),
      menuCameraLockerY_(448 * fixedOne), menuEnterSpeedY_(0),
      styleYOffset_(-128 * fixedOne),
      styleTileY_(0), styleDitherY_(80 * fixedOne), styleRunFrame_(0),
      styleSmoothX_(0), styleCursor_(0), styleSelected_(false),
      stylePlayerDescent_(0), stylePlayerSpeedY_(0),
      styleVerticalDitherY_(-320 * fixedOne)
{
}

int AmigaSceneRuntime::roundedPixel(long value)
{
    return value >= 0 ? static_cast<int>((value + 128) >> 8)
                      : -static_cast<int>((-value + 128) >> 8);
}

int AmigaSceneRuntime::splashDuration(int stage)
{
    if (stage == 2 || stage == 3) return 180;
    if (stage == 4) return 240;
    if (stage == 5 || stage == 6) return 60;
    return 0;
}

void AmigaSceneRuntime::enterMenu()
{
    scene_ = AmigaScene::menu;
    menuTicks_ = 0;
}

void AmigaSceneRuntime::enterStyleSelect()
{
    scene_ = AmigaScene::styleSelect;
}

void AmigaSceneRuntime::stepSplash(bool firePressed)
{
    if (firePressed)
        splashTimer_ = 0;
    else if (splashTimer_ > 0)
        --splashTimer_;
    if (splashTimer_ != 0)
        return;
    ++splashStage_;
    if (splashStage_ >= 7) {
        splashStage_ = 0;
        enterMenu();
    } else {
        splashTimer_ = splashDuration(splashStage_);
    }
}

void AmigaSceneRuntime::stepMenu(const downwell::Input& input)
{
    ++menuTicks_;
    const bool controlsLocked = menuTicks_ <= 30 || menuTeleport_;
    if (menuNapping_) {
        if (!controlsLocked &&
            (input.left || input.right || input.fire || input.pause))
            menuNapping_ = false;
    } else if (!menuTeleport_) {
        if (!controlsLocked && input.left != input.right) {
            menuSpeedX_ += input.left ? -51 : 51;
            if (input.left && menuSpeedX_ > 0) menuSpeedX_ -= 512;
            if (input.right && menuSpeedX_ < 0) menuSpeedX_ += 512;
            if (menuSpeedX_ < -512) menuSpeedX_ = -512;
            if (menuSpeedX_ > 512) menuSpeedX_ = 512;
            menuFacing_ = input.left ? -1 : 1;
        } else if (menuSpeedX_ < 0) {
            menuSpeedX_ += 102;
            if (menuSpeedX_ > 0) menuSpeedX_ = 0;
        } else if (menuSpeedX_ > 0) {
            menuSpeedX_ -= 102;
            if (menuSpeedX_ < 0) menuSpeedX_ = 0;
        }
        menuX_ += menuSpeedX_;
        if (menuX_ < 16 * fixedOne) menuX_ = 16 * fixedOne;
        if (menuX_ > 400 * fixedOne) menuX_ = 400 * fixedOne;
        if (menuGrounded_ && menuX_ >= menuWellLeft && menuX_ <= menuWellRight)
            menuGrounded_ = false;
        if (menuGrounded_) {
            menuY_ = menuGroundY;
            menuSpeedY_ = 0;
        } else {
            menuSpeedY_ += 51;
            if (menuSpeedY_ > 7 * fixedOne) menuSpeedY_ = 7 * fixedOne;
            menuY_ += menuSpeedY_;
            if (menuY_ >= menuTeleportY) {
                menuY_ = menuTeleportY;
                menuTeleport_ = true;
                menuSpeedX_ = 0;
                menuSpeedY_ = 0;
            }
        }
    }

    const bool inCameraLocker =
        menuX_ >= 238 * fixedOne && menuX_ <= 418 * fixedOne &&
        menuY_ >= 248 * fixedOne && menuY_ <= 648 * fixedOne;
    long targetX = menuX_;
    if (inCameraLocker) {
        targetX = 328 * fixedOne;
    } else if (menuSpeedX_ != 0) {
        targetX += 16 * fixedOne * menuFacing_;
    }
    if (targetX < 80 * fixedOne) targetX = 80 * fixedOne;
    if (targetX > 336 * fixedOne) targetX = 336 * fixedOne;
    const long targetY = inCameraLocker
                             ? menuCameraLockerY_
                             : menuY_ - 32 * fixedOne;
    menuCameraX_ += (targetX - menuCameraX_) /
                    (inCameraLocker ? 15 : 10);
    menuCameraY_ += (targetY - menuCameraY_) / 5;
    if (menuCameraY_ < 142 * fixedOne)
        menuCameraY_ = 142 * fixedOne;

    if (!menuNapping_ && menuTitleDelay_ < 0 && menuTitleImage_ == 0 &&
        menuCameraX_ > 324 * fixedOne && menuCameraX_ < 332 * fixedOne)
        menuTitleDelay_ = 15;
    if (menuTitleDelay_ > 0) {
        --menuTitleDelay_;
    } else if (menuTitleDelay_ == 0 && menuTitleImage_ < 9 * fixedOne) {
        menuTitleImage_ += 82;
        if (menuTitleImage_ >= 9 * fixedOne) {
            menuTitleImage_ = 9 * fixedOne;
            menuTitleReadyTimer_ = 76;
            menuTitleDelay_ = -2;
        }
        menuTitleFrame_ = (int)(menuTitleImage_ >> 8);
    }
    if (menuTitleReadyTimer_ > 0) {
        --menuTitleReadyTimer_;
        if (menuTitleReadyTimer_ == 0)
            menuTitleReady_ = true;
    }

    if (menuTeleport_ && menuTitleReady_ && !menuEnterActive_) {
        if (menuSequenceDelay_ < 0)
            menuSequenceDelay_ = 30;
        else if (menuSequenceDelay_ > 0)
            --menuSequenceDelay_;
        else
            menuEnterActive_ = true;
    }
    if (menuEnterActive_) {
        if (menuCameraLockerY_ < 1000 * fixedOne) {
            menuCameraLockerY_ += menuEnterSpeedY_;
            menuEnterSpeedY_ += 64;
        } else if (menuEnterBlackDelay_ < 0) {
            menuEnterBlackDelay_ = 5;
        } else if (menuEnterBlackDelay_ > 0) {
            --menuEnterBlackDelay_;
        } else {
            enterStyleSelect();
        }
    }
}

void AmigaSceneRuntime::stepStyleSelect(const downwell::Input& input,
                                        bool leftPressed, bool rightPressed,
                                        bool firePressed)
{
    styleYOffset_ -= styleYOffset_ / 15;
    if (styleYOffset_ > -fixedOne && styleYOffset_ < fixedOne)
        styleYOffset_ = 0;
    styleRunFrame_ += 64;
    styleTileY_ -= 6 * fixedOne;
    if (styleTileY_ <= -16 * fixedOne)
        styleTileY_ += 16 * fixedOne;
    styleDitherY_ += 16 * fixedOne;

    if (styleYOffset_ == 0 && !styleSelected_) {
        if (rightPressed && styleCursor_ < 4) ++styleCursor_;
        if (leftPressed && styleCursor_ > 0) --styleCursor_;
        if (firePressed && styleCursor_ == 0)
            styleSelected_ = true;
    }
    const long targetX = styleCursor_ * 48 * fixedOne;
    styleSmoothX_ += (targetX - styleSmoothX_) / 5;
    if (styleSmoothX_ > targetX - fixedOne &&
        styleSmoothX_ < targetX + fixedOne)
        styleSmoothX_ = targetX;

    if (styleSelected_) {
        stylePlayerSpeedY_ += 13;
        stylePlayerDescent_ += stylePlayerSpeedY_;
        if (styleVerticalDitherY_ < 320 * fixedOne)
            styleVerticalDitherY_ += 10 * fixedOne;
        else
            scene_ = AmigaScene::gameplay;
    }
    (void)input;
}

void AmigaSceneRuntime::step(const downwell::Input& input)
{
    const bool leftPressed = input.left && !priorLeft_;
    const bool rightPressed = input.right && !priorRight_;
    const bool firePressed = input.fire && !priorFire_;
    ++tick_;
    if (scene_ == AmigaScene::splash)
        stepSplash(firePressed);
    else if (scene_ == AmigaScene::menu)
        stepMenu(input);
    else if (scene_ == AmigaScene::styleSelect)
        stepStyleSelect(input, leftPressed, rightPressed, firePressed);
    priorLeft_ = input.left;
    priorRight_ = input.right;
    priorFire_ = input.fire;
}
