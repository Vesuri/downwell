#ifndef DOWNWELL_CORE_PLAYER_LIFE_RULES_H
#define DOWNWELL_CORE_PLAYER_LIFE_RULES_H
#include "Input.h"
namespace downwell {
// Current simulation uses depleted health as its death boundary. The runner
// publishes global.death in player Begin Step; recovering that phase and its
// death animation is separate work. Do not let either backend keep controlling
// or stomping with a player that this simulation has already declared dead.
inline bool playerCanAct(int health) { return health > 0; }
inline Input playerGameplayInput(Input input, int health) {
    if (!playerCanAct(health)) input.left = input.right = input.fire = false;
    return input; // quit/pause remain platform/menu requests, not movement.
}
}
#endif
