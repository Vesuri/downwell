#ifndef DOWNWELL_CORE_BUILDER_REWARD_CHOICE_H
#define DOWNWELL_CORE_BUILDER_REWARD_CHOICE_H
#include "GameMakerRandom.h"
namespace downwell {
// scrBuilder VM blocks 141--152. The low-health draw precedes the style
// override, even when style 1 will force a crate regardless of its result.
inline int builderRewardChoice(int previous, int health, unsigned style, GameMakerRandom& random) {
    int reward = previous == 0 ? (random.irandomSmall(1) ? 109 : 293)
                              : previous == 109 ? 293 : 109;
    if (health <= 2 && reward != 109 && random.irandomSmall(5) != 0) reward = 109;
    if (style == 1) reward = 109;
    return reward;
}
}
#endif
