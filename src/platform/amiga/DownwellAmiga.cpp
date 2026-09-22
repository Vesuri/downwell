#include <proto/exec.h>
#include <proto/graphics.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include "AmigaInput.h"
#include "AmigaPlayer.h"
#include "AmigaPortRuntime.h"
#include "AmigaRenderer.h"
#include "AmigaSceneRuntime.h"
#include "framework/AmigaHardware.h"
#include "framework/AmigaMemory.h"
#include "framework/AmigaPlatform.h"
#include "framework/Bitmap.h"
#include "framework/CopperList.h"
#include "framework/Palette.h"
#include "framework/Sprite.h"
#include "core/BuilderPatternPack.h"
#include "core/ObjectPack.h"
#include "core/PortCheckpoint.h"
#include "core/PortReplay.h"
#include "core/RoomPack.h"
#include "core/Replay.h"

inline void* operator new(__SIZE_TYPE__, void* address) noexcept {
    return address;
}

extern "C" {
extern const UBYTE amiga_hud_pack[];
extern const UBYTE amiga_hud_pack_end[];
extern const UBYTE amiga_game_pack[];
extern const UBYTE amiga_game_pack_end[];
extern const UBYTE amiga_room_pack[];
extern const UBYTE amiga_room_pack_end[];
extern const UBYTE amiga_object_pack[];
extern const UBYTE amiga_object_pack_end[];
extern const UBYTE amiga_builder_pack[];
extern const UBYTE amiga_builder_pack_end[];
}

enum {
    copperWords = 160,
    playerSpriteHeight = 32,
    playerSpriteBytes = (playerSpriteHeight + 2) * 4,
    bulletSpriteHeight = 8,
    bulletSpriteBytes = (bulletSpriteHeight + 2) * 4,
    bulletSpriteBufferCount = 2 * AmigaPortRuntime::bulletCapacity,
    enemySpriteHeight = 32,
    enemySpriteBytes = (enemySpriteHeight + 2) * 4,
    fetchCropBytes = 2,
    fetchWords = 22,
    fetchWidth = fetchWords * 16,
    displayHStart = downwellDisplayHStart + fetchCropBytes * 8,
    displayHStop = displayHStart + fetchWidth,
    displayVStart = downwellDisplayVStart,
    displayVStop = displayVStart + downwellDisplayHeight,
    fetchStart = 0x30,
    fetchStop = fetchStart + 8 * (fetchWords - 1),
    fetchModulo = downwellBytesPerPlaneLine - fetchWords * 2,
    initialRingOrigin = 300,
    mainViewX = 160,
    mainInitialViewY = 34,
    mainBuilderStartY = 1168,
    gameplayPlayerStartY = 142,
    gameplayPlayerGroundY = 200
};

struct NativeFragment {
    downwell::PackedBuilderPattern pattern;
    int y;
    bool reversed;
    bool sideRoom;
    bool appendPillars;
};

struct TerrainCollisionContext {
    const downwell::RoomPack* rooms;
    const downwell::PackedRoom* room;
    const downwell::BuilderPatternPack* builders;
    const NativeFragment* fragments;
    unsigned fragmentCount;
    const AmigaPortRuntime* runtime;
};

alignas(AmigaPortRuntime)
static UBYTE portRuntimeStorage[sizeof(AmigaPortRuntime)];

enum { nativeFragmentCount = 5, nativeBreakableCapacity = 96 };

static TerrainCollisionContext terrainCollision;

static_assert(displayHStart == 0x71, "cropped display starts at logical x=16");
static_assert(displayHStop == 0x1d1, "352-pixel cropped display window");
static_assert(displayVStop == 0x12e, "284-line display window");
static_assert(fetchWords == 22, "352-pixel DMA fetch");
static_assert(fetchStop == 0xd8, "22-word lores fetch at hardware limit");
static_assert(fetchModulo == 4, "skip two bytes on each side per row");
static_assert(fetchStart == (displayHStart >> 1) - 8,
              "lores fetch/display alignment");

static volatile UBYTE* const ciaaPra = (volatile UBYTE*)0xbfe001;
static volatile unsigned long long lastPortCheckpoint;
static const UBYTE* activeGamePack = amiga_game_pack;
static const UBYTE* activeGamePackEnd = amiga_game_pack_end;

static const uint16_t downwellPaletteData[32] = {
    0x000, 0xfff, 0xf00, 0x000,
    0x000, 0x000, 0x000, 0x000,
    0x000, 0xfff, 0xf00, 0x000,
    0x000, 0x000, 0x000, 0x000,
    0x000, 0xfff, 0xf00, 0x000,
    0x000, 0xfff, 0xf00, 0x000,
    0x000, 0xfff, 0xf00, 0x000,
    0x000, 0xfff, 0xf00, 0x000
};

static_assert(
    copperWords >=
        6u + 16u + 32u +
        2u * (sizeof(downwellPaletteData) / sizeof(downwellPaletteData[0])) +
        4u + 8u + 2u,
    "copper storage must cover registers, planes, sprites, palette and split");

#ifdef DOWNWELL_REPLAY_VERIFY
extern "C" {
volatile ULONG gReplayComplete;
volatile ULONG gReplayFrames;
volatile ULONG gReplayDigestHigh;
volatile ULONG gReplayDigestLow;

__attribute__((noinline, used)) void downwellReplayComplete(void) {
    __asm__ volatile("" ::: "memory");
}
}
#endif

static void drawAsset(AmigaRenderer* renderer, UWORD sprite, UWORD frame,
                      int anchorX, int anchorY, int flipX) {
    amigaRendererDrawAsset(renderer, amiga_hud_pack, amiga_hud_pack_end,
                           sprite, frame, anchorX, anchorY, flipX);
}

static void drawGameAsset(AmigaRenderer* renderer, UWORD sprite, UWORD frame,
                          int anchorX, int anchorY, int flipX) {
    amigaRendererDrawAsset(renderer, activeGamePack, activeGamePackEnd,
                           sprite, frame, anchorX, anchorY, flipX);
}

static void drawGameText(AmigaRenderer* renderer, UWORD text,
                         int x, int y) {
    amigaRendererDrawAssetPen(renderer, activeGamePack, activeGamePackEnd,
                              text, 0, x + 1, y + 1, 0, 2);
    amigaRendererDrawAssetPen(renderer, activeGamePack, activeGamePackEnd,
                              text, 0, x, y, 0, 1);
}

static void drawHealthNumber(AmigaRenderer* renderer, int x, int y,
                             unsigned health, unsigned maximum) {
    const int slashX = x - 4;
    const int digitY = y - 5;
    const int healthX = slashX - 9;
    const int maximumX = slashX + 9;
    drawAsset(renderer, 413, health + 20, healthX + 1, digitY, 0);
    drawAsset(renderer, 413, health + 20, healthX + 1, digitY + 1, 0);
    drawAsset(renderer, 413, health + 20, healthX, digitY + 1, 0);
    drawAsset(renderer, 413, health, healthX, digitY, 0);
    drawAsset(renderer, 414, 2, slashX + 1, digitY, 0);
    drawAsset(renderer, 414, 2, slashX + 1, digitY + 1, 0);
    drawAsset(renderer, 414, 2, slashX, digitY + 1, 0);
    drawAsset(renderer, 414, 0, slashX, digitY, 0);
    drawAsset(renderer, 413, maximum + 20, maximumX + 1, digitY, 0);
    drawAsset(renderer, 413, maximum + 20, maximumX + 1, digitY + 1, 0);
    drawAsset(renderer, 413, maximum + 20, maximumX, digitY + 1, 0);
    drawAsset(renderer, 413, maximum, maximumX, digitY, 0);
}

static void drawCurrencyDigit(AmigaRenderer* renderer, int x, int y,
                              unsigned digit) {
    const int digitX = x - 8;
    const int digitY = y - 5;
    drawAsset(renderer, 413, digit + 10, digitX - 1, digitY, 0);
    drawAsset(renderer, 413, digit + 10, digitX, digitY - 1, 0);
    drawAsset(renderer, 413, digit + 20, digitX + 1, digitY, 0);
    drawAsset(renderer, 413, digit + 20, digitX, digitY + 1, 0);
    drawAsset(renderer, 413, digit, digitX, digitY, 0);
}

static void drawAmmoDigit(AmigaRenderer* renderer, int x, int y,
                          unsigned digit) {
    static const BYTE offsets[14][2] = {
        {-1, 0}, {0, -1}, {1, 0}, {0, 1}, {-1, 1}, {0, 0}, {1, 1},
        {0, 2}, {0, 0}, {1, -1}, {2, 0}, {1, 1}, {0, 1}, {1, 0}
    };
    unsigned index;
    for (index = 0; index < 12; ++index)
        drawAsset(renderer, 413, digit + 20,
                  x + offsets[index][0], y + offsets[index][1], 0);
    for (; index < 14; ++index)
        drawAsset(renderer, 413, digit + 10,
                  x + offsets[index][0], y + offsets[index][1], 0);
    drawAsset(renderer, 413, digit, x, y, 0);
}

static void redrawAmmo(AmigaRenderer* renderer, unsigned ammo) {
    amigaRendererClearClipRect(renderer);
    amigaRendererFillRect(renderer, 282, 232, 20, 20, 0);
    drawAmmoDigit(renderer, 290, 239, ammo);
}

static void redrawHealth(AmigaRenderer* renderer, unsigned health) {
    amigaRendererClearClipRect(renderer);
    amigaRendererFillRect(renderer, 8, 6, 92, 28, 0);
    drawAsset(renderer, 415, 1, 8, 6, 0);
    amigaRendererFillRect(renderer, 12, 11, (int)health * 20, 6, 2);
    drawAsset(renderer, 415, 0, 8, 6, 0);
    drawHealthNumber(renderer, 52, 15, health, 4);
}

static void redrawCurrency(AmigaRenderer* renderer, unsigned currency) {
    unsigned divisor = 1;
    unsigned digits = 1;
    unsigned index;
    while (currency / divisor >= 10 && digits < 5) {
        divisor *= 10;
        ++digits;
    }
    amigaRendererClearClipRect(renderer);
    amigaRendererFillRect(renderer, 316, 4, 64, 30, 0);
    drawAsset(renderer, 376, 0, 362, 18, 0);
    for (index = 0; index < digits; ++index) {
        const unsigned digit = (currency / divisor) % 10;
        drawCurrencyDigit(renderer,
                          354 - (int)(digits - index - 1) * 8,
                          18, digit);
        divisor /= 10;
    }
}

static bool isTerrainSymbol(char symbol) {
    return symbol == '1' || symbol == '2' || symbol == '3' ||
           symbol == 'B' || symbol == '-' || symbol == '|';
}

static UWORD terrainFrame(char symbol) {
    if (symbol == 'B' || symbol == '3')
        return 16;
    if (symbol == '-')
        return 23;
    return 0;
}

static int resolveTerrainLanding(void* opaque, int offsetX,
                                 int minimumOffsetY) {
    const TerrainCollisionContext& context =
        *static_cast<const TerrainCollisionContext*>(opaque);
    const int worldX = 240 + offsetX;
    int landing = 4096 - 176;
    unsigned fragmentIndex;
    for (fragmentIndex = 0; fragmentIndex < context.fragmentCount;
         ++fragmentIndex) {
        const NativeFragment& fragment = context.fragments[fragmentIndex];
        UWORD row;
        for (row = 0; row < fragment.pattern.rowCount; ++row) {
            ULONG width = 0;
            unsigned column;
            const int candidate = fragment.y + (int)row * 16 - 15 - 176;
            if (candidate < minimumOffsetY || candidate >= landing)
                continue;
            if (!context.builders->rowWidth(fragment.pattern, row, width))
                break;
            for (column = 0; column < width; ++column) {
                char symbol = 0;
                const int ordinal = (int)column + 1;
                const int tileX = fragment.reversed
                                      ? 320 - ordinal * 16
                                      : 160 + ordinal * 16;
                if (!context.builders->cell(fragment.pattern, (UWORD)column,
                                            row, symbol))
                    break;
                const bool active = symbol != 'B' || context.runtime == 0 ||
                    context.runtime->breakableActiveAt(
                        tileX - 240,
                        fragment.y + (int)row * 16 - 176);
                if (active && isTerrainSymbol(symbol) &&
                    worldX >= tileX - 8 && worldX < tileX + 8) {
                    landing = candidate;
                    break;
                }
            }
        }
    }
    return landing;
}

static bool resolveTerrainSolid(void* opaque, int offsetX, int offsetY) {
    const TerrainCollisionContext& context =
        *static_cast<const TerrainCollisionContext*>(opaque);
    const int worldX = 240 + offsetX;
    const int worldY = 176 + offsetY;
    if (context.rooms != 0 && context.room != 0) {
        for (unsigned index = 0; index < context.room->instanceCount;
             ++index) {
            downwell::PackedRoomInstance instance;
            if (!context.rooms->roomInstance(*context.room, index, instance) ||
                (instance.objectId != 60 && instance.objectId != 92))
                continue;
            if (worldX >= instance.x - 8 && worldX < instance.x + 8 &&
                worldY >= instance.y - 8 && worldY < instance.y + 8)
                return true;
        }
    }
    for (unsigned fragmentIndex = 0;
         fragmentIndex < context.fragmentCount; ++fragmentIndex) {
        const NativeFragment& fragment = context.fragments[fragmentIndex];
        for (UWORD row = 0; row < fragment.pattern.rowCount; ++row) {
            const int tileY = fragment.y + (int)row * 16;
            ULONG width = 0;
            if (worldY < tileY - 8 || worldY >= tileY + 8)
                continue;
            if (row + 1 < fragment.pattern.rowCount) {
                const int firstX = fragment.sideRoom
                                       ? (fragment.reversed ? 0 : 480)
                                       : (fragment.reversed ? 160 : 320);
                const int secondX = fragment.reversed ? 320 : 160;
                if ((worldX >= firstX - 8 && worldX < firstX + 8) ||
                    (worldX >= secondX - 8 && worldX < secondX + 8))
                    return true;
            }
            if (!context.builders->rowWidth(fragment.pattern, row, width))
                break;
            for (unsigned column = 0; column < width; ++column) {
                char symbol = 0;
                const int ordinal = (int)column + 1;
                const int tileX = fragment.reversed
                                      ? 320 - ordinal * 16
                                      : 160 + ordinal * 16;
                if (!context.builders->cell(fragment.pattern, (UWORD)column,
                                            row, symbol))
                    break;
                if (worldX < tileX - 8 || worldX >= tileX + 8 ||
                    !isTerrainSymbol(symbol))
                    continue;
                if (symbol != 'B' || context.runtime == 0 ||
                    context.runtime->breakableActiveAt(
                        tileX - 240, tileY - 176))
                    return true;
            }
        }
        if (fragment.appendPillars) {
            const int finalRowY = fragment.y +
                ((int)fragment.pattern.rowCount - 1) * 16;
            if (worldY >= finalRowY - 8 &&
                worldY < finalRowY + 3 * 16 - 8) {
                const int firstX = fragment.sideRoom
                                       ? (fragment.reversed ? 0 : 480)
                                       : (fragment.reversed ? 160 : 320);
                const int secondX = fragment.reversed ? 320 : 160;
                if ((worldX >= firstX - 8 && worldX < firstX + 8) ||
                    (worldX >= secondX - 8 && worldX < secondX + 8))
                    return true;
            }
        }
    }
    return false;
}

static unsigned appendFragmentBreakables(
    const downwell::BuilderPatternPack& builders,
    const downwell::PackedBuilderPattern& pattern,
    int patternY, bool reversed, int* x, int* y,
    unsigned count, unsigned capacity) {
    UWORD row;
    for (row = 0; row < pattern.rowCount && count < capacity; ++row) {
        ULONG width = 0;
        unsigned column;
        if (!builders.rowWidth(pattern, row, width))
            break;
        for (column = 0; column < width && count < capacity; ++column) {
            char symbol = 0;
            const int ordinal = (int)column + 1;
            if (!builders.cell(pattern, (UWORD)column, row, symbol))
                break;
            if (symbol != 'B')
                continue;
            x[count] = (reversed ? 320 - ordinal * 16
                                 : 160 + ordinal * 16) - 240;
            y[count] = patternY + (int)row * 16 - 176;
            ++count;
        }
    }
    return count;
}

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

static UWORD mainWallFrame(const downwell::RoomPack& rooms,
                           const downwell::PackedRoom& room,
                           int x, int y) {
    UWORD frame = 0;
    if (roomHasMainWall(rooms, room, x, y - 16)) frame |= 1;
    if (roomHasMainWall(rooms, room, x + 16, y)) frame |= 2;
    if (roomHasMainWall(rooms, room, x, y + 16)) frame |= 4;
    if (roomHasMainWall(rooms, room, x - 16, y)) frame |= 8;
    return frame;
}

static void drawMainRoomWalls(AmigaRenderer* renderer,
                              const downwell::RoomPack& rooms,
                              const downwell::PackedRoom& room,
                              int viewX, int viewY) {
    unsigned index;
    for (index = 0; index < room.instanceCount; ++index) {
        downwell::PackedRoomInstance instance;
        if (!rooms.roomInstance(room, index, instance) ||
            instance.objectId != 60 || instance.y + 8 < viewY ||
            instance.y - 8 >= viewY + downwellDisplayHeight)
            continue;
        drawGameAsset(renderer, 51,
                      mainWallFrame(rooms, room, instance.x, instance.y),
                      downwellPlayfieldX + instance.x - viewX,
                      instance.y - viewY, 0);
    }
}

static void buildReferencePicture(
    AmigaRenderer* renderer,
    const downwell::RoomPack& rooms,
    const downwell::BuilderPatternPack& builders,
    const downwell::PackedBuilderPattern& entrance,
    int viewY) {
    downwell::PackedRoom room;

    amigaRendererClear(renderer, 0);
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, 0,
                            downwellPlayfieldWidth, downwellDisplayHeight);
    if (rooms.findRoom(11, room))
        drawMainRoomWalls(renderer, rooms, room, mainViewX, viewY);

    amigaRendererClearClipRect(renderer);
    drawAsset(renderer, 408, 0, 108, 0, 0);
    drawAsset(renderer, 408, 0, 272, 0, 1);

    drawAsset(renderer, 415, 1, 8, 6, 0);
    amigaRendererFillRect(renderer, 12, 11, 80, 6, 2);
    drawAsset(renderer, 415, 0, 8, 6, 0);
    drawHealthNumber(renderer, 52, 15, 4, 4);

    drawAsset(renderer, 416, 0, 284, 43, 0);
    amigaRendererFillRect(renderer, 289, 49, 10, 184, 1);
    drawAsset(renderer, 416, 1, 284, 43, 0);
    drawAsset(renderer, 376, 0, 362, 18, 0);
    drawCurrencyDigit(renderer, 354, 18, 0);
    drawAmmoDigit(renderer, 290, 239, 8);
    (void)builders;
    (void)entrance;
}

static void drawSplash(AmigaRenderer* renderer, int stage) {
    amigaRendererClear(renderer, 0);
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, 0,
                            downwellPlayfieldWidth, downwellDisplayHeight);
    if (stage == 2)
        drawGameAsset(renderer, 696, 0,
                      downwellPlayfieldX + 80, 140, 0);
    if (stage == 4 || stage == 5)
        drawGameAsset(renderer, 695, 0,
                      downwellPlayfieldX + 80, 140, 0);
    if (stage == 5 || stage == 6) {
        int y;
        int x;
        for (y = 0; y < downwellDisplayHeight; y += 8)
            for (x = downwellPlayfieldX;
                 x < downwellPlayfieldX + downwellPlayfieldWidth; x += 8)
                drawGameAsset(renderer, 733, 11, x, y, 0);
    }
    amigaRendererClearClipRect(renderer);
}

struct MenuDrawItem {
    int x;
    int y;
    int objectId;
    int spriteId;
    int depth;
};

struct MenuDrawList {
    MenuDrawItem items[256];
    unsigned count;
};

static MenuDrawList menuDrawList;

static bool buildMenuDrawList(const downwell::RoomPack& rooms,
                              const downwell::ObjectPack& objects,
                              MenuDrawList& list) {
    downwell::PackedRoom room;
    unsigned index;
    list.count = 0;
    if (!rooms.findRoom(20, room))
        return false;
    for (index = 0; index < room.instanceCount && list.count < 256; ++index) {
        downwell::PackedRoomInstance instance;
        downwell::PackedObject object;
        if (!rooms.roomInstance(room, index, instance) ||
            !objects.findObject(instance.objectId, object) ||
            !object.visible() || object.spriteId < 0 ||
            instance.objectId == 35)
            continue;
        MenuDrawItem item = {
            instance.x,
            instance.y,
            instance.objectId,
            instance.objectId == 60 ? 76 : object.spriteId,
            object.depth
        };
        unsigned destination = list.count;
        while (destination > 0 &&
               list.items[destination - 1].depth < item.depth) {
            list.items[destination] = list.items[destination - 1];
            --destination;
        }
        list.items[destination] = item;
        ++list.count;
    }
    return true;
}

static void drawMenu(AmigaRenderer* renderer,
                     const MenuDrawList& list,
                     const AmigaSceneRuntime& scenes) {
    unsigned index;
    const int viewX = scenes.menuCameraX() - 80;
    const int viewY = scenes.menuCameraY() - 142;

    amigaRendererClear(renderer, 0);
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, 0,
                            downwellPlayfieldWidth, downwellDisplayHeight);
    for (index = 0; index < list.count; ++index) {
        const MenuDrawItem& item = list.items[index];
        int roomX = item.x;
        int roomY = item.y;
        if (item.objectId == 312) {
            roomX = 120 + viewX * 5 / 6;
            roomY = (viewY + 142) * 10 / 11;
        }
        if (item.objectId == 315)
            amigaRendererFillRect(renderer,
                downwellPlayfieldX + roomX - 24 - viewX,
                roomY + 64 - viewY, 160, 1000, 0);
        drawGameAsset(renderer, (UWORD)item.spriteId, 0,
                      downwellPlayfieldX + roomX - viewX,
                      roomY - viewY, 0);
    }

    drawGameAsset(renderer, 654, (UWORD)scenes.menuTitleFrame(),
                  downwellPlayfieldX + 328 - viewX, 432 - viewY, 0);
    if (scenes.menuPlayerVisible()) {
        UWORD sprite = 31;
        UWORD frame = (UWORD)((scenes.tick() / 10U) & 3U);
        if (scenes.menuPlayerNapping()) {
            sprite = 12;
            frame = 0;
        } else if (scenes.menuPlayerY() > 512) {
            sprite = 37;
            frame = 4;
        } else if ((scenes.tick() & 7U) != 0) {
            sprite = 27;
            frame = (UWORD)((scenes.tick() / 3U) & 7U);
        }
        drawGameAsset(renderer, sprite, frame,
                      downwellPlayfieldX + scenes.menuPlayerX() - viewX,
                      scenes.menuPlayerY() - viewY,
                      scenes.menuPlayerFacing() < 0);
    }
    amigaRendererClearClipRect(renderer);
}

static void drawStyleSelect(AmigaRenderer* renderer,
                            const AmigaSceneRuntime& scenes) {
    const int centerY = downwellDisplayHeight / 2 + scenes.styleYOffset();
    const int smoothX = scenes.styleSmoothX();
    int style;
    int row;
    amigaRendererClear(renderer, 0);
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, 0,
                            downwellPlayfieldWidth, downwellDisplayHeight);
    if (!scenes.styleSelected()) {
        for (style = 0; style < 5; ++style) {
            const int drawX = 80 + style * 48 - smoothX;
            const int distance = drawX - 80;
            const int drawY = centerY - 14 - distance * distance / 200;
            drawGameAsset(renderer, style == 0 ? 27 : 43,
                          style == scenes.styleCursor()
                              ? (UWORD)scenes.styleRunFrame() : 0,
                          downwellPlayfieldX + drawX, drawY, 0);
        }
        for (row = 0; row <= 36; ++row) {
            drawGameAsset(renderer, 51, 13, downwellPlayfieldX,
                          scenes.styleTileY() + row * 16, 0);
            drawGameAsset(renderer, 51, 7,
                          downwellPlayfieldX + downwellPlayfieldWidth,
                          scenes.styleTileY() + row * 16, 0);
        }
        if (scenes.styleCursor() < 4)
            drawGameAsset(renderer, 44, 0, downwellPlayfieldX + 96,
                          centerY - 10, 0);
        if (scenes.styleCursor() > 0)
            drawGameAsset(renderer, 44, 0, downwellPlayfieldX + 64,
                          centerY - 10, 1);
        drawGameText(renderer, 65000, downwellPlayfieldX + 80,
                     centerY - 70);
        drawGameText(renderer, scenes.styleCursor() == 0 ? 65001 : 65003,
                     downwellPlayfieldX + 80, centerY + 10);
        if (scenes.styleCursor() == 0)
            amigaRendererDrawAssetPen(
                renderer, activeGamePack, activeGamePackEnd,
                65002, 0, downwellPlayfieldX + 80, centerY + 32,
                0, 1);
    } else {
        drawGameAsset(renderer, 27, (UWORD)scenes.styleRunFrame(),
                      downwellPlayfieldX + 80,
                      centerY - 14 + scenes.stylePlayerDescent(), 0);
    }
    if (scenes.styleDitherY() < downwellDisplayHeight) {
        int y;
        int x;
        for (y = scenes.styleDitherY(); y < downwellDisplayHeight; y += 8)
            for (x = 0; x < downwellPlayfieldWidth; x += 8)
                drawGameAsset(renderer, 733, 11,
                              downwellPlayfieldX + x, y, 0);
    }
    if (scenes.styleSelected() && scenes.styleVerticalDitherY() > 0) {
        int y;
        int x;
        int bottom = scenes.styleVerticalDitherY();
        if (bottom > downwellDisplayHeight)
            bottom = downwellDisplayHeight;
        for (y = 0; y < bottom; y += 8)
            for (x = 0; x < downwellPlayfieldWidth; x += 8)
                drawGameAsset(renderer, 733, 11,
                              downwellPlayfieldX + x, y, 0);
    }
    amigaRendererClearClipRect(renderer);
}

static void drawMainWorldLine(
    AmigaRenderer* renderer,
    const downwell::RoomPack& rooms,
    const downwell::PackedRoom& room,
    const downwell::BuilderPatternPack& builders,
    const NativeFragment* fragments,
    unsigned fragmentCount,
    const AmigaPortRuntime* runtime,
    int viewX, int worldY, int screenY) {
    unsigned index;
    const int centerY = ((worldY + 8) / 16) * 16;
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, screenY,
                            downwellPlayfieldWidth, 1);
    for (index = 0; index < room.instanceCount; ++index) {
        downwell::PackedRoomInstance instance;
        if (!rooms.roomInstance(room, index, instance) ||
            instance.objectId != 60 || worldY < instance.y - 8 ||
            worldY > instance.y + 7)
            continue;
        drawGameAsset(renderer, 51,
                      mainWallFrame(rooms, room, instance.x, instance.y),
                      downwellPlayfieldX + instance.x - viewX,
                      screenY + instance.y - worldY, 0);
    }

    if (centerY >= 704 && centerY < mainBuilderStartY) {
        drawGameAsset(renderer, 51, 0, downwellPlayfieldX,
                      screenY + centerY - worldY, 0);
        drawGameAsset(renderer, 51, 0,
                      downwellPlayfieldX + downwellPlayfieldWidth,
                      screenY + centerY - worldY, 0);
    }
    for (index = 0; index < fragmentCount; ++index) {
        const NativeFragment& fragment = fragments[index];
        const int finalRowY = fragment.y +
            ((int)fragment.pattern.rowCount - 1) * 16;
        const int finalY = finalRowY +
            (fragment.appendPillars ? 3 * 16 : 16);
        if (centerY < fragment.y || centerY >= finalY)
            continue;
        if (centerY >= finalRowY) {
            if (fragment.appendPillars) {
                const int firstX = fragment.sideRoom
                                       ? (fragment.reversed ? 0 : 480)
                                       : (fragment.reversed ? 160 : 320);
                const int secondX = fragment.reversed ? 320 : 160;
                drawGameAsset(renderer, 51, 0,
                              downwellPlayfieldX + firstX - viewX,
                              screenY + centerY - worldY, 0);
                drawGameAsset(renderer, 51, 0,
                              downwellPlayfieldX + secondX - viewX,
                              screenY + centerY - worldY, 0);
            }
            if (centerY > finalRowY)
                break;
        }
        {
            const UWORD row =
                (UWORD)((centerY - fragment.y) / 16);
            ULONG width = 0;
            unsigned column;
            if (row + 1 < fragment.pattern.rowCount) {
                const int firstX = fragment.sideRoom
                                       ? (fragment.reversed ? 0 : 480)
                                       : (fragment.reversed ? 160 : 320);
                const int secondX = fragment.reversed ? 320 : 160;
                drawGameAsset(renderer, 51, 0,
                              downwellPlayfieldX + firstX - viewX,
                              screenY + centerY - worldY, 0);
                drawGameAsset(renderer, 51, 0,
                              downwellPlayfieldX + secondX - viewX,
                              screenY + centerY - worldY, 0);
            }
            if (builders.rowWidth(fragment.pattern, row, width)) {
                for (column = 0; column < width; ++column) {
                    char symbol = 0;
                    const int ordinal = (int)column + 1;
                    const int worldX = fragment.reversed
                                           ? 320 - ordinal * 16
                                           : 160 + ordinal * 16;
                    if (!builders.cell(fragment.pattern, (UWORD)column,
                                       row, symbol))
                        break;
                    if (symbol == 'B' && runtime != 0 &&
                        !runtime->breakableActiveAt(
                            worldX - 240, centerY - 176))
                        continue;
                    if (isTerrainSymbol(symbol))
                        drawGameAsset(
                            renderer, 51, terrainFrame(symbol),
                            downwellPlayfieldX + worldX - viewX,
                            screenY + centerY - worldY, 0);
                }
            }
        }
        break;
    }
    amigaRendererClearClipRect(renderer);
}

static void drawRevealedGameLine(AmigaRenderer* renderer,
                                 const downwell::RoomPack& rooms,
                                 const downwell::PackedRoom& room,
                                 const downwell::BuilderPatternPack& builders,
                                 const NativeFragment* fragments,
                                 unsigned fragmentCount,
                                 const AmigaPortRuntime* runtime,
                                 unsigned ringOrigin,
                                 int viewX, int worldY,
                                 int y) {
    amigaRendererSetGameOrigin(renderer, ringOrigin);
    amigaRendererClearGameRows(renderer, y, 1, 0);
    drawMainWorldLine(renderer, rooms, room, builders,
                      fragments, fragmentCount, runtime,
                      viewX, worldY, y);
}

static bool buildCopper(CopperList& list,
                        const Bitmap& game, const Bitmap& hud,
                        const Sprite& playerSprite,
                        const Sprite* bulletSprites,
                        const Sprite& enemySprite,
                        const Palette& palette,
                        unsigned ringOrigin) {
    unsigned channel;
    const unsigned firstSpan = downwellGameRingHeight - ringOrigin;
    list.reset();
    list.move(bplcon0, 0x4401);
    list.move(bpl1mod, fetchModulo);
    list.move(bpl2mod, fetchModulo);
    list.showBitmap(game, 1, 2, fetchCropBytes * 8, ringOrigin, 2);
    list.showBitmap(hud, 2, 2, fetchCropBytes * 8, 0, 2);

    list.showSprite(0, playerSprite);
    for (channel = 0; channel < AmigaPortRuntime::bulletCapacity; ++channel)
        list.showSprite((UWORD)(channel + 1), bulletSprites[channel]);
    list.showSprite(7, enemySprite);

    list.setPalette(palette);

    if (firstSpan < downwellDisplayHeight) {
        const unsigned resetAfterLine =
            displayVStart + firstSpan - 1;
        /*
         * Repoint after the final fetch from the ring tail and before the next
         * line's DDFSTRT. Copper vertical comparison wraps at line 255, so an
         * explicit line-255 barrier is required for splits below that point.
         */
        if (resetAfterLine > 255)
            list.wait(255, 0xdf);
        list.wait((UWORD)resetAfterLine, 0xdf);
        list.showBitmap(game, 1, 2, fetchCropBytes * 8, 0, 2);
    }

    return list.finish();
}

static void buildBulletSprite(Sprite& sprite, int x, int y) {
    UWORD* data = sprite.data();
    const int startX = downwellDisplayHStart + x;
    const int startY = downwellDisplayVStart + y;
    unsigned line;
    sprite.setX((uint16_t)startX);
    sprite.setY((uint16_t)startY);
    for (line = 0; line < bulletSpriteHeight; ++line) {
        data[2 + line * 2] = line < 2 ? 0xc000 : 0;
        data[3 + line * 2] = 0xc000;
    }
    data[2 + bulletSpriteHeight * 2] = 0;
    data[3 + bulletSpriteHeight * 2] = 0;
}

int main(void) {
    AmigaPlatform platform;
    AmigaMemory memory;
    AmigaRenderer renderer;
    unsigned front = 0;
    int worldViewX = mainViewX;
    int worldViewY = mainInitialViewY;
#ifdef DOWNWELL_REPLAY_VERIFY
    AmigaPortRuntime& port = *new (portRuntimeStorage)
        AmigaPortRuntime(initialRingOrigin);
#else
    AmigaPortRuntime& port = *new (portRuntimeStorage)
        AmigaPortRuntime(initialRingOrigin, false, 1201 - 176, true);
#endif
#ifndef DOWNWELL_REPLAY_VERIFY
    AmigaSceneRuntime scenes;
    AmigaScene presentedScene = scenes.scene();
    int presentedSplash = scenes.splashStage();
#endif

    if (!platform.available())
        return 1;
    const downwell::RoomPack rooms(
        amiga_room_pack,
        (unsigned long)(amiga_room_pack_end - amiga_room_pack));
    const downwell::ObjectPack objects(
        amiga_object_pack,
        (unsigned long)(amiga_object_pack_end - amiga_object_pack));
    const downwell::BuilderPatternPack builders(
        amiga_builder_pack,
        (unsigned long)(amiga_builder_pack_end - amiga_builder_pack));
#ifdef DOWNWELL_REPLAY_VERIFY
    const downwell::ReplayReader verificationReplay(
        downwell::portReplayBytes, downwell::portReplaySize);
#endif
    downwell::PackedBuilderPattern entrance;
    downwell::PackedBuilderPattern main32;
    downwell::PackedBuilderPattern main30;
    downwell::PackedBuilderPattern main27;
    downwell::PackedBuilderPattern right3;
    downwell::PackedRoom mainRoom;
    if (!rooms.valid() || !objects.valid() || !builders.valid() ||
#ifdef DOWNWELL_REPLAY_VERIFY
        !verificationReplay.valid() ||
#endif
        !rooms.findRoom(11, mainRoom) ||
        !buildMenuDrawList(rooms, objects, menuDrawList) ||
        !builders.findPattern(downwell::BuilderPatternFamily::areaOneStart,
                              3, entrance) ||
        !builders.findPattern(downwell::BuilderPatternFamily::areaOneMain,
                              32, main32) ||
        !builders.findPattern(downwell::BuilderPatternFamily::areaOneMain,
                              30, main30) ||
        !builders.findPattern(downwell::BuilderPatternFamily::areaOneMain,
                              27, main27) ||
        !builders.findPattern(downwell::BuilderPatternFamily::areaOneRight,
                              3, right3)) {
        return 1;
    }
    const NativeFragment fragments[nativeFragmentCount] = {
        {entrance, 1168, true, false, false},
        {main32, 1248, true, false, true},
        {main30, 1472, true, false, true},
        {main27, 1776, true, false, true},
        {right3, 1968, false, true, true}
    };
#ifndef DOWNWELL_REPLAY_VERIFY
    {
        static int breakableX[nativeBreakableCapacity];
        static int breakableY[nativeBreakableCapacity];
        unsigned breakableCount = 0;
        for (unsigned fragment = 0;
             fragment < nativeFragmentCount; ++fragment)
            breakableCount = appendFragmentBreakables(
                builders, fragments[fragment].pattern,
                fragments[fragment].y, fragments[fragment].reversed,
                breakableX, breakableY, breakableCount,
                nativeBreakableCapacity);
        port.configureBreakables(breakableX, breakableY, breakableCount);
        terrainCollision.builders = &builders;
        terrainCollision.rooms = &rooms;
        terrainCollision.room = &mainRoom;
        terrainCollision.fragments = fragments;
        terrainCollision.fragmentCount = nativeFragmentCount;
        terrainCollision.runtime = &port;
        port.configureLandingResolver(resolveTerrainLanding,
                                      &terrainCollision);
        port.configureSolidResolver(resolveTerrainSolid,
                                    &terrainCollision);
        port.configureEnemy(0, 111, 163, -16, 1122,
                            128, 0, 5, 6);
        port.configureEnemy(1, 111, 163, 48, 1426,
                            128, 0, 5, 6);
        port.configureEnemy(2, 117, 177, 64, 1840,
                            0, 26, 20, 24);
    }
#endif
    const ULONG gamePackBytes =
        (ULONG)(amiga_game_pack_end - amiga_game_pack);
    if (!memory.allocate(downwellGameRingBytes, downwellLayerBytes,
                         copperWords * 2 * sizeof(UWORD), playerSpriteBytes,
                         bulletSpriteBytes * bulletSpriteBufferCount,
                         enemySpriteBytes,
                         gamePackBytes)) {
        return 1;
    }
    for (ULONG byte = 0; byte < gamePackBytes; ++byte)
        memory.assets()[byte] = amiga_game_pack[byte];
    activeGamePack = memory.assets();
    activeGamePackEnd = activeGamePack + gamePackBytes;

    CopperList copperLists[2] = {
        CopperList(memory.copper(), copperWords),
        CopperList(memory.copper() + copperWords, copperWords)
    };
    Bitmap gameBitmap(memory.game(), downwellVisibleWidth,
                      downwellGameRingHeight, 2, false, false,
                      downwellDmaWidth);
    Bitmap hudBitmap(memory.hud(), downwellVisibleWidth,
                     downwellDisplayHeight, 2, false, false,
                     downwellDmaWidth);
    Palette::initialize();
    Palette palette(downwellPaletteData,
                    sizeof(downwellPaletteData) / sizeof(downwellPaletteData[0]));

    amigaRendererInit(&renderer, memory.game(), memory.hud());
    amigaRendererSetGameOrigin(&renderer, initialRingOrigin);
#ifdef DOWNWELL_REPLAY_VERIFY
    buildReferencePicture(&renderer, rooms, builders, entrance,
                          mainInitialViewY);
#else
    drawSplash(&renderer, scenes.splashStage());
#endif
    if (!amigaRendererBuildHardwareSprite(
            activeGamePack, activeGamePackEnd, 31, 0,
            downwellPlayfieldX + 80,
#ifdef DOWNWELL_REPLAY_VERIFY
            gameplayPlayerGroundY,
#else
            gameplayPlayerStartY,
#endif
            0,
            memory.playerSprite(0), playerSpriteHeight) ||
        !amigaRendererBuildHardwareSprite(
            activeGamePack, activeGamePackEnd, 37, 0,
            downwellPlayfieldX + 80,
#ifdef DOWNWELL_REPLAY_VERIFY
            gameplayPlayerGroundY,
#else
            gameplayPlayerStartY,
#endif
            0,
            memory.playerSprite(1), playerSpriteHeight)) {
        memory.release();
        return 1;
    }
    Sprite playerSprites[2] = {
        Sprite(memory.playerSprite(0), playerSpriteHeight),
        Sprite(memory.playerSprite(1), playerSpriteHeight)
    };
    const unsigned bulletSpriteWordStride =
        bulletSpriteBytes / sizeof(UWORD);
    Sprite bulletSprites[bulletSpriteBufferCount] = {
        Sprite(memory.bulletSprite() + bulletSpriteWordStride * 0,
               bulletSpriteHeight),
        Sprite(memory.bulletSprite() + bulletSpriteWordStride * 1,
               bulletSpriteHeight),
        Sprite(memory.bulletSprite() + bulletSpriteWordStride * 2,
               bulletSpriteHeight),
        Sprite(memory.bulletSprite() + bulletSpriteWordStride * 3,
               bulletSpriteHeight),
        Sprite(memory.bulletSprite() + bulletSpriteWordStride * 4,
               bulletSpriteHeight),
        Sprite(memory.bulletSprite() + bulletSpriteWordStride * 5,
               bulletSpriteHeight),
        Sprite(memory.bulletSprite() + bulletSpriteWordStride * 6,
               bulletSpriteHeight),
        Sprite(memory.bulletSprite() + bulletSpriteWordStride * 7,
               bulletSpriteHeight),
        Sprite(memory.bulletSprite() + bulletSpriteWordStride * 8,
               bulletSpriteHeight),
        Sprite(memory.bulletSprite() + bulletSpriteWordStride * 9,
               bulletSpriteHeight),
        Sprite(memory.bulletSprite() + bulletSpriteWordStride * 10,
               bulletSpriteHeight),
        Sprite(memory.bulletSprite() + bulletSpriteWordStride * 11,
               bulletSpriteHeight)
    };
    Sprite nullSprite(memory.nullSprite(), 0);
    Sprite enemySprites[2] = {
        Sprite(memory.enemySprite(0), enemySpriteHeight),
        Sprite(memory.enemySprite(1), enemySpriteHeight)
    };
    for (unsigned bullet = 0; bullet < bulletSpriteBufferCount; ++bullet)
        bulletSprites[bullet].hide();
    enemySprites[0].hide();
    enemySprites[1].hide();
    if (!buildCopper(copperLists[0], gameBitmap, hudBitmap,
#ifdef DOWNWELL_REPLAY_VERIFY
                     playerSprites[0], bulletSprites, enemySprites[0], palette,
#else
                     nullSprite, bulletSprites, enemySprites[0], palette,
#endif
                     initialRingOrigin) ||
        !buildCopper(copperLists[1], gameBitmap, hudBitmap,
#ifdef DOWNWELL_REPLAY_VERIFY
                     playerSprites[1],
                     bulletSprites + AmigaPortRuntime::bulletCapacity,
                     enemySprites[1], palette,
#else
                     nullSprite,
                     bulletSprites + AmigaPortRuntime::bulletCapacity,
                     enemySprites[1], palette,
#endif
                     initialRingOrigin)) {
        memory.release();
        return 1;
    }

    if (!platform.takeover(copperLists[0],
                           displayHStart, displayHStop,
                           displayVStart, displayVStop,
                           fetchStart, fetchStop)) {
        memory.release();
        return 1;
    }
    amigaRendererEnableBlitter(&renderer);
    while ((*ciaaPra & 0x40u) == 0) platform.waitVblank();
    while ((*ciaaPra & 0x40u) != 0) {
        UBYTE packedInput;
        int scrollDelta = 0;
        bool cameraXChanged = false;
        unsigned destroyedBlockCount = 0;
        int destroyedBlockX[AmigaPortRuntime::bulletCapacity] = {0};
        int destroyedBlockY[AmigaPortRuntime::bulletCapacity] = {0};
        platform.waitVblank();
        {
#ifdef DOWNWELL_REPLAY_VERIFY
            downwell::Input replayInput;
            if (!verificationReplay.frame(port.tick(), replayInput))
                break;
            packedInput = downwell::packReplayInput(replayInput);
#else
            const downwell::Input liveInput = AmigaInput::sample();
            packedInput = downwell::packReplayInput(liveInput);
            scenes.step(liveInput);
            {
                const bool presentationChanged =
                    scenes.scene() != presentedScene ||
                    scenes.splashStage() != presentedSplash;
                const unsigned back = front ^ 1u;
                presentedScene = scenes.scene();
                presentedSplash = scenes.splashStage();
                if (presentedScene == AmigaScene::splash) {
                    if (presentationChanged)
                        drawSplash(&renderer, presentedSplash);
                }
                else if (presentedScene == AmigaScene::menu) {
                    if (presentationChanged || !scenes.menuPlayerNapping())
                        drawMenu(&renderer, menuDrawList, scenes);
                }
                else if (presentedScene == AmigaScene::styleSelect)
                    drawStyleSelect(&renderer, scenes);
                else if (presentationChanged)
                    buildReferencePicture(&renderer, rooms, builders, entrance,
                                          mainInitialViewY);
                if (presentationChanged) {
                    if (!buildCopper(copperLists[back], gameBitmap, hudBitmap,
                                     presentedScene == AmigaScene::gameplay
                                         ? playerSprites[0] : nullSprite,
                                     bulletSprites + back *
                                         AmigaPortRuntime::bulletCapacity,
                                     enemySprites[back], palette,
                                     port.ringOrigin()))
                        break;
                    platform.publish(copperLists[back]);
                    front = back;
                }
            }
            if (scenes.scene() != AmigaScene::gameplay)
                continue;
#endif
            const AmigaPortFrame result = port.step(packedInput);
            scrollDelta = result.scrollDelta;
            cameraXChanged = result.cameraXChanged;
            destroyedBlockCount = result.destroyedBlockCount;
            for (unsigned block = 0; block < destroyedBlockCount; ++block) {
                destroyedBlockX[block] = result.blockX[block];
                destroyedBlockY[block] = result.blockY[block];
            }
            if (result.ammoChanged)
                redrawAmmo(&renderer, (unsigned)port.player().ammo());
            if (result.healthChanged)
                redrawHealth(&renderer, (unsigned)port.health());
            if (result.currencyChanged)
                redrawCurrency(&renderer, port.currency());
        }
        {
            const unsigned back = front ^ 1u;
            bool frameReady = true;
            if (scrollDelta != 0) {
                const int direction = scrollDelta > 0 ? 1 : -1;
                const int steps = scrollDelta > 0 ? scrollDelta : -scrollDelta;
                int step;
                int origin = (int)port.ringOrigin() - scrollDelta;
                while (origin < 0)
                    origin += downwellGameRingHeight;
                origin %= downwellGameRingHeight;
                for (step = 0; step < steps; ++step) {
                    worldViewY += direction;
                    origin += direction;
                    if (origin < 0)
                        origin += downwellGameRingHeight;
                    if (origin >= downwellGameRingHeight)
                        origin -= downwellGameRingHeight;
                    drawRevealedGameLine(
                        &renderer, rooms, mainRoom, builders,
                        fragments, nativeFragmentCount,
                        &port,
                        (unsigned)origin,
                        worldViewX,
                        direction > 0
                            ? worldViewY + downwellDisplayHeight - 1
                            : worldViewY,
                        direction > 0 ? downwellDisplayHeight - 1 : 0);
                }
            }
            if (cameraXChanged) {
                worldViewX = mainViewX + port.cameraOffsetX();
                for (int screenY = 0;
                     screenY < downwellDisplayHeight; ++screenY)
                    drawRevealedGameLine(
                        &renderer, rooms, mainRoom, builders,
                        fragments, nativeFragmentCount, &port,
                        port.ringOrigin(), worldViewX,
                        worldViewY + screenY, screenY);
            }
            if (destroyedBlockCount != 0) {
                for (unsigned block = 0;
                     block < destroyedBlockCount; ++block) {
                    const int centerY = 176 + destroyedBlockY[block];
                    int line;
                    for (line = centerY - 8; line <= centerY + 7; ++line) {
                        const int screenY = line - worldViewY;
                        if (screenY < 0 || screenY >= downwellDisplayHeight)
                            continue;
                        drawRevealedGameLine(
                            &renderer, rooms, mainRoom, builders,
                            fragments, nativeFragmentCount,
                            &port, port.ringOrigin(), worldViewX,
                            line, screenY);
                    }
                    (void)destroyedBlockX[block];
                }
            }
            if (!amigaRendererBuildHardwareSprite(
                    activeGamePack, activeGamePackEnd,
                    (UWORD)port.player().spriteId(),
                    (UWORD)port.player().spriteFrame(),
                    downwellPlayfieldX + 80 + port.player().offsetX() -
                        port.cameraOffsetX(),
#ifdef DOWNWELL_REPLAY_VERIFY
                    gameplayPlayerGroundY + port.player().offsetY(),
#else
                    gameplayPlayerStartY + port.player().offsetY() -
                        port.cameraOffsetY(),
#endif
                    port.player().facing() < 0,
                    memory.playerSprite(back), playerSpriteHeight))
                frameReady = false;
            for (unsigned bullet = 0;
                 bullet < AmigaPortRuntime::bulletCapacity; ++bullet) {
                Sprite& sprite = bulletSprites[
                    back * AmigaPortRuntime::bulletCapacity + bullet];
                const AmigaPortRuntime::BulletState& state =
                    port.bulletAt(bullet);
                if (state.active)
                    buildBulletSprite(
                        sprite,
                        downwellPlayfieldX + 80 + state.x -
                            port.cameraOffsetX(),
                        gameplayPlayerStartY + state.y -
                            port.cameraOffsetY());
                else
                    sprite.hide();
            }
#ifndef DOWNWELL_REPLAY_VERIFY
            enemySprites[back].hide();
            bool actorDrawn = false;
            for (unsigned enemyIndex = 0;
                 enemyIndex < port.enemyCount(); ++enemyIndex) {
                const AmigaPortRuntime::EnemyState& enemy =
                    port.enemyAt(enemyIndex);
                const int screenY = gameplayPlayerStartY + enemy.y -
                    port.cameraOffsetY();
                if (!enemy.active || !enemy.awake || screenY < 16 ||
                    screenY >= downwellDisplayHeight)
                    continue;
                if (!amigaRendererBuildHardwareSprite(
                        activeGamePack, activeGamePackEnd,
                        (UWORD)enemy.spriteId,
                        (UWORD)((port.tick() >> 2) & 1u),
                        downwellPlayfieldX + 80 + enemy.x -
                            port.cameraOffsetX(),
                        screenY, enemy.facing < 0,
                        memory.enemySprite(back), enemySpriteHeight))
                    frameReady = false;
                actorDrawn = true;
                break;
            }
            if (!actorDrawn) {
                for (unsigned gemIndex = 0;
                     gemIndex < AmigaPortRuntime::gemCapacity; ++gemIndex) {
                    const AmigaPortRuntime::GemState& gem =
                        port.gemAt(gemIndex);
                    const int screenY = gameplayPlayerStartY + gem.y -
                        port.cameraOffsetY();
                    if (!gem.active || screenY < 16 ||
                        screenY >= downwellDisplayHeight)
                        continue;
                    if (!amigaRendererBuildHardwareSprite(
                            activeGamePack, activeGamePackEnd,
                            (UWORD)(gem.large ? 134 : 133),
                            (UWORD)((gem.age / 2u) & 3u),
                            downwellPlayfieldX + 80 + gem.x -
                                port.cameraOffsetX(),
                            screenY, 0,
                            memory.enemySprite(back), enemySpriteHeight))
                        frameReady = false;
                    break;
                }
            }
#endif
            if (frameReady &&
                !buildCopper(copperLists[back], gameBitmap, hudBitmap,
                             playerSprites[back],
                             bulletSprites + back *
                                 AmigaPortRuntime::bulletCapacity,
                             enemySprites[back], palette,
                             port.ringOrigin()))
                frameReady = false;
            if (!frameReady)
                break;
            /* COP1LC is sampled at the next VERTB; never restart mid-frame. */
            platform.publish(copperLists[back]);
            front = back;
        }
        lastPortCheckpoint = port.trajectoryDigest();
#ifdef DOWNWELL_REPLAY_VERIFY
        if (port.tick() == verificationReplay.frameCount()) {
            gReplayFrames = port.tick();
            gReplayDigestHigh = (ULONG)(lastPortCheckpoint >> 32);
            gReplayDigestLow = (ULONG)lastPortCheckpoint;
            gReplayComplete = 1;
            downwellReplayComplete();
            for (;;) platform.waitVblank();
        }
#endif
    }

    platform.restore();
    memory.release();
    return 0;
}
