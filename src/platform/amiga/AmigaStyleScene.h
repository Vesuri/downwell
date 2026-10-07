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
    // Begin changes bookkeeping only. Service after the previous cache owner
    // has retired DMA; each call completes/drains one preparation block.
    bool beginPrepare(const UBYTE* pack, const UBYTE* end);
    bool preparePart();
    struct Frame {
        const UBYTE* content = nullptr;
        const UBYTE* border = nullptr;
        const UBYTE* mask = nullptr;
        unsigned paintedPixels = 0;
        const UWORD* sprites[8]{};
        bool hardware = false;
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
    bool preparing_ = false;
    AmigaStylePreparation preparation_;
    UBYTE* targets_[2];
    unsigned frameBytes_;
    AmigaSelectionLayer layers_[2];
    AmigaStyleDrawList drawList_;
    DownwellStyleCopperList lists_[2];
    bool built_[2] = {false, false};
    struct Icon { UWORD* data[2]{}; int x=0,y=0,height=0; } icons_[14];
    bool bindIcons();
    bool prepareIcons(unsigned slot,int top,Frame& frame);
};
#endif
