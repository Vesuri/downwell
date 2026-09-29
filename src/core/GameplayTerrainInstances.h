#ifndef DOWNWELL_CORE_GAMEPLAY_TERRAIN_INSTANCES_H
#define DOWNWELL_CORE_GAMEPLAY_TERRAIN_INSTANCES_H
#include "GameplayTerrainInstancesScalar.h"
#ifdef DOWNWELL_TERRAIN_RUN_STORE
#include "GameplayTerrainInstancesRuns.h"
#endif
namespace downwell {
#ifdef DOWNWELL_TERRAIN_RUN_STORE
using GameplayTerrainInstances=GameplayTerrainInstancesRuns;
#else
using GameplayTerrainInstances=GameplayTerrainInstancesScalar;
#endif
}
#endif
