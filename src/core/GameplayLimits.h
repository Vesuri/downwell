#ifndef DOWNWELL_CORE_GAMEPLAY_LIMITS_H
#define DOWNWELL_CORE_GAMEPLAY_LIMITS_H

namespace downwell {
// Simulation storage must not depend on the target's hardware sprite count.
constexpr unsigned gameplayBulletCapacity = 16;
constexpr unsigned gameplayGemCapacity = 64;
// Background cells repainted after one update: a box per bullet, the
// player's head break, and scenery whose builder art changed.
constexpr unsigned gameplaySceneryRepaintCapacity = 32;
constexpr unsigned gameplayRepaintCapacity = gameplayBulletCapacity + 1 + gameplaySceneryRepaintCapacity;
// Compact scrEffectSpawn (219) and emitMovingFx (220) records per room.
constexpr unsigned gameplayCompactNoLoopCapacity = 16, gameplayCompactMovingCapacity = 48;
// Compact object-370 debris (two per box, scenery bursts).
constexpr unsigned gameplayCompactDebrisCapacity = 24;
// Beyond rmMain's serialized 300000-pixel height, but safe when multiplied
// by Q8 on a 32-bit 68000. Absence of terrain must not create a local floor.
constexpr int gameplayNoLanding = 1000000;
}

#endif
