#ifndef DOWNWELL_CORE_GAMEPLAY_LIMITS_H
#define DOWNWELL_CORE_GAMEPLAY_LIMITS_H

namespace downwell {
// Simulation storage must not depend on the target's hardware sprite count.
constexpr unsigned gameplayBulletCapacity = 16;
constexpr unsigned gameplayGemCapacity = 64;
}

#endif
