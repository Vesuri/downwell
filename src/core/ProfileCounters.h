#ifndef DOWNWELL_CORE_PROFILE_COUNTERS_H
#define DOWNWELL_CORE_PROFILE_COUNTERS_H

/* C-compatible counters shared by the native renderer and C++ runtime. */
enum DownwellProfileCounter {
    counterSolidIndexed, counterSolidFallback, counterLandingIndexed,
    counterLandingFallback, counterBreakableFallback,
    counterSegmentIndexed, counterSegmentCells, counterSegmentFallback,
    counterSpriteHeaderOnly, counterSpritePayloadCopies, counterSpritePayloadWords,
    counterAnimationFullPass, counterAnimationProjectedPass, counterAnimationProjectionBuild,
    counterTerrainSnapshots, counterActorSnapshots,
    counterSnapshotEqualMoves, counterSnapshotChangedMoves,
    counterSpatialFirstPublication, counterSpatialEqualRepublication,
    counterSpatialChangedRepublication,
    counterSpatialBoxQuery, counterSpatialLineQuery,
    counterSpatialHeightRead, counterSpatialNodeCountRead,
    profileCounterCount
};
#ifdef DOWNWELL_PROBES
#ifdef __cplusplus
extern "C" {
#endif
extern volatile unsigned long gDownwellProfileCounters[profileCounterCount];
#ifdef __cplusplus
}
#endif
#define DW_PROFILE_COUNT(counter) (++gDownwellProfileCounters[counter])
#define DW_PROFILE_ADD(counter, amount) (gDownwellProfileCounters[counter] += (amount))
#else
#define DW_PROFILE_COUNT(counter) ((void)0)
#define DW_PROFILE_ADD(counter, amount) ((void)0)
#endif
#endif
