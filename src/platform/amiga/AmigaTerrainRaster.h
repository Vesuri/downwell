#ifndef DOWNWELL_AMIGA_TERRAIN_RASTER_H
#define DOWNWELL_AMIGA_TERRAIN_RASTER_H
#include "AmigaGameplayRows.h"
#include "core/ShopWorld.h"
class Bitmap;
// Fast-RAM raster for the left room, well and right room. Transfer only
// exposed/invalidated spans into the Chip-RAM circular backing/display rings.
class AmigaTerrainRaster {
    UWORD pixels_[3][8192][2][AmigaGameplayRows::terrainWords];
    bool dirty_[3][8192]{};
    unsigned shopRevision_[downwell::ShopWorld::capacity]{};
public:
    static constexpr unsigned rows = 8192, words = AmigaGameplayRows::terrainWords;
    static constexpr unsigned x = AmigaGameplayRows::terrainX;
    static constexpr unsigned stagePaintRows = 16;
    bool ready = false;
    UWORD* row(unsigned column, unsigned y) { return &pixels_[column][y][0][0]; }
    bool& dirty(unsigned column, unsigned y) { return dirty_[column][y]; }
    // First row after the homogeneous clean/dirty run. Invalid/empty requests
    // do not advance. Query before repainting: the caller may clear dirty bits.
    unsigned runEnd(unsigned column, unsigned first, unsigned end) const {
        if(column>=3 || first>=end || end>rows)return first;
        const bool state=dirty_[column][first];
        unsigned next=first+1;
        while(next<end && dirty_[column][next]==state)++next;
        return next;
    }
    void invalidate(int first, int end);
    void invalidateShops(const downwell::ShopWorld& shops);
    // Populate retired staging storage only. Paint must finish all DMA before
    // returning; failed paint leaves that run dirty. Never acknowledge either
    // live display's debts or use its pipelined copy target here.
    template<class Paint>
    int prepareStage(AmigaRenderer* renderer, unsigned column, unsigned worldY,
                     unsigned height, AmigaGameplayRows& debts, Paint paint) {
        if (!ready || column >= 3 || worldY >= rows || height > 48 ||
            height > rows - worldY) return -1;
        unsigned done = 0, budget = stagePaintRows;
        while (done < height) {
            const unsigned first = worldY + done;
            unsigned count = runEnd(column, first, worldY + height) - first;
            const bool repaint = dirty_[column][first];
            if (repaint) {
                if (!budget) break;
                if (count > budget) count = budget;
                if (!paint(done, count)) return -1;
                budget -= count;
            }
            transfer(renderer, column, first, done, count, repaint,
                     nullptr, nullptr, 0, debts);
            if (repaint)
                for (unsigned y = first; y < first + count; ++y) dirty_[column][y] = false;
            done += count;
        }
        return int(done);
    }
    void transfer(AmigaRenderer* renderer, unsigned column, int worldY,
        int screenY, int height, bool store, Bitmap* pipelineTarget,
        const Bitmap* backing, unsigned buffer, AmigaGameplayRows& debts);
};
#endif
