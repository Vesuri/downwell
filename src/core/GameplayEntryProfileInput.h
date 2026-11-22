#ifndef DOWNWELL_GAMEPLAY_ENTRY_PROFILE_INPUT_H
#define DOWNWELL_GAMEPLAY_ENTRY_PROFILE_INPUT_H
#include "Input.h"
namespace downwell {
// Input-only traversal. Scene numbers follow splash, garden, style, gameplay;
// no shortcut, RNG consumption, actor injection or host-clock dependence.
class GameplayEntryProfileInput {
    unsigned scene_=~0u,frame_=0;
public:
    static constexpr unsigned gameplayFrames=120;
    // gardenX: the garden player's x. W1.14: the rim stops a walk, so after
    // 40 frames the garden takes GardenMotion.h's gardenRouteInput.
    Input sample(unsigned scene,int gardenX){
        if(scene!=scene_){scene_=scene;frame_=0;}
        const unsigned frame=++frame_;
        Input input;
        if(scene==0)input.fire=(frame&1)==0;
        else if(scene==1){
            input.right=frame>40&&gardenX<300;
            input.fire=frame>40&&gardenX>=240&&gardenX<300;
        }
        else if(scene==2)input.fire=frame==120;
        // Released input after confirmation measures entry and initial fall,
        // without conflating the first presentation with an injected shot.
        return input;
    }
};
}
#endif
