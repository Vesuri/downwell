#ifndef DOWNWELL_CORE_CORPSE_EXPIRY_EFFECT_H
#define DOWNWELL_CORE_CORPSE_EXPIRY_EFFECT_H
#include "NonLoopSpriteAnimation.h"
namespace downwell {
// Object 242 (fxJobutsu), parent 219 (parentNoloopFx). No Step event,
// TimeStopBound, movement or Create RNG. Ordinary sprite drawing, not drawFx.
struct CorpseExpiryEffect:NonLoopSpriteAnimation {
    std::uint32_t drawBirth=0; // Shared depth -1000 insertion order, assigned by world owner.
    RunnerImageValue x{},y{};
    CorpseExpiryEffect(){imageSpeed={0x3e99999au};spriteId=102;frames=12;}
    // The corpse's published binary32 position, without a binary64 trip.
    bool create(RunnerImageValue px,RunnerImageValue py){
        CorpseExpiryEffect next;next.x=px;next.y=py;
        *this=next;return valid;
    }
    bool create(MotionBinary64 px,MotionBinary64 py){
        CorpseExpiryEffect next;
        next.valid=next.x.assign(px)&&next.y.assign(py);
        *this=next;return valid;
    }
    // Caller supplies verified normal, consecutive unit-key sprite metadata.
    // Preserve source setter semantics and publish changed collision bounds.
    template<class Publish> bool setSprite(int id,unsigned count,Publish publish){
        return NonLoopSpriteAnimation::setSprite(id,count,[&](const auto&){return publish(*this);});
    }
};
}
#endif
