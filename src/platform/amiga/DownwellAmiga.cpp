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
#include "core/ShopFixtures.h"
#include "AmigaRenderer.h"
#include "AmigaSceneRuntime.h"
#include "GameplaySceneStorage.h"
#include "AmigaExitMask.h"
#include "AmigaClearPresenter.h"
#include "AmigaClearSceneAssets.h"
#include "AmigaSplashCache.h"
#include "AmigaStyleRenderer.h"
#include "AmigaShopMenu.h"
#include "AmigaTerrain.h"
#include "framework/AmigaHardware.h"
#include "core/GameMakerRounding.h"
#include "core/HudNumbers.h"
#include "core/AmmoDividers.h"
#include "core/AmmoFill.h"
#include "core/DeathResultsBackdrop.h"
#include "core/DeathResultsRows.h"
#include "core/DeathResultsChoices.h"
#include "core/DeathResultsProgressBar.h"
#include "core/DeathResultsProgressLabels.h"
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
extern const UBYTE amiga_results_pack[];
extern const UBYTE amiga_exit_mask[];
extern const UBYTE amiga_exit_mask_end[];
extern const UBYTE amiga_results_pack_end[];
extern const UBYTE amiga_shop_pack[];
extern const UBYTE amiga_shop_pack_end[];
extern const UBYTE amiga_clear_pack[];
extern const UBYTE amiga_clear_pack_end[];
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
    // The live builder is stepping on this runtime's RNG stream. Register
    // synchronously so stock/farewell draws precede the next pattern cell.
    if (command.objectId == 277)
        return live.runtime->registerShop(command.x, command.y) != nullptr;
    if (command.objectId == 306) live.runtime->registerLevelExit(command.y);
    if (!amigaAddTerrainObject(terrainIndex, command.objectId, command.x, command.y) ||
        !terrainTiles.append(command)) return false;
    if (command.objectId == 60 || command.objectId == 92 || command.objectId == 93 ||
        command.objectId == 289 || command.objectId == 293 || command.objectId == 296 ||
        command.objectId == 109) {
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
const UBYTE* gDownwellResultsPack;
volatile ULONG gDownwellResultsAssetBytes;
volatile ULONG gDownwellShopOverlayChecks;
const UBYTE* gDownwellShopPack;
volatile ULONG gDownwellShopAssetBytes;
static UBYTE* overlayAssetStorage;
static unsigned overlayAssetCapacity;
static bool exitMaskResident;
#ifdef DOWNWELL_EXIT_WIPE_VERIFY
volatile ULONG gDownwellExitWipeFrames, gDownwellExitWipeFailures;
__attribute__((noinline, used)) void downwellExitWipeReady() { __asm__ volatile("" ::: "memory"); }
#endif
static AmigaShopMenu shopMenuOverlay;
#ifdef DOWNWELL_CLEAR_VERIFY
volatile ULONG gDownwellClearFrames, gDownwellClearChoices, gDownwellClearFailures;
volatile ULONG gDownwellClearReentries, gDownwellClearGameplayUpdates;
volatile ULONG gDownwellClearPhase;
const UBYTE* gDownwellClearActualHud;
const UBYTE* gDownwellClearExpectedHud;
const UBYTE* gDownwellClearActualGame;
const UBYTE* gDownwellClearExpectedGame;
__attribute__((noinline, used)) void downwellClearProbe() { __asm__ volatile("" ::: "memory"); }
__attribute__((noinline, used)) void downwellClearReady() { __asm__ volatile("" ::: "memory"); }
#endif

// Results and shop are mutually exclusive users of the retired-garden gap.
// Drain the blitter before replacing a bank it may still be reading. Hardware
// sprites and display bitplanes never point into this immutable artwork region.
static bool selectOverlayAssetBank(bool shop) {
    if (exitMaskResident) return false; // DMA owns this bank until scene retirement.
    if ((shop && gDownwellShopPack) || (!shop && gDownwellResultsPack)) return true;
    const UBYTE* source = shop ? amiga_shop_pack : amiga_results_pack;
    const UBYTE* end = shop ? amiga_shop_pack_end : amiga_results_pack_end;
    const unsigned bytes = end - source;
    if (!overlayAssetStorage || bytes > overlayAssetCapacity ||
        amigaRendererChipPackBytes(source, end) != bytes) return false;
    AmigaHardware::blitterDrain();
    for (unsigned i = 0; i < bytes; ++i) overlayAssetStorage[i] = source[i];
    gDownwellShopPack = shop ? overlayAssetStorage : nullptr;
    gDownwellShopAssetBytes = shop ? bytes : 0;
    gDownwellResultsPack = shop ? nullptr : overlayAssetStorage;
    gDownwellResultsAssetBytes = shop ? 0 : bytes;
    return true;
}
static bool selectExitMaskBank() {
    if (exitMaskResident) return true;
    if (!overlayAssetStorage || AmigaExitMask::bytes > overlayAssetCapacity ||
        amiga_exit_mask_end - amiga_exit_mask != AmigaExitMask::bytes) return false;
    AmigaHardware::blitterDrain();
    for (unsigned i = 0; i < AmigaExitMask::bytes; ++i)
        overlayAssetStorage[i] = amiga_exit_mask[i];
    gDownwellResultsPack = gDownwellShopPack = nullptr;
    gDownwellResultsAssetBytes = gDownwellShopAssetBytes = 0;
    exitMaskResident = true;
    return true;
}
volatile ULONG gDownwellResultsAssetFailures;
volatile ULONG gDownwellResultsBackdropFailures;
volatile ULONG gDownwellResultsBackdropChecks;
}

#if defined(DOWNWELL_SCENE_VERIFY) || defined(DOWNWELL_REPLAY_VERIFY)
extern "C" { volatile ULONG gDownwellStackGuardReady; }
#endif

#ifdef DOWNWELL_REPLAY_VERIFY
#include "core/GemVerification.h"
#include "core/PlayerDeathVerification.h"
extern "C" {
volatile ULONG gReplayComplete;
volatile ULONG gReplayFrames;
volatile ULONG gReplayDigestHigh;
volatile ULONG gReplayDigestLow;
volatile ULONG gGemReplayDigestHigh;
volatile ULONG gGemReplayDigestLow;
volatile ULONG gGemReplayCoverage;
volatile ULONG gDeathReplayDigestHigh;
volatile ULONG gDeathReplayDigestLow;
volatile ULONG gDeathReplayCoverage;

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
volatile ULONG gDownwellConsumableChecks, gDownwellConsumableFailures;
volatile ULONG gDownwellCaptureTick;
volatile LONG gDownwellCapturePlayerXQ8, gDownwellCapturePlayerYQ8;
volatile LONG gDownwellCaptureViewX, gDownwellCaptureViewY;
#ifdef DOWNWELL_RESULTS_RESTART_VERIFY
volatile ULONG gDownwellResultsRestartCount;
volatile ULONG gDownwellResultsRestartMask;
__attribute__((noinline, used)) void downwellResultsRestartReady() { __asm__ volatile("" ::: "memory"); }
#endif
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

__attribute__((noinline, used)) void downwellGameplayAmmoFrameReady(void) { __asm__ volatile ("nop"); }
__attribute__((noinline, used)) void downwellGameplayRechargeFrameReady(void) { __asm__ volatile ("nop"); }
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
    downwell::drawHealthNumberLayers(x, y, health, maximum,
        [renderer](unsigned sprite, unsigned frame, int px, int py) {
            drawAsset(renderer, sprite, frame, px, py, 0);
        });
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
    if (digit < 10) {
        drawAsset(renderer, 65010, digit, x, y, 0);
    } else {
        downwell::drawAmmoNumberLayers(x, y, digit, [renderer](unsigned frame, int px, int py) {
            drawAsset(renderer, 413, frame, px, py, 0);
        });
    }
}

static void redrawAmmo(AmigaRenderer* renderer, unsigned ammo, unsigned maximumAmmo, unsigned offset = 0,
                       const downwell::AmmoGaugeRows* rows = nullptr, unsigned meterOffset = 0) {
    if (ammo > maximumAmmo)
        ammo = maximumAmmo;
    amigaRendererClearClipRect(renderer);
    // Gauge rims move during recharge; clear their complete swept rectangle.
    amigaRendererFillRect(renderer, 282, 40, 24, 216, 3);
    // The label clear overlaps the gauge's lower rim. Restore the two gauge
    // layers in the source draw order when ammo changes, not every frame.
    static unsigned oldDigits = 1;
    unsigned digits = 1;
    for (unsigned value = ammo; value >= 10; value /= 10) ++digits;
    const unsigned clearDigits = digits > oldDigits ? digits : oldDigits;
    oldDigits = digits;
    if (clearDigits == 1) {
        amigaRendererFillRect(renderer, 282, 232, 24, 24, 3);
    } else {
        amigaRendererFillRect(renderer, 282 - (static_cast<int>(clearDigits) - 1) * 4,
            231, 34 + (clearDigits - 1) * 8, 26 + clearDigits - 1, 0);
    }
    drawAsset(renderer, 416, 0, 284, 43 + meterOffset, 0);
    // The underlay stays fixed while the frame and charge rows jiggle.
    downwell::drawAmmoFill(ammo, maximumAmmo, 289,
        [renderer](int x, unsigned y, unsigned w, unsigned h) {
            amigaRendererFillRect(renderer, x, y, w, h, 1);
        });
    if (rows) rows->draw(288, 229 + 4 * meterOffset,
        [renderer](unsigned sprite, unsigned frame, int x, int y) {
            // GameMaker wraps subimages; the bottom divider has only two frames.
            drawAsset(renderer, sprite, sprite == 421 ? frame % 2 : frame, x, y, 0);
        });
    downwell::drawAmmoDividers(ammo, maximumAmmo, 1, 288, 229 + 4 * meterOffset,
        [renderer](unsigned sprite, unsigned frame, int x, unsigned y) {
            drawAsset(renderer, sprite, frame, x, y, 0);
        });
    drawAsset(renderer, 416, 1, 284, 43 + meterOffset, 0);
    drawAmmoDigit(renderer, 290 + offset, 239 + offset, ammo);
}

static void redrawHealth(AmigaRenderer* renderer, unsigned health, unsigned maximum,
                          unsigned pieces = 0, unsigned perHeart = 4) {
    amigaRendererClearClipRect(renderer);
    amigaRendererFillRect(renderer, 8, 6, 92, 28, 3);
    drawAsset(renderer, 415, 1, 8, 6, 0);
    amigaRendererFillRect(renderer, 12, 11, maximum ? (int)(health * 80 / maximum) : 0, 6, 2);
    downwell::drawHeartPieceMeter(8, 6, pieces, perHeart,
        [renderer](int x, int y, int w, int h) { amigaRendererFillRect(renderer, x, y, w, h, 1); },
        [renderer](unsigned sprite, unsigned frame, int x, int y) { drawAsset(renderer, sprite, frame, x, y, 0); });
    drawAsset(renderer, 415, 0, 8, 6, 0);
    drawHealthNumber(renderer, 52, 15, health, maximum);
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

static void composeStreak(AmigaRenderer* renderer, unsigned streak) {
    enum { streakTextBase = 65100, maximumPackedStreak = 99 };
    if (streak == 0)
        return;
    if (streak > maximumPackedStreak)
        streak = maximumPackedStreak;
    drawGameAsset(renderer, 133, 0, downwellPlayfieldX + 65, 55, 0);
    drawGameText(renderer, (UWORD)(streakTextBase + streak),
                 downwellPlayfieldX + 81, 51);
}
static void redrawStreak(AmigaRenderer* renderer, unsigned streak) {
    amigaRendererClearClipRect(renderer);
    amigaRendererFillRect(renderer, 160, 44, 64, 24, 0);
    composeStreak(renderer, streak);
}

static void composeDeathFlash(AmigaRenderer* renderer, bool visible) {
    amigaRendererClearClipRect(renderer); // selects the non-scrolling HUD layer
    amigaRendererFillRect(renderer, downwellPlayfieldX, 0, 160, 284, visible ? 1 : 0);
}

static void composeResultsBackdrop(AmigaRenderer* renderer,
                                    int offset, int viewX, int viewY,
                                    int top = 0, int bottom = 284) {
    UBYTE* saved = renderer->target;
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, top, 160, bottom - top);
    renderer->target = renderer->hud;
    amigaRendererFillRect(renderer, downwellPlayfieldX, top, 160, bottom - top, 0);
    struct Sink {
        AmigaRenderer* renderer;
        void sprite(int frame, int x, int y) {
            amigaRendererDrawAsset(renderer, gDownwellResultsPack,
                gDownwellResultsPack + gDownwellResultsAssetBytes, 733, frame,
                downwellPlayfieldX + x, y, 0);
        }
        void blackRect(int x, int y, int w, int h) {
            amigaRendererFillRect(renderer, downwellPlayfieldX + x, y, w, h, 3);
        }
        void title(int x, int y, bool black) {
            amigaRendererDrawAssetPen(renderer, gDownwellResultsPack,
                gDownwellResultsPack + gDownwellResultsAssetBytes, 65300, 0,
                downwellPlayfieldX + x, y, 0, black ? 3 : 1);
        }
    } sink{renderer};
    downwell::drawDeathResultsBackdrop(sink, offset, viewX, viewY, top, bottom);
    amigaRendererClearClipRect(renderer);
    renderer->target = saved;
}

static bool redrawResultsBackdrop(AmigaRenderer* renderer, const AmigaPortRuntime& port, bool force = false) {
    const int viewX = mainViewX + port.cameraOffsetX();
    const int viewY = mainInitialViewY + port.cameraOffsetY();
    const int offset = port.deathResults().menu.titleOffset;
    static int lastX = -1, lastY = -1, lastOffset = -1;
    const int phaseX = ((viewX % 8) + 8) % 8;
    const int phaseY = ((viewY % 8) + 8) % 8;
    if (!force && lastX == phaseX && lastY == phaseY && lastOffset == offset) return false;
    lastX = phaseX; lastY = phaseY; lastOffset = offset;
    composeResultsBackdrop(renderer, offset, viewX, viewY);
    return true;
}

static void composeResultsRows(AmigaRenderer* renderer,
        const downwell::DeathMenuSequence& menu, unsigned gems, unsigned kills, unsigned ticks, unsigned bestCombo,
        const downwell::RunStage& stage) {
    UBYTE* saved = renderer->target;
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, 0, 160, 284);
    renderer->target = renderer->hud;
    // Include the first icon's upper half and time bounce; progress starts at 148.
    amigaRendererFillRect(renderer, downwellPlayfieldX, 80, 160, 68, 3);
    struct Sink {
        AmigaRenderer* renderer;
        void asset(int id, unsigned frame, int x, int y) {
            amigaRendererDrawAsset(renderer, gDownwellResultsPack,
                gDownwellResultsPack + gDownwellResultsAssetBytes, id, frame,
                downwellPlayfieldX + x, y, 0);
        }
    } sink{renderer};
    downwell::drawDeathResultsRows(sink, menu, gems, kills, ticks, bestCombo, stage);
    amigaRendererClearClipRect(renderer);
    renderer->target = saved;
}

static void redrawResultsRows(AmigaRenderer* renderer, const AmigaPortRuntime& port, bool force) {
    static unsigned lastGems = ~0u, lastKills = ~0u, lastPhase = ~0u;
    static unsigned lastTime = ~0u;
    static unsigned lastCombo = ~0u;
    static int lastArea = -1, lastLevel = -1;
    const auto& menu = port.deathResults().menu;
    const unsigned phase = menu.titleOffset | (menu.rowOffset[1] << 3) |
        (menu.rowOffset[2] << 6) | (menu.visible[1] << 9) | (menu.visible[2] << 10) |
        (menu.rowOffset[4] << 11) | (menu.visible[4] << 14) |
        (menu.rowOffset[3] << 15) | (menu.visible[3] << 18) |
        (menu.rowOffset[0] << 19) | (menu.visible[0] << 22);
    const unsigned gems = port.gameGemTotal(), kills = port.enemiesDefeated();
    const unsigned ticks = port.runTimeTicks();
    const unsigned bestCombo = port.runCombo().state.best;
    if (!force && lastGems == gems && lastKills == kills && lastPhase == phase && lastTime == ticks &&
        lastCombo == bestCombo && lastArea == port.runStage().area && lastLevel == port.runStage().level) return;
    lastGems = gems; lastKills = kills; lastPhase = phase;
    lastTime = ticks;
    lastCombo = bestCombo;
    lastArea = port.runStage().area; lastLevel = port.runStage().level;
    composeResultsRows(renderer, menu, gems, kills, ticks, bestCombo, port.runStage());
}

static void composeResultsProgress(AmigaRenderer* renderer,
        const downwell::DeathResultsProgressBar& bar, const downwell::DeathResults& results) {
    UBYTE* saved = renderer->target;
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, 0, 160, 284);
    renderer->target = renderer->hud;
    amigaRendererFillRect(renderer, downwellPlayfieldX, 148, 160, 38, 3);
    struct Sink {
        AmigaRenderer* renderer;
        void asset(int id, unsigned frame, int x, int y) {
            amigaRendererDrawAsset(renderer, gDownwellResultsPack,
                gDownwellResultsPack + gDownwellResultsAssetBytes, id, frame,
                downwellPlayfieldX + x, y, 0);
        }
        void progressGlyph(unsigned code, int x, int y, unsigned pen) {
            amigaRendererDrawAssetPen(renderer, gDownwellResultsPack,
                gDownwellResultsPack + gDownwellResultsAssetBytes,
                code == 11 ? 65304 : 65320 + code, 0, downwellPlayfieldX + x, y, 0, pen);
        }
        void barRect(int x, int y, int w, int h, bool red) {
            amigaRendererFillRect(renderer, downwellPlayfieldX + x, y, w, h, red ? 2 : 1);
        }
    } sink{renderer};
    downwell::drawDeathResultsProgressLabels(sink, results);
    bar.draw(sink);
    amigaRendererClearClipRect(renderer);
    renderer->target = saved;
}

static void redrawResultsProgress(AmigaRenderer* renderer,
        const downwell::DeathResults& results, bool force) {
    static downwell::DeathResultsProgressBar previous;
    static std::uint64_t previousTotal = 0, previousGoal = 0;
    const auto bar = downwell::DeathResultsProgressBar::from(results);
    if (!force && bar == previous && results.frame.displayedTotal == previousTotal &&
        results.frame.nextGoal == previousGoal) return;
    previous = bar;
    previousTotal = results.frame.displayedTotal; previousGoal = results.frame.nextGoal;
    composeResultsProgress(renderer, bar, results);
}

static void composeResultsChoices(AmigaRenderer* renderer,
        const downwell::DeathMenuSequence& menu, int viewX, int viewY) {
    // SURFACE overlaps the lower dither: restore that region, not a black box.
    composeResultsBackdrop(renderer, menu.titleOffset, viewX, viewY, 186, 222);
    UBYTE* saved = renderer->target;
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, 186, 160, 36);
    renderer->target = renderer->hud;
    struct Sink {
        AmigaRenderer* renderer;
        void choice(unsigned index, int xTenths, int y, unsigned pen) {
            // Offline font-2 canvases have centered origins and eight-pixel advances.
            const int halfWidths[] = {20, 48, 28};
            amigaRendererDrawAssetPen(renderer, gDownwellResultsPack,
                gDownwellResultsPack + gDownwellResultsAssetBytes, 65301 + index, 0,
                downwellPlayfieldX + xTenths / 10 + halfWidths[index], y, 0, pen);
        }
    } sink{renderer};
    downwell::drawDeathResultsChoices(sink, menu);
    amigaRendererClearClipRect(renderer);
    renderer->target = saved;
}

static void redrawResultsChoices(AmigaRenderer* renderer, const AmigaPortRuntime& port, bool force) {
    const auto& menu = port.deathResults().menu;
    const unsigned phase = menu.cursor | (menu.cursorBounceTenths << 2) |
        (menu.bodyOffset << 6) | (menu.sequenceOver << 8) | (menu.goalPassed << 9);
    static unsigned previous = ~0u;
    if (!force && phase == previous) return;
    previous = phase;
    composeResultsChoices(renderer, menu, mainViewX + port.cameraOffsetX(),
        mainInitialViewY + port.cameraOffsetY());
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
static unsigned terrainShopArtRevision[downwell::ShopWorld::capacity];
static void invalidateShopRaster(const downwell::ShopWorld& shops) {
    for (unsigned i = 0; i < downwell::ShopWorld::capacity; ++i) {
        const auto& shop = shops.instances[i];
        if (shop.artRevision == terrainShopArtRevision[i]) continue;
        terrainShopArtRevision[i] = shop.artRevision;
        if (!shop.allocated) continue;
        int first = shop.y - 21, end = shop.y + 12;
        if (first < 0) first = 0;
        if (end > terrainRasterRows) end = terrainRasterRows;
        for (unsigned column = 0; column < 3; ++column)
            for (int y = first; y < end; ++y) terrainRasterDirty[column][y] = true;
    }
}

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
    if (runtime) invalidateShopRaster(runtime->shops());
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
    if (runtime) downwell::drawShopFixtures(runtime->shops(), [&](int sprite, int frame, int x, int y) {
        if (y + 24 >= firstWorldY && y - 24 < finalWorldY)
            drawGameAsset(renderer, sprite, frame, downwellPlayfieldX + x - viewX, y - viewY, 0);
    });
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

    redrawHealth(renderer, 4, 4);

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
extern "C" { volatile ULONG gDownwellOverlayCopperChecks, gDownwellOverlayCopperFailures; }
static bool verifyOverlayCopper(const Bitmap& game, const Bitmap& hud,
                                const Sprite& nullSprite, const Palette& palette) {
    // Isolated, never-published list: exercise real framework word patches on
    // the 68000 without redirecting active DMA or repainting a display buffer.
    static UWORD words[downwellCopperWords], before[downwellCopperWords];
    DownwellGameplayCopperList list(words, downwellCopperWords);
    Bitmap alternate(game.data, downwellVisibleWidth, downwellDisplayHeight, 2, false, false, downwellDmaWidth);
    if (!list.build(game, hud, nullSprite, nullptr, 0, nullSprite, palette, 0)) return false;
    const unsigned offset = list.moveOffset(bpl2pth);
    const unsigned length = list.size();
    for (unsigned origin = 0; origin < downwellGameRingHeight; ++origin) {
        if (!list.update(game, origin)) return false;
        for (unsigned buffer = 0; buffer < 2; ++buffer) {
            for (unsigned i = 0; i < length; ++i) before[i] = words[i];
            const Bitmap& target = buffer ? hud : alternate;
            if (!list.updateOverlay(target) || list.size() != length) return false;
            for (unsigned plane = 0; plane < 2; ++plane) {
                const unsigned at = offset + plane * 4;
                const ULONG wanted = (ULONG)target.data + downwellFetchCropBytes + plane * downwellPlaneBytes;
                if (words[at] != bpl2pth + plane * 8 || words[at + 2] != words[at] + 2 ||
                    (((ULONG)words[at + 1] << 16) | words[at + 3]) != wanted) return false;
            }
            for (unsigned i = 0; i < length; ++i) {
                if (i == offset + 1 || i == offset + 3 || i == offset + 5 || i == offset + 7) continue;
                if (words[i] != before[i]) return false;
            }
            ++gDownwellOverlayCopperChecks;
        }
    }
    for (unsigned i = 0; i < length; ++i) before[i] = words[i];
    Bitmap invalid(game.data, downwellVisibleWidth, downwellGameRingHeight, 2, false, false, downwellDmaWidth);
    if (list.updateOverlay(invalid) || list.update(game, downwellGameRingHeight)) return false;
    for (unsigned i = 0; i < length; ++i) if (words[i] != before[i]) return false;
    return true;
}

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
#if defined(DOWNWELL_SCENE_VERIFY) || defined(DOWNWELL_REPLAY_VERIFY)
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
    unsigned oldShopArt[downwell::ShopWorld::capacity] = {};
#ifdef DOWNWELL_REPLAY_VERIFY
    AmigaPortRuntime& port = *new (portRuntimeStorage)
        AmigaPortRuntime(initialRingOrigin);
#else
    AmigaPortRuntime& port = *new (portRuntimeStorage)
        AmigaPortRuntime(initialRingOrigin, false, 1201 - 176, true);
#ifdef DOWNWELL_SCENE_VERIFY
    // Exercise in-place recreation on the real 68000 before binding terrain.
    // Finish at the ordinary entry state so the complete scene oracle applies.
    if (port.restartFromResults(downwell::DeathMenuSequence::Action::StyleSelect, 42,
            initialRingOrigin) != downwell::ResultsDestination::styleSelect ||
        port.runStage().area != 0 ||
        port.restartFromResults(downwell::DeathMenuSequence::Action::Retry, 2,
            initialRingOrigin) != downwell::ResultsDestination::gameplay ||
        port.health() != 4 || port.runStage().area != 1 || port.tick() != 0)
        return 20;
#endif
#endif
#ifndef DOWNWELL_REPLAY_VERIFY
    // Long-lived scene state must not consume the small AmigaDOS call stack.
    // Construct at entry (not before the framework startup), as for port.
    alignas(AmigaSceneRuntime) static UBYTE sceneStorage[sizeof(AmigaSceneRuntime)];
    AmigaSceneRuntime& scenes = *new (sceneStorage) AmigaSceneRuntime;
    struct ClearState {
        AmigaClearPresenter presenter;
        AmigaClearSceneAssets assets;
        unsigned offerCount = 0, offers[4] = {};
        unsigned char owned[23] = {};
        bool presented = false, left = false, right = false, heartGoal = false;
    };
    alignas(ClearState) static UBYTE clearStorage[sizeof(ClearState)];
    auto& clear = *new (clearStorage) ClearState;
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
    bool sceneResetRequested = false;
    bool newRunWorldPending = false;
    std::uint32_t newRunSeed = 2;
    bool resultsWasActive = false;
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
    alignas(AmigaAreaOneTerrainBuilder) static UBYTE liveBuilderStorage[sizeof(AmigaAreaOneTerrainBuilder)];
    auto& liveBuilder = *new (liveBuilderStorage) AmigaAreaOneTerrainBuilder(fragments,
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
#ifdef DOWNWELL_SCENE_VERIFY
    {
        Sprite empty(memory.nullSprite(), 0);
        if (!verifyOverlayCopper(gameBitmap, hudBitmap, empty, palette))
            ++gDownwellOverlayCopperFailures;
    }
#endif
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
            downwell::Input liveInput = sceneVerificationInput(
                scenes, sceneStyleFrameReported,
                sceneLockedStyleFrameReported,
                sceneGameplayLandedReported, port);
#ifdef DOWNWELL_CLEAR_VERIFY
            if (port.levelClearActive()) {
                liveInput = {};
                liveInput.right = gDownwellClearFrames == 80;
                liveInput.fire = gDownwellClearFrames >= 100;
            }
#endif
#ifdef DOWNWELL_RESULTS_RESTART_VERIFY
            // Inject only results readiness. Confirmation, reset, room changes,
            // resource reuse and subsequent gameplay take the production path.
            if (scenes.scene() == AmigaScene::gameplay && port.tick() == 12 &&
                gDownwellResultsRestartCount < 3) {
                auto& results = const_cast<downwell::DeathResults&>(port.deathResults());
                results = {};
                results.open(true, 0, 0);
                results.menu.sequenceOver = true;
                results.menu.visible[5] = true;
                results.menu.cursor = gDownwellResultsRestartCount;
                liveInput = {};
            } else if (scenes.scene() == AmigaScene::gameplay && port.tick() == 13 &&
                       gDownwellResultsRestartCount < 3) {
                // Present the overlay for a full frame before confirmation.
                liveInput = {};
                liveInput.fire = true;
            }
#endif
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
            if (!sceneResetRequested) scenes.step(liveInput);
            DW_PROFILE_PHASE(profileOther);
            {
                const bool presentationChanged =
                    sceneResetRequested ||
                    scenes.scene() != presentedScene ||
                    scenes.splashStage() != presentedSplash;
                sceneResetRequested = false;
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
                if (presentationChanged && priorPresentedScene == AmigaScene::gameplay &&
                    presentedScene != AmigaScene::gameplay) {
                    // Gameplay reused the intro storage. Retire all DMA before
                    // rebuilding either destination; never draw over a live ring.
                    splashBlankList.reset();
                    splashBlankList.move(bplcon0, 0);
                    for (unsigned channel = 0; channel < 8; ++channel)
                        splashBlankList.showSprite(channel, nullSprite);
                    splashBlankList.finish();
                    platform.publish(splashBlankList);
                    platform.waitVblank();
                    AmigaHardware::blitterDrain();
                    if (presentedScene == AmigaScene::menu &&
                        !amigaRenderMenuCache(menuDrawList, activeGamePack, activeGamePackEnd,
                            memory.menuForeground(), memory.menuBackground(), memory.menuEntry()))
                        break;
                    renderedMenuTitleFrame = -1;
                    menuCopperMode[0] = menuCopperMode[1] = -1;
                    gameListReady[0] = gameListReady[1] = false;
                }
                if (presentationChanged && presentedScene == AmigaScene::styleSelect) {
                    // These framebuffers and copper slots may contain gameplay,
                    // even when the new selection matches the last visit exactly.
                    styleFrameState[0] = {};
                    styleFrameState[1] = {};
                    styleListBuilt[0] = styleListBuilt[1] = false;
                    if (!amigaBuildStyleBorder(activeGamePack, activeGamePackEnd,
                            memory.sceneCache() + amigaStyleFrameBytes * 2))
                        break;
                }
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
                    if (newRunWorldPending) {
                        AmigaHardware::blitterDrain();
                        amigaBuildTerrainIndex(emptyTerrain, terrainIndex);
                        terrainTiles.clear();
                        terrainCollision.fragmentCount = 0;
                        bool roomReady = true;
                        for (unsigned i = 0; i < mainRoom.instanceCount; ++i) {
                            downwell::PackedRoomInstance instance;
                            if (!rooms.roomInstance(mainRoom, i, instance) ||
                                (instance.objectId == 60 && !amigaAddTerrainObject(
                                    terrainIndex, 60, instance.x, instance.y, true))) {
                                roomReady = false;
                                break;
                            }
                        }
                        if (!roomReady) break;
                        port.confirmFirstStage();
                        if (!port.initializeAreaOneBuilder(liveBuilder, -16, 704, scenes.styleCursor())) break;
                        liveBuilder.setCreateObserver(appendGeneratedTerrain, &liveGeneration);
                        liveGeneration.dirtyFirst = terrainRasterRows;
                        liveGeneration.dirtyEnd = 0;
                        port.configureGenerationStep(generateLiveTerrain, &liveGeneration);
                        port.configureLandingResolver(amigaResolveTerrainLanding, &terrainCollision);
                        port.configureSolidResolver(amigaResolveTerrainSolid, &terrainCollision);
                        terrainRasterReady = false;
                        terrainCopyTarget = nullptr;
                        terrainCopyBacking = nullptr;
                        stagedBuffer = memory.menuForeground();
                        for (unsigned buffer = 0; buffer < 2; ++buffer) {
                            gameDisplay[buffer].data = memory.sceneCache() + buffer * downwellGameRingBytes;
                            gameRingInitialized[buffer] = false;
                            gameRowsPending[buffer] = false;
                            gameDirtyFirst[buffer] = downwellGameRingHeight;
                            gameDirtyEnd[buffer] = 0;
                            gameListReady[buffer] = false;
                        }
                        for (unsigned row = 0; row < downwellGameRingHeight; ++row) gameDirtyRows[row] = 3;
                        worldViewX = mainViewX + port.cameraOffsetX();
                        worldViewY = mainInitialViewY + port.cameraOffsetY();
                        stagedViewX = -10000;
                        stagedFirst = stagedEnd = 0;
                        newRunWorldPending = false;
                    }
                    const unsigned hudBytes = amiga_hud_pack_end - amiga_hud_pack;
                    const auto storage = GameplaySceneStorage::partition(
                        menuPlaneBytes * 2 + amigaMenuEntryPlaneBytes,
                        downwellGameRingBytes, hudBytes);
                    const unsigned resultsBytes = amiga_results_pack_end - amiga_results_pack;
                    if (!storage.valid || resultsBytes > storage.resultsCapacity ||
                        amigaRendererChipPackBytes(amiga_results_pack, amiga_results_pack_end) != resultsBytes) {
                        ++gDownwellResultsAssetFailures;
                        exitCode = 20;
                        break;
                    }
                    UBYTE* hudStorage = memory.menuForeground() + storage.hudOffset;
                    for (unsigned byte = 0; byte < hudBytes; ++byte)
                        hudStorage[byte] = amiga_hud_pack[byte];
                    activeHudPack = hudStorage;
                    activeHudPackEnd = activeHudPack + hudBytes;
                    overlayAssetStorage = memory.menuForeground() + storage.resultsOffset;
                    overlayAssetCapacity = storage.resultsCapacity;
                    exitMaskResident = false;
                    // Retired intro DMA is blank. Copy immutable, already planar
                    // assets once; never convert or copy them per results frame.
                    gDownwellResultsPack = gDownwellShopPack = nullptr;
                    shopMenuOverlay.reset();
                    if (!selectOverlayAssetBank(false)) {
                        ++gDownwellResultsAssetFailures;
                        exitCode = 20;
                        break;
                    }
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
            if (port.levelResultRequested() || port.levelClearActive()) {
                const bool entering = !clear.presented;
                if (entering) {
#ifdef DOWNWELL_CLEAR_VERIFY
                    gDownwellClearPhase = 1; downwellClearProbe();
#endif
                    // A staging ring can itself be displayed from garden RAM.
                    // Retire every DMA consumer before repartitioning it.
                    splashBlankList.reset();
                    splashBlankList.move(bplcon0, 0);
                    for (unsigned channel = 0; channel < 8; ++channel)
                        splashBlankList.showSprite(channel, nullSprite);
                    splashBlankList.finish();
                    platform.publish(splashBlankList);
                    platform.waitVblank();
                    AmigaHardware::blitterDrain();
                    if (!port.enterLevelClear(scenes.styleCursor(), clear.heartGoal) ||
                        !clear.assets.prepare(memory.menuForeground(), menuPlaneBytes * 2 + amigaMenuEntryPlaneBytes,
                            amiga_hud_pack_end - amiga_hud_pack, amiga_clear_pack, amiga_clear_pack_end - amiga_clear_pack,
                            amiga_exit_mask, amiga_exit_mask_end - amiga_exit_mask, nullptr, 0,
                            port.runStage().area, port.runStage().level)) { exitCode = 20; break; }
                    clear.presenter.reset(); clear.offerCount = 0;
                    clear.left = liveInput.left; clear.right = liveInput.right;
                    clear.presented = true;
#ifdef DOWNWELL_CLEAR_VERIFY
                    gDownwellClearPhase = 2;
#endif
                    for (unsigned i = 0; i < 23; ++i) clear.owned[i] = port.upgrades().levels[i];
                    for (unsigned buffer = 0; buffer < 2; ++buffer) {
                        gameDisplay[buffer].data = memory.sceneCache() + buffer * downwellGameRingBytes;
                        gameListReady[buffer] = false;
                    }
                } else if (!port.nextLevelRequested()) {
                    bool confirm = liveInput.fire;
                    if (!port.stepLevelClear(confirm, liveInput.left && !clear.left, liveInput.right && !clear.right,
                        clear.heartGoal, [](downwell::LevelClearSelection::Effect, unsigned) {})) { exitCode = 20; break; }
                    clear.left = liveInput.left; clear.right = liveInput.right;
                }
                if (port.nextLevelRequested() && port.runStage().area == 1 && port.runStage().level < 3) {
                    splashBlankList.reset(); splashBlankList.move(bplcon0, 0);
                    for (unsigned channel = 0; channel < 8; ++channel) splashBlankList.showSprite(channel, nullSprite);
                    splashBlankList.finish(); platform.publish(splashBlankList); platform.waitVblank();
                    AmigaHardware::blitterDrain();
                    if (!port.prepareNextAreaOneLevel(liveInput, initialRingOrigin)) { exitCode = 20; break; }
                    clear.presented = false;
                    newRunWorldPending = true; sceneResetRequested = true;
#ifdef DOWNWELL_CLEAR_VERIFY
                    ++gDownwellClearReentries;
                    gDownwellClearGameplayUpdates = 0;
#endif
                    continue;
                }
                const auto& room = port.levelClear();
                const auto& snapshot = port.levelClearFrame();
                const unsigned count = snapshot.choices ? room.selection.lastIndex + 1 : 0;
                bool bankChanged = count != clear.offerCount;
                for (unsigned i = 0; i < count; ++i) bankChanged |= clear.offers[i] != room.offers.ids[i];
                for (unsigned i = 0; i < 23; ++i) bankChanged |= clear.owned[i] != port.upgrades().levels[i];
                if (bankChanged) {
                    AmigaHardware::blitterDrain();
                    if (!clear.assets.changeOffers(memory.menuForeground(), amiga_clear_pack, amiga_clear_pack_end - amiga_clear_pack,
                        room.offers.ids, count, port.runStage().area, port.runStage().level, port.upgrades().levels)) {
                        exitCode = 20; break;
                    }
                    clear.offerCount = count;
                    for (unsigned i = 0; i < count; ++i) clear.offers[i] = room.offers.ids[i];
                    for (unsigned i = 0; i < 23; ++i) clear.owned[i] = port.upgrades().levels[i];
                    clear.presenter.artworkChanged();
                }
                const unsigned back = front ^ 1u;
                AmigaRenderer target = renderer;
#ifdef DOWNWELL_CLEAR_VERIFY
                gDownwellClearPhase = 3;
#endif
                target.game = (UBYTE*)gameDisplay[back].data;
                target.hud = back ? memory.game() : memory.hud();
                const UBYTE* bank = memory.menuForeground() + clear.assets.layout.resultsOffset;
                AmigaClearPresenter::Frame frame;
                if (!clear.presenter.render(back, target, bank, bank + clear.assets.bankBytes,
                        room, snapshot, port.upgrades(), frame)) { exitCode = 20; break; }
                for (unsigned channel = 0; channel < 8; ++channel) {
                    Sprite& sprite = channel == 0 ? playerSprites[back] :
                        actorSpriteAt(bulletSprites + back * gameplayRegularSpriteCount, enemySprites[back], channel - 1);
                    sprite.hide();
                }
                const unsigned playerId = port.player().spriteId(), playerFrame = port.player().spriteFrame();
                const unsigned parts = amigaRendererHardwareSpritePartCount(amiga_game_pack, amiga_game_pack_end, playerId, playerFrame);
                bool ready = parts > 0 && parts <= 8;
                for (unsigned part = 0; ready && part < parts; ++part) {
                    Sprite& sprite = part == 0 ? playerSprites[back] :
                        actorSpriteAt(bulletSprites + back * gameplayRegularSpriteCount, enemySprites[back], part - 1);
                    ready = amigaRendererBuildHardwareSpritePart(amiga_game_pack, amiga_game_pack_end, playerId, playerFrame, part,
                        frame.playerX, frame.playerY, port.player().facing() < 0, sprite.data(), playerSpriteHeight) != 0;
                }
                Bitmap overlay(target.hud, downwellVisibleWidth, downwellDisplayHeight, 2, false, false, downwellDmaWidth);
                if (!ready || !(gameListReady[back] ? gameplayCopperLists[back].update(gameDisplay[back], frame.origin) :
                    gameplayCopperLists[back].build(gameDisplay[back], overlay, playerSprites[back],
                        bulletSprites + back * gameplayRegularSpriteCount, gameplayRegularSpriteCount, enemySprites[back], palette, frame.origin)) ||
                    !gameplayCopperLists[back].updateOverlay(overlay) ||
                    !gameplayCopperLists[back].updateExitMask(port.exitDither().active ?
                        memory.menuForeground() + AmigaExitMask::offset(port.exitDither().y - room.motion.viewY()) : nullptr)) {
                    exitCode = 20; break;
                }
                AmigaHardware::blitterDrain();
#ifdef DOWNWELL_CLEAR_VERIFY
                static UBYTE expectedGame[2][downwellGameRingBytes], expectedHud[2][downwellLayerBytes];
                static AmigaClearPresenter oracle;
                if (entering) oracle.reset();
                if (bankChanged) oracle.artworkChanged();
                AmigaRenderer reference;
                amigaRendererInit(&reference, expectedGame[back], expectedHud[back]);
                AmigaClearPresenter::Frame expected;
                gDownwellClearActualHud = target.hud; gDownwellClearExpectedHud = expectedHud[back];
                gDownwellClearActualGame = target.game; gDownwellClearExpectedGame = expectedGame[back];
                if (!oracle.render(back, reference, bank, bank + clear.assets.bankBytes, room, snapshot, port.upgrades(), expected) ||
                    expected.origin != frame.origin) ++gDownwellClearFailures;
                for (unsigned byte = 0; byte < downwellGameRingBytes; ++byte)
                    if (target.game[byte] != expectedGame[back][byte]) ++gDownwellClearFailures;
                for (unsigned byte = 0; byte < downwellLayerBytes; ++byte)
                    if (target.hud[byte] != expectedHud[back][byte]) ++gDownwellClearFailures;
                ++gDownwellClearFrames;
                if (snapshot.choices) ++gDownwellClearChoices;
                gDownwellClearPhase = 4;
                if (gDownwellClearFrames == 1 || gDownwellClearFrames % 60 == 0)
                    downwellClearProbe();
#endif
                gameListReady[back] = true;
                platform.publish(gameplayCopperLists[back]); front = back;
                continue;
            }
#endif
            DW_PROFILE_PHASE(profileSimulation);
            const bool wasDead = port.dead();
            const bool wasDeathFlash = port.deathSequence().flashVisible();
#ifdef DOWNWELL_EXIT_WIPE_VERIFY
            // Isolated collision fixture, not a change to production generation.
            if (port.tick() == 180) port.registerLevelExit(176 + port.player().offsetY());
#endif
#ifdef DOWNWELL_CLEAR_VERIFY
            if ((port.tick() == 180 && !gDownwellClearReentries) ||
                (gDownwellClearReentries == 1 && gDownwellClearGameplayUpdates == 60))
                port.registerLevelExit(176 + port.player().offsetY());
#endif
            for (unsigned i = 0; i < downwell::ShopWorld::capacity; ++i)
                oldShopArt[i] = port.shops().instances[i].artRevision;
            const AmigaPortFrame result = port.step(packedInput);
#ifdef DOWNWELL_CLEAR_VERIFY
            if (gDownwellClearReentries) {
                ++gDownwellClearGameplayUpdates;
                if (gDownwellClearReentries == 2 && gDownwellClearGameplayUpdates == 60) downwellClearReady();
            }
#endif
            if (!port.generationSucceeded()) break;
#ifndef DOWNWELL_REPLAY_VERIFY
            // Invalidate before selecting a precomposed side-room ring.
            invalidateShopRaster(port.shops());
            for (unsigned i = 0; i < downwell::ShopWorld::capacity; ++i)
                if (oldShopArt[i] != port.shops().instances[i].artRevision) stagedViewX = -10000;
            const auto resultsAction = port.deathResults().inputEffects.action;
            if (resultsAction != downwell::DeathMenuSequence::Action::None) {
                // An entropy boundary, separate from the gameplay RNG stream.
                // Frame timing supplies a live seed; fixtures call reset with explicit seeds.
                newRunSeed = liveFrameCount;
                const auto destination = port.restartFromResults(resultsAction, newRunSeed, initialRingOrigin);
                if (!scenes.enterResultsDestination(destination)) break;
#ifdef DOWNWELL_RESULTS_RESTART_VERIFY
                gDownwellResultsRestartMask |= 1u << (static_cast<unsigned>(resultsAction) - 1);
                ++gDownwellResultsRestartCount;
#endif
                newRunWorldPending = true;
                sceneResetRequested = true;
                resultsWasActive = false;
                continue;
            }
#endif
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
                redrawAmmo(&renderer, (unsigned)port.player().ammo(), (unsigned)port.player().ammoCapacity(), port.ammoHudOffset(), &port.ammoGauge(), port.ammoMeterOffset());
            if (result.healthChanged)
                redrawHealth(&renderer, (unsigned)port.health(), (unsigned)port.healthMaximum(),
                             port.healthReserve().pieces, port.healthReserve().piecesPerHeart);
            if (result.currencyChanged)
                redrawCurrency(&renderer, port.currency());
            if ((result.streakChanged || wasDead != port.dead()) && !port.deathResults().active)
                redrawStreak(&renderer, port.dead() ? 0 : port.gemStreak());
            if (wasDeathFlash != port.deathSequence().flashVisible()) {
                // PF2 is above the world AND hardware sprites. Only touch this
                // overlay on the two visibility edges; preserve the terrain
                // backing and both hardware-scrolled world buffers throughout.
                composeDeathFlash(&renderer, port.deathSequence().flashVisible());
            }
            const downwell::ShopWorld::Instance* activeShop = nullptr;
            for (const auto& shop : port.shops().instances)
                if (shop.present && shop.menu.active) { activeShop = &shop; break; }
            if (activeShop && !port.exitDither().active && !port.deathResults().active && !selectOverlayAssetBank(true)) {
                exitCode = 20;
                break;
            }
            if (!port.deathResults().active && !port.deathSequence().flashVisible() && gDownwellShopPack) {
                if (result.streakChanged || wasDead != port.dead()) shopMenuOverlay.invalidateStreak();
                unsigned streak = port.dead() ? 0 : port.gemStreak();
                if (!shopMenuOverlay.draw(&renderer, gDownwellShopPack,
                        gDownwellShopPack + gDownwellShopAssetBytes, activeShop,
                        mainViewX + port.cameraOffsetX(), mainInitialViewY + port.cameraOffsetY(),
                        port.currency(), [](AmigaRenderer* target, void* context) {
                            composeStreak(target, *static_cast<unsigned*>(context));
                        }, &streak)) { exitCode = 20; break; }
            } else shopMenuOverlay.reset();
            if (port.deathResults().active && !port.exitDither().active) {
                if (!selectOverlayAssetBank(false)) { exitCode = 20; break; }
                const bool backdropChanged = redrawResultsBackdrop(&renderer, port,
#ifndef DOWNWELL_REPLAY_VERIFY
                    !resultsWasActive
#else
                    false
#endif
                );
                redrawResultsRows(&renderer, port, backdropChanged);
                redrawResultsProgress(&renderer, port.deathResults(), backdropChanged);
                redrawResultsChoices(&renderer, port, backdropChanged);
            }
#ifndef DOWNWELL_REPLAY_VERIFY
            resultsWasActive = port.deathResults().active;
#endif
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
            for (unsigned i = 0; i < downwell::ShopWorld::capacity; ++i) {
                const auto& shop = port.shops().instances[i];
                if (!shop.allocated || shop.artRevision == oldShopArt[i]) continue;
                if (backingCopyPending) AmigaHardware::blitterDrain();
                stagedViewX = -10000;
                const int first = shop.y - 21, end = shop.y + 12;
                int below = first > worldViewY + downwellDisplayHeight ? first : worldViewY + downwellDisplayHeight;
                if (end > below && cachedWorldEnd > below) cachedWorldEnd = below;
                int above = end < worldViewY ? end : worldViewY;
                if (first < above && cachedWorldFirst < above) cachedWorldFirst = above;
                int top = first - worldViewY, bottom = end - worldViewY;
                if (top < 0) top = 0;
                if (bottom > downwellDisplayHeight) bottom = downwellDisplayHeight;
                if (top >= bottom) continue;
                amigaRendererSetGameOrigin(&renderer, port.ringOrigin());
                amigaRendererClearGameRows(&renderer, top, bottom - top, 0);
                drawMainWorldSpan(&renderer, rooms, mainRoom, builders,
                    fragments, liveBuilder.fragmentCount(), generatedDecorations,
                    liveBuilder.decorationCount(), &port, worldViewX, worldViewY,
                    top, bottom - top, downwellPlayfieldX, downwellPlayfieldWidth, true);
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
            // Shared current painter order: enemies, merchants, player, bullets, gems.
            static GameplayBob actors[1 + AmigaPortRuntime::bulletCapacity +
                AmigaPortRuntime::enemyCapacity + AmigaPortRuntime::gemCapacity + 2 * downwell::ShopWorld::capacity];
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
            downwell::drawShopKeepers(port.shops(), [&](int sprite, unsigned frame, int x, int y) {
                if (!appendGameplayActor(actors, actorCount, sprite, frame,
                    downwellPlayfieldX + x - worldViewX, y - worldViewY, 0)) frameReady = false;
            });
            downwell::drawShopPrompts(port.shops(), false, [&](int sprite, unsigned frame, int x, int y) {
                if (!appendGameplayActor(actors, actorCount, sprite, frame,
                    downwellPlayfieldX + x - worldViewX, y - worldViewY, 0)) frameReady = false;
            });
            if (port.playerVisible() && !appendGameplayActor(actors, actorCount,
                    (UWORD)port.player().spriteId(port.deathSequence().movementDead()),
                    (UWORD)port.player().spriteFrame(port.deathSequence().movementDead(), port.deathSequence().animation),
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
            const UBYTE* exitMask = nullptr;
            if (port.exitDither().active) {
                if (!selectExitMaskBank()) { exitCode = 20; break; }
                exitMask = overlayAssetStorage + AmigaExitMask::offset(
                    port.exitDither().y - (mainInitialViewY + port.cameraOffsetY()));
            }
            if (!gameplayCopperLists[back].updateExitMask(exitMask)) { exitCode = 20; break; }
            gameListReady[back] = true;
            /* AmigaPlatform makes the published list live at next VBlank. */
            DW_PROFILE_PHASE(profilePublish);
            platform.publish(gameplayCopperLists[back]);
#ifdef DOWNWELL_EXIT_WIPE_VERIFY
            if (exitMask) {
                ++gDownwellExitWipeFrames;
                const auto& list = gameplayCopperLists[back];
                const UWORD* words = list.data();
                if (words[list.moveOffset(0x100) + 1] != 0x6401) ++gDownwellExitWipeFailures;
                for (unsigned reg = 0xf0; reg <= 0xf4; reg += 4) {
                    const unsigned at = list.moveOffset(reg);
                    const ULONG ptr = (ULONG(words[at + 1]) << 16) | words[at + 3];
                    if (ptr != ULONG(exitMask + downwellFetchCropBytes)) ++gDownwellExitWipeFailures;
                }
                if (gDownwellExitWipeFrames == 1)
                    for (unsigned i = 0; i < AmigaExitMask::bytes; ++i)
                        if (overlayAssetStorage[i] != amiga_exit_mask[i]) ++gDownwellExitWipeFailures;
                if (exitMask == overlayAssetStorage) {
                    platform.waitVblank();
                    downwellExitWipeReady();
                }
            }
#endif
#ifdef DOWNWELL_RESULTS_RESTART_VERIFY
            if (gDownwellResultsRestartCount == 3 && port.tick() == 200) {
                platform.waitVblank();
                downwellResultsRestartReady();
            }
#endif
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
                // Verify reserved art survived staging, buffer rotations and
                // the side-room cache probes above. Runs only in this verifier.
                // Exercise the actual bank replacement even though ordinary
                // level 1-1 intentionally does not generate a shop.
                if (!selectOverlayAssetBank(true)) ++gDownwellResultsAssetFailures;
                else {
                    for (ULONG byte = 0; byte < gDownwellShopAssetBytes; ++byte)
                        if (gDownwellShopPack[byte] != amiga_shop_pack[byte])
                            ++gDownwellResultsAssetFailures;
                    const UBYTE* retained = gDownwellShopPack;
                    if (!selectOverlayAssetBank(true) || retained != gDownwellShopPack)
                        ++gDownwellResultsAssetFailures;
                }
                if (!selectOverlayAssetBank(false)) ++gDownwellResultsAssetFailures;
                if (!gDownwellResultsPack || gDownwellResultsAssetBytes !=
                        (ULONG)(amiga_results_pack_end - amiga_results_pack)) {
                    ++gDownwellResultsAssetFailures;
                } else {
                    for (ULONG byte = 0; byte < gDownwellResultsAssetBytes; ++byte)
                        if (gDownwellResultsPack[byte] != amiga_results_pack[byte])
                            ++gDownwellResultsAssetFailures;
                    for (ULONG byte = 0; byte < (ULONG)(amiga_hud_pack_end - amiga_hud_pack); ++byte)
                        if (activeHudPack[byte] != amiga_hud_pack[byte])
                            ++gDownwellResultsAssetFailures;
                }
                // Exercise the real Chip/blitter overlay against the scalar
                // compositor without changing this gameplay replay's death state.
                static UBYTE savedHud[downwellLayerBytes], expectedHud[downwellLayerBytes];
                AmigaHardware::blitterDrain();
                for (unsigned byte = 0; byte < downwellLayerBytes; ++byte)
                    savedHud[byte] = expectedHud[byte] = renderer.hud[byte];
                AmigaRenderer resultsOracle = renderer;
                resultsOracle.hud = resultsOracle.target = expectedHud;
                // An accidental clip/target switch must not write real terrain
                // in either renderer. Also require observable HUD mutations.
                ULONG gameHashBefore = 2166136261UL;
                for (unsigned byte = 0; byte < downwellGameRingPlaneBytes * 2; ++byte)
                    gameHashBefore = (gameHashBefore ^ renderer.game[byte]) * 16777619UL;
                resultsOracle.blitterEnabled = 0;
                if (!selectOverlayAssetBank(true)) ++gDownwellResultsAssetFailures;
                else {
                    static AmigaShopMenu shopActual, shopOracle;
                    shopActual.reset(); shopOracle.reset();
                    alignas(downwell::ShopWorld::Instance) static UBYTE
                        shopFixtureStorage[sizeof(downwell::ShopWorld::Instance)];
                    auto& fixture = *new (shopFixtureStorage) downwell::ShopWorld::Instance;
                    fixture.present = fixture.menu.active = true;
                    fixture.menu.menuState = 1;
                    fixture.x = 240; fixture.y = 240;
                    fixture.stock.items[0] = {100,1,300};
                    composeDeathFlash(&renderer, false);
                    for (unsigned phase = 0; phase < 7; ++phase) {
                        fixture.menu.cursorPhaseQuarters = phase < 4 ? phase * 4 : 0;
                        if (phase == 1) fixture.menu.cursorWobbleX = 5;
                        if (phase == 2) fixture.stock.items[0].remaining = 0;
                        if (phase == 3) fixture.menu.cursor = 0;
                        if (phase == 6) fixture.menu.active = false;
                        unsigned streak = phase == 4 ? 12 : phase == 5 ? 0 : 3;
                        // Actual retained overlay experiences real external HUD
                        // clears; the scalar oracle starts fresh every phase.
                        redrawStreak(&renderer, streak);
                        shopActual.invalidateStreak();
                        composeDeathFlash(&resultsOracle, false);
                        shopOracle.reset();
                        shopOracle.invalidateStreak();
                        const auto foreground = [](AmigaRenderer* target, void* context) {
                            composeStreak(target, *static_cast<unsigned*>(context));
                        };
                        if (!shopActual.draw(&renderer, gDownwellShopPack,
                                gDownwellShopPack + gDownwellShopAssetBytes, &fixture, 160, 34, 200, foreground, &streak) ||
                            !shopOracle.draw(&resultsOracle, gDownwellShopPack,
                                gDownwellShopPack + gDownwellShopAssetBytes, &fixture, 160, 34, 200, foreground, &streak))
                            ++gDownwellResultsAssetFailures;
                        AmigaHardware::blitterDrain();
                        for (unsigned byte = 0; byte < downwellLayerBytes; ++byte)
                            if (renderer.hud[byte] != expectedHud[byte]) ++gDownwellResultsAssetFailures;
                        ++gDownwellShopOverlayChecks;
                    }
                    for (unsigned byte = 0; byte < downwellLayerBytes; ++byte)
                        renderer.hud[byte] = expectedHud[byte] = savedHud[byte];
                }
                if (!selectOverlayAssetBank(false)) ++gDownwellResultsAssetFailures;
                // Verify both flash edges, not only the cleared final state.
                for (unsigned flash = 0; flash < 2; ++flash) {
                    composeDeathFlash(&renderer, flash == 0);
                    amigaRendererClearClipRect(&resultsOracle);
                    amigaRendererFillRect(&resultsOracle, downwellPlayfieldX, 0, 160, 284,
                        flash == 0 ? 1 : 0);
                    AmigaHardware::blitterDrain();
                    for (unsigned byte = 0; byte < downwellLayerBytes; ++byte)
                        if (renderer.hud[byte] != expectedHud[byte]) ++gDownwellResultsBackdropFailures;
                }
                composeResultsBackdrop(&resultsOracle, 4, 160, 34);
                composeResultsBackdrop(&renderer, 4, 160, 34);
                downwell::DeathMenuSequence rowFixture;
                for (unsigned row = 0; row < 5; ++row) rowFixture.visible[row] = true;
                rowFixture.rowOffset[1] = 3;
                rowFixture.rowOffset[2] = 2;
                composeResultsRows(&resultsOracle, rowFixture, 84, 10, 3699, 25, {2,3});
                composeResultsRows(&renderer, rowFixture, 84, 10, 3699, 25, {2,3});
                downwell::DeathResultsProgressBar barFixture;
                barFixture.visible = true; barFixture.length = 47; barFixture.separator = 24;
                downwell::DeathResults labelFixture;
                labelFixture.open(true, 500, 500);
                labelFixture.menu.visible[5] = true;
                labelFixture.frame.displayedTotal = 500;
                composeResultsProgress(&resultsOracle, barFixture, labelFixture);
                composeResultsProgress(&renderer, barFixture, labelFixture);
                rowFixture.sequenceOver = true;
                // The real HUD visits all choices before the final frame;
                // the scalar oracle draws only the final state on fresh backdrop.
                for (int cursor = 0; cursor < 3; ++cursor) {
                    rowFixture.cursor = cursor;
                    for (int bounce = 0; bounce <= 12; bounce += 4) {
                        rowFixture.cursorBounceTenths = bounce;
                        composeResultsChoices(&renderer, rowFixture, 160, 34);
                    }
                }
                rowFixture.goalPassed = true;
                composeResultsChoices(&renderer, rowFixture, 160, 34);
                rowFixture.goalPassed = false;
                rowFixture.cursor = 2;
                rowFixture.cursorBounceTenths = 12;
                composeResultsChoices(&resultsOracle, rowFixture, 160, 34);
                composeResultsChoices(&renderer, rowFixture, 160, 34);
                redrawAmmo(&resultsOracle, 12, 15);
                redrawAmmo(&renderer, 12, 15);
                AmigaHardware::blitterDrain();
                unsigned changedHudBytes = 0;
                ULONG gameHashAfter = 2166136261UL;
                for (unsigned byte = 0; byte < downwellGameRingPlaneBytes * 2; ++byte)
                    gameHashAfter = (gameHashAfter ^ renderer.game[byte]) * 16777619UL;
                if (gameHashAfter != gameHashBefore) ++gDownwellResultsBackdropFailures;
                for (unsigned byte = 0; byte < downwellLayerBytes; ++byte) {
                    if (renderer.hud[byte] != savedHud[byte]) ++changedHudBytes;
                    if (renderer.hud[byte] != expectedHud[byte]) ++gDownwellResultsBackdropFailures;
                    renderer.hud[byte] = savedHud[byte];
                }
                if (changedHudBytes < 100) ++gDownwellResultsBackdropFailures;
                ++gDownwellResultsBackdropChecks;
                // Run the same deferred consumable code on the actual 68000,
                // isolated from the 600-update world's replay and terrain.
                alignas(PlayerMotion) static UBYTE itemPlayerStorage[sizeof(PlayerMotion)];
                auto& itemPlayer = *new (itemPlayerStorage) PlayerMotion(false);
                static downwell::Consumables itemQueue;
                itemQueue = {};
                downwell::CurrencyAccount itemCurrency;
                downwell::HealthReserve itemHealth;
                int hp = 4;
                for (unsigned id = 100; id <= 105; ++id) itemQueue.grant(id, itemCurrency);
                itemQueue.grant(100, itemCurrency);
                itemQueue.beginStep(itemPlayer, hp, itemHealth);
                ++gDownwellConsumableChecks;
                if (hp != 6 || itemHealth.maximum != 6 || itemHealth.pieces != 0 ||
                    itemPlayer.ammoCapacity() != 12 || itemPlayer.ammo() != 12 ||
                    itemQueue.effects.count != 6 || itemQueue.pending[0] != 1)
                    ++gDownwellConsumableFailures;
                itemQueue.beginStep(itemPlayer, hp, itemHealth);
                ++gDownwellConsumableChecks;
                if (itemHealth.pieces != 1 || itemQueue.effects.count != 1 || itemQueue.pending[0])
                    ++gDownwellConsumableFailures;
                itemQueue.beginStep(itemPlayer, hp, itemHealth);
                ++gDownwellConsumableChecks;
                if (itemQueue.effects.count != 0) ++gDownwellConsumableFailures;
                downwellGameplayLongRunReady();
            }
            static bool ammoFrameReported = false;
            static bool rechargeFrameReported = false;
            if (!rechargeFrameReported && port.ammoMeterOffset() == 3) {
                rechargeFrameReported = true;
                exposeGameplayFrame(memory, gameDisplay[back], back, port);
                downwellGameplayRechargeFrameReady();
            }
            if (!ammoFrameReported && port.player().ammo() < port.player().ammoCapacity() && port.ammoHudOffset() == 2) {
                ammoFrameReported = true;
                exposeGameplayFrame(memory, gameDisplay[back], back, port);
                downwellGameplayAmmoFrameReady();
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
            const auto death = downwell::verifyPlayerDeath();
            gDeathReplayDigestHigh = (ULONG)(death.digest >> 32);
            gDeathReplayDigestLow = (ULONG)death.digest;
            gDeathReplayCoverage = death.coverage;
            gReplayComplete = 1;
            downwellReplayComplete();
            for (;;) platform.waitVblank();
        }
#endif
    }

    platform.restore();
    memory.release();
#ifdef DOWNWELL_CLEAR_VERIFY
    gDownwellClearPhase = 100 + exitCode; downwellClearProbe();
#endif
    return exitCode;
}
