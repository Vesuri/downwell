#include "core/GameplayDebrisInstances.h"
#include "core/GameMakerRandom.h"
using namespace downwell;
#ifdef DOWNWELL_NATIVE_QUERY_PROBE
bool nativeDebrisRegistry(DebrisEffects<2>& effects,GameplayDebrisInstances& registry,GameMakerRandom& random){
    const auto h=effects.createRegistered({}, {},1,random,registry);
    const auto key=registry.handle(h);const auto* record=registry.find(key);
    return record&&registry.dispatchAnimation(key,*record,effects,effects.beginPhase())&&
        effects.stepRegisteredOne(h,effects.beginPhase(),random,
            [](auto,const auto&,bool& stopped){stopped=false;return true;},
            [](auto,const auto&,auto,auto,bool& hit){hit=false;return true;},
            [&](auto local,const auto& effect,auto x,auto y){return registry.move(local,effect,x,y);},registry)&&
        effects.clear(registry);
}
#else
#include <cassert>
#include <cstdio>
struct Audit {
    using Record=GameplayDebrisInstances::Record;
    GameplayDebrisInstances* registry=nullptr;
    unsigned births=0,images=0,moves=0,marks=0,retires=0;
    bool failMove=false,failImage=false,failMark=false,failRetire=false;
    RunnerImageValue x{},y{},image{};
    bool birth(InstanceHandle h,const Record& r){
        assert(registry->find(h)==&r&&r.effect&&!r.destroyRequested);
        x=r.effect->x;y=r.effect->y;image=r.effect->imageIndex;++births;return true;
    }
    bool retire(InstanceHandle h,const Record& r){assert(registry->find(h)==&r);++retires;return !failRetire;}
    bool activation(InstanceHandle,const Record&,bool){return true;}
    bool membership(InstanceHandle,const Record&,bool){return true;}
    bool sprite(InstanceHandle,const Record&,int){return true;}
    bool imageIndex(InstanceHandle h,const Record& r,RunnerImageValue value){
        assert(registry->find(h)==&r);++images;if(failImage)return false;image=value;return true;
    }
    bool move(InstanceHandle h,const Record& r,RunnerImageValue px,RunnerImageValue py){
        assert(registry->find(h)==&r&&r.effect->x.bits==x.bits&&r.effect->y.bits==y.bits);
        ++moves;if(failMove)return false;x=px;y=py;return true;
    }
    bool markDestroyed(InstanceHandle h,const Record& r){
        assert(registry->find(h)==&r&&r.effect->destroyRequested&&!r.destroyRequested);
        ++marks;return !failMark;
    }
};
int main(){
    for(unsigned seed=0;seed<256;++seed){
        Audit audit;GameplayDebrisInstances registry(GameplayDebrisInstances::Binding::bindDestruction(audit));
        audit.registry=&registry;DebrisEffects<1> effects;GameMakerRandom random(seed);
        const auto before=effects.beginPhase();
        const auto h=effects.createRegistered({0x406e0800u,0},{0x40660100u,0},1+seed%5,random,registry);
        const auto key=registry.handle(h);const auto* record=registry.find(key);
        assert(record&&record->effect==effects.retained(h)&&registry.size()==1);
        assert(registry.dispatchAnimation(key,*record,effects,before)&&!audit.images);
        const auto phase=effects.beginPhase();
        assert(registry.dispatchAnimation(key,*record,effects,phase)&&audit.images==1);
        assert(audit.image.bits==effects.get(h)->imageIndex.bits);
        unsigned queries=0;
        assert(effects.stepRegisteredOne(h,phase,random,
            [](auto,const auto&,bool& stopped){stopped=false;return true;},
            [&](auto,const auto& effect,auto,auto,bool& hit){
                assert(effect.x.bits==audit.x.bits&&effect.y.bits==audit.y.bits);
                ++queries;hit=false;return true;
            },[&](auto local,const auto& effect,auto x,auto y){return registry.move(local,effect,x,y);},registry));
        assert(audit.moves==2&&queries==3);
        effects.get(h)->alarms.set(0,1);
        assert(effects.advanceRegisteredAlarmOne(h,phase,0,registry)&&audit.marks==1&&record->destroyRequested);
        assert(!effects.get(h)&&registry.find(key)==record&&record->effect==effects.retained(h));
        assert(registry.dispatchAnimation(key,*record,effects,phase)&&audit.images==1);
        assert(registry.retireMarked(key,*record,effects)&&!registry.find(key)&&!registry.size()&&audit.retires==1);
        const auto next=effects.createRegistered({}, {},1,random,registry);
        assert(next>h&&!registry.find(key)&&registry.size()==1);
        assert(effects.clear(registry)&&!registry.size()&&audit.retires==2);
    }
    // Same local IDs in a different effect owner must not impersonate a record.
    {
        Audit audit;GameplayDebrisInstances registry(GameplayDebrisInstances::Binding::bindDestruction(audit));
        audit.registry=&registry;DebrisEffects<1> effects,impostor;GameMakerRandom random(1);
        const auto h=effects.createRegistered({}, {},1,random,registry);
        assert(impostor.create({}, {},1,random)==h);
        const auto key=registry.handle(h);
        assert(!registry.dispatchAnimation(key,*registry.find(key),impostor,impostor.beginPhase()));
        assert(!registry.valid()&&!audit.images&&effects.clear(registry)&&registry.size()==0);
    }
    for(unsigned failure=0;failure<4;++failure){
        Audit audit;GameplayDebrisInstances registry(GameplayDebrisInstances::Binding::bindDestruction(audit));
        audit.registry=&registry;DebrisEffects<1> effects;GameMakerRandom random(9);
        const auto h=effects.createRegistered({}, {},1,random,registry);const auto key=registry.handle(h);
        const auto phase=effects.beginPhase();
        if(failure==0){audit.failImage=true;assert(!registry.dispatchAnimation(key,*registry.find(key),effects,phase));}
        else if(failure==1){
            audit.failMove=true;
            assert(!effects.stepRegisteredOne(h,phase,random,
                [](auto,const auto&,bool& stop){stop=false;return true;},
                [](auto,const auto&,auto,auto,bool& hit){hit=false;return true;},
                [&](auto local,const auto& effect,auto x,auto y){return registry.move(local,effect,x,y);},registry));
        }else{
            effects.get(h)->alarms.set(0,1);audit.failMark=failure==2;
            assert(effects.advanceRegisteredAlarmOne(h,phase,0,registry)==(failure==3));
            if(failure==3){audit.failRetire=true;assert(!registry.retireMarked(key,*registry.find(key),effects));}
        }
        assert(!registry.valid()&&!effects.valid()&&registry.find(key));
        audit.failRetire=false;assert(effects.clear(registry)&&!registry.size()&&!registry.find(key));
        // A terminal registry must be reconstructed, never silently reused.
        assert(!registry.valid());
    }
    std::puts("Debris registry: 256 lifetimes, fractional writes, animation, deferred destruction, stale-owner rejection and failure teardown passed");
}
#endif
