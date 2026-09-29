#ifndef DOWNWELL_AMIGA_STYLE_SCENE_H
#define DOWNWELL_AMIGA_STYLE_SCENE_H
#include "AmigaStyleRenderer.h"
#include "DownwellCopperLists.h"
#include "AmigaStyleDrawList.h"

// Owns style presentation state, not the shared startup/selection rules.
// The application retires DMA before prepare() and publishes only the retired
// slot returned by copper(). Cache and Copper storage are borrowed Chip RAM.
class AmigaStyleScene {
public:
    AmigaStyleScene(UBYTE* cache, unsigned cacheBytes, UWORD* firstList,
                   UWORD* secondList, unsigned listWords,
                   UBYTE* firstFrame, UBYTE* secondFrame, unsigned frameBytes);
    AmigaStyleScene(const AmigaStyleScene&) = delete;
    AmigaStyleScene& operator=(const AmigaStyleScene&) = delete;
    // Preserve only when the caller proves no intervening owner wrote cache_.
    // Presentation canvases and borrowed Copper slots are always invalidated.
    bool prepare(const UBYTE* pack, const UBYTE* end, bool borderPreserved = false);
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
    UBYTE* targets_[2];
    unsigned frameBytes_;
    AmigaSelectionLayer layers_[2];
    AmigaStyleDrawList drawList_;
    DownwellStyleCopperList lists_[2];
    bool built_[2] = {false, false};
};
#endif
