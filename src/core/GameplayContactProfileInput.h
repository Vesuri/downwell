#ifndef DOWNWELL_CORE_GAMEPLAY_CONTACT_PROFILE_INPUT_H
#define DOWNWELL_CORE_GAMEPLAY_CONTACT_PROFILE_INPUT_H
#include "Input.h"
namespace downwell {
// Profiling inputs only: no instance/state injection and no random draws.
// Shared by the host gate and native autorun so contact coverage is testable
// before spending an emulator run on a tape that no longer reaches damage.
constexpr unsigned gameplayContactProfilePhase=7;
constexpr unsigned gameplayContactProfileFrames=1800;
inline Input gameplayContactProfileInput(unsigned tick,int x,int y,unsigned phase=gameplayContactProfilePhase){
    Input input;
    input.right=(tick/90)%2==0;input.left=!input.right;
    input.fire=(tick+phase)%10<5;
    if(y<1264){input.left=x>242;input.right=x<238;}
    return input;
}
// The bounded encounter search and renderer census must replay the same input
// route. Only buttons change; level creation and RNG use ordinary startup.
inline bool gameplayEncounterProfileInput(unsigned tick,int x,int y,unsigned route,Input& out){
    if(route>=64)return false;
    constexpr unsigned periods[]={30,60,120,240,360,480,720,1800};
    constexpr unsigned firePeriods[]={2,4,6,10,16,24,40,60};
    const unsigned movement=periods[route/8],fire=firePeriods[route%8];
    Input input;
    input.right=(tick/movement)%2==0;input.left=!input.right;
    input.fire=tick%fire<fire/2;
    if(y<1264){input.left=x>242;input.right=x<238;}
    out=input;return true;
}
}
#endif
