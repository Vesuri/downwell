#ifndef DOWNWELL_CORE_GAMEPLAY_LIMITS_H
#define DOWNWELL_CORE_GAMEPLAY_LIMITS_H

namespace downwell {
// Simulation storage must not depend on the target's hardware sprite count.
constexpr unsigned gameplayBulletCapacity = 16;
// The compact world's seven guns: a gem-high SHOTGUN shot alone is sixteen
// bulletRanged pellets, alive for about two dozen Steps.
constexpr unsigned gameplayCompactBulletCapacity = 32;
constexpr unsigned gameplayGemCapacity = 64;
// Background cells repainted after one update: a box per bullet, the
// player's head break, and scenery whose builder art changed.
constexpr unsigned gameplaySceneryRepaintCapacity = 32;
constexpr unsigned gameplayRepaintCapacity = gameplayCompactBulletCapacity + 1 + gameplaySceneryRepaintCapacity;
// Compact scrEffectSpawn (219) and emitMovingFx (220) records per room. A
// gem-high SHOTGUN burst alone spawns one 602 per pellet (sixteen).
constexpr unsigned gameplayCompactNoLoopCapacity = 32, gameplayCompactMovingCapacity = 48;
// Compact object-370 debris (two per box, scenery bursts).
constexpr unsigned gameplayCompactDebrisCapacity = 24;
// Compact objDeadBody 263 (W1.6): world-one enemy corpses live 180 updates
// unless they leave the view first.
constexpr unsigned gameplayCompactCorpseCapacity = 16;
// Compact fxTimeShard 238: shardLimit 20 plus one Begin Step's two bursts.
constexpr unsigned gameplayCompactShardCapacity = 32;
// Beyond rmMain's serialized 300000-pixel height, but safe when multiplied
// by Q8 on a 32-bit 68000. Absence of terrain must not create a local floor.
constexpr int gameplayNoLanding = 1000000;
}

#endif
