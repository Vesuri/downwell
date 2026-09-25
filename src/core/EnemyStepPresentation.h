#ifndef DOWNWELL_CORE_ENEMY_STEP_PRESENTATION_H
#define DOWNWELL_CORE_ENEMY_STEP_PRESENTATION_H
#include "EnemyPresentation.h"
#include "FrogPresentation.h"
#include "BatBehavior.h"

namespace downwell {
// End-of-Step selection, after motion and hit handling. Keep the special
// poses here: the generic object sprite is not authoritative for stunned
// enemies, hanging bats or the frog's forced ascending/descending frame.
struct EnemyStepPose {
    int object, auxiliary;
    bool grounded, rising, frogStunned, snailStunned;
};

inline int finishEnemyPresentation(const EnemyStepPose& state,
        FrogPresentation& frog, const BatActivity& bat, const BatHitState& batHit,
        unsigned& phase, unsigned char& remainder) {
    int sprite = enemyPresentationSprite(state.object, state.grounded, state.auxiliary);
    if (state.object == 117 && state.snailStunned) sprite = 178;
    if (state.object == 116) sprite = batPresentation(bat, batHit);
    if (state.object == 113) {
        const auto visual = frog.finishStep(state.frogStunned, state.grounded,
                                           state.auxiliary != 0, state.rising);
        sprite = visual.sprite;
        if (visual.forcedFrame >= 0) {
            phase = static_cast<unsigned>(visual.forcedFrame) * 256u;
            remainder = 0;
        }
    }
    const unsigned cycle = enemySpriteFrameCount(sprite) * 256u;
    while (phase >= cycle) phase -= cycle;
    return sprite;
}
} // namespace downwell
#endif
