/* TINI: tools/generate-initial-terrain.py (tools/prepare-initial-terrain.cpp).
   The first room's (11) walls as the game's direct terrain cache composes
   them: AmigaTerrainPainter::paintCached with only an occupancy index takes
   AmigaTerrainRunSurface::wallRow's prepared six-pixel path for every row,
   so this is that path over the same index rows. Then the periodic corridor
   rows the runtime may borrow. The prepare tool's scalar oracle checks are
   not repeated here; the test compares the result with its output. */
#ifdef INSTALL_AMIGA
#define _STRING_H_ /* the NDK's string.h lacks C linkage; io.h declares memcpy and memset */
#else
#include <stddef.h>
#include <string.h>
#endif
#include "derived_parts.h"
extern "C" {
#include "rooms.h"
}
#include "../../src/core/RoomPack.cpp"

namespace {
/* AmigaTerrainIndex: 128 columns of 16 pixels from x = -512, 512 rows; one
   bit per cell for the wall appearance flags (walls, 60). */
enum { Columns = 128, IndexRows = 512, MinX = -512, Words = 11 };
struct WallIndex { u32 bits[IndexRows][Columns / 32]; };

bool addWall(WallIndex& index, int x, int y)
{
    if ((x & 15) || (y & 15) || x < MinX || x >= MinX + Columns * 16 || y < 0 || y >= IndexRows * 16) return false;
    const unsigned column = (unsigned)(x - MinX) >> 4;
    index.bits[y >> 4][column >> 5] |= 1ul << (column & 31);
    return true;
}

/* AmigaTerrainPainter's wallRowBits. */
u32 rowBits(const WallIndex& index, int row, int column, unsigned count)
{
    if (row < 0 || row >= IndexRows) return 0;
    u32 bits = 0, bit = 1;
    for (unsigned i = 0; i < count; ++i, ++column, bit <<= 1)
        if (column >= 0 && column < Columns && (index.bits[row][column >> 5] >> (column & 31) & 1)) bits |= bit;
    return bits;
}

void mergeWord(u16* p, u16 mask, u16 color) { *p = (u16)((*p & ~mask) | (color & mask)); }

/* paintCached(rows, viewX, viewY, clipY 0, height): the surface is cleared,
   then paintIndexedBatch hands each occupancy row to wallRow. */
void paintWalls(u16* rows, const WallIndex& index, const u8* phase, int viewX, int viewY, int height)
{
    for (int i = 0; i < height * Words * 2; ++i) rows[i] = 0;
    const int left = 107, right = 272; /* downwellTerrainX, + downwellTerrainWidth */
    const int firstX = (viewX + left - 110 + 8) & ~15, lastX = (viewX + right - 110 + 7) & ~15;
    const int firstY = (viewY + 8) & ~15, lastY = (viewY + height + 7) & ~15;
    const unsigned columns = (unsigned)(lastX - firstX) / 16 + 1;
    const int column = (firstX - MinX) / 16 - 1;
    int row = firstY / 16;
    u32 above = rowBits(index, row - 1, column, columns + 2), middle = rowBits(index, row, column, columns + 2);
    for (int y = firstY; y <= lastY; y += 16, ++row) {
        const u32 below = rowBits(index, row + 1, column, columns + 2);
        /* wallRow(anchorX, anchorY, ...): the prepared path (x - 96 = 6 mod 16). */
        const int tileX = 110 + firstX - viewX - 8, tileY = y - viewY - 8;
        const int column0 = tileX >= 96 ? (tileX - 96) / 16 : -1;
        const int top = tileY > 0 ? tileY : 0, end = tileY + 16 < height ? tileY + 16 : height;
        if (top < end)
            for (unsigned cell = 0; cell < columns; ++cell) {
                const u32 bit = 2ul << cell;
                const int col = column0 + (int)cell;
                if (!(middle & bit) || col < 0 || col >= Words) continue;
                const unsigned frame = ((above & bit) ? 1 : 0) | ((middle & (bit << 1)) ? 2 : 0) |
                                       ((below & bit) ? 4 : 0) | ((middle & (bit >> 1)) ? 8 : 0);
                const u8* source = phase + frame * 192 + (top - tileY) * 12;
                u16* target = rows + top * Words * 2 + col;
                for (int r = top; r < end; ++r, source += 12, target += Words * 2) {
                    const u32 coverage = rd_be32(source), low = rd_be32(source + 4), high = rd_be32(source + 8);
                    if (col == 10) {
                        const u16 mask = (u16)(coverage >> 16);
                        mergeWord(target, mask, (u16)(low >> 16));
                        mergeWord(target + Words, mask, (u16)(high >> 16));
                    } else {
                        const u32 mask = coverage & (col == 0 ? 0x001ffffful : 0xfffffffful);
                        mergeWord(target, (u16)(mask >> 16), (u16)(low >> 16));
                        mergeWord(target + 1, (u16)mask, (u16)low);
                        mergeWord(target + Words, (u16)(mask >> 16), (u16)(high >> 16));
                        mergeWord(target + Words + 1, (u16)mask, (u16)high);
                    }
                }
            }
        above = middle; middle = below;
    }
}

u32 hashRow(const u16* row)
{
    u32 h = 2166136261u;
    for (unsigned i = 0; i < 22; ++i) h = (h ^ row[i]) * 16777619u;
    return h;
}
}

extern "C" int derived_initial_terrain(Context* c, const u8* phase)
{
    Buf pack;
    buf_init(&pack);
    if (!rooms_full_pack(c, &pack)) { buf_free(&pack); return 0; }
    WallIndex* index = static_cast<WallIndex*>(dw_zalloc(sizeof(WallIndex)));
    if (!index) { buf_free(&pack); return 0; }
    int rows = 0, ok = 1;
    {
        downwell::RoomPack rooms(pack.p, pack.n);
        downwell::PackedRoom room;
        if (!rooms.valid() || !rooms.findRoom(11, room)) ok = dw_fail("the room pack lacks the first room");
        for (unsigned i = 0; ok && i < room.instanceCount; ++i) {
            downwell::PackedRoomInstance instance;
            if (!rooms.roomInstance(room, i, instance)) { ok = dw_fail("the room pack is damaged"); break; }
            if (instance.objectId != 60) continue;
            if (!addWall(*index, instance.x, instance.y)) { ok = dw_fail("the first room has a wall off the grid"); break; }
            if (instance.y + 32 > rows) rows = instance.y + 32;
        }
    }
    buf_free(&pack);
    if (ok && (rows <= 0 || rows > 8192)) ok = dw_fail("the first room's walls are out of range");
    const unsigned lines = 3 * (unsigned)rows;
    u16* raster = ok ? static_cast<u16*>(dw_alloc(lines * 22 * 2)) : nullptr;
    if (!raster) ok = 0;
    for (unsigned column = 0; ok && column < 3; ++column)
        for (int y = 0; y < rows; y += 284)
            paintWalls(raster + (column * rows + y) * 22, *index, phase, (int)column * 160, y, rows - y < 284 ? rows - y : 284);
    /* The corridor: walls at x 160 and 320 from y 0 to 80, sixteen rows at y 32. */
    u16 corridor[16 * 22];
    if (ok) {
        memset(index, 0, sizeof *index);
        for (int y = 0; y <= 80; y += 16) { addWall(*index, 160, y); addWall(*index, 320, y); }
        paintWalls(corridor, *index, phase, 160, 32, 16);
    }
    dw_free(index);
    /* The row dictionary in first use; references column-major. */
    enum { Slots = 4096 };
    u16* slots = ok ? static_cast<u16*>(dw_zalloc(2 * Slots)) : nullptr;
    u16* references = ok ? static_cast<u16*>(dw_alloc(2 * lines)) : nullptr;
    u16* entries = ok ? static_cast<u16*>(dw_alloc(lines * 22 * 2)) : nullptr;
    unsigned nentries = 0;
    if (!slots || !references || !entries) ok = 0;
    for (unsigned i = 0; ok && i < lines; ++i) {
        const u16* row = raster + i * 22;
        unsigned slot = hashRow(row) & (Slots - 1);
        for (;; slot = (slot + 1) & (Slots - 1)) {
            const unsigned e = slots[slot];
            if (!e || !memcmp(entries + (e - 1) * 22, row, 44)) break;
        }
        if (!slots[slot]) {
            if (nentries >= Slots / 2) { ok = dw_fail("the first room has too many distinct rows"); break; }
            memcpy(entries + nentries * 22, row, 44);
            slots[slot] = (u16)++nentries;
        }
        references[i] = (u16)(slots[slot] - 1);
    }
    if (ok) {
        Buf* out = tables_add(&c->tables, "TINI");
        if (!out) ok = 0;
        else {
            buf_put(out, "DWIT", 4);
            buf_u16(out, 2); buf_u16(out, (u32)rows); buf_u16(out, nentries); buf_u16(out, 0);
            for (unsigned i = 0; i < lines; ++i) buf_u16(out, references[i]);
            for (unsigned i = 0; i < nentries * 22; ++i) buf_u16(out, entries[i]);
            for (unsigned i = 0; i < 16 * 22; ++i) buf_u16(out, corridor[i]);
            if (out->failed) ok = dw_fail("not enough memory");
        }
    }
    dw_free(entries); dw_free(references); dw_free(slots); dw_free(raster);
    return ok;
}
