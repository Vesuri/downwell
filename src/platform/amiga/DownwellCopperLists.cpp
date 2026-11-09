#include <hardware/custom.h>
#include "AmigaMenu.h"
#include "AmigaMenuCache.h"
#include "AmigaMenuScroll.h"
#include "AmigaSplashCache.h"
#include "AmigaStyleRenderer.h"
#include "AmigaSelectionLayer.h"
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

DownwellStyleCopperList::DownwellStyleCopperList(UWORD* words, unsigned capacityWords, unsigned framePlaneBytes)
    : CopperList(words, capacityWords), frame0Offset_(0), frame1Offset_(0), maskOffset_(0), border0Offset_(0), border1Offset_(0), modeOffset_(0), framePlaneBytes_(framePlaneBytes)
{
}

bool DownwellSelectionCopperList::build(const UBYTE* border,const UBYTE* layer,const UBYTE* mask,
    const Sprite& player,const Sprite* actors,unsigned count,const Sprite& overflow,const Palette& palette) {
    if (!border || !layer || !beginNarrowDisplay(*this,true) ||
        !move(bplcon0,mask ? 0x6401 : 0x4401) || !move(bplcon1,0) || !move(bplcon2,0x0044) ||
        !move(bpl1mod,AmigaSelectionLayer::stride-20) || !move(bpl2mod,AmigaSelectionLayer::stride-20)) return false;
    modeOffset_=moveOffset(bplcon0);
    borderOffset_=size();
    if (!pointer(bpl1pth,border) || !pointer(bpl3pth,border+AmigaSelectionLayer::planeBytes)) return false;
    layerOffset_=size();
    if (!pointer(bpl2pth,layer) || !pointer(bpl4pth,layer+AmigaSelectionLayer::planeBytes)) return false;
    maskOffset_=size();
    if (!pointer(bpl5pth,mask) || !pointer(bpl6pth,mask) || !showSprite(0,player)) return false;
    for(unsigned i=0;i<count;++i) if(!showSprite(i+1,actors[i])) return false;
    return showSprite(7,overflow) && setPalette(palette) && finish();
}
bool DownwellSelectionCopperList::update(const UBYTE* border,const UBYTE* layer,const UBYTE* mask) {
    return valid() && border && layer && patchPointer(borderOffset_,border) &&
        patchPointer(borderOffset_+4,border+AmigaSelectionLayer::planeBytes) &&
        patchPointer(layerOffset_,layer) && patchPointer(layerOffset_+4,layer+AmigaSelectionLayer::planeBytes) &&
        patchPointer(maskOffset_,mask) && patchPointer(maskOffset_+4,mask) &&
        patchMove(modeOffset_,mask ? 0x6401 : 0x4401);
}

bool DownwellStyleCopperList::build(const UBYTE* frame, const UBYTE* mask,
                                    const Sprite& nullSprite, const Palette& palette, const UBYTE* border,
                                    const UWORD* const* sprites)
{
    // PF1: retained content on BPL1/3, temporary opaque-black high bit on
    // BPL5. PF2: immutable scrolling border on BPL2/4. PF1 has priority.
    // With carousel sprites, PF1 text and mask are also above all channels.
    if (!border || !beginNarrowDisplay(*this, true) || !move(bplcon0, mask ? 0x5401 : 0x4401) ||
        !move(bplcon1, 0) || !move(bplcon2, sprites?0x0020:0x0024) ||
        !move(bpl1mod, amigaStyleBytesPerRow - 20) ||
        !move(bpl2mod, amigaStyleBytesPerRow - 20)) return false;
    frame0Offset_ = size();
    modeOffset_ = moveOffset(bplcon0);
    if (!pointer(bpl1pth, frame)) return false;
    frame1Offset_ = size();
    if (!pointer(bpl3pth, frame + framePlaneBytes_)) return false;
    maskOffset_ = size();
    if (!pointer(bpl5pth, mask)) return false;
    border0Offset_ = size();
    if (!pointer(bpl2pth, border)) return false;
    border1Offset_ = size();
    if (!pointer(bpl4pth, border + amigaStyleBorderPlaneBytes)) return false;
    spritesOffset_=size();priorityOffset_=moveOffset(bplcon2);nullSprite_=nullSprite.data();
    spritePriority_=sprites!=nullptr;
    for(unsigned i=0;i<8;++i){
        spritePointers_[i]=sprites&&sprites[i]?sprites[i]:nullSprite_;
        if(!pointer(UWORD(spr1pth+4*i),spritePointers_[i]))return false;
    }
    return
           setPalette(palette) && move(color04, 0) && move(color05, 0) &&
           move(color06, 0) && move(color07, 0) && finish();
}

bool DownwellStyleCopperList::update(const UBYTE* frame, const UBYTE* mask, const UBYTE* border,
                                     const UWORD* const* sprites)
{
    for(unsigned i=0;i<8;++i){
        const auto* next=sprites&&sprites[i]?sprites[i]:nullSprite_;
        if(next!=spritePointers_[i]){
            if(!patchPointer(spritesOffset_+4*i,next))return false;
            spritePointers_[i]=next;
        }
    }
    if(spritePriority_!=(sprites!=nullptr)){
        if(!patchMove(priorityOffset_,sprites?0x0020:0x0024))return false;
        spritePriority_=sprites!=nullptr;
    }
    return patchPointer(frame0Offset_, frame) &&
           patchPointer(frame1Offset_, frame + framePlaneBytes_) &&
           patchPointer(maskOffset_, mask) &&
           patchPointer(border0Offset_, border) &&
           patchPointer(border1Offset_, border + amigaStyleBorderPlaneBytes) &&
           patchMove(modeOffset_, mask ? 0x5401 : 0x4401);
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
    if (!beginCroppedDisplay(*this) ||
        !move(bplcon0, 0x4401) ||
        !move(bplcon2, 0x0044) || // PF2 HUD above sprites; sprites above PF1 world
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

    exitModeOffset_ = moveOffset(bplcon0);
    exitMaskOffset_ = size();
    // The same high-bit cover masks both playfields. PF2 keeps its HUD and
    // sprite priority; pens 4..7 and 12..15 are black. No world repaint.
    if (!pointer(bpl5pth, nullptr) || !pointer(bpl6pth, nullptr)) return false;
    exitMaskData_ = nullptr;

    planeOffset_ = moveOffset(bpl1pth);
    overlayOffset_ = moveOffset(bpl2pth);
    wrapWaitOffset_ = size();
    // Fixed shape, including the PAL line-255 bridge, permits word patches.
    if (!wait(255, 0xdf) || !wait(301, 0xdf) ||
        !showBitmap(game, 1, 2, downwellFetchCropBytes * 8, 0, 2) || !finish())
        return false;
    return update(game, ringOrigin);
}

bool DownwellGameplayCopperList::updateExitMask(const UBYTE* mask)
{
    if (mask == exitMaskData_) return true;
    const UBYTE* fetch = mask ? mask + downwellFetchCropBytes : nullptr;
    const bool ok = patchPointer(exitMaskOffset_, fetch) &&
        patchPointer(exitMaskOffset_ + 4, fetch) &&
        patchMove(exitModeOffset_, mask ? 0x6401 : 0x4401);
    if (ok) exitMaskData_ = mask;
    return ok;
}

bool DownwellGameplayCopperList::update(const Bitmap& game, unsigned ringOrigin)
{
    if (ringOrigin >= downwellGameRingHeight) return false;
    if (gameData_ != game.data) {
        const UBYTE* base = (const UBYTE*)game.data + downwellFetchCropBytes;
        if (!patchPointer(wrapWaitOffset_ + 4, base) ||
            !patchPointer(wrapWaitOffset_ + 8, base + downwellGameRingPlaneBytes)) return false;
        gameData_ = game.data;
    }
    unsigned span = downwellGameRingHeight - ringOrigin;
    if (span > downwellDisplayHeight) span = downwellDisplayHeight;
    const unsigned line = downwellDisplayVStartCropped + span - 1;
    const UBYTE* data = (const UBYTE*)game.data + downwellFetchCropBytes +
                        ringOrigin * downwellBytesPerPlaneLine;
    // After $ffdf the copper can fetch the second wait on line 256, whose
    // eight-bit position is 0. Repeating $ffdf there waits for the next field
    // and skips the reload, so line 255 waits for $0001: met on either side
    // of the boundary and before line 256's first fetch at DDFSTRT.
    return patchPointer(planeOffset_, data) &&
           patchPointer(planeOffset_ + 4, data + downwellGameRingPlaneBytes) &&
           patchWait(wrapWaitOffset_, line < 255 ? line : 255, 0xdf) &&
           patchWait(wrapWaitOffset_ + 2, line == 255 ? 0 : line,
                     line == 255 ? 0x01 : 0xdf);
}

bool DownwellGameplayCopperList::updateOverlay(const Bitmap& hud)
{
    if (!valid() || !overlayOffset_ || !hud.data || hud.interleaved ||
        hud.dataWidth != downwellDmaWidth || hud.height != downwellDisplayHeight ||
        hud.bitplanes != 2) return false;
    const UBYTE* base = (const UBYTE*)hud.data + downwellFetchCropBytes;
    return patchPointer(overlayOffset_, base) &&
           patchPointer(overlayOffset_ + 4, base + downwellPlaneBytes);
}

DownwellMenuCopperList::DownwellMenuCopperList(
    UWORD* words, unsigned capacityWords)
    : CopperList(words, capacityWords)
{
}

bool DownwellMenuCopperList::buildScroll(
    const UBYTE* foreground, const UBYTE* background,
    const Sprite& playerSprite, const Sprite& nullSprite,
    const Palette& palette, const AmigaMenuScrollState& scroll, bool entry)
{
    if (!beginNarrowDisplay(*this) ||
        !move(bplcon0, 0x2001) ||
        !move(bplcon1, scroll.scrollControl) ||
        !move(bpl1mod, entry ? amigaMenuEntryBytesPerRow - amigaMenuFetchBytes
                            : amigaMenuModulo) ||
        !move(bpl2mod, amigaMenuModulo) ||
        !pointer(bpl1pth, foreground + scroll.foregroundOffset) ||
        !pointer(bpl2pth, background + scroll.backgroundOffset) ||
        !showSprite(0, playerSprite) ||
        !hideSprites(*this, nullSprite, 1) ||
        !setPalette(palette) ||
        !move(color02, palette[1]) ||
        !move(color03, palette[1]))
        return false;
    // Palette index 1 is the build's white, index 0 its black.
    skyLight_ = palette[1];
    skyDark_ = palette[0];
    scrollOffset_ = moveOffset(bplcon1);
    foregroundModuloOffset_ = moveOffset(bpl1mod);
    foregroundOffset_ = moveOffset(bpl1pth);
    backgroundOffset_ = moveOffset(bpl2pth);
    playerOffset_ = moveOffset(spr1pth);
    // Keep a fixed two-part split layout across PAL line 255. Pen 2 is
    // background-only; pen 3 remains white so foreground pixels survive.
    const unsigned cut = downwellDisplayVStartCropped +
                         scroll.backgroundVisibleLines - 1;
    skyWaitOffset_ = size();
    // $c0 is past this narrow DIW but leaves room for the color move
    // before the separate $ffdf rollover barrier on line 255.
    if (!wait((UWORD)(cut < 255 ? cut : 255), 0xc0)) return false;
    skyColorOffset_ = size();
    if (!move(color02, cut <= 255 ? skyDark_ : skyLight_) || !wait(255, 0xdf))
        return false;
    skyLateWaitOffset_ = size();
    if (!wait((UWORD)(cut > 255 ? cut : 256), 0xdf) || !move(color02, skyDark_))
        return false;
    return finish();
}

bool DownwellMenuCopperList::updateScroll(
    const UBYTE* foreground, const UBYTE* background,
    const UWORD* player, const AmigaMenuScrollState& scroll, bool entry)
{
    const unsigned cut = downwellDisplayVStartCropped +
                         scroll.backgroundVisibleLines - 1;
    return patchMove(scrollOffset_, scroll.scrollControl) &&
           patchWait(skyWaitOffset_, (UWORD)(cut < 255 ? cut : 255), 0xc0) &&
           patchMove(skyColorOffset_, cut <= 255 ? skyDark_ : skyLight_) &&
           patchWait(skyLateWaitOffset_, (UWORD)(cut > 255 ? cut : 256), 0xdf) &&
           patchMove(foregroundModuloOffset_,
                     entry ? amigaMenuEntryBytesPerRow - amigaMenuFetchBytes
                           : amigaMenuModulo) &&
           patchPointer(foregroundOffset_, foreground + scroll.foregroundOffset) &&
           patchPointer(backgroundOffset_, background + scroll.backgroundOffset) &&
           patchPointer(playerOffset_, player);
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
static_assert(downwellDisplayHStartCropped == 0x81,
              "cropped display starts at logical x=16");
static_assert(downwellDisplayHStopCropped == 0x1d1,
              "336-pixel cropped display window");
static_assert(downwellDisplayVStopCropped == 0x12e,
              "284-line display window");
static_assert(downwellFetchStop == 0xd8,
              "21-word lores fetch at hardware limit");
static_assert(downwellFetchModulo == 6,
              "skip two bytes left and four right per row");
static_assert(downwellFetchStart ==
                  (downwellDisplayHStartCropped >> 1) - 8,
              "lores fetch/display alignment");
static_assert(downwellCopperWords >= 340,
              "copper storage must cover narrow dither tile repetition");
