#ifndef DOWNWELL_AMIGA_PROFILE_H
#define DOWNWELL_AMIGA_PROFILE_H

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
    profileAmmoDamage, profileAmmoFillGeometry, profileAmmoWordPublish, profilePhaseCount
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
#define DW_PROFILE_PHASE(phase) ((void)0)
#endif

enum DownwellProfileCounter {
    counterSolidIndexed, counterSolidFallback, counterLandingIndexed,
    counterLandingFallback, counterBreakableFallback,
    counterSegmentIndexed, counterSegmentCells, counterSegmentFallback, profileCounterCount
};
#ifdef DOWNWELL_PROBES
extern "C" volatile unsigned long gDownwellProfileCounters[profileCounterCount];
#define DW_PROFILE_COUNT(counter) (++gDownwellProfileCounters[counter])
#else
#define DW_PROFILE_COUNT(counter) ((void)0)
#endif

#endif
