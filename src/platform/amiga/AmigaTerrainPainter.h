#ifndef DOWNWELL_AMIGA_TERRAIN_PAINTER_H
#define DOWNWELL_AMIGA_TERRAIN_PAINTER_H
#include "AmigaRenderer.h"
#include "AmigaTerrain.h"

// Fresh terrain composition in the retained implementation's painter order. No cache ownership,
// allocation, simulation, buffer publication or whole-frame redraw policy.
class AmigaTerrainPainter {
public:
    static bool bindBatch(const UBYTE* pack, const UBYTE* end, UBYTE* scratch, unsigned bytes);
    static bool bindPrepared(const UBYTE* data,unsigned bytes);
    // Transactional direct-cache attempt. False means the caller must repaint
    // with the retained compositor; no Chip pixels have been modified.
    static bool paintCached(UWORD* rows, AmigaRenderer* renderer,
        const downwell::RoomPack& rooms, const downwell::PackedRoom& room,
        const AmigaTerrainIndex* terrainIndex, const AmigaTerrainTiles& terrainTiles,
        const AmigaTerrainDecoration* decorations, unsigned decorationCount,
        const GameplayWorld* runtime, const UBYTE* pack, const UBYTE* end,
        int viewX, int viewY, int clipY, int clipHeight);
    static bool paint(AmigaRenderer* renderer,
        const downwell::RoomPack& rooms, const downwell::PackedRoom& room,
        const AmigaTerrainIndex* terrainIndex, const AmigaTerrainTiles& terrainTiles,
        const AmigaTerrainDecoration* decorations, unsigned decorationCount,
        const GameplayWorld* runtime, const UBYTE* pack, const UBYTE* end,
        int viewX, int viewY, int clipX, int clipY, int clipWidth, int clipHeight);
};
#endif
