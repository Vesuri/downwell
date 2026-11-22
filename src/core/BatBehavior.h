#ifndef DOWNWELL_CORE_BAT_BEHAVIOR_H
#define DOWNWELL_CORE_BAT_BEHAVIOR_H
#include "BatMotion.h"
namespace downwell {
struct BatActivity {
    bool initialized = false, flying = true;
    int hang = 0;
};
struct BatStepEffects {
    bool hit;
    unsigned wakeSounds;
    int sprite;
    bool succeeded=true;
};
inline int batPresentation(const BatActivity& state, const BatHitState& hit) {
    return !state.flying ? (state.hang > -10 ? 162 : 157) : (hit.stunned ? 154 : 153);
}
// After x/y and sprite_index in enmBat Step: flight copies sprite to mask;
// hanging preserves its mask, optionally mirrors wall art, then resets heading.
template<class Mask,class Scale,class Rest>
bool finishBatFields(const BatActivity& state,int sprite,Mask mask,Scale scale,Rest rest){
    if(state.flying)return mask(sprite);
    if(state.hang>-10&&!scale(-state.hang))return false;
    rest();return true;
}

// enmBat Step: support discovery precedes hit handling. Waking in the inactive
// branch changes art immediately, but pursuit starts on the following Step.
// Pursuit aims at global.eplx/eply (target); waking tests global.ply (wakeY).
// They differ only while a Heart Balloon is held.
template<class Random, class Solid,class Scale=NoBatScale>
BatStepEffects stepBatBehavior(BatMotion& bat, BatHitState& hit, BatActivity& state,
    int publishedX, int publishedY, int previousMask,
    long targetX, long targetY, long wakeY, Random& random, Solid solid,Scale scale={}) {
    const auto meeting = [&](int x, int y) {
        const auto b = contactWorldBounds({x * 256L, y * 256L, bat.facing, 1, previousMask});
        return solid(b.left, b.top) || solid(b.right - 1, b.top) ||
               solid(b.left, b.bottom - 1) || solid(b.right - 1, b.bottom - 1);
    };
    if (!state.initialized) {
        if (meeting(publishedX, publishedY - 16)) { state.flying = false; state.hang = -10; }
        else if (meeting(publishedX + 16, publishedY)) {
            if (!meeting(publishedX, publishedY + 16)) { state.flying = false; state.hang = 1; }
        } else if (meeting(publishedX - 16, publishedY)) {
            if (!meeting(publishedX, publishedY + 16)) { state.flying = false; state.hang = -1; }
        }
        state.initialized = true;
    }
    if (hit.ready) state.flying = true;
    const bool pursuit = state.flying && !hit.stunned && !hit.ready;
    const auto active=state.flying ? stepActiveBat(bat, hit, targetX, targetY, random, solid,scale)
                                  : BatActiveStep{false,true};
    if(!active.succeeded)return {active.hit,0,-1,false};
    const bool consumed=active.hit;
    unsigned sounds = 0;
    if (!pursuit) {
        if (wakeY > (publishedY - 4L) * 256) { state.flying = true; ++sounds; }
        // hang is a GML numeric boolean: -1 selects the left-wall branch.
        const bool support = state.hang == -10 ? meeting(publishedX, publishedY - 16)
            : meeting(publishedX + (state.hang > 0 ? 16 : -16), publishedY);
        if (!support) { state.flying = true; ++sounds; }
    }
    return {consumed, sounds, batPresentation(state, hit)};
}
template<class Random, class Solid,class Scale=NoBatScale>
BatStepEffects stepBatBehavior(BatMotion& bat, BatHitState& hit, BatActivity& state,
    int publishedX, int publishedY, int previousMask,
    long targetX, long targetY, Random& random, Solid solid,Scale scale={}) {
    return stepBatBehavior(bat, hit, state, publishedX, publishedY, previousMask,
        targetX, targetY, targetY, random, solid, scale);
}
}
#endif
