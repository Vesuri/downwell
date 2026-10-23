#ifndef DOWNWELL_AMIGA_SLICE_END_SCENE_H
#define DOWNWELL_AMIGA_SLICE_END_SCENE_H
#include "DownwellCopperLists.h"
#include "framework/Bitmap.h"

// Deliberate port boundary, not an original-game room. Reuses the retired
// clear screen's storage and the credits copper slot; no area-2 bank loads.
class AmigaSliceEndScene {
    DownwellSplashCopperList list_;
public:
    bool presented = false;
    AmigaSliceEndScene(UWORD* words, unsigned capacity) : list_(words,capacity) {}
    bool prepare(UBYTE* game, UBYTE* hud, const Bitmap& bitmap,
        const UBYTE* pack, unsigned bytes, const Sprite& blank, const Palette& palette) {
        for (unsigned id=64700;id<=64701;++id) {
            int x,y,w,h;
            if (!amigaRendererAssetBounds(pack,pack+bytes,id,0,0,0,0,&x,&y,&w,&h)) return false;
        }
        AmigaRenderer target;
        amigaRendererInit(&target,game,hud);
        amigaRendererClear(&target,0);
        amigaRendererSetClipRect(&target,0,0,downwellVisibleWidth,downwellDisplayHeight);
        amigaRendererDrawAsset(&target,pack,pack+bytes,64700,0,190,100,0);
        amigaRendererDrawAsset(&target,pack,pack+bytes,64701,0,190,128,0);
        return list_.buildWide(bitmap,blank,palette);
    }
    const CopperList& presentation() const { return list_; }
};
#endif
