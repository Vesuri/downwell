#ifndef DOWNWELL_PLATFORM_AMIGA_STYLE_RENDERER_H
#define DOWNWELL_PLATFORM_AMIGA_STYLE_RENDERER_H

#include <exec/types.h>

class AmigaSceneRuntime;

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
    amigaStyleBorderBytes = amigaStyleBorderPlaneBytes * 2
};

bool amigaBuildStyleBorder(const UBYTE* pack, const UBYTE* packEnd,
                           UBYTE* border);
bool amigaRenderStyleFrame(const UBYTE* pack, const UBYTE* packEnd,
                           const UBYTE* border,
                           const AmigaSceneRuntime& scenes,
                           UBYTE* destination);

#endif
