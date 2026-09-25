#ifndef DOWNWELL_AMIGA_CONTROLS_SCENE_H
#define DOWNWELL_AMIGA_CONTROLS_SCENE_H
#include "DownwellCopperLists.h"
// Holds the controls page and both dither phases. List storage is borrowed:
// the caller passes only a retired alternating slot and publishes via platform.
// Timers/input remain in the existing source-backed startup controller.
class AmigaControlsScene {
    DownwellSplashCopperList hold_, fades_[2];
    const UBYTE* cache_ = nullptr;
public:
    AmigaControlsScene(UWORD* hold,UWORD* first,UWORD* second,unsigned capacity)
        : hold_(hold,capacity),fades_{DownwellSplashCopperList(first,capacity),DownwellSplashCopperList(second,capacity)} {}
    AmigaControlsScene(const AmigaControlsScene&) = delete;
    AmigaControlsScene& operator=(const AmigaControlsScene&) = delete;
    bool prepare(const UBYTE* cache,unsigned bytes,const Sprite& blank,const Palette& palette);
    const CopperList* presentation(int stage,unsigned dither,unsigned slot,
        const Sprite& blank,const Palette& palette,bool& alternating);
    const UBYTE* pixels() const;
};
#endif
