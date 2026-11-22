#include "AmigaSceneRuntime.h"

namespace {
enum {
    fixedOne = 256,
    fineFixedOne = 65536,
    menuGroundY = 512 * fixedOne,
    menuTeleportY = 625 * fixedOne
};
}

AmigaSceneRuntime::AmigaSceneRuntime()
    : scene_(AmigaScene::splash), tick_(0), priorLeft_(false),
      priorRight_(false), priorFire_(false), splashStage_(1),
      splashTimer_(10), splashDitherTenths_(110),
      menuTicks_(0), menuNapping_(true),
      menuGrounded_(true), menuTeleport_(false),
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
    if (!menuNapping_) return garden_.spriteFrame();
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
    // PlayMenu Create: cursorAt = global.playStyle, smoothx = 0.
    styleSmoothX_ = 0;
    styleCursor_ = int(playStyle_);
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

// soundFootstep/soundLand: soundPlayOL(global.sfxFs or sfxJl of the surface
// [irandom_range(1, 4)], 60, 0, 1, "footsteps"). groundRoom() keeps the
// footstep's full gain.
void AmigaSceneRuntime::playGardenTable(int first)
{
    gardenSoundRandom_ ^= gardenSoundRandom_ << 13;
    gardenSoundRandom_ ^= gardenSoundRandom_ >> 17;
    gardenSoundRandom_ ^= gardenSoundRandom_ << 5;
    sounds_.playOL(first + static_cast<int>(gardenSoundRandom_ % 4u), 60, false, true, "footsteps");
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
            // playerSpriteControl's nap wake: napping = 0 where it slept,
            // then soundLand while grounded.
            garden_.wake(roundedPixel(menuX_), roundedPixel(menuY_), menuFacing_, input.fire);
            menuSprite_ = garden_.spriteId();
            menuGrounded_ = true;
            menuSpeedX_ = menuSpeedY_ = 0;
            playGardenTable(downwell::GardenGround::landingSound(
                downwell::GardenGround::under(garden_.x(), garden_.y())));
        }
    } else if (!menuTeleport_) {
        // W1.14: the full objPlayer_n Step (downwell::GardenMotion).
        downwell::GardenMotion::Sound played[4];
        const unsigned count = garden_.step(input, controlsLocked, played);
        for (unsigned i = 0; i < count; ++i) {
            if (played[i].table) playGardenTable(played[i].first);
            else sounds_.play(played[i].first, played[i].priority, false, true);
        }
        menuX_ = garden_.xFixed();
        menuY_ = garden_.yFixed();
        menuSpeedX_ = garden_.speedXFixed();
        menuFacing_ = garden_.facing();
        menuGrounded_ = garden_.grounded();
        menuSprite_ = garden_.spriteId();
        // objTeleportMain's 160x32 touch area spans the bottom of the shaft.
        if (menuY_ >= menuTeleportY && menuX_ >= 240 * fixedOne && menuX_ <= 400 * fixedOne) {
            menuTeleport_ = true;
            menuSpeedX_ = 0;
            menuSpeedY_ = 0;
        }
    }
    if (menuNapping_ && menuImageSpeed_ != 0)
        menuImage_ = addPositiveFloat(menuImage_, menuImageSpeed_);

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
        menuCameraX_ > 324 * fixedOne && menuCameraX_ < 332 * fixedOne) {
        menuTitleDelay_ = 15;
        if (!firstBoot_) { sounds_.play(188, 80, false, true); firstBoot_ = true; }
    }
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
        if (menuSequenceDelay_ > 0 && --menuSequenceDelay_ == 0) {
            menuEnterActive_ = true;
            sounds_.play(360, 80, false, true); // EnterSequence Create
        }
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

    // PlayMenu Step: past either end the carousel nudges by 4; left at the
    // first style toggles hard mode once it is unlocked. Locked styles
    // ignore confirmation.
    if (styleYOffset_ == 0 && !styleSelected_) {
        if (rightPressed) {
            if (styleCursor_ < 4) { ++styleCursor_; sounds_.playOL(331, 90, false, true, "UI"); }
            else { styleSmoothX_ += 4 * fixedOne; sounds_.playOL(333, 90, false, true, "UI"); }
        }
        if (leftPressed) {
            if (styleCursor_ > 0) { --styleCursor_; sounds_.playOL(331, 90, false, true, "UI"); }
            else {
                styleSmoothX_ -= 4 * fixedOne;
                if (hardUnlocked_) hardMode_ = !hardMode_;
                if (hardMode_) sounds_.playOL(320, 90, false, true, "UI");
                else { sounds_.stop(320); sounds_.playOL(333, 90, false, true, "UI"); }
            }
        }
        if (firePressed && unsigned(styleCursor_) <= styleUnlock_) {
            playStyle_ = unsigned(styleCursor_);
            styleSelected_ = true;
            styleConfirmed_ = true;
            sounds_.playOL(332, 90, false, true, "UI");
        }
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
    step(input,{input.right&&!priorRight_,input.left&&!priorLeft_,input.fire&&!priorFire_,false});
}

void AmigaSceneRuntime::step(const downwell::Input& input,const downwell::ShopInteraction::Input& edges)
{
    ++tick_;
    sounds_.step(tick_);
    if (scene_ == AmigaScene::splash)
        stepSplash(edges.confirm);
    else if (scene_ == AmigaScene::menu)
        stepMenu(input);
    else if (scene_ == AmigaScene::styleSelect)
        stepStyleSelect(input, edges.left, edges.right, edges.confirm);
    priorLeft_ = input.left;
    priorRight_ = input.right;
    priorFire_ = input.fire;
}
