#ifndef DOWNWELL_CORE_GAMEPLAY_TERRAIN_INSTANCES_H
#define DOWNWELL_CORE_GAMEPLAY_TERRAIN_INSTANCES_H
#include "GameplayTerrainInstancesScalar.h"
namespace downwell {
// Registered-terrain reference only. Native gameplay publishes fixed terrain
// directly to its backend and does not instantiate these per-cell records.
using GameplayTerrainInstances=GameplayTerrainInstancesScalar;
}
#endif
