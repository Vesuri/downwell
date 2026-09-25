#ifndef DOWNWELL_AMIGA_GARDEN_SCENE_H
#define DOWNWELL_AMIGA_GARDEN_SCENE_H
#include "DownwellCopperLists.h"
#include "AmigaMenuScroll.h"
class AmigaSceneRuntime;
struct AmigaMenuDrawList;
// Prepared parallax artwork, title updates, offline sprite selection and
// retired-slot Copper patches. Caller retires DMA before prepare()/reuse.
class AmigaGardenScene {
    UBYTE *foreground_, *background_, *entry_;
    const UBYTE *pack_ = nullptr, *end_ = nullptr;
    DownwellMenuCopperList lists_[2];
    bool ready_ = false, built_[2] = {false,false};
    int title_ = -1;
public:
    AmigaGardenScene(UBYTE* foreground,UBYTE* background,UBYTE* entry,
        UWORD* first,UWORD* second,unsigned capacity);
    AmigaGardenScene(const AmigaGardenScene&) = delete;
    AmigaGardenScene& operator=(const AmigaGardenScene&) = delete;
    bool prepare(const AmigaMenuDrawList& list,const UBYTE* pack,const UBYTE* end);
    void enter() { built_[0]=built_[1]=false; }
    struct Frame { UWORD* player = nullptr; AmigaMenuScrollState scroll{}; };
    bool render(unsigned slot,const AmigaSceneRuntime& scenes,bool entry,
        const Sprite& blank,const Palette& palette,Frame& frame);
    DownwellMenuCopperList& copper(unsigned slot) { return lists_[slot]; }
};
#endif
