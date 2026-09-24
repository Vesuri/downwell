#ifndef DOWNWELL_CORE_PLAYER_LIFE_RULES_H
#define DOWNWELL_CORE_PLAYER_LIFE_RULES_H
#include "Input.h"
namespace downwell {
// Contact response retains a depleted-health guard. Movement now receives its
// death flag from PlayerDeathSequence's Begin Step publication; it uses this
// input filter with that explicit state, not directly with collision-time HP.
// Full original same-frame multi-contact scheduling remains separate work.
inline bool playerCanAct(int health) { return health > 0; }
inline Input playerGameplayInput(Input input, int health) {
    if (!playerCanAct(health)) input.left = input.right = input.fire = false;
    return input; // quit/pause remain platform/menu requests, not movement.
}
}
#endif
