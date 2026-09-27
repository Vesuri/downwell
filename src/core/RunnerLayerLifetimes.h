#ifndef DOWNWELL_CORE_RUNNER_LAYER_LIFETIMES_H
#define DOWNWELL_CORE_RUNNER_LAYER_LIFETIMES_H
#include "RunnerLayerInstances.h"
#include "RunnerLayerIds.h"
namespace downwell {
// Shared lifetime/depth adapter over a room's element registry. Install before
// births. Room resource preparation and persistent-state transfer remain the
// room owner's responsibility. Large owners must not live on the Amiga stack.
template<unsigned L,unsigned E,unsigned I> class RunnerLayerLifetimes {
public:
    using Registry=RunnerLayerInstances<L,E>;
    using Layers=typename Registry::Layers;
    struct State {
        InstanceHandle handle;
        std::uint32_t sourceId=0;
        RunnerDepthService::Instance depth;
        bool inactive=false;
    };
    RunnerLayerLifetimes(Layers& layers,Registry& registry,RunnerLayerIds& ids):
        layers_(layers),registry_(registry),ids_(ids){}
    RunnerLayerLifetimes(const RunnerLayerLifetimes&)=delete;
    RunnerLayerLifetimes& operator=(const RunnerLayerLifetimes&)=delete;
    bool succeeded()const{return good_&&layers_.succeeded()&&pending_.succeeded();}
    const State* resolve(InstanceHandle h)const{
        return h.slot<I&&h.generation&&states_[h.slot].handle.generation==h.generation?&states_[h.slot]:nullptr;
    }
    // Adopt surviving instance fields before serialized room initialization.
    // This is not Create: do not attach, truncate depth, or invent/reset the
    // saved layer ID. The room owner supplies fields after source teardown.
    // Pending depth requests belong to that owner too, not to this snapshot.
    bool adopt(const State& state){
        const auto h=state.handle;
        if(!succeeded()||h.slot>=I||!h.generation||states_[h.slot].handle.generation||
            !RunnerSpatialSplit::valid(state.depth.depth))return fail();
        for(unsigned i=0;i<I;++i)
            if(states_[i].handle.generation&&states_[i].sourceId==state.sourceId)return fail();
        states_[h.slot]=state;return true;
    }
    // Globals enumerates surviving handles in original global hash order.
    // Serialized bindings precede fallback and the one inactive-tail pass.
    // Other initializes owned non-instance elements (e.g. backgrounds).
    template<class Globals,class Other> bool initializeRoom(Globals globals,Other other){
        if(!succeeded())return false;
        return registry_.initializeRoom(layers_,
            [&](std::uint32_t id,InstanceHandle& h,RunnerDepthService::Instance*& out){
                out=nullptr;
                for(unsigned i=0;i<I;++i)if(states_[i].handle.generation&&states_[i].sourceId==id){
                    h=states_[i].handle;out=&states_[i].depth;break;
                }
                return true;
            },[&](auto visit){
                return globals([&](InstanceHandle h){
                    if(!resolve(h))return false;
                    auto& state=states_[h.slot];
                    return visit(state.sourceId,h,state.depth,state.inactive);
                });
            },[&](unsigned e){
                const auto* entry=registry_.get(e);
                const auto* state=entry?resolve(entry->instance):nullptr;
                return state&&state->inactive;
            },[&](int depth,unsigned& slot){return ids_.createDynamic(layers_,depth,slot);},other)||fail();
    }
    bool birth(InstanceHandle h,std::uint32_t sourceId,RunnerImageValue depth,bool inactive){
        if(!adopt({h,sourceId,{depth,-1,false,false},inactive}))return false;
        auto& state=states_[h.slot];
        const auto reserved=registry_.find(sourceId);
        const bool ok=reserved!=Registry::End?
            registry_.attach(layers_,layers_.elementOwner(reserved),sourceId,h,state.depth,inactive):
            registry_.attachRoomFallback(layers_,sourceId,h,state.depth,inactive,
                [&](int value,unsigned& slot){return ids_.createDynamic(layers_,value,slot);});
        return ok||fail();
    }
    // Publish flags when source activation changes, but relink only at its
    // separate reconciliation phase (not at the flag-write boundary).
    bool publishInactive(InstanceHandle h,bool inactive){
        if(!succeeded()||!resolve(h))return fail();
        states_[h.slot].inactive=inactive;return true;
    }
    bool reconcileActivation(InstanceHandle h){
        const auto* state=resolve(h);
        return (succeeded()&&state&&layers_.reconcileInstanceActivation(registry_.find(h),state->inactive))||fail();
    }
    bool writeDepth(InstanceHandle h,MotionBinary64 value){
        if(!succeeded()||!resolve(h))return fail();
        return pending_.request(h.slot,states_[h.slot].depth.depth,value)||fail();
    }
    bool serviceDepths(){
        if(!succeeded())return false;
        return pending_.service([&](unsigned slot){
            auto& state=states_[slot];
            if(!resolve(state.handle))return false;
            if(!state.depth.attached)return true;
            const auto element=registry_.find(state.handle);
            if(element==Registry::End)return false;
            return RunnerDepthService::apply(layers_,state.depth,element,
                [&](unsigned e){return e==element;},
                [&](int depth,unsigned& out){return ids_.createDynamic(layers_,depth,out);},
                [&](unsigned e,int id){
                    const auto* entry=registry_.get(e);if(!entry||!entry->instance.generation)return true;
                    const auto* live=resolve(entry->instance);if(!live)return false;
                    states_[entry->instance.slot].depth.layerId=id;return true;
                },[&](unsigned e){
                    const auto* entry=registry_.get(e);
                    const auto* live=entry?resolve(entry->instance):nullptr;
                    return live&&live->inactive;
                });
        })||fail();
    }
    bool retire(InstanceHandle h){
        const auto* state=resolve(h);
        if(!succeeded()||!state)return fail();
        if(!pending_.retire(h.slot)||!registry_.retire(layers_,state->sourceId,h))return fail();
        states_[h.slot]=State{};return true;
    }
    unsigned pendingDepths()const{return pending_.size();}
    // 0x414c50: forward room layers/elements, retaining their links. This
    // boundary is distinct from freeing the room or retiring its instances.
    // In particular it does not discard surviving pending depth requests.
    template<class Other> bool uninitializeRoom(Other other){
        if(!succeeded())return false;
        return layers_.visitLayers([&](unsigned layer,const auto&){
            return layers_.visitElements(layer,[&](unsigned e){
                if(!registry_.get(e))return other(e);
                return registry_.uninitialize(layers_,e,
                    [&](std::uint32_t id,InstanceHandle& h,RunnerDepthService::Instance*& out){
                        out=nullptr;
                        for(unsigned i=0;i<I;++i)if(states_[i].handle.generation&&states_[i].sourceId==id){
                            h=states_[i].handle;out=&states_[i].depth;break;
                        }
                        return true;
                    });
            });
        })||fail();
    }
private:
    bool fail(){good_=false;return false;}
    Layers& layers_;Registry& registry_;RunnerLayerIds& ids_;
    State states_[I?I:1]{};
    RunnerDepthChanges<I> pending_;
    bool good_=true;
};
}
#endif
