#ifndef DOWNWELL_TERRAIN_READ_CENSUS_H
#define DOWNWELL_TERRAIN_READ_CENSUS_H
// Diagnostic API-read census, never a cycle measurement. Queries may nest;
// categories are not disjoint and must not be summed as memory accesses.
enum TerrainReadCounter {
    terrainIdentityRead, terrainPublicationRead, terrainRoomPresence,
    terrainRoomActive, terrainRoomAnimationSkip, terrainRoomReconcile,
    terrainRoomCleanup, terrainLayerRead, terrainLayerCutoffRead,
    terrainFlagsRead, terrainSpriteRead, terrainWallStateRead,
    terrainThinWindowRead, terrainRegistrationRead, terrainTableRead,
    terrainPreparedSegments, terrainPreparedCells, terrainCensusCount
};
#ifdef DOWNWELL_TERRAIN_READ_CENSUS
extern "C" {
extern volatile unsigned long gDownwellTerrainReads[2][terrainCensusCount];
extern volatile unsigned long gDownwellTerrainReadSamples[165][2][terrainCensusCount];
extern volatile unsigned long gDownwellTerrainReadFrames;
extern bool gDownwellTerrainReadActive;
extern unsigned gDownwellTerrainConstructionDepth;
}
inline void terrainReadCensus(TerrainReadCounter counter,unsigned amount=1){
    if(gDownwellTerrainReadActive)
        gDownwellTerrainReads[gDownwellTerrainConstructionDepth?1:0][counter]+=amount;
}
struct TerrainConstructionCensusScope {
    TerrainConstructionCensusScope(){++gDownwellTerrainConstructionDepth;}
    ~TerrainConstructionCensusScope(){--gDownwellTerrainConstructionDepth;}
};
#define DW_TERRAIN_READ(counter) terrainReadCensus(counter)
#define DW_TERRAIN_READ_ADD(counter,n) terrainReadCensus(counter,n)
#define DW_TERRAIN_OBSERVE(counter,expression) (terrainReadCensus(counter),(expression))
#define DW_TERRAIN_SLOT_READ(counter,s,first,last) do { if((s)>=(first)&&(s)<(last)) terrainReadCensus(counter); } while(0)
#define DW_TERRAIN_CONSTRUCTION_SCOPE TerrainConstructionCensusScope terrainConstructionCensusScope
#else
#define DW_TERRAIN_READ(counter) ((void)0)
#define DW_TERRAIN_READ_ADD(counter,n) ((void)0)
#define DW_TERRAIN_OBSERVE(counter,expression) (expression)
#define DW_TERRAIN_SLOT_READ(counter,s,first,last) ((void)0)
#define DW_TERRAIN_CONSTRUCTION_SCOPE ((void)0)
#endif
#endif
