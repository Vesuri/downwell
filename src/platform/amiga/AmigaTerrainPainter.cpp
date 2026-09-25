#include "AmigaTerrainPainter.h"
#include "core/GameplayWorld.h"
#include "core/Profile.h"
#include "core/ShopFixtures.h"

namespace {
static bool roomHasMainWall(const downwell::RoomPack& rooms,
                            const downwell::PackedRoom& room,
                            int x, int y) {
    unsigned index;
    for (index = 0; index < room.instanceCount; ++index) {
        downwell::PackedRoomInstance instance;
        if (rooms.roomInstance(room, index, instance) &&
            instance.objectId == 60 && instance.x == x && instance.y == y)
            return true;
    }
    return false;
}

static UWORD mainWallFrame(const AmigaTerrainIndex* terrainIndex,
                           const downwell::RoomPack& rooms,
                           const downwell::PackedRoom& room,
                           int x, int y) {
    DW_PROFILE_SCOPE(profileWallFrame);
    if (terrainIndex) {
        const auto& index = *terrainIndex;
        return (amigaIndexedWall(index, x, y - 16) ? 1 : 0) |
               (amigaIndexedWall(index, x + 16, y) ? 2 : 0) |
               (amigaIndexedWall(index, x, y + 16) ? 4 : 0) |
               (amigaIndexedWall(index, x - 16, y) ? 8 : 0);
    }
    UWORD frame = 0;
    if (roomHasMainWall(rooms, room, x, y - 16)) frame |= 1;
    if (roomHasMainWall(rooms, room, x + 16, y)) frame |= 2;
    if (roomHasMainWall(rooms, room, x, y + 16)) frame |= 4;
    if (roomHasMainWall(rooms, room, x - 16, y)) frame |= 8;
    return frame;
}
}

void AmigaTerrainPainter::paint(AmigaRenderer* renderer,
    const downwell::RoomPack& rooms, const downwell::PackedRoom& room,
    const AmigaTerrainIndex* terrainIndex, const AmigaTerrainTiles& terrainTiles,
    const AmigaTerrainDecoration* decorations, unsigned decorationCount,
    const GameplayWorld* runtime, const UBYTE* pack, const UBYTE* end,
    int viewX, int viewY, int clipX, int clipY, int clipWidth, int clipHeight) {
    unsigned index;
    const int firstWorldY = viewY + clipY;
    const int finalWorldY = firstWorldY + clipHeight;
    const auto drawGameAsset = [pack, end](AmigaRenderer* target, UWORD sprite,
        UWORD frame, int x, int y, int flip) {
        DW_PROFILE_SCOPE(profileTerrainAsset);
        amigaRendererDrawAsset(target, pack, end, sprite, frame, x, y, flip);
    };
    auto drawIndexedWalls = [&](AmigaRenderer* renderer,
        const downwell::RoomPack& rooms, const downwell::PackedRoom& room,
        int viewX, int viewY, int clipX, int clipY, int width, int height) {
        const int firstX = viewX + clipX - downwellPlayfieldX;
        const int firstY = viewY + clipY;
        for (int y = (firstY - 8) & ~15; y < firstY + height + 8; y += 16)
            for (int x = (firstX - 8) & ~15; x < firstX + width + 8; x += 16)
                if (amigaIndexedWall(*terrainIndex, x, y))
                    drawGameAsset(renderer, 51, mainWallFrame(terrainIndex, rooms, room, x, y),
                                  downwellPlayfieldX + x - viewX, y - viewY, 0);
    };
    amigaRendererSetClipRect(renderer, clipX, clipY, clipWidth, clipHeight);
    {
    DW_PROFILE_SCOPE(profileTerrainWalls);
    if (terrainIndex)
        drawIndexedWalls(renderer, rooms, room, viewX, viewY,
                             clipX, clipY, clipWidth, clipHeight);
    for (index = 0; !terrainIndex && index < room.instanceCount; ++index) {
        downwell::PackedRoomInstance instance;
        if (!rooms.roomInstance(room, index, instance) ||
            instance.objectId != 60 || instance.y + 8 < firstWorldY ||
            instance.y - 8 >= finalWorldY)
            continue;
        drawGameAsset(renderer, 51,
                      mainWallFrame(terrainIndex, rooms, room, instance.x, instance.y),
                      downwellPlayfieldX + instance.x - viewX,
                      instance.y - viewY, 0);
    }
    }
    {
    DW_PROFILE_SCOPE(profileTerrainTiles);
    for (int rowY = (firstWorldY - 8) & ~15; rowY < finalWorldY + 8; rowY += 16) {
        for (unsigned id = terrainTiles.first(rowY); id; ) {
            const auto& tile = terrainTiles.at(id);
            id = tile.next;
            if (tile.objectId == 92 && runtime &&
                !runtime->breakableActiveAt(tile.x - 240, rowY - 176)) continue;
            drawGameAsset(renderer, 51, tile.frame,
                          downwellPlayfieldX + tile.x - viewX, rowY - viewY, 0);
        }
    }
    }
    {
    DW_PROFILE_SCOPE(profileTerrainDecorations);
    for (index = 0; index < decorationCount; ++index) {
        const AmigaTerrainDecoration& decoration = decorations[index];
        if (decoration.y + 24 < firstWorldY ||
            decoration.y - 24 >= finalWorldY)
            continue;
        drawGameAsset(renderer, (UWORD)decoration.spriteId,
                      (UWORD)decoration.frame,
                      downwellPlayfieldX + decoration.x - viewX,
                      decoration.y - viewY,
                      decoration.flipX ? 1 : 0);
    }
    }
    if (runtime) downwell::drawShopFixtures(runtime->shops(), [&](int sprite, int frame, int x, int y) {
        if (y + 24 >= firstWorldY && y - 24 < finalWorldY)
            drawGameAsset(renderer, sprite, frame, downwellPlayfieldX + x - viewX, y - viewY, 0);
    });
    amigaRendererClearClipRect(renderer);
}
