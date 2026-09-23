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
#include "AmigaProfile.h"
#include "DownwellCopperLists.h"
#include "AmigaMenu.h"
#include "AmigaMenuCache.h"
#include "AmigaMenuScroll.h"
#include "AmigaPlayer.h"
#include "AmigaPortRuntime.h"
#include "AmigaRenderer.h"
#include "AmigaSceneRuntime.h"
#include "AmigaSplashCache.h"
#include "AmigaStyleRenderer.h"
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
    dynamicCopper0 = 0,
    dynamicCopper1 = 1,
    copperListCount = 6,
    playerSpriteHeight = 32,
    playerSpriteBytes = (playerSpriteHeight + 2) * 4,
    /* Channels 1..6 are shared by bullets, enemies and gems each frame. */
    bulletSpriteHeight = 32,
    bulletSpriteBytes = (bulletSpriteHeight + 2) * 4,
    bulletSpriteBufferCount = 2 * AmigaPortRuntime::bulletCapacity,
    enemySpriteHeight = 32,
    enemySpriteBytes = (enemySpriteHeight + 2) * 4,
    menuPlaneBytes = amigaMenuCacheBytesPerRow * amigaMenuCacheHeight,
    initialRingOrigin = 300,
    mainViewX = 160,
    mainInitialViewY = 34,
    mainBuilderStartY = 1168,
    gameplayPlayerStartY = 142,
    gameplayPlayerGroundY = 200,
    gameplayPrefetchLines = 16,
    gameplayPrefetchScreenY =
        downwellDisplayHeight - gameplayPrefetchLines
};

enum {
    styleCacheBytes = amigaStyleCacheBytes,
    sceneCacheBytes = amigaSplashCacheBytes > styleCacheBytes
                          ? amigaSplashCacheBytes : styleCacheBytes
};

alignas(AmigaPortRuntime)
static UBYTE portRuntimeStorage[sizeof(AmigaPortRuntime)];

static AmigaTerrainCollisionContext terrainCollision;

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
    downwellCopperWords >=
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

extern "C" __attribute__((noinline, used)) void
downwellMenuTitleSpriteReady(void) {
    __asm__ volatile("" ::: "memory");
}

extern "C" __attribute__((noinline, used)) void
downwellMenuEntryScrollReady(void) {
    __asm__ volatile("" ::: "memory");
}

extern "C" __attribute__((noinline, used)) void downwellMenuSkyCovered(void) {
    __asm__ volatile("" ::: "memory");
}

extern "C" {
volatile const UBYTE* gDownwellStyleFrame;
volatile const UBYTE* gDownwellLockedStyleFrame;
volatile const UBYTE* gDownwellSelectedStyleFrame;
volatile const UBYTE* gDownwellStyleMask;
volatile const UWORD* gDownwellStyleCopper;
volatile ULONG gDownwellStyleReferenceFailures;
volatile ULONG gDownwellStyleReferenceFrames;
volatile const UBYTE* gDownwellSplashFrame;
volatile const UBYTE* gDownwellControlsFrame;
volatile const UWORD* gDownwellControlsCopper;
volatile const UBYTE* gDownwellCreditsFrame;
volatile const UWORD* gDownwellCreditsCopper;
#ifdef DOWNWELL_SCENE_VERIFY
volatile ULONG gDownwellSplashCheckedFrames[7];
volatile ULONG gDownwellSplashPublicationFailures;
volatile const UWORD* gDownwellMenuSpriteOracle;
volatile ULONG gDownwellMenuCopperPatchFailures;
#endif

struct DownwellMenuFrameCapture {
    ULONG foregroundOffset;
    ULONG backgroundOffset;
    UWORD scrollControl;
    UWORD titlePartCount;
};

volatile DownwellMenuFrameCapture gDownwellMenuFrameCapture;
volatile const UBYTE* gDownwellMenuForeground;
volatile const UBYTE* gDownwellMenuBackground;
volatile const UWORD* gDownwellMenuPlayerSprite;
volatile const UBYTE* gDownwellMenuEntryFrame;
volatile UWORD gDownwellMenuEntryTitlePartCount;
volatile const UBYTE* gDownwellGameplayGame;
volatile const UBYTE* gDownwellGameplayHud;
volatile const UWORD* gDownwellGameplayPlayerSprite;
volatile const UWORD* gDownwellGameplayActorSprites;
volatile const UWORD* gDownwellGameplayOverflowSprite;
volatile UWORD gDownwellGameplayRingOrigin;

__attribute__((noinline, used)) void downwellStyleFrameReady(void) {
    __asm__ volatile("" ::: "memory");
}

__attribute__((noinline, used)) void downwellStyleRevealFrameReady(void) {
    __asm__ volatile("" ::: "memory");
}

__attribute__((noinline, used)) void downwellLockedStyleFrameReady(void) {
    __asm__ volatile("" ::: "memory");
}

__attribute__((noinline, used)) void downwellSelectedStyleFrameReady(void) {
    __asm__ volatile("" ::: "memory");
}

__attribute__((noinline, used)) void downwellSplashFrameReady(void) {
    __asm__ volatile("" ::: "memory");
}

__attribute__((noinline, used)) void downwellControlsFrameReady(void) {
    __asm__ volatile("" ::: "memory");
}

__attribute__((noinline, used)) void downwellCreditsFrameReady(void) {
    __asm__ volatile("" ::: "memory");
}

__attribute__((noinline, used)) void downwellMenuWakeFrameReady(void) {
    __asm__ volatile("" ::: "memory");
}

__attribute__((noinline, used)) void downwellMenuTitleFrameReady(void) {
    __asm__ volatile("" ::: "memory");
}

__attribute__((noinline, used)) void downwellMenuEntryFrameReady(void) {
    __asm__ volatile("" ::: "memory");
}

__attribute__((noinline, used)) void downwellGameplayFrameReady(void) {
    __asm__ volatile("" ::: "memory");
}

__attribute__((noinline, used)) void downwellGameplayActiveFrameReady(void) {
    __asm__ volatile("" ::: "memory");
}
}

static_assert(sizeof(DownwellMenuFrameCapture) == 12,
              "menu capture has a stable debugger wire format");

extern "C" __attribute__((noinline, used)) void downwellSceneGameplayReady(void) {
    __asm__ volatile("" ::: "memory");
}

extern "C" __attribute__((noinline, used)) void downwellSceneGameplayLanded(void) {
    __asm__ volatile("" ::: "memory");
}

extern "C" __attribute__((noinline, used)) void
downwellSceneGameplayActive(void) {
    __asm__ volatile("" ::: "memory");
}
#endif

#ifdef DOWNWELL_SCENE_VERIFY
static downwell::Input sceneVerificationInput(
    const AmigaSceneRuntime& scenes, bool styleFrameReported,
    bool lockedStyleFrameReported,
    bool gameplayLanded, const AmigaPortRuntime& port) {
    downwell::Input input;
    if (scenes.scene() == AmigaScene::menu)
        input.right = true;
    else if (scenes.scene() == AmigaScene::styleSelect &&
             scenes.styleYOffset() == 0 && styleFrameReported &&
             !scenes.styleSelected()) {
        if (!lockedStyleFrameReported)
            input.right = scenes.styleCursor() == 0;
        else if (scenes.styleCursor() != 0)
            input.left = true;
        else
            input.fire = true;
    }
    else if (scenes.scene() == AmigaScene::gameplay && gameplayLanded) {
        const unsigned movementPhase = (port.tick() / 120u) & 1u;
        input.right = movementPhase == 0;
        input.left = movementPhase != 0;
        const unsigned firePhase = port.tick() % 16u;
        input.fire = firePhase == 0 || firePhase == 3;
    }
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

static void drawMainWorldLine(
    AmigaRenderer* renderer,
    const downwell::RoomPack& rooms,
    const downwell::PackedRoom& room,
    const downwell::BuilderPatternPack& builders,
    const AmigaTerrainFragment* fragments,
    unsigned fragmentCount,
    const AmigaTerrainDecoration* decorations,
    unsigned decorationCount,
    const AmigaPortRuntime* runtime,
    int viewX, int worldY, int screenY);

static void drawMainWorldSpan(
    AmigaRenderer* renderer,
    const downwell::RoomPack& rooms,
    const downwell::PackedRoom& room,
    const downwell::BuilderPatternPack& builders,
    const AmigaTerrainFragment* fragments,
    unsigned fragmentCount,
    const AmigaTerrainDecoration* decorations,
    unsigned decorationCount,
    const AmigaPortRuntime* runtime,
    int viewX, int viewY, int clipY, int clipHeight) {
    unsigned index;
    const int firstWorldY = viewY + clipY;
    const int finalWorldY = firstWorldY + clipHeight;
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, clipY,
                            downwellPlayfieldWidth, clipHeight);
    for (index = 0; index < room.instanceCount; ++index) {
        downwell::PackedRoomInstance instance;
        if (!rooms.roomInstance(room, index, instance) ||
            instance.objectId != 60 || instance.y + 8 < firstWorldY ||
            instance.y - 8 >= finalWorldY)
            continue;
        drawGameAsset(renderer, 51,
                      mainWallFrame(rooms, room, instance.x, instance.y),
                      downwellPlayfieldX + instance.x - viewX,
                      instance.y - viewY, 0);
    }
    for (int wallY = 704; wallY < mainBuilderStartY; wallY += 16) {
        if (wallY + 8 < firstWorldY || wallY - 8 >= finalWorldY)
            continue;
        drawGameAsset(renderer, 51, 0, downwellPlayfieldX,
                      wallY - viewY, 0);
        drawGameAsset(renderer, 51, 0,
                      downwellPlayfieldX + downwellPlayfieldWidth,
                      wallY - viewY, 0);
    }
    for (index = 0; index < fragmentCount; ++index) {
        const AmigaTerrainFragment& fragment = fragments[index];
        for (UWORD row = 0; row < fragment.pattern.rowCount; ++row) {
            const int rowY = fragment.y + (int)row * 16;
            ULONG width = 0;
            if (rowY + 8 < firstWorldY || rowY - 8 >= finalWorldY)
                continue;
            if (row + 1 < fragment.pattern.rowCount) {
                const int firstX = fragment.sideRoom
                                       ? (fragment.reversed ? 0 : 480)
                                       : (fragment.reversed ? 160 : 320);
                const int secondX = fragment.reversed ? 320 : 160;
                drawGameAsset(renderer, 51, 0,
                              downwellPlayfieldX + firstX - viewX,
                              rowY - viewY, 0);
                drawGameAsset(renderer, 51, 0,
                              downwellPlayfieldX + secondX - viewX,
                              rowY - viewY, 0);
            }
            if (!builders.rowWidth(fragment.pattern, row, width))
                break;
            for (unsigned column = 0; column < width; ++column) {
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
                        worldX - 240, rowY - 176))
                    continue;
                if (amigaIsLandingTerrainSymbol(symbol))
                    drawGameAsset(renderer, 51, terrainFrame(symbol),
                                  downwellPlayfieldX + worldX - viewX,
                                  rowY - viewY, 0);
            }
        }
        if (fragment.appendPillars) {
            const int finalRowY = fragment.y +
                ((int)fragment.pattern.rowCount - 1) * 16;
            const int firstX = fragment.sideRoom
                                   ? (fragment.reversed ? 0 : 480)
                                   : (fragment.reversed ? 160 : 320);
            const int secondX = fragment.reversed ? 320 : 160;
            for (int padding = 0; padding < 3; ++padding) {
                const int rowY = finalRowY + padding * 16;
                if (rowY + 8 < firstWorldY || rowY - 8 >= finalWorldY)
                    continue;
                drawGameAsset(renderer, 51, 0,
                              downwellPlayfieldX + firstX - viewX,
                              rowY - viewY, 0);
                drawGameAsset(renderer, 51, 0,
                              downwellPlayfieldX + secondX - viewX,
                              rowY - viewY, 0);
            }
        }
    }
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
    amigaRendererClearClipRect(renderer);
}

static void drawMainWorld(
    AmigaRenderer* renderer,
    const downwell::RoomPack& rooms,
    const downwell::PackedRoom& room,
    const downwell::BuilderPatternPack& builders,
    const AmigaTerrainFragment* fragments,
    unsigned fragmentCount,
    const AmigaTerrainDecoration* decorations,
    unsigned decorationCount,
    const AmigaPortRuntime* runtime,
    int viewX, int viewY) {
    drawMainWorldSpan(renderer, rooms, room, builders,
                      fragments, fragmentCount,
                      decorations, decorationCount, runtime,
                      viewX, viewY, 0, downwellDisplayHeight);
}

static void buildReferencePicture(
    AmigaRenderer* renderer,
    const downwell::RoomPack& rooms,
    const downwell::PackedRoom& room,
    const downwell::BuilderPatternPack& builders,
    const AmigaTerrainFragment* fragments,
    unsigned fragmentCount,
    const AmigaTerrainDecoration* decorations,
    unsigned decorationCount,
    const AmigaPortRuntime* runtime,
    int viewX, int viewY) {

    amigaRendererClear(renderer, 0);
    drawMainWorld(renderer, rooms, room, builders,
                  fragments, fragmentCount,
                  decorations, decorationCount, runtime,
                  viewX, viewY);

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

static AmigaMenuDrawList menuDrawList;

static void drawMainWorldLine(
    AmigaRenderer* renderer,
    const downwell::RoomPack& rooms,
    const downwell::PackedRoom& room,
    const downwell::BuilderPatternPack& builders,
    const AmigaTerrainFragment* fragments,
    unsigned fragmentCount,
    const AmigaTerrainDecoration* decorations,
    unsigned decorationCount,
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
        break;
    }
    for (index = 0; index < decorationCount; ++index) {
        const AmigaTerrainDecoration& decoration = decorations[index];
        if (worldY < decoration.y - 24 || worldY >= decoration.y + 24)
            continue;
        drawGameAsset(renderer, (UWORD)decoration.spriteId,
                      (UWORD)decoration.frame,
                      downwellPlayfieldX + decoration.x - viewX,
                      screenY + decoration.y - worldY,
                      decoration.flipX ? 1 : 0);
    }
    amigaRendererClearClipRect(renderer);
}

static void drawRevealedGameLine(AmigaRenderer* renderer,
                                 const downwell::RoomPack& rooms,
                                 const downwell::PackedRoom& room,
                                 const downwell::BuilderPatternPack& builders,
                                 const AmigaTerrainFragment* fragments,
                                 unsigned fragmentCount,
                                 const AmigaTerrainDecoration* decorations,
                                 unsigned decorationCount,
                                 const AmigaPortRuntime* runtime,
                                 unsigned ringOrigin,
                                 int viewX, int worldY,
                                 int y) {
    amigaRendererSetGameOrigin(renderer, ringOrigin);
    amigaRendererClearGameRows(renderer, y, 1, 0);
    drawMainWorldLine(renderer, rooms, room, builders,
                      fragments, fragmentCount,
                      decorations, decorationCount, runtime,
                      viewX, worldY, y);
}

static void drawFutureGameBlock(
    AmigaRenderer* renderer,
    const downwell::RoomPack& rooms,
    const downwell::PackedRoom& room,
    const downwell::BuilderPatternPack& builders,
    const AmigaTerrainFragment* fragments,
    unsigned fragmentCount,
    const AmigaTerrainDecoration* decorations,
    unsigned decorationCount,
    const AmigaPortRuntime* runtime,
    unsigned ringOrigin, int viewX, int viewY) {
    const unsigned futureOrigin =
        (ringOrigin + gameplayPrefetchLines) % downwellGameRingHeight;
    const int futureViewY = viewY + gameplayPrefetchLines;
    amigaRendererSetGameOrigin(renderer, futureOrigin);
    amigaRendererClearGameRows(renderer, gameplayPrefetchScreenY,
                               gameplayPrefetchLines, 0);
    drawMainWorldSpan(renderer, rooms, room, builders,
                      fragments, fragmentCount,
                      decorations, decorationCount, runtime,
                      viewX, futureViewY,
                      gameplayPrefetchScreenY, gameplayPrefetchLines);
}

static Sprite& actorSpriteAt(Sprite* regular, Sprite& overflow,
                            unsigned index) {
    return index < AmigaPortRuntime::bulletCapacity
               ? regular[index] : overflow;
}

#ifdef DOWNWELL_SCENE_VERIFY
static void exposeGameplayFrame(const AmigaMemory& memory, unsigned buffer,
                                unsigned ringOrigin) {
    gDownwellGameplayGame = memory.game();
    gDownwellGameplayHud = memory.hud();
    gDownwellGameplayPlayerSprite = memory.playerSprite(buffer);
    gDownwellGameplayActorSprites = memory.bulletSprite() +
        buffer * AmigaPortRuntime::bulletCapacity *
            (bulletSpriteBytes / sizeof(UWORD));
    gDownwellGameplayOverflowSprite = memory.enemySprite(buffer);
    gDownwellGameplayRingOrigin = (UWORD)ringOrigin;
}
#endif

int main(void) {
    AmigaPlatform platform;
    AmigaMemory memory;
    AmigaRenderer renderer;
    unsigned front = 0;
    int worldViewX = mainViewX;
    int worldViewY = mainInitialViewY;
    int gameplayPrefetchRemaining = 0;
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
#ifdef DOWNWELL_SKIP_TO_GAME
    // Force the normal main-room presentation setup on the first frame.
    presentedScene = AmigaScene::splash;
#endif
    int presentedSplash = scenes.splashStage();
    unsigned presentedSplashDither = scenes.splashDitherFrame();
    unsigned liveFrameCount = 0;
    unsigned menuSpriteFrameCount = 0;
    int renderedMenuTitleFrame = -1;
    int menuCopperMode[2] = {-1, -1};
#ifdef DOWNWELL_SCENE_VERIFY
    const UWORD* expectedSplashCopper = 0;
    bool sceneGameplayLandedReported = false;
    bool sceneGameplayActiveReported = false;
    bool sceneMenuTitleReported = false;
    bool sceneMenuEntryReported = false;
    bool sceneMenuWakeFrameReported = false;
    bool sceneStyleFrameReported = false;
    bool sceneLockedStyleFrameReported = false;
    bool sceneSelectedStyleFrameReported = false;
    bool sceneSplashFrameReported = false;
    bool sceneControlsFrameReported = false;
    bool sceneCreditsFrameReported = false;
    int sceneGameplayLandedCameraY = 0;
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
                           downwellDisplayHStartCropped,
                           downwellDisplayHStopCropped,
                           downwellDisplayVStartCropped,
                           downwellDisplayVStopCropped,
                           downwellFetchStart, downwellFetchStop)) {
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
        return 1;
    }
    const ULONG gamePackBytes =
        (ULONG)(amiga_game_pack_end - amiga_game_pack);
    if (!memory.allocate(downwellGameRingBytes, downwellLayerBytes,
                         menuPlaneBytes, amigaMenuEntryPlaneBytes,
                         downwellCopperWords * copperListCount * sizeof(UWORD),
                         playerSpriteBytes,
                         bulletSpriteBytes * bulletSpriteBufferCount,
                         enemySpriteBytes,
                         sceneCacheBytes,
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
    static AmigaTerrainDecoration
        generatedDecorations[amigaTerrainDecorationCapacity];
    unsigned generatedEnemyCount = 0;
    unsigned generatedDecorationCount = 0;
    downwell::GameMakerRandom enemyRandom(2);
    downwell::PackedRoom mainRoom;
    if (!rooms.valid() || !objects.valid() || !builders.valid() ||
#ifdef DOWNWELL_REPLAY_VERIFY
        !verificationReplay.valid() ||
#endif
        !rooms.findRoom(11, mainRoom) ||
        !amigaBuildMenuDrawList(rooms, objects, menuDrawList) ||
        !amigaBuildInitialAreaOneTerrain(
            builders, fragments, amigaInitialTerrainFragmentCount,
            generatedEnemies, AmigaPortRuntime::enemyCapacity,
            generatedEnemyCount, &enemyRandom,
            generatedDecorations, amigaTerrainDecorationCapacity,
            &generatedDecorationCount)) {
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
#ifndef DOWNWELL_REPLAY_VERIFY
    /*
     * Compose the garden completely while the startup display is still
     * blank.  Once presented, these two one-bit planes are immutable: the
     * copper scrolls them solely through pointers and BPLCON1.
     */
    if (!amigaRenderMenuCache(menuDrawList, activeGamePack,
                              activeGamePackEnd,
                              memory.menuForeground(),
                              memory.menuBackground(),
                              memory.menuEntry())) {
        platform.restore();
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
        memory.release();
        return 1;
    }
#endif
    if (!amigaBuildSplashCache(activeGamePack, activeGamePackEnd,
                               memory.sceneCache(), memory.hud())) {
        platform.restore();
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
        memory.release();
        return 1;
    }

#if defined(DOWNWELL_SKIP_INTRO) && !defined(DOWNWELL_SKIP_TO_GAME)
    if (!amigaBuildStyleBorder(activeGamePack, activeGamePackEnd,
                              memory.sceneCache() + amigaStyleFrameBytes * 2)) {
        platform.restore();
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
        memory.release();
        return 1;
    }
#endif
    /*
     * The scene-specific wrappers below deliberately share two alternating
     * Chip RAM programs. Only the wrapper for the active scene writes a slot;
     * this retains Rescue's dedicated-list API without reserving three pairs
     * of mutually exclusive copper storage on a 512 KB machine.
     */
    UWORD* const dynamicCopperWords[2] = {
        memory.copper(), memory.copper() + downwellCopperWords
    };
    DownwellGameplayCopperList gameplayCopperLists[2] = {
        DownwellGameplayCopperList(dynamicCopperWords[0],
                                   downwellCopperWords),
        DownwellGameplayCopperList(dynamicCopperWords[1],
                                   downwellCopperWords)
    };
    DownwellMenuCopperList menuCopperLists[2] = {
        DownwellMenuCopperList(dynamicCopperWords[0], downwellCopperWords),
        DownwellMenuCopperList(dynamicCopperWords[1], downwellCopperWords)
    };
    DownwellSplashCopperList dynamicSplashLists[2] = {
        DownwellSplashCopperList(dynamicCopperWords[0],
                                 downwellCopperWords),
        DownwellSplashCopperList(dynamicCopperWords[1],
                                 downwellCopperWords)
    };
    DownwellStyleCopperList styleLists[2] = {
        DownwellStyleCopperList(dynamicCopperWords[0], downwellCopperWords),
        DownwellStyleCopperList(dynamicCopperWords[1], downwellCopperWords)
    };
    bool styleListBuilt[2] = {false, false};
    AmigaStyleFrameState styleFrameState[2];
    DownwellSplashCopperList splashBlankList(
        memory.copper() + downwellCopperWords * 2, downwellCopperWords);
    DownwellSplashCopperList splashLogoList(
        memory.copper() + downwellCopperWords * 3, downwellCopperWords);
    DownwellSplashCopperList splashCreditsList(
        memory.copper() + downwellCopperWords * 4, downwellCopperWords);
    DownwellSplashCopperList splashControlsList(
        memory.copper() + downwellCopperWords * 5, downwellCopperWords);
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
    if (
#ifdef DOWNWELL_REPLAY_VERIFY
        !gameplayCopperLists[dynamicCopper0].build(
            gameBitmap, hudBitmap, playerSprites[0], bulletSprites,
            AmigaPortRuntime::bulletCapacity, enemySprites[0], palette,
            port.ringOrigin())
#else
        !splashBlankList.buildNarrow(
            amigaSplashFrame(memory.sceneCache(), 1), nullSprite, palette) ||
        !splashLogoList.buildNarrow(
            amigaSplashFrame(memory.sceneCache(), 2), nullSprite, palette) ||
        !splashCreditsList.buildWide(hudBitmap, nullSprite, palette) ||
        !splashControlsList.buildNarrow(
            amigaSplashFrame(memory.sceneCache(), 4), nullSprite, palette)
#endif
    ) {
        platform.restore();
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
        memory.release();
        return 1;
    }
#ifdef DOWNWELL_REPLAY_VERIFY
    platform.publish(gameplayCopperLists[dynamicCopper0]);
#else
    platform.publish(splashBlankList);
#ifdef DOWNWELL_SCENE_VERIFY
    expectedSplashCopper = splashBlankList.data();
#endif
#endif
    platform.waitVblank();
    FreeMem(startupCopperWords, 4 * sizeof(UWORD));
    if (!gameplayCopperLists[dynamicCopper1].build(
        gameBitmap, hudBitmap,
#ifdef DOWNWELL_REPLAY_VERIFY
        playerSprites[1],
#else
        nullSprite,
#endif
        bulletSprites + AmigaPortRuntime::bulletCapacity,
        AmigaPortRuntime::bulletCapacity, enemySprites[1], palette,
        port.ringOrigin())) {
        platform.restore();
        memory.release();
        return 1;
    }
#ifdef DOWNWELL_REPLAY_VERIFY
    buildReferencePicture(
        &renderer, rooms, mainRoom, builders,
        fragments, amigaInitialTerrainFragmentCount,
        generatedDecorations, generatedDecorationCount,
        &port, mainViewX, mainInitialViewY);
#else
    downwellStartupReady();
#endif
    while ((*ciaaPra & 0x40u) == 0) platform.waitVblank();
    while ((*ciaaPra & 0x40u) != 0) {
        DW_PROFILE_FRAME((unsigned)scenes.scene());
        UBYTE packedInput;
        int scrollDelta = 0;
        bool cameraXChanged = false;
        unsigned destroyedBlockCount = 0;
        int destroyedBlockX[AmigaPortRuntime::bulletCapacity] = {0};
        int destroyedBlockY[AmigaPortRuntime::bulletCapacity] = {0};
        DW_PROFILE_PHASE(profileWait);
        platform.waitVblank();
        DW_PROFILE_PHASE(profileInput);
#ifdef DOWNWELL_SCENE_VERIFY
        // Check AFTER the preceding iteration completed, not at its first
        // publish: a later scene branch must not replace the splash program.
        if (presentedScene == AmigaScene::splash) {
            ++gDownwellSplashCheckedFrames[presentedSplash];
            if (platform.publishedCopper() != expectedSplashCopper)
                ++gDownwellSplashPublicationFailures;
        }
#endif
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
            const downwell::Input liveInput = sceneVerificationInput(
                scenes, sceneStyleFrameReported,
                sceneLockedStyleFrameReported,
                sceneGameplayLandedReported, port);
#else
            downwell::Input liveInput = AmigaInput::sample();
#ifdef DOWNWELL_PROFILE_AUTORUN
            static unsigned profileMenuFrames = 0;
            liveInput = downwell::Input();
            if (scenes.scene() == AmigaScene::splash)
                liveInput.fire = (liveFrameCount & 1) != 0;
            else if (scenes.scene() == AmigaScene::menu)
                liveInput.right = ++profileMenuFrames > 40;
#ifdef DOWNWELL_PROFILE_NAVIGATE
            else if (scenes.scene() == AmigaScene::styleSelect &&
                     scenes.styleYOffset() == 0 && (liveFrameCount % 16) == 0) {
                liveInput.right = ((liveFrameCount / 80) & 1) != 0;
                liveInput.left = !liveInput.right;
            }
#endif
#endif
#endif
            packedInput = downwell::packReplayInput(liveInput);
            DW_PROFILE_PHASE(profileSimulation);
            scenes.step(liveInput);
            DW_PROFILE_PHASE(profileOther);
            {
                const bool presentationChanged =
                    scenes.scene() != presentedScene ||
                    scenes.splashStage() != presentedSplash;
                const AmigaScene priorPresentedScene = presentedScene;
                const bool splashDitherChanged =
                    scenes.splashDitherFrame() != presentedSplashDither;
                const unsigned back = front ^ 1u;
                const bool menuEntry =
                    scenes.scene() == AmigaScene::menu &&
                    scenes.menuEnterSequenceActive();
                presentedScene = scenes.scene();
                presentedSplash = scenes.splashStage();
                presentedSplashDither = scenes.splashDitherFrame();
                if (presentedScene == AmigaScene::splash) {
                    if (presentationChanged || splashDitherChanged) {
                        CopperList* splashProgram =
                            &dynamicSplashLists[back];
                        bool built = true;
                        if (presentationChanged && presentedSplash == 2) {
                            splashProgram = &splashLogoList;
                        } else if (presentationChanged &&
                                   presentedSplash == 3) {
                            splashProgram = &splashCreditsList;
                        } else if (presentationChanged &&
                                   presentedSplash == 4) {
                            splashProgram = &splashControlsList;
                        } else {
                            built = dynamicSplashLists[back].buildNarrow(
                                amigaSplashFrame(memory.sceneCache(),
                                                 presentedSplash),
                                nullSprite, palette,
                                presentedSplash == 5 || presentedSplash == 6
                                    ? amigaSplashDitherTile(
                                          memory.sceneCache(),
                                          presentedSplashDither)
                                    : 0);
                            front = back;
                        }
                        if (!built)
                            break;
                        platform.publish(*splashProgram);
#ifdef DOWNWELL_SCENE_VERIFY
                        expectedSplashCopper = splashProgram->data();
#endif
                        /*
                         * Let hardware restart the selected program at VBlank
                         * before reporting the discrete stage to capture tools.
                         */
                        if (presentationChanged)
                            platform.waitVblank();
#ifdef DOWNWELL_SCENE_VERIFY
                        if (!sceneCreditsFrameReported &&
                            presentedSplash == 3) {
                            sceneCreditsFrameReported = true;
                            gDownwellCreditsFrame = memory.hud();
                            gDownwellCreditsCopper = platform.publishedCopper();
                            downwellCreditsFrameReady();
                        }
                        if (!sceneSplashFrameReported &&
                            presentedSplash == 2) {
                            sceneSplashFrameReported = true;
                            gDownwellSplashFrame = amigaSplashFrame(
                                memory.sceneCache(), presentedSplash);
                            downwellSplashFrameReady();
                        }
                        if (!sceneControlsFrameReported &&
                            presentedSplash == 4) {
                            gDownwellControlsCopper = platform.publishedCopper();
                            sceneControlsFrameReported = true;
                            gDownwellControlsFrame = amigaSplashFrame(
                                memory.sceneCache(), presentedSplash);
                            downwellControlsFrameReady();
                        }
#endif
                    }
                    // Splash owns this frame. Falling through would publish
                    // the gameplay list over it on every stage transition.
                    continue;
                }
                else if (presentedScene == AmigaScene::menu) {
                    if (presentationChanged &&
                        !amigaBuildStyleBorder(
                            activeGamePack, activeGamePackEnd,
                            memory.sceneCache() +
                                amigaStyleFrameBytes * 2))
                        break;
                }
                else if (presentedScene != AmigaScene::styleSelect &&
                         presentationChanged) {
                    buildReferencePicture(
                        &renderer, rooms, mainRoom, builders,
                        fragments, amigaInitialTerrainFragmentCount,
                        generatedDecorations, generatedDecorationCount,
                        &port, worldViewX, worldViewY);
                    if (presentedScene == AmigaScene::gameplay) {
                        drawFutureGameBlock(
                            &renderer, rooms, mainRoom, builders,
                            fragments, amigaInitialTerrainFragmentCount,
                            generatedDecorations, generatedDecorationCount,
                            &port, port.ringOrigin(), worldViewX, worldViewY);
                        gameplayPrefetchRemaining = gameplayPrefetchLines;
                    }
                }
                if (presentedScene == AmigaScene::menu) {
                    bool menuFrameReady = true;
                    UWORD* menuPlayerData = memory.nullSprite();
                    if (presentationChanged)
                        menuCopperMode[0] = menuCopperMode[1] = -1;
                    AmigaMenuScrollState menuScroll = {0, 0, 0};
                    DW_PROFILE_PHASE(profileTitle);
                    if (renderedMenuTitleFrame != scenes.menuTitleFrame()) {
                        menuFrameReady = amigaRenderMenuTitle(
                            (UWORD)scenes.menuTitleFrame(),
                            activeGamePack, activeGamePackEnd,
                            memory.menuForeground(), memory.menuEntry());
                        if (menuFrameReady)
                            renderedMenuTitleFrame = scenes.menuTitleFrame();
                    }
                    DW_PROFILE_PHASE(profileSprite);
                    if (scenes.menuPlayerVisible()) {
                        menuPlayerData = amigaMenuHardwareSprite(
                            activeGamePack, activeGamePackEnd,
                            (UWORD)scenes.menuPlayerSprite(),
                            (UWORD)scenes.menuPlayerFrame(),
                            scenes.menuPlayerFacing() < 0, back,
                            downwellPlayfieldX + scenes.menuPlayerX() -
                                (scenes.menuCameraX() - 80),
                            scenes.menuPlayerY() -
                                (scenes.menuCameraY() - 142));
                        menuFrameReady = menuFrameReady && menuPlayerData;
                    }
                    if (menuFrameReady) {
                        DW_PROFILE_PHASE(profileCopper);
                        DW_PROFILE_PHASE(profileScroll);
                        menuScroll = menuEntry
                            ? amigaMenuEntryScrollState(scenes.menuCameraX() - 80,
                                                        scenes.menuCameraY() - 142)
                            : amigaMenuScrollState(scenes.menuCameraX() - 80,
                                                   scenes.menuCameraY() - 142);
                        const UBYTE* foreground = menuEntry
                            ? memory.menuEntry() : memory.menuForeground();
                        DW_PROFILE_PHASE(profileCopper);
                        if (menuCopperMode[back] != 0)
                            menuFrameReady = menuCopperLists[back].buildScroll(
                                foreground, memory.menuBackground(), nullSprite,
                                nullSprite, palette, menuScroll, menuEntry);
                        menuFrameReady = menuFrameReady &&
                            menuCopperLists[back].updateScroll(
                                foreground, memory.menuBackground(),
                                menuPlayerData, menuScroll, menuEntry);
                        menuCopperMode[back] = 0;
                    }
                    if (!menuFrameReady)
                        break;
#ifdef DOWNWELL_SCENE_VERIFY
                    // Test the patched program against a freshly emitted oracle.
                    UWORD oracleWords[downwellCopperWords];
                    DownwellMenuCopperList oracle(oracleWords, downwellCopperWords);
                    Sprite oraclePlayer(menuPlayerData, playerSpriteHeight);
                    const bool oracleReady = oracle.buildScroll(
                                             menuEntry ? memory.menuEntry() : memory.menuForeground(),
                                             memory.menuBackground(), oraclePlayer,
                                             nullSprite, palette, menuScroll, menuEntry);
                    if (!oracleReady || oracle.size() != menuCopperLists[back].size())
                        ++gDownwellMenuCopperPatchFailures;
                    else for (unsigned word = 0; word < oracle.size(); ++word)
                        if (oracleWords[word] != menuCopperLists[back].data()[word])
                            ++gDownwellMenuCopperPatchFailures;
#endif
                    DW_PROFILE_PHASE(profilePublish);
                    platform.publish(menuCopperLists[back]);
#ifdef DOWNWELL_SCENE_VERIFY
                    if (menuEntry && menuScroll.backgroundVisibleLines == 0)
                        downwellMenuSkyCovered();
#endif
                    DW_PROFILE_PHASE(profileOther);
                    front = back;
                    if (presentationChanged &&
                        priorPresentedScene == AmigaScene::splash)
                        platform.waitVblank();
                    if (++menuSpriteFrameCount == 2)
                        downwellMenuSpriteReady();
#ifdef DOWNWELL_SCENE_VERIFY
                    if (!sceneMenuWakeFrameReported && !menuEntry &&
                        !scenes.menuPlayerNapping() &&
                        scenes.menuPlayerVisible() &&
                        scenes.menuPlayerSprite() == 27) {
                        sceneMenuWakeFrameReported = true;
                        // Test-only portable oracle; never used for presentation.
                        amigaRendererBuildHardwareSprite(
                            activeGamePack, activeGamePackEnd,
                            (UWORD)scenes.menuPlayerSprite(),
                            (UWORD)scenes.menuPlayerFrame(),
                            downwellPlayfieldX + scenes.menuPlayerX() -
                                (scenes.menuCameraX() - 80),
                            scenes.menuPlayerY() - (scenes.menuCameraY() - 142),
                            scenes.menuPlayerFacing() < 0,
                            memory.playerSprite(back), playerSpriteHeight);
                        gDownwellMenuSpriteOracle = memory.playerSprite(back);
                        gDownwellMenuFrameCapture.foregroundOffset =
                            menuScroll.foregroundOffset;
                        gDownwellMenuFrameCapture.backgroundOffset =
                            menuScroll.backgroundOffset;
                        gDownwellMenuFrameCapture.scrollControl =
                            menuScroll.scrollControl;
                        gDownwellMenuFrameCapture.titlePartCount = 0;
                        gDownwellMenuForeground = memory.menuForeground();
                        gDownwellMenuBackground = memory.menuBackground();
                        gDownwellMenuPlayerSprite =
                            menuPlayerData;
                        downwellMenuWakeFrameReady();
                    }
                    if (!sceneMenuTitleReported &&
                        scenes.menuTitleFrame() == 9) {
                        sceneMenuTitleReported = true;
                        gDownwellMenuFrameCapture.foregroundOffset =
                            menuScroll.foregroundOffset;
                        gDownwellMenuFrameCapture.backgroundOffset =
                            menuScroll.backgroundOffset;
                        gDownwellMenuFrameCapture.scrollControl =
                            menuScroll.scrollControl;
                        gDownwellMenuFrameCapture.titlePartCount = 0;
                        gDownwellMenuForeground = memory.menuForeground();
                        gDownwellMenuBackground = memory.menuBackground();
                        gDownwellMenuPlayerSprite =
                            menuPlayerData;
                        downwellMenuTitleFrameReady();
                        /* Retain the old verification breakpoint name. */
                        downwellMenuTitleSpriteReady();
                    }
                    if (!sceneMenuEntryReported && menuEntry) {
                        sceneMenuEntryReported = true;
                        gDownwellMenuEntryFrame = memory.menuEntry();
                        gDownwellMenuFrameCapture.foregroundOffset = menuScroll.foregroundOffset;
                        gDownwellMenuFrameCapture.backgroundOffset = menuScroll.backgroundOffset;
                        gDownwellMenuFrameCapture.scrollControl = menuScroll.scrollControl;
                        gDownwellMenuFrameCapture.titlePartCount = 0;
                        gDownwellMenuBackground = memory.menuBackground();
                        gDownwellMenuEntryTitlePartCount = 0;
                        downwellMenuEntryFrameReady();
                        downwellMenuEntryScrollReady();
                    }
#endif
                    continue;
                }
                if (presentedScene == AmigaScene::styleSelect) {
                    if (!amigaRenderStyleFrame(
                            activeGamePack, activeGamePackEnd,
                            memory.sceneCache() +
                                amigaStyleFrameBytes * 2,
                            scenes,
                            memory.sceneCache() +
                                back * amigaStyleFrameBytes, styleFrameState[back]))
                        break;
                    const UBYTE* styleFrame = memory.sceneCache() + back * amigaStyleFrameBytes;
#ifdef DOWNWELL_SCENE_VERIFY
                    AmigaStyleFrameState referenceState;
                    if (!amigaRenderStyleFrame(activeGamePack, activeGamePackEnd,
                            memory.sceneCache() + amigaStyleFrameBytes * 2,
                            scenes, memory.hud(), referenceState, true)) break;
                    ++gDownwellStyleReferenceFrames;
                    UBYTE* styleComposite = memory.hud() + amigaStyleFrameBytes;
                    const UBYTE* referenceBorder = memory.sceneCache() + amigaStyleFrameBytes * 2 - scenes.styleTileY() * amigaStyleBytesPerRow;
                    for (unsigned byte = 0; byte < amigaStylePlaneBytes; ++byte) {
                        const UBYTE occupied = styleFrame[byte] | styleFrame[byte + amigaStylePlaneBytes];
                        styleComposite[byte] = styleFrame[byte] | (referenceBorder[byte] & ~occupied);
                        styleComposite[byte + amigaStylePlaneBytes] = styleFrame[byte + amigaStylePlaneBytes] |
                            (referenceBorder[byte + amigaStyleBorderPlaneBytes] & ~occupied);
                    }
                    for (unsigned byte = 0; byte < amigaStyleFrameBytes; ++byte) {
                        if (styleComposite[byte] != memory.hud()[byte]) {
                            ++gDownwellStyleReferenceFailures;
                            break;
                        }
                    }
#endif
                    DW_PROFILE_PHASE(profileCopper);
                    const UBYTE* styleMask = scenes.styleDitherY() < 684 || scenes.styleSelected()
                        ? amigaStyleMask(memory.sceneCache(), scenes) : 0;
                    const UBYTE* styleBorder = memory.sceneCache() + amigaStyleFrameBytes * 2 - scenes.styleTileY() * amigaStyleBytesPerRow;
                    if (!(styleListBuilt[back]
                            ? styleLists[back].update(styleFrame, styleMask, styleBorder)
                            : styleLists[back].build(styleFrame, styleMask, nullSprite, palette, styleBorder)))
                        break;
                    styleListBuilt[back] = true;
                    DW_PROFILE_PHASE(profilePublish);
                    platform.publish(styleLists[back]);
                    front = back;
                    DW_PROFILE_PHASE(profileOther);
#ifdef DOWNWELL_SCENE_VERIFY
                    gDownwellStyleMask = styleMask;
                    gDownwellStyleCopper = styleLists[back].data();
                    if (scenes.styleDitherY() == 528 && !scenes.styleSelected()) {
                        gDownwellStyleFrame = styleComposite;
                        downwellStyleRevealFrameReady();
                    }
                    if (!sceneStyleFrameReported &&
                        scenes.styleYOffset() == 0 &&
                        !scenes.styleSelected()) {
                        sceneStyleFrameReported = true;
                        gDownwellStyleFrame =
                            styleComposite;
                        downwellStyleFrameReady();
                    }
                    if (!sceneLockedStyleFrameReported &&
                        sceneStyleFrameReported &&
                        scenes.styleYOffset() == 0 &&
                        scenes.styleCursor() == 1 &&
                        !scenes.styleSelected()) {
                        sceneLockedStyleFrameReported = true;
                        gDownwellLockedStyleFrame =
                            styleComposite;
                        downwellLockedStyleFrameReady();
                    }
                    if (!sceneSelectedStyleFrameReported &&
                        scenes.styleSelected() &&
                        scenes.styleVerticalDitherY() == 0) {
                        sceneSelectedStyleFrameReported = true;
                        gDownwellSelectedStyleFrame =
                            styleComposite;
                        downwellSelectedStyleFrameReady();
                    }
#endif
                    continue;
                }
                if (presentationChanged) {
                    if (!gameplayCopperLists[back].build(
                            gameBitmap, hudBitmap,
                            presentedScene == AmigaScene::gameplay
                                ? playerSprites[0] : nullSprite,
                            bulletSprites + back *
                                AmigaPortRuntime::bulletCapacity,
                            AmigaPortRuntime::bulletCapacity,
                            enemySprites[back], palette,
                            port.ringOrigin()))
                        break;
                    platform.publish(gameplayCopperLists[back]);
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
                    if (direction > 0 && gameplayPrefetchRemaining > 0) {
                        --gameplayPrefetchRemaining;
                        if (gameplayPrefetchRemaining == 0) {
                            drawFutureGameBlock(
                                &renderer, rooms, mainRoom, builders,
                                fragments,
                                amigaInitialTerrainFragmentCount,
                                generatedDecorations,
                                generatedDecorationCount,
                                &port, (unsigned)origin,
                                worldViewX, worldViewY);
                            gameplayPrefetchRemaining =
                                gameplayPrefetchLines;
                        }
                    } else {
                        drawRevealedGameLine(
                            &renderer, rooms, mainRoom, builders,
                            fragments, amigaInitialTerrainFragmentCount,
                            generatedDecorations, generatedDecorationCount,
                            &port, (unsigned)origin, worldViewX,
                            direction > 0
                                ? worldViewY + downwellDisplayHeight - 1
                                : worldViewY,
                            direction > 0
                                ? downwellDisplayHeight - 1 : 0);
                    }
                }
                if (direction < 0) {
                    drawFutureGameBlock(
                        &renderer, rooms, mainRoom, builders,
                        fragments, amigaInitialTerrainFragmentCount,
                        generatedDecorations, generatedDecorationCount,
                        &port, port.ringOrigin(), worldViewX, worldViewY);
                    gameplayPrefetchRemaining = gameplayPrefetchLines;
                }
            }
            if (cameraXChanged) {
                worldViewX = mainViewX + port.cameraOffsetX();
                for (int screenY = 0;
                     screenY < downwellDisplayHeight; ++screenY)
                    drawRevealedGameLine(
                        &renderer, rooms, mainRoom, builders,
                        fragments, amigaInitialTerrainFragmentCount,
                        generatedDecorations, generatedDecorationCount,
                        &port,
                        port.ringOrigin(), worldViewX,
                        worldViewY + screenY, screenY);
                drawFutureGameBlock(
                    &renderer, rooms, mainRoom, builders,
                    fragments, amigaInitialTerrainFragmentCount,
                    generatedDecorations, generatedDecorationCount,
                    &port, port.ringOrigin(), worldViewX, worldViewY);
                gameplayPrefetchRemaining = gameplayPrefetchLines;
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
                            generatedDecorations, generatedDecorationCount,
                            &port, port.ringOrigin(), worldViewX,
                            line, screenY);
                    }
                    (void)destroyedBlockX[block];
                }
            }
            if (!port.playerVisible()) {
                playerSprites[back].hide();
            } else if (!amigaRendererBuildHardwareSprite(
                           activeGamePack, activeGamePackEnd,
                           (UWORD)(port.dead()
                                       ? 0 : port.player().spriteId()),
                           (UWORD)(port.dead()
                                       ? 0 : port.player().spriteFrame()),
                           downwellPlayfieldX + 80 +
                               port.player().offsetX() -
                               port.cameraOffsetX(),
#ifdef DOWNWELL_REPLAY_VERIFY
                           gameplayPlayerGroundY + port.player().offsetY(),
#else
                           gameplayPlayerStartY +
                               port.player().offsetY() -
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
                    Sprite& sprite = actorSpriteAt(
                        actorSprites, enemySprites[back], actorCount);
                    const int spriteResult =
                        amigaRendererBuildHardwareSprite(
                        activeGamePack, activeGamePackEnd, 459, 0,
                        downwellPlayfieldX + 80 + state.x -
                            port.cameraOffsetX(),
                        gameplayPlayerStartY + state.y -
                            port.cameraOffsetY(),
                        0, sprite.data(), bulletSpriteHeight);
                    if (spriteResult == 0)
                        frameReady = false;
                    else if (spriteResult == 1)
                        ++actorCount;
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
                const unsigned partCount =
                    amigaRendererHardwareSpritePartCount(
                        activeGamePack, activeGamePackEnd,
                        (UWORD)enemy.spriteId, (UWORD)enemy.imageFrame);
                if (partCount == 0) {
                    frameReady = false;
                    continue;
                }
                for (unsigned part = 0;
                     part < partCount &&
                     actorCount <= AmigaPortRuntime::bulletCapacity;
                     ++part) {
                    Sprite& sprite = actorSpriteAt(
                        actorSprites, enemySprites[back], actorCount);
                    const int spriteResult =
                        amigaRendererBuildHardwareSpritePart(
                            activeGamePack, activeGamePackEnd,
                            (UWORD)enemy.spriteId,
                            (UWORD)enemy.imageFrame, part,
                            downwellPlayfieldX + 80 + enemy.x -
                                port.cameraOffsetX(),
                            screenY, enemy.facing < 0,
                            sprite.data(), enemySpriteHeight);
                    if (spriteResult == 0)
                        frameReady = false;
                    else if (spriteResult == 1)
                        ++actorCount;
                }
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
                Sprite& sprite = actorSpriteAt(
                    actorSprites, enemySprites[back], actorCount);
                const int spriteResult =
                    amigaRendererBuildHardwareSprite(
                        activeGamePack, activeGamePackEnd,
                        (UWORD)(gem.large ? 134 : 133),
                        (UWORD)((gem.age / 2u) & 3u),
                        downwellPlayfieldX + 80 + gem.x -
                            port.cameraOffsetX(),
                        screenY, 0, sprite.data(), enemySpriteHeight);
                if (spriteResult == 0)
                    frameReady = false;
                else if (spriteResult == 1)
                    ++actorCount;
            }
#endif
            while (actorCount < AmigaPortRuntime::bulletCapacity)
                actorSprites[actorCount++].hide();
            if (actorCount <= AmigaPortRuntime::bulletCapacity)
                enemySprites[back].hide();
            if (frameReady &&
                !gameplayCopperLists[back].build(
                    gameBitmap, hudBitmap, playerSprites[back],
                    bulletSprites + back *
                        AmigaPortRuntime::bulletCapacity,
                    AmigaPortRuntime::bulletCapacity,
                    enemySprites[back], palette, port.ringOrigin()))
                frameReady = false;
            if (!frameReady)
                break;
            /* AmigaPlatform makes the published list live at next VBlank. */
            platform.publish(gameplayCopperLists[back]);
            front = back;
#ifdef DOWNWELL_SCENE_VERIFY
            if (!sceneGameplayLandedReported && port.player().grounded()) {
                sceneGameplayLandedReported = true;
                sceneGameplayLandedCameraY = port.cameraOffsetY();
                exposeGameplayFrame(memory, back, port.ringOrigin());
                downwellGameplayFrameReady();
                downwellSceneGameplayLanded();
            } else if (sceneGameplayLandedReported &&
                       !sceneGameplayActiveReported &&
                       port.cameraOffsetY() >=
                           sceneGameplayLandedCameraY + 256) {
                bool awakeEnemy = false;
                for (unsigned enemy = 0; enemy < port.enemyCount(); ++enemy)
                    awakeEnemy = awakeEnemy || port.enemyAt(enemy).awake;
                if (awakeEnemy) {
                    sceneGameplayActiveReported = true;
                    exposeGameplayFrame(memory, back, port.ringOrigin());
                    downwellGameplayActiveFrameReady();
                    downwellSceneGameplayActive();
                }
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
