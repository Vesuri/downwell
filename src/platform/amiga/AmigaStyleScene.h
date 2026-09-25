#ifndef DOWNWELL_AMIGA_STYLE_SCENE_H
#define DOWNWELL_AMIGA_STYLE_SCENE_H
#include "AmigaStyleRenderer.h"
#include "DownwellCopperLists.h"

// Owns style presentation state, not the shared startup/selection rules.
// The application retires DMA before prepare() and publishes only the retired
// slot returned by copper(). Cache and Copper storage are borrowed Chip RAM.
class AmigaStyleScene {
public:
    AmigaStyleScene(UBYTE* cache, unsigned cacheBytes, UWORD* firstList,
                   UWORD* secondList, unsigned listWords);
    AmigaStyleScene(const AmigaStyleScene&) = delete;
    AmigaStyleScene& operator=(const AmigaStyleScene&) = delete;
    bool prepare(const UBYTE* pack, const UBYTE* end);
    struct Frame {
        const UBYTE* content = nullptr;
        const UBYTE* border = nullptr;
        const UBYTE* mask = nullptr;
    };
    bool render(unsigned slot, const AmigaSceneRuntime& scenes, Frame& frame);
    bool updateCopper(unsigned slot, const Frame& frame,
                      const Sprite& nullSprite, const Palette& palette);
    DownwellStyleCopperList& copper(unsigned slot) { return lists_[slot]; }
private:
    UBYTE* cache_;
    unsigned cacheBytes_;
    const UBYTE* pack_ = nullptr;
    const UBYTE* end_ = nullptr;
    bool prepared_ = false;
    AmigaStyleFrameState frames_[2];
    DownwellStyleCopperList lists_[2];
    bool built_[2] = {false, false};
};
#endif
