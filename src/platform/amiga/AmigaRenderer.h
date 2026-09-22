#ifndef DOWNWELL_PLATFORM_AMIGA_RENDERER_H
#define DOWNWELL_PLATFORM_AMIGA_RENDERER_H

#include <exec/types.h>

enum {
    downwellVisibleWidth = 380,
    downwellDmaWidth = 384,
    downwellDisplayHeight = 284,
    downwellBytesPerPlaneLine = downwellDmaWidth / 8,
    downwellBytesPerLayerLine = downwellBytesPerPlaneLine * 2,
    downwellLayerBytes = downwellBytesPerLayerLine * downwellDisplayHeight,
    downwellBackingBias = 2,
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
