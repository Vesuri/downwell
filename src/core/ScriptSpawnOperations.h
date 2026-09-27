#ifndef DOWNWELL_CORE_SCRIPT_SPAWN_OPERATIONS_H
#define DOWNWELL_CORE_SCRIPT_SPAWN_OPERATIONS_H
#include "ScriptSpawnEffect.h"
namespace downwell {
// Shared scrEffectSpawn operations. The registry maps owner-local lifetimes to
// backend identities. Caller publication and verified resource frame lookup
// remain explicit callbacks; no platform-specific game rules belong here.
template<unsigned Capacity,class Registry,class Frames,class Caller>
struct ScriptSpawnOperations {
    using Handle=typename ScriptSpawnEffects<Capacity>::Handle;
    ScriptSpawnEffects<Capacity>& effects;
    Registry& registry;Frames frames;Caller caller;bool& succeeded;
    bool check(bool ok){if(!ok)succeeded=false;return ok;}
    bool ready(){return succeeded&&check(effects.boundTo(registry));}
    bool create(int object,MotionBinary64 x,MotionBinary64 y,Handle& child){
        if(!succeeded)return false;
        if(object!=219)return check(false);
        const auto next=effects.create(x,y,registry);
        if(!check(next!=0))return false;
        child=next;return true;
    }
    bool myFx(Handle child){return ready()&&check(effects.get(child)&&caller(child));}
    bool sprite(Handle child,int id){
        if(!ready())return false;
        unsigned count=0;
        return check(frames(id,count)&&effects.setSprite(child,id,count,registry));
    }
    bool imageSpeed(Handle child,MotionBinary64 value){
        return ready()&&check(effects.mutate(child,[&](auto& e){
            return e.setValue(e.imageSpeed,value,[&](const auto& state){return registry.speed(child,state);});
        }));
    }
    bool imageAngle(Handle child,MotionBinary64 value){
        return ready()&&check(effects.mutate(child,[&](auto& e){
            return e.setValue(e.angle,value,[&](const auto& state){return registry.angle(child,state);});
        }));
    }
    bool depth(Handle child,MotionBinary64 value){
        return ready()&&check(effects.mutate(child,[&](auto& e){
            return e.setValue(e.depth,value,[&](const auto& state){return registry.depth(child,state);});
        }));
    }
};
template<unsigned Capacity,class Registry,class Frames,class Caller>
auto scriptSpawnOperations(ScriptSpawnEffects<Capacity>& effects,Registry& registry,
                          Frames frames,Caller caller,bool& succeeded){
    return ScriptSpawnOperations<Capacity,Registry,Frames,Caller>{effects,registry,frames,caller,succeeded};
}
}
#endif
