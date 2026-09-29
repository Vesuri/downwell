#ifndef DOWNWELL_CORE_SCENE_DESCENT_VERIFICATION_H
#define DOWNWELL_CORE_SCENE_DESCENT_VERIFICATION_H
#include "Input.h"
namespace downwell {
// Test input, not game policy. After the first landing, seek the central
// opening instead of repeatedly jumping between the two upper ledges.
// Position is room-space Q8 on both backends. No teleport, damage immunity,
// terrain edits or RNG intervention; success still requires real descent.
inline Input sceneDescentVerificationInput(unsigned tick,long roomXQ8){
    Input input;
    input.left=roomXQ8>242L*256;
    input.right=roomXQ8<238L*256;
    const unsigned phase=tick%16;
    input.fire=phase==0||phase==3;
    return input;
}
}
#endif
