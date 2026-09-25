#ifndef DOWNWELL_AMIGA_PORT_RUNTIME_H
#define DOWNWELL_AMIGA_PORT_RUNTIME_H
#include "core/GameplayWorld.h"
#include "AmigaPlayer.h"

// Compatibility names only. Simulation lives in core, not in this backend.
using AmigaPortRuntime = GameplayWorld;
using AmigaPortFrame = GameplayFrame;
enum {
    amigaPortBulletCapacity = gameplayWorldBulletCapacity,
    amigaPortDestroyedCapacity = gameplayWorldDestroyedCapacity,
    amigaPortEnemyCapacity = gameplayWorldEnemyCapacity,
    amigaPortGemCapacity = gameplayWorldGemCapacity
};
#endif
