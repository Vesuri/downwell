#ifndef DOWNWELL_AMIGA_SCENE_RUNTIME_H
#define DOWNWELL_AMIGA_SCENE_RUNTIME_H

#include "core/Input.h"
#include "core/ShopInteraction.h"
#include "core/ResultsDestination.h"
#include "core/FrontEndSounds.h"
#include "core/GardenMotion.h"
#include <cstdint>

enum class AmigaScene {
    splash,
    menu,
    styleSelect,
    gameplay
};

class AmigaSceneRuntime {
public:
    AmigaSceneRuntime();
    void step(const downwell::Input& input);
    void step(const downwell::Input& input,const downwell::ShopInteraction::Input& edges);
    // Scene-local state only; caller must retire DMA and restore resources,
    // reset the gameplay world and apply the shared RunSession restart.
    bool enterResultsDestination(downwell::ResultsDestination destination);

    AmigaScene scene() const { return scene_; }
    unsigned tick() const { return tick_; }
    int splashStage() const { return splashStage_; }
    unsigned splashDitherFrame() const {
        return (unsigned)(splashDitherTenths_ / 10);
    }
    bool menuPlayerNapping() const { return menuNapping_; }
    bool menuPlayerVisible() const { return !menuTeleport_; }
    int menuPlayerX() const { return roundedPixel(menuX_); }
    int menuPlayerY() const { return roundedPixel(menuY_); }
    int menuPlayerFacing() const { return menuFacing_; }
    int menuPlayerSprite() const { return menuSprite_; }
    unsigned menuPlayerFrame() const;
    int menuCameraX() const { return roundedPixel(menuCameraX_); }
    int menuCameraY() const { return roundedPixel(menuCameraY_); }
    int menuTitleFrame() const { return menuTitleFrame_; }
    int menuTitleY() const;
    bool menuTeleportActive() const { return menuTeleport_; }
    bool menuEnterSequenceActive() const { return menuEnterActive_; }
    int menuEnterSequenceY() const { return roundedPixel(menuCameraLockerY_); }
    int styleYOffset() const { return roundedPixel(styleYOffset_); }
    int styleTileY() const { return roundedPixel(styleTileY_); }
    int styleDitherY() const { return roundedPixel(styleDitherY_); }
    int styleSmoothX() const { return roundedPixel(styleSmoothX_); }
    int styleCursor() const { return styleCursor_; }
    bool styleSelected() const { return styleSelected_; }
    int stylePlayerDescent() const {
        return roundedFinePixel(stylePlayerDescent_);
    }
    int styleVerticalDitherY() const {
        return roundedPixel(styleVerticalDitherY_);
    }
    unsigned styleRunFrame() const { return (styleRunFrame_ >> 8) & 7U; }
    // W1.10 PlayMenu: global.styleUnlock, hardUnlocked, playStyle and
    // hardMode from the save. Style select starts at the saved style.
    void configureStyles(unsigned styleUnlock, bool hardUnlocked, unsigned style, bool hard) {
        styleUnlock_ = styleUnlock > 4 ? 4 : styleUnlock; hardUnlocked_ = hardUnlocked;
        playStyle_ = style > styleUnlock_ ? 0 : style; hardMode_ = hard && hardUnlocked;
    }
    bool styleUnlocked(int style) const { return style >= 0 && unsigned(style) <= styleUnlock_; }
    bool hardUnlocked() const { return hardUnlocked_; }
    bool hardMode() const { return hardMode_; }
    unsigned playStyle() const { return playStyle_; }
    // One-shot: PlayMenu confirmed (ini_write_real style and hardMode).
    bool takeStyleConfirmed() { const bool value = styleConfirmed_; styleConfirmed_ = false; return value; }
    // W2.3: the front end's sound requests (drained by the backend).
    downwell::FrontEndSounds& sounds() { return sounds_; }

private:
    static int roundedPixel(long value);
    static int roundedFinePixel(long value);
    static std::uint32_t addPositiveFloat(std::uint32_t left,
                                          std::uint32_t right);
    static unsigned positiveFloatFloor(std::uint32_t value);
    static std::uint32_t subtractEight(std::uint32_t value);
    static int splashDuration(int stage);
    void enterMenu();
    void enterStyleSelect();
    void stepSplash(bool firePressed);
    void stepMenu(const downwell::Input& input);
    void stepStyleSelect(const downwell::Input& input,
                         bool leftPressed, bool rightPressed,
                         bool firePressed);

    AmigaScene scene_;
    unsigned tick_;
    bool priorLeft_;
    bool priorRight_;
    bool priorFire_;
    int splashStage_;
    int splashTimer_;
    int splashDitherTenths_;

    unsigned menuTicks_;
    bool menuNapping_;
    bool menuGrounded_;
    bool menuTeleport_;
    int menuTitleDelay_;
    std::uint32_t menuTitleImage_;
    unsigned menuTitleRevealTicks_;
    int menuTitleReadyTimer_;
    bool menuTitleReady_;
    int menuSequenceDelay_;
    bool menuEnterActive_;
    int menuEnterBlackDelay_;
    int menuFacing_;
    int menuSprite_;
    std::uint32_t menuImage_;
    std::uint32_t menuImageSpeed_;
    int menuTitleFrame_;
    long menuX_;
    long menuY_;
    long menuSpeedX_;
    long menuSpeedY_;
    long menuCameraX_;
    long menuCameraY_;
    long menuCameraLockerY_;
    long menuEnterSpeedY_;

    long styleYOffset_;
    long styleTileY_;
    long styleDitherY_;
    long styleRunFrame_;
    long styleSmoothX_;
    int styleCursor_;
    bool styleSelected_;
    long stylePlayerDescent_;
    long stylePlayerSpeedY_;
    long styleVerticalDitherY_;
    unsigned styleUnlock_ = 0, playStyle_ = 0;
    bool hardUnlocked_ = false, hardMode_ = false, styleConfirmed_ = false;
    bool firstBoot_ = false; // global.firstBoot: the title sting plays once
    downwell::FrontEndSounds sounds_;
    // W2.6: the garden's footstep and landing sample choice. The front end
    // has no game RNG (the first run starts from the builder checkpoint), so
    // irandom_range(1, 4) uses its own stream; it only picks the sample.
    std::uint32_t gardenSoundRandom_ = 0x2545f491u;
    void playGardenTable(int first);
    // W1.14: the awake garden player (objPlayer_n's Step on rmMenu's solids).
    downwell::GardenMotion garden_;
};

#endif
