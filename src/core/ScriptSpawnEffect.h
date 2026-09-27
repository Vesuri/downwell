#ifndef DOWNWELL_CORE_SCRIPT_SPAWN_EFFECT_H
#define DOWNWELL_CORE_SCRIPT_SPAWN_EFFECT_H
#include "NonLoopSpriteAnimation.h"
#include "NonLoopEffects.h"
namespace downwell {
constexpr unsigned scriptSpawnEffectCapacity=64;
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
template<unsigned Capacity> class ScriptSpawnEffects:public NonLoopEffects<ScriptSpawnEffect,Capacity> {
    using Base=NonLoopEffects<ScriptSpawnEffect,Capacity>;
    const void* registry_=nullptr;
    template<class Registry> bool bind(Registry& registry){
        if(!registry_){registry_=&registry;return true;}
        if(registry_==&registry)return true;
        return Base::mutate(0,[](auto&){return false;}); // Latch mismatched ownership.
    }
public:
    ScriptSpawnEffects()=default;
    ScriptSpawnEffects(const ScriptSpawnEffects&)=delete;
    ScriptSpawnEffects& operator=(const ScriptSpawnEffects&)=delete;
    template<class Registry> bool boundTo(const Registry& registry)const{return registry_==&registry;}
    template<class Registry> typename Base::Handle create(MotionBinary64 x,MotionBinary64 y,Registry& registry){
        return bind(registry)?Base::create(x,y,registry):0;
    }
    template<class Registry> bool setSprite(typename Base::Handle h,int sprite,unsigned count,Registry& registry){
        return bind(registry)&&Base::setSprite(h,sprite,count,registry);
    }
    template<class Registry> bool advanceAnimation(typename Base::Phase phase,Registry& registry){
        return bind(registry)&&Base::advanceAnimation(phase,registry,
            [&](auto h,const auto& effect){return registry.image(h,effect);});
    }
    template<class Registry> bool clear(Registry& registry){return bind(registry)&&Base::clear(registry);}
    template<class Registry> bool advanceRegistered(typename Base::Phase phase,Registry& registry){
        return bind(registry)&&Base::advanceDeferred(phase,
            [&](auto h,const auto& effect){return registry.image(h,effect);},
            [&](auto h,const auto& effect){return registry.destroyed(h,effect);});
    }
    template<class Registry> bool advanceRegisteredOne(typename Base::Handle h,typename Base::Phase phase,Registry& registry){
        return bind(registry)&&Base::advanceDeferredOne(h,phase,
            [&](auto key,const auto& effect){return registry.image(key,effect);},
            [&](auto key,const auto& effect){return registry.destroyed(key,effect);});
    }
    template<class Registry> bool retireMarked(typename Base::Handle h,Registry& registry){
        return bind(registry)&&Base::retireMarked(h,registry);
    }
};
}
#endif
