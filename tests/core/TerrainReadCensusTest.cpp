#include "core/TerrainReadCensus.h"
#include "core/TerrainOwnershipRuns.h"
#include <cassert>
#ifdef DOWNWELL_TERRAIN_READ_CENSUS
extern "C" {
volatile unsigned long gDownwellTerrainReads[2][terrainCensusCount]{};
volatile unsigned long gDownwellTerrainReadSamples[165][2][terrainCensusCount]{};
volatile unsigned long gDownwellTerrainReadFrames=0;
bool gDownwellTerrainReadActive=false;
unsigned gDownwellTerrainConstructionDepth=0;
}
#endif
int main(){
    downwell::TerrainOwnershipRuns<8> runs;
    assert(runs.append({0,1},60,0,32));
    int evaluated=0;
    assert(DW_TERRAIN_OBSERVE(terrainThinWindowRead,++evaluated)==1);
    assert(evaluated==1);
#ifdef DOWNWELL_TERRAIN_READ_CENSUS
    assert(gDownwellTerrainReads[0][terrainIdentityRead]==0);
    gDownwellTerrainReadActive=true;
    assert(runs.contains({0,1}));
    assert(gDownwellTerrainReads[0][terrainIdentityRead]==1);
    {
        DW_TERRAIN_CONSTRUCTION_SCOPE;
        assert(runs.contains({0,1}));
        {DW_TERRAIN_CONSTRUCTION_SCOPE;assert(!runs.contains({0,2}));}
        assert(gDownwellTerrainConstructionDepth==1);
    }
    assert(gDownwellTerrainConstructionDepth==0);
    assert(gDownwellTerrainReads[1][terrainIdentityRead]==2);
    assert(gDownwellTerrainReads[0][terrainIdentityRead]==1);
    gDownwellTerrainReadActive=false;
    assert(runs.contains({0,1}));
    assert(gDownwellTerrainReads[0][terrainIdentityRead]==1);
#else
    DW_TERRAIN_READ_ADD(terrainIdentityRead,++evaluated);
    DW_TERRAIN_SLOT_READ(terrainIdentityRead,++evaluated,0,8);
    assert(evaluated==1); // Disabled instrumentation has no evaluation cost.
#endif
}
