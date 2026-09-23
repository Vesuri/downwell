#ifndef DOWNWELL_AMIGA_COPPER_LISTS_H
#define DOWNWELL_AMIGA_COPPER_LISTS_H

#include <exec/types.h>
#include "AmigaRenderer.h"
#include "framework/CopperList.h"

class Bitmap;
class Palette;
class Sprite;
struct AmigaMenuScrollState;

enum {
    downwellCopperWords = 400,
    downwellFetchCropBytes = 2,
    downwellFetchWords = 22,
    downwellFetchWidth = downwellFetchWords * 16,
    downwellDisplayHStartCropped = downwellDisplayHStart +
                                   downwellFetchCropBytes * 8,
    downwellDisplayHStopCropped = downwellDisplayHStartCropped +
                                  downwellFetchWidth,
    downwellDisplayVStartCropped = downwellDisplayVStart,
    downwellDisplayVStopCropped = downwellDisplayVStartCropped +
                                  downwellDisplayHeight,
    downwellFetchStart = 0x30,
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
};

class DownwellMenuCopperList : public CopperList {
public:
    DownwellMenuCopperList(UWORD* words, unsigned capacityWords);

    bool buildScroll(const UBYTE* foreground, const UBYTE* background,
                     const Sprite& playerSprite, const Sprite& nullSprite,
                     const Palette& palette,
                     const AmigaMenuScrollState& scroll);
    bool buildEntry(const UBYTE* entry, const Sprite& nullSprite,
                    const Palette& palette, int viewY);
    bool updateScroll(const UBYTE* foreground, const UBYTE* background,
                      const UWORD* player, const AmigaMenuScrollState& scroll);
    bool updateEntry(const UBYTE* entry, int viewY);
private:
    unsigned scrollOffset_, foregroundOffset_, backgroundOffset_, playerOffset_;
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

#endif
