#include "AmigaTerrainRaster.h"
#include "framework/Bitmap.h"
#include <cstring>

void AmigaTerrainRaster::invalidate(int first, int end) {
    if (first < 0) first = 0;
    if (end > int(rows)) end = rows;
    if(first<end){
        ++revision_;
        if(unsigned(first)<prepareFirst_)prepareFirst_=unsigned(first);
        if(unsigned(end)>prepareEnd_)prepareEnd_=unsigned(end);
    }
#ifdef DOWNWELL_REFERENCE_INITIAL_RASTER_STORAGE
    for (unsigned column = 0; column < 3; ++column)
        for (int y = first; y < end; ++y) setDirty(column,y,true);
#else
    // setDirty(column,y,true) preserves the borrowed-art flags in bits 1-4:
    // set bit 0 of every byte in the band, four rows per longword.
    typedef ULONG DirtyLong __attribute__((aligned(2),may_alias));
    for (unsigned column = 0; column < 3 && first < end; ++column) {
        UBYTE* p = &dirty_[column][first];
        unsigned n = unsigned(end - first);
        if (reinterpret_cast<__UINTPTR_TYPE__>(p) & 1u) { *p++ |= 1; --n; }
        for (; n >= 4; n -= 4, p += 4) *reinterpret_cast<DirtyLong*>(p) |= 0x01010101u;
        while (n--) *p++ |= 1;
    }
#endif
}

void AmigaTerrainRaster::invalidateShops(const downwell::ShopWorld& shops) {
#ifndef DOWNWELL_REFERENCE_FRAME_WORK
    if(!shops.allocatedCount){
        if(shopRevisionsPopulated_){
            for(auto& revision:shopRevision_)revision=0;
            shopRevisionsPopulated_=false;
        }
        return;
    }
    shopRevisionsPopulated_=true;
#endif
    for (unsigned i = 0; i < downwell::ShopWorld::capacity; ++i) {
        const auto& shop = shops.instances[i];
        if (shop.artRevision == shopRevision_[i]) continue;
        shopRevision_[i] = shop.artRevision;
        if (shop.allocated) invalidate(shop.y - 21, shop.y + 12);
    }
}

typedef ULONG RasterLong __attribute__((may_alias));

#if defined(__m68k__) && defined(DOWNWELL_PREPARED_PATTERN_ROWS)
extern "C" void downwellPreparePatternBand(const AmigaPatternRow*,UWORD*,unsigned);
#ifdef DOWNWELL_TERRAIN_BATCH_VERIFY
extern "C" {
extern volatile ULONG gDownwellTerrainBatchFailures,gDownwellPatternTransferChecks;
volatile ULONG gDownwellPatternPreparationChecks=0;
}
#endif
#endif

unsigned AmigaTerrainRaster::materializePatternRows(unsigned first,unsigned budget){
    if(first>=rows||!budget||!needsPatternMaterialization(first))return 0;
    const unsigned band=(first+8)/16;
    unsigned end=first;
    while(end<rows&&end-first<budget&&(end+8)/16==band&&needsPatternMaterialization(end))++end;
    const auto source=bands_[band].offset((first+8)&15);
#if defined(__m68k__) && defined(DOWNWELL_PREPARED_PATTERN_ROWS)
    auto* pixels=writeRows(1,first,end-first);
    downwellPreparePatternBand(&source,pixels,end-first);
#ifdef DOWNWELL_TERRAIN_BATCH_VERIFY
    for(unsigned y=0;y<end-first;++y){
        UWORD expected[22];source.offset(y).write(expected,expected+words);
        for(unsigned w=0;w<22;++w)if(expected[w]!=pixels[y*22+w])++gDownwellTerrainBatchFailures;
        ++gDownwellPatternTransferChecks;
        ++gDownwellPatternPreparationChecks;
    }
#endif
#else
    for(unsigned y=0;y<end-first;++y){
        auto* target=writeRows(1,first+y,1);
        source.offset(y).write(target,target+words);
    }
#endif
    return end-first;
}

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

// A run of composition descriptors in the existing 44-byte raster slots.
// Source strips need not be contiguous: each row retains its own four loans.
static void composeTerrainRowsPortable(const UWORD* descriptors,UBYTE* low,UBYTE* high,unsigned rows){
    for(unsigned y=0;y<rows;++y){
        AmigaPatternRow source;
        std::memcpy(&source,descriptors+y*22,sizeof source);
        source.write(reinterpret_cast<UWORD*>(low+y*48),reinterpret_cast<UWORD*>(high+y*48));
    }
}
#ifdef __m68k__
extern "C" void downwellComposePatternRows(const UWORD*,UBYTE*,UBYTE*,unsigned);
extern "C" void downwellComposePatternBand(const AmigaPatternRow*,UBYTE*,UBYTE*,unsigned);
#ifdef DOWNWELL_TERRAIN_BATCH_VERIFY
extern "C" {
extern volatile ULONG gDownwellTerrainBatchFailures;
volatile ULONG gDownwellPatternTransferChecks=0;
}
#endif
#endif
static void composeTerrainRows(const UWORD* descriptors,UBYTE* low,UBYTE* high,unsigned rows){
#ifdef __m68k__
    static_assert(sizeof(AmigaPatternRow)==16,"68000 descriptor layout");
    downwellComposePatternRows(descriptors,low,high,rows);
#ifdef DOWNWELL_TERRAIN_BATCH_VERIFY
    for(unsigned y=0;y<rows;++y){
        UWORD expected[48]{};
        composeTerrainRowsPortable(descriptors+y*22,reinterpret_cast<UBYTE*>(expected),
            reinterpret_cast<UBYTE*>(expected+24),1);
        for(unsigned word=0;word<11;++word)
            if(expected[word]!=reinterpret_cast<const UWORD*>(low+y*48)[word]||
               expected[24+word]!=reinterpret_cast<const UWORD*>(high+y*48)[word])
                ++gDownwellTerrainBatchFailures;
        ++gDownwellPatternTransferChecks;
    }
#endif
#else
    composeTerrainRowsPortable(descriptors,low,high,rows);
#endif
}

static void composeTerrainBand(const AmigaPatternRow& band,UBYTE* low,UBYTE* high,unsigned rows){
#ifdef __m68k__
    downwellComposePatternBand(&band,low,high,rows);
#endif
#if !defined(__m68k__) || defined(DOWNWELL_TERRAIN_BATCH_VERIFY)
    for(unsigned y=0;y<rows;++y){
        const auto source=band.offset(y);
#ifdef __m68k__
        UWORD expected[22];source.write(expected,expected+11);
        for(unsigned w=0;w<11;++w)
            if(expected[w]!=reinterpret_cast<const UWORD*>(low+y*48)[w]||
               expected[w+11]!=reinterpret_cast<const UWORD*>(high+y*48)[w])
                ++gDownwellTerrainBatchFailures;
        ++gDownwellPatternTransferChecks;
#else
        source.write(reinterpret_cast<UWORD*>(low+y*48),reinterpret_cast<UWORD*>(high+y*48));
#endif
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
            if(initial_||patternBorrowed_){
                UWORD scratch[AmigaInitialTerrain::words];
                unsigned done=0;
                while(done<count){
                    if(dirty_[column][worldY+done]&32){
                        const unsigned first=done,band=(worldY+done+8)/16;
                        const auto source=bands_[band].offset((worldY+done+8)&15);
                        do{++done;}while(done<count&&(worldY+done+8)/16==band&&
                            (dirty_[column][worldY+done]&32));
                        if(source.tiles[0])composeTerrainBand(source,row+first*48,
                            row+downwellGameRingPlaneBytes+first*48,done-first);
                        else{
#ifdef DOWNWELL_PLATFORM_AMIGA
                            copyTerrainRows68000(reinterpret_cast<const UWORD*>(source.walls),
                                row+first*48,row+downwellGameRingPlaneBytes+first*48,done-first);
#else
                            for(unsigned y=first;y<done;++y){
                                const auto* pixels=readRow(column,worldY+y,scratch);
                                copyTerrainRows68000(pixels,row+y*48,row+downwellGameRingPlaneBytes+y*48,1);
                            }
#endif
                        }
                    }else if(dirty_[column][worldY+done]&16){
                        const unsigned first=done;
                        do{++done;}while(done<count&&(dirty_[column][worldY+done]&16));
                        composeTerrainRows(&pixels_[column][worldY+first][0][0],row+first*48,
                            row+downwellGameRingPlaneBytes+first*48,done-first);
                    }else if(dirty_[column][worldY+done]&8){
                        const unsigned end=borrowedRunEnd(column,worldY+done,worldY+count)-worldY;
#ifdef DOWNWELL_PLATFORM_AMIGA
                        const auto* source=readRow(column,worldY+done,scratch);
                        copyTerrainRows68000(source,row+done*48,
                            row+downwellGameRingPlaneBytes+done*48,end-done);
                        done=end;
#else
                        for(;done<end;++done){
                            const auto* source=readRow(column,worldY+done,scratch);
                            copyTerrainRows68000(source,row+done*48,
                                row+downwellGameRingPlaneBytes+done*48,1);
                        }
#endif
                    }else if(dirty_[column][worldY+done]&6){
                        const auto* source=readRow(column,worldY+done,scratch);
                        copyTerrainRows68000(source,row+done*48,
                            row+downwellGameRingPlaneBytes+done*48,1);
                        ++done;
                    }else{
                        const unsigned first=done;
                        do{++done;}while(done<count&&!(dirty_[column][worldY+done]&62));
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
