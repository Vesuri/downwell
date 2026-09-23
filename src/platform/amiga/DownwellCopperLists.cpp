#include <hardware/custom.h>
#include "AmigaMenu.h"
#include "AmigaMenuCache.h"
#include "AmigaMenuScroll.h"
#include "AmigaSplashCache.h"
#include "AmigaStyleRenderer.h"
#include "DownwellCopperLists.h"
#include "framework/AmigaHardware.h"
#include "framework/Bitmap.h"
#include "framework/Palette.h"
#include "framework/Sprite.h"

static bool beginCroppedDisplay(CopperList& list)
{
    list.reset();
    return list.move(diwstrt,
                     (UWORD)((downwellDisplayVStartCropped << 8) |
                             (downwellDisplayHStartCropped & 0xff))) &&
           list.move(diwstop,
                     (UWORD)(((downwellDisplayVStopCropped & 0xff) << 8) |
                             (downwellDisplayHStopCropped & 0xff))) &&
           list.move(ddfstrt, downwellFetchStart) &&
           list.move(ddfstop, downwellFetchStop);
}

static bool beginNarrowDisplay(CopperList& list, bool staticFrame = false)
{
    list.reset();
    return list.move(diwstrt,
                     (UWORD)((downwellDisplayVStartCropped << 8) |
                             (amigaMenuDisplayHStart & 0xff))) &&
           list.move(diwstop,
                     (UWORD)(((downwellDisplayVStopCropped & 0xff) << 8) |
                             (amigaMenuDisplayHStop & 0xff))) &&
           // Only scrolling needs the extra word before the visible window.
           list.move(ddfstrt, staticFrame ? amigaMenuFetchStart + 8
                                         : amigaMenuFetchStart) &&
           list.move(ddfstop, amigaMenuFetchStop);
}

static bool hideSprites(CopperList& list, const Sprite& nullSprite,
                        unsigned first = 0)
{
    for (unsigned channel = first; channel < 8; ++channel) {
        if (!list.showSprite((UWORD)channel, nullSprite)) return false;
    }
    return true;
}

static bool buildNarrow(CopperList& list, const UBYTE* frame,
                        const Sprite& nullSprite, const Palette& palette,
                        const UBYTE* ditherTile)
{
    if (!beginNarrowDisplay(list, true) ||
        !list.move(bplcon0, ditherTile ? 0x3001 : 0x2001) ||
        !list.move(bplcon1, 0) ||
        !list.move(bpl1mod, amigaSplashBytesPerRow - amigaSplashVisibleWidth / 8) ||
        !list.move(bpl2mod, amigaSplashBytesPerRow - amigaSplashVisibleWidth / 8) ||
        !list.pointer(bpl1pth, frame) ||
        !list.pointer(bpl2pth, frame + amigaStylePlaneBytes) ||
        (ditherTile && !list.pointer(bpl3pth, ditherTile)) ||
        !hideSprites(list, nullSprite) ||
        !list.setPalette(palette))
        return false;

    if (ditherTile) {
        bool crossedLine255 = false;
        for (unsigned line = amigaSplashDitherTileHeight;
             line < amigaSplashHeight;
             line += amigaSplashDitherTileHeight) {
            const unsigned resetAfterLine =
                downwellDisplayVStartCropped + line - 1;
            if (resetAfterLine > 255 && !crossedLine255) {
                if (!list.wait(255, 0xdf)) return false;
                crossedLine255 = true;
            }
            if (!list.wait((UWORD)resetAfterLine, 0xdf) ||
                !list.pointer(bpl3pth, ditherTile))
                return false;
        }
    }
    return list.finish();
}

DownwellGameplayCopperList::DownwellGameplayCopperList(
    UWORD* words, unsigned capacityWords)
    : CopperList(words, capacityWords)
{
}

bool DownwellGameplayCopperList::build(
    const Bitmap& game, const Bitmap& hud,
    const Sprite& playerSprite, const Sprite* actorSprites,
    unsigned actorCount, const Sprite& overflowSprite,
    const Palette& palette, unsigned ringOrigin)
{
    const unsigned firstSpan = downwellGameRingHeight - ringOrigin;
    if (!beginCroppedDisplay(*this) ||
        !move(bplcon0, 0x4401) ||
        !move(bplcon1, 0) ||
        !move(bpl1mod, downwellFetchModulo) ||
        !move(bpl2mod, downwellFetchModulo) ||
        !showBitmap(game, 1, 2, downwellFetchCropBytes * 8,
                    ringOrigin, 2) ||
        !showBitmap(hud, 2, 2, downwellFetchCropBytes * 8, 0, 2) ||
        !showSprite(0, playerSprite))
        return false;

    for (unsigned channel = 0; channel < actorCount; ++channel) {
        if (!showSprite((UWORD)(channel + 1), actorSprites[channel]))
            return false;
    }
    if (!showSprite(7, overflowSprite) || !setPalette(palette))
        return false;

    if (firstSpan < downwellDisplayHeight) {
        const unsigned resetAfterLine =
            downwellDisplayVStartCropped + firstSpan - 1;
        if (resetAfterLine > 255 && !wait(255, 0xdf)) return false;
        if (!wait((UWORD)resetAfterLine, 0xdf) ||
            !showBitmap(game, 1, 2, downwellFetchCropBytes * 8, 0, 2))
            return false;
    }
    return finish();
}

DownwellMenuCopperList::DownwellMenuCopperList(
    UWORD* words, unsigned capacityWords)
    : CopperList(words, capacityWords)
{
}

bool DownwellMenuCopperList::buildScroll(
    const UBYTE* foreground, const UBYTE* background,
    const Sprite& playerSprite, const Sprite& nullSprite,
    const Palette& palette, const AmigaMenuScrollState& scroll)
{
    if (!beginNarrowDisplay(*this) ||
        !move(bplcon0, 0x2001) ||
        !move(bplcon1, scroll.scrollControl) ||
        !move(bpl1mod, amigaMenuModulo) ||
        !move(bpl2mod, amigaMenuModulo) ||
        !pointer(bpl1pth, foreground + scroll.foregroundOffset) ||
        !pointer(bpl2pth, background + scroll.backgroundOffset) ||
        !showSprite(0, playerSprite) ||
        !hideSprites(*this, nullSprite, 1) ||
        !setPalette(palette) ||
        !move(color02, 0xfff) ||
        !move(color03, 0xfff))
        return false;
    scrollOffset_ = moveOffset(bplcon1);
    foregroundOffset_ = moveOffset(bpl1pth);
    backgroundOffset_ = moveOffset(bpl2pth);
    playerOffset_ = moveOffset(spr1pth);
    return finish();
}

bool DownwellMenuCopperList::updateScroll(
    const UBYTE* foreground, const UBYTE* background,
    const UWORD* player, const AmigaMenuScrollState& scroll)
{
    return patchMove(scrollOffset_, scroll.scrollControl) &&
           patchPointer(foregroundOffset_, foreground + scroll.foregroundOffset) &&
           patchPointer(backgroundOffset_, background + scroll.backgroundOffset) &&
           patchPointer(playerOffset_, player);
}

bool DownwellMenuCopperList::updateEntry(const UBYTE* entry, int viewY)
{
    return patchPointer(foregroundOffset_, entry + amigaMenuEntryOffset(viewY));
}

bool DownwellMenuCopperList::buildEntry(
    const UBYTE* entry, const Sprite& nullSprite,
    const Palette& palette, int viewY)
{
    if (!beginNarrowDisplay(*this) ||
        !move(bplcon0, 0x1001) ||
        !move(bplcon1, 0) ||
        !move(bpl1mod, 0) ||
        !move(bpl2mod, 0) ||
        !pointer(bpl1pth, entry + amigaMenuEntryOffset(viewY)) ||
        !hideSprites(*this, nullSprite) ||
        !setPalette(palette))
        return false;
    foregroundOffset_ = moveOffset(bpl1pth);
    return finish();
}

DownwellSplashCopperList::DownwellSplashCopperList(
    UWORD* words, unsigned capacityWords)
    : CopperList(words, capacityWords)
{
}

bool DownwellSplashCopperList::buildNarrow(
    const UBYTE* frame, const Sprite& nullSprite, const Palette& palette,
    const UBYTE* ditherTile)
{
    return ::buildNarrow(*this, frame, nullSprite, palette, ditherTile);
}

bool DownwellSplashCopperList::buildWide(
    const Bitmap& frame, const Sprite& nullSprite, const Palette& palette)
{
    if (!beginCroppedDisplay(*this) ||
        !move(bplcon0, 0x2001) ||
        !move(bplcon1, 0) ||
        !move(bpl1mod, downwellFetchModulo) ||
        !move(bpl2mod, downwellFetchModulo) ||
        // Single playfield: initialize both consecutive DMA channels.
        !showBitmap(frame, 1, 1, downwellFetchCropBytes * 8, 0, 2) ||
        !hideSprites(*this, nullSprite) ||
        !setPalette(palette))
        return false;
    return finish();
}

static_assert((unsigned)amigaSplashPlaneBytes ==
                  (unsigned)amigaStylePlaneBytes,
              "narrow screens must share one copper plane stride");
static_assert(downwellDisplayHStartCropped == 0x71,
              "cropped display starts at logical x=16");
static_assert(downwellDisplayHStopCropped == 0x1d1,
              "352-pixel cropped display window");
static_assert(downwellDisplayVStopCropped == 0x12e,
              "284-line display window");
static_assert(downwellFetchStop == 0xd8,
              "22-word lores fetch at hardware limit");
static_assert(downwellFetchModulo == 4,
              "skip two bytes on each side per row");
static_assert(downwellFetchStart ==
                  (downwellDisplayHStartCropped >> 1) - 8,
              "lores fetch/display alignment");
static_assert(downwellCopperWords >= 340,
              "copper storage must cover narrow dither tile repetition");
