#ifndef DOWNWELL_AMIGA_COPPER_LISTS_H
#define DOWNWELL_AMIGA_COPPER_LISTS_H

#include <exec/types.h>
#include "AmigaRenderer.h"
#include "AmigaStyleRenderer.h"
#include "framework/CopperList.h"

class Bitmap;
class Palette;
class Sprite;
struct AmigaMenuScrollState;

enum {
    downwellCopperWords = 400,
    downwellFetchCropBytes = 2,
    downwellFetchWords = 21,
    downwellFetchWidth = downwellFetchWords * 16,
    downwellDisplayHStartCropped = downwellDisplayHStart +
                                   downwellFetchCropBytes * 8,
    downwellDisplayHStopCropped = downwellDisplayHStartCropped +
                                  downwellFetchWidth,
    downwellDisplayVStartCropped = downwellDisplayVStart,
    // The last displayable PAL line is $137, so the 284-row planes' last
    // row stays hidden.
    downwellDisplayVStopCropped = 0x138,
    // A fetch starting before $38 takes sprite 7's DMA slots: its control
    // words still arm it in the blank lines, then its stale data latch
    // repeats down the screen. $38-$D8 is the widest lores fetch that keeps
    // all eight channels: 21 words, logical x=16..351.
    downwellFetchStart = 0x38,
    downwellFetchStop = downwellFetchStart + 8 * (downwellFetchWords - 1),
    downwellFetchModulo = downwellBytesPerPlaneLine -
                           downwellFetchWords * 2
};

/*
 * Scene-specific copper programs follow the Rescue on Fractalus framework
 * model: the application supplies storage, while each class owns the layout
 * and the rules for rebuilding that scene's program.
 */
class DownwellGameplayCopperList : public CopperList {
public:
    DownwellGameplayCopperList(UWORD* words, unsigned capacityWords);

    bool build(const Bitmap& game, const Bitmap& hud,
               const Sprite& playerSprite,
               const Sprite* actorSprites, unsigned actorCount,
               const Sprite& overflowSprite,
               const Palette& palette, unsigned ringOrigin);
    bool update(const Bitmap& game, unsigned ringOrigin);
    // Patch only the inactive list's PF2 pointers for a retained overlay swap.
    bool updateOverlay(const Bitmap& hud);
    bool updateExitMask(const UBYTE* mask);
private:
    unsigned planeOffset_ = 0, wrapWaitOffset_ = 0, overlayOffset_ = 0;
    unsigned exitModeOffset_, exitMaskOffset_;
    const UBYTE* exitMaskData_ = nullptr;
    const void* gameData_ = nullptr;
};

class DownwellMenuCopperList : public CopperList {
public:
    DownwellMenuCopperList(UWORD* words, unsigned capacityWords);

    bool buildScroll(const UBYTE* foreground, const UBYTE* background,
                     const Sprite& playerSprite, const Sprite& nullSprite,
                     const Palette& palette,
                     const AmigaMenuScrollState& scroll, bool entry = false);
    bool updateScroll(const UBYTE* foreground, const UBYTE* background,
                      const UWORD* player, const AmigaMenuScrollState& scroll,
                      bool entry = false);
private:
    unsigned scrollOffset_, foregroundOffset_, backgroundOffset_, playerOffset_;
    unsigned foregroundModuloOffset_;
    unsigned skyWaitOffset_, skyColorOffset_, skyLateWaitOffset_;
    // The sky split's white and black, through the selected palette.
    UWORD skyLight_ = 0xfff, skyDark_ = 0;
};

// Narrow dual-playfield selection display, using retained world-space planes.
// PF1 borders, PF2 text/icons above sprites; common high-bit wipe covers both.
class DownwellSelectionCopperList : public CopperList {
    unsigned borderOffset_=0, layerOffset_=0, maskOffset_=0, modeOffset_=0;
public:
    DownwellSelectionCopperList(UWORD* words,unsigned capacityWords) : CopperList(words,capacityWords) {}
    bool build(const UBYTE* border,const UBYTE* layer,const UBYTE* mask,
        const Sprite& player,const Sprite* actors,unsigned count,const Sprite& overflow,const Palette& palette);
    bool update(const UBYTE* border,const UBYTE* layer,const UBYTE* mask);
};

class DownwellSplashCopperList : public CopperList {
public:
    DownwellSplashCopperList(UWORD* words, unsigned capacityWords);

    bool buildNarrow(const UBYTE* frame, const Sprite& nullSprite,
                     const Palette& palette,
                     const UBYTE* ditherTile = 0);
    bool buildWide(const Bitmap& frame, const Sprite& nullSprite,
                   const Palette& palette);
};

class DownwellStyleCopperList : public CopperList {
public:
    DownwellStyleCopperList(UWORD* words, unsigned capacityWords,
                           unsigned framePlaneBytes = amigaStylePlaneBytes);
    bool build(const UBYTE* frame, const UBYTE* mask,
               const Sprite& nullSprite, const Palette& palette, const UBYTE* border = 0,
               const UWORD* const* sprites = nullptr);
    bool update(const UBYTE* frame, const UBYTE* mask, const UBYTE* border = 0,
                const UWORD* const* sprites = nullptr);
private:
    unsigned frame0Offset_, frame1Offset_, maskOffset_, border0Offset_, border1Offset_, modeOffset_;
    unsigned framePlaneBytes_;
    unsigned spritesOffset_=0,priorityOffset_=0;
    const UWORD* nullSprite_=nullptr;
    const UWORD* spritePointers_[8]{};
    bool spritePriority_=false;
};

#endif
