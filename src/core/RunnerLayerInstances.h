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
