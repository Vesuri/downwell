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

#ifdef __cplusplus
}
#endif

#endif
