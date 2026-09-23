#ifndef DOWNWELL_PLATFORM_AMIGA_STYLE_RENDERER_H
#define DOWNWELL_PLATFORM_AMIGA_STYLE_RENDERER_H

#include <exec/types.h>

class AmigaSceneRuntime;

struct AmigaStyleFrameState {
    bool initialized = false;
    int centerY = 0;
    int textPanel = -1;
    bool hardwareBorder = false;
    unsigned dirtyCount = 0;
    int smoothX = 0, cursor = -1;
    unsigned runFrame = 0;
    struct Rect { UWORD x, y, width, height; } dirty[8];
};

enum {
    amigaStyleWidth = 176,
    amigaStyleVisibleWidth = 160,
    amigaStyleHeight = 284,
    amigaStyleBorderHeight = amigaStyleHeight + 16,
    amigaStyleBytesPerRow = amigaStyleWidth / 8,
    amigaStylePlaneBytes = amigaStyleBytesPerRow * amigaStyleHeight,
    amigaStyleFrameBytes = amigaStylePlaneBytes * 2,
    amigaStyleBorderPlaneBytes =
        amigaStyleBytesPerRow * amigaStyleBorderHeight,
    amigaStyleBorderBytes = amigaStyleBorderPlaneBytes * 2,
    amigaStyleMaskHeight = amigaStyleHeight * 2 + 80,
    amigaStyleMaskBytes = amigaStyleBytesPerRow * amigaStyleMaskHeight,
    amigaStyleMaskOffset = amigaStyleFrameBytes * 2 + amigaStyleBorderBytes,
    amigaStyleCacheBytes = amigaStyleMaskOffset + amigaStyleMaskBytes * 2
};

bool amigaBuildStyleBorder(const UBYTE* pack, const UBYTE* packEnd,
                           UBYTE* border);
const UBYTE* amigaStyleMask(const UBYTE* cache, const AmigaSceneRuntime& scenes);
bool amigaRenderStyleFrame(const UBYTE* pack, const UBYTE* packEnd,
                           const UBYTE* border,
                           const AmigaSceneRuntime& scenes,
                           UBYTE* destination, AmigaStyleFrameState& state
#ifdef DOWNWELL_SCENE_VERIFY
                           , bool reference = false
#endif
                           );

#endif
