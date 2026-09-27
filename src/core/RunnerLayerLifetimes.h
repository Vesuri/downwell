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
    bool birth(InstanceHandle h,std::uint32_t sourceId,RunnerImageValue depth,bool inactive){
        if(!succeeded()||h.slot>=I||!h.generation||states_[h.slot].handle.generation||
            !RunnerSpatialSplit::valid(depth))return fail();
        auto& state=states_[h.slot];state={h,sourceId,{depth,-1,false,false},inactive};
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
            auto& state=states_[slot];const auto element=registry_.find(state.handle);
            if(!resolve(state.handle)||element==Registry::End)return false;
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
private:
    bool fail(){good_=false;return false;}
    Layers& layers_;Registry& registry_;RunnerLayerIds& ids_;
    State states_[I?I:1]{};
    RunnerDepthChanges<I> pending_;
    bool good_=true;
};
}
#endif
