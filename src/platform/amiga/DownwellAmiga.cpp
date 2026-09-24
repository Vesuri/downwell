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
#include "AmigaActorChannels.h"
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
#include "core/GameMakerRounding.h"
#include "core/HudNumbers.h"
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
    gameplayRegularSpriteCount = 6,
    bulletSpriteBufferCount = 2 * gameplayRegularSpriteCount,
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
static AmigaTerrainIndex terrainIndex;
static AmigaTerrainTiles terrainTiles;

struct LiveGenerationContext {
    AmigaAreaOneTerrainBuilder* builder;
    const downwell::BuilderPatternPack* patterns;
    const downwell::ObjectPack* objects;
    AmigaPortRuntime* runtime;
    int dirtyFirst = 8192;
    int dirtyEnd = 0;
};
extern "C" {
volatile ULONG gDownwellLiveFragments = 0;
volatile ULONG gDownwellLiveEnemies = 0;
volatile ULONG gDownwellLiveRandomDraws = 0;
volatile ULONG gDownwellLiveGenerationFailed = 0;
}
static bool generateLiveTerrain(void*, AmigaPortRuntime&,
    downwell::GameMakerRandom&, int);

static bool appendGeneratedTerrain(void* opaque,
    const downwell::DeferredCreateInstance& command) {
    auto& live = *static_cast<LiveGenerationContext*>(opaque);
    if (!amigaAddTerrainObject(terrainIndex, command.objectId, command.x, command.y) ||
        !terrainTiles.append(command)) return false;
    if (command.objectId == 60 || command.objectId == 92 || command.objectId == 93 ||
        command.objectId == 289 || command.objectId == 293 || command.objectId == 296 ||
        command.objectId == 109 || command.objectId == 306) {
        if (command.y - 24 < live.dirtyFirst) live.dirtyFirst = command.y - 24;
        if (command.y + 24 > live.dirtyEnd) live.dirtyEnd = command.y + 24;
    }
    if (command.objectId == 92) {
        const int x = command.x - 240, y = command.y - 176;
        return live.runtime->appendBreakables(&x, &y, 1);
    }
    if (amigaEnemyHealth(command.objectId)) {
        downwell::PackedObject object;
        return live.objects->findObject(command.objectId, object) &&
            live.runtime->configureEnemy(live.runtime->enemyCount(), command.objectId,
                object.spriteId, command.x - 240, command.y - 176,
                command.integerVelocityX, command.integerVelocityY,
                amigaEnemyHealth(command.objectId), amigaEnemyReward(command.objectId),
                command.motionState, command.auxiliaryState,
                command.alarms.get(0), command.alarms.get(1),
                command.integerDirection, command.integerMoveSpeed,
                command.active, command.grounded, true);
    }
    return true;
}

static volatile UBYTE* const ciaaPra = (volatile UBYTE*)0xbfe001;
static volatile unsigned long long lastPortCheckpoint;
static const UBYTE* activeGamePack = amiga_game_pack;
static const UBYTE* activeGamePackEnd = amiga_game_pack_end;
static const UBYTE* activeHudPack = amiga_hud_pack;
static const UBYTE* activeHudPackEnd = amiga_hud_pack_end;

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

extern "C" {
volatile ULONG gDownwellActorFailureSprite;
volatile ULONG gDownwellActorFailureFrame;
volatile ULONG gDownwellStartupFailure;
volatile ULONG gDownwellStartupChipFree;
volatile ULONG gDownwellStartupChipLargest;
volatile ULONG gDownwellStartupAssetBytes;
volatile ULONG gDownwellStartupChipRemaining;
}

#ifdef DOWNWELL_REPLAY_VERIFY
#include "core/GemVerification.h"
extern "C" {
volatile ULONG gReplayComplete;
volatile ULONG gReplayFrames;
volatile ULONG gReplayDigestHigh;
volatile ULONG gReplayDigestLow;
volatile ULONG gGemReplayDigestHigh;
volatile ULONG gGemReplayDigestLow;
volatile ULONG gGemReplayCoverage;

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
volatile ULONG gDownwellGameBufferFailures;
volatile ULONG gDownwellGameplayBobChecks;
volatile ULONG gDownwellGameplayBobFailures;
volatile ULONG gDownwellTerrainRasterFailures;
volatile ULONG gDownwellStackGuardReady;
volatile ULONG gDownwellTerrainRasterChecks;
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
volatile ULONG gDownwellCaptureTick;
volatile LONG gDownwellCapturePlayerXQ8, gDownwellCapturePlayerYQ8;
volatile LONG gDownwellCaptureViewX, gDownwellCaptureViewY;
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
__attribute__((noinline, used)) void downwellGameplayLongRunReady(void) {
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
    amigaRendererDrawAsset(renderer, activeHudPack, activeHudPackEnd,
                           sprite, frame, anchorX, anchorY, flipX);
}

static void drawGameAsset(AmigaRenderer* renderer, UWORD sprite, UWORD frame,
                          int anchorX, int anchorY, int flipX) {
    DW_PROFILE_SCOPE(profileTerrainAsset);
    amigaRendererDrawAsset(renderer, activeGamePack, activeGamePackEnd,
                           sprite, frame, anchorX, anchorY, flipX);
}

static void drawGameText(AmigaRenderer* renderer, UWORD text,
                         int x, int y) {
    downwell::drawGemmyTextLayers(x, y, [&](int px, int py, downwell::Color color) {
        const unsigned pen = color.r == 0 ? 3 : color.g == 0 ? 2 : 1;
        amigaRendererDrawAssetPen(renderer, activeGamePack, activeGamePackEnd,
                                  text, 0, px, py, 0, pen);
    });
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
    drawAsset(renderer, 65010, digit, x, y, 0);
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
    // The label clear overlaps the gauge's lower rim. Restore the two gauge
    // layers in the source draw order when ammo changes, not every frame.
    amigaRendererFillRect(renderer, 282, 232, 20, 20, 3);
    drawAsset(renderer, 416, 0, 284, 43, 0);
    /*
     * The frame is static on PF2, but its interior follows gunboot charge.
     * Clear the previous white column before filling upward from the recovered
     * 233-pixel bottom anchor, exactly like the desktop 4:3 HUD path.
     */
    amigaRendererFillRect(renderer, gaugeX, gaugeTop,
                          gaugeWidth, gaugeLength, 3);
    if (filled > 0)
        amigaRendererFillRect(renderer, gaugeX,
                              gaugeTop + gaugeLength - filled,
                              gaugeWidth, filled, 1);
    drawAsset(renderer, 416, 1, 284, 43, 0);
    drawAmmoDigit(renderer, 290, 239, ammo);
}

static void redrawHealth(AmigaRenderer* renderer, unsigned health) {
    amigaRendererClearClipRect(renderer);
    amigaRendererFillRect(renderer, 8, 6, 92, 28, 3);
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
    amigaRendererFillRect(renderer, 316, 4, 64, 30, 3);
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
    DW_PROFILE_SCOPE(profileWallFrame);
    if (terrainCollision.index) {
        const auto& index = *terrainCollision.index;
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


// Each display ring independently owes modified rows of the private backing.
static UBYTE gameDirtyRows[downwellGameRingHeight];
static bool gameRowsPending[2];
static unsigned gameDirtyFirst[2] = {downwellGameRingHeight, downwellGameRingHeight};
static unsigned gameDirtyEnd[2];
static bool gameRingInitialized[2];
static Bitmap* terrainCopyTarget;
static const Bitmap* terrainCopyBacking;
static unsigned terrainCopyBuffer;

// Three camera columns (left room, well, right room), word-aligned around
// logical x=110..269. The level raster is Fast RAM; only exposed rows enter
// the Chip-RAM rings. Keep the original compositor as the cache oracle.
enum { terrainRasterRows = 8192, terrainRasterWords = 11, terrainRasterX = 96 };
static UWORD terrainRaster[3][terrainRasterRows][2][terrainRasterWords];
static bool terrainRasterDirty[3][terrainRasterRows];
static bool terrainRasterReady;

static bool generateLiveTerrain(void* opaque, AmigaPortRuntime& runtime,
    downwell::GameMakerRandom& random, int bottom) {
    auto& live = *static_cast<LiveGenerationContext*>(opaque);
    live.dirtyFirst = terrainRasterRows;
    live.dirtyEnd = 0;
    downwell::AreaOneBuilderStep result;
    const bool ok = live.builder->stepVisible(*live.patterns, random, bottom, result);
    gDownwellLiveFragments = live.builder->fragmentCount();
    gDownwellLiveEnemies = runtime.enemyCount();
    gDownwellLiveRandomDraws = static_cast<ULONG>(random.drawCount());
    if (!ok) gDownwellLiveGenerationFailed = 1;
    if (live.dirtyFirst < 0) live.dirtyFirst = 0;
    if (live.dirtyEnd > terrainRasterRows) live.dirtyEnd = terrainRasterRows;
    for (unsigned column = 0; column < 3; ++column)
        for (int y = live.dirtyFirst; y < live.dirtyEnd; ++y)
            terrainRasterDirty[column][y] = true;
    return ok;
}
typedef ULONG RasterLong __attribute__((may_alias));

static void copyTerrainRows68000(const UWORD* sourceWords, UBYTE* plane0,
                                UBYTE* plane1, unsigned rows) {
    if (!rows) return;
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
}

static void transferTerrainRows(AmigaRenderer* renderer, int viewX,
                               int worldY, int screenY, int height, bool store) {
    const int column = (viewX - mainViewX + 160) / 160;
    UWORD* cachedRow = &terrainRaster[column][worldY][0][0];
    unsigned physical = renderer->gameOrigin + screenY;
    if (physical >= downwellGameRingHeight) physical -= downwellGameRingHeight;
    if (!store) {
        const bool pipeline = height > 96 && terrainCopyTarget;
        unsigned remaining = height;
        while (remaining) {
            unsigned count = downwellGameRingHeight - physical;
            if (count > remaining) count = remaining;
            if (pipeline && count > 128) count = 128;
            UBYTE* row = renderer->game + physical * downwellBytesPerPlaneLine + terrainRasterX / 8;
            copyTerrainRows68000(cachedRow, row, row + downwellGameRingPlaneBytes, count);
            if (pipeline) {
                // These completed rows are immutable while the CPU prepares
                // the next disjoint chunk. DMA copies them concurrently.
                terrainCopyTarget->copy(*terrainCopyBacking, terrainRasterX, physical,
                    terrainRasterX, physical, terrainRasterWords * 16, count);
                const UBYTE bit = (UBYTE)~(1u << terrainCopyBuffer);
                for (unsigned y = physical; y < physical + count; ++y)
                    gameDirtyRows[y] &= bit;
            }
            remaining -= count;
            cachedRow += count * terrainRasterWords * 2;
            physical += count;
            if (physical == downwellGameRingHeight) physical = 0;
        }
        return;
    }
    for (int y = 0; y < height; ++y) {
        for (unsigned plane = 0; plane < 2; ++plane) {
            UWORD* chip = (UWORD*)(renderer->game + plane * downwellGameRingPlaneBytes +
                physical * downwellBytesPerPlaneLine + terrainRasterX / 8);
            UWORD* cached = cachedRow + plane * terrainRasterWords;
            RasterLong* destination = (RasterLong*)(store ? cached : chip);
            const RasterLong* source = (const RasterLong*)(store ? chip : cached);
            // Native 68000 long moves require only word alignment. This
            // bounded 22-byte row avoids byte/word loops and runtime multiplies.
            destination[0] = source[0]; destination[1] = source[1];
            destination[2] = source[2]; destination[3] = source[3];
            destination[4] = source[4];
            ((UWORD*)destination)[10] = ((const UWORD*)source)[10];
        }
        cachedRow += terrainRasterWords * 2;
        if (++physical == downwellGameRingHeight) physical = 0;
    }
}

static void dirtyGameRows(unsigned origin, unsigned y, unsigned height) {
    gameRowsPending[0] = gameRowsPending[1] = true;
    unsigned first = (origin + y) % downwellGameRingHeight;
    unsigned end = first + height;
    if (end > downwellGameRingHeight) { first = 0; end = downwellGameRingHeight; }
    for (unsigned buffer = 0; buffer < 2; ++buffer) {
        if (first < gameDirtyFirst[buffer]) gameDirtyFirst[buffer] = first;
        if (end > gameDirtyEnd[buffer]) gameDirtyEnd[buffer] = end;
    }
    unsigned physical = (origin + y) % downwellGameRingHeight;
    for (unsigned row = 0; row < height; ++row) {
        gameDirtyRows[physical] = 3;
        if (++physical == downwellGameRingHeight) physical = 0;
    }
}

static void syncGameRows(Bitmap& destination, const Bitmap& backing, unsigned buffer) {
    DW_PROFILE_SCOPE(profileGameCopy);
    if (!gameRowsPending[buffer]) return;
    gameRowsPending[buffer] = false;
    const UBYTE bit = (UBYTE)(1u << buffer);
    unsigned row = gameDirtyFirst[buffer];
    const unsigned end = gameDirtyEnd[buffer];
    gameDirtyFirst[buffer] = downwellGameRingHeight;
    gameDirtyEnd[buffer] = 0;
    while (row < end) {
        if (!(gameDirtyRows[row] & bit)) { ++row; continue; }
        const unsigned first = row;
        while (row < end && (gameDirtyRows[row] & bit))
            gameDirtyRows[row++] &= (UBYTE)~bit;
        const unsigned x = gameRingInitialized[buffer] ? terrainRasterX : 0;
        const unsigned width = gameRingInitialized[buffer] ? terrainRasterWords * 16 : downwellDmaWidth;
        destination.copy(backing, x, first, x, first, width, row - first);
    }
    // There is no CPU work left to overlap here. Give the queued DMA copy
    // the slots that a busy-waiting CPU would otherwise steal.
    AmigaHardware::setBlitterNasty(true);
    AmigaHardware::blitterDrain();
    AmigaHardware::setBlitterNasty(false);
    gameRingInitialized[buffer] = true;
}

static void drawIndexedWalls(AmigaRenderer* renderer,
    const downwell::RoomPack& rooms, const downwell::PackedRoom& room,
    int viewX, int viewY, int clipX, int clipY, int width, int height) {
    const int firstX = viewX + clipX - downwellPlayfieldX;
    const int firstY = viewY + clipY;
    for (int y = (firstY - 8) & ~15; y < firstY + height + 8; y += 16)
        for (int x = (firstX - 8) & ~15; x < firstX + width + 8; x += 16)
            if (amigaIndexedWall(*terrainCollision.index, x, y))
                drawGameAsset(renderer, 51, mainWallFrame(rooms, room, x, y),
                              downwellPlayfieldX + x - viewX, y - viewY, 0);
}

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
    int viewX, int viewY, int clipY, int clipHeight,
    int clipX = downwellPlayfieldX, int clipWidth = downwellPlayfieldWidth,
    bool refresh = false) {
    unsigned index;
    const int firstWorldY = viewY + clipY;
    const int finalWorldY = firstWorldY + clipHeight;
    dirtyGameRows(renderer->gameOrigin, clipY, clipHeight);
    const bool cacheable = viewX >= mainViewX - 160 && viewX <= mainViewX + 160 &&
        (viewX - mainViewX) % 160 == 0 && firstWorldY >= 0 &&
        finalWorldY <= terrainRasterRows;
    if (terrainRasterReady && cacheable && !refresh) {
        const int column = (viewX - mainViewX + 160) / 160;
        for (int y = firstWorldY; y < finalWorldY; ++y)
            if (terrainRasterDirty[column][y]) { refresh = true; break; }
        if (!refresh) {
            transferTerrainRows(renderer, viewX, firstWorldY, clipY, clipHeight, false);
#ifdef DOWNWELL_SCENE_VERIFY
            // Compare the cached row transfer against fresh composition at
            // the same ring position, including the physical wrap boundary.
            static UWORD expected[downwellDisplayHeight][2][terrainRasterWords];
            for (int y = 0; y < clipHeight; ++y)
                for (unsigned plane = 0; plane < 2; ++plane) {
                    const unsigned physical = (renderer->gameOrigin + clipY + y) % downwellGameRingHeight;
                    const UWORD* chip = (const UWORD*)(renderer->game + plane * downwellGameRingPlaneBytes +
                        physical * downwellBytesPerPlaneLine + terrainRasterX / 8);
                    for (unsigned word = 0; word < terrainRasterWords; ++word)
                        expected[y][plane][word] = chip[word];
                }
            terrainRasterReady = false;
            amigaRendererClearGameRows(renderer, clipY, clipHeight, 0);
            drawMainWorldSpan(renderer, rooms, room, builders, fragments, fragmentCount,
                decorations, decorationCount, runtime, viewX, viewY, clipY, clipHeight, clipX, clipWidth);
            AmigaHardware::blitterDrain();
            terrainRasterReady = true;
            for (int y = 0; y < clipHeight; ++y)
                for (unsigned plane = 0; plane < 2; ++plane) {
                    const unsigned physical = (renderer->gameOrigin + clipY + y) % downwellGameRingHeight;
                    const UWORD* chip = (const UWORD*)(renderer->game + plane * downwellGameRingPlaneBytes +
                        physical * downwellBytesPerPlaneLine + terrainRasterX / 8);
                    for (unsigned word = 0; word < terrainRasterWords; ++word)
                        if (expected[y][plane][word] != chip[word]) ++gDownwellTerrainRasterFailures;
                }
            ++gDownwellTerrainRasterChecks;
#endif
            return;
        }
        amigaRendererClearGameRows(renderer, clipY, clipHeight, 0);
    }
    if (terrainRasterReady && !cacheable && !refresh)
        amigaRendererClearGameRows(renderer, clipY, clipHeight, 0);
    amigaRendererSetClipRect(renderer, clipX, clipY, clipWidth, clipHeight);
    {
    DW_PROFILE_SCOPE(profileTerrainWalls);
    if (terrainCollision.index)
        drawIndexedWalls(renderer, rooms, room, viewX, viewY,
                             clipX, clipY, clipWidth, clipHeight);
    for (index = 0; !terrainCollision.index && index < room.instanceCount; ++index) {
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
    amigaRendererClearClipRect(renderer);
    if (terrainRasterReady && cacheable && refresh) {
        AmigaHardware::blitterDrain();
        transferTerrainRows(renderer, viewX, firstWorldY, clipY, clipHeight, true);
        const int column = (viewX - mainViewX + 160) / 160;
        for (int y = firstWorldY; y < finalWorldY; ++y)
            terrainRasterDirty[column][y] = false;
    }
}

static void prepareTerrainRaster(AmigaRenderer* renderer,
    const downwell::RoomPack& rooms, const downwell::PackedRoom& room,
    const downwell::BuilderPatternPack& builders,
    const AmigaTerrainFragment* fragments, unsigned fragmentCount,
    const AmigaTerrainDecoration* decorations, unsigned decorationCount,
    const AmigaPortRuntime* runtime) {
    int lastY = 0;
    for (unsigned i = 0; i < room.instanceCount; ++i) {
        downwell::PackedRoomInstance instance;
        if (rooms.roomInstance(room, i, instance) && instance.objectId == 60 && instance.y + 32 > lastY)
            lastY = instance.y + 32;
    }
    for (unsigned i = 0; i < fragmentCount; ++i) {
        const int end = fragments[i].y + fragments[i].pattern.rowCount * 16 + 64;
        if (end > lastY) lastY = end;
    }
    for (unsigned i = 0; i < decorationCount; ++i)
        if (decorations[i].y + 64 > lastY) lastY = decorations[i].y + 64;
    if (lastY > terrainRasterRows) return; // retain the generic compositor
    for (int x = mainViewX - 160; x <= mainViewX + 160; x += 160)
        for (int y = 0; y < lastY; y += downwellDisplayHeight) {
            int height = lastY - y;
            if (height > downwellDisplayHeight) height = downwellDisplayHeight;
            amigaRendererSetGameOrigin(renderer, 0);
            amigaRendererClearGameRows(renderer, 0, height, 0);
            drawMainWorldSpan(renderer, rooms, room, builders, fragments, fragmentCount,
                decorations, decorationCount, runtime, x, y, 0, height);
            AmigaHardware::blitterDrain();
            transferTerrainRows(renderer, x, y, 0, height, true);
        }
    terrainRasterReady = true;
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
    dirtyGameRows(0, 0, downwellGameRingHeight);
    drawMainWorld(renderer, rooms, room, builders,
                  fragments, fragmentCount,
                  decorations, decorationCount, runtime,
                  viewX, viewY);

    amigaRendererClearClipRect(renderer);
    // PF2 is above all sprites; its nonzero black pen masks the HUD margins.
    amigaRendererFillRect(renderer, 0, 0, downwellPlayfieldX,
                          downwellDisplayHeight, 3);
    amigaRendererFillRect(renderer, downwellPlayfieldX + downwellPlayfieldWidth,
                          0, downwellVisibleWidth - downwellPlayfieldX - downwellPlayfieldWidth,
                          downwellDisplayHeight, 3);
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
    if (!terrainRasterReady)
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
    return index < gameplayRegularSpriteCount
               ? regular[index] : overflow;
}

struct GameplayBob {
    UWORD sprite, frame;
    int x, y, flip;
};

extern "C" __attribute__((noinline, used)) void downwellStartupFailed() {
    __asm__ volatile("" ::: "memory");
}
extern "C" __attribute__((noinline)) void downwellGameplayRenderFailure() {
    __asm__ volatile("" ::: "memory");
}

static bool appendGameplayActor(GameplayBob* actors, unsigned& count,
                                 UWORD sprite, UWORD frame, int x, int y, int flip) {
    // Offline gameplay actors have <=64-pixel canvases and origins inside
    // them. Avoid Chip-directory reads for actors far outside the viewport.
    if (x < downwellPlayfieldX - 64 || x > downwellPlayfieldX + downwellPlayfieldWidth + 64 ||
        y < -64 || y > downwellDisplayHeight + 64) return true;
    int left, top, width, height;
    if (!amigaRendererAssetBounds(activeGamePack, activeGamePackEnd,
            sprite, frame, x, y, flip, &left, &top, &width, &height)) {
        gDownwellActorFailureSprite = sprite;
        gDownwellActorFailureFrame = frame;
        return false;
    }
    if (left < downwellPlayfieldX + downwellPlayfieldWidth &&
        left + width > downwellPlayfieldX && top < downwellDisplayHeight && top + height > 0)
        actors[count++] = {sprite, frame, x, y, flip};
    return true;
}

#ifdef DOWNWELL_BOB_VERIFY
extern "C" __attribute__((noinline)) void downwellGameplayBobsReady() {
    __asm__ volatile("" ::: "memory");
}
#endif

// Overflow actors are ordinary masked planar blits into the inactive display,
// never into the pristine terrain backing. Only their touched rows are restored
// before this display buffer is reused.
static void drawGameplayBobs(AmigaRenderer renderer, Bitmap& display,
                             unsigned origin, const GameplayBob* bobs,
                             unsigned count) {
    renderer.game = (UBYTE*)display.data;
    renderer.target = renderer.game;
    amigaRendererSetGameOrigin(&renderer, origin);
    amigaRendererSetClipRect(&renderer, downwellPlayfieldX, 0,
                             downwellPlayfieldWidth, downwellDisplayHeight);
#ifdef DOWNWELL_SCENE_VERIFY
    static UBYTE expected[downwellGameRingBytes];
    for (unsigned byte = 0; byte < downwellGameRingBytes; ++byte)
        expected[byte] = renderer.game[byte];
    AmigaRenderer oracle = renderer;
    oracle.game = oracle.target = expected;
    oracle.blitterEnabled = 0;
#endif
    for (unsigned i = 0; i < count; ++i) {
        const GameplayBob& bob = bobs[i];
        int x, y, width, height;
        if (!amigaRendererAssetBounds(activeGamePack, activeGamePackEnd,
                bob.sprite, bob.frame, bob.x, bob.y, bob.flip,
                &x, &y, &width, &height)) continue;
        if (x >= downwellPlayfieldX + downwellPlayfieldWidth ||
            x + width <= downwellPlayfieldX || y >= downwellDisplayHeight ||
            y + height <= 0) continue;
        amigaRendererDrawAsset(&renderer, activeGamePack, activeGamePackEnd,
                               bob.sprite, bob.frame, bob.x, bob.y, bob.flip);
#ifdef DOWNWELL_SCENE_VERIFY
        amigaRendererDrawAsset(&oracle, activeGamePack, activeGamePackEnd,
                               bob.sprite, bob.frame, bob.x, bob.y, bob.flip);
        ++gDownwellGameplayBobChecks;
#endif
        int bottom = y + height;
        if (y < 0) y = 0;
        if (bottom > downwellDisplayHeight) bottom = downwellDisplayHeight;
        dirtyGameRows(origin, y, bottom - y);
    }
    AmigaHardware::blitterDrain();
#ifdef DOWNWELL_SCENE_VERIFY
    for (unsigned byte = 0; byte < downwellGameRingBytes; ++byte)
        if (expected[byte] != renderer.game[byte]) ++gDownwellGameplayBobFailures;
#ifdef DOWNWELL_BOB_VERIFY
    if (gDownwellGameplayBobChecks >= 100) downwellGameplayBobsReady();
#endif
#endif
}

#ifdef DOWNWELL_SCENE_VERIFY
static void exposeGameplayFrame(const AmigaMemory& memory, const Bitmap& display, unsigned buffer,
                                const AmigaPortRuntime& port) {
    gDownwellCaptureTick = port.tick();
    gDownwellCapturePlayerXQ8 = 240L * 256 + port.player().offsetXFixed();
    gDownwellCapturePlayerYQ8 = 176L * 256 + port.player().offsetYFixed();
    gDownwellCaptureViewX = 160 + port.cameraOffsetX();
    gDownwellCaptureViewY = 34 + port.cameraOffsetY();
    gDownwellGameplayGame = (const UBYTE*)display.data;
    for (unsigned byte = 0; byte < downwellGameRingBytes; ++byte)
        // Dirty rows include deliberately drawn BOBs, unlike pristine backing.
        if (!(gameDirtyRows[(byte % downwellGameRingPlaneBytes) /
                           downwellBytesPerPlaneLine] & (1u << buffer)) &&
            gDownwellGameplayGame[byte] != memory.game()[byte])
            ++gDownwellGameBufferFailures;
    gDownwellGameplayHud = memory.hud();
    gDownwellGameplayPlayerSprite = memory.playerSprite(buffer);
    gDownwellGameplayActorSprites = memory.bulletSprite() +
        buffer * gameplayRegularSpriteCount *
            (bulletSpriteBytes / sizeof(UWORD));
    gDownwellGameplayOverflowSprite = memory.enemySprite(buffer);
    gDownwellGameplayRingOrigin = (UWORD)port.ringOrigin();
}
#endif

int main(void) {
#ifdef DOWNWELL_SCENE_VERIFY
    // The emulator debugger cannot reliably write target memory. Initialize
    // the canary on the CPU, below this frame with room for this loop itself.
    ULONG currentStack;
    __asm__ volatile("move.l %%sp,%0" : "=r"(currentStack));
    volatile ULONG* stackGuard = (volatile ULONG*)SysBase->ThisTask->tc_SPLower;
    if (currentStack > (ULONG)stackGuard + 512) {
        for (unsigned word = 0; word < 64; ++word)
            stackGuard[word] = 0x57ac6a4d;
        gDownwellStackGuardReady = 1;
    }
#endif
    AmigaPlatform platform;
    AmigaMemory memory;
    AmigaRenderer renderer;
    unsigned front = 0;
    int worldViewX = mainViewX;
    int worldViewY = mainInitialViewY;
    int cachedWorldFirst = worldViewY;
    int cachedWorldEnd = worldViewY + downwellDisplayHeight;
    int stagedViewX = -10000, stagedFirst = 0, stagedEnd = 0;
#ifdef DOWNWELL_REPLAY_VERIFY
    AmigaPortRuntime& port = *new (portRuntimeStorage)
        AmigaPortRuntime(initialRingOrigin);
#else
    AmigaPortRuntime& port = *new (portRuntimeStorage)
        AmigaPortRuntime(initialRingOrigin, false, 1201 - 176, true);
#endif
#ifndef DOWNWELL_REPLAY_VERIFY
    // Long-lived scene state must not consume the small AmigaDOS call stack.
    // Construct at entry (not before the framework startup), as for port.
    alignas(AmigaSceneRuntime) static UBYTE sceneStorage[sizeof(AmigaSceneRuntime)];
    AmigaSceneRuntime& scenes = *new (sceneStorage) AmigaSceneRuntime;
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

    if (!platform.available()) {
        gDownwellStartupFailure = 1;
        downwellStartupFailed();
        return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
    }
    UWORD* startupCopperWords =
        (UWORD*)AllocMem(4 * sizeof(UWORD), MEMF_CHIP | MEMF_CLEAR);
    if (!startupCopperWords) {
        gDownwellStartupFailure = 2;
        downwellStartupFailed();
        return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
    }
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
        gDownwellStartupFailure = 3;
        downwellStartupFailed();
        return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
    }
    const ULONG gamePackBytes =
        amigaRendererChipPackBytes(amiga_game_pack, amiga_game_pack_end);
    gDownwellStartupChipFree = AvailMem(MEMF_CHIP);
    gDownwellStartupChipLargest = AvailMem(MEMF_CHIP | MEMF_LARGEST);
    gDownwellStartupAssetBytes = gamePackBytes;
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
        gDownwellStartupFailure = 4;
        downwellStartupFailed();
        return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
    }
    gDownwellStartupChipRemaining = AvailMem(MEMF_CHIP);
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
    static AmigaTerrainFragment fragments[amigaInitialTerrainFragmentCount];
    static AmigaTerrainDecoration
        generatedDecorations[amigaTerrainDecorationCapacity];
    AmigaAreaOneTerrainBuilder liveBuilder(fragments,
        amigaInitialTerrainFragmentCount, nullptr, 0,
        generatedDecorations, amigaTerrainDecorationCapacity);
    LiveGenerationContext liveGeneration{&liveBuilder, &builders, &objects, &port};
    downwell::GameMakerRandom enemyRandom(2);
    downwell::PackedRoom mainRoom;
    const AmigaTerrainCollisionContext emptyTerrain{0, 0, &builders, 0, 0, 0};
    amigaBuildTerrainIndex(emptyTerrain, terrainIndex);
    terrainTiles.clear();
#ifndef DOWNWELL_REPLAY_VERIFY
    const int emptyBlock = 0;
    port.configureBreakables(&emptyBlock, &emptyBlock, 0);
#endif
    if (!rooms.valid() || !objects.valid() || !builders.valid() ||
#ifdef DOWNWELL_REPLAY_VERIFY
        !verificationReplay.valid() ||
#endif
        !rooms.findRoom(11, mainRoom) ||
        !amigaBuildMenuDrawList(rooms, objects, menuDrawList)) {
        platform.restore();
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
        memory.release();
        gDownwellStartupFailure = 5;
        downwellStartupFailed();
        return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
    }
#ifndef DOWNWELL_REPLAY_VERIFY
    {
        terrainCollision.builders = &builders;
        terrainCollision.rooms = &rooms;
        terrainCollision.room = &mainRoom;
        terrainCollision.fragments = fragments;
        terrainCollision.fragmentCount = 0;
        terrainCollision.runtime = &port;
        for (unsigned i = 0; i < mainRoom.instanceCount; ++i) {
            downwell::PackedRoomInstance instance;
            if (!rooms.roomInstance(mainRoom, i, instance) ||
                (instance.objectId == 60 && !amigaAddTerrainObject(
                    terrainIndex, 60, instance.x, instance.y, true))) {
                platform.restore();
                FreeMem(startupCopperWords, 4 * sizeof(UWORD));
                gDownwellStartupFailure = 6;
                downwellStartupFailed();
                return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
            }
        }
        terrainCollision.index = &terrainIndex;
        port.configureLandingResolver(amigaResolveTerrainLanding,
                                      &terrainCollision);
        port.configureSolidResolver(amigaResolveTerrainSolid,
                                    &terrainCollision);
        liveBuilder.resetInitialCheckpoint(enemyRandom);
        liveBuilder.setCreateObserver(appendGeneratedTerrain, &liveGeneration);
        port.configureEnemyRandom(enemyRandom);
        port.configureGenerationStep(generateLiveTerrain, &liveGeneration);
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
        gDownwellStartupFailure = 8;
        downwellStartupFailed();
        return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
    }
#endif
    if (!amigaBuildSplashCache(activeGamePack, activeGamePackEnd,
                               memory.sceneCache(), memory.hud())) {
        platform.restore();
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
        memory.release();
        gDownwellStartupFailure = 9;
        downwellStartupFailed();
        return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
    }

#if defined(DOWNWELL_SKIP_INTRO) && !defined(DOWNWELL_SKIP_TO_GAME)
    if (!amigaBuildStyleBorder(activeGamePack, activeGamePackEnd,
                              memory.sceneCache() + amigaStyleFrameBytes * 2)) {
        platform.restore();
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
        memory.release();
        gDownwellStartupFailure = 10;
        downwellStartupFailed();
        return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
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
    UBYTE* stagedBuffer = memory.menuForeground();
    Bitmap gameBitmap(memory.game(), downwellVisibleWidth,
                      downwellGameRingHeight, 2, false, false,
                      downwellDmaWidth);
    static_assert(sceneCacheBytes >= downwellGameRingBytes * 2,
                  "Completed intro cache must hold two gameplay rings");
    Bitmap gameDisplay[2] = {
        Bitmap(memory.sceneCache(), downwellVisibleWidth,
               downwellGameRingHeight, 2, false, false, downwellDmaWidth),
        Bitmap(memory.sceneCache() + downwellGameRingBytes, downwellVisibleWidth,
               downwellGameRingHeight, 2, false, false, downwellDmaWidth)
    };
    bool gameListReady[2] = {false, false};
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
            amiga_game_pack, amiga_game_pack_end, 31, 0,
            downwellPlayfieldX + 80,
#ifdef DOWNWELL_REPLAY_VERIFY
            gameplayPlayerGroundY,
#else
            gameplayPlayerStartY - port.cameraOffsetY(),
#endif
            0,
            memory.playerSprite(0), playerSpriteHeight) ||
        !amigaRendererBuildHardwareSprite(
            amiga_game_pack, amiga_game_pack_end, 37, 0,
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
        gDownwellStartupFailure = 11;
        downwellStartupFailed();
        return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
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
            gameplayRegularSpriteCount, enemySprites[0], palette,
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
        gDownwellStartupFailure = 12;
        downwellStartupFailed();
        return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
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
        bulletSprites + gameplayRegularSpriteCount,
        gameplayRegularSpriteCount, enemySprites[1], palette,
        port.ringOrigin())) {
        platform.restore();
        memory.release();
        gDownwellStartupFailure = 13;
        downwellStartupFailed();
        return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
    }
#ifdef DOWNWELL_REPLAY_VERIFY
    buildReferencePicture(
        &renderer, rooms, mainRoom, builders,
        fragments, liveBuilder.fragmentCount(),
        generatedDecorations, liveBuilder.decorationCount(),
        &port, mainViewX, mainInitialViewY);
#else
    downwellStartupReady();
#endif
    int exitCode = 20; // Internal loop breaks are failures, not normal quits.
    while ((*ciaaPra & 0x40u) == 0) platform.waitVblank();
    for (;;) {
        if ((*ciaaPra & 0x40u) == 0) {
            exitCode = 0;
            break;
        }
        DW_PROFILE_FRAME((unsigned)scenes.scene());
        UBYTE packedInput;
        int scrollDelta = 0;
        bool cameraXChanged = false;
        unsigned destroyedBlockCount = 0;
        int destroyedBlockX[AmigaPortRuntime::destroyedCapacity] = {0};
        int destroyedBlockY[AmigaPortRuntime::destroyedCapacity] = {0};
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
            if (!verificationReplay.frame(port.tick(), replayInput)) {
                exitCode = 0;
                break;
            }
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
            else if (scenes.scene() == AmigaScene::gameplay) {
                liveInput.right = ((port.tick() / 120) & 1) == 0;
                liveInput.left = !liveInput.right;
                liveInput.fire = (port.tick() % 16) == 0 || (port.tick() % 16) == 3;
            }
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
                    // Retire intro DMA before reusing its cache as display rings.
                    splashBlankList.reset();
                    splashBlankList.move(bplcon0, 0);
                    for (unsigned channel = 0; channel < 8; ++channel)
                        splashBlankList.showSprite(channel, nullSprite);
                    splashBlankList.finish();
                    platform.publish(splashBlankList);
                    platform.waitVblank();
                    const unsigned hudBytes = amiga_hud_pack_end - amiga_hud_pack;
                    if (hudBytes > amigaMenuEntryPlaneBytes) break;
                    for (unsigned byte = 0; byte < hudBytes; ++byte)
                        memory.menuEntry()[byte] = amiga_hud_pack[byte];
                    activeHudPack = memory.menuEntry();
                    activeHudPackEnd = activeHudPack + hudBytes;
                    Bitmap stageClear(stagedBuffer, downwellVisibleWidth,
                        downwellGameRingHeight, 2, false, false, downwellDmaWidth);
                    stageClear.clear();
                    AmigaHardware::blitterDrain();
                    if (!terrainRasterReady)
                        prepareTerrainRaster(&renderer, rooms, mainRoom, builders,
                            fragments, liveBuilder.fragmentCount(),
                            generatedDecorations, liveBuilder.decorationCount(), &port);
                    amigaRendererSetGameOrigin(&renderer, port.ringOrigin());
                    buildReferencePicture(
                        &renderer, rooms, mainRoom, builders,
                        fragments, liveBuilder.fragmentCount(),
                        generatedDecorations, liveBuilder.decorationCount(),
                        &port, worldViewX, worldViewY);
                    if (presentedScene == AmigaScene::gameplay) {
                        // Capture the entry frame's input, not the later first
                        // simulation frame: a release between them must unlock.
                        if (port.tick() == 0)
                            port.initializeEntryInput((packedInput & 4) != 0);
                        drawFutureGameBlock(
                            &renderer, rooms, mainRoom, builders,
                            fragments, liveBuilder.fragmentCount(),
                            generatedDecorations, liveBuilder.decorationCount(),
                            &port, port.ringOrigin(), worldViewX, worldViewY);
                        cachedWorldFirst = worldViewY;
                        cachedWorldEnd = worldViewY + downwellDisplayHeight + gameplayPrefetchLines;
                        syncGameRows(gameDisplay[0], gameBitmap, 0);
                        syncGameRows(gameDisplay[1], gameBitmap, 1);
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
                    // Verification scratch is not nested/reentrant. Keep it
                    // off the 4 KiB AmigaDOS stack shared with game stepping.
                    static UWORD oracleWords[downwellCopperWords];
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
                            amiga_game_pack, amiga_game_pack_end,
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
                    syncGameRows(gameDisplay[back], gameBitmap, back);
                    if (!gameplayCopperLists[back].build(
                            gameDisplay[back], hudBitmap,
                            presentedScene == AmigaScene::gameplay
                                ? playerSprites[back] : nullSprite,
                            bulletSprites + back *
                                gameplayRegularSpriteCount,
                            gameplayRegularSpriteCount,
                            enemySprites[back], palette,
                            port.ringOrigin()))
                        break;
                    gameListReady[back] = true;
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
            DW_PROFILE_PHASE(profileSimulation);
            const AmigaPortFrame result = port.step(packedInput);
            if (!port.generationSucceeded()) break;
            DW_PROFILE_PHASE(profileTitle);
            scrollDelta = result.scrollDelta;
            cameraXChanged = result.cameraXChanged;
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
            gDownwellProfileScroll = scrollDelta;
            gDownwellProfileCameraChange = result.cameraXDelta;
#endif
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
            bool backingCopyPending = false;
            terrainCopyTarget = &gameDisplay[back];
            terrainCopyBacking = &gameBitmap;
            terrainCopyBuffer = back;
            DW_PROFILE_PHASE(profileScroll);
            worldViewY += scrollDelta;
            if (liveGeneration.dirtyFirst < liveGeneration.dirtyEnd) {
                stagedViewX = -10000;
                // Discard dirty retained rows outside the current view. Visible
                // intersections are refreshed below, after any camera swap.
                if (liveGeneration.dirtyFirst < worldViewY &&
                    liveGeneration.dirtyEnd > cachedWorldFirst) {
                    const int end = liveGeneration.dirtyEnd < worldViewY
                        ? liveGeneration.dirtyEnd : worldViewY;
                    if (end > cachedWorldFirst) cachedWorldFirst = end;
                }
                const int viewEnd = worldViewY + downwellDisplayHeight;
                if (liveGeneration.dirtyEnd > viewEnd &&
                    liveGeneration.dirtyFirst < cachedWorldEnd) {
                    const int first = liveGeneration.dirtyFirst > viewEnd
                        ? liveGeneration.dirtyFirst : viewEnd;
                    if (first < cachedWorldEnd) cachedWorldEnd = first;
                }
            }
            if (cameraXChanged) {
                worldViewX = mainViewX + port.cameraOffsetX();
                if (stagedViewX == worldViewX && stagedFirst <= worldViewY &&
                    stagedEnd >= worldViewY + downwellDisplayHeight) {
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
                    ++gDownwellProfileStagedHits;
#endif
                    // The completed staged ring becomes the inactive display
                    // ring; its old storage becomes the next staging buffer.
                    // Only the private restoration backing needs a DMA copy.
                    UBYTE* retired = (UBYTE*)gameDisplay[back].data;
                    gameDisplay[back].data = stagedBuffer;
                    stagedBuffer = retired;
                    gameBitmap.copy(gameDisplay[back], terrainRasterX, 0, terrainRasterX, 0,
                        terrainRasterWords * 16, downwellGameRingHeight);
                    // Sprite selection/positioning can run while DMA restores
                    // the private backing. Drain before any backing mutation,
                    // or at the normal final synchronization boundary.
                    backingCopyPending = true;
                    dirtyGameRows(0, 0, downwellGameRingHeight);
                    const UBYTE keep = (UBYTE)~(1u << back);
                    for (unsigned row = 0; row < downwellGameRingHeight; ++row)
                        gameDirtyRows[row] &= keep;
                    cachedWorldFirst = stagedFirst;
                    cachedWorldEnd = stagedEnd;
#ifdef DOWNWELL_SCENE_VERIFY
                    AmigaHardware::blitterDrain();
                    const unsigned column = (worldViewX - mainViewX + 160) / 160;
                    for (unsigned y = 0; y < downwellDisplayHeight; ++y) {
                        const unsigned physical = (port.ringOrigin() + y) % downwellGameRingHeight;
                        for (unsigned plane = 0; plane < 2; ++plane) {
                            const UWORD* displayed = (const UWORD*)((const UBYTE*)gameDisplay[back].data +
                                plane * downwellGameRingPlaneBytes + physical * downwellBytesPerPlaneLine + terrainRasterX / 8);
                            for (unsigned word = 0; word < terrainRasterWords; ++word)
                                if (displayed[word] != terrainRaster[column][worldViewY + y][plane][word])
                                    ++gDownwellTerrainRasterFailures;
                        }
                    }
                    amigaRendererSetGameOrigin(&renderer, port.ringOrigin());
                    drawMainWorldSpan(&renderer, rooms, mainRoom, builders,
                        fragments, liveBuilder.fragmentCount(), generatedDecorations,
                        liveBuilder.decorationCount(), &port, worldViewX, worldViewY, 0, downwellDisplayHeight);
#endif
                } else {
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
                gDownwellProfileStageShortfall = worldViewY + downwellDisplayHeight - stagedEnd;
#endif
                amigaRendererSetGameOrigin(&renderer, port.ringOrigin());
                if (!terrainRasterReady)
                    amigaRendererClearGameRows(&renderer, 0, downwellDisplayHeight, 0);
                drawMainWorldSpan(&renderer, rooms, mainRoom, builders,
                    fragments, liveBuilder.fragmentCount(),
                    generatedDecorations, liveBuilder.decorationCount(),
                    &port, worldViewX, worldViewY, 0, downwellDisplayHeight);
                drawFutureGameBlock(
                    &renderer, rooms, mainRoom, builders,
                    fragments, liveBuilder.fragmentCount(),
                    generatedDecorations, liveBuilder.decorationCount(),
                    &port, port.ringOrigin(), worldViewX, worldViewY);
                cachedWorldFirst = worldViewY;
                cachedWorldEnd = worldViewY + downwellDisplayHeight + gameplayPrefetchLines;
                }
                stagedViewX = -10000;
            }
            // Keep both ends of the actual cached interval. A jumping camera
            // can reverse into retained rows without repainting either edge.
            int missingFirst = 0, missingHeight = 0;
            if (worldViewY < cachedWorldFirst) {
                missingFirst = worldViewY - 8;
                missingHeight = cachedWorldFirst - missingFirst;
                cachedWorldFirst = missingFirst;
                if (cachedWorldEnd > cachedWorldFirst + downwellGameRingHeight)
                    cachedWorldEnd = cachedWorldFirst + downwellGameRingHeight;
            } else if (worldViewY + downwellDisplayHeight > cachedWorldEnd) {
                missingFirst = cachedWorldEnd;
                missingHeight = worldViewY + downwellDisplayHeight + 8 - cachedWorldEnd;
                cachedWorldEnd += missingHeight;
                if (cachedWorldFirst < cachedWorldEnd - downwellGameRingHeight)
                    cachedWorldFirst = cachedWorldEnd - downwellGameRingHeight;
            }
            if (missingHeight > 0) {
                if (backingCopyPending) AmigaHardware::blitterDrain();
                int origin = (int)port.ringOrigin() + missingFirst - worldViewY;
                while (origin < 0) origin += downwellGameRingHeight;
                amigaRendererSetGameOrigin(&renderer, origin % downwellGameRingHeight);
                if (!terrainRasterReady)
                    amigaRendererClearGameRows(&renderer, 0, missingHeight, 0);
                drawMainWorldSpan(&renderer, rooms, mainRoom, builders,
                    fragments, liveBuilder.fragmentCount(),
                    generatedDecorations, liveBuilder.decorationCount(),
                    &port, worldViewX, missingFirst, 0, missingHeight);
            }
            if (liveGeneration.dirtyFirst < liveGeneration.dirtyEnd) {
                int top = liveGeneration.dirtyFirst - worldViewY;
                int bottom = liveGeneration.dirtyEnd - worldViewY;
                if (top < 0) top = 0;
                if (bottom > downwellDisplayHeight) bottom = downwellDisplayHeight;
                if (top < bottom) {
                    if (backingCopyPending) AmigaHardware::blitterDrain();
                    amigaRendererSetGameOrigin(&renderer, port.ringOrigin());
                    amigaRendererClearGameRows(&renderer, top, bottom - top, 0);
                    drawMainWorldSpan(&renderer, rooms, mainRoom, builders,
                        fragments, liveBuilder.fragmentCount(), generatedDecorations,
                        liveBuilder.decorationCount(), &port, worldViewX, worldViewY,
                        top, bottom - top, downwellPlayfieldX, downwellPlayfieldWidth, true);
                }
            }
            if (destroyedBlockCount != 0) {
                if (backingCopyPending) AmigaHardware::blitterDrain();
                stagedViewX = -10000;
                for (unsigned block = 0;
                     block < destroyedBlockCount; ++block) {
                    const int centerY = 176 + destroyedBlockY[block];
                    for (unsigned column = 0; column < 3; ++column)
                        for (int y = centerY - 8; y < centerY + 8; ++y)
                            if (y >= 0 && y < terrainRasterRows)
                                terrainRasterDirty[column][y] = true;
                    // Bullets can change rows retained just outside the view.
                    // Exclude those rows from the ring's valid interval so
                    // they are refreshed before the camera exposes them.
                    int below = centerY - 8;
                    if (below < worldViewY + downwellDisplayHeight)
                        below = worldViewY + downwellDisplayHeight;
                    if (centerY + 8 > below && cachedWorldEnd > below)
                        cachedWorldEnd = below;
                    int above = centerY + 8;
                    if (above > worldViewY) above = worldViewY;
                    if (centerY - 8 < above && cachedWorldFirst < above)
                        cachedWorldFirst = above;
                    int top = centerY - 8 - worldViewY;
                    int bottom = top + 16;
                    int left = downwellPlayfieldX + 240 + destroyedBlockX[block] - 8 - worldViewX;
                    int right = left + 16;
                    if (top < 0) top = 0;
                    if (bottom > downwellDisplayHeight) bottom = downwellDisplayHeight;
                    if (left < downwellPlayfieldX) left = downwellPlayfieldX;
                    if (right > downwellPlayfieldX + downwellPlayfieldWidth)
                        right = downwellPlayfieldX + downwellPlayfieldWidth;
                    if (top >= bottom || left >= right) continue;
                    amigaRendererSetGameOrigin(&renderer, port.ringOrigin());
                    amigaRendererSetClipRect(&renderer, left, top, right - left, bottom - top);
                    amigaRendererFillRect(&renderer, left, top, right - left, bottom - top, 0);
                    drawMainWorldSpan(&renderer, rooms, mainRoom, builders,
                        fragments, liveBuilder.fragmentCount(),
                        generatedDecorations, liveBuilder.decorationCount(),
                        &port, worldViewX, worldViewY, top, bottom - top, left, right - left, true);
                }
            }
            // Prepare a possible side-room view a small strip at a time in
            // retired garden memory. The actual camera jump then needs only
            // Chip-to-Chip DMA, not an entire Fast-to-Chip raster transfer.
            if (!cameraXChanged && missingHeight == 0 && destroyedBlockCount == 0 && terrainRasterReady) {
                const int playerX = port.player().offsetX();
                const int cameraX = port.cameraOffsetX();
                const int wanted = cameraX == 0
                    ? (playerX < -16 ? -160 : playerX > 16 ? 160 : 10000)
                    : (cameraX < 0 ? (playerX > -144 ? 0 : 10000)
                                   : (playerX < 144 ? 0 : 10000));
                if (wanted != 10000 && worldViewY >= 0 &&
                    worldViewY + downwellGameRingHeight <= terrainRasterRows) {
                    const int targetX = mainViewX + wanted;
                    if (targetX != stagedViewX) {
                        stagedViewX = targetX;
                        stagedFirst = stagedEnd = worldViewY;
                    }
                    int first = stagedEnd, count = worldViewY + downwellDisplayHeight + 16 - stagedEnd;
                    if (worldViewY < stagedFirst) {
                        count = stagedFirst - worldViewY;
                        if (count > 48) count = 48;
                        first = stagedFirst - count;
                    } else if (count > 48) count = 48;
                    if (count > 0) {
                        bool clean = true;
                        const unsigned column = (targetX - mainViewX + 160) / 160;
                        for (int row = first; row < first + count; ++row)
                            if (terrainRasterDirty[column][row]) clean = false;
                        if (clean) {
                            AmigaRenderer stage = renderer;
                            stage.game = stagedBuffer;
                            int origin = port.ringOrigin() + first - worldViewY;
                            while (origin < 0) origin += downwellGameRingHeight;
                            stage.gameOrigin = origin % downwellGameRingHeight;
                            transferTerrainRows(&stage, targetX, first, 0, count, false);
                            if (first < stagedFirst) stagedFirst = first;
                            if (first + count > stagedEnd) stagedEnd = first + count;
                            if (stagedEnd - stagedFirst > downwellGameRingHeight) {
                                if (first == stagedFirst) stagedEnd = stagedFirst + downwellGameRingHeight;
                                else stagedFirst = stagedEnd - downwellGameRingHeight;
                            }
                        } else stagedViewX = -10000;
                    }
                }
            }
            DW_PROFILE_PHASE(profileSprite);
            // SDL painter order: generated enemies, player, bullets, gems.
            static GameplayBob actors[1 + AmigaPortRuntime::bulletCapacity +
                AmigaPortRuntime::enemyCapacity + AmigaPortRuntime::gemCapacity];
            unsigned actorCount = 0;
#ifndef DOWNWELL_REPLAY_VERIFY
            for (unsigned index = 0; index < port.enemyCount(); ++index) {
                const auto& enemy = port.enemyAt(index);
                if (!enemy.active || !enemy.awake) continue;
                if (!appendGameplayActor(actors, actorCount,
                        (UWORD)enemy.spriteId, (UWORD)enemy.drawFrame(),
                        downwellPlayfieldX + 80 + enemy.x - port.cameraOffsetX(),
                        gameplayPlayerStartY + enemy.y - port.cameraOffsetY(),
                        enemy.objectId != 119 && enemy.facing < 0)) frameReady = false;
            }
#endif
            if (port.playerVisible() && !appendGameplayActor(actors, actorCount,
                    (UWORD)port.player().spriteId(port.dead()),
                    (UWORD)port.player().spriteFrame(port.dead()),
                    downwellPlayfieldX + 80 + downwell::roundGameMakerQ8(port.player().offsetXFixed()) - port.cameraOffsetX(),
#ifdef DOWNWELL_REPLAY_VERIFY
                    gameplayPlayerGroundY + port.player().offsetY(),
#else
                    gameplayPlayerStartY + downwell::roundGameMakerQ8(port.player().offsetYFixed()) - port.cameraOffsetY(),
#endif
                    port.player().facing() < 0)) frameReady = false;
            for (unsigned index = 0; index < AmigaPortRuntime::bulletCapacity; ++index) {
                const auto& bullet = port.bulletAt(index);
                if (bullet.active && !appendGameplayActor(actors, actorCount, 459, 0,
                        downwellPlayfieldX + 80 + bullet.x - port.cameraOffsetX(),
                        gameplayPlayerStartY + bullet.y - port.cameraOffsetY(), 0))
                    frameReady = false;
            }
#ifndef DOWNWELL_REPLAY_VERIFY
            for (unsigned index = 0; index < AmigaPortRuntime::gemCapacity; ++index) {
                const auto& gem = port.gemAt(index);
                if (gem.active && !appendGameplayActor(actors, actorCount,
                        (UWORD)(gem.large ? 134 : 133), (UWORD)gem.motion.animation.frame(),
                        downwellPlayfieldX + 80 + gem.x - port.cameraOffsetX(),
                        gameplayPlayerStartY + gem.y - port.cameraOffsetY(), 0))
                    frameReady = false;
            }
#endif
            Sprite* regularSprites = bulletSprites + back * gameplayRegularSpriteCount;
#ifdef DOWNWELL_BOB_VERIFY
            AmigaActorChannels channels(0);
#else
            AmigaActorChannels channels;
#endif
            unsigned bobCount = actorCount;
            // Assign frontmost actors first. Any remaining prefix is painted
            // back-to-front into PF1, below every allocated hardware sprite.
            while (bobCount > 0) {
                const GameplayBob& actor = actors[bobCount - 1];
                const unsigned parts = amigaRendererHardwareSpritePartCount(
                    amiga_game_pack, amiga_game_pack_end, actor.sprite, actor.frame);
                if (parts == 0) {
                    gDownwellActorFailureSprite = actor.sprite;
                    gDownwellActorFailureFrame = actor.frame;
                    frameReady = false;
                    break;
                }
                const int first = channels.take(parts);
                if (first < 0) break;
                for (unsigned part = 0; part < parts; ++part) {
                    const unsigned channel = first + part;
                    Sprite& sprite = channel == 0 ? playerSprites[back]
                        : actorSpriteAt(regularSprites, enemySprites[back], channel - 1);
                    if (!amigaRendererBuildHardwareSpritePart(
                            amiga_game_pack, amiga_game_pack_end, actor.sprite, actor.frame, part,
                            actor.x, actor.y, actor.flip, sprite.data(), enemySpriteHeight))
                        frameReady = false;
                }
                --bobCount;
            }
            for (unsigned channel = channels.used(); channel < 8; ++channel) {
                Sprite& sprite = channel == 0 ? playerSprites[back]
                    : actorSpriteAt(regularSprites, enemySprites[back], channel - 1);
                sprite.hide();
            }
            DW_PROFILE_PHASE(profileCopper);
            syncGameRows(gameDisplay[back], gameBitmap, back);
            if (bobCount)
                drawGameplayBobs(renderer, gameDisplay[back], port.ringOrigin(), actors, bobCount);
            if (frameReady &&
                !(gameListReady[back] ? gameplayCopperLists[back].update(gameDisplay[back], port.ringOrigin()) :
                  gameplayCopperLists[back].build(
                    gameDisplay[back], hudBitmap, playerSprites[back],
                    bulletSprites + back *
                        gameplayRegularSpriteCount,
                    gameplayRegularSpriteCount,
                    enemySprites[back], palette, port.ringOrigin())))
                frameReady = false;
            if (!frameReady) {
                downwellGameplayRenderFailure();
                break;
            }
            gameListReady[back] = true;
            /* AmigaPlatform makes the published list live at next VBlank. */
            DW_PROFILE_PHASE(profilePublish);
            platform.publish(gameplayCopperLists[back]);
            DW_PROFILE_PHASE(profileOther);
            front = back;
#ifdef DOWNWELL_SCENE_VERIFY
            if (port.tick() == 600) {
                // Deterministic coverage independent of enemy encounters and
                // travel distance: validate every strip of all three views.
                // Use retired staging memory, never the displayed ring.
                AmigaHardware::blitterDrain();
                AmigaRenderer probe = renderer;
                probe.game = stagedBuffer;
                probe.gameOrigin = 0;
                for (int x = mainViewX - 160; x <= mainViewX + 160; x += 160)
                    for (int strip = 0; strip < downwellDisplayHeight; strip += 16) {
                        const int count = strip + 16 <= downwellDisplayHeight
                            ? 16 : downwellDisplayHeight - strip;
                        // First materialize any newly dirty rows; the second
                        // read exercises the cache-transfer/fresh-render oracle.
                        drawMainWorldSpan(&probe, rooms, mainRoom, builders,
                            fragments, liveBuilder.fragmentCount(), generatedDecorations,
                            liveBuilder.decorationCount(), &port, x, worldViewY + strip, 0, count);
                        drawMainWorldSpan(&probe, rooms, mainRoom, builders,
                            fragments, liveBuilder.fragmentCount(), generatedDecorations,
                            liveBuilder.decorationCount(), &port, x, worldViewY + strip, 0, count);
                    }
                exposeGameplayFrame(memory, gameDisplay[back], back, port);
                downwellGameplayLongRunReady();
            }
            if (!sceneGameplayLandedReported && port.player().grounded()) {
                sceneGameplayLandedReported = true;
                sceneGameplayLandedCameraY = port.cameraOffsetY();
                exposeGameplayFrame(memory, gameDisplay[back], back, port);
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
                    exposeGameplayFrame(memory, gameDisplay[back], back, port);
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
            const auto gems = downwell::verifyGemSimulation();
            gGemReplayDigestHigh = (ULONG)(gems.digest >> 32);
            gGemReplayDigestLow = (ULONG)gems.digest;
            gGemReplayCoverage = gems.coverage;
            gReplayComplete = 1;
            downwellReplayComplete();
            for (;;) platform.waitVblank();
        }
#endif
    }

    platform.restore();
    memory.release();
    return exitCode;
}
