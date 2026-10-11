#ifndef DOWNWELL_CORE_RUNNER_LAYER_LIFETIMES_H
#define DOWNWELL_CORE_RUNNER_LAYER_LIFETIMES_H
#include "RunnerLayerInstances.h"
#include "RunnerLayerIds.h"
namespace downwell {
template<bool Small> struct RunnerLayerSourceLink { using Type=unsigned; };
template<> struct RunnerLayerSourceLink<true> { using Type=std::uint16_t; };
// Shared lifetime/depth adapter over a room's element registry. Install before
// births. Room resource preparation and persistent-state transfer remain the
// room owner's responsibility. Large owners must not live on the Amiga stack.
template<unsigned L,unsigned E,unsigned I,bool DirectIdentity=false,bool CompactUnnumbered=false,
    class LayerStorage=RunnerLayers<L,E>,bool IndexedSourceIds=true> class RunnerLayerLifetimes {
public:
    using Registry=RunnerLayerInstances<L,E,DirectIdentity?I:0,LayerStorage>;
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
    void observeFailure(bool& owner){good_.observe(owner);layers_.observeFailure(owner);pending_.observeFailure(owner);}
    const State* resolve(InstanceHandle h)const{
        if constexpr(CompactUnnumbered)if(h.slot<I&&compact_[h.slot]){
            const unsigned element=registry_.find(h);
            const auto* entry=registry_.get(element);
            const auto* layer=layers_.get(layers_.elementOwner(element));
            if(!entry||entry->sourceId||!layer)return nullptr;
            // Publish stable ordinary storage on first observation.
            states_[h.slot]={h,0,{layer->encodedDepth,layer->id,true,false},compact_[h.slot]==2};
            compact_[h.slot]=0;
        }
        return h.slot<I&&h.generation&&states_[h.slot].handle.generation==h.generation?&states_[h.slot]:nullptr;
    }
    bool attachedLayer(InstanceHandle h,int& id)const{
        if constexpr(CompactUnnumbered)if(h.slot<I&&compact_[h.slot]){
            const auto* layer=layers_.get(layers_.elementOwner(registry_.find(h)));
            if(!layer)return false;id=layer->id;return true;
        }
        const auto* state=resolve(h);if(!state)return false;
        id=state->depth.layerId;return true;
    }
    // Adopt surviving instance fields before serialized room initialization.
    // This is not Create: do not attach, truncate depth, or invent/reset the
    // saved layer ID. The room owner supplies fields after source teardown.
    // Pending depth requests belong to that owner too, not to this snapshot.
    bool adopt(const State& state){
        if(!state.sourceId)return fail();
        return adoptState(state);
    }
private:
    // Numbered ownership is maintained at birth/retirement, not rediscovered
    // by scanning every possible family slot for every room instance. This
    // directory does not order source events or replace layer membership.
    unsigned findSource(std::uint32_t id)const{
        if constexpr(IndexedSourceIds){
            if(id){
                for(unsigned slot=sourceHead_;slot!=I;slot=sourceNext_[slot])
                    if(states_[slot].sourceId==id)return slot;
                return I;
            }
        }
        // Preserve the reference's first-slot rule for unnumbered lookup.
        for(unsigned slot=0;slot<I;++slot)
            if(states_[slot].handle.generation&&states_[slot].sourceId==id)return slot;
        return I;
    }
    void publishSource(unsigned slot){
        if constexpr(IndexedSourceIds)if(states_[slot].sourceId){
            sourcePrevious_[slot]=I;sourceNext_[slot]=sourceHead_;
            if(sourceHead_!=I)sourcePrevious_[sourceHead_]=slot;
            sourceHead_=slot;
        }
    }
    void retireSource(unsigned slot){
        if constexpr(IndexedSourceIds)if(states_[slot].sourceId){
            const unsigned before=sourcePrevious_[slot],after=sourceNext_[slot];
            if(before==I)sourceHead_=after;else sourceNext_[before]=after;
            if(after!=I)sourcePrevious_[after]=before;
        }
    }
    bool adoptState(const State& state){
        DW_PROFILE_SCOPE(profileLayerAdopt);
        const auto h=state.handle;
        if(!succeeded()||h.slot>=I||!h.generation||states_[h.slot].handle.generation||
            !RunnerSpatialSplit::valid(state.depth.depth))return fail();
        if constexpr(CompactUnnumbered)if(compact_[h.slot])return fail();
        if(state.sourceId&&findSource(state.sourceId)!=I)return fail();
        // Scalar stores avoid bytewise aggregate construction in the 68000
        // compiler. Copy all semantic fields; padding has no stored meaning.
        auto& target=states_[h.slot];
        target.handle.slot=state.handle.slot;
        target.handle.generation=state.handle.generation;
        target.sourceId=state.sourceId;
        target.depth.depth.bits=state.depth.depth.bits;
        target.depth.layerId=state.depth.layerId;
        target.depth.attached=state.depth.attached;
        target.depth.append=state.depth.append;
        target.inactive=state.inactive;
        publishSource(h.slot);
        return true;
    }
public:
    // Globals enumerates surviving handles in original global hash order.
    // Serialized bindings precede fallback and the one inactive-tail pass.
    // Other initializes owned non-instance elements (e.g. backgrounds).
    template<class Globals,class Other> bool initializeRoom(Globals globals,Other other){
        if(!succeeded())return false;
        return registry_.initializeRoom(layers_,
            [&](std::uint32_t id,InstanceHandle& h,RunnerDepthService::Instance*& out){
                out=nullptr;
                const unsigned slot=findSource(id);
                if(slot!=I){h=states_[slot].handle;out=&states_[slot].depth;}
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
        if(!sourceId)return fail();
        return birthState(h,sourceId,depth,inactive);
    }
    // Gameplay room changes retire all actors; no global instance can bind to
    // a serialized placeholder. Prove that once, not once per placeholder.
    template<class Other> bool initializeEmptyRoom(Other other){
        if(!succeeded())return false;
        for(unsigned i=0;i<I;++i){
            if(states_[i].handle.generation)return fail();
            if constexpr(CompactUnnumbered)if(compact_[i])return fail();
        }
        return registry_.initializeRoom(layers_,
            [](std::uint32_t,InstanceHandle&,RunnerDepthService::Instance*& out){out=nullptr;return true;},
            [](auto){return true;},[](unsigned){return false;},
            [&](int depth,unsigned& slot){return ids_.createDynamic(layers_,depth,slot);},other)||fail();
    }
    // Native actors whose original global numeric ID is not recovered yet.
    // Ownership/migration use handles; no synthetic source ID is allocated.
    // Such lifetimes must be retired, never carried through room ID lookup.
    bool birthUnnumbered(InstanceHandle h,RunnerImageValue depth,bool inactive,bool compact=false){
        if constexpr(CompactUnnumbered)if(compact){
            if(!succeeded()||h.slot>=I||!h.generation||compact_[h.slot]||
                states_[h.slot].handle.generation||!RunnerSpatialSplit::valid(depth))return fail();
            if(!registry_.attachFreshUnnumbered(layers_,h,depth,
                [&](int value,unsigned& slot){return ids_.createDynamic(layers_,value,slot);},
                [&](RunnerImageValue publishedDepth,int layer,bool attached){
                    // Preserve adoption and partial attachment on failure.
                    states_[h.slot]={h,0,{publishedDepth,layer,attached,false},inactive};
                }))return fail();
            compact_[h.slot]=inactive?2:1;return true;
        }
        return birthState(h,0,depth,inactive);
    }
private:
    bool birthState(InstanceHandle h,std::uint32_t sourceId,RunnerImageValue depth,bool inactive){
        if(!adoptState({h,sourceId,{depth,-1,false,false},inactive}))return false;
        auto& state=states_[h.slot];
        const auto reserved=registry_.find(sourceId);
        const bool ok=reserved!=Registry::End?
            registry_.attach(layers_,layers_.elementOwner(reserved),sourceId,h,state.depth,inactive):
            registry_.attachRoomFallback(layers_,sourceId,h,state.depth,inactive,
                [&](int value,unsigned& slot){return ids_.createDynamic(layers_,value,slot);});
        return ok||fail();
    }
public:
    // Publish flags when source activation changes, but relink only at its
    // separate reconciliation phase (not at the flag-write boundary).
    bool publishInactive(InstanceHandle h,bool inactive){
        if constexpr(CompactUnnumbered)if(h.slot<I&&compact_[h.slot]){
            if(!succeeded()||registry_.find(h)==Registry::End)return fail();
            compact_[h.slot]=inactive?2:1;return true;
        }
        if(!succeeded()||!resolve(h))return fail();
        states_[h.slot].inactive=inactive;return true;
    }
    bool reconcileActivation(InstanceHandle h){
        if constexpr(CompactUnnumbered)if(h.slot<I&&compact_[h.slot])
            return (succeeded()&&layers_.reconcileInstanceActivation(registry_.find(h),compact_[h.slot]==2))||fail();
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
                    if constexpr(CompactUnnumbered)if(entry->instance.slot<I&&compact_[entry->instance.slot])
                        return registry_.find(entry->instance)==e;
                    const auto* live=resolve(entry->instance);if(!live)return false;
                    states_[entry->instance.slot].depth.layerId=id;return true;
                },[&](unsigned e){
                    const auto* entry=registry_.get(e);
                    if constexpr(CompactUnnumbered)if(entry&&entry->instance.slot<I&&compact_[entry->instance.slot])
                        return compact_[entry->instance.slot]==2;
                    const auto* live=entry?resolve(entry->instance):nullptr;
                    return live&&live->inactive;
                });
        })||fail();
    }
    bool retire(InstanceHandle h){
        if constexpr(CompactUnnumbered)if(h.slot<I&&compact_[h.slot]){
            if(!succeeded()||registry_.find(h)==Registry::End||!pending_.retire(h.slot)||
                !registry_.retire(layers_,0,h))return fail();
            compact_[h.slot]=0;return true;
        }
        const auto* state=resolve(h);
        if(!succeeded()||!state)return fail();
        if(!pending_.retire(h.slot))return fail();
        // 0x4258f4 skips detachment when room uninitialization has already
        // cleared the attached flag. Its unbound placeholder is room-owned.
        if(state->depth.attached){
            if(!registry_.retire(layers_,state->sourceId,h))return fail();
        }else if(registry_.find(h)!=Registry::End)return fail();
        retireSource(h.slot);
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
                        const unsigned slot=findSource(id);
                        if(slot!=I){h=states_[slot].handle;out=&states_[slot].depth;}
                        return true;
                    });
            });
        })||fail();
    }
private:
    bool fail(){good_=false;return false;}
    Layers& layers_;Registry& registry_;RunnerLayerIds& ids_;
    mutable State states_[I?I:1]{};
    using SourceLink=typename RunnerLayerSourceLink<(I<65535)>::Type;
    SourceLink sourceHead_=I;
    SourceLink sourceNext_[IndexedSourceIds&&I?I:1]{};
    SourceLink sourcePrevious_[IndexedSourceIds&&I?I:1]{};
    // Enabled only by owners routing all depth/attachment mutations here.
    // 0 = ordinary/absent, 1 = compact active, 2 = compact inactive.
    mutable std::uint8_t compact_[CompactUnnumbered&&I?I:1]{};
    RunnerDepthChanges<I> pending_;
    FailureSignal<> good_=true;
};
}
#endif
