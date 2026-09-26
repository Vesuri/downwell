#ifndef DOWNWELL_CORE_GAMEPLAY_CONTACT_PROFILE_INPUT_H
#define DOWNWELL_CORE_GAMEPLAY_CONTACT_PROFILE_INPUT_H
#include "Input.h"
namespace downwell {
// Profiling inputs only: no instance/state injection and no random draws.
// Shared by the host gate and native autorun so contact coverage is testable
// before spending an emulator run on a tape that no longer reaches damage.
constexpr unsigned gameplayContactProfilePhase=5;
constexpr unsigned gameplayContactProfileFrames=1800;
inline Input gameplayContactProfileInput(unsigned tick,int x,int y,unsigned phase=gameplayContactProfilePhase){
    Input input;
    input.right=(tick/90)%2==0;input.left=!input.right;
    input.fire=(tick+phase)%10<5;
    if(y<1264){input.left=x>242;input.right=x<238;}
    return input;
}
}
#endif
