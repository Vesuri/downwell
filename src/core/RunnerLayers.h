#ifndef DOWNWELL_CORE_RUNNER_LAYERS_H
#define DOWNWELL_CORE_RUNNER_LAYERS_H
#include "RunnerLayerElements.h"
#include "RunnerImageValue.h"
#include <cstdint>
namespace downwell {
template<unsigned Capacity,bool Enabled> struct RunnerRetainedLayerPopulation {};
template<unsigned Capacity> struct RunnerRetainedLayerPopulation<Capacity,true> {
    unsigned retained[Capacity?Capacity:1]{};
    unsigned logical=0;
};
// Selected GMS2 layer storage and integer-depth movement. Source IDs are
// supplied by the room owner, not invented from reusable storage slots.
// Does not service the pending instance-depth queue or define Draw traversal.
template<unsigned Capacity,unsigned Elements,bool Retained=false> class RunnerLayers:
    private RunnerRetainedLayerPopulation<Capacity,Retained> {
public:
    static constexpr unsigned End=Capacity;
    struct Layer {
        std::int32_t id=0,depth=0;
        unsigned previous=End,next=End;
        bool dynamic=false,present=false;
        // Resource/mutation-time preparation; every attachment uses the same
        // nearest-even binary32 encoding until this layer's depth changes.
        RunnerImageValue encodedDepth{};
    };
    const Layer* get(unsigned slot)const{return slot<Capacity&&layers_[slot].present?&layers_[slot]:nullptr;}
    bool succeeded()const{return !failed_;}
    bool discardUninitialized(){
        if(failed_)return false;
        invalidateLookups();
        for(auto& layer:layers_)layer=Layer{};
        elements_.clear();head_=tail_=End;
        if constexpr(Retained){for(auto& count:this->retained)count=0;this->logical=0;}
        return true;
    }
    unsigned find(std::int32_t id)const{
        if(idLookup_.valid&&idLookup_.key==id)return idLookup_.slot;
        unsigned slot=End;
        for(unsigned i=head_;i!=End;i=layers_[i].next)if(layers_[i].id==id){slot=i;break;}
        idLookup_.key=id;idLookup_.slot=slot;idLookup_.valid=true;return slot;
    }
    unsigned findDynamic(std::int32_t depth)const{
        if(depthLookup_.valid&&depthLookup_.key==depth)return depthLookup_.slot;
        unsigned slot=End;
        for(unsigned i=head_;i!=End;i=layers_[i].next){
            if(layers_[i].depth>depth)break;
            if(layers_[i].depth==depth&&layers_[i].dynamic){slot=i;break;}
        }
        depthLookup_.key=depth;depthLookup_.slot=slot;depthLookup_.valid=true;return slot;
    }
    // 0x412820 dynamic creation and 0x4143f6 serialized room-layer insertion:
    // after all equal-depth layers. Call in source creation/serialized order.
    bool insertAfterEquals(std::int32_t id,std::int32_t depth,bool dynamic,unsigned& slot){
        if(failed_||find(id)!=End)return false;
        unsigned free=0;while(free<Capacity&&layers_[free].present)++free;
        if(free==Capacity)return false;
        unsigned previous=End;
        for(unsigned i=head_;i!=End&&layers_[i].depth<=depth;i=layers_[i].next)previous=i;
        layers_[free]={id,depth,End,End,dynamic,true};
        layers_[free].encodedDepth.assignInteger(depth);
        linkAfter(free,previous);slot=free;return true;
    }
    unsigned elementOwner(unsigned element)const{return elements_.owner(element);}
    unsigned firstFreeElement(unsigned start=0)const{
        if constexpr(Retained)if(this->logical==Elements)return Elements;
        return elements_.firstFree(start);
    }
    unsigned elementCount(unsigned layer)const{
        if constexpr(Retained)return elements_.count(layer)+(layer<Capacity?this->retained[layer]:0);
        else return elements_.count(layer);
    }
    // Retained records have logical population but no ordinary element link.
    // Their authoritative owner supplies order, flags and identity separately.
    // Admission consumes the SAME bounded population as ordinary elements.
    unsigned retainedCount(unsigned layer)const{
        if constexpr(Retained)return layer<Capacity?this->retained[layer]:0;
        else return 0;
    }
    bool retain(unsigned layer,unsigned count){
        static_assert(Retained,"Retained population is opt-in");
        if(failed_||!get(layer)||count>Elements-this->logical)return false;
        this->retained[layer]+=count;this->logical+=count;return true;
    }
    unsigned retainedAvailable(unsigned layer)const{
        static_assert(Retained,"Retained population is opt-in");
        return failed_||!get(layer)?0:Elements-this->logical;
    }
    bool releaseRetained(unsigned layer,unsigned count){
        static_assert(Retained,"Retained population is opt-in");
        if(failed_||!get(layer)||count>this->retained[layer])return false;
        this->retained[layer]-=count;this->logical-=count;
        if(count&&layers_[layer].dynamic&&!elementCount(layer))eraseEmpty(layer);
        return true;
    }
    template<class Active> bool attach(unsigned layer,unsigned element,bool instance,bool append,Active active){
        if constexpr(Retained)if(this->logical==Elements)return false;
        const bool ok=!failed_&&get(layer)&&elements_.insert(layer,element,instance,append,active);
        if constexpr(Retained)if(ok)++this->logical;
        return ok;
    }
    // 0x4266e0, called in room reconciliation order: remove/reinsert the SAME
    // element at head (active) or tail (inactive). Do not use detach here:
    // a singleton dynamic layer must survive this temporary empty list.
    // Adapter checks attached/type/identity before invoking this operation.
    bool reconcileInstanceActivation(unsigned element,bool inactive){
        if(failed_)return false;
        const unsigned layer=elements_.owner(element);
        return get(layer)&&elements_.remove(element)&&
            elements_.insert(layer,element,true,inactive,[](unsigned){return false;});
    }
    // Hash removal and external instance/current-element publication belong to
    // the adapter. Empty dynamic layers are retired; fixed layers remain.
    bool detach(unsigned element){
        if(failed_)return false;
        const unsigned layer=elements_.owner(element);
        if(!get(layer)||!elements_.remove(element))return false;
        if constexpr(Retained)--this->logical;
        if(layers_[layer].dynamic&&!elementCount(layer))eraseEmpty(layer);
        return true;
    }
    // Serialized element rebinding (0x412f32): unlink the old element, but
    // keep its layer even when dynamic and empty. Instance retirement and
    // pending-depth state are deliberately not part of this operation.
    bool detachForRebind(unsigned element){
        if(failed_||!get(elements_.owner(element))||!elements_.remove(element))return false;
        if constexpr(Retained)--this->logical;
        return true;
    }
    // Room initialization (0x41363e): forward layer traversal, then the same
    // backward inactive-to-tail pass used after merging. Invoke once after
    // serialized initialization and global-instance fallback, before Draw.
    // Predicate must reject null/non-instance elements and must not mutate
    // owners. This is deliberately NOT an idempotent stable partition.
    template<class Inactive> bool normalizeRoomInstances(Inactive inactive){
        if(failed_)return false;
        // A caller with virtual records must provide their logical ordering;
        // silently normalizing only physical entries would lose cutoffs.
        if constexpr(Retained)for(unsigned i=0;i<Capacity;++i)if(this->retained[i])return false;
        for(unsigned i=head_;i!=End;i=layers_[i].next)
            if(!elements_.normalize(i,inactive)){failed_=true;return false;}
        return true;
    }
    // 0x4147b0: merge is only possible while INCREASING depth. Publication
    // sees source element order and the destination ID before list splicing.
    // publish must filter type-2/null references and write instance layer IDs.
    // Failed publication is terminal, preserving already-published changes.
    template<class Publish,class Inactive> bool move(unsigned slot,std::int32_t depth,
        bool merge,Publish publish,Inactive inactive){
        return moveLogical(slot,depth,merge,[&](unsigned source,int id){
            if(retainedCount(source))return false;
            return elements_.visitForward(source,[&](unsigned e){return publish(e,id);});
        },[&](unsigned destination){
            if(retainedCount(destination))return false;
            return elements_.normalize(destination,inactive);
        });
    }
    // A retained owner publishes ALL logical instance IDs in source order,
    // then normalizes ALL logical elements after concatenation. The callbacks
    // own retained order/cutoffs; physical entries are exposed by visitElements
    // and normalizePhysicalInstances. Do not infer order from population alone.
    template<class PublishLogical,class NormalizeLogical> bool moveLogical(unsigned slot,std::int32_t depth,
        bool merge,PublishLogical publishLogical,NormalizeLogical normalizeLogical){
        if(failed_||!get(slot))return false;
        auto& layer=layers_[slot];const auto old=layer.depth;
        if(depth==old)return true;
        invalidateLookups();
        layer.depth=depth;
        layer.encodedDepth.assignInteger(depth);
        if(depth<old){
            unsigned previous=layer.previous;
            while(previous!=End&&layers_[previous].depth>=depth)previous=layers_[previous].previous;
            if(previous!=layer.previous){unlink(slot);linkAfter(slot,previous);}
            return true;
        }
        unsigned next=layer.next;
        while(next!=End&&layers_[next].depth<=depth){
            if(layers_[next].depth==depth&&merge&&layer.dynamic&&layers_[next].dynamic){
                const auto id=layers_[next].id;
                if(!publishLogical(slot,id)){failed_=true;return false;}
                if(!elements_.concatenate(next,slot)){failed_=true;return false;}
                if constexpr(Retained){this->retained[next]+=this->retained[slot];this->retained[slot]=0;}
                if(!normalizeLogical(next)){failed_=true;return false;}
                eraseEmpty(slot);return true;
            }
            next=layers_[next].next;
        }
        if(next!=layer.next){
            unlink(slot);
            linkAfter(slot,next==End?tail_:layers_[next].previous);
        }
        return true;
    }
    template<class Inactive> bool normalizePhysicalInstances(unsigned layer,Inactive inactive){
        return !failed_&&get(layer)&&elements_.normalize(layer,inactive);
    }
    template<class NormalizeLogical> bool normalizeRoomLogical(NormalizeLogical normalize){
        if(failed_)return false;
        for(unsigned i=head_;i!=End;i=layers_[i].next)
            if(!normalize(i)){failed_=true;return false;}
        return true;
    }
    template<class Visit> bool visitLayers(Visit visit)const{
        for(unsigned i=head_;i!=End;i=layers_[i].next)if(!visit(i,layers_[i]))return false;
        return true;
    }
    template<class Visit> bool visitLayersReverse(Visit visit)const{
        for(unsigned i=tail_;i!=End;i=layers_[i].previous)if(!visit(i,layers_[i]))return false;
        return true;
    }
    template<class Visit> bool visitElements(unsigned layer,Visit visit)const{
        return get(layer)&&elements_.visitForward(layer,visit);
    }
    template<class Visit> bool visitDrawElements(unsigned layer,Visit visit)const{
        return get(layer)&&elements_.visitDraw(layer,visit);
    }
    unsigned firstDrawElement(unsigned layer)const{return elements_.first(layer);}
    unsigned nextDrawElement(unsigned element)const{return elements_.successor(element);}
    unsigned previousDrawElement(unsigned element)const{return elements_.predecessor(element);}
    template<class Visit> bool visitElementsBackward(unsigned layer,Visit visit)const{
        return get(layer)&&elements_.visitBackward(layer,visit);
    }
private:
    // Last-query caches accelerate repeated births at one prepared depth.
    // Invalidate before depth publication (callbacks can query mid-move) and
    // on every topology change. Slot numbers are private storage, not IDs.
    struct Lookup {std::int32_t key=0;unsigned slot=End;bool valid=false;};
    mutable Lookup idLookup_,depthLookup_;
    void invalidateLookups(){idLookup_.valid=depthLookup_.valid=false;}
    void eraseEmpty(unsigned slot){unlink(slot);layers_[slot]=Layer{};}
    void unlink(unsigned slot){
        invalidateLookups();
        const auto& l=layers_[slot];
        if(l.previous==End)head_=l.next;else layers_[l.previous].next=l.next;
        if(l.next==End)tail_=l.previous;else layers_[l.next].previous=l.previous;
    }
    void linkAfter(unsigned slot,unsigned previous){
        invalidateLookups();
        auto& l=layers_[slot];l.previous=previous;l.next=previous==End?head_:layers_[previous].next;
        if(l.next==End)tail_=slot;else layers_[l.next].previous=slot;
        if(previous==End)head_=slot;else layers_[previous].next=slot;
    }
    Layer layers_[Capacity?Capacity:1]{};
    RunnerLayerElements<Capacity,Elements> elements_;
    unsigned head_=End,tail_=End;
    bool failed_=false;
};
}
#endif
