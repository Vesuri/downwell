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
};
inline int batPresentation(const BatActivity& state, const BatHitState& hit) {
    return !state.flying ? (state.hang > -10 ? 162 : 157) : (hit.stunned ? 154 : 153);
}

// enmBat Step: support discovery precedes hit handling. Waking in the inactive
// branch changes art immediately, but pursuit starts on the following Step.
template<class Random, class Solid>
BatStepEffects stepBatBehavior(BatMotion& bat, BatHitState& hit, BatActivity& state,
    int publishedX, int publishedY, int previousSprite,
    long targetX, long targetY, Random& random, Solid solid) {
    const auto meeting = [&](int x, int y) {
        const auto b = contactWorldBounds({x * 256L, y * 256L, bat.facing, 1, previousSprite});
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
    const bool consumed = state.flying && stepActiveBat(bat, hit, targetX, targetY, random, solid);
    unsigned sounds = 0;
    if (!pursuit) {
        if (targetY > (publishedY - 4L) * 256) { state.flying = true; ++sounds; }
        // hang is a GML numeric boolean: -1 selects the left-wall branch.
        const bool support = state.hang == -10 ? meeting(publishedX, publishedY - 16)
            : meeting(publishedX + (state.hang > 0 ? 16 : -16), publishedY);
        if (!support) { state.flying = true; ++sounds; }
    }
    if (!state.flying) {
        if (state.hang > -10) bat.facing = -state.hang;
        bat.direction = 270;
        bat.speedFortieths = 80;
    }
    return {consumed, sounds, batPresentation(state, hit)};
}
}
#endif
