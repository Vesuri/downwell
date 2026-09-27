#ifndef DOWNWELL_CORE_RUNNER_LAYER_INSTANCES_H
#define DOWNWELL_CORE_RUNNER_LAYER_INSTANCES_H
#include "InstanceHandle.h"
#include "RunnerDepthChanges.h"
#include "RunnerLayers.h"
namespace downwell {
// Source-instance-ID lookup for type-2 layer elements. Empty serialized
// entries exist before room instances do (0x413cdc); attachment fills them in
// place (0x412ab0), independently of instance creation/storage order.
template<unsigned LayerCapacity,unsigned ElementCapacity> class RunnerLayerInstances {
public:
    using Layers=RunnerLayers<LayerCapacity,ElementCapacity>;
    static constexpr unsigned End=ElementCapacity;
    struct Entry {std::uint32_t sourceId=0;InstanceHandle instance;bool present=false;};
    const Entry* get(unsigned element)const{return element<ElementCapacity&&entries_[element].present?&entries_[element]:nullptr;}
    unsigned find(std::uint32_t sourceId)const{
        for(unsigned i=0;i<ElementCapacity;++i)if(entries_[i].present&&entries_[i].sourceId==sourceId)return i;
        return End;
    }
    unsigned find(InstanceHandle h)const{
        if(!h.generation)return End;
        for(unsigned i=0;i<ElementCapacity;++i)if(entries_[i].present&&
            entries_[i].instance.slot==h.slot&&entries_[i].instance.generation==h.generation)return i;
        return End;
    }
    // Call in serialized membership-array order; each placeholder enters at
    // head, so final element order is reversed. No source element ID is
    // fabricated: element slots and source instance IDs are distinct concepts.
    bool reserve(Layers& layers,unsigned layer,std::uint32_t sourceId){
        if(find(sourceId)!=End)return false;
        const unsigned element=free(layers);
        if(element==End||!layers.attach(layer,element,true,false,[](unsigned){return false;}))return false;
        entries_[element]={sourceId,{},true};return true;
    }
    bool attach(Layers& layers,unsigned layer,std::uint32_t sourceId,InstanceHandle h,
        RunnerDepthService::Instance& state,bool inactive){
        if(!h.generation||h.slot==0xffffffffu||state.attached||find(h)!=End||!layers.succeeded())return false;
        const auto* destination=layers.get(layer);if(!destination)return false;
        unsigned element=find(sourceId);const bool reused=element!=End;
        if(reused){
            if(entries_[element].instance.generation||layers.elementOwner(element)!=layer)return false;
        }else{element=free(layers);if(element==End)return false;}
        RunnerImageValue depth;
        if(!depth.assign(MotionBinary64::fromInteger(destination->depth)))return false;
        entries_[element]={sourceId,h,true};
        state.attached=true;state.layerId=destination->id;state.depth=depth;
        if(!reused)return layers.attach(layer,element,true,state.append,[](unsigned){return false;});
        return !inactive||layers.reconcileInstanceActivation(element,true);
    }
    bool retire(Layers& layers,std::uint32_t sourceId,InstanceHandle h){
        const unsigned element=find(h);
        if(element==End||entries_[element].sourceId!=sourceId||!layers.detach(element))return false;
        entries_[element]=Entry{};return true;
    }
    // 0x41498b: uninitialize a bound type-2 element without deleting its
    // room membership. Source lookup, not the cached pointer, finds the
    // instance whose attachment fields are cleared. Depth is untouched.
    template<class Lookup>
    bool uninitialize(Layers& layers,unsigned element,Lookup lookup){
        if(!layers.succeeded()||!get(element))return false;
        auto& entry=entries_[element];
        if(!entry.instance.generation)return true;
        if(entry.sourceId<=0x7fffffffu){
            InstanceHandle h;RunnerDepthService::Instance* state=nullptr;
            if(!lookup(entry.sourceId,h,state))return false;
            if(state){
                if(!h.generation||h.slot==0xffffffffu)return false;
                state->attached=false;state->layerId=-1;
            }
        }
        entry.instance={};return true;
    }
    // Search only this room for the attachment described by the instance.
    // Missing layer/element or a different lifetime is a source no-op. Do not
    // clear the instance's attached flag, saved ID or depth: the destination
    // initialization publishes those fields after this release succeeds.
    bool releaseForRebind(Layers& layers,InstanceHandle h,const RunnerDepthService::Instance& state){
        if(!layers.succeeded())return false;
        if(!state.attached||state.layerId==-1)return true;
        const unsigned layer=layers.find(state.layerId);
        if(layer==Layers::End)return true;
        const unsigned element=find(h);
        if(element==End||layers.elementOwner(element)!=layer)return true;
        if(!layers.detachForRebind(element))return false;
        entries_[element]=Entry{};return true;
    }
    template<class Lookup>
    bool initializeReserved(Layers& layers,unsigned element,Lookup lookup){
        return initializeReserved(layers,element,lookup,[&](InstanceHandle h,const auto& state){
            return releaseForRebind(layers,h,state);
        });
    }
    // Serialized type-2 initialization (0x412ea5..0x412fe6), not ordinary
    // birth attachment. Missing global instances leave placeholders unbound.
    // Lookup returns a borrowed state pointer plus generation-checked handle;
    // a null pointer means absent. Release implements the source's search of
    // THIS room for the old attachment, not global destruction. It must leave
    // the destination placeholder/layer and borrowed instance state alive.
    // In particular no depth write or inactive tail move occurs here.
    template<class Lookup,class Release>
    bool initializeReserved(Layers& layers,unsigned element,Lookup lookup,Release release){
        if(!layers.succeeded()||!get(element))return false;
        auto& entry=entries_[element];
        if(entry.instance.generation||entry.sourceId>0x7fffffffu)return true;
        const unsigned layer=layers.elementOwner(element);
        const auto* destination=layers.get(layer);if(!destination)return false;
        InstanceHandle h;RunnerDepthService::Instance* state=nullptr;
        if(!lookup(entry.sourceId,h,state))return false;
        if(!state)return true;
        if(!h.generation||h.slot==0xffffffffu)return false;
        const auto id=destination->id;
        if(state->attached&&state->layerId!=-1&&!release(h,*state))return false;
        // A surviving same-room attachment would alias the lifetime. Release
        // must retire it before publication, without retiring the instance.
        if(find(h)!=End||!get(element)||layers.elementOwner(element)!=layer||
            !layers.get(layer)||layers.get(layer)->id!=id)return false;
        entry.instance=h;state->attached=true;state->layerId=id;
        return true;
    }
    // Room initialization's fallback (0x413590..0x413610), after serialized
    // element initialization. Caller supplies original global-instance hash
    // iteration order, then performs the separate inactive-tail normalization.
    // This is NOT depth migration: a surviving saved ID may name a FIXED layer
    // and takes priority over depth, even if the instance depth differs.
    template<class CreateLayer>
    bool attachRoomFallback(Layers& layers,std::uint32_t sourceId,InstanceHandle h,
        RunnerDepthService::Instance& state,bool inactive,CreateLayer create){
        if(!layers.succeeded())return false;
        if(state.attached)return true;
        unsigned layer=state.layerId==-1?Layers::End:layers.find(state.layerId);
        if(layer==Layers::End){
            std::int32_t depth;
            if(!RunnerDepthService::integerDepth(state.depth,depth))return false;
            layer=layers.findDynamic(depth);
            if(layer==Layers::End){
                if(!create(depth,layer))return false;
                const auto* created=layers.get(layer);
                if(!created||!created->dynamic||created->depth!=depth)return false;
            }
        }
        return attach(layers,layer,sourceId,h,state,inactive);
    }
    // Instance-element projection of 0x413500. Other serialized element types
    // must be initialized by their owner before this projection can represent
    // a complete room. Globals visits allocated instances in original global
    // hash order; it must not create/retire instances. Lookup uses the same
    // contract as initializeReserved. Inactive accepts an element slot and
    // must reject null references (and validate the borrowed lifetime).
    template<class Lookup,class Globals,class Inactive,class CreateLayer>
    bool initializeRoom(Layers& layers,Lookup lookup,Globals globals,
        Inactive inactive,CreateLayer create){
        return initializeRoom(layers,lookup,globals,inactive,create,[](unsigned){return false;});
    }
    // Mixed element lists use their non-instance owner for initialization.
    // Unknown/unowned slots must fail, not silently disappear from traversal.
    template<class Lookup,class Globals,class Inactive,class CreateLayer,class Other>
    bool initializeRoom(Layers& layers,Lookup lookup,Globals globals,
        Inactive inactive,CreateLayer create,Other other){
        if(!layers.succeeded())return false;
        if(!layers.visitLayers([&](unsigned layer,const auto&){
            return layers.visitElements(layer,[&](unsigned element){
                return get(element)?initializeReserved(layers,element,lookup):other(element);
            });
        }))return false;
        if(!globals([&](std::uint32_t sourceId,InstanceHandle h,
            RunnerDepthService::Instance& state,bool dormant){
            return attachRoomFallback(layers,sourceId,h,state,dormant,create);
        }))return false;
        return layers.normalizeRoomInstances(inactive);
    }
private:
    unsigned free(const Layers& layers)const{
        for(unsigned i=0;i<ElementCapacity;++i)
            if(!entries_[i].present&&layers.elementOwner(i)==Layers::End)return i;
        return End;
    }
    Entry entries_[ElementCapacity?ElementCapacity:1]{};
};
}
#endif
