#ifndef DOWNWELL_AMIGA_TERRAIN_PROFILE_H
#define DOWNWELL_AMIGA_TERRAIN_PROFILE_H

// Bounded diagnostic history, copied only when a new worst frame is found.
// kind: 0=cached transfer, 1=dirty cache, 2=forced, 3=uncached.
// cause: 0=exposure/camera/prefetch, 1=generation, 2=shop, 3=broken block.
struct AmigaTerrainProfileSpan {
    long viewX, firstY, height, clipX, width, kind, cause;
};
#ifdef DOWNWELL_PROBES
void downwellProfileTerrainSpan(int viewX, int firstY, int height,
    int clipX, int width, unsigned kind, unsigned cause);
#else
inline void downwellProfileTerrainSpan(int, int, int, int, int, unsigned, unsigned) {}
#endif
#endif
