#ifndef DOWNWELL_PLATFORM_AMIGA_RENDERER_H
#define DOWNWELL_PLATFORM_AMIGA_RENDERER_H

#include <exec/types.h>

enum {
    downwellVisibleWidth = 380,
    downwellDmaWidth = 384,
    downwellDisplayHeight = 284,
    downwellBytesPerPlaneLine = downwellDmaWidth / 8,
    downwellPlaneBytes = downwellBytesPerPlaneLine * downwellDisplayHeight,
    downwellLayerBytes = downwellPlaneBytes * 2,
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
    int clipX;
    int clipY;
    int clipWidth;
    int clipHeight;
} AmigaRenderer;

void amigaRendererInit(AmigaRenderer* renderer, UBYTE* game, UBYTE* hud);
void amigaRendererClear(AmigaRenderer* renderer, unsigned pen);
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
int amigaRendererBuildHardwareSprite(const UBYTE* pack, const UBYTE* packEnd,
                                     UWORD sprite, UWORD frame,
                                     int anchorX, int anchorY,
                                     UWORD* destination,
                                     unsigned destinationHeight);

#endif
