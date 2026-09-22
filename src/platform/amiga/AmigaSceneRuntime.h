#ifndef DOWNWELL_AMIGA_SCENE_RUNTIME_H
#define DOWNWELL_AMIGA_SCENE_RUNTIME_H

#include "core/Input.h"

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

    AmigaScene scene() const { return scene_; }
    unsigned tick() const { return tick_; }
    int splashStage() const { return splashStage_; }
    bool menuPlayerNapping() const { return menuNapping_; }
    bool menuPlayerVisible() const { return !menuTeleport_; }
    int menuPlayerX() const { return roundedPixel(menuX_); }
    int menuPlayerY() const { return roundedPixel(menuY_); }
    int menuPlayerFacing() const { return menuFacing_; }
    int menuCameraX() const { return roundedPixel(menuCameraX_); }
    int menuCameraY() const { return roundedPixel(menuCameraY_); }
    int menuTitleFrame() const { return menuTitleFrame_; }
    bool menuTeleportActive() const { return menuTeleport_; }
    int styleYOffset() const { return roundedPixel(styleYOffset_); }
    int styleTileY() const { return roundedPixel(styleTileY_); }
    int styleDitherY() const { return roundedPixel(styleDitherY_); }
    int styleSmoothX() const { return roundedPixel(styleSmoothX_); }
    int styleCursor() const { return styleCursor_; }
    bool styleSelected() const { return styleSelected_; }
    int stylePlayerDescent() const { return roundedPixel(stylePlayerDescent_); }
    int styleVerticalDitherY() const {
        return roundedPixel(styleVerticalDitherY_);
    }
    unsigned styleRunFrame() const { return (styleRunFrame_ >> 8) & 7U; }

private:
    static int roundedPixel(long value);
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

    unsigned menuTicks_;
    bool menuNapping_;
    bool menuGrounded_;
    bool menuTeleport_;
    int menuTeleportTimer_;
    int menuFacing_;
    int menuTitleFrame_;
    long menuX_;
    long menuY_;
    long menuSpeedX_;
    long menuSpeedY_;
    long menuCameraX_;
    long menuCameraY_;
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
};

#endif
