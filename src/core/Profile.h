#ifndef DOWNWELL_CORE_PROFILE_H
#define DOWNWELL_CORE_PROFILE_H

// Hardware-free instrumentation interface. Backends provide timing and
// counters when enabled; ordinary builds compile these hooks away.

enum DownwellProfilePhase {
    profileOther, profileWait, profileInput, profileSimulation,
    profileTitle, profileSprite, profileScroll, profileCopper,
    profilePublish, profileStyleCopy, profileStyleActors, profileStyleText,
    profileStyleDrain, profileStyleMirror, profileTerrainCollision, profileGameCopy,
    profilePlayer, profileBullets, profileEnemyAlarms, profileEnemyMotion,
    profileEnemyContact, profileGems, profileTerrainWalls, profileTerrainTiles,
    profileTerrainDecorations, profileTerrainAsset, profileWallFrame,
    profileAmmoBase, profileAmmoRows, profileAmmoDividers, profileAmmoLabel,
    profileCamera, profileGeneration, profileLifecycle, profileHudState,
    profileGeneratedRegistration, profileGeneratedIndex,
    profileAmmoDamage, profileAmmoFillGeometry, profileAmmoWordPublish,
    profilePlayerMotion, profilePlayerShops,
    profileActorCollect, profileActorChannels, profileHardwareSpriteUpdate,
    profileLaserTrace, profileLaserPrepare, profileLaserDraw,
    profileLaserDrawDrain, profileGameplayBobs, profileLaserRestore, profileLaserPrepareDrain,
    profileClearEntry, profileClearStep, profileClearBank, profileClearDrawList,
    profileClearBorder, profileClearOverlay, profileClearSprites, profileClearCopper,
    profileClearDrain, profileClearPublish,
    profileClearBorderPaint, profileClearOverlayPaint, profileClearTrig,
    profileGoalFieldPrepare, profileGoalFieldDraw,
    profileBuilderCommand, profileBuilderInitialize, profileGeneratedCell,
    profileGeneratedLanding, profileGeneratedTiles, profileGeneratedBreakable,
    profileGeneratedEnemy, profileGeneratedShop,
    profileContactDirection, profileDamageNumberCreate, profileDamageNumberStep,
    profileDamageNumberDraw,
    profileControllerComposition,
    profileActorLayerWalk, profileActorPreparation,
    profileInstanceBirth, profileInstanceBounds, profileInstanceBoundsTrig,
    profileInstanceLayerAttach, profileEffectBirth,
    profileBoundsGeometry, profileBoundsPublication,
    profileRoomAnimation, profileStandaloneEffectAnimation,
    profileAnimationDispatch,
    profileSpatialInsert, profileSpatialRemove, profileSpatialPartition, profileSpatialChoose,
    profileDrawCleanup,
    profileBirthRegistration, profileBirthMetadata, profileBirthRoomOrder,
    profileBulletEnemyQuery, profileBulletSolidQuery, profileBulletPublication,
    profileProjectileCoordinate,
    profileSpatialPartitionFallback, profileSpatialChooseFallback,
    profileAlarmCapture, profileAlarmSort, profileAlarmScan, profileAlarmCallback,
    profileEnemyWalker, profileEnemyBat, profileEnemyFrog, profileEnemySnail,
    profileEnemyCrawler, profileEnemyFinalize,
    profileBindingDefinition, profileBindingMembership, profileBindingProperties,
    profileLayerAdopt, profileLayerResolve, profileLayerAttach, profileLayerAllocate,
    profileObjectPopulation,
    profileTerrainRecord, profileTerrainCreate,
    profileEffectClear, profileEffectCurrency, profileEffectSmoke,
    profileEffectDebris, profileEffectCover, profileEffectShade,
    profileEffectVisibility, profileEffectPublish,
    profileEffectPlacement, profileEffectRaster, profileEffectStamp,
    profileEnemyPositionPublish, profileEnemyPresentationPublish,
    profileEntryFields, profileEntryWorld, profileEntryAssets,
    profileEntryRaster, profileEntryBacking,
    profilePhaseCount
};

#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
void downwellProfileVblank();
void downwellProfileBegin(unsigned scene);
void downwellProfileEnd();
void downwellProfilePhase(unsigned phase);
void downwellProfileGameplay(unsigned tick,long playerYQ8,long viewY,int health,unsigned enemySlots);
extern "C" volatile long gDownwellProfileScroll;
extern "C" volatile long gDownwellProfileCameraChange;
extern "C" volatile long gDownwellProfileStagedHits;
extern "C" volatile long gDownwellProfileStageShortfall;
extern "C" volatile unsigned long gDownwellProfileSection;
extern "C" volatile unsigned long gDownwellProfileDamageBirths;
#define DW_PROFILE_DAMAGE_BIRTH() (++gDownwellProfileDamageBirths)
class DownwellProfileFrame {
public:
    explicit DownwellProfileFrame(unsigned scene) { downwellProfileBegin(scene); }
    ~DownwellProfileFrame() { downwellProfileEnd(); }
};
#define DW_PROFILE_FRAME(scene) DownwellProfileFrame profileFrame(scene)
#ifdef DOWNWELL_PROBES
unsigned downwellProfileEnter(unsigned phase);
class DownwellProfileScope {
    unsigned previous_;
public:
    explicit DownwellProfileScope(unsigned phase) : previous_(downwellProfileEnter(phase)) {}
    ~DownwellProfileScope() { downwellProfilePhase(previous_); }
};
#define DW_PROFILE_SCOPE(phase) DownwellProfileScope profileScope(phase)
#define DW_PROFILE_PHASE(phase) downwellProfilePhase(phase)
#else
#if defined(DOWNWELL_PHASE_SAMPLING)
// Word-sized phase publication is atomic on 68000. The VBlank sampler reads
// this marker; no clock read or out-of-line call is needed at scope boundaries.
extern "C" volatile unsigned short gDownwellSamplePhase;
class DownwellSampleScope {
    unsigned short previous_;
public:
    explicit DownwellSampleScope(unsigned phase):previous_(gDownwellSamplePhase){
        gDownwellSamplePhase=static_cast<unsigned short>(phase);
    }
    ~DownwellSampleScope(){gDownwellSamplePhase=previous_;}
};
#define DW_PROFILE_SCOPE(phase) DownwellSampleScope profileScope(phase)
#define DW_PROFILE_PHASE(phase) (gDownwellSamplePhase=static_cast<unsigned short>(phase))
#elif defined(DOWNWELL_GENERATION_TIMING)
// Opt-in inclusive timing of one selected stage; every other bracket compiles
// away. The backend ignores nested entries of the same stage to avoid counting
// recursive work twice. Default remains the complete generation transaction.
#ifndef DOWNWELL_GENERATION_TIMING_PHASE
#define DOWNWELL_GENERATION_TIMING_PHASE profileGeneration
#endif
static_assert(DOWNWELL_GENERATION_TIMING_PHASE>=profileGeneration&&
    DOWNWELL_GENERATION_TIMING_PHASE<profilePhaseCount,"Invalid coarse timing stage");
bool downwellProfileGenerationEnter(unsigned long& start);
void downwellProfileGenerationLeave(unsigned long start);
template<unsigned Phase> class DownwellGenerationScope {};
template<> class DownwellGenerationScope<DOWNWELL_GENERATION_TIMING_PHASE> {
    unsigned long start_;
    bool active_;
public:
    DownwellGenerationScope():active_(downwellProfileGenerationEnter(start_)){}
    ~DownwellGenerationScope(){if(active_)downwellProfileGenerationLeave(start_);}
};
#define DW_PROFILE_SCOPE(phase) [[maybe_unused]] DownwellGenerationScope<phase> profileScope
#else
#define DW_PROFILE_SCOPE(phase) ((void)0)
#endif
#if !defined(DOWNWELL_PHASE_SAMPLING)
#define DW_PROFILE_PHASE(phase) ((void)0)
#endif
#endif
#else
#define DW_PROFILE_SCOPE(phase) ((void)0)
#define DW_PROFILE_FRAME(scene) ((void)0)
#define DW_PROFILE_DAMAGE_BIRTH() ((void)0)
#define DW_PROFILE_PHASE(phase) ((void)0)
#endif

#include "ProfileCounters.h"

// Startup-only measurement, before hardware takeover. The backend supplies an
// OS-owned field counter; no clock or platform dependency enters shared logic.
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
unsigned long downwellTerrainPreparationBegin();
void downwellTerrainPreparationEnd(unsigned long start);
class DownwellTerrainPreparationScope {
    unsigned long start_;
public:
    DownwellTerrainPreparationScope():start_(downwellTerrainPreparationBegin()){}
    ~DownwellTerrainPreparationScope(){downwellTerrainPreparationEnd(start_);}
};
#define DW_TERRAIN_PREPARATION_SCOPE DownwellTerrainPreparationScope terrainPreparationScope
#else
#define DW_TERRAIN_PREPARATION_SCOPE ((void)0)
#endif

// Inner generation brackets are deliberately opt-in: hundreds of phase
// switches per burst can cost more than the operations being measured.
#if defined(DOWNWELL_PROBES) && defined(DOWNWELL_GENERATION_DETAIL)
#define DW_GENERATION_SCOPE(phase) DW_PROFILE_SCOPE(phase)
#define DW_GENERATION_PHASE(phase) DW_PROFILE_PHASE(phase)
#else
#define DW_GENERATION_SCOPE(phase) ((void)0)
#define DW_GENERATION_PHASE(phase) ((void)0)
#endif

#endif
