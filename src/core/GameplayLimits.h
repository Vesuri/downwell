#ifndef DOWNWELL_CORE_GAMEPLAY_LIMITS_H
#define DOWNWELL_CORE_GAMEPLAY_LIMITS_H

namespace downwell {
// Simulation storage must not depend on the target's hardware sprite count.
constexpr unsigned gameplayBulletCapacity = 16;
constexpr unsigned gameplayGemCapacity = 64;
// Beyond rmMain's serialized 300000-pixel height, but safe when multiplied
// by Q8 on a 32-bit 68000. Absence of terrain must not create a local floor.
constexpr int gameplayNoLanding = 1000000;
}

#endif
