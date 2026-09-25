#ifndef DOWNWELL_AMIGA_PUBLISHER_SCENE_H
#define DOWNWELL_AMIGA_PUBLISHER_SCENE_H
#include "DownwellCopperLists.h"
class AmigaPublisherScene {
    DownwellSplashCopperList list_;
    const UBYTE* frame_ = nullptr;
public:
    AmigaPublisherScene(UWORD* words,unsigned capacity) : list_(words,capacity) {}
    AmigaPublisherScene(const AmigaPublisherScene&) = delete;
    AmigaPublisherScene& operator=(const AmigaPublisherScene&) = delete;
    bool prepare(const UBYTE* cache,unsigned bytes,const Sprite& blank,const Palette& palette);
    const CopperList* presentation() const { return frame_ ? &list_ : nullptr; }
    const UBYTE* pixels() const { return frame_; }
};
#endif
