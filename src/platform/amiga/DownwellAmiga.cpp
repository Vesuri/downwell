#include <proto/exec.h>
#ifdef DOWNWELL_FIELD_VERIFY
#include "core/GameplayFieldReentryVerification.h"
extern "C" {
volatile unsigned gPreparedObjects, gPreparedAlarms, gPreparedSprites;
volatile unsigned gDownwellFieldChecks, gDownwellFieldStage, gDownwellFieldGameSteps,
    gDownwellFieldClearSteps, gDownwellFieldFirst, gDownwellFieldLater, gDownwellFieldRetry;
__attribute__((noinline,used)) void downwellFieldReentryReady(){__asm__ volatile("" ::: "memory");}
}
#endif
#include <proto/graphics.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include "AmigaInput.h"
#include "core/SceneDescentVerification.h"
#include "AmigaProfile.h"
#include "AmigaTerrainProfile.h"
#include "DownwellCopperLists.h"
#include "AmigaMenu.h"
#include "AmigaMenuCache.h"
#include "AmigaGardenScene.h"
#include "AmigaGameplayRows.h"
#include "AmigaGameplayScene.h"
#include "AmigaGameplayGeneration.h"
#include "AmigaWallCollision.h"
#include "AmigaGameplayActors.h"
#include "AmigaGameplayDrawList.h"
#include "AmigaGameplayEffects.h"
#include "core/CurrencySmokeComposition.h"
#include "AmigaTerrainRaster.h"
#include "AmigaWallDamage.h"
#include "AmigaTerrainPainter.h"
#include "AmigaMenuScroll.h"
#include "AmigaPlayer.h"
#include "AmigaPortRuntime.h"
#include "core/ShopFixtures.h"
#include "AmigaRenderer.h"
#include "AmigaAmmoHud.h"
#include "AmigaLaserSight.h"
#include "AmigaGoalField.h"
#include "AmigaBufferCompare.h"
#include "AmigaSceneRuntime.h"
#include "GameplaySceneStorage.h"
#include "AmigaExitMask.h"
#include "AmigaEntryMask.h"
#include "AmigaClearPresenter.h"
#include "AmigaClearSceneAssets.h"
#include "AmigaUpgradeScene.h"
#include "AmigaSplashCache.h"
#include "AmigaStyleScene.h"
#include "AmigaPublisherScene.h"
#include "AmigaCreditsScene.h"
#include "AmigaControlsScene.h"
#include "AmigaShopMenu.h"
#ifdef DOWNWELL_SHOP_VERIFY
#include "AmigaShopVerification.h"
static AmigaShopVerification shopVerification;
#endif
#if defined(DOWNWELL_SHOP_VERIFY) || defined(DOWNWELL_NATURAL_VERIFY)
extern "C" {
volatile ULONG gDownwellShopLiveCoverage, gDownwellShopLiveFailures;
volatile ULONG gDownwellShopLiveFrames, gDownwellShopLivePixelFailures;
__attribute__((noinline, used)) void downwellShopLiveReady() { __asm__ volatile("" ::: "memory"); }
}
#endif
#include "AmigaTerrain.h"
#include "framework/AmigaHardware.h"
#include "core/GameMakerRounding.h"
#include "core/GameplayContactProfileInput.h"
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
#include "core/GameplayCollisionOwner.h"
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
extern const UBYTE amiga_entry_mask[];
extern const UBYTE amiga_entry_mask_end[];
extern const UBYTE amiga_selection_mask[];
extern const UBYTE amiga_selection_mask_end[];
extern const UBYTE amiga_results_pack_end[];
extern const UBYTE amiga_shop_pack[];
extern const UBYTE amiga_shop_pack_end[];
extern const UBYTE amiga_clear_pack[];
extern const UBYTE amiga_clear_pack_end[];
extern const UBYTE amiga_room_pack[];
extern const UBYTE amiga_room_pack_end[];
extern const UBYTE amiga_object_pack[];
extern const UBYTE amiga_object_pack_end[];
extern const UBYTE amiga_collision_pack[];
extern const UBYTE amiga_collision_pack_end[];
extern const UBYTE amiga_collision_masks[];
extern const UBYTE amiga_collision_masks_end[];
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

extern "C" {
volatile ULONG gDownwellLiveFragments = 0;
volatile ULONG gDownwellLiveEnemies = 0;
volatile ULONG gDownwellLiveRandomDraws = 0;
volatile ULONG gDownwellLiveGenerationFailed = 0;
}
static bool generateLiveTerrain(void*, AmigaPortRuntime&,
    downwell::GameMakerRandom&, int);

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
enum class ResidentCover {none,exit,entry};
static ResidentCover residentCover;
#ifdef DOWNWELL_ENTRY_WIPE_VERIFY
volatile ULONG gDownwellEntryWipeFrames,gDownwellEntryWipeFailures,gDownwellEntryWipeReuses,gDownwellEntryLifetimeChecks;
__attribute__((noinline, used)) void downwellEntryWipeReady(){__asm__ volatile("" ::: "memory");}
#endif
#ifdef DOWNWELL_EXIT_WIPE_VERIFY
volatile ULONG gDownwellExitWipeFrames, gDownwellExitWipeFailures;
__attribute__((noinline, used)) void downwellExitWipeReady() { __asm__ volatile("" ::: "memory"); }
#endif
static AmigaShopMenu shopMenuOverlay;
#ifdef DOWNWELL_NATURAL_VERIFY
#include "AmigaNaturalRouteVerification.h"
static AmigaNaturalRouteVerification naturalRoute;
volatile ULONG gDownwellNaturalExitTick;
volatile ULONG gDownwellNaturalFailures, gDownwellNaturalReturnFrames;
__attribute__((noinline, used)) void downwellNaturalReady() { __asm__ volatile("" ::: "memory"); }
#endif
#ifdef DOWNWELL_CLEAR_VERIFY
volatile ULONG gDownwellClearFrames, gDownwellClearChoices, gDownwellClearFailures;
volatile ULONG gDownwellClearChoiceInputFrames, gDownwellClearCursorMasks[2];
volatile ULONG gDownwellClearReentries, gDownwellClearGameplayUpdates;
volatile ULONG gDownwellClearFieldFrames;
volatile ULONG gDownwellClearTick;
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
    if (residentCover!=ResidentCover::none) return false; // DMA owns this bank until scene retirement.
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
    if (residentCover==ResidentCover::exit) return true;
    if (residentCover!=ResidentCover::none) return false;
    if (!overlayAssetStorage || AmigaExitMask::bytes > overlayAssetCapacity ||
        amiga_exit_mask_end - amiga_exit_mask != AmigaExitMask::bytes) return false;
    AmigaHardware::blitterDrain();
    for (unsigned i = 0; i < AmigaExitMask::bytes; ++i)
        overlayAssetStorage[i] = amiga_exit_mask[i];
    gDownwellResultsPack = gDownwellShopPack = nullptr;
    gDownwellResultsAssetBytes = gDownwellShopAssetBytes = 0;
    residentCover = ResidentCover::exit;
    return true;
}
static bool selectEntryMaskBank(){
    if(residentCover==ResidentCover::entry)return true;
    if(residentCover!=ResidentCover::none||!overlayAssetStorage||AmigaEntryMask::bytes>overlayAssetCapacity||
       amiga_entry_mask_end-amiga_entry_mask!=AmigaEntryMask::bytes)return false;
    AmigaHardware::blitterDrain();
    for(unsigned i=0;i<AmigaEntryMask::bytes;++i)overlayAssetStorage[i]=amiga_entry_mask[i];
    gDownwellResultsPack=gDownwellShopPack=nullptr;
    gDownwellResultsAssetBytes=gDownwellShopAssetBytes=0;
    residentCover=ResidentCover::entry;return true;
}
volatile ULONG gDownwellResultsAssetFailures;
volatile ULONG gDownwellResultsBackdropFailures;
volatile ULONG gDownwellResultsBackdropChecks;
}

#if defined(DOWNWELL_SCENE_VERIFY) || defined(DOWNWELL_REPLAY_VERIFY)
extern "C" { volatile ULONG gDownwellStackGuardReady; }
extern "C" { volatile ULONG gDownwellPreciseMaskChecks; }
#endif
#ifdef DOWNWELL_SCENE_VERIFY
extern "C" { volatile ULONG gDownwellSceneStackFailureStage; }
extern "C" __attribute__((noinline,used)) void downwellSceneStackFailed(){
    __asm__ volatile("" ::: "memory");
}
__attribute__((noinline)) static void checkSceneStack(ULONG stage){
    if(gDownwellSceneStackFailureStage||!gDownwellStackGuardReady)return;
    const volatile ULONG* guard=(const volatile ULONG*)SysBase->ThisTask->tc_SPLower;
    for(unsigned i=0;i<64;++i)if(guard[i]!=0x57ac6a4d){
        gDownwellSceneStackFailureStage=stage;downwellSceneStackFailed();return;
    }
}
#else
static inline void checkSceneStack(ULONG){}
#endif

#ifdef DOWNWELL_REPLAY_VERIFY
#include "AmigaMemoryCopyVerification.h"
#include "AmigaCoverageWordVerification.h"
#include <cstring>
#include "core/GemVerification.h"
#include "core/MotionBinary64Verification.h"
#include "core/SmallUnsignedRemainderVerification.h"
#include "core/PlayerDeathVerification.h"
#include "core/MachineGunVerification.h"
#include "core/RunnerSpatialVerification.h"
#include "core/CorpseVerification.h"
#include "core/DamageNumberVerification.h"
#include "core/PlayerPreciseVerification.h"
#include "core/ReplayFrameTrace.h"
extern "C" {
#ifdef DOWNWELL_REPLAY_FRAME_TRACE
volatile std::uint32_t gReplayTrace[downwell::ReplayFrameTrace::capacity][downwell::ReplayFrameTrace::words];
#endif
volatile ULONG gReplayComplete;
volatile unsigned gMemoryCopyVerifiedCases;
volatile unsigned gCoverageWordVerifiedCases;
volatile ULONG gReplayFrames;
volatile ULONG gReplayDigestHigh;
volatile ULONG gReplayDigestLow;
volatile ULONG gGemReplayDigestHigh;
volatile ULONG gGemReplayDigestLow;
volatile ULONG gGemReplayCoverage;
volatile ULONG gDeathReplayDigestHigh;
volatile ULONG gDeathReplayDigestLow;
volatile ULONG gDeathReplayCoverage;
volatile ULONG gWeaponReplayDigestHigh;
volatile ULONG gWeaponReplayDigestLow;
volatile ULONG gWeaponReplayCoverage;
volatile ULONG gMotionReplayDigest, gMotionReplayUpdates, gMotionReplayFailures;
volatile ULONG gSmallRemainderFailures;
volatile ULONG gMotionBenchDigest, gMotionBenchFailures, gMotionBenchFields;
volatile ULONG gBoneReplayDigest, gBoneReplayUpdates, gBoneReplayFailures;
volatile ULONG gBoneBenchDigest, gBoneBenchFailures, gBoneBenchFields;
volatile ULONG gBoneCreateDigest, gBoneCreateCount, gBoneCreateFailures;
volatile ULONG gBoneAnimationDigest, gBoneAnimationCount, gBoneAnimationFailures;
volatile ULONG gSpatialChecks, gSpatialDigest, gSpatialFailures;
volatile ULONG gCorpseChecks, gCorpseDigest, gCorpseFailures;
volatile ULONG gDamageNumberChecks, gDamageNumberDigest, gDamageNumberFailures;
volatile ULONG gPlayerPreciseChecks, gPlayerPreciseDigest, gPlayerPreciseFailures;
volatile ULONG gReplayStackFailureStage;

// Identify the first workload that consumes the guard without resetting it.
// Stage 1 includes startup/gameplay; subsequent stages follow the checks below.
__attribute__((noinline)) void downwellReplayCheckStack(ULONG stage) {
    if (gReplayStackFailureStage || !gDownwellStackGuardReady) return;
    const volatile ULONG* guard = (const volatile ULONG*)SysBase->ThisTask->tc_SPLower;
    for (unsigned word = 0; word < 64; ++word) {
        if (guard[word] != 0x57ac6a4d) {
            gReplayStackFailureStage = stage;
            return;
        }
    }
}

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
volatile LONG gDownwellTerrainFirstMismatch[8];
volatile ULONG gDownwellColdStageRows;
volatile ULONG gDownwellSplashCheckedFrames[7];
volatile ULONG gDownwellSplashPublicationFailures;
volatile const UWORD* gDownwellMenuSpriteOracle;
volatile ULONG gDownwellMenuCopperPatchFailures;
volatile ULONG gDownwellConsumableChecks, gDownwellConsumableFailures;
#ifdef DOWNWELL_CAMERA_VERIFY
volatile ULONG gDownwellCameraColumns, gDownwellCameraTransitions;
volatile ULONG gDownwellCameraStaged, gDownwellCameraFallback;
volatile ULONG gDownwellCameraFrameChecks;
volatile ULONG gDownwellCameraLocks, gDownwellCameraReleases;
#endif
volatile ULONG gDownwellCaptureTick;
#ifdef DOWNWELL_GEMHIGH_VERIFY
volatile ULONG gDownwellGemHighCapture = 1;
#else
volatile ULONG gDownwellGemHighCapture;
#endif
volatile ULONG gDownwellGemHighVisibleShots;
volatile ULONG gDownwellGemHighStreak;
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

__attribute__((noinline, used)) void downwellGameplayGemHighFrameReady(void) {
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
#ifdef DOWNWELL_SCENE_DESCENT_VERIFY
        input = downwell::sceneDescentVerificationInput(port.tick(),
            240L * 256 + port.player().offsetXFixed());
#else
        const unsigned movementPhase = (port.tick() / 120u) & 1u;
        input.right = movementPhase == 0;
        input.left = movementPhase != 0;
        const unsigned firePhase = port.tick() % 16u;
        input.fire = firePhase == 0 || firePhase == 3;
#endif
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

static AmigaAmmoHud ammoHud;
static void redrawAmmo(AmigaRenderer* renderer, unsigned ammo, unsigned maximumAmmo, unsigned offset = 0,
                       const downwell::AmmoGaugeRows* rows = nullptr, unsigned meterOffset = 0) {
    ammoHud.draw(renderer, activeHudPack, activeHudPackEnd, ammo, maximumAmmo, offset, rows, meterOffset);
    DW_PROFILE_PHASE(profileTitle);
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
    enum { streakTextBase = 65100 };
    if (streak == 0)
        return;
    drawGameAsset(renderer, 133, 0, downwellPlayfieldX + 65, 55, 0);
    downwell::drawGemmyNumberLayers(downwellPlayfieldX + 81, 51, streak,
        [&](unsigned digit, int x, int y, downwell::Color color) {
            const unsigned pen = color.r == 0 ? 3 : color.g == 0 ? 2 : 1;
            amigaRendererDrawAssetPen(renderer, activeGamePack, activeGamePackEnd,
                streakTextBase + digit, 0, x, y, 0, pen);
        });
}
static void redrawStreak(AmigaRenderer* renderer, unsigned streak) {
    amigaRendererClearClipRect(renderer);
    amigaRendererFillRect(renderer, 148, 44, 88, 24, 0);
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



// Each display ring independently owes modified rows of the private backing.
static AmigaGameplayRows gameplayRows;
static AmigaLaserSight gameplayRays[2];
static AmigaTimeFieldPair gameplayFields;
static Bitmap* terrainCopyTarget;
static const Bitmap* terrainCopyBacking;
static unsigned terrainCopyBuffer;

// Three camera columns (left room, well, right room), word-aligned around
// logical x=110..269. The level raster is Fast RAM; only exposed rows enter
// the Chip-RAM rings. Keep the original compositor as the cache oracle.
enum { terrainRasterRows = 8192, terrainRasterWords = AmigaGameplayRows::terrainWords,
       terrainRasterX = AmigaGameplayRows::terrainX };
static AmigaTerrainRaster terrainRaster{};
static AmigaWallDamage<> wallDamage;
static AmigaWallCollision wallCollision{terrainIndex};
static void invalidateShopRaster(const downwell::ShopWorld& shops) {
    terrainRaster.invalidateShops(shops);
}

static bool generateLiveTerrain(void* opaque, AmigaPortRuntime& runtime,
    downwell::GameMakerRandom& random, int bottom) {
    auto& live = *static_cast<AmigaGameplayGeneration*>(opaque);
    const bool ok = live.step(random, bottom);
    gDownwellLiveFragments = live.fragmentCount();
    gDownwellLiveEnemies = runtime.enemyCount();
    gDownwellLiveRandomDraws = static_cast<ULONG>(random.drawCount());
    if (!ok) gDownwellLiveGenerationFailed = 1;
    terrainRaster.invalidate(live.dirtyFirst, live.dirtyEnd);
    return ok;
}
static void transferTerrainRows(AmigaRenderer* renderer, int viewX,
                               int worldY, int screenY, int height, bool store) {
    terrainRaster.transfer(renderer, (viewX - mainViewX + 160) / 160,
        worldY, screenY, height, store, terrainCopyTarget, terrainCopyBacking,
        terrainCopyBuffer, gameplayRows);
}

static void dirtyGameRows(unsigned origin, unsigned y, unsigned height, UBYTE buffers = 3) {
    gameplayRows.damage(origin, y, height, buffers);
}

static void syncGameRows(Bitmap& destination, const Bitmap& backing, unsigned buffer) {
    gameplayRows.sync(destination, backing, buffer);
}


static bool terrainPaintSucceeded=true;
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
    bool refresh = false, unsigned refreshCause = 0) {
    if(!terrainPaintSucceeded)return;
    const int firstWorldY = viewY + clipY;
    const int finalWorldY = firstWorldY + clipHeight;
    const bool forcedRefresh = refresh;
    if (runtime) invalidateShopRaster(runtime->shops());
    dirtyGameRows(renderer->gameOrigin, clipY, clipHeight);
    const bool cacheable = viewX >= mainViewX - 160 && viewX <= mainViewX + 160 &&
        (viewX - mainViewX) % 160 == 0 && firstWorldY >= 0 &&
        finalWorldY <= terrainRasterRows;
    if (terrainRaster.ready && cacheable && !refresh) {
        const int column = (viewX - mainViewX + 160) / 160;
        // A dirty strip must not force reconstruction of clean retained rows.
        // Each recursive call is homogeneous, so splitting adds one stack
        // level only (the verification oracle may add its existing level).
        const unsigned firstEnd=terrainRaster.runEnd(column,firstWorldY,finalWorldY);
        if(firstEnd<unsigned(finalWorldY)){
            unsigned first=firstWorldY,end=firstEnd;
            do {
                drawMainWorldSpan(renderer,rooms,room,builders,fragments,fragmentCount,
                    decorations,decorationCount,runtime,viewX,viewY,int(first)-viewY,int(end-first),
                    clipX,clipWidth,false,refreshCause);
                first=end;
                end=terrainRaster.runEnd(column,first,finalWorldY);
            }while(first<unsigned(finalWorldY));
            return;
        }
        refresh = firstWorldY < finalWorldY && terrainRaster.dirty(column, firstWorldY);
        if (!refresh) {
            downwellProfileTerrainSpan(viewX, firstWorldY, clipHeight, clipX, clipWidth, 0, refreshCause);
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
            terrainRaster.ready = false;
            amigaRendererClearGameRows(renderer, clipY, clipHeight, 0);
            drawMainWorldSpan(renderer, rooms, room, builders, fragments, fragmentCount,
                decorations, decorationCount, runtime, viewX, viewY, clipY, clipHeight, clipX, clipWidth);
            AmigaHardware::blitterDrain();
            terrainRaster.ready = true;
            for (int y = 0; y < clipHeight; ++y)
                for (unsigned plane = 0; plane < 2; ++plane) {
                    const unsigned physical = (renderer->gameOrigin + clipY + y) % downwellGameRingHeight;
                    const UWORD* chip = (const UWORD*)(renderer->game + plane * downwellGameRingPlaneBytes +
                        physical * downwellBytesPerPlaneLine + terrainRasterX / 8);
                    for (unsigned word = 0; word < terrainRasterWords; ++word)
                        if (expected[y][plane][word] != chip[word]) {
                            if (!gDownwellTerrainRasterFailures) {
                                const LONG details[] = {viewX, firstWorldY, clipHeight, y,
                                    LONG(plane), LONG(word), expected[y][plane][word], chip[word]};
                                for (unsigned i=0;i<8;++i) gDownwellTerrainFirstMismatch[i]=details[i];
                            }
                            ++gDownwellTerrainRasterFailures;
                        }
                }
            ++gDownwellTerrainRasterChecks;
#endif
            return;
        }
        amigaRendererClearGameRows(renderer, clipY, clipHeight, 0);
    }
    if (terrainRaster.ready && !cacheable && !refresh)
        amigaRendererClearGameRows(renderer, clipY, clipHeight, 0);
    downwellProfileTerrainSpan(viewX, firstWorldY, clipHeight, clipX, clipWidth,
        forcedRefresh ? 2 : terrainRaster.ready && cacheable ? 1 : 3, refreshCause);
    if(!AmigaTerrainPainter::paint(renderer, rooms, room, terrainCollision.index,
        terrainTiles, decorations, decorationCount, runtime, activeGamePack, activeGamePackEnd,
        viewX, viewY, clipX, clipY, clipWidth, clipHeight)){
        terrainPaintSucceeded=false;return;
    }
    if (terrainRaster.ready && cacheable && refresh) {
        AmigaHardware::blitterDrain();
        transferTerrainRows(renderer, viewX, firstWorldY, clipY, clipHeight, true);
        const int column = (viewX - mainViewX + 160) / 160;
        for (int y = firstWorldY; y < finalWorldY; ++y)
            terrainRaster.dirty(column, y) = false;
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
    terrainRaster.ready = true;
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
    ammoHud.invalidate();
    for (auto& ray : gameplayRays) ray.discard();
    for (auto& field : gameplayFields) field.discard();
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
    // Prepare retained HUD geometry/artwork with the level's initial picture,
    // not on the first shot. Subsequent frames only update changed strips.
    if (runtime) redrawAmmo(renderer, runtime->player().ammo(), runtime->player().ammoCapacity(),
        runtime->ammoHudOffset(), &runtime->ammoGauge(), runtime->ammoMeterOffset());
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
    if (!terrainRaster.ready)
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


extern "C" __attribute__((noinline, used)) void downwellStartupFailed() {
    __asm__ volatile("" ::: "memory");
}
extern "C" __attribute__((noinline)) void downwellGameplayRenderFailure() {
    __asm__ volatile("" ::: "memory");
}


#ifdef DOWNWELL_BOB_VERIFY
extern "C" __attribute__((noinline)) void downwellGameplayBobsReady() {
    __asm__ volatile("" ::: "memory");
}
#endif

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
    alignas(2) static UBYTE expected[downwellGameRingBytes];
    for (unsigned byte = 0; byte < downwellGameRingBytes; ++byte) expected[byte] = memory.game()[byte];
    static downwell::TimeFieldOutlines fieldOracle;
    fieldOracle.set(port.timeFields(), port.goalField(), port.levelExitPresent(),
                    port.goalFieldBirthBoundary(), 160 + port.cameraOffsetX(),
                    34 + port.cameraOffsetY(), downwellPlayfieldX,port.startTimeField());
    if (fieldOracle.visible) {
        AmigaRenderer oracle;
        amigaRendererInit(&oracle, expected, nullptr);
        amigaRendererSetGameOrigin(&oracle, port.ringOrigin());
        amigaRendererSetClipRect(&oracle, downwellPlayfieldX, 0, downwellPlayfieldWidth, downwellDisplayHeight);
        fieldOracle.draw([&](int x, int y, int pen) { amigaRendererFillRect(&oracle, x, y, 1, 1, pen); });
    }
    if (const auto* line = gameplayRays[buffer].line()) {
        AmigaRenderer oracle;
        amigaRendererInit(&oracle, expected, nullptr);
        amigaRendererSetGameOrigin(&oracle, gameplayRays[buffer].origin());
        amigaRendererSetClipRect(&oracle, downwellPlayfieldX, 0, downwellPlayfieldWidth, downwellDisplayHeight);
        line->draw([&](int x, int y) { amigaRendererFillRect(&oracle, x, y, 1, 1, 2); });
    }
    // Test damage once per row, not two software divisions per plane byte.
    // Dirty rows include deliberately drawn BOBs, unlike pristine backing.
    for (unsigned row = 0; row < downwellGameRingHeight; ++row) {
        if (gameplayRows.dirty(row, buffer)) continue;
        for (unsigned plane = 0; plane < 2; ++plane) {
            const unsigned offset = plane * downwellGameRingPlaneBytes + row * downwellBytesPerPlaneLine;
            gDownwellGameBufferFailures += amigaBufferDifferences(
                static_cast<const UBYTE*>(display.data) + offset, expected + offset, downwellBytesPerPlaneLine);
        }
    }
    gDownwellGameplayHud = memory.hud();
    gDownwellGameplayPlayerSprite = memory.playerSprite(buffer);
    gDownwellGameplayActorSprites = memory.bulletSprite() +
        buffer * gameplayRegularSpriteCount *
            (bulletSpriteBytes / sizeof(UWORD));
    gDownwellGameplayOverflowSprite = memory.enemySprite(buffer);
    gDownwellGameplayRingOrigin = (UWORD)port.ringOrigin();
}
#endif

// Keep the large application's frame out of the entry wrapper so a caller
// with an undersized task stack can fail before any deep initialization.
__attribute__((noinline)) static int runDownwell(void) {
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
#ifdef DOWNWELL_REPLAY_VERIFY
    gMemoryCopyVerifiedCases=verifyAmigaMemoryCopy(std::memcpy);
    if(gMemoryCopyVerifiedCases!=384)return 20;
    gCoverageWordVerifiedCases=verifyAmigaCoverageWords();
    if(gCoverageWordVerifiedCases!=80)return 20;
#endif
    AmigaPlatform platform;
    AmigaMemory memory;
    AmigaRenderer renderer;
    unsigned front = 0;
    int worldViewX = mainViewX;
    int worldViewY = mainInitialViewY;
    unsigned oldShopArt[downwell::ShopWorld::capacity] = {};
    const downwell::ObjectPack objects(amiga_object_pack,
        (unsigned long)(amiga_object_pack_end-amiga_object_pack));
    const downwell::SpriteCollisionPack collisionSprites(amiga_collision_pack,
        (unsigned long)(amiga_collision_pack_end-amiga_collision_pack));
    const downwell::SpriteMaskPack collisionMasks(amiga_collision_masks,
        (unsigned long)(amiga_collision_masks_end-amiga_collision_masks));
    const downwell::RoomPack rooms(amiga_room_pack,
        (unsigned long)(amiga_room_pack_end-amiga_room_pack));
    if(!objects.valid()||!collisionSprites.valid()||!collisionMasks.valid()||!rooms.valid())return 20;
    checkSceneStack(1);
    // Ownership precedes player construction and survives in-place run resets.
    // Keep these bounded tables off the AmigaDOS stack, like the world itself.
    alignas(downwell::GameplayCollisionOwner) static UBYTE collisionStorage[sizeof(downwell::GameplayCollisionOwner)];
    auto& collisionOwner=*new(collisionStorage) downwell::GameplayCollisionOwner(objects,collisionSprites,
        downwell::GameplayTerrainInstances::Presentation::bind(wallDamage),
        downwell::GameplayTerrainInstances::WallCollision::bind(wallCollision),&rooms);
    if(!collisionOwner.valid())return 20;
    checkSceneStack(2);
#ifdef DOWNWELL_REPLAY_VERIFY
    AmigaPortRuntime& port = *new (portRuntimeStorage)
        AmigaPortRuntime(initialRingOrigin,true,0,false,collisionOwner.enemyBinding(),
            collisionOwner.breakableBinding(),collisionOwner.playerBinding(),&collisionOwner.terrain(),&rooms);
#else
    AmigaPortRuntime& port = *new (portRuntimeStorage)
        AmigaPortRuntime(initialRingOrigin, false, 1201 - 176, true,collisionOwner.enemyBinding(),
            collisionOwner.breakableBinding(),collisionOwner.playerBinding(),&collisionOwner.terrain(),&rooms);
#endif
    if(!port.effectsSucceeded()||!collisionOwner.attach(port))return 20;
#ifdef DOWNWELL_ENTRY_WIPE_VERIFY
    {
        // Explicit-ID lifecycle fixture, not automatic production admission.
        auto& entries=collisionOwner.entryDithers();downwell::InstanceHandle key;
        auto camera=[](downwell::MotionBinary64& v){v=downwell::MotionBinary64::fromInteger(240);return true;};
        if(!entries.create(400001,camera,[&](downwell::MotionBinary64& v){
            const auto* view=port.roomEntryView();if(!view)return false;v=view->y.promoted();return true;
        },key))return 20;
        for(unsigned frame=1;frame<=120;++frame){
            port.step(0);
            if(!port.effectsSucceeded()||!collisionOwner.valid())return 20;
            const auto* r=entries.find(key);std::int32_t y;
            if(!r||!r->y.promoted().roundPixel(y)||y!=16*int(frame<120?frame:119)||
               r->effect.alarm0!=120-int(frame)||r->destroyRequested!=(frame==120))return 20;
            ++gDownwellEntryLifetimeChecks;
        }
        if(!collisionOwner.prepareDraw()||entries.size()||entries.find(key))return 20;
        // Restore the ordinary initial gameplay state before the independent
        // presentation sweep. The fixture above uses the real room scheduler.
        if(port.restartFromResults(downwell::DeathMenuSequence::Action::Retry,2,initialRingOrigin)!=
            downwell::ResultsDestination::gameplay)return 20;
    }
#endif
    checkSceneStack(3);
#ifndef DOWNWELL_REPLAY_VERIFY
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
    checkSceneStack(4);
#endif
#endif
#ifdef DOWNWELL_FIELD_VERIFY
    using FieldTape = downwell::GameplayFieldReentryVerification;
    alignas(FieldTape) static UBYTE fieldTapeStorage[sizeof(FieldTape)];
    auto& fieldTape = *new (fieldTapeStorage) FieldTape;
    const bool fieldPassed = fieldTape.run(port,collisionOwner,objects,collisionSprites,initialRingOrigin);
    gPreparedObjects=fieldTape.preparedResources.objectsChecked;
    gPreparedAlarms=fieldTape.preparedResources.alarmsChecked;
    gPreparedSprites=fieldTape.preparedResources.spritesChecked;
    gDownwellFieldChecks=fieldTape.checks;gDownwellFieldStage=fieldTape.stage;
    gDownwellFieldGameSteps=fieldTape.gameSteps;gDownwellFieldClearSteps=fieldTape.clearSteps;
    gDownwellFieldFirst=fieldTape.firstGeneration;gDownwellFieldLater=fieldTape.laterGeneration;
    gDownwellFieldRetry=fieldTape.retryGeneration;
    checkSceneStack(40);
    downwellFieldReentryReady();
    if(!fieldPassed)return 20;
#endif
#ifndef DOWNWELL_REPLAY_VERIFY
    // Long-lived scene state must not consume the small AmigaDOS call stack.
    // Construct at entry (not before the framework startup), as for port.
    alignas(AmigaSceneRuntime) static UBYTE sceneStorage[sizeof(AmigaSceneRuntime)];
    AmigaSceneRuntime& scenes = *new (sceneStorage) AmigaSceneRuntime;
    alignas(AmigaUpgradeScene) static UBYTE clearStorage[sizeof(AmigaUpgradeScene)];
    auto& clear = *new (clearStorage) AmigaUpgradeScene;
    AmigaScene presentedScene = scenes.scene();
#ifdef DOWNWELL_SKIP_TO_GAME
    // Force the normal main-room presentation setup on the first frame.
    presentedScene = AmigaScene::splash;
#endif
    int presentedSplash = scenes.splashStage();
    unsigned presentedSplashDither = scenes.splashDitherFrame();
    unsigned liveFrameCount = 0;
#ifdef DOWNWELL_SKIP_LEVEL
    bool skipLevelPending = true;
#endif
    unsigned menuSpriteFrameCount = 0;
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
    AmigaGameplayGeneration liveGeneration{liveBuilder, builders, objects, terrainIndex, terrainTiles};
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
        !verificationReplay.valid() || verificationReplay.frameCount()>downwell::ReplayFrameTrace::capacity ||
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
        if (!liveGeneration.loadRoom(port,rooms,mainRoom)) {
            platform.restore();
            FreeMem(startupCopperWords, 4 * sizeof(UWORD));
            gDownwellStartupFailure = 6;
            downwellStartupFailed();
            return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
        }
        terrainCollision.index = &terrainIndex;
#ifdef DOWNWELL_SCENE_VERIFY
        // Exercise the target's actual precise-mask route, not an occupancy
        // answer. This does not replace the legacy live collision adapters.
        // The verification callback probes resources only, not game events.
        // Keep this full heterogeneous phase workspace off the task stack.
        using MaskPhase=downwell::RunnerStepSnapshot<downwell::GameplayCollisionView::capacity>;
        alignas(MaskPhase) static UBYTE maskPhaseStorage[sizeof(MaskPhase)];
        auto& maskPhase=*new(maskPhaseStorage) MaskPhase;
        bool testedWall=false;
        const bool maskChecksSucceeded=collisionOwner.dispatchStepPhase(2,maskPhase,
            [&](downwell::InstanceHandle shared,const downwell::PackedObjectEvent& event){
            if(testedWall||event.codeId!=986)return true;
            downwell::GameplayCollisionView::Kind kind;downwell::InstanceHandle local;
            if(!downwell::GameplayCollisionView::decode(shared,kind,local)||
                kind!=downwell::GameplayCollisionView::Kind::terrain)return false;
            const auto* wall=collisionOwner.terrain().find(local);
            if(!wall||wall->objectId!=60||!wall->active)return false;
            bool hit=false;
            if(!collisionOwner.terrainPositionMeeting(60,downwell::MotionBinary64::fromInteger(wall->x),
                downwell::MotionBinary64::fromInteger(wall->y),collisionMasks,hit)||!hit)return false;
            ++gDownwellPreciseMaskChecks;testedWall=true;
            return true;
        });
        bool outside=true;
        if(!maskChecksSucceeded||!testedWall||!collisionOwner.terrainPositionMeeting(60,downwell::MotionBinary64::fromInteger(-10000),
            downwell::MotionBinary64::fromInteger(-10000),collisionMasks,outside)||outside){
            platform.restore();
            FreeMem(startupCopperWords,4*sizeof(UWORD));
            memory.release();
            gDownwellStartupFailure=6;
            downwellStartupFailed();
            return 20;
        }
        ++gDownwellPreciseMaskChecks;
#endif
        checkSceneStack(5);
        port.configureLandingResolver(amigaResolveTerrainLanding,
                                      &terrainCollision);
        port.configureSolidResolver(amigaResolveTerrainSolid,
                                    &terrainCollision, amigaResolveTerrainSegment, amigaResolveTerrainPlayerMask,amigaResolveTerrainWallContacts);
        liveBuilder.resetInitialCheckpointBeforeEntrance(enemyRandom);
        port.configureEnemyRandom(enemyRandom);
        if(!port.createRoomStartField()){
            platform.restore();
            FreeMem(startupCopperWords,4*sizeof(UWORD));
            memory.release();
            gDownwellStartupFailure=6;
            downwellStartupFailed();
            return 20;
        }
        enemyRandom=port.randomState();
#ifdef DOWNWELL_NATURAL_VERIFY
        AmigaNaturalRouteVerification::entry(liveBuilder, enemyRandom);
#endif
        liveGeneration.bind(port);
        port.configureEnemyRandom(enemyRandom);
        port.configureGenerationStep(generateLiveTerrain, &liveGeneration);
    }
#endif
    if(!amigaRendererCopyChipPack(amiga_game_pack,amiga_game_pack_end,memory.assets(),gamePackBytes)){
        platform.restore();FreeMem(startupCopperWords,4*sizeof(UWORD));memory.release();return 20;
    }
    activeGamePack = memory.assets();
    activeGamePackEnd = activeGamePack + gamePackBytes;
    static downwell::EffectMaskAssets effectAssets;
    alignas(downwell::CurrencySmokeComposition) static UBYTE effectStorage[sizeof(downwell::CurrencySmokeComposition)];
    auto& controllerEffects = *new (effectStorage) downwell::CurrencySmokeComposition;
    if (!effectAssets.bind(activeGamePack,gamePackBytes)||
        !effectAssets.bindEntryMask(amiga_entry_mask,amiga_entry_mask_end-amiga_entry_mask)) {
        platform.restore();
        FreeMem(startupCopperWords,4*sizeof(UWORD));
        memory.release();
        return 20;
    }
    alignas(AmigaGardenScene) static UBYTE gardenStorage[sizeof(AmigaGardenScene)];
    auto& gardenScene=*new(gardenStorage) AmigaGardenScene(memory.menuForeground(), memory.menuBackground(),
        memory.menuEntry(), memory.copper(), memory.copper() + downwellCopperWords,
        downwellCopperWords);
#ifndef DOWNWELL_REPLAY_VERIFY
    /*
     * Compose the garden completely while the startup display is still
     * blank.  Once presented, these two one-bit planes are immutable: the
     * copper scrolls them solely through pointers and BPLCON1.
     */
    if (!gardenScene.prepare(menuDrawList, activeGamePack, activeGamePackEnd)) {
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

    alignas(AmigaStyleScene) static UBYTE styleStorage[sizeof(AmigaStyleScene)];
    auto& styleScene = *new (styleStorage) AmigaStyleScene(memory.sceneCache(), sceneCacheBytes,
        memory.copper(), memory.copper() + downwellCopperWords, downwellCopperWords,
        memory.game(), memory.hud(), downwellLayerBytes);
#if defined(DOWNWELL_SKIP_INTRO) && !defined(DOWNWELL_SKIP_TO_GAME)
    if (!styleScene.prepare(activeGamePack, activeGamePackEnd)) {
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
    alignas(AmigaGameplayScene) static UBYTE gameplayStorage[sizeof(AmigaGameplayScene)];
    auto& gameplayScene=*new(gameplayStorage) AmigaGameplayScene(dynamicCopperWords[0], dynamicCopperWords[1], downwellCopperWords);
    gameplayScene.setBackingRange(worldViewY, worldViewY + downwellDisplayHeight);
#ifndef DOWNWELL_REPLAY_VERIFY
    clear.bindDisplay(dynamicCopperWords[0], dynamicCopperWords[1], downwellCopperWords);
#endif
    static_assert(sceneCacheBytes >= 2 * AmigaSelectionLayer::bytes,
                  "Selection canvases must fit the retired scene cache");
    static_assert(downwellLayerBytes >= AmigaSelectionLayer::bytes,
                  "Selection borders must fit the smaller retired HUD buffer");
    DownwellSplashCopperList splashBlankList(
        memory.copper() + downwellCopperWords * 2, downwellCopperWords);
    alignas(AmigaPublisherScene) static UBYTE publisherStorage[sizeof(AmigaPublisherScene)];
    auto& publisherScene=*new(publisherStorage) AmigaPublisherScene(
        memory.copper() + downwellCopperWords * 3, downwellCopperWords);
    alignas(AmigaCreditsScene) static UBYTE creditsStorage[sizeof(AmigaCreditsScene)];
    auto& creditsScene=*new(creditsStorage) AmigaCreditsScene(
        memory.copper() + downwellCopperWords * 4, downwellCopperWords);
    alignas(AmigaControlsScene) static UBYTE controlsStorage[sizeof(AmigaControlsScene)];
    auto& controlsScene=*new(controlsStorage) AmigaControlsScene(
        memory.copper() + downwellCopperWords * 5,
        dynamicCopperWords[0], dynamicCopperWords[1], downwellCopperWords);
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
        !gameplayScene.build(dynamicCopper0,
            gameBitmap, hudBitmap, playerSprites[0], bulletSprites,
            gameplayRegularSpriteCount, enemySprites[0], palette,
            port.ringOrigin())
#else
        !splashBlankList.buildNarrow(
            amigaSplashFrame(memory.sceneCache(), 1), nullSprite, palette) ||
        !publisherScene.prepare(memory.sceneCache(), sceneCacheBytes, nullSprite, palette) ||
        !creditsScene.prepare(hudBitmap, nullSprite, palette) ||
        !controlsScene.prepare(memory.sceneCache(), sceneCacheBytes, nullSprite, palette)
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
    platform.publish(gameplayScene.copper(dynamicCopper0));
#else
    platform.publish(splashBlankList);
#ifdef DOWNWELL_SCENE_VERIFY
    expectedSplashCopper = splashBlankList.data();
#endif
#endif
    platform.waitVblank();
    FreeMem(startupCopperWords, 4 * sizeof(UWORD));
    if (!gameplayScene.build(dynamicCopper1,
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
    // Startup lists use the private backing; normal frames must bind display
    // rings, and intervening intro scenes share these Copper slots.
    gameplayScene.invalidate();
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
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
        if(scenes.scene()==AmigaScene::gameplay)
            downwellProfileGameplay(port.tick(),176L*256+port.player().offsetYFixed(),
                worldViewY,port.health(),port.enemyCount());
#endif
        UBYTE packedInput;
        int scrollDelta = 0;
        bool cameraXChanged = false;
        unsigned destroyedBlockCount = 0;
        const unsigned oldWallTileCount=port.wallBackgroundTiles().size();
        int destroyedBlockX[AmigaPortRuntime::destroyedCapacity] = {0};
        int destroyedBlockY[AmigaPortRuntime::destroyedCapacity] = {0};
        DW_PROFILE_PHASE(profileWait);
        // The spatial tree is derived state. Only observing queries (or log
        // pressure) should materialize it; even idle-time service can overrun
        // VBI and spends cycles on state this gameplay path never reads.
        platform.waitVblank();
        if(!collisionOwner.valid()){exitCode=20;break;}
        // The preceding detached list has crossed the publication boundary.
        // Only now may another overlay overwrite an entry cover's DMA bank.
        if(residentCover==ResidentCover::entry&&gameplayScene.coverDetached())
            residentCover=ResidentCover::none;
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
#ifdef DOWNWELL_SHOP_VERIFY
            if (scenes.scene() == AmigaScene::gameplay)
                liveInput = shopVerification.input(port);
#endif
#ifdef DOWNWELL_CAMERA_VERIFY
            // Synthetic flat collision fixture, not a natural shop traversal.
            // Real movement/camera and all display staging/publication still run.
            if (scenes.scene() == AmigaScene::gameplay) {
                liveInput = {};
                const unsigned phase = port.tick() % 320;
                liveInput.right = phase < 80 || phase >= 240;
                liveInput.left = !liveInput.right;
            }
#endif
#ifdef DOWNWELL_LASER_VERIFY
            if (sceneGameplayLandedReported && !port.upgrades().levels[12]) port.grantUpgrade(12);
#endif
            if (gDownwellGemHighCapture == 1 && sceneGameplayLandedReported) {
                // Synthetic accumulated combo only; normal landing rewards,
                // controller selection, firing and rendering do the rest.
                const_cast<downwell::RunCombo&>(port.runCombo()).state.count = 8;
                liveInput = {}; // Let the grounded reward finish before jumping.
                gDownwellGemHighCapture = 2;
            }
#ifdef DOWNWELL_CLEAR_VERIFY
            if (port.levelClearActive()) {
                liveInput = {};
                const unsigned frame = gDownwellClearChoiceInputFrames;
                // Each room independently exercises all three choices and
                // held-key edge filtering before confirmation is allowed.
                liveInput.right = frame == 12 || frame == 13 || frame == 36 || frame == 37;
                liveInput.left = frame == 20 || frame == 21 || frame == 28 || frame == 29;
                liveInput.fire = frame >= 52;
            }
#endif
#ifdef DOWNWELL_NATURAL_VERIFY
            if (scenes.scene() == AmigaScene::gameplay)
                liveInput = naturalRoute.input(port, terrainCollision, gDownwellClearGameplayUpdates);
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
#ifdef DOWNWELL_SKIP_LEVEL
                static unsigned clearProfileFrames = 0;
                if (port.levelClearActive()) {
                    ++clearProfileFrames;
                    liveInput.right = clearProfileFrames >= 160 && clearProfileFrames < 240 && (clearProfileFrames % 16) == 0;
                    liveInput.fire = clearProfileFrames >= 240;
                    gDownwellProfileSection = clearProfileFrames >= 240 ? 5 :
                        clearProfileFrames >= 160 ? 4 : port.levelClearFrame().choices ? 3 : 2;
                } else gDownwellProfileSection = clearProfileFrames ? 6 : 0;
#else
#ifdef DOWNWELL_PROFILE_LASER
                if (!port.upgrades().levels[12]) port.grantUpgrade(12);
#endif
                liveInput.right = ((port.tick() / 120) & 1) == 0;
                liveInput.left = !liveInput.right;
                liveInput.fire = (port.tick() % 16) == 0 || (port.tick() % 16) == 3;
#ifdef DOWNWELL_PROFILE_CONTACT
                liveInput=downwell::gameplayContactProfileInput(port.tick(),
                    240+port.player().offsetX(),176+port.player().offsetY());
#endif
#endif
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
                        !gardenScene.prepare(menuDrawList, activeGamePack, activeGamePackEnd))
                        break;
                    gameplayScene.invalidate();
                }
                if (presentationChanged && presentedScene == AmigaScene::styleSelect) {
                    // These framebuffers and copper slots may contain gameplay,
                    // even when the new selection matches the last visit exactly.
                    // Garden prepared the immutable border/masks on entry and
                    // only uses its separate menu planes thereafter. Other
                    // predecessors may have overwritten that shared cache.
                    if (!styleScene.prepare(activeGamePack, activeGamePackEnd,
                        priorPresentedScene == AmigaScene::menu))
                        break;
                }
                if (presentedScene == AmigaScene::splash) {
                    if (presentationChanged || splashDitherChanged) {
                        const CopperList* splashProgram = nullptr;
                        bool alternating = false;
                        if (presentedSplash == 1) splashProgram = &splashBlankList;
                        else if (presentedSplash == 2) splashProgram = publisherScene.presentation();
                        else if (presentedSplash == 3) splashProgram = creditsScene.presentation();
                        else splashProgram = controlsScene.presentation(presentedSplash,
                            presentedSplashDither, back, nullSprite, palette, alternating);
                        if (!splashProgram) break;
                        if (alternating) front = back;
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
                            gDownwellCreditsFrame = creditsScene.pixels();
                            gDownwellCreditsCopper = platform.publishedCopper();
                            downwellCreditsFrameReady();
                        }
                        if (!sceneSplashFrameReported &&
                            presentedSplash == 2) {
                            sceneSplashFrameReported = true;
                            gDownwellSplashFrame = publisherScene.pixels();
                            downwellSplashFrameReady();
                        }
                        if (!sceneControlsFrameReported &&
                            presentedSplash == 4) {
                            gDownwellControlsCopper = platform.publishedCopper();
                            sceneControlsFrameReported = true;
                            gDownwellControlsFrame = controlsScene.pixels();
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
                        !styleScene.beginPrepare(activeGamePack, activeGamePackEnd))
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
                        if (!liveGeneration.loadRoom(port,rooms,mainRoom)) break;
                        port.confirmFirstStage();
                        if (!port.initializeAreaOneBuilder(liveBuilder, -16, 704, scenes.styleCursor())) break;
                        liveGeneration.bind(port);
                        port.configureGenerationStep(generateLiveTerrain, &liveGeneration);
                        port.configureLandingResolver(amigaResolveTerrainLanding, &terrainCollision);
                        port.configureSolidResolver(amigaResolveTerrainSolid, &terrainCollision,
                            amigaResolveTerrainSegment, amigaResolveTerrainPlayerMask,amigaResolveTerrainWallContacts);
                        terrainRaster.ready = false;
                        wallDamage.clear();
                        terrainCopyTarget = nullptr;
                        terrainCopyBacking = nullptr;
                        stagedBuffer = memory.menuForeground();
                        for (unsigned buffer = 0; buffer < 2; ++buffer) {
                            gameDisplay[buffer].data = memory.sceneCache() + buffer * downwellGameRingBytes;
                            gameplayScene.invalidate(buffer);
                        }
                        gameplayRows.reset();
                        worldViewX = mainViewX + port.cameraOffsetX();
                        worldViewY = mainInitialViewY + port.cameraOffsetY();
                        gameplayScene.resetStage();
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
                    residentCover = ResidentCover::none;
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
                    if (!terrainRaster.ready)
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
                        gameplayScene.setBackingRange(worldViewY,
                            worldViewY + downwellDisplayHeight + gameplayPrefetchLines);
                        syncGameRows(gameDisplay[0], gameBitmap, 0);
                        syncGameRows(gameDisplay[1], gameBitmap, 1);
                    }
                }
                if (presentedScene == AmigaScene::menu) {
                    if (presentationChanged)
                        gardenScene.enter();
                    AmigaGardenScene::Frame garden;
                    if (!gardenScene.render(back, scenes, menuEntry, nullSprite, palette, garden))
                        break;
                    const auto& menuScroll = garden.scroll;
                    UWORD* menuPlayerData = garden.player;
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
                    if (!oracleReady || oracle.size() != gardenScene.copper(back).size())
                        ++gDownwellMenuCopperPatchFailures;
                    else for (unsigned word = 0; word < oracle.size(); ++word)
                        if (oracleWords[word] != gardenScene.copper(back).data()[word])
                            ++gDownwellMenuCopperPatchFailures;
#endif
                    DW_PROFILE_PHASE(profilePublish);
                    platform.publish(gardenScene.copper(back));
#ifdef DOWNWELL_SCENE_VERIFY
                    if (menuEntry && menuScroll.backgroundVisibleLines == 0)
                        downwellMenuSkyCovered();
#endif
                    DW_PROFILE_PHASE(profileOther);
                    front = back;
                    if (presentationChanged &&
                        priorPresentedScene == AmigaScene::splash)
                        platform.waitVblank();
                    // On later garden frames the previous garden list has
                    // already run, so splash/gameplay no longer read this
                    // cache. Prepare immutable style data without delaying
                    // game-state transitions or overwriting a live DMA source.
                    if (!presentationChanged && !styleScene.preparePart())
                        break;
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
                    AmigaStyleScene::Frame style;
                    if (!styleScene.render(back, scenes, style))
                        break;
#ifdef DOWNWELL_SCENE_VERIFY
                    const UBYTE* styleFrame = style.content;
                    AmigaStyleFrameState referenceState;
                    if (!amigaRenderStyleFrame(activeGamePack, activeGamePackEnd,
                            memory.sceneCache() + amigaStyleFrameBytes * 2,
                            scenes, memory.sceneCache(), referenceState, true)) break;
                    ++gDownwellStyleReferenceFrames;
                    UBYTE* styleComposite = memory.sceneCache() + amigaStyleFrameBytes;
                    const UBYTE* referenceBorder = memory.sceneCache() + amigaStyleFrameBytes * 2 - scenes.styleTileY() * amigaStyleBytesPerRow;
                    for (unsigned byte = 0; byte < amigaStylePlaneBytes; ++byte) {
                        const UBYTE occupied = styleFrame[byte] | styleFrame[byte + AmigaSelectionLayer::planeBytes];
                        styleComposite[byte] = styleFrame[byte] | (referenceBorder[byte] & ~occupied);
                        styleComposite[byte + amigaStylePlaneBytes] = styleFrame[byte + AmigaSelectionLayer::planeBytes] |
                            (referenceBorder[byte + amigaStyleBorderPlaneBytes] & ~occupied);
                    }
                    for (unsigned byte = 0; byte < amigaStyleFrameBytes; ++byte) {
                        if (styleComposite[byte] != memory.sceneCache()[byte]) {
                            ++gDownwellStyleReferenceFailures;
                            break;
                        }
                    }
#endif
                    DW_PROFILE_PHASE(profileCopper);
                    if (!styleScene.updateCopper(back, style, nullSprite, palette))
                        break;
                    DW_PROFILE_PHASE(profilePublish);
                    platform.publish(styleScene.copper(back));
                    front = back;
                    DW_PROFILE_PHASE(profileOther);
#ifdef DOWNWELL_SCENE_VERIFY
                    gDownwellStyleMask = style.mask;
                    gDownwellStyleCopper = styleScene.copper(back).data();
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
                    if (!gameplayScene.build(back,
                            gameDisplay[back], hudBitmap,
                            presentedScene == AmigaScene::gameplay
                                ? playerSprites[back] : nullSprite,
                            bulletSprites + back *
                                gameplayRegularSpriteCount,
                            gameplayRegularSpriteCount,
                            enemySprites[back], palette,
                            port.ringOrigin()))
                        break;
                    platform.publish(gameplayScene.copper(back));
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
#ifdef DOWNWELL_SKIP_LEVEL
            if (skipLevelPending) {
                // Initialization has now bound terrain and the entry RNG.
                // Only the first level is skipped; later levels play normally.
                port.registerLevelExit(176 + port.player().offsetY());
                skipLevelPending = false;
            }
#endif
            if (port.levelResultRequested() || port.levelClearActive()) {
                const bool entering = !clear.presented;
                if (entering) {
                    DW_PROFILE_SCOPE(profileClearEntry);
#ifdef DOWNWELL_NATURAL_VERIFY
                    gDownwellNaturalExitTick = port.tick();
#endif
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
                    gDownwellProfileSection = 1;
#endif
#ifdef DOWNWELL_CLEAR_VERIFY
                    gDownwellClearPhase = 1; downwellClearProbe();
                    gDownwellClearChoiceInputFrames = 0;
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
                    if (!clear.enter(port, scenes.styleCursor(), liveInput,
                            memory.menuForeground(), menuPlaneBytes * 2 + amigaMenuEntryPlaneBytes,
                            amiga_hud_pack_end - amiga_hud_pack, amiga_clear_pack, amiga_clear_pack_end - amiga_clear_pack,
                            amiga_selection_mask, amiga_selection_mask_end - amiga_selection_mask)) { exitCode = 20; break; }
#ifdef DOWNWELL_CLEAR_VERIFY
                    gDownwellClearPhase = 2;
#endif
                    for (unsigned buffer = 0; buffer < 2; ++buffer) {
                        gameDisplay[buffer].data = memory.sceneCache() + buffer * downwellGameRingBytes;
                        gameplayScene.invalidate(buffer);
                    }
                } else if (!port.nextLevelRequested()) {
                    DW_PROFILE_SCOPE(profileClearStep);
                    if (!clear.step(port, liveInput)) { exitCode = 20; break; }
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
                const bool bankChanged = clear.needsArtwork(port);
                if (bankChanged) {
                    DW_PROFILE_SCOPE(profileClearBank);
                    AmigaHardware::blitterDrain();
                    if (!clear.refreshArtwork(port, memory.menuForeground(), amiga_clear_pack, amiga_clear_pack_end - amiga_clear_pack)) {
                        exitCode = 20; break;
                    }
                }
                const unsigned back = front ^ 1u;
                AmigaRenderer target = renderer;
#ifdef DOWNWELL_CLEAR_VERIFY
                gDownwellClearPhase = 3;
#endif
                target.game = back ? memory.game() : memory.hud();
                target.hud = memory.sceneCache() + back * AmigaSelectionLayer::bytes;
                const UBYTE* bank = memory.menuForeground() + clear.assets.layout.resultsOffset;
                AmigaUpgradeScene::Frame frame;
                if (!clear.render(back, target.game, target.hud, bank,
                        bank + clear.assets.bankBytes, port, frame)) { exitCode = 20; break; }
                DW_PROFILE_PHASE(profileClearSprites);
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
                DW_PROFILE_PHASE(profileClearCopper);
                const UBYTE* mask = port.exitDither().active ? memory.menuForeground() +
                    AmigaExitMask::offset(port.exitDither().y - room.motion.viewY()) /
                    AmigaExitMask::stride * AmigaExitMask::selectionStride : nullptr;
                if (!ready || !clear.updateDisplay(back, frame, mask, playerSprites[back],
                        bulletSprites + back * gameplayRegularSpriteCount, gameplayRegularSpriteCount, enemySprites[back], palette)) {
                    exitCode = 20; break;
                }
                DW_PROFILE_PHASE(profileClearDrain);
                AmigaHardware::blitterDrain();
#ifdef DOWNWELL_CLEAR_VERIFY
                alignas(2) static UBYTE expectedGame[2][downwellGameRingBytes], expectedHud[2][downwellLayerBytes];
                static AmigaClearPresenter oracle;
                if (entering) oracle.reset();
                if (bankChanged) oracle.artworkChanged();
                AmigaRenderer reference;
                amigaRendererInit(&reference, expectedGame[back], expectedHud[back]);
                AmigaClearPresenter::Frame expected;
                alignas(2) static UBYTE actualGame[downwellGameRingBytes], actualHud[downwellLayerBytes];
                gDownwellClearActualHud = actualHud; gDownwellClearExpectedHud = expectedHud[back];
                gDownwellClearActualGame = actualGame; gDownwellClearExpectedGame = expectedGame[back];
                if (!oracle.render(back, reference, bank, bank + clear.assets.bankBytes, room, snapshot, port.upgrades(), expected))
                    ++gDownwellClearFailures;
                // Compare visible pixels, not storage: the new canvases are
                // world-space narrow planes, the oracle uses a wide ring.
                for (unsigned layer = 0; layer < 2; ++layer) {
                    const UBYTE* compact = layer ? frame.layer : frame.border;
                    const UBYTE* wide = layer ? expectedHud[back] : expectedGame[back];
                    UBYTE* actual = layer ? actualHud : actualGame;
                    const unsigned size = layer ? downwellLayerBytes : downwellGameRingBytes;
                    for (unsigned i = 0; i < size; ++i) actual[i] = wide[i];
                    for (unsigned plane = 0; plane < 2; ++plane)
                        for (unsigned y = 0; y < downwellDisplayHeight; ++y) {
                            const unsigned row = layer ? y : (expected.origin + y) % downwellGameRingHeight;
                            for (unsigned word = 0; word < 10; ++word) {
                                const unsigned index = plane * (size / 2) + row * 48 + 13 + word * 2;
                                const UBYTE* src = compact + plane * AmigaSelectionLayer::planeBytes + y * 22 + word * 2;
                                const unsigned value = (unsigned(src[0]) << 8) | src[1];
                                const unsigned expectedWord = ((unsigned(wide[index]) << 14) |
                                    (unsigned(wide[index+1]) << 6) | (wide[index+2] >> 2)) & 0xffff;
                                if (value != expectedWord) ++gDownwellClearFailures;
                                actual[index] = (actual[index] & 0xfc) | (value >> 14);
                                actual[index+1] = value >> 6;
                                actual[index+2] = (actual[index+2] & 3) | (value << 2);
                            }
                        }
                }
                ++gDownwellClearFrames;
                if (snapshot.choices) ++gDownwellClearChoices;
                if (snapshot.choices && room.selection.visible && gDownwellClearReentries < 2) {
                    ++gDownwellClearChoiceInputFrames;
                    gDownwellClearCursorMasks[gDownwellClearReentries] |= 1u << room.selection.cursor;
                    const unsigned frame = gDownwellClearChoiceInputFrames;
                    const unsigned expectedCursor = frame >= 13 && frame <= 20 ? 2 :
                        frame >= 29 && frame <= 36 ? 0 : 1;
#ifndef DOWNWELL_NATURAL_VERIFY
                    if (room.selection.cursor != expectedCursor) ++gDownwellClearFailures;
#else
                    (void)expectedCursor;
#endif
                }
                gDownwellClearPhase = 4;
                if (gDownwellClearFrames == 1 || gDownwellClearFrames % 60 == 0)
                    downwellClearProbe();
#endif
                DW_PROFILE_PHASE(profileClearPublish);
                platform.publish(clear.copper(back)); front = back;
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
#if defined(DOWNWELL_CLEAR_VERIFY) && !defined(DOWNWELL_NATURAL_VERIFY)
            if ((port.tick() == 180 && !gDownwellClearReentries) ||
                (gDownwellClearReentries == 1 && gDownwellClearGameplayUpdates == 60))
                port.registerLevelExit(176 + port.player().offsetY());
#endif
            for (unsigned i = 0; i < downwell::ShopWorld::capacity; ++i)
                oldShopArt[i] = port.shops().instances[i].artRevision;
#ifdef DOWNWELL_CAMERA_VERIFY
            // Bind through the world: step() rebinds the player's callbacks.
            port.configureLandingResolver([](void*, int, int) { return 0; }, nullptr);
            port.configureSolidResolver([](void*, int, int) { return false; }, nullptr);
            if (!port.tick()) {
                // Exercise cold staging, not just transfers from a fully
                // prepared raster. Production invalidation follows generation.
                for (unsigned y=0; y<terrainRasterRows; ++y) {
                    terrainRaster.dirty(0,y)=true;
                    terrainRaster.dirty(2,y)=true;
                }
            }
            if (!port.tick() && (!port.registerSideCameraLocker(385, 256) ||
                                 !port.registerSideCameraLocker(95, 288))) return 20;
            const bool wasSideLocked = port.cameraSideLocked();
#endif
            checkSceneStack(6);
            const AmigaPortFrame result = port.step(packedInput);
            checkSceneStack(7);
#ifdef DOWNWELL_CAMERA_VERIFY
            if (!wasSideLocked && port.cameraSideLocked()) ++gDownwellCameraLocks;
            if (wasSideLocked && !port.cameraSideLocked()) ++gDownwellCameraReleases;
#endif
#ifdef DOWNWELL_SHOP_VERIFY
            shopVerification.check(port);
            gDownwellShopLiveCoverage = shopVerification.coverage;
            gDownwellShopLiveFailures = shopVerification.failures;
#endif
#ifdef DOWNWELL_CLEAR_VERIFY
            gDownwellClearTick = port.tick();
            if (port.tick() % 60 == 0) downwellClearProbe();
            if (gDownwellClearReentries) {
                ++gDownwellClearGameplayUpdates;
#ifdef DOWNWELL_NATURAL_VERIFY
                naturalRoute.check(port);
                gDownwellNaturalFailures = naturalRoute.failures;
                gDownwellNaturalReturnFrames = naturalRoute.returnFrames;
#else
                if (gDownwellClearReentries == 2 && gDownwellClearGameplayUpdates == 60) downwellClearReady();
#endif
            }
#endif
            if (!port.effectsSucceeded()) { exitCode=20; break; }
            if (!port.generationSucceeded()) { exitCode=20; break; }
#ifndef DOWNWELL_REPLAY_VERIFY
            // Invalidate before selecting a precomposed side-room ring.
            invalidateShopRaster(port.shops());
            for (unsigned i = 0; i < downwell::ShopWorld::capacity; ++i)
                if (oldShopArt[i] != port.shops().instances[i].artRevision) gameplayScene.invalidateStage();
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
            // The runner sweeps marked instances at Draw entry. Inter-phase
            // reconciliation deliberately keeps those identities alive.
            if (!collisionOwner.prepareDraw()) { exitCode=20; break; }
            scrollDelta = result.scrollDelta;
            cameraXChanged = result.cameraXChanged;
#ifdef DOWNWELL_CAMERA_VERIFY
            gDownwellCameraColumns |= 1u << ((port.cameraOffsetX() + 160) / 160);
            if (cameraXChanged) ++gDownwellCameraTransitions;
#endif
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
#if defined(DOWNWELL_SHOP_VERIFY) || defined(DOWNWELL_NATURAL_VERIFY)
            if (port.shops().allocatedCount && gDownwellShopPack) {
                alignas(2) static UBYTE expected[downwellLayerBytes];
                AmigaHardware::blitterDrain();
                for (unsigned i = 0; i < downwellLayerBytes; ++i) expected[i] = renderer.hud[i];
                AmigaRenderer oracle = renderer;
                oracle.hud = oracle.target = expected;
                oracle.blitterEnabled = 0;
                amigaRendererClearClipRect(&oracle);
                amigaRendererFillRect(&oracle, downwellPlayfieldX, 0, 160, 284, 0);
                static AmigaShopMenu fresh;
                fresh.reset();
                if (!fresh.draw(&oracle, gDownwellShopPack,
                        gDownwellShopPack + gDownwellShopAssetBytes, activeShop,
                        mainViewX + port.cameraOffsetX(), mainInitialViewY + port.cameraOffsetY(),
                        port.currency())) ++gDownwellShopLiveFailures;
                composeStreak(&oracle, port.dead() ? 0 : port.gemStreak());
                for (unsigned i = 0; i < downwellLayerBytes; ++i)
                    if (expected[i] != renderer.hud[i]) ++gDownwellShopLivePixelFailures;
                ++gDownwellShopLiveFrames;
            }
#endif
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
            // Retained wall patches use sparse debts, not the min/max span of
            // unrelated generation work. Preserve clean rows between bands.
            const auto& wallTiles=port.wallBackgroundTiles();
            for(unsigned i=oldWallTileCount;i<wallTiles.size();++i){
                std::int32_t y;
                if(!wallTiles.at(i)->y.roundPixel(y)||y>2147483647-16){terrainPaintSucceeded=false;break;}
                if(!wallDamage.mark(y,y+16)){terrainPaintSucceeded=false;break;}
            }
            if(!wallDamage.visit([&](int first,int end){
                terrainRaster.invalidate(first,end);
                gameplayScene.invalidateStage();
                gameplayScene.invalidateBackingOutside(first,end,worldViewY,downwellDisplayHeight);
                return true;
            }))terrainPaintSucceeded=false;
            if (liveGeneration.dirtyFirst < liveGeneration.dirtyEnd) {
                gameplayScene.invalidateStage();
                // Discard dirty retained rows outside the current view. Visible
                // intersections are refreshed below, after any camera swap.
                gameplayScene.invalidateBackingOutside(liveGeneration.dirtyFirst,
                    liveGeneration.dirtyEnd, worldViewY, downwellDisplayHeight);
            }
            if (cameraXChanged) {
                worldViewX = mainViewX + port.cameraOffsetX();
#ifdef DOWNWELL_CAMERA_VERIFY
                // Force exactly one absent-prefetch case as well as real hits.
                if (gDownwellCameraTransitions == 1) gameplayScene.invalidateStage();
#endif
                if (gameplayScene.stageCovers(worldViewX, worldViewY, downwellDisplayHeight)) {
#ifdef DOWNWELL_CAMERA_VERIFY
                    ++gDownwellCameraStaged;
#endif
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
                    gameplayRows.copied(0, downwellGameRingHeight, back);
                    gameplayScene.setBackingRange(gameplayScene.stageFirst(), gameplayScene.stageEnd());
#ifdef DOWNWELL_SCENE_VERIFY
                    AmigaHardware::blitterDrain();
                    const unsigned column = (worldViewX - mainViewX + 160) / 160;
                    for (unsigned y = 0; y < downwellDisplayHeight; ++y) {
                        const unsigned physical = (port.ringOrigin() + y) % downwellGameRingHeight;
                        for (unsigned plane = 0; plane < 2; ++plane) {
                            const UWORD* displayed = (const UWORD*)((const UBYTE*)gameDisplay[back].data +
                                plane * downwellGameRingPlaneBytes + physical * downwellBytesPerPlaneLine + terrainRasterX / 8);
                            for (unsigned word = 0; word < terrainRasterWords; ++word)
                                if (displayed[word] != terrainRaster.row(column, worldViewY + y)[plane * terrainRasterWords + word])
                                    ++gDownwellTerrainRasterFailures;
                        }
                    }
                    amigaRendererSetGameOrigin(&renderer, port.ringOrigin());
                    drawMainWorldSpan(&renderer, rooms, mainRoom, builders,
                        fragments, liveBuilder.fragmentCount(), generatedDecorations,
                        liveBuilder.decorationCount(), &port, worldViewX, worldViewY, 0, downwellDisplayHeight);
#endif
                } else {
#ifdef DOWNWELL_CAMERA_VERIFY
                ++gDownwellCameraFallback;
#endif
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
                gDownwellProfileStageShortfall = worldViewY + downwellDisplayHeight - gameplayScene.stageEnd();
#endif
                amigaRendererSetGameOrigin(&renderer, port.ringOrigin());
                if (!terrainRaster.ready)
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
                gameplayScene.setBackingRange(worldViewY,
                    worldViewY + downwellDisplayHeight + gameplayPrefetchLines);
                }
                gameplayScene.invalidateStage();
            }
            // Keep both ends of the actual cached interval. A jumping camera
            // can reverse into retained rows without repainting either edge.
            const auto exposed = gameplayScene.exposeBacking(worldViewY,
                downwellDisplayHeight, downwellGameRingHeight);
            const int missingFirst = exposed.first, missingHeight = exposed.count;
            if (missingHeight > 0) {
                if (backingCopyPending) AmigaHardware::blitterDrain();
                int origin = (int)port.ringOrigin() + missingFirst - worldViewY;
                while (origin < 0) origin += downwellGameRingHeight;
                amigaRendererSetGameOrigin(&renderer, origin % downwellGameRingHeight);
                if (!terrainRaster.ready)
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
                        top, bottom - top, downwellPlayfieldX, downwellPlayfieldWidth, true, 1);
                }
            }
            if(!wallDamage.repair([&](int first,int end){
                if(!terrainPaintSucceeded)return false;
                const int viewEnd=worldViewY+downwellDisplayHeight;
                if(end<=worldViewY||first>=viewEnd)return true;
                const int top=first>worldViewY?first-worldViewY:0;
                const int bottom=end<viewEnd?end-worldViewY:downwellDisplayHeight;
                if(backingCopyPending)AmigaHardware::blitterDrain();
                amigaRendererSetGameOrigin(&renderer,port.ringOrigin());
                amigaRendererClearGameRows(&renderer,top,bottom-top,0);
                drawMainWorldSpan(&renderer,rooms,mainRoom,builders,fragments,liveBuilder.fragmentCount(),
                    generatedDecorations,liveBuilder.decorationCount(),&port,worldViewX,worldViewY,
                    top,bottom-top,downwellPlayfieldX,downwellPlayfieldWidth,true,1);
                return terrainPaintSucceeded;
            }))terrainPaintSucceeded=false;
            for (unsigned i = 0; i < downwell::ShopWorld::capacity; ++i) {
                const auto& shop = port.shops().instances[i];
                if (!shop.allocated || shop.artRevision == oldShopArt[i]) continue;
                if (backingCopyPending) AmigaHardware::blitterDrain();
                gameplayScene.invalidateStage();
                const int first = shop.y - 21, end = shop.y + 12;
                gameplayScene.invalidateBackingOutside(first, end, worldViewY, downwellDisplayHeight);
                int top = first - worldViewY, bottom = end - worldViewY;
                if (top < 0) top = 0;
                if (bottom > downwellDisplayHeight) bottom = downwellDisplayHeight;
                if (top >= bottom) continue;
                amigaRendererSetGameOrigin(&renderer, port.ringOrigin());
                amigaRendererClearGameRows(&renderer, top, bottom - top, 0);
                drawMainWorldSpan(&renderer, rooms, mainRoom, builders,
                    fragments, liveBuilder.fragmentCount(), generatedDecorations,
                    liveBuilder.decorationCount(), &port, worldViewX, worldViewY,
                    top, bottom - top, downwellPlayfieldX, downwellPlayfieldWidth, true, 2);
            }
            if (destroyedBlockCount != 0) {
                if (backingCopyPending) AmigaHardware::blitterDrain();
                gameplayScene.invalidateStage();
                for (unsigned block = 0;
                     block < destroyedBlockCount; ++block) {
                    const int centerY = 176 + destroyedBlockY[block];
                    terrainRaster.invalidate(centerY - 8, centerY + 8);
                    // Bullets can change rows retained just outside the view.
                    // Exclude those rows from the ring's valid interval so
                    // they are refreshed before the camera exposes them.
                    gameplayScene.invalidateBackingOutside(centerY - 8, centerY + 8,
                        worldViewY, downwellDisplayHeight);
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
                        &port, worldViewX, worldViewY, top, bottom - top, left, right - left, true, 3);
                }
            }
            // Prepare a possible side-room view a small strip at a time in
            // retired garden memory. The actual camera jump then needs only
            // Chip-to-Chip DMA, not an entire Fast-to-Chip raster transfer.
            if (!cameraXChanged && missingHeight == 0 && destroyedBlockCount == 0 && terrainRaster.ready) {
                const int playerX = port.player().offsetX();
                const int cameraX = port.cameraOffsetX();
                const int wanted = AmigaGameplayScene::stageColumn(playerX,
                    port.player().speedXFixed(), cameraX, port.dead());
                const int targetY = wanted != AmigaGameplayScene::noStageColumn
                    ? port.previewCameraViewY(wanted) : -1;
                if (targetY >= 0 && targetY + downwellGameRingHeight <= terrainRasterRows) {
                    const int targetX = mainViewX + wanted;
                    const auto span = gameplayScene.stageSpan(targetX, targetY, downwellDisplayHeight,
                        AmigaTerrainRaster::stagePaintRows);
                    const int first = span.first, count = span.count;
                    if (count > 0) {
                        const unsigned column = (targetX - mainViewX + 160) / 160;
                        AmigaRenderer stage = renderer;
                        stage.game = stage.target = stagedBuffer;
                        int origin = port.ringOrigin() + first - worldViewY;
                        while (origin < 0) origin += downwellGameRingHeight;
                        stage.gameOrigin = origin % downwellGameRingHeight;
                        const int prepared = terrainRaster.prepareStage(&stage, column, first, count,
                            gameplayRows, [&](unsigned top, unsigned height) {
#ifdef DOWNWELL_SCENE_VERIFY
                                gDownwellColdStageRows += height;
#endif
                                downwellProfileTerrainSpan(targetX, first + top, height,
                                    downwellPlayfieldX, downwellPlayfieldWidth, 1, 4);
                                amigaRendererClearGameRows(&stage, top, height, 0);
                                if(!AmigaTerrainPainter::paint(&stage, rooms, mainRoom, terrainCollision.index,
                                    terrainTiles, generatedDecorations, liveBuilder.decorationCount(), &port,
                                    activeGamePack, activeGamePackEnd, targetX, first,
                                    downwellPlayfieldX, top, downwellPlayfieldWidth, height))
                                    return terrainPaintSucceeded=false;
                                AmigaHardware::blitterDrain();
                                return true;
                            });
                        if (prepared >= 0)
                            gameplayScene.staged({first, prepared}, downwellGameRingHeight);
                        else gameplayScene.invalidateStage();
                    }
                }
            }
            DW_PROFILE_PHASE(profileSprite);
            static downwell::TimeFieldOutlines fieldShape;
            fieldShape.set(port.timeFields(), port.goalField(), port.levelExitPresent(),
                port.goalFieldBirthBoundary(), worldViewX, worldViewY, downwellPlayfieldX,port.startTimeField());
            bool fieldChanged = !gameplayFields[back].matches(
                (UBYTE*)gameDisplay[back].data, port.ringOrigin(), fieldShape);
            if (!fieldChanged && fieldShape.visible)
                fieldChanged=gameplayFields[back].touchesDirtyRows(
                    [&](unsigned row){return gameplayRows.dirty(row,back);});
            DW_PROFILE_PHASE(profileActorCollect);
            bool compositeCurrency = !port.dead();
#ifdef DOWNWELL_REPLAY_VERIFY
            compositeCurrency = false;
#endif
            static AmigaGameplayDrawList actorList{};
#ifdef DOWNWELL_ENTRY_WIPE_VERIFY
            // Presentation sweep through real registered Draw records. This
            // deliberately supplies fixture positions, not natural Step timing.
            auto& entryFixture=collisionOwner.entryDithers();
            if(!entryFixture.clear()){exitCode=20;break;}
            if(gDownwellEntryWipeFrames<25){
                downwell::InstanceHandle key;
                const int screenY=gDownwellEntryWipeFrames?320+int(gDownwellEntryWipeFrames-1)*16:-116;
                if(!entryFixture.create(400100+gDownwellEntryWipeFrames,
                    [&](downwell::MotionBinary64& value){value=downwell::MotionBinary64::fromInteger(worldViewX+80);return true;},
                    [&](downwell::MotionBinary64& value){value=downwell::MotionBinary64::fromInteger(worldViewY+screenY);return true;},key)){
                    exitCode=20;break;
                }
            }
#endif
            static_assert(gameplayPlayerStartY == AmigaGameplayDrawList::playerStartY &&
                          gameplayPlayerGroundY == AmigaGameplayDrawList::replayGroundY,
                          "Actor and gameplay camera anchors must agree");
            if (!actorList.collect(port, activeGamePack, activeGamePackEnd, worldViewX, worldViewY, compositeCurrency)) {
                gDownwellActorFailureSprite = actorList.failedSprite;
                gDownwellActorFailureFrame = actorList.failedFrame;
                frameReady = false;
            }
            using Real = downwell::MotionBinary64;
            const downwell::EffectSurfacePlacement::View effectView{Real::fromInteger(worldViewX),Real::fromInteger(worldViewY),
                Real::fromInteger(160),Real::fromInteger(284)};
            bool hardwareEntryCover=false;
            // Immutable cover DMA cannot reproduce shade-subtracted alpha.
            if(!actorList.entryCoverHardwareEligible(port,worldViewX,worldViewY,hardwareEntryCover)){
                exitCode=20;break;
            }
            const bool compositeEffects=compositeCurrency||(actorList.entryCoverVisible&&!hardwareEntryCover);
            if(compositeEffects){
                DW_PROFILE_SCOPE(profileControllerComposition);
                downwell::CurrencySmokeComposition::Settings settings;
                settings.noControl=port.shops().noControl;settings.death=!compositeCurrency;
                if(!controllerEffects.prepareWithSurfaceProducers([&](auto emit){
                    for(unsigned i=port.gemSlotEnd();i;--i){
                        const auto& gem=port.gemAt(i-1);
                        if(gem.active&&gem.motion.lifetime.visible())
                            emit(gem.large,gem.motion.animation.frame(),Real::fromInteger(240+gem.x),Real::fromInteger(176+gem.y));
                    }
                },port.smokeEffects(),effectAssets,effectView,settings,[&](auto emit){
                    return port.visitDebrisDraw([&](const downwell::DebrisAnimation& effect){
                        return downwell::DebrisDraw::surface(effect,effectAssets,effectView,emit);
                    });
                },[&](auto emit){
                    if(!actorList.entryCoverVisible||hardwareEntryCover)return true;
                    downwell::EffectMaskAssets::Asset mask;
                    return effectAssets.entryDither(80,actorList.entryCover.y,mask)&&emit(mask,effectView.x,effectView.y);
                },[&](auto emit){
                    for(unsigned i=0;i<port.enemyCount();++i){
                        const auto& enemy=port.enemyAt(i);
                        if(enemy.active&&enemy.awake)
                            emit(Real::fromInteger(240+enemy.x),Real::fromInteger(176+enemy.y));
                    }
                },[](Real n,std::int32_t& out){return n.roundPixel(out);})){
                    exitCode=20;break;
                }
            }
            const GameplayBob* actors = actorList.actors;
            const unsigned actorCount = actorList.count;
            const auto& rayLine = actorList.rayLine;
            const bool rayVisible = actorList.rayVisible;
            static_assert(gameplayRegularSpriteCount == AmigaGameplayActors::regularChannels,
                          "Gameplay Copper and sprite slots must agree");
            DW_PROFILE_PHASE(profileActorChannels);
            const auto assignment = AmigaGameplayActors::assign(actors, actorCount,
                amiga_game_pack, amiga_game_pack_end, playerSprites[back],
                bulletSprites + back * gameplayRegularSpriteCount, enemySprites[back],
                enemySpriteHeight,
#ifdef DOWNWELL_BOB_VERIFY
                0
#else
                8
#endif
                , compositeEffects && controllerEffects.visible() ? &controllerEffects.surface() : nullptr
            );
            const unsigned bobCount = assignment.bobCount;
            if (!assignment.ready) {
                gDownwellActorFailureSprite = assignment.failedSprite;
                gDownwellActorFailureFrame = assignment.failedFrame;
                frameReady = false;
            }
            DW_PROFILE_PHASE(profileCopper);
            if (rayVisible || gameplayRays[back].present()) {
                DW_PROFILE_SCOPE(profileLaserPrepare);
                bool changed = fieldChanged && (fieldShape.visible || gameplayFields[back].present());
                if (rayVisible && gameplayRays[back].matches(
                        (UBYTE*)gameDisplay[back].data, port.ringOrigin(), rayLine)) {
                    rayLine.draw([&](int x, int y) {
                        if (x >= downwellPlayfieldX && x < downwellPlayfieldX + downwellPlayfieldWidth &&
                            y >= 0 && y < downwellDisplayHeight &&
                            gameplayRows.dirty(AmigaLaserSight::physicalRow(port.ringOrigin(), y), back))
                            changed = true;
                    });
                    const int left = rayLine.x0 < rayLine.x1 ? rayLine.x0 : rayLine.x1;
                    const int right = rayLine.x0 > rayLine.x1 ? rayLine.x0 : rayLine.x1;
                    const int top = rayLine.y0 < rayLine.y1 ? rayLine.y0 : rayLine.y1;
                    const int bottom = rayLine.y0 > rayLine.y1 ? rayLine.y0 : rayLine.y1;
                    for (unsigned i = 0; !changed && i < bobCount; ++i) {
                        const auto& actor = actors[i];
                        if (actor.sprite == gameplayRayMarker) continue;
                        int x, y, w, h;
                        if (amigaRendererAssetBounds(activeGamePack, activeGamePackEnd,
                            actor.sprite, actor.frame, actor.x, actor.y, actor.flip, &x,&y,&w,&h) &&
                            x <= right && x+w > left && y <= bottom && y+h > top) changed = true;
                    }
                }
                { DW_PROFILE_SCOPE(profileLaserPrepareDrain); AmigaHardware::blitterDrain(); }
                {
                    DW_PROFILE_SCOPE(profileLaserRestore);
                    gameplayRays[back].prepare((UBYTE*)gameDisplay[back].data, port.ringOrigin(),
                        rayVisible ? &rayLine : nullptr, changed);
                }
            }
            if (fieldShape.visible || gameplayFields[back].present()) {
                DW_PROFILE_SCOPE(profileGoalFieldPrepare);
                AmigaHardware::blitterDrain();
                if (!gameplayFields[back].prepare((UBYTE*)gameDisplay[back].data, port.ringOrigin(), fieldShape, fieldChanged,
                        [&](unsigned row) { return gameplayRows.dirty(row, back); })) {
                    exitCode = 20; break;
                }
            }
            syncGameRows(gameDisplay[back], gameBitmap, back);
            if (fieldShape.visible) {
                DW_PROFILE_SCOPE(profileGoalFieldDraw);
                AmigaHardware::blitterDrain();
                gameplayFields[back].draw();
#ifdef DOWNWELL_CLEAR_VERIFY
                // Compare the retained field before BOB damage can exclude rows.
                exposeGameplayFrame(memory, gameDisplay[back], back, port);
                ++gDownwellClearFieldFrames;
                if (gDownwellClearFieldFrames % 60 == 0) downwellClearProbe();
#endif
            }
            if (bobCount)
                AmigaGameplayActors::draw(renderer, gameDisplay[back], back, port.ringOrigin(), actors, bobCount,
                    activeGamePack, activeGamePackEnd, gameplayRows, gameplayRays[back],
                    rayVisible ? &rayLine : nullptr);
            if (compositeEffects)
                AmigaGameplayEffects::draw(renderer,gameDisplay[back],back,port.ringOrigin(),
                    controllerEffects.surface(),gameplayRows);
            if (frameReady &&
                !gameplayScene.update(back,
                    gameDisplay[back], hudBitmap, playerSprites[back],
                    bulletSprites + back *
                        gameplayRegularSpriteCount,
                    gameplayRegularSpriteCount,
                    enemySprites[back], palette, port.ringOrigin()))
                frameReady = false;
            if(!terrainPaintSucceeded){exitCode=20;break;}
            if (!frameReady) {
                downwellGameplayRenderFailure();
                break;
            }
            const UBYTE* exitMask = nullptr;
            if(actorList.entryCoverVisible&&port.exitDither().active){exitCode=20;break;}
            if(hardwareEntryCover){
                // Two different cover lifetimes cannot share one DMA bank.
                if(port.exitDither().active||!selectEntryMaskBank()||
                   !gameplayScene.entryMask(back,overlayAssetStorage,AmigaEntryMask::bytes,actorList.entryCover.y)){
                    exitCode=20;break;
                }
            }else{
                if(port.exitDither().active){
                    if(!selectExitMaskBank()){exitCode=20;break;}
                    exitMask=overlayAssetStorage+AmigaExitMask::offset(
                        port.exitDither().y-(mainInitialViewY+port.cameraOffsetY()));
                }
                if(!gameplayScene.mask(back,exitMask)){exitCode=20;break;}
            }
#ifdef DOWNWELL_ENTRY_WIPE_VERIFY
            const bool entryAttached=gDownwellEntryWipeFrames<25;
            const int entryY=gDownwellEntryWipeFrames?320+int(gDownwellEntryWipeFrames-1)*16:-116;
            unsigned entryOffset=0;
            if(actorList.entryCoverVisible!=entryAttached||
                (entryAttached&&(!AmigaEntryMask::offset(entryY,entryOffset)||
                    actorList.entryCover.y!=entryY||actorList.entryCover.offset!=entryOffset))){
                exitCode=20;break;
            }
            const auto& entryList=gameplayScene.copper(back);
            const UWORD* entryWords=entryList.data();
            if(entryWords[entryList.moveOffset(0x100)+1]!=(entryAttached?0x6401:0x4401))++gDownwellEntryWipeFailures;
            for(unsigned reg=0xf0;reg<=0xf4;reg+=4){
                const unsigned at=entryList.moveOffset(reg);
                const ULONG pointer=(ULONG(entryWords[at+1])<<16)|entryWords[at+3];
                const ULONG expected=entryAttached?ULONG(overlayAssetStorage+entryOffset+downwellFetchCropBytes):0;
                if(pointer!=expected)++gDownwellEntryWipeFailures;
            }
            if(!entryAttached){
                if(gameplayScene.coverDetached()!=(gDownwellEntryWipeFrames==26))++gDownwellEntryWipeFailures;
                if(selectOverlayAssetBank(true))++gDownwellEntryWipeFailures; // Not fenced yet.
            }
            for(unsigned i=0;i<AmigaEntryMask::bytes;++i)
                if(overlayAssetStorage[i]!=amiga_entry_mask[i])++gDownwellEntryWipeFailures;
            ++gDownwellEntryWipeFrames;
#endif
            /* AmigaPlatform makes the published list live at next VBlank. */
            DW_PROFILE_PHASE(profilePublish);
            platform.publish(gameplayScene.copper(back));
#ifdef DOWNWELL_ENTRY_WIPE_VERIFY
            if(gDownwellEntryWipeFrames==27){
                platform.waitVblank(); // Final detached list is now published.
                if(!gameplayScene.coverDetached()){exitCode=20;break;}
                residentCover=ResidentCover::none;
                if(!selectOverlayAssetBank(true)){exitCode=20;break;}
                for(unsigned i=0;i<gDownwellShopAssetBytes;++i)
                    if(overlayAssetStorage[i]!=amiga_shop_pack[i])++gDownwellEntryWipeFailures;
                ++gDownwellEntryWipeReuses;
                downwellEntryWipeReady();
            }
#endif
#ifdef DOWNWELL_NATURAL_VERIFY
            if (naturalRoute.done) { platform.waitVblank(); downwellNaturalReady(); }
#endif
#ifdef DOWNWELL_EXIT_WIPE_VERIFY
            if (exitMask) {
                ++gDownwellExitWipeFrames;
                const auto& list = gameplayScene.copper(back);
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
#ifdef DOWNWELL_SHOP_VERIFY
            if (shopVerification.frame == 14) downwellShopLiveReady();
#endif
#ifdef DOWNWELL_SCENE_VERIFY
#ifdef DOWNWELL_CAMERA_VERIFY
            static unsigned cameraFramesToCheck = 0;
            if (cameraXChanged) cameraFramesToCheck = 2;
            if (cameraFramesToCheck) {
                // Check the switched frame and the other alternating display.
                AmigaHardware::blitterDrain();
                exposeGameplayFrame(memory, gameDisplay[back], back, port);
                ++gDownwellCameraFrameChecks;
                --cameraFramesToCheck;
            }
#endif
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
            if (gDownwellGemHighCapture == 2) {
                gDownwellGemHighStreak = port.gemStreak();
                unsigned visible = 0;
                for (unsigned i = 0; i < AmigaPortRuntime::bulletCapacity; ++i) {
                    const auto& bullet = port.bulletAt(i);
                    const int x = 80 + bullet.x - port.cameraOffsetX();
                    const int y = gameplayPlayerStartY + bullet.y - port.cameraOffsetY();
                    if (bullet.active && bullet.gemHigh && bullet.animation.frame() != 0 &&
                            x >= 8 && x < 152 && y >= 8 && y < 276)
                        ++visible;
                }
                if (visible) {
                    gDownwellGemHighVisibleShots = visible;
                    gDownwellGemHighCapture = 3;
                    exposeGameplayFrame(memory, gameDisplay[back], back, port);
                    downwellGameplayGemHighFrameReady();
                }
            }
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
#ifdef DOWNWELL_REPLAY_FRAME_TRACE
        if(port.tick()&&port.tick()<=downwell::ReplayFrameTrace::capacity)
            downwell::ReplayFrameTrace::capture(port,gReplayTrace[port.tick()-1]);
#endif
        if (port.tick() == verificationReplay.frameCount()) {
            gReplayFrames = port.tick();
            gReplayDigestHigh = (ULONG)(lastPortCheckpoint >> 32);
            gReplayDigestLow = (ULONG)lastPortCheckpoint;
            gSmallRemainderFailures = downwell::verifySmallUnsignedRemainder();
            downwellReplayCheckStack(1);
            const auto gems = downwell::verifyGemSimulation();
            downwellReplayCheckStack(2);
            gGemReplayDigestHigh = (ULONG)(gems.digest >> 32);
            gGemReplayDigestLow = (ULONG)gems.digest;
            gGemReplayCoverage = gems.coverage;
            const auto death = downwell::verifyPlayerDeath();
            downwellReplayCheckStack(3);
            gDeathReplayDigestHigh = (ULONG)(death.digest >> 32);
            gDeathReplayDigestLow = (ULONG)death.digest;
            gDeathReplayCoverage = death.coverage;
            const auto weapon = downwell::verifyMachineGun();
            downwellReplayCheckStack(4);
            gWeaponReplayDigestHigh = (ULONG)(weapon.digest >> 32);
            gWeaponReplayDigestLow = (ULONG)weapon.digest;
            gWeaponReplayCoverage = weapon.coverage;
            const auto motion = downwell::verifyMotionBinary64();
            downwellReplayCheckStack(5);
            gMotionReplayDigest = motion.digest; gMotionReplayUpdates = motion.updates;
            gMotionReplayFailures = motion.failures;
            const UWORD motionStart = AmigaPlatform::vblankCount();
            const auto motionBench = downwell::benchmarkMotionBinary64();
            downwellReplayCheckStack(6);
            gMotionBenchFields = UWORD(AmigaPlatform::vblankCount() - motionStart);
            gMotionBenchDigest = motionBench.digest; gMotionBenchFailures = motionBench.failures;
            const auto bones = downwell::verifyBoneMotion();
            downwellReplayCheckStack(7);
            gBoneReplayDigest = bones.digest; gBoneReplayUpdates = bones.updates; gBoneReplayFailures = bones.failures;
            const UWORD boneStart = AmigaPlatform::vblankCount();
            const auto boneBench = downwell::verifyBoneMotion(true);
            downwellReplayCheckStack(8);
            gBoneBenchFields = UWORD(AmigaPlatform::vblankCount() - boneStart);
            gBoneBenchDigest = boneBench.digest; gBoneBenchFailures = boneBench.failures;
            const auto createdBones = downwell::verifyBoneCreate();
            downwellReplayCheckStack(9);
            gBoneCreateDigest = createdBones.digest; gBoneCreateCount = createdBones.updates;
            gBoneCreateFailures = createdBones.failures;
            const auto animatedBones=downwell::verifyBoneAnimation();
            downwellReplayCheckStack(10);
            gBoneAnimationDigest=animatedBones.digest; gBoneAnimationCount=animatedBones.updates;
            gBoneAnimationFailures=animatedBones.failures;
            const auto spatial=downwell::verifyRunnerSpatialMutations();
            downwellReplayCheckStack(11);
            gSpatialChecks=spatial.checks;gSpatialDigest=spatial.digest;gSpatialFailures=spatial.failures;
            const auto corpses=downwell::verifyCorpses();
            downwellReplayCheckStack(12);
            gCorpseChecks=corpses.checks;gCorpseDigest=corpses.digest;gCorpseFailures=corpses.failures;
            const auto numbers=downwell::verifyDamageNumbers();
            downwellReplayCheckStack(13);
            gDamageNumberChecks=numbers.checks;gDamageNumberDigest=numbers.digest;gDamageNumberFailures=numbers.failures;
            const auto precise=downwell::verifyPlayerPrecise();
            downwellReplayCheckStack(14);
            gPlayerPreciseChecks=precise.checks;gPlayerPreciseDigest=precise.digest;gPlayerPreciseFailures=precise.failures;
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

int main(void){
    const ULONG lower=(ULONG)SysBase->ThisTask->tc_SPLower;
    const ULONG upper=(ULONG)SysBase->ThisTask->tc_SPUpper;
    if(upper<lower||upper-lower<8192)return 20;
    return runDownwell();
}
