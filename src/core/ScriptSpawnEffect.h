#ifndef DOWNWELL_CORE_SCRIPT_SPAWN_EFFECT_H
#define DOWNWELL_CORE_SCRIPT_SPAWN_EFFECT_H
#include "NonLoopSpriteAnimation.h"
#include "NonLoopEffects.h"
namespace downwell {
// Object 219 defaults, before scrEffectSpawn's caller-side overrides.
// No Create event or RNG; coordinates remain absolute published room values.
struct ScriptSpawnEffect:NonLoopSpriteAnimation {
    RunnerImageValue x{},y{},angle{},depth{};
    bool create(MotionBinary64 px,MotionBinary64 py){
        ScriptSpawnEffect next;next.valid=next.x.assign(px)&&next.y.assign(py);
        *this=next;return valid;
    }
    template<class Publish> bool setSprite(int id,unsigned count,Publish publish){
        return NonLoopSpriteAnimation::setSprite(id,count,[&](const auto&){return publish(*this);});
    }
    // The owner/registry supplies the actual publication hook. Keep each
    // setter separate, including depth-zero assignments, for source ordering.
    template<class Publish> bool setValue(RunnerImageValue& field,MotionBinary64 value,Publish publish){
        if(!valid)return false;
        RunnerImageValue next;
        if(!next.assign(value)){valid=false;return false;}
        field=next;valid=publish(*this);return valid;
    }
};
template<unsigned Capacity> using ScriptSpawnEffects=NonLoopEffects<ScriptSpawnEffect,Capacity>;
}
#endif
