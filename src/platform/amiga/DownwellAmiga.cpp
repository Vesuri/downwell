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
#include "AmigaKeyboard.h"
#include "AmigaSoundBanks.h"
#include "AmigaTables.h"
#include "AmigaDataFiles.h"
#include "../../../generated/amiga-sfx.h"
#include "../../../generated/installed/bind-tables.h"
#ifdef DOWNWELL_SCENE_COLLISION_BACKEND
#include "AmigaEncounterResources.h"
#endif
#include "AmigaFrameBudget.h"
#include "AmigaShopArtSnapshot.h"
#include "AmigaVerificationScratch.h"
#include "AmigaGameplayAssetAdmission.h"
#include "AmigaSceneAdmission.h"
extern "C" { volatile unsigned gDownwellSceneAdmissionFailure; }
#if defined(DOWNWELL_REPLAY_VERIFY) || defined(DOWNWELL_PLANE_BANK_VERIFY)
#include "../../../generated/amiga-game-planes.h"
#endif
#include "../../../generated/amiga-cavern-planes.h"
#ifdef DOWNWELL_INSTALLED_TABLES
#include "../../../generated/installed/downwell-area-titles.h" // WI.3: bound from Downwell.tables
#else
#include "../../../generated/downwell-area-titles.h"
#endif
#ifdef DOWNWELL_INSTALLED_TABLES
#include "../../../generated/installed/downwell-shop-speech.h" // WI.3: bound from Downwell.tables
#else
#include "../../../generated/downwell-shop-speech.h"
#endif
extern "C" int amigaBlitterBindGamePlanes(const UBYTE*,unsigned,const UBYTE*,unsigned,const UBYTE*);
extern "C" void amigaBlitterReleaseGamePlanes();
extern "C" {
volatile unsigned gDownwellGamePlaneResidentBytes, gDownwellGamePlaneLoads, gDownwellGamePlaneReleases,
    gDownwellGamePlaneExpectedBytes;
volatile unsigned gDownwellGameArtResidentBytes, gDownwellGameArtLoads, gDownwellGameArtReleases,
    gDownwellGameArtPreparations, gDownwellGameArtExpectedBytes;
volatile unsigned gDownwellBuilderPatternBytes, gDownwellBuilderPatternLoads, gDownwellBuilderPatternReleases;
volatile unsigned gDownwellTerrainRasterResidentBytes;
__attribute__((noinline, used)) void downwellGamePlanesReady(){ __asm__ volatile("" ::: "memory"); }
}
#ifdef DOWNWELL_PLANE_BANK_VERIFY
extern "C" int amigaBlitterVerifyGamePlanes(const UBYTE*,UBYTE*);
#endif
#include "AmigaHealthHud.h"
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
#include "AmigaInitialTerrain.h"
#ifdef DOWNWELL_TERRAIN_BATCH_VERIFY
extern "C" {
volatile ULONG gDownwellInitialTerrainChecks=0,gDownwellInitialTerrainEnabled=0;
void downwellTerrainBatchMismatch();
}
#endif
#include "AmigaWallCollision.h"
#include "AmigaGameplayActors.h"
#include "AmigaGameplayDrawList.h"
#include "core/TimeShardDraw.h"
#include "AmigaGameplayEffects.h"
#include "AmigaEffectSurface.h"
#include "AmigaEffectStampBank.h"
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
#include "AmigaPauseOverlay.h"
#include "core/PauseMenu.h"
#ifdef DOWNWELL_INSTALLED_TABLES
#include "../../../generated/installed/downwell-menu-text.h" // WI.3: bound from Downwell.tables
#else
#include "../../../generated/downwell-menu-text.h"
#endif
#ifdef DOWNWELL_INSTALLED_TABLES
#include "../../../generated/installed/downwell-palettes.h" // WI.3: bound from Downwell.tables
#else
#include "../../../generated/downwell-palettes.h"
#endif
#ifdef DOWNWELL_PREPARED_FIELDS
#include "AmigaPreparedTimeFields.h"
#include "AmigaFieldColumns.h"
#include "AmigaFieldColumnBank.h"
static AmigaFieldColumnBank gameplayColumnBank;
static_assert(downwell::fieldColumnBytes<=70816,"Field column scene budget needs review");
extern "C" { volatile ULONG gDownwellFieldColumnLoads=0,gDownwellFieldColumnResidentBytes=0; }
// Change only initialized data for identical-code comparisons.
// Both modes load the same images and reserve the same scene-bank partition.
extern "C" { volatile unsigned gDownwellPreparedFieldMode=DOWNWELL_FIELD_MODE; }
extern "C" bool amigaBlitterBindFieldBands(UBYTE*,bool (*)(void*,UBYTE*,unsigned),void*);
extern "C" void amigaBlitterInvalidateFieldBands();
extern "C" int amigaBlitterFieldBands(const UBYTE*,UBYTE*,const downwell::TimeFieldOutlines::Circle&,unsigned,bool,UWORD*);
extern "C" int amigaBlitterRetireFieldBands(const UBYTE*,UBYTE*,const downwell::TimeFieldOutlines::Circle&,const downwell::TimeFieldOutlines::Circle&,unsigned);
extern "C" int amigaBlitterComposeFieldBands(const UBYTE*,UBYTE*,const downwell::TimeFieldOutlines::Circle&,unsigned,UWORD*);
extern "C" int amigaBlitterCaptureField(UBYTE*,const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD);
extern "C" int amigaBlitterRestoreField(const UBYTE*,const UBYTE*,UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD);
extern "C" int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,
    UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int);
#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
#define DOWNWELL_VERIFY_PART 1
#include "DownwellAmigaPreparedFieldsVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
struct PreparedFieldOps {
#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
    static constexpr bool verify=true;
#else
    static constexpr bool verify=false;
#endif
    // Only the verifier cycles modes below columns at run time.
    static constexpr bool referenceFallback=verify||DOWNWELL_FIELD_MODE<4;
    static bool enabled(){return gDownwellPreparedFieldMode!=2;}
    static bool staticEnabled(){return gDownwellPreparedFieldMode>=3;}
    static bool columnsEnabled(){return gDownwellPreparedFieldMode>=4;}
    static bool compositeEnabled(){return gDownwellPreparedFieldMode>=7;}
    static bool retireOutside(const UBYTE* terrain,UBYTE* target,const downwell::TimeFieldOutlines::Circle& old,
        const downwell::TimeFieldOutlines::Circle& next,unsigned origin){return amigaBlitterRetireFieldBands(terrain,target,old,next,origin);}
    static bool compose(const UBYTE* terrain,UBYTE* target,const downwell::TimeFieldOutlines::Circle& old,
        const downwell::TimeFieldOutlines::Circle& next,unsigned origin,UWORD* rows){
#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
        amigaFieldVerifyCompositeTransfer(terrain,target,old,next,origin);
#else
        (void)old;
#endif
        const bool ok=amigaBlitterComposeFieldBands(terrain,target,next,origin,rows);
#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
        if(ok)amigaFieldVerifyFinish(target);
#endif
        return ok;
    }
    static bool columns(const UBYTE* terrain,UBYTE* target,const downwell::TimeFieldOutlines::Circle& circle,
        unsigned origin,bool restore,UWORD* rows){
#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
        amigaFieldVerifyColumn(terrain,target,circle,origin,restore);
#endif
        const bool ok=(gDownwellPreparedFieldMode>=5&&amigaBlitterFieldBands(terrain,target,circle,origin,restore,rows)) ||
            amigaFieldColumns(terrain,target,circle,origin,restore,rows,gameplayColumnBank.data());
#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
        amigaFieldVerifyFinish(target);
#endif
        return ok;
    }
#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
#define DOWNWELL_VERIFY_PART 2
#include "DownwellAmigaPreparedFieldsVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
    static bool restoreTerrain(const UBYTE* source,const UBYTE* mask,UBYTE* target,
        unsigned x,unsigned y,unsigned sx,unsigned sy,unsigned w,unsigned h){
#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
        amigaFieldVerifyTerrain(source,mask,target,x,y,sx,sy,w,h);
#endif
        const bool ok=amigaBlitterRestoreField(source,mask,target,x,y,sx,sy,w,h)!=0;
#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
        amigaFieldVerifyFinish(target);
#endif
        return ok;
    }
    static bool capture(UBYTE* saved,const UBYTE* target,const UBYTE* mask,unsigned x,unsigned y,
        unsigned sx,unsigned sy,unsigned w,unsigned h){
#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
        amigaFieldVerifyCapture(saved,target,mask,x,y,sx,sy,w,h);
#endif
        const bool ok=amigaBlitterCaptureField(saved,target,mask,x,y,sx,sy,w,h)!=0;
#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
        amigaFieldVerifyFinish(saved);
#endif
        return ok;
    }
    static bool masked(const UBYTE* source,const UBYTE* mask,UBYTE* target,
        unsigned x,unsigned y,unsigned sx,unsigned sy,unsigned w,unsigned h){
#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
        amigaFieldVerifyMask(source,mask,target,x,y,sx,sy,w,h);
#endif
        const bool ok=amigaBlitterDrawPlanarMasked(source,mask,64,56,64,target,
            downwellGameRingHeight,x,y,sx,sy,w,h,-1)!=0;
#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
        amigaFieldVerifyFinish(target);
#endif
        return ok;
    }
};
#endif
#include "AmigaLaserSight.h"
#include "AmigaGoalField.h"
#include "AmigaBufferCompare.h"
#include "AmigaSceneRuntime.h"
#include "GameplaySceneStorage.h"
#include "AmigaImmutableUpload.h"
#include "AmigaExitMask.h"
#include "AmigaEntryMask.h"
#include "AmigaClearPresenter.h"
#ifdef DOWNWELL_CLEAR_VERIFY
#include "AmigaClearOracleBuffers.h"
#endif
#include "AmigaClearSceneAssets.h"
#include "AmigaUpgradeScene.h"
#include "AmigaSplashCache.h"
#include "AmigaStyleScene.h"
#include "AmigaPublisherScene.h"
#include "AmigaCreditsScene.h"
#include "AmigaSliceEndScene.h"
#include "AmigaGemStreakHud.h"
#include "AmigaTimerHud.h"
#include "AmigaLevelTitleHud.h"
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
#include "../../../generated/amiga-scene-banks.h"
#include "framework/AmigaHardware.h"
#include "core/GameMakerRounding.h"
#include "core/GameplayContactProfileInput.h"
#include "core/GameplayEntryProfileInput.h"
#include "core/HudNumbers.h"
#include "AmigaHudText.h"
#include "AmigaPreparedHudText.h"
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
// WR.5: the shipping world-1 build runs the compact world without the
// runner emulation. Fixtures that exercise registered families, the scene
// collision backend and COLLISION_OWNER=1 (the owned A/B reference) keep it.
#if defined(DOWNWELL_SCENE_COLLISION_BACKEND) || defined(DOWNWELL_SCENE_VERIFY) || \
    defined(DOWNWELL_FIELD_VERIFY) || defined(DOWNWELL_ENTRY_WIPE_VERIFY) || \
    defined(DOWNWELL_REPLAY_VERIFY) || defined(DOWNWELL_COLLISION_OWNER)
#define DOWNWELL_GAMEPLAY_COLLISION_OWNER 1
#include "core/GameplayEncounterSession.h"
#else
#include "core/EncounterSession.h"
#endif
#include "AmigaSpatialTreeStorage.h"
#include "AmigaSpatialJournalStorage.h"
#ifdef DOWNWELL_LOCAL_DRAW_CAPTURE
namespace downwell { volatile unsigned localDrawCaptureMode=DOWNWELL_LOCAL_DRAW_CAPTURE; }
#endif
#include "core/PortCheckpoint.h"
#include "core/PortReplay.h"
#include "core/RoomPack.h"
#include "core/Replay.h"

#include "core/PlacementNew.h"

extern "C" {
extern const UBYTE amiga_hud_pack[];
extern const UBYTE amiga_hud_pack_end[];
#ifdef DOWNWELL_PREPARED_FIELDS
extern const UBYTE amiga_field_images[],amiga_field_images_end[];
#endif
#ifdef DOWNWELL_REFERENCE_SCENE_BANKS
extern const UBYTE amiga_results_pack[];
extern const UBYTE amiga_results_pack_end[];
extern const UBYTE amiga_shop_pack[];
extern const UBYTE amiga_shop_pack_end[];
#endif
#ifdef DOWNWELL_EXIT_WIPE_VERIFY
extern const UBYTE amiga_exit_mask[];
extern const UBYTE amiga_exit_mask_end[];
#endif
#ifdef DOWNWELL_ENTRY_WIPE_VERIFY
extern const UBYTE amiga_entry_mask[];
extern const UBYTE amiga_entry_mask_end[];
#endif
#ifdef DOWNWELL_SCENE_VERIFY
// Raw masks are consumed only by the retained scene collision oracle below.
// Live precise-mask events must declare a scene-owned dependency when wired;
// they must not restore the whole reference catalogue to shipping startup.
extern const UBYTE amiga_collision_masks[];
extern const UBYTE amiga_collision_masks_end[];
#endif
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
static const UBYTE* activeGamePack = nullptr;
static const UBYTE* activeGamePackEnd = nullptr;
static const UBYTE* activeEntryMask = nullptr;
extern "C" { volatile ULONG gDownwellEntryMaskLoads=0, gDownwellEntryMaskResidentBytes=0; }
static const UBYTE* activeHudPack = nullptr;
static const UBYTE* activeHudPackEnd = nullptr;
static bool entryHudReady=false;

#include "AmigaTransientBank.h"
#include "../../../generated/amiga-transient-banks.h"
using TransientOwner = AmigaTransientBank::Owner;
static constexpr AmigaTransientBank::Contract shopBankContract{
    TransientOwner::shop, AmigaDataFile::shop, DOWNWELL_SHOP_BANK_BYTES, DOWNWELL_SHOP_BANK_BUDGET};
static constexpr AmigaTransientBank::Contract resultsBankContract{
    TransientOwner::results, AmigaDataFile::results, DOWNWELL_RESULTS_BANK_BYTES, DOWNWELL_RESULTS_BANK_BUDGET};
static constexpr AmigaTransientBank::Contract clearBankContract{
    TransientOwner::upgrade, AmigaDataFile::clear, DOWNWELL_CLEAR_BANK_BYTES, DOWNWELL_CLEAR_BANK_BUDGET};
static AmigaTransientBank overlayBank;
static AmigaPlatform* sceneAssetPlatform;
extern "C" { volatile ULONG gDownwellTransientLoads, gDownwellUpgradeSourceBytes; }
extern "C" { volatile ULONG gDownwellExitMaskLoads=0, gDownwellSelectionMaskLoads=0; }
extern "C" { volatile ULONG gDownwellHudLoads=0; }
static_assert(DOWNWELL_HUD_BANK_BYTES<=DOWNWELL_HUD_BANK_BUDGET,
    "HUD growth requires an explicit scene residency budget");

static bool readTransientBank(const char* file, UBYTE* target, unsigned bytes) {
    if (!sceneAssetPlatform || sceneAssetPlatform->loadData(file, target, bytes) != LONG(bytes)) return false;
    ++gDownwellTransientLoads;
    return true;
}
static bool validateTransientBank(const UBYTE* data, unsigned bytes) {
    return amigaRendererChipPackBytes(data, data + bytes) == bytes;
}
#ifdef DOWNWELL_REFERENCE_SCENE_BANKS
static bool sameSceneBank(const UBYTE* loaded, const UBYTE* reference, unsigned bytes) {
    for (unsigned i=0; i<bytes; ++i) if (loaded[i] != reference[i]) return false;
    return true;
}
#endif
// Ordinary source memory exists only during upgrade selection. Selected Chip
// records are owned by AmigaUpgradeScene, and have a separate DMA lifetime.
class UpgradeSource {
    UBYTE* storage_ = nullptr;
    AmigaTransientBank bank_;
    static constexpr unsigned storageBytes=clearBankContract.bytes+DOWNWELL_SELECTIONMASK_BANK_BYTES;
public:
    ~UpgradeSource() { release(); }
    bool acquire() {
        if (bank_.view(TransientOwner::upgrade).data) return true;
        if (!storage_) storage_ = static_cast<UBYTE*>(AllocMem(storageBytes, MEMF_PUBLIC));
        if (!storage_ || !bank_.load(TransientOwner::upgrade, clearBankContract, storage_,
                clearBankContract.bytes, readTransientBank, [](const UBYTE* data,unsigned bytes){
                    // Validate the full directory, including the CPU-resident
                    // player DMA streams excluded from the Chip-only subset.
                    return amigaRendererChipPackBytes(data,data+bytes)!=0;
                })) {
            release(); return false;
        }
        // Upgrade preparation copies this source into its own DMA bank. It
        // must never become a permanent linked asset or a gameplay load.
        if (!sceneAssetPlatform || sceneAssetPlatform->loadData(AmigaDataFile::selectionMask,
                storage_+clearBankContract.bytes,DOWNWELL_SELECTIONMASK_BANK_BYTES)!=
                LONG(DOWNWELL_SELECTIONMASK_BANK_BYTES)) { release(); return false; }
        ++gDownwellSelectionMaskLoads;
        gDownwellUpgradeSourceBytes = storageBytes;
        return true;
    }
    AmigaTransientBank::View view() const { return bank_.view(TransientOwner::upgrade); }
    const UBYTE* mask() const { return storage_?storage_+clearBankContract.bytes:nullptr; }
    void release() {
        amigaRendererReleaseHardwareSpriteCopies();
        bank_.release();
        if (storage_) FreeMem(storage_, storageBytes);
        storage_ = nullptr; gDownwellUpgradeSourceBytes = 0;
    }
};

// Boot (splash) and front
// (garden and style select) banks are files, each read whole in one load
// into one shared Chip region. Boot art is composed into the splash cache
// and dropped; front art is reloaded whenever gameplay returns to it.
// Gameplay reuses the region for terrain/field staging. The separate gameplay
// DMA atlas is a separate gameplay lease, absent during front-end scenes.
static const char* const bootBankName = AmigaDataFile::boot;
static const char* const frontBankName = AmigaDataFile::front;
static const ULONG baseSceneBankBytes = DOWNWELL_FRONT_BANK_BYTES > DOWNWELL_BOOT_BANK_BYTES
    ? DOWNWELL_FRONT_BANK_BYTES : DOWNWELL_BOOT_BANK_BYTES;
#ifdef DOWNWELL_PREPARED_FIELDS
static const ULONG fieldBandOffset=75360;
static_assert(downwell::fieldBandBytes<=71456,
    "Large-field bank exceeds its reviewed scene budget");
static const ULONG sceneBankBytes=fieldBandOffset+downwell::fieldBandBytes+sizeof(UWORD)>baseSceneBankBytes?
    fieldBandOffset+downwell::fieldBandBytes+sizeof(UWORD):baseSceneBankBytes;
#else
static const ULONG sceneBankBytes=baseSceneBankBytes;
#endif
static UBYTE* sceneBank;
static const UBYTE* frontPack;
static const UBYTE* frontPackEnd;
static void invalidateGameplayFieldBank();
extern "C" volatile ULONG gDownwellSceneBankLoads;
volatile ULONG gDownwellSceneBankLoads;
extern "C" volatile ULONG gDownwellSplashPreparations, gDownwellGardenPreparations, gDownwellStartupScene;
volatile ULONG gDownwellSplashPreparations, gDownwellGardenPreparations, gDownwellStartupScene;
extern "C" volatile ULONG gDownwellInitialRoomPreparations;
volatile ULONG gDownwellInitialRoomPreparations;
#ifdef DOWNWELL_PROFILE_IDLE
extern "C" volatile ULONG gDownwellIdleInputViolations, gDownwellIdleBulletSlots;
volatile ULONG gDownwellIdleInputViolations, gDownwellIdleBulletSlots;
#endif
static bool loadSceneBank(AmigaPlatform& platform, const char* name, ULONG bytes) {
    AmigaHardware::blitterDrain();
    invalidateGameplayFieldBank();
    frontPack = frontPackEnd = nullptr;
    if (!sceneBank || bytes > sceneBankBytes ||
        platform.loadData(name, sceneBank, bytes) != (LONG)bytes ||
        amigaRendererChipPackBytes(sceneBank, sceneBank + bytes) != bytes) return false;
    ++gDownwellSceneBankLoads;
    return true;
}
static bool loadFrontBank(AmigaPlatform& platform) {
    if (!loadSceneBank(platform, frontBankName, DOWNWELL_FRONT_BANK_BYTES)) return false;
    frontPack = sceneBank;
    frontPackEnd = sceneBank + DOWNWELL_FRONT_BANK_BYTES;
    return true;
}

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

// A palette remaps the build's black, white and red (scrShaderListInit).
static uint16_t selectedPaletteData[32];
static void selectPaletteData(unsigned shader) {
    const auto& entry = downwell::palettes::entries[shader < downwell::palettes::count ? shader : 0];
    for (unsigned i = 0; i < 32; ++i) selectedPaletteData[i] = entry.remap(downwellPaletteData[i]);
}

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
// Gameplay-loop RETURN_FAIL exits record their source line for the native
// verifiers (break on downwellGameplayFailed).
volatile ULONG gDownwellGameplayFailLine;
__attribute__((noinline, used)) int downwellGameplayFailed(unsigned line) {
    gDownwellGameplayFailLine = line; __asm__ volatile("" ::: "memory"); return 20;
}
#ifdef DOWNWELL_ENTRY_WIPE_VERIFY
volatile ULONG gDownwellEntryWipeFrames,gDownwellEntryWipeFailures,gDownwellEntryWipeReuses,gDownwellEntryLifetimeChecks;
__attribute__((noinline, used)) void downwellEntryWipeReady(){__asm__ volatile("" ::: "memory");}
#endif
#ifdef DOWNWELL_EXIT_WIPE_VERIFY
volatile ULONG gDownwellExitWipeFrames, gDownwellExitWipeFailures;
__attribute__((noinline, used)) void downwellExitWipeReady() { __asm__ volatile("" ::: "memory"); }
#endif
static AmigaShopMenu shopMenuOverlay;
#ifdef DOWNWELL_PROFILE_OVERFLOW
#include "AmigaOverflowProfileEntry.h"
#endif
#ifdef DOWNWELL_OVERFLOW_VERIFY
#include "AmigaOverflowWitness.h"
#include "../../../generated/overflow-witness.h"
volatile ULONG gDownwellOverflowChecks,gDownwellOverflowFailureFrame,gDownwellOverflowFailureGroup;
volatile ULONG gDownwellOverflowActual[2];
__attribute__((noinline, used)) void downwellOverflowMismatch(){__asm__ volatile("" ::: "memory");}
__attribute__((noinline, used)) void downwellOverflowProgress(){__asm__ volatile("" ::: "memory");}
#endif
#if defined(DOWNWELL_NATURAL_VERIFY) || defined(DOWNWELL_PROFILE_SHOP)
#include "AmigaNaturalRouteVerification.h"
#endif
#ifdef DOWNWELL_PROFILE_SHOP
#include "core/GameplayShopProfileInput.h"
#include "../../../generated/shop-profile-input.h"
static downwell::GameplayShopProfileInput shopProfileInput(
    downwell::shopProfileLevel1,sizeof downwell::shopProfileLevel1,
    downwell::shopProfileLevel2,sizeof downwell::shopProfileLevel2);
extern "C" {
extern volatile ULONG gDownwellProfileTargetFrames,gDownwellProfileFrames[4];
volatile ULONG gDownwellShopProfileFrames[2],gDownwellShopProfileFailures;
volatile ULONG gDownwellShopProfileSideFrames,gDownwellShopProfileMenuFrames,gDownwellShopProfileDone;
const UBYTE* volatile gDownwellShopHud=nullptr;
volatile ULONG gDownwellShopHudBytes=0;
}
#endif
#ifdef DOWNWELL_NATURAL_VERIFY
static AmigaNaturalRouteVerification naturalRoute;
volatile ULONG gDownwellNaturalExitTick;
volatile ULONG gDownwellNaturalFailures, gDownwellNaturalReturnFrames;
__attribute__((noinline, used)) void downwellNaturalReady() { __asm__ volatile("" ::: "memory"); }
#endif
#ifdef DOWNWELL_CLEAR_VERIFY
#define DOWNWELL_VERIFY_PART 3
#include "DownwellAmigaClearVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif

// Results and shop are mutually exclusive users of the retired-garden gap.
// Drain the blitter before replacing a bank it may still be reading. Hardware
// sprites and display bitplanes never point into this immutable artwork region.
static bool selectOverlayAssetBank(bool shop) {
    if (residentCover!=ResidentCover::none) return false; // DMA owns this bank until scene retirement.
    const auto& contract = shop ? shopBankContract : resultsBankContract;
    const auto held = overlayBank.view(contract.owner);
    if (held.data == overlayAssetStorage && held.bytes == contract.bytes) return true;
    AmigaHardware::blitterDrain();
    gDownwellShopPack = gDownwellResultsPack = nullptr;
    gDownwellShopAssetBytes = gDownwellResultsAssetBytes = 0;
    if (!overlayBank.load(contract.owner, contract, overlayAssetStorage, overlayAssetCapacity,
            readTransientBank, validateTransientBank)) return false;
#ifdef DOWNWELL_REFERENCE_SCENE_BANKS
    const UBYTE* source = shop ? amiga_shop_pack : amiga_results_pack;
    if (!sameSceneBank(overlayAssetStorage, source, contract.bytes)) { overlayBank.release(); return false; }
#endif
    gDownwellShopPack = shop ? overlayAssetStorage : nullptr;
    gDownwellShopAssetBytes = shop ? contract.bytes : 0;
    gDownwellResultsPack = shop ? nullptr : overlayAssetStorage;
    gDownwellResultsAssetBytes = shop ? 0 : contract.bytes;
    return true;
}
} // extern "C": the W1.10 helpers below have C++ linkage.
// W1.10 objPauseMenu and its sub-menus: the pause bank (font-2 glyph masks,
// sprDitherFade, sprProgress) borrows the same retired-garden gap. A living,
// controllable player owns neither shop nor results art there.
static constexpr AmigaTransientBank::Contract pauseBankContract{
    TransientOwner::pause, AmigaDataFile::pause, DOWNWELL_PAUSE_BANK_BYTES, DOWNWELL_PAUSE_BANK_BUDGET};
static AmigaTransientBank::View selectPauseBank() {
    if (residentCover != ResidentCover::none || !overlayAssetStorage) return {nullptr, 0};
    const auto held = overlayBank.view(TransientOwner::pause);
    if (held.data == overlayAssetStorage) return held;
    AmigaHardware::blitterDrain();
    gDownwellShopPack = gDownwellResultsPack = nullptr;
    gDownwellShopAssetBytes = gDownwellResultsAssetBytes = 0;
    if (!overlayBank.load(TransientOwner::pause, pauseBankContract, overlayAssetStorage,
            overlayAssetCapacity, readTransientBank, validateTransientBank)) return {nullptr, 0};
    return overlayBank.view(TransientOwner::pause);
}

// W1.10 save.ini: scrLoadSave at startup; the original's ini_write_real
// moments are batched into whole-file writes at the same boundaries (results,
// pause-menu changes, style confirmation, the slice end and quitting).
static downwell::SaveFile saveState, savedState;
static bool saveOnDisk = false;
static downwell::PauseMenu pauseMenu;
static downwell::PauseMenuText pauseText;
// Scratch HUD layer for the pause menus, held only while paused.
static UBYTE* pauseScratch = nullptr;
static bool pauseTextReady = false;
static AmigaPauseOverlay::Key pauseKey;
static unsigned pauseRunQuarters = ~0u;
static void loadSave(AmigaPlatform& platform) {
    char text[downwell::SaveFile::formatCapacity * 2];
    const LONG read = platform.loadFile("PROGDIR:save.ini", (UBYTE*)text, sizeof text);
    saveOnDisk = read > 0;
    saveState = saveOnDisk ? downwell::SaveFile::parse(text, unsigned(read)) : downwell::SaveFile{};
    savedState = saveState;
}
static void flushSave(AmigaPlatform& platform) {
    if (saveOnDisk && saveState == savedState) return;
    char text[downwell::SaveFile::formatCapacity];
    const unsigned bytes = saveState.format(text, sizeof text);
    if (bytes && platform.saveFile("PROGDIR:save.ini", (const UBYTE*)text, bytes)) {
        savedState = saveState; saveOnDisk = true;
    }
}
// saveStats / comboDone: lifetime gems, best run gems and best combo live in
// the run; checkAreaLevel's furthest stage is recorded at the end of a run.
// checkAreaLevel records the furthest level as each level starts, so a quit
// mid-level keeps it too.
template<class Port> static void syncRunRecords(const Port& port) {
    const auto records = port.runRecords();
    saveState.totalGems = records.lifetimeGems;
    saveState.bestRunGems = records.bestRunGems;
    saveState.bestCombo = records.bestCombo;
    const unsigned reached = unsigned(port.runStage().area * 10 + port.runStage().level) +
        (port.hardMode() ? 100u : 0u);
    if (reached > saveState.furthestReached) saveState.furthestReached = reached;
}

extern "C" {
static bool selectExitMaskBank() {
    if (residentCover==ResidentCover::exit) return true;
    if (residentCover!=ResidentCover::none) return false;
    if (!overlayAssetStorage || AmigaExitMask::bytes > overlayAssetCapacity ||
        DOWNWELL_EXITMASK_BANK_BYTES != AmigaExitMask::bytes || !sceneAssetPlatform) return false;
    AmigaHardware::blitterDrain();
    overlayBank.release();
    gDownwellResultsPack = gDownwellShopPack = nullptr;
    gDownwellResultsAssetBytes = gDownwellShopAssetBytes = 0;
    if (sceneAssetPlatform->loadData(AmigaDataFile::exitMask,overlayAssetStorage,
            AmigaExitMask::bytes)!=LONG(AmigaExitMask::bytes)) return false;
    ++gDownwellExitMaskLoads;
    residentCover = ResidentCover::exit;
    return true;
}
#ifdef DOWNWELL_PROFILE_ENTRY
extern "C" {
volatile ULONG gDownwellEntryResultsDiscarded=0;
volatile ULONG gDownwellEntryResultsFrames=0;
const UBYTE* volatile gDownwellEntryHud=nullptr;
volatile ULONG gDownwellEntryHudBytes=0;
const UBYTE* volatile gDownwellEntryFields=nullptr;
volatile ULONG gDownwellEntryFieldBytes=0;
}
#endif
static bool selectEntryMaskBank(){
    if(residentCover==ResidentCover::entry)return true;
    if(residentCover!=ResidentCover::none||!overlayAssetStorage||AmigaEntryMask::bytes>overlayAssetCapacity||
       !activeEntryMask||gDownwellEntryMaskResidentBytes!=AmigaEntryMask::bytes)return false;
    AmigaHardware::blitterDrain();
    overlayBank.release();
#ifdef DOWNWELL_PROFILE_ENTRY
    if(gDownwellResultsPack)++gDownwellEntryResultsDiscarded;
#endif
    for(unsigned i=0;i<AmigaEntryMask::bytes;++i)overlayAssetStorage[i]=activeEntryMask[i];
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
#define DOWNWELL_VERIFY_PART 4
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
#else
static inline void checkSceneStack(ULONG){}
#endif

#ifdef DOWNWELL_REPLAY_VERIFY
#define DOWNWELL_VERIFY_PART 5
#include "DownwellAmigaReplayVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif

#ifndef DOWNWELL_REPLAY_VERIFY
extern "C" __attribute__((noinline, used)) void downwellStartupReady(void) {
    __asm__ volatile("" ::: "memory");
}

extern "C" __attribute__((noinline, used)) void downwellFrameLoopReady(void) {
    __asm__ volatile("" ::: "memory");
}

extern "C" __attribute__((noinline, used)) void downwellWorldOneComplete(void) {
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
#ifdef DOWNWELL_CELL_REPAIR_VERIFY
volatile ULONG gDownwellCellRepairChecks;
volatile ULONG gDownwellCellRepairMerges;
volatile ULONG gDownwellCellRepairFailures;
#endif
volatile const UBYTE* gDownwellSplashFrame;
volatile const UBYTE* gDownwellControlsFrame;
volatile const UWORD* gDownwellControlsCopper;
volatile const UBYTE* gDownwellCreditsFrame;
volatile const UWORD* gDownwellCreditsCopper;
#ifdef DOWNWELL_SCENE_VERIFY
#define DOWNWELL_VERIFY_PART 6
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
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
#define DOWNWELL_VERIFY_PART 7
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
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

enum { currencyShift = AmigaHudText::currencyShift };

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
#ifdef DOWNWELL_AMMO_FACTORED
volatile unsigned amigaAmmoFactoredMode=DOWNWELL_AMMO_FACTORED;
#endif
#ifdef DOWNWELL_AMMO_FACTORED_VERIFY
extern "C" {
volatile ULONG gDownwellAmmoChecks,gDownwellAmmoFailures,gDownwellAmmoFactoredUses;
}
alignas(2) static UBYTE ammoExpected[downwellLayerBytes];
static AmigaAmmoHud ammoOriginal;
#endif
static void redrawAmmo(AmigaRenderer* renderer, unsigned ammo, unsigned maximumAmmo, unsigned offset = 0,
                       const downwell::AmmoGaugeRows* rows = nullptr, unsigned meterOffset = 0) {
#ifdef DOWNWELL_AMMO_FACTORED_VERIFY
    for(unsigned i=0;i<downwellLayerBytes;++i)ammoExpected[i]=renderer->hud[i];
#endif
#ifdef DOWNWELL_AMMO_FACTORED
    ammoHud.sharedRowProducer(true);
#endif
    ammoHud.draw(renderer, activeHudPack, activeHudPackEnd, ammo, maximumAmmo, offset, rows, meterOffset);
#ifdef DOWNWELL_AMMO_FACTORED_VERIFY
    AmigaRenderer oracle=*renderer;
    oracle.target=oracle.hud=ammoExpected;oracle.blitterEnabled=0;
    ammoOriginal.draw(&oracle,activeHudPack,activeHudPackEnd,ammo,maximumAmmo,offset,rows,meterOffset,true);
    ++gDownwellAmmoChecks;
    for(unsigned i=0;i<downwellLayerBytes;++i)
        if(ammoExpected[i]!=renderer->hud[i])++gDownwellAmmoFailures;
#endif
    DW_PROFILE_PHASE(profileTitle);
}

static AmigaHealthHud healthHud;
static void redrawHealth(AmigaRenderer* renderer, unsigned health, unsigned maximum,
                          unsigned pieces = 0, unsigned perHeart = 4) {
    healthHud.draw(renderer,activeHudPack,activeHudPackEnd,health,maximum,pieces,perHeart,
#ifdef DOWNWELL_RETAIN_HEALTH_HUD
        false
#else
        true
#endif
    );
}

#if defined(DOWNWELL_REFERENCE_HUD_WORDS) || defined(DOWNWELL_REFERENCE_HUD_TEXT)
static AmigaHudText hudText;
#else
static AmigaPreparedHudText hudText;
// The prepared tables bind only packs of the sizes they were derived from;
// any other size falls back to the reference on every redraw.
static_assert(DOWNWELL_GAME_BANK_BYTES==downwellHudPrepared::gameBytes&&
    DOWNWELL_CAVERN_BANK_BYTES==downwellHudPrepared::cavernBytes,
    "Prepared HUD text derived from other game art banks");
#endif
static void invalidateHudText(bool all = false) {
#if !defined(DOWNWELL_REFERENCE_HUD_WORDS) && !defined(DOWNWELL_REFERENCE_HUD_TEXT)
    if(all) hudText.invalidate();
    else hudText.invalidateStreak();
#else
    (void)all;
#endif
}
#ifdef DOWNWELL_FRAME_DUMP
extern "C" { volatile ULONG gDownwellHudWordFallbacks=0; }
#endif
static void redrawCurrency(AmigaRenderer* renderer, unsigned currency) {
#ifndef DOWNWELL_REFERENCE_HUD_TEXT
    if (hudText.bind(activeHudPack, activeHudPackEnd, activeGamePack, activeGamePackEnd) &&
        hudText.currency(renderer, currency)) return;
#if defined(DOWNWELL_FRAME_DUMP) && !defined(DOWNWELL_REFERENCE_HUD_WORDS)
    ++gDownwellHudWordFallbacks;
#endif
#endif
    AmigaHudText::currencyReference(renderer, activeHudPack, activeHudPackEnd, currency);
}

// scrDrawHud4x3's owned upgrades: global.ug[ugOrder[i]][2] frame 1 at
// (-98 + 16 * (i - 1), g_cameraHeight - 12) from the playfield's left edge,
// the seventh onwards 178 pixels further right. Later icons cover earlier.
static unsigned shownUpgradeCount=0;
static void redrawUpgradeIcons(AmigaRenderer* renderer, const downwell::UpgradeInventory& upgrades) {
    static const UWORD icons[23]={524,528,512,520,518,521,522,506,507,508,509,510,511,
                                  516,523,525,529,530,531,532,533,534,535};
    amigaRendererClearClipRect(renderer);
    amigaRendererFillRect(renderer, 0, 260, downwellPlayfieldX, 24, 0);
    amigaRendererFillRect(renderer, downwellPlayfieldX + downwellPlayfieldWidth, 260,
                          downwellDmaWidth - downwellPlayfieldX - downwellPlayfieldWidth, 24, 0);
    for (unsigned i = 1; i <= upgrades.count && i < sizeof(upgrades.order); ++i) {
        const unsigned id = upgrades.order[i];
        if (id >= sizeof(icons) / sizeof(icons[0])) continue;
        int x = downwellPlayfieldX - 98 + 16 * int(i - 1);
        if (i > 6) x += 178;
        if (x - 12 >= downwellDmaWidth) continue;
        drawGameAsset(renderer, icons[id], 1, x, downwellDisplayHeight - 12, 0);
    }
    shownUpgradeCount = upgrades.count;
}
static int streakYOffset=0;
// W1.7: the whole scrDrawHud4x3Top streak block (bar, text, gem high).
static AmigaGemStreakHud gemStreakHud;
static AmigaGemStreakHud::View streakView;
// W1.7: levelNumber on the fixed HUD layer.
static AmigaLevelTitleHud titleHud;
static AmigaLevelTitleHud::View titleView;
static void invalidateTitle() { titleHud.invalidate(); titleView = {}; }
// W1.10: Options > Timer in the left HUD column.
static AmigaTimerHud timerHud;
static unsigned shownCurrencyText=0;
// A fixed ordinary streak (no bar line) at the current offset, for scene
// setup and the shop fixture.
static AmigaGemStreakHud::View plainStreakView(unsigned streak) {
    AmigaGemStreakHud::View v;
    v.visible=streak!=0;v.streak=streak;v.drawY=50+streakYOffset;
    return v;
}
static void composeStreak(AmigaRenderer* renderer, const AmigaGemStreakHud::View& view) {
    invalidateHudText();
    if(gemStreakHud.bind(activeGamePack, activeGamePackEnd))(void)gemStreakHud.compose(renderer,view);
}
extern "C" { volatile ULONG gDownwellStreakFailure; volatile LONG gDownwellStreakFailureView[6]; } // 1 bind, 2 draw; +16 renderer->hud null
static bool redrawStreak(AmigaRenderer* renderer, const AmigaGemStreakHud::View& view) {
    if(!gemStreakHud.bind(activeGamePack, activeGamePackEnd)){gDownwellStreakFailure=1;return false;}
    if(!gemStreakHud.draw(renderer,view)){
        gDownwellStreakFailure=2+(renderer&&renderer->hud?0:16);
        gDownwellStreakFailureView[0]=view.high;gDownwellStreakFailureView[1]=LONG(view.streak);
        gDownwellStreakFailureView[2]=view.line;gDownwellStreakFailureView[3]=view.cap;
        gDownwellStreakFailureView[4]=view.motion;gDownwellStreakFailureView[5]=view.drawY;
        return false;
    }
    return true;
}
static void redrawStreak(AmigaRenderer* renderer, unsigned streak) {
    (void)redrawStreak(renderer,plainStreakView(streak));
}

static void composeDeathFlash(AmigaRenderer* renderer, bool visible) {
    invalidateHudText();
    gemStreakHud.invalidate();
    invalidateTitle();
    amigaRendererClearClipRect(renderer); // selects the non-scrolling HUD layer
    amigaRendererFillRect(renderer, downwellPlayfieldX, 0, 160, 284, visible ? 1 : 0);
}

static void composeResultsBackdrop(AmigaRenderer* renderer,
                                    int offset, int viewX, int viewY,
                                    int top = 0, int bottom = 284) {
    invalidateHudText();
    invalidateTitle();
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
    const int viewX = mainViewX + port.cameraSteadyOffsetX();
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

static bool redrawResultsRows(AmigaRenderer* renderer, const AmigaPortRuntime& port, bool force) {
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
        lastCombo == bestCombo && lastArea == port.runStage().area && lastLevel == port.runStage().level) return false;
    lastGems = gems; lastKills = kills; lastPhase = phase;
    lastTime = ticks;
    lastCombo = bestCombo;
    lastArea = port.runStage().area; lastLevel = port.runStage().level;
    composeResultsRows(renderer, menu, gems, kills, ticks, bestCombo, port.runStage());
    return true;
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

static bool redrawResultsProgress(AmigaRenderer* renderer,
        const downwell::DeathResults& results, bool force) {
    static downwell::DeathResultsProgressBar previous;
    static std::uint64_t previousTotal = 0, previousGoal = 0;
    const auto bar = downwell::DeathResultsProgressBar::from(results);
    if (!force && bar == previous && results.frame.displayedTotal == previousTotal &&
        results.frame.nextGoal == previousGoal) return false;
    previous = bar;
    previousTotal = results.frame.displayedTotal; previousGoal = results.frame.nextGoal;
    composeResultsProgress(renderer, bar, results);
    return true;
}

// W1.10 DeathMenu Draw's unlock notice: at goalNotif >= 2 sprShopDesc 489
// slides up from view y + 84 to + 80, at >= 3 the STYLE/PALETTE UNLOCKED
// header (scrGemmyText) and the name (scrDrawBorderTextBlack, from the
// 64800 + code glyphs) slide up four pixels, with the style's run sprite
// (25, 26, 28, 29) between them. It is drawn over the rows and the progress
// bar; unlockNotice(gr) names step gr + 1: styles at steps 3, 7, 11 and 13,
// otherwise the next palette. Its sound is the results 329 (RunSession).
struct ResultsNotice {
    int stage = 0, desc = 4, text = 4;
    unsigned frameTenths = 0; // styleUnlockSpriteFrame, never reset by Create
    bool operator==(const ResultsNotice& o) const {
        return stage == o.stage && desc == o.desc && text == o.text &&
            (stage < 3 || frameTenths / 10 == o.frameTenths / 10);
    }
};
static ResultsNotice resultsNotice, shownResultsNotice;
static void composeResultsNotice(AmigaRenderer* renderer, const ResultsNotice& notice, unsigned goal) {
    if (notice.stage < 2) return;
    UBYTE* saved = renderer->target;
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, 0, 160, 284);
    renderer->target = renderer->hud;
    const UBYTE* pack = gDownwellResultsPack;
    const UBYTE* end = gDownwellResultsPack + gDownwellResultsAssetBytes;
    const int uny = 80;
    amigaRendererDrawAsset(renderer, pack, end, 489, 0, downwellPlayfieldX, uny + notice.desc, 0);
    if (notice.stage >= 3) {
        const unsigned step = goal + 1;
        const bool style = downwell::UnlockProgress::styleStep(step);
        const int top = uny + (style ? 20 : 16) + notice.text - 4;
        // scrGemmyText: red (-1,0), (0,-1); black (1,0), (0,1); white.
        static const signed char dx[5] = {-1, 0, 1, 0, 0}, dy[5] = {0, -1, 0, 1, 0};
        static const unsigned char pen[5] = {2, 2, 3, 3, 1};
        for (unsigned layer = 0; layer < 5; ++layer)
            amigaRendererDrawAssetPen(renderer, pack, end, style ? 65305 : 65306, 0,
                downwellPlayfieldX + 80 + dx[layer], top + dy[layer], 0, pen[layer]);
        const char* name = nullptr;
        if (style) {
            const unsigned index = step == 3 ? 0 : step == 7 ? 1 : step == 11 ? 2 : 3;
            static const unsigned sprites[4] = {25, 26, 28, 29};
            amigaRendererDrawAsset(renderer, pack, end, sprites[index], (notice.frameTenths / 10) % 8,
                downwellPlayfieldX + 80, top + 32, 0);
            name = pauseTextReady ? pauseText.styleName[index + 1] : nullptr;
        } else {
            const unsigned palette = downwell::UnlockProgress::from(
                downwell::DeathMenuProgress::goals[goal]).paletteUnlock;
            if (palette < downwell::palettes::count)
                name = downwell::palettes::entries[palette].name;
        }
        if (name) {
            unsigned length = 0;
            while (name[length]) ++length;
            const int left = 80 - 4 * int(length);
            const int y = top + (style ? 48 : 34);
            static const signed char bx[5] = {1, -1, 0, 0, 0}, by[5] = {0, 0, 1, -1, 0};
            for (unsigned layer = 0; layer < 5; ++layer)
                for (unsigned i = 0; i < length; ++i) {
                    const unsigned char c = (unsigned char)name[i];
                    if (c <= ' ' || c >= 127) continue;
                    amigaRendererDrawAssetPen(renderer, pack, end, UWORD(64800 + c), 0,
                        downwellPlayfieldX + left + 8 * int(i) + bx[layer], y + by[layer], 0,
                        layer == 4 ? 1 : 3);
                }
        }
    }
    amigaRendererClearClipRect(renderer);
    renderer->target = saved;
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
                downwellPlayfieldX + std::uint16_t(xTenths) / 10 + halfWidths[index], y, 0, pen); // 150..332
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
    composeResultsChoices(renderer, menu, mainViewX + port.cameraSteadyOffsetX(),
        mainInitialViewY + port.cameraOffsetY());
}



// Each display ring independently owes modified rows of the private backing.
static AmigaGameplayRows gameplayRows;
static AmigaLaserSight gameplayRays[2];
#ifdef DOWNWELL_PREPARED_FIELDS
static AmigaPreparedTimeFieldPair<PreparedFieldOps> gameplayFields;
static void invalidateGameplayFieldBank(){
    gameplayFields.invalidateBank();
    amigaBlitterInvalidateFieldBands();
}
extern "C" volatile ULONG gDownwellFieldBankUploads,gDownwellFieldBankReuses;
volatile ULONG gDownwellFieldBankUploads,gDownwellFieldBankReuses;
#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
#define DOWNWELL_VERIFY_PART 8
#include "DownwellAmigaPreparedFieldsVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
#else
static AmigaTimeFieldPair gameplayFields;
static void invalidateGameplayFieldBank(){}
#endif
static Bitmap* terrainCopyTarget;
static const Bitmap* terrainCopyBacking;
static unsigned terrainCopyBuffer;

// Three camera columns (left room, well, right room), word-aligned around
// logical x=110..269. The level raster is Fast RAM; only exposed rows enter
// the Chip-RAM rings. Keep the original compositor as the cache oracle.
enum { terrainRasterRows = 8192, terrainRasterWords = AmigaGameplayRows::terrainWords,
       terrainRasterX = AmigaGameplayRows::terrainX };
static AmigaTerrainRaster terrainRaster{};
static downwell::PreparedRoomTerrain roomTerrainPlan;
// Static lifetime: compiled placement pointers must not consume the 8 KiB
// task stack or outlive the index/source they reference.
static AmigaRoomTerrainPlan roomTerrainBackend;
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
extern "C" { volatile ULONG gDownwellRoomPlanFields; }
#endif
#ifdef DOWNWELL_PROBES
extern "C" { volatile ULONG gDownwellRoomPlanTicks; }
#endif
static AmigaInitialTerrain initialTerrain;
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
#ifdef DOWNWELL_PREPARED_PATTERN_ROWS
static AmigaFrameBudget::Stamp frameWorkStamp(){
    UWORD field,high,low,again;
    do {
        field=AmigaPlatform::vblankCount();
        high=*vposrPointer;low=*vhposrPointer;again=*vposrPointer;
    }while(field!=AmigaPlatform::vblankCount()||((high^again)&1));
    return {field,(ULONG(high&1)<<16)|low};
}
// Prepare complete tile rows once, independent of the smaller strips exposed
// by the camera. Live mutations invalidate this same raster before reuse.
static bool preparePatternCache(AmigaRenderer* renderer,const downwell::RoomPack& rooms,
    const downwell::PackedRoom& room,const AmigaTerrainDecoration* decorations,unsigned decorationCount,
    const AmigaPortRuntime* runtime,int viewX,int first,int end){
    if(!terrainCollision.index||viewX!=mainViewX||first<8||end>terrainRasterRows-8)return false;
    for(int y=((first+8)&~15)-8;y<end;y+=16){
        bool dirty=false;
        for(int i=0;i<16&&!dirty;++i)dirty=terrainRaster.dirty(1,y+i);
        if(!dirty)continue;
        int start=y;
        // Cold visible rows use the same admitted composition as paint-ahead:
        // one admission and at most two eight-line pieces. Unsupported art
        // still falls through to the portable painter without acknowledging it.
        for(unsigned piece=0;piece<3;++piece){
            while(start<y+16&&!terrainRaster.dirty(1,start))++start;
            if(start==y+16)break;
            if(!AmigaTerrainPainter::preparePatternPiece(terrainRaster,*terrainCollision.index,
                    terrainTiles,decorations,decorationCount,runtime,viewX,start))break;
        }
        while(start<y+16&&!terrainRaster.dirty(1,start))++start;
        if(start==y+16)continue;
        int count=y+16-start;
        if(!AmigaTerrainPainter::paintCached(terrainRaster.writeRows(1,start,count),renderer,
                rooms,room,terrainCollision.index,terrainTiles,decorations,decorationCount,
                runtime,activeGamePack,activeGamePackEnd,viewX,start,0,count))return false;
        for(int i=0;i<count;++i)terrainRaster.setDirty(1,start+i,false);
    }
    return true;
}
#endif
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
        std::int16_t(viewX - mainViewX) % 160 == 0 && firstWorldY >= 0 &&
        finalWorldY <= terrainRasterRows;
    if (terrainRaster.ready && cacheable && !refresh) {
        const int column = std::uint16_t(viewX - mainViewX + 160) / 160; // cacheable: 0..320
#ifdef DOWNWELL_PREPARED_PATTERN_ROWS
        if(column==1&&clipX==downwellPlayfieldX&&clipWidth==downwellPlayfieldWidth&&
           (terrainRaster.dirty(column,firstWorldY)||
            terrainRaster.runEnd(column,firstWorldY,finalWorldY)<unsigned(finalWorldY)))
            (void)preparePatternCache(renderer,rooms,room,decorations,decorationCount,
                runtime,viewX,firstWorldY,finalWorldY);
#endif
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
        if(refresh&&column==1&&clipX==downwellPlayfieldX&&clipWidth==downwellPlayfieldWidth&&
           !decorationCount&&runtime&&!runtime->terrainInstances()&&
           (!runtime->nativeTerrainInstances()||!runtime->nativeTerrainInstances()->hasWallPresentationState())&&
           !runtime->shops().allocatedCount&&terrainCollision.index&&
           AmigaTerrainPainter::corridorRows(*terrainCollision.index,terrainTiles,firstWorldY,finalWorldY)&&
           terrainRaster.borrowCorridor(firstWorldY,clipHeight))refresh=false;
        if (!refresh) {
            downwellProfileTerrainSpan(viewX, firstWorldY, clipHeight, clipX, clipWidth, 0, refreshCause);
            transferTerrainRows(renderer, viewX, firstWorldY, clipY, clipHeight, false);
#ifdef DOWNWELL_SCENE_VERIFY
#define DOWNWELL_VERIFY_PART 9
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
            return;
        }
#ifdef DOWNWELL_TERRAIN_DIRECT
        if(clipX==downwellPlayfieldX&&clipWidth==downwellPlayfieldWidth&&
            AmigaTerrainPainter::paintCached(terrainRaster.writeRows(column,firstWorldY,clipHeight),renderer,
                rooms,room,terrainCollision.index,terrainTiles,decorations,decorationCount,
                runtime,activeGamePack,activeGamePackEnd,viewX,viewY,clipY,clipHeight)){
            transferTerrainRows(renderer,viewX,firstWorldY,clipY,clipHeight,false);
            for(int y=firstWorldY;y<finalWorldY;++y)terrainRaster.setDirty(column,y,false);
            return;
        }
#endif
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
        const int column = std::uint16_t(viewX - mainViewX + 160) / 160; // cacheable: 0..320
        for (int y = firstWorldY; y < finalWorldY; ++y)
            terrainRaster.setDirty(column,y,false);
    }
}

static void prepareTerrainRaster(AmigaRenderer* renderer,
    const downwell::RoomPack& rooms, const downwell::PackedRoom& room,
    const downwell::BuilderPatternPack& builders,
    const AmigaTerrainFragment* fragments, unsigned fragmentCount,
    const AmigaTerrainDecoration* decorations, unsigned decorationCount,
    const AmigaPortRuntime* runtime, bool freshlyLoadedRoom) {
#ifdef DOWNWELL_TERRAIN_BATCH_VERIFY
    // Keep both oracle diagnostics available in retained-reference builds too.
    (void)gDownwellInitialTerrainEnabled;(void)gDownwellInitialTerrainChecks;
#endif
    const bool retainedRoom=roomTerrainPlan.matches(rooms,room);
    int lastY = retainedRoom?roomTerrainPlan.wallEnd():0;
    for (unsigned i = 0; !retainedRoom && i < room.instanceCount; ++i) {
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
#if defined(DOWNWELL_TERRAIN_DIRECT) && !defined(DOWNWELL_REFERENCE_INITIAL_TERRAIN) && !defined(DOWNWELL_REFERENCE_INITIAL_RASTER)
    bool pristine=freshlyLoadedRoom&&runtime&&!runtime->terrainInstances()&&
        retainedRoom&&room.roomId==11&&!fragmentCount&&!decorationCount&&!terrainTiles.size()&&
        initialTerrain.rows()!=0&&initialTerrain.rows()==unsigned(lastY);
    if(pristine)for(const auto& shop:runtime->shops().instances)if(shop.present)pristine=false;
    if(pristine){
#ifndef DOWNWELL_REFERENCE_INITIAL_RASTER_STORAGE
        if(!terrainRaster.borrowInitial(initialTerrain)){terrainPaintSucceeded=false;return;}
#ifdef DOWNWELL_PREPARED_PATTERN_ROWS
        // Spend the dictionary expansion once before the level is presented,
        // not once per exposed line during the initial fall. Side columns
        // remain borrowed; no extra raster allocation or asset bank is needed.
        if(!terrainRaster.materializeInitialColumn(1)){terrainPaintSucceeded=false;return;}
#endif
#endif
        for(unsigned column=0;column<3;++column){
#ifdef DOWNWELL_REFERENCE_INITIAL_RASTER_STORAGE
            if(!initialTerrain.copy(column,terrainRaster.writeRows(column,0,lastY))){terrainPaintSucceeded=false;return;}
#endif
#ifdef DOWNWELL_TERRAIN_BATCH_VERIFY
#define DOWNWELL_VERIFY_PART 10
#include "DownwellAmigaTerrainBatchVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
        }
        terrainRaster.ready=true;return;
    }
#endif
    for (int x = mainViewX - 160; x <= mainViewX + 160; x += 160)
        for (int y = 0, height = 0; y < lastY; y += height) {
            height = lastY - y;
            if (height > downwellDisplayHeight) height = downwellDisplayHeight;
            if (unsigned(height) > AmigaTerrainRaster::contiguousRows(unsigned(y)))
                height = int(AmigaTerrainRaster::contiguousRows(unsigned(y)));
            amigaRendererSetGameOrigin(renderer, 0);
#if defined(DOWNWELL_TERRAIN_DIRECT) && !defined(DOWNWELL_REFERENCE_INITIAL_RASTER)
            // The raster is the destination, not a copy of scratch Chip RAM.
            // This shares runtime's exact compositor and its unsupported-art
            // fallback; it neither publishes pixels nor advances game state.
            const unsigned column=unsigned(std::uint16_t(x-mainViewX+160)/160);
            if(AmigaTerrainPainter::paintCached(terrainRaster.writeRows(column,y,height),renderer,
                rooms,room,terrainCollision.index,terrainTiles,decorations,decorationCount,
                runtime,activeGamePack,activeGamePackEnd,x,y,0,height))continue;
#endif
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

    DW_PROFILE_SCOPE(profilePictureClear);
    amigaRendererClear(renderer, 0);
    ammoHud.invalidate();
    healthHud.invalidate();
    invalidateHudText(true);
    for (auto& ray : gameplayRays) ray.discard();
    for (auto& field : gameplayFields) field.discard();
    dirtyGameRows(0, 0, downwellGameRingHeight);
    DW_PROFILE_PHASE(profilePictureTerrain);
    drawMainWorld(renderer, rooms, room, builders,
                  fragments, fragmentCount,
                  decorations, decorationCount, runtime,
                  viewX, viewY);

    DW_PROFILE_PHASE(profilePictureMargins);
    amigaRendererClearClipRect(renderer);
    // PF2 is above all sprites; its nonzero black pen masks the HUD margins.
    amigaRendererFillRect(renderer, 0, 0, downwellPlayfieldX,
                          downwellDisplayHeight, 3);
    amigaRendererFillRect(renderer, downwellPlayfieldX + downwellPlayfieldWidth,
                          0, downwellVisibleWidth - downwellPlayfieldX - downwellPlayfieldWidth,
                          downwellDisplayHeight, 3);
    DW_PROFILE_PHASE(profilePictureBorderLeft);
    drawAsset(renderer, 408, 0, 108, 0, 0);
    DW_PROFILE_PHASE(profilePictureBorderRight);
    drawAsset(renderer, 408, 0, 272, 0, 1);

    DW_PROFILE_PHASE(profilePictureHealth);
    if (runtime) redrawHealth(renderer, unsigned(runtime->health()), unsigned(runtime->healthMaximum()),
                              runtime->healthReserve().pieces, runtime->healthReserve().piecesPerHeart);
    else redrawHealth(renderer, 4, 4);
    shownUpgradeCount=0;

    DW_PROFILE_PHASE(profilePictureAmmoArt);
    drawAsset(renderer, 416, 0, 284, 43, 0);
    amigaRendererFillRect(renderer, 289, 49, 10, 184, 1);
    drawAsset(renderer, 416, 1, 284, 43, 0);
    DW_PROFILE_PHASE(profilePictureText);
#if !defined(DOWNWELL_REFERENCE_HUD_WORDS) && !defined(DOWNWELL_REFERENCE_HUD_TEXT)
    // Establish the publisher's ownership with the initial picture. Leaving
    // these on the generic path makes the first pickup redraw static art.
    redrawCurrency(renderer, 0);
    shownCurrencyText=0;
    gemStreakHud.invalidate();streakView={};
    invalidateTitle();
    timerHud.invalidate();
    redrawStreak(renderer, 0);
#else
    drawAsset(renderer, 376, 0, 362 - currencyShift, 18, 0);
    drawCurrencyDigit(renderer, 354 - currencyShift, 18, 0);
#endif
    drawAmmoDigit(renderer, 290, 239, 8);
    DW_PROFILE_PHASE(profilePictureAmmoState);
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
        std::uint16_t(ringOrigin + gameplayPrefetchLines) % downwellGameRingHeight; // callers pass the ring origin (<320)
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

// WR.1: the most memory the game allocates beyond its loaded hunks, as
// bytes of Chip (DMA data only) and of other public memory (Fast when
// present, else Chip). Measured on the world-one route on the A500 with
// Fast (2026-11-21): free memory after boot (out/FreeMemoryProbe) less the
// route's low-water mark less the loaded hunks, rounded up to whole KB
// (676,672 bytes of public memory with the ring terrain raster). DOWNWELL_MEMORY_SAMPLE=1 on the native routes fails when
// the use it sees exceeds these. The hunks themselves must already have
// loaded for this check to run.
extern "C" {
// volatile keeps the objects (and the startup check's reads) through LTO
// for the native routes, which read them by name.
extern const volatile ULONG downwellChipRequirement;
extern const volatile ULONG downwellPublicRequirement;
__attribute__((used)) const volatile ULONG downwellChipRequirement = 940u * 1024u;
__attribute__((used)) const volatile ULONG downwellPublicRequirement = 661u * 1024u;
}

static char* appendNumber(char* out, ULONG value)
{
    char digits[12];
    unsigned n = 0;
    do { digits[n++] = char('0' + value % 10); value /= 10; } while (value);
    while (n) *out++ = digits[--n];
    return out;
}

static char* appendKilobytes(char* out, ULONG bytes)
{
    out = appendNumber(out, (bytes + 1023) / 1024);
    *out++ = ' '; *out++ = 'K'; *out++ = 'B';
    return out;
}

static char* appendText(char* out, const char* text)
{
    while (*text) *out++ = *text++;
    return out;
}

extern "C" void downwellRefuse(const char* message); // AmigaStart.c
extern "C" const char* volatile gDownwellRefusal;

// Refuse before allocating anything, while the shell still shows output:
// Chip must hold the DMA data, and Chip plus Fast must hold everything.
static bool downwellMemoryFits()
{
    const ULONG chip = AvailMem(MEMF_CHIP), any = AvailMem(MEMF_PUBLIC);
    const ULONG anyNeeded = downwellChipRequirement + downwellPublicRequirement;
    gDownwellStartupChipFree = chip;
    if (chip >= downwellChipRequirement && any >= anyNeeded) return true;
    char line[160];
    char* out = appendText(line, "Downwell needs ");
    if (chip < downwellChipRequirement) {
        out = appendKilobytes(out, downwellChipRequirement);
        out = appendText(out, " of Chip memory (");
        out = appendKilobytes(out, chip);
        out = appendText(out, " free)");
        if (any < anyNeeded) out = appendText(out, " and ");
    }
    if (any < anyNeeded) {
        out = appendKilobytes(out, anyNeeded);
        out = appendText(out, " of memory in all (");
        out = appendKilobytes(out, any);
        out = appendText(out, " free)");
    }
    out = appendText(out, ".");
    *out = 0;
    downwellRefuse(line);
    return false;
}

// An allocation failed after the free-memory check passed (the memory is
// there, but not in blocks large enough).
static void downwellRefuseMemory()
{
    char line[128];
    char* out = appendText(line, "Not enough free memory: Downwell needs ");
    out = appendKilobytes(out, downwellChipRequirement);
    out = appendText(out, " of Chip memory and ");
    out = appendKilobytes(out, downwellChipRequirement + downwellPublicRequirement);
    out = appendText(out, " in all.");
    *out = 0;
    downwellRefuse(line);
}

// WI.6: every data file this build reads, with the size it was built for,
// checked before anything loads so that a missing or wrong file is named
// while the Shell or Workbench can still show it. The loaders read exactly
// these sizes; a later read that fails still ends the game with an error.
struct DownwellDataFile { const char* name; ULONG bytes; };
static const DownwellDataFile downwellDataFiles[] = {
    {AmigaDataFile::tables, DOWNWELL_TABLES_BYTES},
    {AmigaDataFile::boot, DOWNWELL_BOOT_BANK_BYTES},
    {AmigaDataFile::front, DOWNWELL_FRONT_BANK_BYTES},
#if defined(DOWNWELL_REPLAY_VERIFY) || defined(DOWNWELL_PLANE_BANK_VERIFY)
    {AmigaDataFile::game, DOWNWELL_GAME_BANK_BYTES},
    {AmigaDataFile::planes, DOWNWELL_GAME_PLANE_BYTES},
#else
    {AmigaDataFile::cavern, DOWNWELL_CAVERN_BANK_BYTES},
    {AmigaDataFile::cavernPlanes, DOWNWELL_CAVERN_PLANE_BYTES},
#endif
    {AmigaDataFile::results, DOWNWELL_RESULTS_BANK_BYTES},
    {AmigaDataFile::shop, DOWNWELL_SHOP_BANK_BYTES},
    {AmigaDataFile::pause, DOWNWELL_PAUSE_BANK_BYTES},
    {AmigaDataFile::clear, DOWNWELL_CLEAR_BANK_BYTES},
    {AmigaDataFile::selectionMask, DOWNWELL_SELECTIONMASK_BANK_BYTES},
    {AmigaDataFile::exitMask, DOWNWELL_EXITMASK_BANK_BYTES},
    {AmigaDataFile::entryMask, DOWNWELL_ENTRYMASK_BANK_BYTES},
    {AmigaDataFile::patterns, DOWNWELL_PATTERNS_BANK_BYTES},
#ifndef DOWNWELL_REFERENCE_ENTRY_ASSETS
    {AmigaDataFile::hud, DOWNWELL_HUD_BANK_BYTES},
#endif
#ifdef DOWNWELL_PREPARED_FIELDS
    {AmigaDataFile::fields, downwell::fieldBandBytes},
    {AmigaDataFile::columns, downwell::fieldColumnBytes},
#endif
#ifdef DOWNWELL_PREPARED_PATTERN_ROWS
    {AmigaDataFile::terrain, DOWNWELL_TERRAIN_BANK_BYTES},
#endif
#ifdef DOWNWELL_SCENE_COLLISION_BACKEND
    {AmigaDataFile::encounterMasks, DOWNWELL_ENCOUNTER_MASK_BYTES},
#endif
    {AmigaDataFile::soundRun, DOWNWELL_SFX_RUN_BYTES},
    {AmigaDataFile::soundFront, DOWNWELL_SFX_FRONT_BYTES},
    {AmigaDataFile::soundResults, DOWNWELL_SFX_RESULTS_BYTES},
    {AmigaDataFile::soundClear, DOWNWELL_SFX_CLEAR_BYTES},
};

// "Cannot read data/Downwell.x: reinstall Downwell." for a missing file, or
// "data/Downwell.x is not the file Downwell needs: ..." for a wrong one (its
// size or its header). The name is where the program looked (data/ unless it
// was found beside the program).
static void refuseDataFile(const AmigaPlatform& platform, const char* name, bool missing)
{
    bool installed;
    (void)platform.dataSize(name, installed);
    char line[128];
    char* out = missing ? appendText(line, "Cannot read ") : line;
    if (installed) out = appendText(out, "data/");
    out = appendText(out, name);
    out = appendText(out, missing ? ": reinstall Downwell." : " is not the file Downwell needs: reinstall Downwell.");
    *out = 0;
    downwellRefuse(line);
}

static bool downwellDataFilesPresent(const AmigaPlatform& platform)
{
    for (const DownwellDataFile& file : downwellDataFiles) {
        bool installed;
        const LONG size = platform.dataSize(file.name, installed);
        if (size == LONG(file.bytes)) continue;
        refuseDataFile(platform, file.name, size < 0);
        return false;
    }
    return true;
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
#define DOWNWELL_VERIFY_PART 11
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif

#ifdef DOWNWELL_FRAME_DUMP
// Debugger hook: amiga/dump_frames.sh breaks here at chosen ticks and saves
// the published gameplay buffer in the decode-amiga-gameplay-frame format.
extern "C" {
volatile const UBYTE* gDownwellDumpGame;
volatile const UBYTE* gDownwellDumpHud;
volatile const UWORD* gDownwellDumpPlayerSprite;
volatile const UWORD* gDownwellDumpActorSprites;
volatile const UWORD* gDownwellDumpOverflowSprite;
volatile UWORD gDownwellDumpRingOrigin;
volatile ULONG gDownwellDumpTick;
volatile const GameplayBob* gDownwellDumpBobs;
volatile ULONG gDownwellDumpBobCount;
volatile const UBYTE* gDownwellDumpPack;
volatile ULONG gDownwellDumpPackBytes;
__attribute__((noinline, used)) void downwellFrameDump(void) {
    (void)gDownwellHudWordFallbacks; // Retain the witness in reference builds too.
    __asm__ volatile("" ::: "memory");
}
}
#endif

#ifdef DOWNWELL_ENCOUNTER_UPDATE_FAILURE_VERIFY
extern "C" {
volatile unsigned gDownwellEncounterUpdateDraws=0;
volatile unsigned gDownwellEncounterUpdatePublications=0;
}
#endif
// Keep the large application's frame out of the entry wrapper so a caller
// with an undersized task stack can fail before any deep initialization.
__attribute__((noinline)) static int runDownwell(void) {
#if defined(DOWNWELL_SCENE_VERIFY) || defined(DOWNWELL_REPLAY_VERIFY)
#define DOWNWELL_VERIFY_PART 12
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
#ifdef DOWNWELL_REPLAY_VERIFY
    gMemoryCopyVerifiedCases=verifyAmigaMemoryCopy(std::memcpy);
    if(gMemoryCopyVerifiedCases!=384)return 20;
    gCoverageWordVerifiedCases=verifyAmigaCoverageWords();
    if(gCoverageWordVerifiedCases!=80)return 20;
#endif
    AmigaPlatform platform;
#ifdef DOWNWELL_RESULTS_RESTART_VERIFY
    UBYTE rejectDrawMarker=0;
    gDownwellEncounterRejectDraw=platform.loadFile("Downwell.reject-draw",&rejectDrawMarker,1)>=0;
#endif
    sceneAssetPlatform = &platform;
    UpgradeSource upgradeSource;
#ifdef DOWNWELL_SCENE_VERIFY
    AmigaVerificationScratch::Storage verificationStorage;
    if (!verificationStorage.valid()) return 20;
#endif
    AmigaMemory memory;
    AmigaRenderer renderer;
    unsigned front = 0;
    int worldViewX = mainViewX;
    int worldViewY = mainInitialViewY;
    AmigaShopArtSnapshot oldShopArt;
    // WI.6: refuse with a reason before the first load or allocation.
    if (!platform.available()) {
        gDownwellStartupFailure = 1;
        downwellStartupFailed();
        downwellRefuse("Downwell needs Kickstart 2.0 or later.");
        return 20; // AmigaDOS RETURN_FAIL
    }
    if (!downwellDataFilesPresent(platform)) return 20;
    // WI.3: the derived tables come from the installed Downwell.tables.
    AmigaTables tables;
    const AmigaTables::Load tablesLoad = tables.load(platform);
    if (tablesLoad == AmigaTables::noMemory) { downwellRefuseMemory(); return 20; }
    if (tablesLoad == AmigaTables::wrongFile) { refuseDataFile(platform, AmigaDataFile::tables, false); return 20; }
    if(!downwell_installed_tables::bind(
            [&](unsigned long tag,unsigned long& bytes){
                ULONG size=0;UBYTE* section=tables.writable(tag,size);
                bytes=size;return section;}))return 20;
    const AmigaTables::View objectTable=tables.find(DOWNWELL_TABLE_OBJT);
    const AmigaTables::View collisionTable=tables.find(DOWNWELL_TABLE_SCOL);
    const AmigaTables::View roomTable=tables.find(DOWNWELL_TABLE_ROOM);
    const AmigaTables::View effectStampTable=tables.find(DOWNWELL_TABLE_STMP);
#ifdef DOWNWELL_TERRAIN_DIRECT
    const AmigaTables::View phase6Table=tables.find(DOWNWELL_TABLE_TPH6);
    const AmigaTables::View initialTerrainTable=tables.find(DOWNWELL_TABLE_TINI);
#endif
    if(!effectStampTable.data)return 20;
    const downwell::ObjectPack objects(objectTable.data,objectTable.bytes);
    const downwell::SpriteCollisionPack collisionSprites(collisionTable.data,collisionTable.bytes);
#if defined(DOWNWELL_SCENE_VERIFY) && !defined(DOWNWELL_RESULTS_RESTART_VERIFY) && !defined(DOWNWELL_SCENE_COLLISION_BACKEND)
    const downwell::SpriteMaskPack collisionMasks(amiga_collision_masks,
        (unsigned long)(amiga_collision_masks_end-amiga_collision_masks));
    if(!collisionMasks.valid())return 20;
#endif
    const downwell::RoomPack rooms(roomTable.data,roomTable.bytes);
    if(!objects.valid()||!collisionSprites.valid()||!rooms.valid())return 20;
    checkSceneStack(1);
#ifdef DOWNWELL_GAMEPLAY_COLLISION_OWNER
    // Ownership precedes player construction and survives in-place run resets.
    // Keep these bounded tables off the AmigaDOS stack, like the world itself.
    alignas(downwell::GameplayCollisionOwner) static UBYTE collisionStorage[sizeof(downwell::GameplayCollisionOwner)];
    // Gameplay answers wall collision from the occupancy grid, never
    // dispatches wall End Step and issues no spatial query through the owner,
    // so it keeps only admitted event order. Scene, field and entry fixtures
    // exercise the complete reference; results/re-entry deliberately exercises
    // shipping native ownership across all three results destinations.
#if !defined(DOWNWELL_SCENE_COLLISION_BACKEND) && ((defined(DOWNWELL_SCENE_VERIFY) && !defined(DOWNWELL_RESULTS_RESTART_VERIFY)) || defined(DOWNWELL_FIELD_VERIFY) || defined(DOWNWELL_ENTRY_WIPE_VERIFY))
    constexpr auto ownership=downwell::GameplayCollisionOwner::Ownership::reference;
#elif defined(DOWNWELL_SCENERY_LAYERS)
    constexpr auto ownership=downwell::GameplayCollisionOwner::Ownership::nativeScenery;
#else
    constexpr auto ownership=downwell::GameplayCollisionOwner::Ownership::native;
#endif
    downwell::GameplayCollisionOwner::ReferenceStorage* referenceStorage=nullptr;
#if !defined(DOWNWELL_SCENE_COLLISION_BACKEND) && ((defined(DOWNWELL_SCENE_VERIFY) && !defined(DOWNWELL_RESULTS_RESTART_VERIFY)) || defined(DOWNWELL_FIELD_VERIFY) || defined(DOWNWELL_ENTRY_WIPE_VERIFY))
    using ReferenceStorage=downwell::GameplayCollisionOwner::ReferenceStorage;
    alignas(ReferenceStorage) static UBYTE referenceMemory[sizeof(ReferenceStorage)];
    referenceStorage=new(referenceMemory) ReferenceStorage;
#endif
    AmigaSpatialTreeStorage<downwell::GameplayCollisionOwner::spatialNodeCapacity> spatialTreeStorage;
    AmigaSpatialJournalStorage<downwell::GameplayCollisionOwner::Owner::SpatialIndex::JournalStorage> spatialJournalStorage;
    auto& collisionOwner=*new(collisionStorage) downwell::GameplayCollisionOwner(objects,collisionSprites,
        downwell::GameplayTerrainInstances::Presentation::bind(wallDamage),
        downwell::GameplayTerrainInstances::WallCollision::bind(wallCollision),&rooms,ownership,
        DOWNWELL_NATIVE_BOXES!=0,DOWNWELL_NATIVE_FIELDS!=0,referenceStorage);
    if(!collisionOwner.valid()||!collisionOwner.bindSpatialTree(spatialTreeStorage.provider())||
       !collisionOwner.bindSpatialJournal(spatialJournalStorage.provider()))return 20;
#ifndef DOWNWELL_SCENE_COLLISION_BACKEND
    if(ownership!=downwell::GameplayCollisionOwner::Ownership::reference&&
       !collisionOwner.declareExternalCollisionBackend())return 20;
#endif
    checkSceneStack(2);
#ifdef DOWNWELL_REPLAY_VERIFY
    AmigaPortRuntime& port = *new (portRuntimeStorage)
        AmigaPortRuntime(initialRingOrigin,true,0,false,collisionOwner.enemyBinding(),
            collisionOwner.breakableBinding(),collisionOwner.playerBinding(),&collisionOwner.terrain(),&rooms);
    port.configureOpenSpaceContacts();
#else
    // Native collision/painting still uses occupancy (unchecked walls are
    // sprite 51 with a grid-derived frame). Generation separately retains
    // identity runs; they are not duplicate generic per-wall runner records.
    AmigaPortRuntime& port = *new (portRuntimeStorage)
        AmigaPortRuntime(initialRingOrigin, false, 1201 - 176, true,collisionOwner.enemyBinding(),
            collisionOwner.breakableBinding(),collisionOwner.playerBinding(),
            (ownership==downwell::GameplayCollisionOwner::Ownership::native||
             ownership==downwell::GameplayCollisionOwner::Ownership::nativeScenery)?nullptr:&collisionOwner.terrain(),&rooms,
#if defined(DOWNWELL_ENTRY_WIPE_VERIFY) || defined(DOWNWELL_FIELD_VERIFY)
            GameplayWorld::PlayerBirthTiming::construction);
#else
            GameplayWorld::PlayerBirthTiming::authoredRoom);
#endif
#endif
    if(!port.effectsSucceeded()||!collisionOwner.attach(port))return 20;
#ifdef DOWNWELL_SCENE_COLLISION_BACKEND
    AmigaEncounterResources encounterResources(port,collisionOwner);
#endif
    downwell::GameplayEncounterSession encounterSession(port,collisionOwner);
    if(!encounterSession.start())return 20;
#else
    // Native collision/painting uses occupancy; generation retains terrain
    // identity runs. The world owns every family world 1 draws.
    AmigaPortRuntime& port = *new (portRuntimeStorage)
        AmigaPortRuntime(initialRingOrigin, false, 1201 - 176, true,{},{},{},nullptr,&rooms,
            GameplayWorld::PlayerBirthTiming::authoredRoom);
    if(!port.effectsSucceeded()||!port.bindOwnedDitherAbove())return 20;
    downwell::NoCollisionOwner noCollisionOwner;
    downwell::UnownedEncounterSession encounterSession(port,noCollisionOwner);
    if(!encounterSession.start())return 20;
#endif
#ifdef DOWNWELL_ENTRY_WIPE_VERIFY
#define DOWNWELL_VERIFY_PART 13
#include "DownwellAmigaEntryWipeVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
    checkSceneStack(3);
#ifndef DOWNWELL_REPLAY_VERIFY
#ifdef DOWNWELL_SCENE_VERIFY
#define DOWNWELL_VERIFY_PART 14
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
#endif
#ifdef DOWNWELL_FIELD_VERIFY
#define DOWNWELL_VERIFY_PART 15
#include "DownwellAmigaFieldVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
#ifndef DOWNWELL_REPLAY_VERIFY
    // Long-lived scene state must not consume the small AmigaDOS call stack.
    // Construct at entry (not before the framework startup), as for port.
    alignas(AmigaSceneRuntime) static UBYTE sceneStorage[sizeof(AmigaSceneRuntime)];
    AmigaSceneRuntime& scenes = *new (sceneStorage) AmigaSceneRuntime;
    loadSave(platform);
    {
        const auto unlock = downwell::UnlockProgress::from(saveState.totalGems);
        if (saveState.shader > unlock.paletteUnlock) saveState.shader = 0;
        scenes.configureStyles(unlock.styleUnlock, saveState.hardUnlocked, saveState.style, saveState.hardMode);
        port.restoreRunRecords({saveState.totalGems, saveState.bestRunGems, saveState.bestCombo});
        pauseTextReady = pauseText.configure(downwell::menu_text::entries, downwell::menu_text::count, "english");
    }
    alignas(AmigaUpgradeScene) static UBYTE clearStorage[sizeof(AmigaUpgradeScene)];
    auto& clear = *new (clearStorage) AmigaUpgradeScene;
#ifdef DOWNWELL_CLEAR_VERIFY
    AmigaClearOracleBuffers clearOracle;
#endif
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
#define DOWNWELL_VERIFY_PART 16
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
    worldViewY += port.cameraOffsetY();
#endif

    if (!downwellMemoryFits()) {
        gDownwellStartupFailure = 5;
        downwellStartupFailed();
        return 20;
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
    ULONG gamePackBytes = 0; // No gameplay lease has been admitted yet.
    gDownwellGamePlaneExpectedBytes=gamePackBytes;
    const ULONG effectStampBytes=effectStampTable.bytes;
    // The enemy shade's alpha plane (sprite 117, 96x96) follows the effect
    // surface in Chip: its subtraction is a blit only from Chip.
    // Then the Chip mirror arena for other catalogue effect assets.
    constexpr unsigned shadeChipCapacity=12*96,effectMirrorBytes=6144;
    const ULONG shadeChipOffset=(AmigaEffectSurface::bytes+3)&~3UL;
    const ULONG sceneBankOffset=shadeChipOffset+shadeChipCapacity+effectMirrorBytes;
    // The retired selection cache holds two game rings followed by DMA-only
    // effect words. Other scenes may overwrite them; reload on gameplay entry.
    constexpr unsigned effectStampCacheOffset=2*downwellGameRingBytes;
    static_assert(sceneCacheBytes>=effectStampCacheOffset,"Gameplay rings exceed scene cache");
    if(effectStampBytes<AmigaEffectStampBank::payloadOffset||
       effectStampBytes-AmigaEffectStampBank::payloadOffset>sceneCacheBytes-effectStampCacheOffset){
        platform.restore();FreeMem(startupCopperWords,4*sizeof(UWORD));return 20;
    }
    const unsigned effectPayloadBytes=effectStampBytes-AmigaEffectStampBank::payloadOffset;
    gDownwellStartupChipFree = AvailMem(MEMF_CHIP);
    gDownwellStartupChipLargest = AvailMem(MEMF_CHIP | MEMF_LARGEST);
    gDownwellStartupAssetBytes = 0;
    if (!memory.allocate(downwellGameRingBytes, downwellLayerBytes,
                         menuPlaneBytes, amigaMenuEntryPlaneBytes,
                         downwellCopperWords * copperListCount * sizeof(UWORD),
                         playerSpriteBytes,
                         bulletSpriteBytes * bulletSpriteBufferCount,
                         enemySpriteBytes,
                         sceneCacheBytes,
                         sceneBankOffset+sceneBankBytes)) {
        platform.restore();
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
        gDownwellStartupFailure = 4;
        downwellStartupFailed();
        return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
    }
    gDownwellStartupChipRemaining = AvailMem(MEMF_CHIP);
    // W2.5: Paula and the scene sound banks. Failure leaves the game silent.
    // Static storage (no runtime for guarded statics); released explicitly.
    alignas(downwell::AmigaSoundBanks) static UBYTE soundBankStorage[sizeof(downwell::AmigaSoundBanks)];
    auto& soundBanks = *new (soundBankStorage) downwell::AmigaSoundBanks();
    (void)soundBanks.acquire(platform);
    sceneBank = memory.assets() + sceneBankOffset;
    UBYTE* effectStamps=memory.sceneCache()+effectStampCacheOffset;
    memcpy(effectStamps,effectStampTable.data+AmigaEffectStampBank::payloadOffset,effectPayloadBytes);
    UBYTE* const shadeChip=memory.assets()+shadeChipOffset;
    downwell::BuilderPatternPack builders(nullptr,0);
    static_assert(DOWNWELL_PATTERNS_BANK_BYTES<=DOWNWELL_PATTERNS_BANK_BUDGET,
        "Builder pattern residency budget");
    const auto acquireBuilderPatterns=[&](){
        if(builders.valid())return memory.builderPatterns()!=nullptr;
        if(!memory.acquireBuilderPatterns(DOWNWELL_PATTERNS_BANK_BYTES))return false;
        if(platform.loadData(AmigaDataFile::patterns,memory.builderPatterns(),
                DOWNWELL_PATTERNS_BANK_BYTES)!=LONG(DOWNWELL_PATTERNS_BANK_BYTES)){
            memory.releaseBuilderPatterns();return false;
        }
        builders=downwell::BuilderPatternPack(memory.builderPatterns(),DOWNWELL_PATTERNS_BANK_BYTES);
        if(!builders.valid()){
            builders=downwell::BuilderPatternPack(nullptr,0);
            memory.releaseBuilderPatterns();return false;
        }
        gDownwellBuilderPatternBytes=DOWNWELL_PATTERNS_BANK_BYTES;
        ++gDownwellBuilderPatternLoads;return true;
    };
#ifdef DOWNWELL_REPLAY_VERIFY
    if(!acquireBuilderPatterns()){
        platform.restore();FreeMem(startupCopperWords,4*sizeof(UWORD));memory.release();return 20;
    }
    const downwell::ReplayReader verificationReplay(
        downwell::portReplayBytes, downwell::portReplaySize);
#endif
    static AmigaTerrainFragment fragments[amigaAreaOneTerrainFragmentCapacity];
    static AmigaTerrainDecoration
        generatedDecorations[amigaTerrainDecorationCapacity];
    alignas(AmigaAreaOneTerrainBuilder) static UBYTE liveBuilderStorage[sizeof(AmigaAreaOneTerrainBuilder)];
    auto& liveBuilder = *new (liveBuilderStorage) AmigaAreaOneTerrainBuilder(fragments,
        amigaAreaOneTerrainFragmentCapacity, nullptr, 0,
        generatedDecorations, amigaTerrainDecorationCapacity);
    // Retain native identity/geometry in runs without duplicating generic
    // runner state per wall. Storage must not consume the 8 KiB task stack.
    alignas(AmigaGameplayGeneration::TerrainLifetimes) static UBYTE nativeTerrainStorage[
        sizeof(AmigaGameplayGeneration::TerrainLifetimes)];
    auto* nativeTerrain=port.terrainCreatesBackendOnly()?
        new(nativeTerrainStorage) AmigaGameplayGeneration::TerrainLifetimes:nullptr;
    AmigaGameplayGeneration liveGeneration{liveBuilder, builders, objects, terrainIndex, terrainTiles,
        nativeTerrain};
    downwell::GameMakerRandom enemyRandom(2);
    downwell::PackedRoom mainRoom;
    const AmigaTerrainCollisionContext emptyTerrain{0, 0, &builders, 0, 0, 0};
#ifdef DOWNWELL_REPLAY_VERIFY
    amigaBuildTerrainIndex(emptyTerrain, terrainIndex);
    terrainTiles.clear();
#endif
#ifndef DOWNWELL_REPLAY_VERIFY
    const int emptyBlock = 0;
    port.configureBreakables(&emptyBlock, &emptyBlock, 0);
#endif
    // levelNumber's langString("area" + n), English as the shipped default.
    port.configureCompactAreaTitles(downwell::area_titles::entries,downwell::area_titles::count,"english");
    // Shop Step's scrRisingTextShop strings for the merchant's speech.
    if (!port.configureCompactShopSpeech(downwell::shop_speech::entries,downwell::shop_speech::count,"english") ||
        !rooms.valid() || !objects.valid() ||
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
    bool initialGameplayPrepared=false;
    const auto prepareInitialGameplay=[&](){
        DW_PROFILE_SCOPE(profileEntryRoomAdmission);
        {
            DW_PROFILE_SCOPE(profileEntryPatternLease);
            if(!acquireBuilderPatterns())return false;
        }
        if(initialGameplayPrepared)return true;
        {
        DW_PROFILE_SCOPE(profileEntryRoomMetadata);
        amigaBuildTerrainIndex(emptyTerrain, terrainIndex);
        terrainTiles.clear();
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
        // Keep the zero-cost reference measurement visible to the debugger.
        (void)gDownwellRoomPlanFields;
#endif
#ifdef DOWNWELL_PROBES
        (void)gDownwellRoomPlanTicks;
#endif
#ifndef DOWNWELL_REFERENCE_ROOM_TERRAIN
        roomTerrainPlan.prepare(rooms,mainRoom);
#ifndef DOWNWELL_REFERENCE_ROOM_INDEX
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
        const auto roomPlanStart=AmigaPlatform::vblankCount();
#endif
#ifdef DOWNWELL_PROBES
        const auto roomPlanTick=downwellAmigaProfileTimestamp();
#endif
        roomTerrainBackend.prepare(roomTerrainPlan,terrainIndex);
#ifdef DOWNWELL_PROBES
        gDownwellRoomPlanTicks=downwellAmigaProfileTimestamp()-roomPlanTick;
#endif
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
        gDownwellRoomPlanFields=UWORD(AmigaPlatform::vblankCount()-roomPlanStart);
#endif
#endif
#endif
        terrainCollision.builders = &builders;
        terrainCollision.rooms = &rooms;
        terrainCollision.room = &mainRoom;
        terrainCollision.fragments = fragments;
        terrainCollision.fragmentCount = 0;
        terrainCollision.runtime = &port;
        }
        {
        DW_PROFILE_SCOPE(profileEntryRoomCreates);
        if (!liveGeneration.loadRoomWithStartField(port,rooms,mainRoom,&roomTerrainPlan,&roomTerrainBackend,
            [](AmigaPortRuntime& world,AmigaAreaOneTerrainBuilder& builder,
               const downwell::PackedRoomInstance& instance,void* context){
                if(instance.x!=-16||instance.y!=704)return false;
                auto& random=*static_cast<downwell::GameMakerRandom*>(context);
                builder.resetInitialCheckpointBeforeEntrance(random);
#ifdef DOWNWELL_PROFILE_OVERFLOW
                if(!amigaOverflowProfileEntry(builder,random))return false;
#endif
                world.configureEnemyRandom(random);return true;
            },&enemyRandom)) {
            gDownwellStartupFailure = 6;
            downwellStartupFailed();
            return false;
        }
        }
        worldViewX = mainViewX + port.cameraSteadyOffsetX();
        worldViewY = mainInitialViewY + port.cameraOffsetY();
        terrainCollision.index = &terrainIndex;
#if defined(DOWNWELL_SCENE_VERIFY) && !defined(DOWNWELL_RESULTS_RESTART_VERIFY) && !defined(DOWNWELL_SCENE_COLLISION_BACKEND)
        // Exercise the target's actual precise-mask route, not an occupancy
        // answer. This does not replace the legacy live collision adapters.
        // The verification callback probes resources only, not game events.
        // This is a resource-query probe, not an event dispatch. Borrow the
        // registration stream rather than allocating a full room snapshot.
        // Terrain may be run-owned; never consult the unused scalar store.
        bool testedWall=false;
        const bool maskChecksSucceeded=collisionOwner.visitRegistrationOrder(
            [&](downwell::InstanceHandle shared,int object,std::uint32_t flags){
            if(testedWall||object!=60||(flags&0x100003u))return true;
            GameplayWorld::NativeTerrain::Identity wall;bool active,destroyed;
            if(!downwell::GameplayCollisionView(port).readTerrain(shared,wall,active,destroyed)||
                wall.object!=60||!active||destroyed)return false;
            bool hit=false;
            if(!collisionOwner.terrainPositionMeeting(60,downwell::MotionBinary64::fromInteger(wall.x),
                downwell::MotionBinary64::fromInteger(wall.y),collisionMasks,hit)||!hit)return false;
            ++gDownwellPreciseMaskChecks;testedWall=true;
            return true;
        });
        bool outside=true;
        if(!maskChecksSucceeded||!testedWall||!collisionOwner.terrainPositionMeeting(60,downwell::MotionBinary64::fromInteger(-10000),
            downwell::MotionBinary64::fromInteger(-10000),collisionMasks,outside)||outside){
            gDownwellStartupFailure=6;
            downwellStartupFailed();
            return false;
        }
        ++gDownwellPreciseMaskChecks;
#endif
        checkSceneStack(5);
        port.configureLandingResolver(amigaResolveTerrainLanding,
                                      &terrainCollision);
        port.configureSolidResolver(amigaResolveTerrainSolid,
                                    &terrainCollision, amigaResolveTerrainSegment, amigaResolveTerrainPlayerMask,amigaResolveTerrainWallContacts,amigaResolveTerrainPlatformContact,amigaResolveTerrainCorners);
        port.configureSolidCells(amigaTerrainSolidCells(terrainCollision));
        enemyRandom=port.randomState();
#if defined(DOWNWELL_NATURAL_VERIFY) || defined(DOWNWELL_PROFILE_SHOP)
        AmigaNaturalRouteVerification::entry(liveBuilder, enemyRandom);
#endif
        liveGeneration.bind(port);
        port.configureEnemyRandom(enemyRandom);
        port.configureGenerationStep(generateLiveTerrain, &liveGeneration);
        initialGameplayPrepared=true;++gDownwellInitialRoomPreparations;
        return true;
    };
#if (defined(DOWNWELL_SCENE_VERIFY) && !defined(DOWNWELL_SCENE_DESCENT_VERIFY)) || defined(DOWNWELL_FIELD_VERIFY) || defined(DOWNWELL_ENTRY_WIPE_VERIFY)
    // These explicit fixtures inspect the authored room before their scene
    // loop. The full-scene route exercises ordinary deferred room admission.
#ifdef DOWNWELL_SCENE_COLLISION_BACKEND
    if(!encounterResources.acquire(platform)){
        platform.restore();FreeMem(startupCopperWords,4*sizeof(UWORD));memory.release();return 20;
    }
#endif
    if(!prepareInitialGameplay()){
        platform.restore();FreeMem(startupCopperWords,4*sizeof(UWORD));memory.release();return 20;
    }
#endif
#endif
#ifdef DOWNWELL_TERRAIN_DIRECT
    if(!AmigaTerrainPainter::bindPrepared(phase6Table.data,phase6Table.bytes)){
        platform.restore();FreeMem(startupCopperWords,4*sizeof(UWORD));memory.release();return 20;
    }
#endif
#if defined(DOWNWELL_TERRAIN_DIRECT) && !defined(DOWNWELL_REFERENCE_INITIAL_TERRAIN) && !defined(DOWNWELL_REFERENCE_INITIAL_RASTER)
    // Unsupported banks retain the ordinary painter. This admits immutable
    // pixels only: publication into the live raster still waits for room entry.
    const bool initialTerrainReady=initialTerrain.bind(initialTerrainTable.data,
        initialTerrainTable.bytes);
#ifdef DOWNWELL_TERRAIN_BATCH_VERIFY
    gDownwellInitialTerrainEnabled=initialTerrainReady;
#else
    (void)initialTerrainReady;
#endif
#endif
#ifndef DOWNWELL_REFERENCE_ACTOR_PREFIX
    static AmigaActorFrames actorFrames;
#endif
    static downwell::EffectMaskAssets effectAssets;
    // The shade's Chip alpha plane, copied from the catalogue when gameplay
    // art binds and again on each gameplay entry, with the stamp payload.
    const auto copyShade=[&]()->bool{
        const auto* shade=effectAssets.shade();
        if(!shade)return false;
        const unsigned bytes=shade->mask.stride*shade->mask.height;
        if(bytes>shadeChipCapacity||shade->mask.bytes<bytes)return false;
        memcpy(shadeChip,shade->mask.data,bytes);return true;
    };
    alignas(downwell::CurrencySmokeComposition) static UBYTE effectStorage[sizeof(downwell::CurrencySmokeComposition)];
    auto& controllerEffects = *new (effectStorage) downwell::CurrencySmokeComposition;
#if defined(DOWNWELL_REPLAY_VERIFY) || defined(DOWNWELL_PLANE_BANK_VERIFY)
    static_assert(DOWNWELL_GAME_BANK_BYTES<=DOWNWELL_GAME_BANK_BUDGET,"Reference art admission budget");
#else
    static_assert(DOWNWELL_CAVERN_BANK_BYTES<=DOWNWELL_CAVERN_BANK_BUDGET,"Cavern art admission budget");
#endif
    gDownwellGameArtExpectedBytes=0;
    const auto releaseTerrainRaster=[&](){
        terrainRaster.releaseStorage();memory.releaseTerrainRaster();
        gDownwellTerrainRasterResidentBytes=0;
    };
    const auto acquireTerrainRaster=[&](){
        if(terrainRaster.hasStorage())return memory.terrainRaster()!=nullptr;
        if(!memory.acquireTerrainRaster(AmigaTerrainRaster::storageBytes)||
           !terrainRaster.bindStorage(memory.terrainRaster(),AmigaTerrainRaster::storageBytes))return false;
        gDownwellTerrainRasterResidentBytes=AmigaTerrainRaster::storageBytes;
        return true;
    };
    const auto releaseFieldColumns=[&](){
#ifdef DOWNWELL_PREPARED_FIELDS
        gameplayColumnBank.invalidate();memory.releaseFieldColumns();
        gDownwellFieldColumnResidentBytes=0;
#endif
    };
    const auto acquireFieldColumns=[&](){
#ifdef DOWNWELL_PREPARED_FIELDS
        if(!memory.acquireFieldColumns(downwell::fieldColumnBytes)||
            !gameplayColumnBank.load(memory.fieldColumns(),downwell::fieldColumnBytes,
                [&](UBYTE* target,unsigned bytes){
                    ++gDownwellFieldColumnLoads;
                    return platform.loadData(AmigaDataFile::columns,target,bytes)==LONG(bytes);
                }))return false;
        gDownwellFieldColumnResidentBytes=downwell::fieldColumnBytes;
#endif
        return true;
    };
    const auto releaseGameplayArt=[&](){
        (void)AmigaTerrainPainter::bindPatterns(nullptr,0);
        amigaRendererReleaseHardwareSpriteCopies();
        if(gDownwellGameArtResidentBytes)++gDownwellGameArtReleases;
        if(gDownwellGamePlaneResidentBytes)++gDownwellGamePlaneReleases;
        releaseFieldColumns();
        amigaBlitterReleaseGamePlanes();
#ifdef DOWNWELL_TERRAIN_BATCH
        (void)AmigaTerrainPainter::bindBatch(nullptr,nullptr,nullptr,0);
#endif
#ifndef DOWNWELL_REFERENCE_ACTOR_PREFIX
        actorFrames.invalidate();
#endif
        // A frame without composited effects leaves the previous rows
        // dirty; bindStorage refuses a dirty surface at the next lease.
        controllerEffects.surface().clear();
        AmigaHardware::blitterDrain();
        effectAssets.release();
        AmigaGameplayActors::releaseDamageText();
        hudText.release();
        gemStreakHud.release();
        timerHud.release();
        titleHud.release();titleView={};
        activeGamePack=activeGamePackEnd=activeEntryMask=nullptr;
        gDownwellEntryMaskResidentBytes=0;
        releaseTerrainRaster();
        memory.releaseGameAssets();memory.releaseGameCatalogue();
        gDownwellGamePlaneResidentBytes=gDownwellGameArtResidentBytes=0;
    };
    const auto acquireGameplayPlanes=[&](){
        DW_PROFILE_SCOPE(profileEntryGameArtAdmission);
#ifndef DOWNWELL_REPLAY_VERIFY
#ifdef DOWNWELL_PLANE_BANK_VERIFY
        constexpr bool artReferenceOracle=true;
#else
        constexpr bool artReferenceOracle=false;
#endif
        if(!AmigaSceneAdmission::acquireGameplay(scenes.scene(),artReferenceOracle)){
            gDownwellSceneAdmissionFailure=1;return false;
        }
#endif
#if defined(DOWNWELL_REPLAY_VERIFY) || defined(DOWNWELL_PLANE_BANK_VERIFY)
        // These explicit full-catalogue pixel oracles include later-area art.
        if(!AmigaGameplayAssetAdmission::referenceOracle(port.runStage().area))return false;
        constexpr ULONG cpuBytes=DOWNWELL_GAME_BANK_BYTES,planeBytes=DOWNWELL_GAME_PLANE_BYTES;
        const char* cpuPath=AmigaDataFile::game;
        const char* planePath=AmigaDataFile::planes;
        const auto* planeHash=downwellGamePlaneSourceHash;
#else
        if(!AmigaGameplayAssetAdmission::production(port.runStage().area))return false;
        constexpr ULONG cpuBytes=DOWNWELL_CAVERN_BANK_BYTES,planeBytes=DOWNWELL_CAVERN_PLANE_BYTES;
        const char* cpuPath=AmigaDataFile::cavern;
        const char* planePath=AmigaDataFile::cavernPlanes;
        const auto* planeHash=downwellCavernPlaneSourceHash;
#endif
        if(gDownwellGameArtResidentBytes)
            return memory.gameAssets()&&activeGamePack&&acquireTerrainRaster()&&acquireFieldColumns();
        gamePackBytes=planeBytes;
        gDownwellGameArtExpectedBytes=cpuBytes;
        gDownwellGamePlaneExpectedBytes=gamePackBytes;
        // The CPU compositor can need this mask after the DMA wipe retires.
        // Keep it with the gameplay lease, never in the startup executable or
        // the shared Chip overlay which shops/results can overwrite.
        static_assert(DOWNWELL_ENTRYMASK_BANK_BYTES==AmigaEntryMask::bytes&&
            DOWNWELL_ENTRYMASK_BANK_BYTES<=DOWNWELL_ENTRYMASK_BANK_BUDGET,
            "Gameplay entry mask residency budget");
#ifdef DOWNWELL_PREPARED_PATTERN_ROWS
        static_assert(DOWNWELL_TERRAIN_BANK_BYTES<=DOWNWELL_TERRAIN_BANK_BUDGET,
            "Prepared terrain artwork residency budget");
        constexpr ULONG terrainArtBytes=DOWNWELL_TERRAIN_BANK_BYTES;
#else
        constexpr ULONG terrainArtBytes=0;
#endif
        if(!memory.acquireGameCatalogue(cpuBytes+DOWNWELL_ENTRYMASK_BANK_BYTES+terrainArtBytes))return false;
        activeGamePack=memory.gameCatalogue();activeGamePackEnd=activeGamePack+cpuBytes;
        UBYTE* entryMask=memory.gameCatalogue()+cpuBytes;
        if(platform.loadData(AmigaDataFile::entryMask,entryMask,
                DOWNWELL_ENTRYMASK_BANK_BYTES)!=LONG(DOWNWELL_ENTRYMASK_BANK_BYTES)){
            releaseGameplayArt();return false;
        }
        activeEntryMask=entryMask;
        gDownwellEntryMaskResidentBytes=DOWNWELL_ENTRYMASK_BANK_BYTES;
        ++gDownwellEntryMaskLoads;
#ifdef DOWNWELL_PREPARED_PATTERN_ROWS
        UBYTE* terrainArt=entryMask+DOWNWELL_ENTRYMASK_BANK_BYTES;
        if(platform.loadData(AmigaDataFile::terrain,terrainArt,terrainArtBytes)!=LONG(terrainArtBytes)||
           !AmigaTerrainPainter::bindPatterns(terrainArt,terrainArtBytes)){
            releaseGameplayArt();return false;
        }
#endif
#ifdef DOWNWELL_ENTRY_WIPE_VERIFY
        for(unsigned i=0;i<AmigaEntryMask::bytes;++i)
            if(entryMask[i]!=amiga_entry_mask[i]){releaseGameplayArt();return false;}
#endif
        bool good=acquireTerrainRaster()&&
            platform.loadData(cpuPath,memory.gameCatalogue(),cpuBytes)==LONG(cpuBytes)&&
            effectAssets.bind(activeGamePack,cpuBytes)&&
            effectAssets.bindEntryMask(activeEntryMask,DOWNWELL_ENTRYMASK_BANK_BYTES)&&
            AmigaGameplayActors::prepareDamageText(activeGamePack,activeGamePackEnd);
        good=good&&acquireFieldColumns();
#ifndef DOWNWELL_REFERENCE_ACTOR_PREFIX
        good=good&&actorFrames.prepare(activeGamePack,activeGamePackEnd);
#endif
#ifdef DOWNWELL_TERRAIN_BATCH
        good=good&&AmigaTerrainPainter::bindBatch(activeGamePack,activeGamePackEnd,sceneBank,sceneBankBytes);
#endif
        good=good&&copyShade()&&AmigaEffectSurface::bind(controllerEffects.surface(),memory.assets(),
            effectStampTable.data,effectStampBytes,effectStamps,effectPayloadBytes,effectAssets,shadeChip,
            shadeChip+shadeChipCapacity,effectMirrorBytes)&&
            AmigaEffectSurface::fixtures(controllerEffects.surface(),effectAssets)&&
            memory.acquireGameAssets(gamePackBytes)&&
            platform.loadData(planePath,memory.gameAssets(),gamePackBytes)==LONG(gamePackBytes)&&
            amigaBlitterBindGamePlanes(activeGamePack,cpuBytes,
                memory.gameAssets(),gamePackBytes,planeHash);
        if(!good){releaseGameplayArt();return false;}
        gDownwellGameArtResidentBytes=cpuBytes;
        gDownwellGamePlaneResidentBytes=gamePackBytes;
        ++gDownwellGameArtLoads;++gDownwellGameArtPreparations;++gDownwellGamePlaneLoads;
        downwellGamePlanesReady();return true;
    };
#if defined(DOWNWELL_REPLAY_VERIFY) || defined(DOWNWELL_PLANE_BANK_VERIFY)
    // Explicit gameplay-only oracles, not ordinary startup.
    if(!acquireGameplayPlanes()){
        gDownwellStartupFailure=7;downwellStartupFailed();
        platform.restore();FreeMem(startupCopperWords,4*sizeof(UWORD));memory.release();return 20;
    }
    gDownwellStartupAssetBytes=gamePackBytes;
#endif
#ifdef DOWNWELL_PLANE_BANK_VERIFY
    if(!amigaBlitterVerifyGamePlanes(activeGamePack,memory.game())){
        gDownwellStartupFailure=7;downwellStartupFailed();
        platform.restore();FreeMem(startupCopperWords,4*sizeof(UWORD));memory.release();return 20;
    }
#endif
    alignas(AmigaGardenScene) static UBYTE gardenStorage[sizeof(AmigaGardenScene)];
    auto& gardenScene=*new(gardenStorage) AmigaGardenScene(memory.menuForeground(), memory.menuBackground(),
        memory.menuEntry(), memory.copper(), memory.copper() + downwellCopperWords,
        downwellCopperWords);
#ifdef DOWNWELL_REPLAY_VERIFY
    const bool initialSplash = false;
#else
    const bool initialSplash = scenes.scene() == AmigaScene::splash;
    gDownwellStartupScene = static_cast<ULONG>(scenes.scene());
#endif
    if (initialSplash && (!loadSceneBank(platform, bootBankName, DOWNWELL_BOOT_BANK_BYTES) ||
        !amigaBuildSplashCache(sceneBank, sceneBank + DOWNWELL_BOOT_BANK_BYTES,
                               memory.sceneCache(), memory.hud()))) {
        platform.restore();
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
        memory.release();
        gDownwellStartupFailure = 9;
        downwellStartupFailed();
        return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
    }
    if (initialSplash) ++gDownwellSplashPreparations;
#if defined(DOWNWELL_SKIP_INTRO) && !defined(DOWNWELL_SKIP_TO_GAME) && !defined(DOWNWELL_REPLAY_VERIFY)
    if (!loadFrontBank(platform)) {
        platform.restore();
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
        memory.release();
        gDownwellStartupFailure = 11;
        downwellStartupFailed();
        return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
    }
#endif

    alignas(AmigaStyleScene) static UBYTE styleStorage[sizeof(AmigaStyleScene)];
    auto& styleScene = *new (styleStorage) AmigaStyleScene(memory.sceneCache(), sceneCacheBytes,
        memory.copper(), memory.copper() + downwellCopperWords, downwellCopperWords,
        memory.game(), memory.hud(), downwellLayerBytes);
#if defined(DOWNWELL_SKIP_INTRO) && !defined(DOWNWELL_SKIP_TO_GAME)
    if (!styleScene.prepare(frontPack, frontPackEnd)) {
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
    AmigaSliceEndScene sliceEnd(memory.copper() + downwellCopperWords * 4, downwellCopperWords);
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
    selectPaletteData(saveState.shader);
    Palette palette(selectedPaletteData,
                    sizeof(selectedPaletteData) / sizeof(selectedPaletteData[0]));

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
#ifdef DOWNWELL_REPLAY_VERIFY
#define DOWNWELL_VERIFY_PART 17
#include "DownwellAmigaReplayVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
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
    // A skipped splash owns no cached pixels. Publish a DMA-disabled list,
    // not a list pointing at unprepared or later scene-owned storage.
    splashBlankList.reset();
    splashBlankList.move(bplcon0, 0);
    for (unsigned channel=0;channel<8;++channel) splashBlankList.showSprite(channel,nullSprite);
    splashBlankList.finish();
    if (
#ifdef DOWNWELL_REPLAY_VERIFY
        !gameplayScene.build(dynamicCopper0,
            gameBitmap, hudBitmap, playerSprites[0], bulletSprites,
            gameplayRegularSpriteCount, enemySprites[0], palette,
            port.ringOrigin())
#else
        initialSplash && (!splashBlankList.buildNarrow(
            amigaSplashFrame(memory.sceneCache(), 1), nullSprite, palette) ||
        !publisherScene.prepare(memory.sceneCache(), sceneCacheBytes, nullSprite, palette) ||
        !creditsScene.prepare(hudBitmap, nullSprite, palette) ||
        !controlsScene.prepare(memory.sceneCache(), sceneCacheBytes, nullSprite, palette))
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
    AmigaKeyboard keyboard;
    const bool keyboardReady=keyboard.acquire();
    while ((*ciaaPra & 0x40u) == 0) platform.waitVblank();
    // After a plain gameplay frame the next simulation step needs no display
    // memory, so it runs while that frame's list waits for its VBlank.
    bool overlapNextStep = false;
    UWORD fencedVblank = AmigaPlatform::vblankCount();
    bool pauseFireLatch = false; // afterPause: a held confirm is not a jump
    downwell::Input pausePrior;
    // W2.6: hand the sound requests to the Paula mixer and publish its
    // commands for the next VBlank: the front end's ring, then the run's.
    // Runs at the top of each iteration, so after the previous one's bank
    // changes, and before a results restart, which rebuilds the run.
    // The pause menu's volume option (option.masterGain in tenths, 1.0 by
    // default, so a full-scale sound plays at Paula volume 64) is
    // audio_master_gain: it scales every voice.
    unsigned appliedGainTenths = ~0u;
    auto drainSounds = [&] {
        auto& audio = soundBanks.audio();
        if (saveState.masterGainTenths != appliedGainTenths) {
            appliedGainTenths = saveState.masterGainTenths;
            audio.setMasterQ16(appliedGainTenths * 65536u / 10u);
        }
#ifndef DOWNWELL_REPLAY_VERIFY // The replay verifier has no scene runtime.
        audio.drain(scenes.sounds().requests());
#endif
        audio.drain(port.soundRequests());
        audio.frame();
    };
    for (;;) {
        if(!keyboardReady)break;
        drainSounds();
#ifndef DOWNWELL_REPLAY_VERIFY
        // W1.10: in gameplay Escape and P are objControlerN's pause input;
        // elsewhere (and at the slice end) Escape still leaves the game.
        unsigned pauseKeys = 0;
        if (scenes.scene() == AmigaScene::gameplay && !sliceEnd.presented) pauseKeys = keyboard.pause();
        if ((*ciaaPra & 0x40u) == 0 || keyboard.quit()) {
            syncRunRecords(port);
            flushSave(platform);
            exitCode = 0;
            break;
        }
        if (scenes.scene() != AmigaScene::gameplay) (void)keyboard.pause();
#else
        if ((*ciaaPra & 0x40u) == 0 || keyboard.quit()) {
            exitCode = 0;
            break;
        }
#endif
        if (sliceEnd.presented) { platform.waitVblank(); continue; }
        decltype(encounterSession)::Frame encounterFrame(encounterSession);
        if(!encounterFrame.valid())break;
#ifdef DOWNWELL_CENSUS
        {
            const bool censusGameplay = scenes.scene() == AmigaScene::gameplay;
            downwellCensusUpdate((unsigned)scenes.scene(), censusGameplay ? port.roomSpeed() : 60,
                censusGameplay ? unsigned(port.runStage().area * 4 + port.runStage().level) : 0u,
                censusGameplay && (port.levelResultRequested() || port.levelClearActive()),
                censusGameplay ? port.cameraOffsetX() : 0);
        }
#endif
        DW_PROFILE_FRAME((unsigned)scenes.scene());
#ifdef DOWNWELL_PREPARED_PATTERN_ROWS
        AmigaFrameBudget frameBudget(frameWorkStamp());
#endif
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
#ifndef DOWNWELL_FRAME_WORK_CANDIDATE
        int destroyedBlockX[AmigaPortRuntime::destroyedCapacity] = {0};
        int destroyedBlockY[AmigaPortRuntime::destroyedCapacity] = {0};
#else
        AmigaPortFrame gameplayResult;
        const auto& destroyedBlockX=gameplayResult.blockX;
        const auto& destroyedBlockY=gameplayResult.blockY;
#endif
        DW_PROFILE_PHASE(profileWait);
        // The spatial tree is derived state. Only observing queries (or log
        // pressure) should materialize it; even idle-time service can overrun
        // VBI and spends cycles on state this gameplay path never reads.
        const bool overlapStep = overlapNextStep;
        overlapNextStep = false;
        bool displayFenced = false;
        // Precedes every display, overlay, sprite or Copper write of this
        // iteration: the previously published list is live, so the back
        // buffers are free, and at most one iteration runs per field.
        auto fenceDisplay = [&] {
            if (displayFenced) return;
            displayFenced = true;
            if (overlapStep) {
                DW_PROFILE_SCOPE(profileWait);
#ifdef DOWNWELL_PREPARED_PATTERN_ROWS
                const auto waitStart=frameWorkStamp();
#endif
                // Only prepare RNG output during an already-required wait.
                // One word is bounded; stop well before the PAL VBlank edge.
                fencedVblank = platform.waitPublished(fencedVblank,[](void* context){
                    return static_cast<AmigaPortRuntime*>(context)->prepareRandomIdleWord();
                },&port,8);
#ifdef DOWNWELL_PREPARED_PATTERN_ROWS
                frameBudget.excludeWait(waitStart,frameWorkStamp());
#endif
            }
            // The preceding detached list has crossed the publication boundary.
            // Only now may another overlay overwrite an entry cover's DMA bank.
            if(residentCover==ResidentCover::entry&&gameplayScene.coverDetached())
                residentCover=ResidentCover::none;
        };
#ifndef DOWNWELL_REPLAY_VERIFY
        {
            // W1.5 fxTimeChange: room_speed 30..55 lengthens each Step to
            // 60/room_speed fields, and its Create sleep(30) stalls ~1.5
            // PAL fields. Present-only waits; the simulation is unchanged.
            static unsigned speedCredit=0,sleepCredit=0;
            const int speed=port.roomSpeed();
            if(speed>0&&speed<60){
                speedCredit+=unsigned(60-speed);
                while(speedCredit>=unsigned(speed)){speedCredit-=unsigned(speed);platform.waitVblank();
#ifdef DOWNWELL_CENSUS
                    downwellCensusMark(censusDeliberate);
#endif
                }
            }else speedCredit=0;
            sleepCredit+=port.takePauseMilliseconds();
            while(sleepCredit>=20u){sleepCredit-=20u;platform.waitVblank();
#ifdef DOWNWELL_CENSUS
                downwellCensusMark(censusDeliberate);
#endif
            }
        }
#endif
        if (!overlapStep) {
            platform.waitVblank();
            fencedVblank = AmigaPlatform::vblankCount();
            fenceDisplay();
        }
#ifdef DOWNWELL_GAMEPLAY_COLLISION_OWNER
        if(!collisionOwner.valid()){exitCode=downwellGameplayFailed(__LINE__);break;}
#endif
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
                exitCode = encounterFrame.finish()?0:20;
                break;
            }
            packedInput = downwell::packReplayInput(replayInput);
            if(!encounterFrame.sampleInput(replayInput))break;
#else
#ifdef DOWNWELL_SCENE_VERIFY
#define DOWNWELL_VERIFY_PART 18
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
#else
            downwell::Input liveInput = AmigaInput::sample();
            liveInput.cancel=keyboard.cancel()||liveInput.cancel;
#ifdef DOWNWELL_PROFILE_AUTORUN
            static unsigned profileMenuFrames = 0;
            liveInput = downwell::Input();
            if (scenes.scene() == AmigaScene::splash)
                liveInput.fire = (liveFrameCount & 1) != 0;
            else if (scenes.scene() == AmigaScene::menu)
                liveInput.right = ++profileMenuFrames > 40;
            else if (scenes.scene() == AmigaScene::gameplay) {
#ifdef DOWNWELL_PROFILE_LASER
                if (!port.upgrades().levels[12]) port.grantUpgrade(12);
#endif
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
#ifndef DOWNWELL_PROFILE_IDLE
                liveInput.right = ((port.tick() / 120) & 1) == 0;
                liveInput.left = !liveInput.right;
                liveInput.fire = (port.tick() % 16) == 0 || (port.tick() % 16) == 3;
#endif
#ifdef DOWNWELL_PROFILE_CONTACT
                liveInput=downwell::gameplayContactProfileInput(port.tick(),
                    240+port.player().offsetX(),176+port.player().offsetY());
#endif
#ifdef DOWNWELL_PROFILE_OVERFLOW
                downwell::gameplayEncounterProfileInput(port.tick(),
                    240+port.player().offsetX(),176+port.player().offsetY(),amigaOverflowRoute,liveInput);
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
#ifdef DOWNWELL_PROFILE_ENTRY
            static downwell::GameplayEntryProfileInput entryProfileInput;
            liveInput=entryProfileInput.sample(static_cast<unsigned>(scenes.scene()),scenes.menuPlayerX());
#endif
#ifdef DOWNWELL_PROFILE_SHOP
            liveInput=downwell::unpackReplayInput(shopProfileInput.packed(port.runStage().level,
                port.levelResultRequested()||port.levelClearActive()));
            gDownwellProfileSection=port.runStage().level==1?
                (port.levelResultRequested()||port.levelClearActive()?1:0):
                port.shops().noControl?4:port.cameraSteadyOffsetX()?3:2;
#endif
#endif
#endif
            // afterPause: the confirmation that closed the menu is not a jump.
            if (pauseFireLatch) { if (liveInput.fire) liveInput.fire = false; else pauseFireLatch = false; }
            packedInput = downwell::packReplayInput(liveInput);
            if(!encounterFrame.sampleInput(liveInput))break;
#ifdef DOWNWELL_PROFILE_IDLE
            if (scenes.scene()==AmigaScene::gameplay) {
                if (packedInput) ++gDownwellIdleInputViolations;
                if (port.bulletSlotEnd()>gDownwellIdleBulletSlots) gDownwellIdleBulletSlots=port.bulletSlotEnd();
            }
#endif
            DW_PROFILE_PHASE(profileSimulation);
            if (!sceneResetRequested) scenes.step(liveInput,encounterSession.inputPublication());
#ifndef DOWNWELL_REPLAY_VERIFY
            if (scenes.takeStyleConfirmed()) {
                // PlayMenu: ini_write_real style, then hardMode.
                saveState.style = scenes.playStyle(); saveState.hardMode = scenes.hardMode();
                flushSave(platform);
            }
#endif
#ifdef DOWNWELL_SCENE_COLLISION_BACKEND
            if(scenes.scene()!=AmigaScene::gameplay)encounterResources.releaseScene();
            else if(encounterResources.ready()&&!encounterResources.bind(encounterFrame))break;
#endif
            DW_PROFILE_PHASE(profileOther);
            {
                const bool presentationChanged =
                    sceneResetRequested ||
                    scenes.scene() != presentedScene ||
                    scenes.splashStage() != presentedSplash;
                sceneResetRequested = false;
                if (presentationChanged || scenes.scene() != AmigaScene::gameplay) fenceDisplay();
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
                    entryHudReady=false;
                    activeHudPack=activeHudPackEnd=nullptr;
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
                    overlayBank.release();
                    gDownwellResultsPack = gDownwellShopPack = nullptr;
                    gDownwellResultsAssetBytes = gDownwellShopAssetBytes = 0;
                    releaseGameplayArt();
                    // The run's sounds leave with its art.
                    soundBanks.scene(downwell::AmigaSoundBanks::none);
                    soundBanks.resident(downwell::AmigaSoundBanks::none);
                    frontPack = frontPackEnd = nullptr;
                    gameplayScene.invalidate();
                }
                if (presentationChanged && (presentedScene == AmigaScene::menu ||
                        presentedScene == AmigaScene::styleSelect)) {
                    // Terrain runs borrow prepared pattern bytes. Art/DMA
                    // teardown alone is insufficient: release only after all
                    // borrowing lifetimes have retired, including Retry paths.
                    if(builders.valid()&&(!nativeTerrain||!nativeTerrain->size())){
                        builders=downwell::BuilderPatternPack(nullptr,0);
                        memory.releaseBuilderPatterns();gDownwellBuilderPatternBytes=0;
                        ++gDownwellBuilderPatternReleases;
                    }
                    if (!frontPack && !loadFrontBank(platform)) break;
                    // The front end's sounds, only while no run bank is resident.
                    if (soundBanks.bank(downwell::AmigaPaulaAudio::resident) == downwell::AmigaSoundBanks::none)
                        soundBanks.scene(downwell::AmigaSoundBanks::front);
                    // Front art is admitted only when a consumer enters. The
                    // garden planes are prepared once per entry, never during
                    // splash/gameplay or a direct style-selection shortcut.
                    if (presentedScene == AmigaScene::menu) {
                        if (!gardenScene.prepare(menuDrawList,frontPack,frontPackEnd)) break;
                        ++gDownwellGardenPreparations;
                    }
                }
                if (presentationChanged && presentedScene == AmigaScene::styleSelect) {
                    entryHudReady=false;
                    activeHudPack=activeHudPackEnd=nullptr;
                    // These framebuffers and copper slots may contain gameplay,
                    // even when the new selection matches the last visit exactly.
                    // Garden prepared the immutable border/masks on entry and
                    // only uses its separate menu planes thereafter. Other
                    // predecessors may have overwritten that shared cache.
                    if (!styleScene.prepare(frontPack, frontPackEnd,
                        priorPresentedScene == AmigaScene::menu))
                        break;
                }
                if (presentedScene == AmigaScene::splash) {
                    if(!encounterFrame.beginDraw())break;
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
                        if(!encounterFrame.publishable())break;
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
#define DOWNWELL_VERIFY_PART 19
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
                    }
                    // Splash owns this frame. Falling through would publish
                    // the gameplay list over it on every stage transition.
                    if(!encounterFrame.finish())break;
                    continue;
                }
                else if (presentedScene == AmigaScene::menu) {
                    if (presentationChanged &&
                        !styleScene.beginPrepare(frontPack, frontPackEnd))
                        break;
                }
                else if (presentedScene != AmigaScene::styleSelect &&
                         presentationChanged) {
                    // Retire intro DMA before reusing its cache as display rings.
                    // Front art is not resident in gameplay; its region is free.
                    frontPack = frontPackEnd = nullptr;
                    splashBlankList.reset();
                    splashBlankList.move(bplcon0, 0);
                    for (unsigned channel = 0; channel < 8; ++channel)
                        splashBlankList.showSprite(channel, nullSprite);
                    splashBlankList.finish();
                    platform.publish(splashBlankList);
                    platform.waitVblank();
                    DW_PROFILE_PHASE(profileEntryFields);
#ifdef DOWNWELL_SCENE_COLLISION_BACKEND
                    if(!encounterResources.acquire(platform)||!encounterResources.bind(encounterFrame)){exitCode=downwellGameplayFailed(__LINE__);break;}
#endif
                    // Free the front end's (or a results screen's) sounds before
                    // the cavern atlas; the run bank follows the atlas.
                    soundBanks.scene(downwell::AmigaSoundBanks::none);
                    if(!prepareInitialGameplay()){exitCode=downwellGameplayFailed(__LINE__);break;}
                    if (!acquireGameplayPlanes()) { exitCode=downwellGameplayFailed(__LINE__); break; }
                    soundBanks.resident(downwell::AmigaSoundBanks::run);
#ifdef DOWNWELL_PREPARED_FIELDS
                    // Terrain batching owns the prefix. Rebind after every
                    // front-bank reload, only after retiring its DMA.
                    AmigaHardware::blitterDrain();
#ifdef DOWNWELL_REFERENCE_FIELD_BANK
                    gameplayFields.invalidateBank();
                    amigaBlitterInvalidateFieldBands();
#endif
                    const bool fieldBankResident=gameplayFields.resident(
                        sceneBank+downwellTerrainBatchBytes,amiga_field_images,
                        amiga_field_images_end-amiga_field_images);
                    DW_PROFILE_PHASE(profileEntrySmallFields);
                    if(fieldBandOffset<downwellTerrainBatchBytes || !gameplayFields.bind(
                        sceneBank+downwellTerrainBatchBytes,fieldBandOffset-downwellTerrainBatchBytes,
                        amiga_field_images,amiga_field_images_end-amiga_field_images,memory.game())){
                        exitCode=downwellGameplayFailed(__LINE__);break;
                    }
#ifdef DOWNWELL_PROFILE_ENTRY
                    gDownwellEntryFields=sceneBank+downwellTerrainBatchBytes;
                    gDownwellEntryFieldBytes=amiga_field_images_end-amiga_field_images;
#endif
                    if(!fieldBankResident){
                        DW_PROFILE_PHASE(profileEntryLargeFields);
                        if(!amigaBlitterBindFieldBands(sceneBank+fieldBandOffset,
                            [](void* p,UBYTE* target,unsigned bytes){
                                return static_cast<AmigaPlatform*>(p)->loadData(
                                    AmigaDataFile::fields,target,bytes)==LONG(bytes);
                            },&platform)){exitCode=downwellGameplayFailed(__LINE__);break;}
                        ++gDownwellFieldBankUploads;
                    }else ++gDownwellFieldBankReuses;
#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
                    if(!amigaFieldVerifyTerrainFixture(memory.game(),(UBYTE*)gameDisplay[0].data,
                        sceneBank+sceneBankBytes-448)||
                        !verifyFieldComposites(memory.game(),(UBYTE*)gameDisplay[0].data)){
                        exitCode=downwellGameplayFailed(__LINE__);break;
                    }
#endif
#endif
                    DW_PROFILE_PHASE(profileEntryWorld);
                    // Simulation ticks span levels; freshness is the load
                    // boundary, not a zero tick on every room entry.
                    const bool freshlyLoadedRoom=newRunWorldPending||port.tick()==0;
                    // W1.10: a new run takes style select's style and mode
                    // (objPlayer_n Create's styleUpdate at 1-1). The first
                    // boot's builder checkpoint was taken before the choice.
                    // A results restart is still at stage 0 here; it begins
                    // the run below, before the builder reads the mode.
                    if (freshlyLoadedRoom && !newRunWorldPending && port.runStage().level <= 1) {
                        if (!port.beginRun(scenes.playStyle(), scenes.hardMode())) {
                            exitCode=downwellGameplayFailed(__LINE__);break;
                        }
                        liveBuilder.applyRunStyle(scenes.playStyle(), scenes.hardMode());
                    }
                    if (newRunWorldPending) {
                        AmigaHardware::blitterDrain();
                        amigaBuildTerrainIndex(emptyTerrain, terrainIndex);
                        terrainTiles.clear();
                        terrainCollision.fragmentCount = 0;
                        // A results restart (stage 0) begins its run at 1-1;
                        // a next-level entry keeps the run.
                        const bool runRestart = port.runStage().area == 0;
                        port.confirmFirstStage();
                        if (runRestart && !port.beginRun(scenes.playStyle(), scenes.hardMode())) {
                            exitCode=downwellGameplayFailed(__LINE__);break;
                        }
                        unsigned style=scenes.playStyle();
                        if (!liveGeneration.loadRoomWithStartField(port,rooms,mainRoom,&roomTerrainPlan,&roomTerrainBackend,
                            [](AmigaPortRuntime& world,AmigaAreaOneTerrainBuilder& builder,
                               const downwell::PackedRoomInstance& instance,void* context){
                                return world.resetAreaOneBuilder(builder,instance.x,instance.y,
                                    *static_cast<unsigned*>(context));
                            },&style)) break;
                        liveGeneration.bind(port);
                        port.configureGenerationStep(generateLiveTerrain, &liveGeneration);
                        port.configureLandingResolver(amigaResolveTerrainLanding, &terrainCollision);
                        port.configureSolidResolver(amigaResolveTerrainSolid, &terrainCollision,
                            amigaResolveTerrainSegment, amigaResolveTerrainPlayerMask,amigaResolveTerrainWallContacts,amigaResolveTerrainPlatformContact,amigaResolveTerrainCorners);
                        port.configureSolidCells(amigaTerrainSolidCells(terrainCollision));
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
                        worldViewX = mainViewX + port.cameraSteadyOffsetX();
                        worldViewY = mainInitialViewY + port.cameraOffsetY();
                        gameplayScene.resetStage();
                        newRunWorldPending = false;
                    }
                    DW_PROFILE_PHASE(profileEntryAssets);
                    AmigaHardware::blitterDrain();
                    memcpy(effectStamps,effectStampTable.data+AmigaEffectStampBank::payloadOffset,effectPayloadBytes);
                    if(effectAssets.shade())copyShade();
                    AmigaEffectSurface::resetMirrors();
                    const unsigned hudBytes = DOWNWELL_HUD_BANK_BYTES;
                    const auto storage = GameplaySceneStorage::partition(
                        menuPlaneBytes * 2 + amigaMenuEntryPlaneBytes,
                        downwellGameRingBytes, hudBytes);
                    // The level-exit and entry cover masks load into the same middle.
                    if (!storage.valid || DOWNWELL_RESULTS_BANK_BYTES > storage.resultsCapacity ||
                        DOWNWELL_SHOP_BANK_BYTES > storage.resultsCapacity ||
                        DOWNWELL_EXITMASK_BANK_BYTES > storage.resultsCapacity ||
                        DOWNWELL_ENTRYMASK_BANK_BYTES > storage.resultsCapacity ||
                        DOWNWELL_PAUSE_BANK_BYTES > storage.resultsCapacity) {
                        ++gDownwellResultsAssetFailures;
                        exitCode=downwellGameplayFailed(__LINE__);
                        break;
                    }
                    UBYTE* hudStorage = memory.menuForeground() + storage.hudOffset;
                    DW_PROFILE_PHASE(profileEntryHudCopy);
#ifdef DOWNWELL_REFERENCE_ENTRY_ASSETS
                    for (unsigned byte = 0; byte < hudBytes; ++byte)
                        hudStorage[byte] = amiga_hud_pack[byte];
#else
                    if (!entryHudReady) {
                        if (platform.loadData(AmigaDataFile::hud,hudStorage,hudBytes)!=LONG(hudBytes) ||
                            !validateTransientBank(hudStorage,hudBytes)) {
                            exitCode=downwellGameplayFailed(__LINE__);
                            break;
                        }
                        ++gDownwellHudLoads;
                    }
#endif
#ifdef DOWNWELL_REFERENCE_SCENE_BANKS
                    // Linked bytes are an independent verifier, never the
                    // shipping upload source. Check the actual file-loaded art.
                    bool hudMatches=hudBytes==unsigned(amiga_hud_pack_end-amiga_hud_pack);
                    for(unsigned byte=0;hudMatches&&byte<hudBytes;++byte)
                        hudMatches=hudStorage[byte]==amiga_hud_pack[byte];
                    if(!hudMatches){exitCode=downwellGameplayFailed(__LINE__);break;}
#endif
                    // Gameplay and upgrade banks reserve this same high-end
                    // region. Keep residency across level handoffs; garden
                    // reuse and fresh style preparation invalidate it first.
                    entryHudReady=true;
                    activeHudPack = hudStorage;
                    activeHudPackEnd = activeHudPack + hudBytes;
#ifdef DOWNWELL_REFERENCE_ENTRY_ASSETS
                    healthHud.bindLinkedCopy(amiga_hud_pack,amiga_hud_pack_end,
                                             activeHudPack,activeHudPackEnd);
#ifdef DOWNWELL_AMMO_FACTORED
                    ammoHud.bindLinkedCopy(amiga_hud_pack,amiga_hud_pack_end,
                                           activeHudPack,activeHudPackEnd);
#endif
#endif
#ifdef DOWNWELL_RESULTS_RESTART_VERIFY
                    // Verify residency after the actual presentation handoff,
                    // including garden/style writers and direct Retry reuse.
                    for(unsigned byte=0;byte<hudBytes;++byte)
                        if(activeHudPack[byte]!=amiga_hud_pack[byte])
                            ++gDownwellResultsHudFailures;
                    ++gDownwellResultsHudChecks;
#endif
#ifdef DOWNWELL_PROFILE_ENTRY
                    gDownwellEntryHud=activeHudPack;
                    gDownwellEntryHudBytes=hudBytes;
#endif
                    DW_PROFILE_PHASE(profileEntryHudBind);
#ifndef DOWNWELL_REFERENCE_HUD_TEXT
                    // Bind HUD text backgrounds here, not on the first gem.
                    hudText.bind(activeHudPack, activeHudPackEnd, activeGamePack, activeGamePackEnd);
#endif
                    if(!gemStreakHud.bind(activeGamePack, activeGamePackEnd)){exitCode=downwellGameplayFailed(__LINE__);break;}
                    if(!titleHud.bind(activeGamePack, activeGamePackEnd)){exitCode=downwellGameplayFailed(__LINE__);break;}
                    if(!timerHud.bind(activeGamePack, activeGamePackEnd)){exitCode=downwellGameplayFailed(__LINE__);break;}
                    overlayAssetStorage = memory.menuForeground() + storage.resultsOffset;
                    overlayAssetCapacity = storage.resultsCapacity;
                    residentCover = ResidentCover::none;
                    // No overlay is required by a living player. The first
                    // actual shop/results consumer acquires its own bank.
                    overlayBank.release();
                    gDownwellResultsPack = gDownwellShopPack = nullptr;
                    gDownwellResultsAssetBytes = gDownwellShopAssetBytes = 0;
                    shopMenuOverlay.reset();
#ifdef DOWNWELL_RESULTS_RESTART_VERIFY
                    if(gDownwellResultsPack || gDownwellShopPack || gDownwellResultsAssetBytes || gDownwellShopAssetBytes)
                        ++gDownwellResultsBankFailures;
                    ++gDownwellResultsBankChecks;
#endif
                    DW_PROFILE_PHASE(profileEntryRaster);
                    Bitmap stageClear(stagedBuffer, downwellVisibleWidth,
                        downwellGameRingHeight, 2, false, false, downwellDmaWidth);
                    stageClear.clear();
                    AmigaHardware::blitterDrain();
                    if (!terrainRaster.ready)
                        prepareTerrainRaster(&renderer, rooms, mainRoom, builders,
                            fragments, liveBuilder.fragmentCount(),
                            generatedDecorations, liveBuilder.decorationCount(), &port, freshlyLoadedRoom);
                    DW_PROFILE_PHASE(profileEntryBacking);
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
#ifdef DOWNWELL_PROFILE_OVERFLOW
                            // The source-backed skip-to-game checkpoint has
                            // released entry input. Route tick zero is the
                            // first gameplay input, not a held confirmation.
                            port.initializeEntryInput(false);
#else
                            port.initializeEntryInput((packedInput & 4) != 0);
#endif
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
                    DW_PROFILE_PHASE(profileOther);
                }
                if (presentedScene == AmigaScene::menu) {
                    if(!encounterFrame.beginDraw())break;
                    if (presentationChanged)
                        gardenScene.enter();
                    AmigaGardenScene::Frame garden;
                    if (!gardenScene.render(back, scenes, menuEntry, nullSprite, palette, garden))
                        break;
                    const auto& menuScroll = garden.scroll;
                    UWORD* menuPlayerData = garden.player;
#ifdef DOWNWELL_SCENE_VERIFY
#define DOWNWELL_VERIFY_PART 20
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
                    DW_PROFILE_PHASE(profilePublish);
                    if(!encounterFrame.publishable())break;
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
#define DOWNWELL_VERIFY_PART 21
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
                    if(!encounterFrame.finish())break;
                    continue;
                }
                if (presentedScene == AmigaScene::styleSelect) {
                    if(!encounterFrame.beginDraw())break;
                    AmigaStyleScene::Frame style;
                    if (!styleScene.render(back, scenes, style))
                        break;
#ifdef DOWNWELL_SCENE_VERIFY
#define DOWNWELL_VERIFY_PART 22
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
                    DW_PROFILE_PHASE(profileCopper);
                    if (!styleScene.updateCopper(back, style, nullSprite, palette))
                        break;
#ifdef DOWNWELL_SCENE_VERIFY
#define DOWNWELL_VERIFY_PART 23
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
                    DW_PROFILE_PHASE(profilePublish);
#ifdef DOWNWELL_RESULTS_RESTART_VERIFY
                    if(gDownwellEncounterRejectDraw){
                        port.withShopStepState([](auto&,auto&){return false;});
                        gDownwellEncounterDrawRejected=1;
                    }
#endif
                    if(!encounterFrame.publishable())break;
#ifdef DOWNWELL_RESULTS_RESTART_VERIFY
                    if(gDownwellEncounterRejectDraw)++gDownwellEncounterFailurePublications;
#endif
                    platform.publish(styleScene.copper(back));
                    front = back;
                    DW_PROFILE_PHASE(profileOther);
#if defined(DOWNWELL_PREPARED_FIELDS) && !defined(DOWNWELL_REFERENCE_FIELD_BANK)
                    static_assert(DOWNWELL_FRONT_STYLE_BYTES<=downwellTerrainBatchBytes,
                        "Small-field staging must not overwrite style sources");
                    if(!presentationChanged&&!style.paintedPixels)
                        gameplayFields.stageBank(sceneBank+downwellTerrainBatchBytes,
                            fieldBandOffset-downwellTerrainBatchBytes,amiga_field_images,
                            amiga_field_images_end-amiga_field_images,256);
#endif
#ifdef DOWNWELL_SCENE_VERIFY
#define DOWNWELL_VERIFY_PART 24
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
                    if(!encounterFrame.finish())break;
                    continue;
                }
                if (presentationChanged) {
                    if(!encounterFrame.beginDraw())break;
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
                    if(!encounterFrame.publishable())break;
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
                    if(!encounterFrame.finish())break;
                    continue;
                }
            }
            if (scenes.scene() != AmigaScene::gameplay){
                if(!encounterFrame.finish())break;
                continue;
            }
#ifdef DOWNWELL_SKIP_LEVEL
            if (skipLevelPending) {
                // Initialization has now bound terrain and the entry RNG.
                // Only the first level is skipped; later levels play normally.
                port.registerLevelExit(176 + port.player().offsetY());
                skipLevelPending = false;
            }
#endif
            if (port.levelResultRequested() || port.levelClearActive()) {
                fenceDisplay();
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
                    overlayBank.release();
                    gDownwellResultsPack = gDownwellShopPack = nullptr;
                    gDownwellResultsAssetBytes = gDownwellShopAssetBytes = 0;
                    releaseGameplayArt();
                    soundBanks.scene(downwell::AmigaSoundBanks::clear); // in the atlas's room
#ifdef DOWNWELL_CLEAR_VERIFY
                    if(!clearOracle.acquire()){exitCode=downwellGameplayFailed(__LINE__);break;}
                    gDownwellClearOracleBytes=AmigaClearOracleBuffers::bytes;
#endif
                    if (!upgradeSource.acquire()) { exitCode=downwellGameplayFailed(__LINE__); break; }
                    if (!clear.enter(port, scenes.playStyle(), liveInput,
                            memory.menuForeground(), menuPlaneBytes * 2 + amigaMenuEntryPlaneBytes,
                            DOWNWELL_HUD_BANK_BYTES, upgradeSource.view().data, upgradeSource.view().bytes,
                            upgradeSource.mask(), DOWNWELL_SELECTIONMASK_BANK_BYTES)) { exitCode=downwellGameplayFailed(__LINE__); break; }
#ifdef DOWNWELL_CLEAR_VERIFY
                    gDownwellClearPhase = 2;
#endif
                    for (unsigned buffer = 0; buffer < 2; ++buffer) {
                        gameDisplay[buffer].data = memory.sceneCache() + buffer * downwellGameRingBytes;
                        gameplayScene.invalidate(buffer);
                    }
                } else if (!port.nextLevelRequested()) {
                    DW_PROFILE_SCOPE(profileClearStep);
                    if (!clear.step(port, liveInput)) { exitCode=downwellGameplayFailed(__LINE__); break; }
                }
                if (port.nextLevelRequested() && port.runStage().area == 1 && port.runStage().level == 3) {
                    // Finish Result's original Alarm 1 before stopping at the
                    // port boundary. Retire DMA before reusing its bitplanes.
                    splashBlankList.reset(); splashBlankList.move(bplcon0,0);
                    for (unsigned channel=0;channel<8;++channel) splashBlankList.showSprite(channel,nullSprite);
                    splashBlankList.finish(); platform.publish(splashBlankList); platform.waitVblank();
                    AmigaHardware::blitterDrain();
                    clear.invalidateDisplay();
                    const UBYTE* bank=memory.menuForeground()+clear.assets.layout.resultsOffset;
                    if (!port.finishWorldOneSlice() ||
                        !sliceEnd.prepare(memory.game(),memory.hud(),gameBitmap,bank,clear.assets.bankBytes,nullSprite,palette)) {
                        exitCode=downwellGameplayFailed(__LINE__);break;
                    }
                    // The slice ends the run here: record it as saveStats would.
                    (void)port.recordRunEnd();
                    syncRunRecords(port);
                    flushSave(platform);
                    if (!encounterFrame.beginDraw() || !encounterFrame.publishable()) break;
                    platform.publish(sliceEnd.presentation());
                    sliceEnd.presented=true;
                    upgradeSource.release();
                    if (!encounterFrame.finish()) break;
                    platform.waitVblank();
                    downwellWorldOneComplete();
                    continue;
                }
                if (port.nextLevelRequested() && port.runStage().area == 1 && port.runStage().level < 3) {
                    splashBlankList.reset(); splashBlankList.move(bplcon0, 0);
                    for (unsigned channel = 0; channel < 8; ++channel) splashBlankList.showSprite(channel, nullSprite);
                    splashBlankList.finish(); platform.publish(splashBlankList); platform.waitVblank();
                    AmigaHardware::blitterDrain();
                    clear.invalidateDisplay();
                    upgradeSource.release();
                    soundBanks.scene(downwell::AmigaSoundBanks::none); // before the atlas reloads
#ifdef DOWNWELL_CLEAR_VERIFY
                    clearOracle.release();gDownwellClearOracleBytes=0;
#endif
                    {
                        DW_PROFILE_SCOPE(profileEntryWorld);
                        if (!port.prepareNextAreaOneLevel(liveInput, initialRingOrigin)) { exitCode=downwellGameplayFailed(__LINE__); break; }
                    }
                    clear.presented = false;
                    newRunWorldPending = true; sceneResetRequested = true;
#ifdef DOWNWELL_CLEAR_VERIFY
                    ++gDownwellClearReentries;
                    gDownwellClearGameplayUpdates = 0;
#endif
                    if(!encounterFrame.finish())break;
                    continue;
                }
                if(!encounterFrame.beginDraw())break;
                const auto& room = port.levelClear();
                const auto& snapshot = port.levelClearFrame();
                const bool bankChanged = clear.needsArtwork(port);
                if (bankChanged) {
                    DW_PROFILE_SCOPE(profileClearBank);
                    AmigaHardware::blitterDrain();
                    if (!clear.refreshArtwork(port, memory.menuForeground(), upgradeSource.view().data, upgradeSource.view().bytes)) {
                        exitCode=downwellGameplayFailed(__LINE__); break;
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
                if (!clear.render(back, memory.hud(), memory.game(), target.hud, bank,
                        bank + clear.assets.bankBytes, port, frame)) { exitCode=downwellGameplayFailed(__LINE__); break; }
                DW_PROFILE_PHASE(profileClearSprites);
                for (unsigned channel = 0; channel < 8; ++channel) {
                    Sprite& sprite = channel == 0 ? playerSprites[back] :
                        actorSpriteAt(bulletSprites + back * gameplayRegularSpriteCount, enemySprites[back], channel - 1);
                    sprite.hide();
                }
                unsigned playerId = port.player().spriteId(), playerFrame = port.player().spriteFrame();
                // The upgrade bank keeps style 0's living poses only (its
                // DMA budget); other styles use the matching usual pose.
                switch (playerId) {
                case 25: case 26: case 28: case 29: playerId = 27; break;
                case 32: case 33: case 34: case 35: playerId = 31; break;
                case 38: case 40: case 42: playerId = 37; break;
                default: break;
                }
                const auto playerArt = upgradeSource.view();
                const unsigned parts = amigaRendererHardwareSpritePartCount(playerArt.data, playerArt.data + playerArt.bytes, playerId, playerFrame);
                bool ready = parts > 0 && parts <= 8;
                for (unsigned part = 0; ready && part < parts; ++part) {
                    Sprite& sprite = part == 0 ? playerSprites[back] :
                        actorSpriteAt(bulletSprites + back * gameplayRegularSpriteCount, enemySprites[back], part - 1);
                    ready = amigaRendererBuildHardwareSpritePart(playerArt.data, playerArt.data + playerArt.bytes, playerId, playerFrame, part,
                        frame.playerX, frame.playerY, port.player().facing() < 0, sprite.data(), playerSpriteHeight) != 0;
                }
                DW_PROFILE_PHASE(profileClearCopper);
                const UBYTE* mask = port.exitDither().active ? memory.menuForeground() +
                    std::uint16_t(AmigaExitMask::offset(port.exitDither().y - room.motion.viewY())) /
                    AmigaExitMask::stride * AmigaExitMask::selectionStride : nullptr;
                if (!ready || !clear.updateDisplay(back, frame, mask, playerSprites[back],
                        bulletSprites + back * gameplayRegularSpriteCount, gameplayRegularSpriteCount, enemySprites[back], palette)) {
                    exitCode=downwellGameplayFailed(__LINE__); break;
                }
                DW_PROFILE_PHASE(profileClearDrain);
                AmigaHardware::blitterDrain();
#ifdef DOWNWELL_CLEAR_VERIFY
#define DOWNWELL_VERIFY_PART 25
#include "DownwellAmigaClearVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
                DW_PROFILE_PHASE(profileClearPublish);
                if(!encounterFrame.publishable())break;
                platform.publish(clear.copper(back)); front = back;
                if(!encounterFrame.finish())break;
                continue;
            }
#endif
#ifndef DOWNWELL_REPLAY_VERIFY
            // W1.10 objPauseMenu. objControlerN opens it on pauseInput while
            // the player can act in rmMain; the paused world neither steps
            // nor draws, and the menu owns the fixed PF2 well until it closes.
            {
                bool shopMenuOpen = false;
                for (const auto& shop : port.shops().instances)
                    if (shop.present && shop.menu.active) shopMenuOpen = true;
                const bool pausable = pauseTextReady && activeGamePack && !port.dead() &&
                    !port.deathResults().active && !port.shops().noControl && !shopMenuOpen &&
                    !gDownwellShopPack && !port.exitDither().active && residentCover == ResidentCover::none &&
                    !port.deathSequence().flashVisible();
                if ((pauseKeys & 1) && !pauseMenu.open() && (port.dead() || port.deathResults().active)) {
                    // Escape on the death screens still leaves the game.
                    syncRunRecords(port); flushSave(platform);
                    (void)encounterFrame.finish(); exitCode = 0; break;
                }
                bool opened = false;
                if (!pauseMenu.open() && pauseKeys && pausable &&
                    (pauseScratch || (pauseScratch = static_cast<UBYTE*>(
                        AllocMem(AmigaPauseOverlay::scratchBytes, MEMF_ANY))))) {
                    fenceDisplay();
                    if (!selectPauseBank().data) { exitCode=downwellGameplayFailed(__LINE__); break; }
                    AmigaHardware::blitterDrain();
                    AmigaPauseOverlay::begin(&renderer, pauseScratch);
                    pauseMenu.openPause(false);
                    // objPauseMenu Create (audio_pause_all), then sound 326.
                    port.pauseAllSounds();
                    port.playUiSound(pauseMenu.sound);
                    pauseKey = {};
                    pausePrior = liveInput;
                    opened = true;
                }
                if (pauseMenu.open()) {
                    fenceDisplay();
                    using Menu = downwell::PauseMenu;
                    Menu::Action action = Menu::Action::none;
                    const auto unlock = downwell::UnlockProgress::from(saveState.totalGems);
                    if (!opened) {
                        // The menu is created during objControlerN's Step and
                        // first steps on the next update.
                        Menu::Input in;
                        in.leftPressed = liveInput.left && !pausePrior.left;
                        in.rightPressed = liveInput.right && !pausePrior.right;
                        in.left = liveInput.left; in.right = liveInput.right;
                        in.confirm = liveInput.fire && !pausePrior.fire;
                        in.cancel = liveInput.cancel && !pausePrior.cancel;
                        in.pause = pauseKeys != 0;
                        pausePrior = liveInput;
                        const unsigned oldShader = saveState.shader;
                        action = pauseMenu.step(in, saveState, unlock);
                        if (pauseMenu.sound >= 0) port.playUiSound(pauseMenu.sound);
                        if (saveState.shader != oldShader) {
                            selectPaletteData(saveState.shader);
                            palette.assign(selectedPaletteData);
                            gameplayScene.repaint(palette);
                        }
                    }
                    if (action == Menu::Action::none) {
                        const auto bank = overlayBank.view(TransientOwner::pause);
                        if (!bank.data) { exitCode=downwellGameplayFailed(__LINE__); break; }
                        const auto key = AmigaPauseOverlay::key(pauseMenu, saveState);
                        if (key != pauseKey || pauseMenu.screen == Menu::Screen::palette) {
                            Menu::View view;
                            view.text = &pauseText;
                            view.palettes = downwell::palettes::entries;
                            view.paletteCount = downwell::palettes::count;
                            view.save = &saveState; view.unlock = unlock;
                            view.area = port.runStage().area; view.level = port.runStage().level;
                            view.hard = port.hardMode();
                            AmigaHardware::blitterDrain();
                            AmigaPauseOverlay::compose(&renderer, pauseScratch, {bank.data, bank.data + bank.bytes,
                                activeGamePack, activeGamePackEnd}, pauseMenu, view, 0, 284);
                            pauseKey = key;
                        }
                        if(!encounterFrame.finish())break;
                        continue;
                    }
                    // Closing: give the well back to the HUD's own overlays.
                    AmigaHardware::blitterDrain();
                    AmigaPauseOverlay::clear(&renderer);
                    FreeMem(pauseScratch, AmigaPauseOverlay::scratchBytes); pauseScratch = nullptr;
                    overlayBank.release();
                    invalidateHudText(); gemStreakHud.invalidate(); streakView = {}; invalidateTitle();
                    flushSave(platform);
                    if (action == Menu::Action::resume) {
                        port.resumeAllSounds();
                        pauseFireLatch = liveInput.fire;
                        if(!encounterFrame.finish())break;
                        continue;
                    }
                    if (action == Menu::Action::quit) {
                        syncRunRecords(port); flushSave(platform);
                        (void)encounterFrame.finish(); exitCode = 0; break;
                    }
                    // RETRY and STYLE SELECT: ResetPlayer without saveStats.
                    newRunSeed = liveFrameCount;
                    drainSounds();
                    const auto destination = port.restartFromResults(action == Menu::Action::retry ?
                        downwell::DeathMenuSequence::Action::Retry : downwell::DeathMenuSequence::Action::StyleSelect,
                        newRunSeed, initialRingOrigin);
                    soundBanks.scene(downwell::AmigaSoundBanks::none);
                    if (!scenes.enterResultsDestination(destination)) break;
                    newRunWorldPending = true;
                    sceneResetRequested = true;
                    resultsWasActive = false;
                    if(!encounterFrame.finish())break;
                    continue;
                }
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
            oldShopArt.capture(port.shops());
#ifdef DOWNWELL_CAMERA_VERIFY
#define DOWNWELL_VERIFY_PART 26
#include "DownwellAmigaCameraVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
            checkSceneStack(6);
            if(!AmigaSceneAdmission::stepGameplay(
#ifdef DOWNWELL_REPLAY_VERIFY
                    AmigaScene::gameplay, // Replay harness has no front-end scene owner.
#else
                    scenes.scene(),
#endif
                    activeGamePack!=nullptr, memory.gameAssets()!=nullptr,
                    activeEntryMask!=nullptr, terrainRaster.hasStorage())){
                gDownwellSceneAdmissionFailure=2;exitCode=downwellGameplayFailed(__LINE__);break;
            }
#ifndef DOWNWELL_FRAME_WORK_CANDIDATE
            const AmigaPortFrame result = port.step(packedInput);
#else
            gameplayResult=port.step(packedInput);
            const auto& result=gameplayResult;
#endif
#ifdef DOWNWELL_OVERFLOW_VERIFY
#define DOWNWELL_VERIFY_PART 27
#include "DownwellAmigaOverflowVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
#ifdef DOWNWELL_PROFILE_SHOP
            if(!shopProfileInput.advance(port.runStage().level))++gDownwellShopProfileFailures;
            gDownwellShopProfileFrames[0]=shopProfileInput.frames(1);
            gDownwellShopProfileFrames[1]=shopProfileInput.frames(2);
            if(port.cameraSteadyOffsetX())++gDownwellShopProfileSideFrames;
            if(port.shops().noControl)++gDownwellShopProfileMenuFrames;
            if(shopProfileInput.done()){
                const long actual[]={port.health(),static_cast<long>(port.currency()),
                    port.player().offsetX(),port.player().offsetY()};
                for(unsigned i=0;i<4;++i)if(actual[i]!=downwell::shopProfileFinal[i])++gDownwellShopProfileFailures;
                if(port.randomDrawCount()!=downwell::shopProfileRandom[0]||
                    port.randomState().stateIndex()!=downwell::shopProfileRandom[1])++gDownwellShopProfileFailures;
                for(unsigned i=0;i<16;++i)if(port.randomState().stateWord(i)!=downwell::shopProfileRandom[i+2])++gDownwellShopProfileFailures;
                if(port.dead()||port.cameraSteadyOffsetX()||port.shops().noControl)++gDownwellShopProfileFailures;
                gDownwellShopProfileDone=1;
                gDownwellShopHud=activeHudPack;
                gDownwellShopHudBytes=activeHudPackEnd-activeHudPack;
#ifdef DOWNWELL_PREPARED_FIELDS
                // Keep both diagnostics in the forced-upload reference too;
                // LTO otherwise drops its provably unused reuse counter.
                (void)gDownwellFieldBankUploads;(void)gDownwellFieldBankReuses;
#endif
                gDownwellProfileTargetFrames=gDownwellProfileFrames[3]+1;
            }
#endif
            fenceDisplay();
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
#define DOWNWELL_VERIFY_PART 28
#include "DownwellAmigaClearVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
            if (!port.effectsSucceeded()) { exitCode=downwellGameplayFailed(__LINE__); break; }
            if (!port.generationSucceeded()) { exitCode=downwellGameplayFailed(__LINE__); break; }
#ifndef DOWNWELL_REPLAY_VERIFY
            // Invalidate before selecting a precomposed side-room ring.
            invalidateShopRaster(port.shops());
#ifdef DOWNWELL_FRAME_WORK_CANDIDATE
            if(port.shops().allocatedCount)
#endif
            for (unsigned i = 0; i < downwell::ShopWorld::capacity; ++i)
                if (oldShopArt.changed(i,port.shops().instances[i])) gameplayScene.invalidateStage();
            const auto resultsAction = port.deathResults().inputEffects.action;
            if (resultsAction != downwell::DeathMenuSequence::Action::None) {
                // An entropy boundary, separate from the gameplay RNG stream.
                // Frame timing supplies a live seed; fixtures call reset with explicit seeds.
                newRunSeed = liveFrameCount;
                drainSounds();
                const auto destination = port.restartFromResults(resultsAction, newRunSeed, initialRingOrigin);
                soundBanks.scene(downwell::AmigaSoundBanks::none);
                if (!scenes.enterResultsDestination(destination)) break;
#ifdef DOWNWELL_RESULTS_RESTART_VERIFY
                gDownwellResultsRestartMask |= 1u << (static_cast<unsigned>(resultsAction) - 1);
                ++gDownwellResultsRestartCount;
#endif
                newRunWorldPending = true;
                sceneResetRequested = true;
                resultsWasActive = false;
                if(!encounterFrame.finish())break;
                continue;
            }
#endif
            DW_PROFILE_PHASE(profileTitle);
            // The runner sweeps marked instances at Draw entry. Inter-phase
            // reconciliation deliberately keeps those identities alive.
            if (!encounterFrame.beginDraw(true)) { exitCode=downwellGameplayFailed(__LINE__); break; }
#ifdef DOWNWELL_ENCOUNTER_UPDATE_FAILURE_VERIFY
            ++gDownwellEncounterUpdateDraws;
#endif
            scrollDelta = result.scrollDelta;
            cameraXChanged = result.cameraXChanged;
#ifdef DOWNWELL_CAMERA_VERIFY
            gDownwellCameraColumns |= 1u << ((port.cameraSteadyOffsetX() + 160) / 160);
            if (cameraXChanged) ++gDownwellCameraTransitions;
#endif
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
            gDownwellProfileScroll = scrollDelta;
            gDownwellProfileCameraChange = result.cameraXDelta;
#endif
            destroyedBlockCount = result.destroyedBlockCount;
#ifndef DOWNWELL_FRAME_WORK_CANDIDATE
            for (unsigned block = 0; block < destroyedBlockCount; ++block) {
                destroyedBlockX[block] = result.blockX[block];
                destroyedBlockY[block] = result.blockY[block];
            }
#endif
            // A box gone this update must not come back from retained rows:
            // a camera column change or exposure later in this frame
            // transfers cached spans before the box repaint below.
            for (unsigned block = 0; block < destroyedBlockCount; ++block) {
                const int centerY = 176 + result.blockY[block];
                terrainRaster.invalidate(centerY - 8, centerY + 8);
            }
            if (destroyedBlockCount != 0 && !port.compactScenery().empty()) {
                // Scenery art changed under a repainted cell: broken plants,
                // a flashing/opened gem chunk, a placed or removed torch.
                int first = result.blockY[0], last = first;
                for (unsigned block = 1; block < destroyedBlockCount; ++block) {
                    if (result.blockY[block] < first) first = result.blockY[block];
                    if (result.blockY[block] > last) last = result.blockY[block];
                }
                first += 176 - 16; last += 176 + 16;
                for (unsigned d = 0; d < liveBuilder.decorationCount(); ++d) {
                    auto& decoration = generatedDecorations[d];
                    if (decoration.y < first || decoration.y > last ||
                        (decoration.objectId != 289 && decoration.objectId != 293 && decoration.objectId != 296)) continue;
                    int sprite = decoration.spriteId < 0 ? (decoration.objectId == 296 ? 568 : -1) : decoration.spriteId;
                    int frame = decoration.frame;
                    if (port.sceneryAppearance(decoration.objectId, decoration.x, decoration.y, sprite, frame)) {
                        decoration.spriteId = sprite; decoration.frame = frame;
                    } else decoration.spriteId = -1;
                }
            }
            if (result.ammoChanged)
                redrawAmmo(&renderer, (unsigned)port.player().ammo(), (unsigned)port.player().ammoCapacity(), port.ammoHudOffset(), &port.ammoGauge(), port.ammoMeterOffset());
            if (port.upgrades().count != shownUpgradeCount && activeGamePack)
                redrawUpgradeIcons(&renderer, port.upgrades());
            if (result.healthChanged)
                redrawHealth(&renderer, (unsigned)port.health(), (unsigned)port.healthMaximum(),
                             port.healthReserve().pieces, port.healthReserve().piecesPerHeart);
            std::int32_t streakWorldY=0;
            const int streakViewY=mainInitialViewY+port.cameraOffsetY();
            if(!port.drawGemStreakRise(streakViewY,streakWorldY)){exitCode=downwellGameplayFailed(__LINE__);break;}
            // scrDrawHud4x3 counts currencyText towards the currency; Top
            // then advances the streak block's Draw state.
            const bool streakDrawn=port.drawGemStreakHud();
            if (port.currencyText() != shownCurrencyText) {
                shownCurrencyText = port.currencyText();
                redrawCurrency(&renderer, shownCurrencyText);
            }
            const int nextStreakOffset=port.gemStreak()&&!port.dead()?streakWorldY-streakViewY-50:streakYOffset;
            const int previousStreakOffset=streakYOffset;
            streakYOffset=nextStreakOffset;
            const auto nextStreakView=AmigaGemStreakHud::view(port.gemStreakHud(),streakDrawn,port.gemStreak(),
                port.upgrades().levels[20]!=0,50+streakYOffset);
            const bool streakViewChanged=nextStreakView!=streakView;
            (void)wasDead;
            // The level-clear handoff releases the game art before its last
            // Draw-only iteration; the clear scene repaints the HUD itself.
            if (streakViewChanged && !port.deathResults().active && activeGamePack) {
                streakView=nextStreakView;
                if(!redrawStreak(&renderer,streakView)){exitCode=downwellGameplayFailed(__LINE__);break;}
            }
            // The death flash covers the playfield; the title returns after it.
            if ((port.compactLevelTitle().live || titleView.length) && activeGamePack &&
                !port.deathSequence().flashVisible() && !port.deathResults().active) {
                const auto nextTitleView = AmigaLevelTitleHud::view(port.compactLevelTitle());
                if (nextTitleView != titleView) {
                    titleView = nextTitleView;
                    if (!titleHud.bind(activeGamePack, activeGamePackEnd) || !titleHud.draw(&renderer, titleView)) {
                        exitCode=downwellGameplayFailed(__LINE__);break;
                    }
                }
            }
            // W1.10 timerNumber: global.gameTime is the run clock.
            if (activeGamePack && !port.deathResults().active &&
                !timerHud.draw(&renderer, AmigaTimerHud::view(saveState.showTimer, port.runTimeTicks()))) {
                exitCode=downwellGameplayFailed(__LINE__);break;
            }
            if (wasDeathFlash != port.deathSequence().flashVisible()) {
                // PF2 is above the world AND hardware sprites. Only touch this
                // overlay on the two visibility edges; preserve the terrain
                // backing and both hardware-scrolled world buffers throughout.
                composeDeathFlash(&renderer, port.deathSequence().flashVisible());
            }
            const downwell::ShopWorld::Instance* activeShop = nullptr;
#ifdef DOWNWELL_FRAME_WORK_CANDIDATE
            if(port.shops().allocatedCount)
#endif
            for (const auto& shop : port.shops().instances)
                if (shop.present && shop.menu.active) { activeShop = &shop; break; }
            if (activeShop && !port.exitDither().active && !port.deathResults().active && !selectOverlayAssetBank(true)) {
                exitCode=downwellGameplayFailed(__LINE__);
                break;
            }
            if (!port.deathResults().active && !port.deathSequence().flashVisible() && gDownwellShopPack) {
                if (streakViewChanged)
                    shopMenuOverlay.invalidateStreak(previousStreakOffset,streakYOffset);
                if (!shopMenuOverlay.draw(&renderer, gDownwellShopPack,
                        gDownwellShopPack + gDownwellShopAssetBytes, activeShop,
                        mainViewX + port.cameraSteadyOffsetX(), mainInitialViewY + port.cameraOffsetY(),
                        port.currency(), [](AmigaRenderer* target, void* context) {
                            composeStreak(target, *static_cast<const AmigaGemStreakHud::View*>(context));
                        }, &streakView)) { exitCode=downwellGameplayFailed(__LINE__); break; }
            } else shopMenuOverlay.reset();
#if defined(DOWNWELL_SHOP_VERIFY) || defined(DOWNWELL_NATURAL_VERIFY)
#define DOWNWELL_VERIFY_PART 29
#include "DownwellAmigaShopVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
            if (!activeShop && gDownwellShopPack) {
                // draw(nullptr) above repairs the old menu footprint first.
                AmigaHardware::blitterDrain();
                overlayBank.release();
                gDownwellShopPack = nullptr; gDownwellShopAssetBytes = 0;
            }
            if (port.deathResults().active && !port.exitDither().active) {
#ifdef DOWNWELL_PROFILE_ENTRY
                ++gDownwellEntryResultsFrames;
#endif
#ifndef DOWNWELL_REPLAY_VERIFY
                if (!resultsWasActive) soundBanks.scene(downwell::AmigaSoundBanks::results);
                if (!resultsWasActive) {
                    // saveStats ran at the first death movement; write it as
                    // DeathMenu opens (not mid-fall), with checkAreaLevel.
                    syncRunRecords(port);
                    flushSave(platform);
                }
#endif
                if (!selectOverlayAssetBank(false)) { exitCode=downwellGameplayFailed(__LINE__); break; }
                const bool backdropChanged = redrawResultsBackdrop(&renderer, port,
#ifndef DOWNWELL_REPLAY_VERIFY
                    !resultsWasActive
#else
                    false
#endif
                );
                {
                    // DeathMenu Draw state of the unlock notice.
                    const int stage = port.deathResults().menu.goalNotice;
                    if (stage == 1 && resultsNotice.stage == 0) { resultsNotice.desc = 4; resultsNotice.text = 4; }
                    resultsNotice.stage = stage;
                    if (stage >= 2 && resultsNotice.desc > 0) --resultsNotice.desc;
                    if (stage >= 3) {
                        resultsNotice.frameTenths += 3;
                        if (resultsNotice.text > 0) --resultsNotice.text;
                    }
                }
                const bool noticeChanged = !(resultsNotice == shownResultsNotice);
                // The notice covers rows and part of the bar: repaint under it.
                const bool underNotice = backdropChanged ||
                    (noticeChanged && (resultsNotice.stage >= 2 || shownResultsNotice.stage >= 2));
                const bool rowsRedrawn = redrawResultsRows(&renderer, port, underNotice);
                const bool progressRedrawn = redrawResultsProgress(&renderer, port.deathResults(), underNotice);
                if (resultsNotice.stage >= 2 && (underNotice || rowsRedrawn || progressRedrawn))
                    composeResultsNotice(&renderer, resultsNotice, port.deathResults().progress.goal);
                shownResultsNotice = resultsNotice;
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
                worldViewX = mainViewX + port.cameraSteadyOffsetX();
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
#define DOWNWELL_VERIFY_PART 30
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
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
                amigaRendererSetGameOrigin(&renderer, std::uint16_t(origin) % downwellGameRingHeight); // 0..603
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
#ifdef DOWNWELL_FRAME_WORK_CANDIDATE
            if(port.shops().allocatedCount)
#endif
            for (unsigned i = 0; i < downwell::ShopWorld::capacity; ++i) {
                const auto& shop = port.shops().instances[i];
                if (!shop.allocated || !oldShopArt.changed(i,shop)) continue;
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
                struct Cell { int top, bottom, left, right; };
                Cell cells[AmigaPortRuntime::destroyedCapacity];
                unsigned cellCount = 0;
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
#ifdef DOWNWELL_REFERENCE_CELL_REPAIR
                    cells[0] = {top, bottom, left, right}; cellCount = 1;
#else
                    // In top order, for the merge below.
                    unsigned at = cellCount++;
                    for (; at && cells[at - 1].top > top; --at) cells[at] = cells[at - 1];
                    cells[at] = {top, bottom, left, right};
                    continue;
                }
                {
#endif
                    // Each repaint has a fixed cost per span of rows (the
                    // indexed wall batch, decorations, a full-width store
                    // into the raster), so cells whose rows overlap or touch
                    // share one repaint of their bounding rectangle. The
                    // painter's output is the backing for any terrain pixel,
                    // so cells inside that rectangle that did not change are
                    // repainted as they were.
                    const auto repaint = [&](const Cell& cell) {
                        amigaRendererSetGameOrigin(&renderer, port.ringOrigin());
                        amigaRendererSetClipRect(&renderer, cell.left, cell.top,
                            cell.right - cell.left, cell.bottom - cell.top);
                        amigaRendererFillRect(&renderer, cell.left, cell.top,
                            cell.right - cell.left, cell.bottom - cell.top, 0);
                        drawMainWorldSpan(&renderer, rooms, mainRoom, builders,
                            fragments, liveBuilder.fragmentCount(),
                            generatedDecorations, liveBuilder.decorationCount(),
                            &port, worldViewX, worldViewY, cell.top, cell.bottom - cell.top,
                            cell.left, cell.right - cell.left, true, 3);
                    };
                    for (unsigned first = 0; first < cellCount;) {
                        Cell span = cells[first];
                        unsigned next = first + 1;
                        for (; next < cellCount && cells[next].top <= span.bottom; ++next) {
                            if (cells[next].bottom > span.bottom) span.bottom = cells[next].bottom;
                            if (cells[next].left < span.left) span.left = cells[next].left;
                            if (cells[next].right > span.right) span.right = cells[next].right;
                        }
#if defined(DOWNWELL_CELL_REPAIR_VERIFY) && !defined(DOWNWELL_REFERENCE_CELL_REPAIR)
                        // The span's rows after one merged repaint must equal
                        // the per-cell reference applied to the same rows.
                        // Live input, so the world-one route can drive it.
                        static UBYTE before[downwellLayerBytes], merged[downwellLayerBytes];
                        const auto rows = [&](UBYTE* buffer, bool save) {
                            AmigaHardware::blitterDrain();
                            for (int y = span.top; y < span.bottom; ++y)
                                for (unsigned plane = 0; plane < 2; ++plane) {
                                    UBYTE* chip = renderer.game + plane * downwellGameRingPlaneBytes +
                                        ((port.ringOrigin() + unsigned(y)) % downwellGameRingHeight) *
                                        downwellBytesPerPlaneLine;
                                    UBYTE* copy = buffer + (unsigned(y - span.top) * 2 + plane) *
                                        downwellBytesPerPlaneLine;
                                    if (save) std::memcpy(copy, chip, downwellBytesPerPlaneLine);
                                    else std::memcpy(chip, copy, downwellBytesPerPlaneLine);
                                }
                        };
                        rows(before, true);
                        repaint(span);
                        rows(merged, true);
                        rows(before, false);
                        for (unsigned cell = first; cell < next; ++cell) repaint(cells[cell]);
                        AmigaHardware::blitterDrain();
                        bool same = true;
                        for (int y = span.top; same && y < span.bottom; ++y)
                            for (unsigned plane = 0; plane < 2; ++plane) {
                                const UBYTE* chip = renderer.game + plane * downwellGameRingPlaneBytes +
                                    ((port.ringOrigin() + unsigned(y)) % downwellGameRingHeight) *
                                    downwellBytesPerPlaneLine;
                                const UBYTE* copy = merged + (unsigned(y - span.top) * 2 + plane) *
                                    downwellBytesPerPlaneLine;
                                for (unsigned byte = 0; byte < downwellBytesPerPlaneLine; ++byte)
                                    if (chip[byte] != copy[byte]) same = false;
                            }
                        if (!same) ++gDownwellCellRepairFailures;
                        ++gDownwellCellRepairChecks;
                        if (next - first > 1) ++gDownwellCellRepairMerges;
#else
                        repaint(span);
#endif
                        first = next;
                    }
                }
            }
#ifdef DOWNWELL_PREPARED_PATTERN_ROWS
            // Only presentation is advanced: geometry and RNG stay at their
            // source trigger. The builder is 128 pixels ahead of the view.
            // New generation owns its update; use the intervening update to
            // fill the hidden ring margin, rather than waiting for exposure.
            if(!cameraXChanged&&missingHeight==0&&destroyedBlockCount==0&&
               liveGeneration.dirtyFirst>=liveGeneration.dirtyEnd&&terrainRaster.ready){
                const auto ahead=gameplayScene.prepareBacking(worldViewY,
                    downwellDisplayHeight,downwellGameRingHeight,16);
                bool ready=ahead.count>0&&ahead.first>=0&&ahead.first+ahead.count<=terrainRasterRows;
                for(int y=ahead.first;ready&&y<ahead.first+ahead.count;++y)
                    ready=!terrainRaster.dirty(unsigned((worldViewX-mainViewX+160)/160),y);
                if(ready){
                    if(backingCopyPending)AmigaHardware::blitterDrain();
                    int origin=int(port.ringOrigin())+ahead.first-worldViewY;
                    while(origin<0)origin+=downwellGameRingHeight;
                    amigaRendererSetGameOrigin(&renderer,origin%downwellGameRingHeight);
#ifdef DOWNWELL_SCENE_VERIFY
                    drawMainWorldSpan(&renderer,rooms,mainRoom,builders,fragments,liveBuilder.fragmentCount(),
                        generatedDecorations,liveBuilder.decorationCount(),&port,worldViewX,ahead.first,0,ahead.count);
#else
                    dirtyGameRows(renderer.gameOrigin,0,ahead.count);
                    transferTerrainRows(&renderer,worldViewX,ahead.first,0,ahead.count,false);
#endif
                    if(terrainPaintSucceeded&&!gameplayScene.preparedBacking(ahead,downwellGameRingHeight))
                        terrainPaintSucceeded=false;
                }
            }
#endif
            // Prepare a possible side-room view a small strip at a time in
            // retired garden memory. The actual camera jump then needs only
            // Chip-to-Chip DMA, not an entire Fast-to-Chip raster transfer.
            if (!cameraXChanged && missingHeight == 0 && destroyedBlockCount == 0 && terrainRaster.ready) {
                const int playerX = port.player().offsetX();
                const int cameraX = port.cameraSteadyOffsetX();
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
                        const unsigned column = std::uint16_t(targetX - mainViewX + 160) / 160; // wanted is -160, 0 or 160
                        AmigaRenderer stage = renderer;
                        stage.game = stage.target = stagedBuffer;
                        int origin = port.ringOrigin() + first - worldViewY;
                        while (origin < 0) origin += downwellGameRingHeight;
                        stage.gameOrigin = origin % downwellGameRingHeight;
                        const int prepared = terrainRaster.prepareStage(&stage, column, first, count,
                            gameplayRows, [&](unsigned top, unsigned height) -> AmigaTerrainRaster::StagePaint {
#ifdef DOWNWELL_SCENE_VERIFY
                                gDownwellColdStageRows += height;
#endif
                                downwellProfileTerrainSpan(targetX, first + top, height,
                                    downwellPlayfieldX, downwellPlayfieldWidth, 1, 4);
#ifdef DOWNWELL_TERRAIN_DIRECT
                                if(AmigaTerrainPainter::paintCached(terrainRaster.writeRows(column,first+top,height),
                                    &stage,rooms,mainRoom,terrainCollision.index,terrainTiles,
                                    generatedDecorations,liveBuilder.decorationCount(),&port,
                                    activeGamePack,activeGamePackEnd,targetX,first,top,height))
                                    return AmigaTerrainRaster::stageInCache;
#endif
                                amigaRendererClearGameRows(&stage, top, height, 0);
                                if(!AmigaTerrainPainter::paint(&stage, rooms, mainRoom, terrainCollision.index,
                                    terrainTiles, generatedDecorations, liveBuilder.decorationCount(), &port,
                                    activeGamePack, activeGamePackEnd, targetX, first,
                                    downwellPlayfieldX, top, downwellPlayfieldWidth, height)){
                                    terrainPaintSucceeded=false;
                                    return AmigaTerrainRaster::stageFailed;
                                }
                                AmigaHardware::blitterDrain();
                                return AmigaTerrainRaster::stageInBacking;
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
#define DOWNWELL_VERIFY_PART 31
#include "DownwellAmigaEntryWipeVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
            static_assert(gameplayPlayerStartY == AmigaGameplayDrawList::playerStartY &&
                          gameplayPlayerGroundY == AmigaGameplayDrawList::replayGroundY,
                          "Actor and gameplay camera anchors must agree");
            if (!actorList.collect(port, activeGamePack, activeGamePackEnd, worldViewX, worldViewY, compositeCurrency
#ifndef DOWNWELL_REFERENCE_ACTOR_PREFIX
                , &actorFrames
#endif
            )) {
                gDownwellActorFailureSprite = actorList.failedSprite;
                gDownwellActorFailureFrame = actorList.failedFrame;
                frameReady = false;
            }
            using Real = downwell::MotionBinary64;
            bool hardwareEntryCover=false;
            // Immutable cover DMA cannot reproduce shade-subtracted alpha.
            if(!actorList.entryCoverHardwareEligible(port,worldViewX,worldViewY,hardwareEntryCover)){
                exitCode=downwellGameplayFailed(__LINE__);break;
            }
            bool hardwareCurrency=false;
#if !defined(DOWNWELL_REFERENCE_EFFECT_COMPOSITION) && !defined(DOWNWELL_BOB_VERIFY) && !defined(DOWNWELL_REPLAY_VERIFY) && !defined(DOWNWELL_REFERENCE_ACTOR_PREFIX)
            if(compositeCurrency&&port.gemSlotEnd()){
                DW_PROFILE_SCOPE(profileControllerComposition);
                hardwareCurrency=actorList.promoteCurrency(port,worldViewX,worldViewY,effectAssets,actorFrames);
            }
#endif
            const bool compositeEffects=actorList.hasControllerProducers(port)&&((compositeCurrency&&(!hardwareCurrency||actorList.currencyRasterRequired||
                port.smokeEffects().size()||port.compactDebris().size()||port.compactCorpses().size()||
                port.compactSurfaceUpgrades()||
                (port.registeredSmoke()&&port.registeredSmoke()->size())||
                (port.registeredCorpses()&&port.registeredCorpses()->size())||
                (port.timeShardInstances()&&port.timeShardInstances()->size())))||
                (actorList.entryCoverVisible&&!hardwareEntryCover)||
                (port.levelNumbers()&&port.levelNumbers()->size()));
            if(compositeEffects){
                DW_PROFILE_SCOPE(profileControllerComposition);
                const downwell::EffectSurfacePlacement::View effectView{Real::fromInteger(worldViewX),Real::fromInteger(worldViewY),
                    Real::fromInteger(160),Real::fromInteger(284)};
                downwell::CurrencySmokeComposition::Settings settings;
                bool shadesSucceeded=true;
                settings.noControl=port.shops().noControl;settings.death=!compositeCurrency;
                if(!controllerEffects.prepareWithSurfaceProducers([&](auto emit){
                    for(unsigned i=port.gemSlotEnd();i>(hardwareCurrency?actorList.currencyRasterFirst:0);--i){
                        const auto& gem=port.gemAt(i-1);
                        if(gem.active&&gem.motion.lifetime.visible())
                            emit(gem.large,gem.motion.animation.frame(),std::int32_t(240+gem.x),std::int32_t(176+gem.y));
                    }
                },port.smokeEffects(),effectAssets,effectView,settings,[&](auto emit){
                    return port.visitRegisteredEffectSurfaces(effectAssets,effectView,emit);
                },[&](auto emit){
                    auto drawCover=[&](){
                        if(!actorList.entryCoverVisible||hardwareEntryCover)return true;
                        downwell::EffectMaskAssets::Asset mask;
                        return effectAssets.entryDither(80,actorList.entryCover.y,mask)&&emit(mask,effectView.x,effectView.y);
                    };
                    // The collected singleton cover needs no second room walk
                    // unless a title participates in this surface pass.
                    if(!port.levelNumbers()||!port.levelNumbers()->size())return drawCover();
                    return port.visitUnconditionalSurfaceDraw([&](auto kind,auto key,auto){
                        if(kind==GameplayWorld::DrawActor::levelNumber)
                            return actorList.drawLevelNumber(port,key,effectView,emit);
                        return drawCover();
                    });
                },[&](auto emit){
                    for(unsigned i=0;i<port.enemyCount();++i){
                        const auto& enemy=port.enemyAt(i);
                        if(enemy.active&&enemy.awake)
                            emit(std::int32_t(240+enemy.x),std::int32_t(176+enemy.y));
                    }
                    shadesSucceeded=port.visitRegisteredEnemyShades([&](Real x,Real y){emit(x,y);return true;});
                },[](Real n,std::int32_t& out){return n.roundPixel(out);})||!shadesSucceeded){
                    exitCode=downwellGameplayFailed(__LINE__);break;
                }
            }
            if(compositeEffects&&!AmigaEffectSurface::verify(controllerEffects.surface())){exitCode=downwellGameplayFailed(__LINE__);break;}
            GameplayBob* actors = actorList.actors;
            const unsigned actorCount = actorList.count;
            const auto& rayLine = actorList.rayLine;
            const bool rayVisible = actorList.rayVisible;
            static_assert(gameplayRegularSpriteCount == AmigaGameplayActors::regularChannels,
                          "Gameplay Copper and sprite slots must agree");
            DW_PROFILE_PHASE(profileActorChannels);
            const auto assignment = AmigaGameplayActors::assign(actors, actorCount,
                activeGamePack, activeGamePackEnd, playerSprites[back],
                bulletSprites + back * gameplayRegularSpriteCount, enemySprites[back],
                enemySpriteHeight,
#ifdef DOWNWELL_BOB_VERIFY
                0
#else
                8
#endif
                , compositeEffects && controllerEffects.visible() ? &controllerEffects.surface() : nullptr,
                hardwareCurrency?actorList.currencySpriteCount:0, -port.cameraShakeX()
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
                        bool bounded;
                        if(actor.sprite==gameplayBlackRectMarker||actor.sprite==gameplayWhiteRectMarker){
                            x=actor.x;y=actor.y;w=actor.frame;h=actor.flip;bounded=true;
                        }else bounded=amigaRendererAssetBounds(activeGamePack, activeGamePackEnd,
                            actor.sprite, actor.frame, actor.x, actor.y, actor.flip, &x,&y,&w,&h);
                        if (bounded &&
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
                        [&](unsigned row) { return gameplayRows.dirty(row, back); }
#ifdef DOWNWELL_PREPARED_FIELDS
                        // Actors/effects mark rows for the sync below. Laser
                        // prepare has retired its pixels when the field changed;
                        // a pending ray will draw after the new field.
                        , !gameplayRays[back].retainsPixels()
#endif
                        )) {
                    exitCode=downwellGameplayFailed(__LINE__); break;
                }
            }
            syncGameRows(gameDisplay[back], gameBitmap, back);
            if (fieldShape.visible) {
                DW_PROFILE_SCOPE(profileGoalFieldDraw);
                AmigaHardware::blitterDrain();
#ifdef DOWNWELL_PREPARED_FIELDS
                if(!gameplayFields[back].draw()){exitCode=downwellGameplayFailed(__LINE__);break;}
#else
                gameplayFields[back].draw();
#endif
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
                    enemySprites[back], palette, port.ringOrigin(), -port.cameraShakeX()))
                frameReady = false;
            if(!terrainPaintSucceeded){exitCode=downwellGameplayFailed(__LINE__);break;}
            if (!frameReady) {
                downwellGameplayRenderFailure();
                break;
            }
            const UBYTE* exitMask = nullptr;
            if(actorList.entryCoverVisible&&port.exitDither().active){exitCode=downwellGameplayFailed(__LINE__);break;}
            if(hardwareEntryCover){
                // Two different cover lifetimes cannot share one DMA bank.
                if(port.exitDither().active||!selectEntryMaskBank()||
                   !gameplayScene.entryMask(back,overlayAssetStorage,AmigaEntryMask::bytes,actorList.entryCover.y)){
                    exitCode=downwellGameplayFailed(__LINE__);break;
                }
            }else{
                if(port.exitDither().active){
                    if(!selectExitMaskBank()){exitCode=downwellGameplayFailed(__LINE__);break;}
                    exitMask=overlayAssetStorage+AmigaExitMask::offset(
                        port.exitDither().y-(mainInitialViewY+port.cameraOffsetY()));
                }
                if(!gameplayScene.mask(back,exitMask)){exitCode=downwellGameplayFailed(__LINE__);break;}
            }
#ifdef DOWNWELL_ENTRY_WIPE_VERIFY
#define DOWNWELL_VERIFY_PART 32
#include "DownwellAmigaEntryWipeVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
            /* AmigaPlatform makes the published list live at next VBlank. */
#ifdef DOWNWELL_PREPARED_PATTERN_ROWS
            // Required rendering is finished. Spend only this update's actual
            // remaining work budget on future raster rows, before publication.
            // No Chip/display writes, simulation or RNG advances occur here.
            if(!cameraXChanged&&!destroyedBlockCount&&worldViewX==mainViewX&&
               liveGeneration.dirtyFirst>=liveGeneration.dirtyEnd&&
               terrainRaster.ready&&terrainCollision.index){
                const int first=worldViewY+downwellDisplayHeight;
                int end=first+128<terrainRasterRows-8?first+128:terrainRasterRows-8;
                if(end>liveBuilder.frontierY()-24)end=liveBuilder.frontierY()-24;
                for(unsigned piece=0;piece<4&&end>first&&frameBudget.canPrepare(frameWorkStamp());++piece){
                    const unsigned y=terrainRaster.nextPreparation(first,end);
                    if(y>=unsigned(end))break;
                    if(terrainRaster.needsPatternMaterialization(y)){
                        const unsigned budget=unsigned(end)-y<8?unsigned(end)-y:8;
                        if(!terrainRaster.materializePatternRows(y,budget)){terrainPaintSucceeded=false;break;}
                        continue;
                    }
                    if(terrainRaster.preparationFallback(y)){
                        const int painted=terrainRaster.prepareFallback(y,end,
                            [&](UWORD* pixels,unsigned top,unsigned height){
                                return AmigaTerrainPainter::paintCached(pixels,&renderer,rooms,mainRoom,
                                    terrainCollision.index,terrainTiles,generatedDecorations,
                                    liveBuilder.decorationCount(),&port,activeGamePack,activeGamePackEnd,
                                    worldViewX,top,0,height);
                            });
                        if(painted<=0){terrainRaster.skipPreparation(((y+8)&~15)+8);break;}
                        continue;
                    }
                    if(!AmigaTerrainPainter::preparePatternPiece(terrainRaster,*terrainCollision.index,
                        terrainTiles,generatedDecorations,liveBuilder.decorationCount(),&port,worldViewX,y)){
                        terrainRaster.deferPreparationFallback(y);break;
                    }
                }
            }
#endif
            DW_PROFILE_PHASE(profilePublish);
            if(!encounterFrame.publishable()){exitCode=downwellGameplayFailed(__LINE__);break;}
#ifdef DOWNWELL_ENCOUNTER_UPDATE_FAILURE_VERIFY
            ++gDownwellEncounterUpdatePublications;
#endif
            platform.publish(gameplayScene.copper(back));
            overlapNextStep = true;
#ifdef DOWNWELL_FRAME_DUMP
            // The last BOB/effect blit may still be finishing when the CPU
            // publishes. Debugger snapshots must include its completed planes.
            AmigaHardware::blitterDrain();
            gDownwellDumpGame=(const UBYTE*)gameDisplay[back].data;
            gDownwellDumpHud=memory.hud();
            gDownwellDumpPlayerSprite=memory.playerSprite(back);
            gDownwellDumpActorSprites=memory.bulletSprite()+back*gameplayRegularSpriteCount*(bulletSpriteBytes/sizeof(UWORD));
            gDownwellDumpOverflowSprite=memory.enemySprite(back);
            gDownwellDumpRingOrigin=(UWORD)port.ringOrigin();
            gDownwellDumpTick=port.tick();
            gDownwellDumpBobs=actors;gDownwellDumpBobCount=bobCount;
            gDownwellDumpPack=activeGamePack;gDownwellDumpPackBytes=activeGamePackEnd-activeGamePack;
            downwellFrameDump();
#endif
#ifdef DOWNWELL_ENTRY_WIPE_VERIFY
#define DOWNWELL_VERIFY_PART 33
#include "DownwellAmigaEntryWipeVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
#ifdef DOWNWELL_NATURAL_VERIFY
            if (naturalRoute.done) { platform.waitVblank(); downwellNaturalReady(); }
#endif
#ifdef DOWNWELL_EXIT_WIPE_VERIFY
#define DOWNWELL_VERIFY_PART 34
#include "DownwellAmigaExitWipeVerify.inc"
#undef DOWNWELL_VERIFY_PART
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
#define DOWNWELL_VERIFY_PART 35
#include "DownwellAmigaSceneVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
        }
        if(!encounterFrame.finish()){exitCode=downwellGameplayFailed(__LINE__);break;}
        lastPortCheckpoint = port.trajectoryDigest();
#ifdef DOWNWELL_REPLAY_VERIFY
#define DOWNWELL_VERIFY_PART 36
#include "DownwellAmigaReplayVerify.inc"
#undef DOWNWELL_VERIFY_PART
#endif
    }

    if(!encounterSession.retire())exitCode=downwellGameplayFailed(__LINE__);
#ifdef DOWNWELL_SCENE_COLLISION_BACKEND
    encounterResources.releaseScene();
#endif
    keyboard.release();
    soundBanks.release();
    platform.restore();
    memory.release();
#ifdef DOWNWELL_CLEAR_VERIFY
    gDownwellClearPhase = 100 + exitCode; downwellClearProbe();
#endif
    return exitCode;
}

#ifdef DOWNWELL_MULTIPLY_BINARY64_HOOK
extern "C" void install_cpu_init(void);
#endif
int main(void){
    const ULONG lower=(ULONG)SysBase->ThisTask->tc_SPLower;
    const ULONG upper=(ULONG)SysBase->ThisTask->tc_SPUpper;
    // AmigaStart.c gives main 8192 bytes of stack on Kickstart 2.0 or later.
    if(upper<lower||upper-lower<8192){
        downwellRefuse("Downwell needs Kickstart 2.0 or later.");
        return 20;
    }
#ifdef DOWNWELL_MULTIPLY_BINARY64_HOOK
    install_cpu_init(); // the wide multiply only on a 68020 or 68030
#endif
    const int result=runDownwell();
    // WI.6: a failure that has not said why yet says so now, with the display
    // given back: an allocation at startup, else the internal failure's code.
    if(result&&!gDownwellRefusal){
        if(gDownwellStartupFailure==2||gDownwellStartupFailure==4)downwellRefuseMemory();
        else{
            char line[64];
            char* out=appendText(line,"Downwell stopped after an internal error (");
            out=appendNumber(out,gDownwellStartupFailure?gDownwellStartupFailure:gDownwellGameplayFailLine);
            out=appendText(out,").");
            *out=0;
            downwellRefuse(line);
        }
    }
    return result;
}
