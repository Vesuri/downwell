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
#include "AmigaTerrain.h"
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
    /* Channels 1..6 are shared by bullets, enemies and gems each frame. */
    bulletSpriteHeight = 32,
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

alignas(AmigaPortRuntime)
static UBYTE portRuntimeStorage[sizeof(AmigaPortRuntime)];

static AmigaTerrainCollisionContext terrainCollision;

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

#ifndef DOWNWELL_REPLAY_VERIFY
extern "C" __attribute__((noinline, used)) void downwellStartupReady(void) {
    __asm__ volatile("" ::: "memory");
}

extern "C" __attribute__((noinline, used)) void downwellFrameLoopReady(void) {
    __asm__ volatile("" ::: "memory");
}

extern "C" __attribute__((noinline, used)) void downwellMenuSpriteReady(void) {
    __asm__ volatile("" ::: "memory");
}

extern "C" __attribute__((noinline, used)) void downwellSceneGameplayReady(void) {
    __asm__ volatile("" ::: "memory");
}

extern "C" __attribute__((noinline, used)) void downwellSceneGameplayLanded(void) {
    __asm__ volatile("" ::: "memory");
}
#endif

#ifdef DOWNWELL_SCENE_VERIFY
static downwell::Input sceneVerificationInput(
    const AmigaSceneRuntime& scenes) {
    downwell::Input input;
    if (scenes.scene() == AmigaScene::menu)
        input.right = true;
    else if (scenes.scene() == AmigaScene::styleSelect &&
             scenes.styleYOffset() == 0 && !scenes.styleSelected())
        input.fire = true;
    return input;
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
                              text, 0, x, y + 1, 0, 2);
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
    enum {
        gaugeX = 289,
        gaugeTop = 49,
        gaugeWidth = 10,
        gaugeLength = 184,
        maximumAmmo = 8
    };
    if (ammo > maximumAmmo)
        ammo = maximumAmmo;
    const int filled = gaugeLength * (int)ammo / maximumAmmo;
    amigaRendererClearClipRect(renderer);
    /*
     * The frame is static on PF2, but its interior follows gunboot charge.
     * Clear the previous white column before filling upward from the recovered
     * 233-pixel bottom anchor, exactly like the desktop 4:3 HUD path.
     */
    amigaRendererFillRect(renderer, gaugeX, gaugeTop,
                          gaugeWidth, gaugeLength, 0);
    if (filled > 0)
        amigaRendererFillRect(renderer, gaugeX,
                              gaugeTop + gaugeLength - filled,
                              gaugeWidth, filled, 1);
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

static void redrawStreak(AmigaRenderer* renderer, unsigned streak) {
    enum { streakTextBase = 65100, maximumPackedStreak = 99 };
    amigaRendererClearClipRect(renderer);
    /* PF2 is transparent here, so clearing reveals the scrolling PF1 again. */
    amigaRendererFillRect(renderer, 160, 44, 64, 24, 0);
    if (streak == 0)
        return;
    if (streak > maximumPackedStreak)
        streak = maximumPackedStreak;
    drawGameAsset(renderer, 133, 0, downwellPlayfieldX + 65, 55, 0);
    drawGameText(renderer, (UWORD)(streakTextBase + streak),
                 downwellPlayfieldX + 81, 51);
}

static UWORD terrainFrame(char symbol) {
    if (symbol == 'B' || symbol == '3')
        return 16;
    if (symbol == '-')
        return 23;
    return 0;
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
                  downwellPlayfieldX + 328 - viewX,
                  scenes.menuTitleY() - viewY, 0);
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
    {
        const int enterTop = scenes.styleDitherY() - 400;
        amigaRendererFillRect(renderer, downwellPlayfieldX, enterTop,
                              downwellPlayfieldWidth, 800, 0);
        drawGameAsset(renderer, 65004, 0,
                      downwellPlayfieldX, enterTop, 0);
    }
    if (scenes.styleSelected()) {
        const int verticalEdge = scenes.styleVerticalDitherY();
        amigaRendererFillRect(renderer, downwellPlayfieldX,
                              verticalEdge - 720,
                              downwellPlayfieldWidth, 800, 0);
        drawGameAsset(renderer, 65005, 0,
                      downwellPlayfieldX, verticalEdge, 0);
    }
    amigaRendererClearClipRect(renderer);
}

static void drawMainWorldLine(
    AmigaRenderer* renderer,
    const downwell::RoomPack& rooms,
    const downwell::PackedRoom& room,
    const downwell::BuilderPatternPack& builders,
    const AmigaTerrainFragment* fragments,
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
        const AmigaTerrainFragment& fragment = fragments[index];
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
                    if (amigaIsLandingTerrainSymbol(symbol))
                        drawGameAsset(
                            renderer, 51, terrainFrame(symbol),
                            downwellPlayfieldX + worldX - viewX,
                            screenY + centerY - worldY, 0);
                }
            }
        }
        if (fragment.sideRoom && fragment.pattern.sourceIndex == 3) {
            for (UWORD specialRow = 0;
                 specialRow < fragment.pattern.rowCount; ++specialRow) {
                ULONG specialWidth = 0;
                if (!builders.rowWidth(fragment.pattern, specialRow,
                                       specialWidth))
                    break;
                for (unsigned specialColumn = 0;
                     specialColumn < specialWidth; ++specialColumn) {
                    char symbol = 0;
                    if (!builders.cell(fragment.pattern,
                                       (UWORD)specialColumn,
                                       specialRow, symbol))
                        break;
                    if (symbol != '@')
                        continue;
                    const int ordinal = (int)specialColumn + 1;
                    const int rewardX = fragment.reversed
                                            ? 320 - ordinal * 16
                                            : 160 + ordinal * 16;
                    const int rewardY = fragment.y +
                        (int)specialRow * 16;
                    if (worldY >= rewardY - 16 && worldY < rewardY + 16)
                        drawGameAsset(
                            renderer, 569, 0,
                            downwellPlayfieldX + rewardX - viewX,
                            screenY + rewardY - worldY, 0);
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
                                 const AmigaTerrainFragment* fragments,
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
    unsigned liveFrameCount = 0;
    unsigned menuSpriteFrameCount = 0;
#ifdef DOWNWELL_SCENE_VERIFY
    bool sceneGameplayLandedReported = false;
#endif
    worldViewY += port.cameraOffsetY();
#endif

    if (!platform.available())
        return 1;
    UWORD* startupCopperWords =
        (UWORD*)AllocMem(4 * sizeof(UWORD), MEMF_CHIP | MEMF_CLEAR);
    if (!startupCopperWords)
        return 1;
    CopperList startupCopper(startupCopperWords, 4);
    if (!startupCopper.move(color00, 0x000) ||
        !startupCopper.finish() ||
        !platform.takeover(startupCopper,
                           displayHStart, displayHStop,
                           displayVStart, displayVStop,
                           fetchStart, fetchStop)) {
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
        return 1;
    }
    const ULONG gamePackBytes =
        (ULONG)(amiga_game_pack_end - amiga_game_pack);
    if (!memory.allocate(downwellGameRingBytes, downwellLayerBytes,
                         copperWords * 2 * sizeof(UWORD), playerSpriteBytes,
                         bulletSpriteBytes * bulletSpriteBufferCount,
                         enemySpriteBytes,
                         gamePackBytes)) {
        platform.restore();
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
        return 1;
    }
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
    AmigaTerrainFragment fragments[amigaInitialTerrainFragmentCount];
    static AmigaTerrainEnemy generatedEnemies[AmigaPortRuntime::enemyCapacity];
    unsigned generatedEnemyCount = 0;
    downwell::GameMakerRandom enemyRandom(2);
    downwell::PackedRoom mainRoom;
    if (!rooms.valid() || !objects.valid() || !builders.valid() ||
#ifdef DOWNWELL_REPLAY_VERIFY
        !verificationReplay.valid() ||
#endif
        !rooms.findRoom(11, mainRoom) ||
        !buildMenuDrawList(rooms, objects, menuDrawList) ||
        !amigaBuildInitialAreaOneTerrain(
            builders, fragments, amigaInitialTerrainFragmentCount,
            generatedEnemies, AmigaPortRuntime::enemyCapacity,
            generatedEnemyCount, &enemyRandom)) {
        platform.restore();
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
        memory.release();
        return 1;
    }
#ifndef DOWNWELL_REPLAY_VERIFY
    {
        static int breakableX[amigaTerrainBreakableCapacity];
        static int breakableY[amigaTerrainBreakableCapacity];
        const unsigned breakableCount = amigaCollectTerrainBreakables(
            builders, fragments, amigaInitialTerrainFragmentCount,
            breakableX, breakableY, amigaTerrainBreakableCapacity);
        port.configureBreakables(breakableX, breakableY, breakableCount);
        terrainCollision.builders = &builders;
        terrainCollision.rooms = &rooms;
        terrainCollision.room = &mainRoom;
        terrainCollision.fragments = fragments;
        terrainCollision.fragmentCount = amigaInitialTerrainFragmentCount;
        terrainCollision.runtime = &port;
        port.configureLandingResolver(amigaResolveTerrainLanding,
                                      &terrainCollision);
        port.configureSolidResolver(amigaResolveTerrainSolid,
                                    &terrainCollision);
        port.configureEnemyRandom(enemyRandom);
        for (unsigned enemyIndex = 0;
             enemyIndex < generatedEnemyCount; ++enemyIndex) {
            const AmigaTerrainEnemy& enemy = generatedEnemies[enemyIndex];
            downwell::PackedObject definition;
            if (!objects.findObject(enemy.objectId, definition) ||
                !port.configureEnemy(
                    enemyIndex, enemy.objectId, definition.spriteId,
                    enemy.x, enemy.y, enemy.speedX, enemy.speedY,
                    amigaEnemyHealth(enemy.objectId),
                    amigaEnemyReward(enemy.objectId),
                    enemy.motionState, enemy.auxiliaryState,
                    enemy.alarm0, enemy.alarm1,
                    enemy.direction, enemy.moveSpeed,
                    enemy.objectId != 116)) {
                platform.restore();
                FreeMem(startupCopperWords, 4 * sizeof(UWORD));
                memory.release();
                return 1;
            }
        }
    }
#endif
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
    amigaRendererSetGameOrigin(&renderer, port.ringOrigin());
    amigaRendererEnableBlitter(&renderer);
    if (!amigaRendererBuildHardwareSprite(
            activeGamePack, activeGamePackEnd, 31, 0,
            downwellPlayfieldX + 80,
#ifdef DOWNWELL_REPLAY_VERIFY
            gameplayPlayerGroundY,
#else
            gameplayPlayerStartY - port.cameraOffsetY(),
#endif
            0,
            memory.playerSprite(0), playerSpriteHeight) ||
        !amigaRendererBuildHardwareSprite(
            activeGamePack, activeGamePackEnd, 37, 0,
            downwellPlayfieldX + 80,
#ifdef DOWNWELL_REPLAY_VERIFY
            gameplayPlayerGroundY,
#else
            gameplayPlayerStartY - port.cameraOffsetY(),
#endif
            0,
            memory.playerSprite(1), playerSpriteHeight)) {
        platform.restore();
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
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
                     port.ringOrigin())) {
        platform.restore();
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
        memory.release();
        return 1;
    }
    platform.publish(copperLists[0]);
    platform.waitVblank();
    FreeMem(startupCopperWords, 4 * sizeof(UWORD));
    if (!buildCopper(copperLists[1], gameBitmap, hudBitmap,
#ifdef DOWNWELL_REPLAY_VERIFY
                     playerSprites[1],
                     bulletSprites + AmigaPortRuntime::bulletCapacity,
                     enemySprites[1], palette,
#else
                     nullSprite,
                     bulletSprites + AmigaPortRuntime::bulletCapacity,
                     enemySprites[1], palette,
#endif
                     port.ringOrigin())) {
        platform.restore();
        memory.release();
        return 1;
    }
#ifdef DOWNWELL_REPLAY_VERIFY
    buildReferencePicture(&renderer, rooms, builders, fragments[0].pattern,
                          mainInitialViewY);
#else
    drawSplash(&renderer, scenes.splashStage());
    downwellStartupReady();
#endif
    while ((*ciaaPra & 0x40u) == 0) platform.waitVblank();
    while ((*ciaaPra & 0x40u) != 0) {
        UBYTE packedInput;
        int scrollDelta = 0;
        bool cameraXChanged = false;
        unsigned destroyedBlockCount = 0;
        int destroyedBlockX[AmigaPortRuntime::bulletCapacity] = {0};
        int destroyedBlockY[AmigaPortRuntime::bulletCapacity] = {0};
        platform.waitVblank();
#ifndef DOWNWELL_REPLAY_VERIFY
        if (++liveFrameCount == 64)
            downwellFrameLoopReady();
#endif
        {
#ifdef DOWNWELL_REPLAY_VERIFY
            downwell::Input replayInput;
            if (!verificationReplay.frame(port.tick(), replayInput))
                break;
            packedInput = downwell::packReplayInput(replayInput);
#else
#ifdef DOWNWELL_SCENE_VERIFY
            const downwell::Input liveInput = sceneVerificationInput(scenes);
#else
            const downwell::Input liveInput = AmigaInput::sample();
#endif
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
                    buildReferencePicture(&renderer, rooms, builders,
                                          fragments[0].pattern,
                                          worldViewY);
                if (presentedScene == AmigaScene::menu) {
                    bool menuFrameReady = true;
                    if (scenes.menuPlayerVisible()) {
                        menuFrameReady = amigaRendererBuildHardwareSprite(
                            activeGamePack, activeGamePackEnd,
                            (UWORD)scenes.menuPlayerSprite(),
                            (UWORD)scenes.menuPlayerFrame(),
                            downwellPlayfieldX + scenes.menuPlayerX() -
                                (scenes.menuCameraX() - 80),
                            scenes.menuPlayerY() -
                                (scenes.menuCameraY() - 142),
                            scenes.menuPlayerFacing() < 0,
                            memory.playerSprite(back), playerSpriteHeight);
                    } else {
                        playerSprites[back].hide();
                    }
                    if (menuFrameReady)
                        menuFrameReady = buildCopper(
                            copperLists[back], gameBitmap, hudBitmap,
                            playerSprites[back],
                            bulletSprites + back *
                                AmigaPortRuntime::bulletCapacity,
                            enemySprites[back], palette,
                            port.ringOrigin());
                    if (!menuFrameReady)
                        break;
                    platform.publish(copperLists[back]);
                    front = back;
                    if (++menuSpriteFrameCount == 2)
                        downwellMenuSpriteReady();
                    continue;
                }
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
                /*
                 * GameMaker creates the rmMain player on the transition
                 * frame, but its first movement Step runs on the following
                 * frame. Keep the native physics clock on that boundary.
                 */
                if (presentationChanged &&
                    presentedScene == AmigaScene::gameplay) {
#ifdef DOWNWELL_SCENE_VERIFY
                    downwellSceneGameplayReady();
#endif
                    continue;
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
            if (result.streakChanged)
                redrawStreak(&renderer, port.gemStreak());
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
                        fragments, amigaInitialTerrainFragmentCount,
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
                        fragments, amigaInitialTerrainFragmentCount, &port,
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
                            fragments, amigaInitialTerrainFragmentCount,
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
            unsigned actorCount = 0;
            Sprite* actorSprites = bulletSprites +
                back * AmigaPortRuntime::bulletCapacity;
            for (unsigned bullet = 0;
                 bullet < AmigaPortRuntime::bulletCapacity; ++bullet) {
                const AmigaPortRuntime::BulletState& state =
                    port.bulletAt(bullet);
                if (state.active) {
                    Sprite& sprite = actorSprites[actorCount++];
                    if (!amigaRendererBuildHardwareSprite(
                        activeGamePack, activeGamePackEnd, 459, 0,
                        downwellPlayfieldX + 80 + state.x -
                            port.cameraOffsetX(),
                        gameplayPlayerStartY + state.y -
                            port.cameraOffsetY(),
                        0, sprite.data(), bulletSpriteHeight))
                        frameReady = false;
                }
            }
#ifndef DOWNWELL_REPLAY_VERIFY
            for (unsigned enemyIndex = 0;
                 enemyIndex < port.enemyCount() &&
                 actorCount <= AmigaPortRuntime::bulletCapacity;
                 ++enemyIndex) {
                const AmigaPortRuntime::EnemyState& enemy =
                    port.enemyAt(enemyIndex);
                const int screenY = gameplayPlayerStartY + enemy.y -
                    port.cameraOffsetY();
                if (!enemy.active || !enemy.awake || screenY < 16 ||
                    screenY >= downwellDisplayHeight)
                    continue;
                Sprite& sprite = actorCount < AmigaPortRuntime::bulletCapacity
                                     ? actorSprites[actorCount]
                                     : enemySprites[back];
                if (!amigaRendererBuildHardwareSprite(
                        activeGamePack, activeGamePackEnd,
                        (UWORD)enemy.spriteId,
                        (UWORD)enemy.imageFrame,
                        downwellPlayfieldX + 80 + enemy.x -
                            port.cameraOffsetX(),
                        screenY, enemy.facing < 0,
                        sprite.data(), enemySpriteHeight))
                    frameReady = false;
                ++actorCount;
            }
            for (unsigned gemIndex = 0;
                 gemIndex < AmigaPortRuntime::gemCapacity &&
                 actorCount <= AmigaPortRuntime::bulletCapacity;
                 ++gemIndex) {
                const AmigaPortRuntime::GemState& gem =
                    port.gemAt(gemIndex);
                const int screenY = gameplayPlayerStartY + gem.y -
                    port.cameraOffsetY();
                if (!gem.active || screenY < 16 ||
                    screenY >= downwellDisplayHeight)
                    continue;
                Sprite& sprite = actorCount < AmigaPortRuntime::bulletCapacity
                                     ? actorSprites[actorCount]
                                     : enemySprites[back];
                if (!amigaRendererBuildHardwareSprite(
                        activeGamePack, activeGamePackEnd,
                        (UWORD)(gem.large ? 134 : 133),
                        (UWORD)((gem.age / 2u) & 3u),
                        downwellPlayfieldX + 80 + gem.x -
                            port.cameraOffsetX(),
                        screenY, 0, sprite.data(), enemySpriteHeight))
                    frameReady = false;
                ++actorCount;
            }
#endif
            while (actorCount < AmigaPortRuntime::bulletCapacity)
                actorSprites[actorCount++].hide();
            if (actorCount == AmigaPortRuntime::bulletCapacity)
                enemySprites[back].hide();
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
#ifdef DOWNWELL_SCENE_VERIFY
            if (!sceneGameplayLandedReported && port.player().grounded()) {
                sceneGameplayLandedReported = true;
                downwellSceneGameplayLanded();
            }
#endif
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
