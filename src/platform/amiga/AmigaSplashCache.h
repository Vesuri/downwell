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
    amigaSplashFrameCount = 3,
    amigaSplashDitherFrameCount = 12,
    amigaSplashDitherTileHeight = 8,
    amigaSplashDitherTileBytes =
        amigaSplashBytesPerRow * amigaSplashDitherTileHeight,
    amigaSplashDitherBytes =
        amigaSplashDitherTileBytes * amigaSplashDitherFrameCount,
    amigaSplashCacheBytes =
        amigaSplashFrameBytes * amigaSplashFrameCount +
        amigaSplashDitherBytes
};

bool amigaBuildSplashCache(const UBYTE* pack, const UBYTE* packEnd,
                           UBYTE* cache, UBYTE* wideCredits);
const UBYTE* amigaSplashFrame(const UBYTE* cache, int stage);
const UBYTE* amigaSplashDitherTile(const UBYTE* cache, unsigned frame);

#endif
