#ifndef DOWNWELL_PLATFORM_AMIGA_RENDERER_H
#define DOWNWELL_PLATFORM_AMIGA_RENDERER_H

#include <exec/types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    downwellVisibleWidth = 380,
    downwellDmaWidth = 384,
    downwellDisplayHeight = 284,
    downwellGameRingHeight = 320,
    downwellBytesPerPlaneLine = downwellDmaWidth / 8,
    downwellPlaneBytes = downwellBytesPerPlaneLine * downwellDisplayHeight,
    downwellLayerBytes = downwellPlaneBytes * 2,
    downwellGameRingPlaneBytes =
        downwellBytesPerPlaneLine * downwellGameRingHeight,
    downwellGameRingBytes = downwellGameRingPlaneBytes * 2,
    /* Beam-space origin for logical x=0 after the 16-pixel left crop. */
    downwellDisplayHStart = 0x61,
    downwellDisplayVStart = 0x12,
    downwellPlayfieldX = 110,
    downwellPlayfieldWidth = 160
};

typedef struct AmigaRenderer {
    UBYTE* game;
    UBYTE* hud;
    UBYTE* target;
    unsigned gameOrigin;
    int clipX;
    int clipY;
    int clipWidth;
    int clipHeight;
    int blitterEnabled;
} AmigaRenderer;

void amigaRendererInit(AmigaRenderer* renderer, UBYTE* game, UBYTE* hud);
void amigaRendererSetGameOrigin(AmigaRenderer* renderer, unsigned origin);
void amigaRendererEnableBlitter(AmigaRenderer* renderer);
void amigaRendererClear(AmigaRenderer* renderer, unsigned pen);
void amigaRendererClearGameRows(AmigaRenderer* renderer,
                                int firstY, int height, unsigned pen);
void amigaRendererSetClipRect(AmigaRenderer* renderer,
                              int x, int y, int width, int height);
void amigaRendererClearClipRect(AmigaRenderer* renderer);
void amigaRendererFillRect(AmigaRenderer* renderer,
                           int x, int y, int width, int height,
                           unsigned pen);
void amigaRendererDrawAsset(AmigaRenderer* renderer,
                            const UBYTE* pack, const UBYTE* packEnd,
                            UWORD sprite, UWORD frame,
                            int anchorX, int anchorY, int flipX);
/* Native words copied from an offline planar asset, not pixel conversion. */
typedef struct AmigaWordStamp {
    UWORD low[8], high[8], mask[8];
    UWORD runLow[64], runHigh[64], runMask[64];
    unsigned first, last;
} AmigaWordStamp;
int amigaRendererPrepareWordStamp(AmigaWordStamp*, const UBYTE*, const UBYTE*, UWORD, UWORD);
int amigaRendererDrawWordStamp(AmigaRenderer*, const AmigaWordStamp*, int, int);
int amigaRendererDrawWordStampRun(AmigaRenderer*, const AmigaWordStamp*, int, int, unsigned);
unsigned amigaRendererCopyHudWords(UBYTE*, const UBYTE*, int, int, int, int, int);
// Compose one retained masked column and publish only changed plane words.
unsigned amigaRendererPublishHudColumn(UBYTE*, UBYTE*, const UWORD*, unsigned,
                                      int, int, int, UWORD);
/* One aligned HUD column, already clipped by caller; masked planar words. */
void amigaRendererFillHudWordColumn(UBYTE*, unsigned, unsigned, unsigned, UWORD, UWORD, UWORD);
void amigaRendererCacheHudColumns(UWORD*, const UBYTE*, unsigned, unsigned);
void amigaRendererApplyHudColumns(AmigaRenderer*, const UWORD*, unsigned, unsigned, int, int, int, int);
int amigaRendererAssetBounds(const UBYTE* pack, const UBYTE* packEnd,
                             UWORD sprite, UWORD frame, int anchorX,
                             int anchorY, int flipX, int* x, int* y,
                             int* width, int* height);
void amigaRendererDrawAssetPen(AmigaRenderer* renderer,
                               const UBYTE* pack, const UBYTE* packEnd,
                               UWORD sprite, UWORD frame,
                               int anchorX, int anchorY, int flipX,
                               unsigned pen);
/*
 * Returns zero on an invalid asset, one when at least one sprite pixel can
 * appear inside the game playfield, and two when the valid sprite is wholly
 * clipped.  A clipped result still writes a hidden hardware-sprite record.
 */
int amigaRendererBuildHardwareSprite(const UBYTE* pack, const UBYTE* packEnd,
                                     UWORD sprite, UWORD frame,
                                     int anchorX, int anchorY, int flipX,
                                     UWORD* destination,
                                     unsigned destinationHeight);
unsigned amigaRendererHardwareSpritePartCount(
    const UBYTE* pack, const UBYTE* packEnd, UWORD sprite, UWORD frame);
/* Short-lived descriptor: its bank must remain unchanged through drawing. */
typedef struct AmigaResolvedSpritePart {
    const UBYTE* pack;
    const UBYTE* end;
    const UBYTE* record;
    unsigned partCount;
} AmigaResolvedSpritePart;
int amigaRendererResolveSpritePart(AmigaResolvedSpritePart*, const UBYTE*, const UBYTE*, UWORD, UWORD, unsigned, int);
int amigaRendererDrawResolvedSpritePart(const AmigaResolvedSpritePart*, int, int, UWORD*, unsigned);
ULONG amigaRendererChipPackBytes(const UBYTE* pack, const UBYTE* packEnd);
int amigaRendererBuildHardwareSpritePart(
    const UBYTE* pack, const UBYTE* packEnd,
    UWORD sprite, UWORD frame, unsigned part,
    int anchorX, int anchorY, int flipX,
    UWORD* destination, unsigned destinationHeight);

#ifdef __cplusplus
}
#endif

#endif
