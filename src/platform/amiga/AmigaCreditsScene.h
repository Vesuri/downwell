#ifndef DOWNWELL_AMIGA_CREDITS_SCENE_H
#define DOWNWELL_AMIGA_CREDITS_SCENE_H
#include "DownwellCopperLists.h"
class AmigaCreditsScene {
    DownwellSplashCopperList list_;
    const UBYTE* frame_ = nullptr;
public:
    AmigaCreditsScene(UWORD* words,unsigned capacity) : list_(words,capacity) {}
    AmigaCreditsScene(const AmigaCreditsScene&) = delete;
    AmigaCreditsScene& operator=(const AmigaCreditsScene&) = delete;
    bool prepare(const Bitmap& frame,const Sprite& blank,const Palette& palette);
    const CopperList* presentation() const { return frame_ ? &list_ : nullptr; }
    const UBYTE* pixels() const { return frame_; }
};
#endif
