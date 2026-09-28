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
    profilePhaseCount
};

#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
void downwellProfileVblank();
void downwellProfileBegin(unsigned scene);
void downwellProfileEnd();
void downwellProfilePhase(unsigned phase);
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
#define DW_PROFILE_SCOPE(phase) ((void)0)
#define DW_PROFILE_PHASE(phase) ((void)0)
#endif
#else
#define DW_PROFILE_SCOPE(phase) ((void)0)
#define DW_PROFILE_FRAME(scene) ((void)0)
#define DW_PROFILE_DAMAGE_BIRTH() ((void)0)
#define DW_PROFILE_PHASE(phase) ((void)0)
#endif

#include "ProfileCounters.h"

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
