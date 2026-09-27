#ifndef DOWNWELL_CORE_SCRIPT_EFFECT_RUNTIME_BINDING_H
#define DOWNWELL_CORE_SCRIPT_EFFECT_RUNTIME_BINDING_H
#include "InstanceHandle.h"
namespace downwell {
// Borrowed stable owners, valid while registered effects exist. No allocation
// or retained stack lambda. Room owners detach this binding once records vanish.
struct ScriptEffectRuntimeBinding {
    void* effects=nullptr;void* registry=nullptr;
    std::uint32_t cutoff=0;
    std::uint32_t (*capture)(void*)=nullptr;
    bool (*animation)(void*,void*,InstanceHandle,const void*,std::uint32_t)=nullptr;
    bool (*retirement)(void*,void*,InstanceHandle,const void*)=nullptr;
    bool (*clear)(void*,void*)=nullptr;
    void beginAnimation(){if(capture)cutoff=capture(effects);}
    bool advance(InstanceHandle key,const void* record){
        return animation&&animation(effects,registry,key,record,cutoff);
    }
    bool retire(InstanceHandle key,const void* record){
        return retirement&&retirement(effects,registry,key,record);
    }
    bool clearRoom(){return clear&&clear(effects,registry);}
    template<class Record,class Effects,class Registry>
    static ScriptEffectRuntimeBinding bind(Effects& effects,Registry& registry){
        return {&effects,&registry,0,
            [](void* p){return static_cast<Effects*>(p)->beginPhase().newest;},
            [](void* p,void* r,InstanceHandle key,const void* record,std::uint32_t cutoff){
                return static_cast<Registry*>(r)->dispatchAnimation(key,*static_cast<const Record*>(record),
                    *static_cast<Effects*>(p),typename Effects::Phase{cutoff});
            },
            [](void* p,void* r,InstanceHandle key,const void* record){
                return static_cast<Registry*>(r)->retireMarked(key,*static_cast<const Record*>(record),*static_cast<Effects*>(p));
            },
            [](void* p,void* r){return static_cast<Effects*>(p)->clear(*static_cast<Registry*>(r));}};
    }
};
}
#endif
