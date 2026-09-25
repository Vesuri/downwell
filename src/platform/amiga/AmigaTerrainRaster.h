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
    bool ready = false;
    UWORD* row(unsigned column, unsigned y) { return &pixels_[column][y][0][0]; }
    bool& dirty(unsigned column, unsigned y) { return dirty_[column][y]; }
    void invalidate(int first, int end);
    void invalidateShops(const downwell::ShopWorld& shops);
    void transfer(AmigaRenderer* renderer, unsigned column, int worldY,
        int screenY, int height, bool store, Bitmap* pipelineTarget,
        const Bitmap* backing, unsigned buffer, AmigaGameplayRows& debts);
};
#endif
