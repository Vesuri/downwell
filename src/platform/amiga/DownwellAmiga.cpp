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
    copperWords = 128,
    playerSpriteHeight = 32,
    playerSpriteBytes = (playerSpriteHeight + 2) * 4,
    bulletSpriteHeight = 8,
    bulletSpriteBytes = (bulletSpriteHeight + 2) * 4,
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
    gameplayPlayerStartY = 142,
    gameplayPlayerGroundY = 200
};

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

static const uint16_t downwellPaletteData[20] = {
    0x000, 0xfff, 0xf00, 0x000,
    0x000, 0x000, 0x000, 0x000,
    0x000, 0xfff, 0xf00, 0x000,
    0x000, 0x000, 0x000, 0x000,
    0x000, 0xfff, 0xf00, 0x000
};

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
    amigaRendererDrawAsset(renderer, amiga_game_pack, amiga_game_pack_end,
                           sprite, frame, anchorX, anchorY, flipX);
}

static void drawGameText(AmigaRenderer* renderer, UWORD text,
                         int x, int y) {
    amigaRendererDrawAssetPen(renderer, amiga_game_pack, amiga_game_pack_end,
                              text, 0, x + 1, y + 1, 0, 2);
    amigaRendererDrawAssetPen(renderer, amiga_game_pack, amiga_game_pack_end,
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

static bool isTerrainSymbol(char symbol) {
    return symbol == '1' || symbol == '2' || symbol == '3' ||
           symbol == 'B' || symbol == '-' || symbol == '|';
}

static void drawEntrancePattern(AmigaRenderer* renderer,
                                const downwell::BuilderPatternPack& builders,
                                const downwell::PackedBuilderPattern& pattern) {
    const int baseY = downwellDisplayHeight - pattern.rowCount * 16;
    unsigned row;
    for (row = 0; row < pattern.rowCount; ++row) {
        ULONG width = 0;
        unsigned column;
        if (!builders.rowWidth(pattern, (UWORD)row, width))
            return;
        for (column = 0; column < width; ++column) {
            char symbol = 0;
            if (!builders.cell(pattern, (UWORD)column, (UWORD)row, symbol))
                return;
            if (isTerrainSymbol(symbol))
                drawGameAsset(renderer, 51, 0,
                              downwellPlayfieldX + 8 + column * 16,
                              baseY + row * 16, 0);
            if (symbol == 'd')
                drawGameAsset(renderer, 166, 0,
                              downwellPlayfieldX + 8 + column * 16,
                              baseY + row * 16, 0);
        }
    }
}

static void buildReferencePicture(
    AmigaRenderer* renderer,
    const downwell::BuilderPatternPack& builders,
    const downwell::PackedBuilderPattern& entrance) {

    amigaRendererClear(renderer, 0);
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, 0,
                            downwellPlayfieldWidth, downwellDisplayHeight);
    drawEntrancePattern(renderer, builders, entrance);

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

static void drawMenu(AmigaRenderer* renderer,
                     const downwell::RoomPack& rooms,
                     const downwell::ObjectPack& objects,
                     const AmigaSceneRuntime& scenes) {
    downwell::PackedRoom room;
    UWORD order[256];
    unsigned count = 0;
    unsigned index;
    const int viewX = scenes.menuCameraX() - 80;
    const int viewY = scenes.menuCameraY() - 142;
    if (!rooms.findRoom(20, room))
        return;

    for (index = 0; index < room.instanceCount && count < 256; ++index) {
        downwell::PackedRoomInstance instance;
        downwell::PackedObject object;
        if (rooms.roomInstance(room, index, instance) &&
            objects.findObject(instance.objectId, object) &&
            object.visible() && object.spriteId >= 0 &&
            instance.objectId != 35)
            order[count++] = (UWORD)index;
    }
    for (index = 1; index < count; ++index) {
        const UWORD value = order[index];
        downwell::PackedRoomInstance valueInstance;
        downwell::PackedObject valueObject;
        unsigned destination = index;
        rooms.roomInstance(room, value, valueInstance);
        objects.findObject(valueInstance.objectId, valueObject);
        while (destination > 0) {
            downwell::PackedRoomInstance priorInstance;
            downwell::PackedObject priorObject;
            rooms.roomInstance(room, order[destination - 1], priorInstance);
            objects.findObject(priorInstance.objectId, priorObject);
            if (priorObject.depth >= valueObject.depth)
                break;
            order[destination] = order[destination - 1];
            --destination;
        }
        order[destination] = value;
    }

    amigaRendererClear(renderer, 0);
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, 0,
                            downwellPlayfieldWidth, downwellDisplayHeight);
    for (index = 0; index < count; ++index) {
        downwell::PackedRoomInstance instance;
        downwell::PackedObject object;
        int roomX;
        int roomY;
        int sprite;
        if (!rooms.roomInstance(room, order[index], instance) ||
            !objects.findObject(instance.objectId, object))
            continue;
        roomX = instance.x;
        roomY = instance.y;
        if (instance.objectId == 312) {
            roomX = 120 + viewX * 5 / 6;
            roomY = (viewY + 142) * 10 / 11;
        }
        sprite = instance.objectId == 60 ? 76 : object.spriteId;
        if (instance.objectId == 315)
            amigaRendererFillRect(renderer,
                downwellPlayfieldX + roomX - 24 - viewX,
                roomY + 64 - viewY, 160, 1000, 0);
        drawGameAsset(renderer, (UWORD)sprite, 0,
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
                renderer, amiga_game_pack, amiga_game_pack_end,
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

static void drawRevealedGameLine(AmigaRenderer* renderer,
                                 const downwell::BuilderPatternPack& builders,
                                 const downwell::PackedBuilderPattern& pattern,
                                 unsigned ringOrigin,
                                 unsigned worldY) {
    const int y = downwellDisplayHeight - 1;
    const unsigned patternY = worldY % (pattern.rowCount * 16u);
    const UWORD row = (UWORD)(patternY >> 4);
    const unsigned line = patternY & 15u;
    ULONG width = 0;
    unsigned column;
    amigaRendererSetGameOrigin(renderer, ringOrigin);
    amigaRendererClearGameRows(renderer, y, 1, 0);
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, 0,
                            downwellPlayfieldWidth, downwellDisplayHeight);

    /* The side walls are permanent world geometry, revealed a line at a time. */
    amigaRendererFillRect(renderer, downwellPlayfieldX, y, 2, 1, 1);
    amigaRendererFillRect(renderer,
                          downwellPlayfieldX + downwellPlayfieldWidth - 2,
                          y, 2, 1, 1);

    if (builders.rowWidth(pattern, row, width)) {
        for (column = 0; column < width; ++column) {
            char symbol = 0;
            if (!builders.cell(pattern, (UWORD)column, row, symbol))
                break;
            if (isTerrainSymbol(symbol))
                amigaRendererFillRect(renderer,
                    downwellPlayfieldX + 8 + column * 16,
                    y, 16, 1, 1);
            if (symbol == '_' && line < 2u)
                amigaRendererFillRect(renderer,
                    downwellPlayfieldX + 8 + column * 16,
                    y, 16, 1, 1);
        }
    }
    amigaRendererClearClipRect(renderer);
}

static bool buildCopper(CopperList& list,
                        const Bitmap& game, const Bitmap& hud,
                        const Sprite& playerSprite,
                        const Sprite& bulletSprite,
                        const Sprite& nullSprite,
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
    list.showSprite(1, bulletSprite);
    for (channel = 2; channel < 8; ++channel)
        list.showSprite((UWORD)channel, nullSprite);

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

static void offsetHardwareSprite(UWORD* sprite, int deltaX, int deltaY) {
    UBYTE* control = (UBYTE*)sprite;
    int startX = ((int)control[1] << 1) | (control[3] & 1);
    int startY = control[0] | ((control[3] & 4) << 6);
    int stopY = control[2] | ((control[3] & 2) << 7);
    startX += deltaX;
    startY += deltaY;
    stopY += deltaY;
    control[0] = (UBYTE)startY;
    control[1] = (UBYTE)(startX >> 1);
    control[2] = (UBYTE)stopY;
    control[3] = (UBYTE)((startX & 1) |
                         (stopY > 255 ? 2 : 0) |
                         (startY > 255 ? 4 : 0));
}

int main(void) {
    AmigaPlatform platform;
    AmigaMemory memory;
    AmigaRenderer renderer;
    unsigned front = 0;
    unsigned worldBottom;
#ifdef DOWNWELL_REPLAY_VERIFY
    AmigaPortRuntime port(initialRingOrigin);
#else
    AmigaPortRuntime port(initialRingOrigin, false,
                          gameplayPlayerGroundY - gameplayPlayerStartY);
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
    if (!rooms.valid() || !objects.valid() || !builders.valid() ||
#ifdef DOWNWELL_REPLAY_VERIFY
        !verificationReplay.valid() ||
#endif
        !builders.findPattern(downwell::BuilderPatternFamily::areaOneStart,
                              0, entrance)) {
        return 1;
    }
    worldBottom = entrance.rowCount * 16u - 1u;

    if (!memory.allocate(downwellGameRingBytes, downwellLayerBytes,
                         copperWords * 2 * sizeof(UWORD), playerSpriteBytes,
                         bulletSpriteBytes)) {
        return 1;
    }

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
    buildReferencePicture(&renderer, builders, entrance);
#else
    drawSplash(&renderer, scenes.splashStage());
#endif
    if (!amigaRendererBuildHardwareSprite(
            amiga_game_pack, amiga_game_pack_end, 31, 0,
            downwellPlayfieldX + 80,
#ifdef DOWNWELL_REPLAY_VERIFY
            gameplayPlayerGroundY,
#else
            gameplayPlayerStartY,
#endif
            memory.playerSprite(0), playerSpriteHeight) ||
        !amigaRendererBuildHardwareSprite(
            amiga_game_pack, amiga_game_pack_end, 37, 0,
            downwellPlayfieldX + 88,
#ifdef DOWNWELL_REPLAY_VERIFY
            gameplayPlayerGroundY - 8,
#else
            gameplayPlayerStartY - 8,
#endif
            memory.playerSprite(1), playerSpriteHeight)) {
        memory.release();
        return 1;
    }
    Sprite playerSprites[2] = {
        Sprite(memory.playerSprite(0), playerSpriteHeight),
        Sprite(memory.playerSprite(1), playerSpriteHeight)
    };
    Sprite bulletSprite(memory.bulletSprite(), bulletSpriteHeight);
    Sprite nullSprite(memory.nullSprite(), 0);
    bulletSprite.hide();
    if (!buildCopper(copperLists[0], gameBitmap, hudBitmap,
#ifdef DOWNWELL_REPLAY_VERIFY
                     playerSprites[0], bulletSprite, nullSprite, palette,
#else
                     nullSprite, bulletSprite, nullSprite, palette,
#endif
                     initialRingOrigin) ||
        !buildCopper(copperLists[1], gameBitmap, hudBitmap,
#ifdef DOWNWELL_REPLAY_VERIFY
                     playerSprites[1], bulletSprite, nullSprite, palette,
#else
                     nullSprite, bulletSprite, nullSprite, palette,
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
    while ((*ciaaPra & 0x40u) == 0) platform.waitVblank();
    while ((*ciaaPra & 0x40u) != 0) {
        UBYTE packedInput;
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
                else if (presentedScene == AmigaScene::menu)
                    drawMenu(&renderer, rooms, objects, scenes);
                else if (presentedScene == AmigaScene::styleSelect)
                    drawStyleSelect(&renderer, scenes);
                else if (presentationChanged)
                    buildReferencePicture(&renderer, builders, entrance);
                if (presentationChanged) {
                    if (!buildCopper(copperLists[back], gameBitmap, hudBitmap,
                                     presentedScene == AmigaScene::gameplay
                                         ? playerSprites[0] : nullSprite,
                                     bulletSprite, nullSprite, palette,
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
            const AmigaPlayerMotion motion = result.playerMotion;
            if (motion.deltaX || motion.deltaY) {
                offsetHardwareSprite(memory.playerSprite(0),
                                     motion.deltaX, motion.deltaY);
                offsetHardwareSprite(memory.playerSprite(1),
                                     motion.deltaX, motion.deltaY);
            }
            if (result.bulletChanged) {
                if (port.bulletActive())
                    buildBulletSprite(bulletSprite,
                        downwellPlayfieldX + 80 + port.bulletX(),
                        gameplayPlayerStartY + port.bulletY());
                else
                    bulletSprite.hide();
            }
            if (result.ammoChanged)
                redrawAmmo(&renderer, (unsigned)port.player().ammo());
        }
        if ((port.tick() & 1u) == 0) {
            const unsigned back = front ^ 1u;
            ++worldBottom;
            drawRevealedGameLine(&renderer, builders, entrance,
                                 port.ringOrigin(), worldBottom);
            if (!buildCopper(copperLists[back], gameBitmap, hudBitmap,
                             playerSprites[port.player().grounded() ? 0u : 1u],
                             bulletSprite, nullSprite, palette,
                             port.ringOrigin()))
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
