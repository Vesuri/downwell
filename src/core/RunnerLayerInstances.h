#ifndef DOWNWELL_CORE_RUNNER_LAYER_INSTANCES_H
#define DOWNWELL_CORE_RUNNER_LAYER_INSTANCES_H
#include "InstanceHandle.h"
#include "RunnerDepthChanges.h"
#include "RunnerLayers.h"
namespace downwell {
template<bool Derived> struct RunnerLayerEntry {
    std::uint32_t sourceId=0;InstanceHandle instance;bool present=false;
};
template<> struct RunnerLayerEntry<true> {
    std::uint32_t sourceId=0;InstanceHandle instance;
};
// Source-instance-ID lookup for type-2 layer elements. Empty serialized
// entries exist before room instances do (0x413cdc); attachment fills them in
// place (0x412ab0), independently of instance creation/storage order.
// IdentitySlots opts into a bounded, one-live-generation-per-slot owner.
// Zero keeps the general full-handle hash index (including overlapping
// generations of one slot). Neither index determines element/Draw order.
template<unsigned LayerCapacity,unsigned ElementCapacity,unsigned IdentitySlots=0,
    class LayerStorage=RunnerLayers<LayerCapacity,ElementCapacity>> class RunnerLayerInstances {
public:
    using Layers=LayerStorage;
    static constexpr unsigned End=ElementCapacity;
    using Entry=RunnerLayerEntry<(IdentitySlots!=0)>;
    bool discardUninitialized(){
        for(const auto& entry:entries_)if(entry.instance.generation)return false;
        for(auto& entry:entries_)entry=Entry{};
        for(auto& head:identityHead_)head=0;
        for(auto& next:identityNext_)next=0;
        for(auto& element:identityElement_)element=0;
        usedEnd_=0;
        return true;
    }
    const Entry* get(unsigned element)const{return element<ElementCapacity&&present(entries_[element])?&entries_[element]:nullptr;}
    unsigned find(std::uint32_t sourceId)const{
        if(!sourceId)return End; // Unnumbered native lifetimes have no ID lookup.
        for(unsigned i=0;i<usedEnd_;++i)if(present(entries_[i])&&entries_[i].sourceId==sourceId)return i;
        return End;
    }
    unsigned find(InstanceHandle h)const{
        if(!h.generation)return End;
        if constexpr(IdentitySlots){
            if(h.slot>=IdentitySlots||!identityElement_[h.slot])return End;
            const unsigned element=identityElement_[h.slot]-1;
            return entries_[element].instance.generation==h.generation?element:End;
        }else{
            for(unsigned link=identityHead_[bucket(h)];link;link=identityNext_[link-1]){
                const unsigned i=link-1;
                if(entries_[i].instance.slot==h.slot&&entries_[i].instance.generation==h.generation)return i;
            }
            return End;
        }
    }
    // Call in serialized membership-array order; each placeholder enters at
    // head, so final element order is reversed. No source element ID is
    // fabricated: element slots and source instance IDs are distinct concepts.
    bool reserve(Layers& layers,unsigned layer,std::uint32_t sourceId){
        if(!sourceId||find(sourceId)!=End)return false;
        const unsigned element=free(layers);
        if(element==End||!layers.attach(layer,element,true,false,[](unsigned){return false;}))return false;
        entries_[element]={sourceId,{}};
        if constexpr(!IdentitySlots)entries_[element].present=true;
        if(element>=usedEnd_)usedEnd_=element+1;
        return true;
    }
    bool attach(Layers& layers,unsigned layer,std::uint32_t sourceId,InstanceHandle h,
        RunnerDepthService::Instance& state,bool inactive){
        DW_PROFILE_SCOPE(profileLayerAttach);
        if(!h.generation||h.slot==0xffffffffu||state.attached||!identityAvailable(h)||!layers.succeeded())return false;
        const auto* destination=layers.get(layer);if(!destination)return false;
        unsigned element=find(sourceId);const bool reused=element!=End;
        if(reused){
            if(entries_[element].instance.generation||layers.elementOwner(element)!=layer)return false;
        }else{element=free(layers);if(element==End)return false;}
        const RunnerImageValue depth=destination->encodedDepth;
        // Keep aligned identity fields as scalar stores on 68000; aggregate
        // construction otherwise expands these into individual byte writes.
        auto& entry=entries_[element];
        entry.sourceId=sourceId;
        entry.instance.slot=h.slot;
        entry.instance.generation=h.generation;
        if constexpr(!IdentitySlots)entry.present=true;
        indexIdentity(element);
        if(element>=usedEnd_)usedEnd_=element+1;
        state.attached=true;state.layerId=destination->id;state.depth=depth;
        if(!reused)return layers.attach(layer,element,true,state.append,[](unsigned){return false;});
        return !inactive||layers.reconcileInstanceActivation(element,true);
    }
    // Fresh unnumbered compact lifetime: the attached depth/ID are derived
    // from the owning layer until observed. Do not build a disposable ordinary
    // attachment record on success. Failure must publish exactly the record
    // which attachRoomFallback would have left before returning false.
    template<class CreateLayer,class Failure>
    bool attachFreshUnnumbered(Layers& layers,InstanceHandle h,RunnerImageValue depth,
        CreateLayer create,Failure failure){
        auto unattached=[&]{failure(depth,-1,false);return false;};
        if(!layers.succeeded())return unattached();
        std::int32_t integer;
        if(!RunnerDepthService::integerDepth(depth,integer))return unattached();
        unsigned layer=layers.findDynamic(integer);
        if(layer==Layers::End){
            if(!create(integer,layer))return unattached();
            const auto* created=layers.get(layer);
            if(!created||!created->dynamic||created->depth!=integer)return unattached();
        }
        if(!h.generation||h.slot==0xffffffffu||!identityAvailable(h)||!layers.succeeded())return unattached();
        const auto* destination=layers.get(layer);if(!destination)return unattached();
        const unsigned element=free(layers);if(element==End)return unattached();
        auto& entry=entries_[element];
        entry.sourceId=0;entry.instance.slot=h.slot;entry.instance.generation=h.generation;
        if constexpr(!IdentitySlots)entry.present=true;
        indexIdentity(element);
        if(element>=usedEnd_)usedEnd_=element+1;
        if(layers.attach(layer,element,true,false,[](unsigned){return false;}))return true;
        failure(destination->encodedDepth,destination->id,true);return false;
    }
    bool retire(Layers& layers,std::uint32_t sourceId,InstanceHandle h){
        const unsigned element=find(h);
        if(element==End||entries_[element].sourceId!=sourceId||!layers.detach(element))return false;
        unindexIdentity(element);entries_[element]=Entry{};return true;
    }
    // 0x41498b: uninitialize a bound type-2 element without deleting its
    // room membership. Source lookup, not the cached pointer, finds the
    // instance whose attachment fields are cleared. Depth is untouched.
    template<class Lookup>
    bool uninitialize(Layers& layers,unsigned element,Lookup lookup){
        if(!layers.succeeded()||!get(element))return false;
        auto& entry=entries_[element];
        if(!entry.instance.generation)return true;
        // An unnumbered lifetime cannot survive through the original global
        // numeric-ID lookup. Its caller must retire it before room teardown.
        if(!entry.sourceId)return false;
        if(entry.sourceId<=0x7fffffffu){
            InstanceHandle h;RunnerDepthService::Instance* state=nullptr;
            if(!lookup(entry.sourceId,h,state))return false;
            if(state){
                if(!h.generation||h.slot==0xffffffffu)return false;
                state->attached=false;state->layerId=-1;
            }
        }
        unindexIdentity(element);entry.instance={};return true;
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
        unindexIdentity(element);entries_[element]=Entry{};return true;
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
        if constexpr(IdentitySlots)if(h.slot>=IdentitySlots)return false;
        const auto id=destination->id;
        if(state->attached&&state->layerId!=-1&&!release(h,*state))return false;
        // A surviving same-room attachment would alias the lifetime. Release
        // must retire it before publication, without retiring the instance.
        if(!identityAvailable(h)||!get(element)||layers.elementOwner(element)!=layer||
            !layers.get(layer)||layers.get(layer)->id!=id)return false;
        entry.instance=h;indexIdentity(element);state->attached=true;state->layerId=id;
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
        DW_PROFILE_SCOPE(profileLayerResolve);
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
    static_assert(ElementCapacity<65535,"Compact identity links require 16-bit elements");
    static constexpr unsigned Buckets=ElementCapacity<16?16:ElementCapacity<64?64:256;
    static unsigned bucket(InstanceHandle h){
        return (h.slot+((h.generation<<5)|(h.generation>>27)))&(Buckets-1);
    }
    bool identityAvailable(InstanceHandle h)const{
        if constexpr(IdentitySlots)return h.slot<IdentitySlots&&!identityElement_[h.slot];
        else return find(h)==End;
    }
    void indexIdentity(unsigned element){
        if constexpr(IdentitySlots)identityElement_[entries_[element].instance.slot]=std::uint16_t(element+1);
        else{
            const unsigned b=bucket(entries_[element].instance);
            identityNext_[element]=identityHead_[b];identityHead_[b]=std::uint16_t(element+1);
        }
    }
    void unindexIdentity(unsigned element){
        if constexpr(IdentitySlots)identityElement_[entries_[element].instance.slot]=0;
        else{
            auto* link=&identityHead_[bucket(entries_[element].instance)];
            while(*link&&*link!=element+1)link=&identityNext_[*link-1];
            if(*link)*link=identityNext_[element];
            identityNext_[element]=0;
        }
    }
    unsigned free(const Layers& layers){
        DW_PROFILE_SCOPE(profileLayerAllocate);
        // Shared links account for non-instance owners too. Keep the registry
        // presence check for detached/unpublished records, as in the scan.
        for(unsigned slot=layers.firstFreeElement();slot!=End;slot=layers.firstFreeElement(slot+1))
            if(!present(entries_[slot]))return slot;
        return End;
    }
    static bool present(const Entry& entry){
        // A placeholder has a nonzero source ID. An unnumbered binding has
        // a nonzero generation. Retire/rebind clear both; failed attachment
        // retains the published handle just as the explicit flag did.
        if constexpr(IdentitySlots)return entry.sourceId||entry.instance.generation;
        else return entry.present;
    }
    Entry entries_[ElementCapacity?ElementCapacity:1]{};
    // Lookup acceleration only: allocation, source-ID lookup and painter
    // ordering still use the original scans/lists. Zero means no element.
    std::uint16_t identityHead_[IdentitySlots?1:Buckets]{},identityNext_[!IdentitySlots&&ElementCapacity?ElementCapacity:1]{};
    std::uint16_t identityElement_[IdentitySlots?IdentitySlots:1]{};
    // Source-ID search bound: slots below it may be holes or non-instance elements.
    // Allocation and painter order remain governed by the original scans/lists.
    unsigned usedEnd_=0;
};
}
#endif
