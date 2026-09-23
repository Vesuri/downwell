#ifndef DOWNWELL_PLATFORM_AMIGA_SPLASH_CACHE_H
#define DOWNWELL_PLATFORM_AMIGA_SPLASH_CACHE_H

#include <exec/types.h>

enum {
    amigaSplashWidth = 176,
    amigaSplashVisibleWidth = 160,
    amigaSplashHeight = 284,
    amigaSplashBytesPerRow = amigaSplashWidth / 8,
    amigaSplashPlaneBytes = amigaSplashBytesPerRow * amigaSplashHeight,
    amigaSplashFrameBytes = amigaSplashPlaneBytes * 2,
    amigaSplashFrameCount = 5,
    amigaSplashCacheBytes = amigaSplashFrameBytes * amigaSplashFrameCount
};

bool amigaBuildSplashCache(const UBYTE* pack, const UBYTE* packEnd,
                           UBYTE* cache);
const UBYTE* amigaSplashFrame(const UBYTE* cache, int stage);

#endif
