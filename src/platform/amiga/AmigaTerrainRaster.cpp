#include "AmigaTerrainRaster.h"
#include "framework/Bitmap.h"
#include <cstring>

void AmigaTerrainRaster::invalidate(int first, int end) {
    if (first < 0) first = 0;
    if (end > int(rows)) end = rows;
    for (unsigned column = 0; column < 3; ++column)
        for (int y = first; y < end; ++y) setDirty(column,y,true);
}

void AmigaTerrainRaster::invalidateShops(const downwell::ShopWorld& shops) {
    for (unsigned i = 0; i < downwell::ShopWorld::capacity; ++i) {
        const auto& shop = shops.instances[i];
        if (shop.artRevision == shopRevision_[i]) continue;
        shopRevision_[i] = shop.artRevision;
        if (shop.allocated) invalidate(shop.y - 21, shop.y + 12);
    }
}

typedef ULONG RasterLong __attribute__((may_alias));

static void copyTerrainRows68000(const UWORD* sourceWords, UBYTE* plane0,
                                UBYTE* plane1, unsigned rows) {
    if (!rows) return;
 #ifdef __m68k__
    register const UWORD* source __asm("a0") = sourceWords;
    register UBYTE* destination0 __asm("a1") = plane0;
    register UBYTE* destination1 __asm("a2") = plane1;
    register unsigned count __asm("d5") = rows - 1;
    // Assembly twin of the 22-byte planar row transfer below. Each row in
    // Fast RAM contains PF1's two planes; Chip RAM retains its 48-byte stride.
    __asm__ volatile(
        "1: movem.l (%0)+,%%d0-%%d4\n\t"
        "movem.l %%d0-%%d4,(%1)\n\t"
        "move.w (%0)+,20(%1)\n\t"
        "movem.l (%0)+,%%d0-%%d4\n\t"
        "movem.l %%d0-%%d4,(%2)\n\t"
        "move.w (%0)+,20(%2)\n\t"
        "lea 48(%1),%1\n\t"
        "lea 48(%2),%2\n\t"
        "dbra %3,1b"
        : "+a"(source), "+a"(destination0), "+a"(destination1), "+d"(count)
        : : "d0", "d1", "d2", "d3", "d4", "cc", "memory");
#else
    for (unsigned y = 0; y < rows; ++y) {
        std::memcpy(plane0 + y * 48, sourceWords + y * 22, 22);
        std::memcpy(plane1 + y * 48, sourceWords + y * 22 + 11, 22);
    }
#endif
}

void AmigaTerrainRaster::transfer(AmigaRenderer* renderer, unsigned column,
    int worldY, int screenY, int height, bool store, Bitmap* terrainCopyTarget,
    const Bitmap* terrainCopyBacking, unsigned terrainCopyBuffer, AmigaGameplayRows& gameplayRows) {
    UWORD* cachedRow = store?writeRows(column,worldY,height):&pixels_[column][worldY][0][0];
    unsigned physical = renderer->gameOrigin + screenY;
    if (physical >= downwellGameRingHeight) physical -= downwellGameRingHeight;
    if (!store) {
        const bool pipeline = height > 96 && terrainCopyTarget;
        unsigned remaining = height;
        while (remaining) {
            unsigned count = downwellGameRingHeight - physical;
            if (count > remaining) count = remaining;
            if (pipeline && count > 128) count = 128;
            UBYTE* row = renderer->game + physical * downwellBytesPerPlaneLine + x / 8;
#ifndef DOWNWELL_REFERENCE_INITIAL_RASTER_STORAGE
            if(initial_){
                UWORD scratch[AmigaInitialTerrain::words];
                unsigned done=0;
                while(done<count){
                    if(dirty_[column][worldY+done]&6){
                        const auto* source=readRow(column,worldY+done,scratch);
                        copyTerrainRows68000(source,row+done*48,
                            row+downwellGameRingPlaneBytes+done*48,1);
                        ++done;
                    }else{
                        const unsigned first=done;
                        do{++done;}while(done<count&&!(dirty_[column][worldY+done]&6));
                        copyTerrainRows68000(cachedRow+first*words*2,row+first*48,
                            row+downwellGameRingPlaneBytes+first*48,done-first);
                    }
                }
            }else
#endif
                copyTerrainRows68000(cachedRow, row, row + downwellGameRingPlaneBytes, count);
            if (pipeline) {
                // These completed rows are immutable while the CPU prepares
                // the next disjoint chunk. DMA copies them concurrently.
                terrainCopyTarget->copy(*terrainCopyBacking, x, physical,
                    x, physical, words * 16, count);
                gameplayRows.copied(physical, count, terrainCopyBuffer);
            }
            remaining -= count;
            cachedRow += count * words * 2;
            worldY += count;
            physical += count;
            if (physical == downwellGameRingHeight) physical = 0;
        }
        return;
    }
    for (int y = 0; y < height; ++y) {
        for (unsigned plane = 0; plane < 2; ++plane) {
            UWORD* chip = (UWORD*)(renderer->game + plane * downwellGameRingPlaneBytes +
                physical * downwellBytesPerPlaneLine + x / 8);
            UWORD* cached = cachedRow + plane * words;
#ifdef __m68k__
            RasterLong* destination = (RasterLong*)(store ? cached : chip);
            const RasterLong* source = (const RasterLong*)(store ? chip : cached);
            // Native 68000 long moves require only word alignment. This
            // bounded 22-byte row avoids byte/word loops and runtime multiplies.
            destination[0] = source[0]; destination[1] = source[1];
            destination[2] = source[2]; destination[3] = source[3];
            destination[4] = source[4];
            ((UWORD*)destination)[10] = ((const UWORD*)source)[10];
#else
            std::memcpy(cached, chip, words * sizeof(UWORD));
#endif
        }
        cachedRow += words * 2;
        if (++physical == downwellGameRingHeight) physical = 0;
    }
}
