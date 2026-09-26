#include "AmigaSceneRuntime.h"

namespace {
enum {
    fixedOne = 256,
    fineFixedOne = 65536,
    menuGroundY = 512 * fixedOne,
    menuWellLeft = 300 * fixedOne,
    menuWellRight = 356 * fixedOne,
    menuTeleportY = 625 * fixedOne
};
}

AmigaSceneRuntime::AmigaSceneRuntime()
    : scene_(AmigaScene::splash), tick_(0), priorLeft_(false),
      priorRight_(false), priorFire_(false), splashStage_(1),
      splashTimer_(10), splashDitherTenths_(110),
      menuTicks_(0), menuNapping_(true),
      menuGrounded_(true), menuFallTicks_(0), menuTeleport_(false),
      menuTitleDelay_(-1),
      menuTitleImage_(0), menuTitleRevealTicks_(0),
      menuTitleReadyTimer_(-1),
      menuTitleReady_(false), menuSequenceDelay_(-1),
      menuEnterActive_(false), menuEnterBlackDelay_(-1),
      menuFacing_(1), menuSprite_(12), menuImage_(0),
      menuImageSpeed_(0x3e19999aUL), menuTitleFrame_(0),
      menuX_(112 * fixedOne),
      menuY_(menuGroundY), menuSpeedX_(0), menuSpeedY_(0),
      menuCameraX_(112 * fixedOne), menuCameraY_(512 * fixedOne),
      menuCameraLockerY_(448 * fixedOne), menuEnterSpeedY_(0),
      styleYOffset_(-128 * fixedOne),
      styleTileY_(0), styleDitherY_(80 * fixedOne), styleRunFrame_(0),
      styleSmoothX_(0), styleCursor_(0), styleSelected_(false),
      stylePlayerDescent_(0), stylePlayerSpeedY_(0),
      styleVerticalDitherY_(-320 * fixedOne)
{
#ifdef DOWNWELL_SKIP_TO_GAME
    splashStage_ = 0;
    scene_ = AmigaScene::gameplay;
#elif defined(DOWNWELL_SKIP_INTRO)
    splashStage_ = 0;
    enterStyleSelect();
#endif
}

int AmigaSceneRuntime::roundedPixel(long value)
{
    return value >= 0 ? static_cast<int>((value + 128) >> 8)
                      : -static_cast<int>((-value + 128) >> 8);
}

int AmigaSceneRuntime::roundedFinePixel(long value)
{
    return value >= 0 ? static_cast<int>((value + 32768) >> 16)
                      : -static_cast<int>((-value + 32768) >> 16);
}

unsigned AmigaSceneRuntime::menuPlayerFrame() const
{
    unsigned frame = positiveFloatFloor(menuImage_);
    unsigned count = 1;
    if (menuSprite_ == 12 || menuSprite_ == 27)
        count = 8;
    else if (menuSprite_ == 31)
        count = 4;
    else if (menuSprite_ == 37)
        count = 5;
    return frame % count;
}

int AmigaSceneRuntime::menuTitleY() const
{
    const int rise = menuTitleRevealTicks_ >= 24 ? 3
                     : menuTitleRevealTicks_ >= 15 ? 2
                     : menuTitleRevealTicks_ >= 5 ? 1 : 0;
    return 432 - rise;
}

static std::uint32_t shiftRightJam(std::uint32_t value, unsigned distance)
{
    if (distance == 0)
        return value;
    if (distance >= 32)
        return value != 0;
    const std::uint32_t discarded = value << (32 - distance);
    return (value >> distance) | (discarded != 0);
}

std::uint32_t AmigaSceneRuntime::addPositiveFloat(std::uint32_t left,
                                                   std::uint32_t right)
{
    if (left == 0)
        return right;
    if (right == 0)
        return left;
    unsigned leftExponent = (left >> 23) & 0xffU;
    unsigned rightExponent = (right >> 23) & 0xffU;
    std::uint32_t leftSignificand =
        ((left & 0x7fffffUL) | 0x800000UL) << 3;
    std::uint32_t rightSignificand =
        ((right & 0x7fffffUL) | 0x800000UL) << 3;
    if (leftExponent < rightExponent) {
        const unsigned exponent = leftExponent;
        const std::uint32_t significand = leftSignificand;
        leftExponent = rightExponent;
        leftSignificand = rightSignificand;
        rightExponent = exponent;
        rightSignificand = significand;
    }
    rightSignificand = shiftRightJam(
        rightSignificand, leftExponent - rightExponent);
    std::uint32_t result = leftSignificand + rightSignificand;
    if ((result & 0x08000000UL) != 0) {
        result = shiftRightJam(result, 1);
        ++leftExponent;
    }
    const unsigned roundBits = result & 7U;
    result >>= 3;
    if (roundBits > 4U || (roundBits == 4U && (result & 1U) != 0))
        ++result;
    if ((result & 0x01000000UL) != 0) {
        result >>= 1;
        ++leftExponent;
    }
    return (leftExponent << 23) | (result & 0x7fffffUL);
}

unsigned AmigaSceneRuntime::positiveFloatFloor(std::uint32_t value)
{
    const unsigned exponent = (value >> 23) & 0xffU;
    if (exponent < 127)
        return 0;
    const unsigned shift = exponent - 127;
    const std::uint32_t significand =
        (value & 0x7fffffUL) | 0x800000UL;
    if (shift >= 23)
        return significand << (shift - 23);
    return significand >> (23 - shift);
}

std::uint32_t AmigaSceneRuntime::subtractEight(std::uint32_t value)
{
    unsigned exponent = (value >> 23) & 0xffU;
    std::uint32_t significand =
        (value & 0x7fffffUL) | 0x800000UL;
    if (exponent != 130 || significand < 0x800000UL)
        return value;
    significand -= 0x800000UL;
    if (significand == 0)
        return 0;
    while ((significand & 0x800000UL) == 0) {
        significand <<= 1;
        --exponent;
    }
    return (exponent << 23) | (significand & 0x7fffffUL);
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
    menuNapping_ = true;
    menuGrounded_ = true;
    menuFallTicks_ = 0;
    menuTeleport_ = false;
    menuTitleDelay_ = -1;
    menuTitleImage_ = 0;
    menuTitleRevealTicks_ = 0;
    menuTitleReadyTimer_ = -1;
    menuTitleReady_ = false;
    menuSequenceDelay_ = -1;
    menuEnterActive_ = false;
    menuEnterBlackDelay_ = -1;
    menuFacing_ = 1;
    menuSprite_ = 12;
    menuImage_ = 0;
    menuImageSpeed_ = 0x3e19999aUL;
    menuTitleFrame_ = 0;
    menuX_ = 112 * fixedOne;
    menuY_ = menuGroundY;
    menuSpeedX_ = menuSpeedY_ = 0;
    menuCameraX_ = 112 * fixedOne;
    menuCameraY_ = 512 * fixedOne;
    menuCameraLockerY_ = 448 * fixedOne;
    menuEnterSpeedY_ = 0;
}

void AmigaSceneRuntime::enterStyleSelect()
{
    scene_ = AmigaScene::styleSelect;
    styleYOffset_ = -128 * fixedOne;
    styleTileY_ = 0;
    styleDitherY_ = 80 * fixedOne;
    styleRunFrame_ = 0;
    styleSmoothX_ = 0;
    styleCursor_ = 0;
    styleSelected_ = false;
    stylePlayerDescent_ = stylePlayerSpeedY_ = 0;
    styleVerticalDitherY_ = -320 * fixedOne;
}

bool AmigaSceneRuntime::enterResultsDestination(downwell::ResultsDestination destination)
{
    switch (destination) {
    case downwell::ResultsDestination::surface: enterMenu(); break;
    case downwell::ResultsDestination::styleSelect: enterStyleSelect(); break;
    case downwell::ResultsDestination::gameplay: scene_ = AmigaScene::gameplay; break;
    default: return false;
    }
    // Preserve physical key history: holding the confirmation key across a
    // room change is not a new press in the destination room.
    splashStage_ = 0;
    return true;
}

void AmigaSceneRuntime::stepSplash(bool firePressed)
{
    if (splashStage_ == 3 || splashStage_ == 4)
        splashDitherTenths_ = 110;
    else if (splashStage_ == 5 && splashDitherTenths_ >= 10)
        splashDitherTenths_ -= 3;
    else if (splashStage_ == 6 && splashDitherTenths_ <= 110)
        splashDitherTenths_ += 3;
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
    const bool teleportWasActive = menuTeleport_;
    const bool titleWasReady = menuTitleReady_;
    const bool controlsLocked = menuTicks_ <= 30 || menuTeleport_;
    if (menuNapping_) {
        if (!controlsLocked &&
            (input.left || input.right || input.fire || input.pause)) {
            menuNapping_ = false;
            menuY_ = menuGroundY;
            menuSprite_ = 31;
            menuImage_ = 0;
            menuImageSpeed_ = 0x3dcccccdUL;
            menuGrounded_ = true;
            menuFallTicks_ = 0;
            menuSpeedY_ = 0;
        }
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
            menuFallTicks_ = 0;
            menuY_ = menuGroundY;
            menuSpeedY_ = 0;
        } else {
            ++menuFallTicks_;
            menuSpeedY_ += 51;
            if (menuSpeedY_ > 7 * fixedOne) menuSpeedY_ = 7 * fixedOne;
            menuY_ += menuSpeedY_;
            if (menuY_ > menuGroundY) {
                if (menuX_ < menuWellLeft)
                    menuX_ = menuWellLeft;
                else if (menuX_ > menuWellRight)
                    menuX_ = menuWellRight;
            }
            if (menuY_ >= menuTeleportY) {
                menuTeleport_ = true;
                menuSpeedX_ = 0;
                menuSpeedY_ = 0;
            }
        }

    }

    if (!menuNapping_) {
        if (!menuGrounded_) {
            menuSprite_ = 37;
            menuImageSpeed_ = 0;
            if (teleportWasActive || menuFallTicks_ < 5)
                menuImage_ = 0x40000000UL;
            else if (menuFallTicks_ < 10)
                menuImage_ = 0x40400000UL;
            else
                menuImage_ = 0x40800000UL;
        } else {
            menuSprite_ = menuSpeedX_ == 0 ? 31 : 27;
            menuImageSpeed_ = menuSpeedX_ == 0
                                  ? 0x3dcccccdUL
                                  : 0x3e99999aUL;
        }
    }

    if (menuImageSpeed_ != 0)
        menuImage_ = addPositiveFloat(menuImage_, menuImageSpeed_);
    if (!menuNapping_ && menuGrounded_ && menuSprite_ == 27 &&
        positiveFloatFloor(menuImage_) >= 8)
        menuImage_ = subtractEight(menuImage_);

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
    } else if (menuTitleDelay_ == 0 &&
               positiveFloatFloor(menuTitleImage_) < 9) {
        menuTitleImage_ = addPositiveFloat(menuTitleImage_, 0x3ea3d70aUL);
        ++menuTitleRevealTicks_;
        if (positiveFloatFloor(menuTitleImage_) >= 9) {
            menuTitleImage_ = 0x41100000UL;
            menuTitleReadyTimer_ = 76;
            menuTitleDelay_ = -2;
        }
        menuTitleFrame_ = (int)positiveFloatFloor(menuTitleImage_);
    }
    if (menuTitleReadyTimer_ > 0) {
        --menuTitleReadyTimer_;
        if (menuTitleReadyTimer_ == 0)
            menuTitleReady_ = true;
    }

    const bool enterWasActive = menuEnterActive_;
    // Teleport Begin Step observes the previous alarm phase's titleOk.
    // Its newly assigned Alarm 1 is decremented in this update, and fires
    // on reaching zero, rather than waiting one extra update at zero.
    if (teleportWasActive && titleWasReady && !menuEnterActive_) {
        if (menuSequenceDelay_ < 0)
            menuSequenceDelay_ = 30;
        if (menuSequenceDelay_ > 0 && --menuSequenceDelay_ == 0)
            menuEnterActive_ = true;
    }
    if (enterWasActive) {
        if (menuCameraLockerY_ < 1000 * fixedOne) {
            menuCameraLockerY_ += menuEnterSpeedY_;
            menuEnterSpeedY_ += 64;
        } else if (menuEnterBlackDelay_ < 0) {
            menuEnterBlackDelay_ = 5;
        } else if (menuEnterBlackDelay_ > 0) {
            --menuEnterBlackDelay_;
            if (menuEnterBlackDelay_ == 0)
                enterStyleSelect();
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
        stylePlayerSpeedY_ += fineFixedOne / 20;
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
