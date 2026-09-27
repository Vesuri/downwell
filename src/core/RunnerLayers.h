#ifndef DOWNWELL_CORE_RUNNER_LAYERS_H
#define DOWNWELL_CORE_RUNNER_LAYERS_H
#include "RunnerLayerElements.h"
#include <cstdint>
namespace downwell {
// Selected GMS2 layer storage and integer-depth movement. Source IDs are
// supplied by the room owner, not invented from reusable storage slots.
// Does not service the pending instance-depth queue or define Draw traversal.
template<unsigned Capacity,unsigned Elements> class RunnerLayers {
public:
    static constexpr unsigned End=Capacity;
    struct Layer {
        std::int32_t id=0,depth=0;
        unsigned previous=End,next=End;
        bool dynamic=false,present=false;
    };
    const Layer* get(unsigned slot)const{return slot<Capacity&&layers_[slot].present?&layers_[slot]:nullptr;}
    bool succeeded()const{return !failed_;}
    unsigned find(std::int32_t id)const{
        for(unsigned i=head_;i!=End;i=layers_[i].next)if(layers_[i].id==id)return i;
        return End;
    }
    unsigned findDynamic(std::int32_t depth)const{
        for(unsigned i=head_;i!=End;i=layers_[i].next){
            if(layers_[i].depth>depth)break;
            if(layers_[i].depth==depth&&layers_[i].dynamic)return i;
        }
        return End;
    }
    // 0x412820 dynamic creation and 0x4143f6 serialized room-layer insertion:
    // after all equal-depth layers. Call in source creation/serialized order.
    bool insertAfterEquals(std::int32_t id,std::int32_t depth,bool dynamic,unsigned& slot){
        if(failed_||find(id)!=End)return false;
        unsigned free=0;while(free<Capacity&&layers_[free].present)++free;
        if(free==Capacity)return false;
        unsigned previous=End;
        for(unsigned i=head_;i!=End&&layers_[i].depth<=depth;i=layers_[i].next)previous=i;
        layers_[free]={id,depth,End,End,dynamic,true};linkAfter(free,previous);slot=free;return true;
    }
    unsigned elementOwner(unsigned element)const{return elements_.owner(element);}
    unsigned elementCount(unsigned layer)const{return elements_.count(layer);}
    template<class Active> bool attach(unsigned layer,unsigned element,bool instance,bool append,Active active){
        return !failed_&&get(layer)&&elements_.insert(layer,element,instance,append,active);
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
        if(layers_[layer].dynamic&&!elements_.count(layer))eraseEmpty(layer);
        return true;
    }
    // Serialized element rebinding (0x412f32): unlink the old element, but
    // keep its layer even when dynamic and empty. Instance retirement and
    // pending-depth state are deliberately not part of this operation.
    bool detachForRebind(unsigned element){
        return !failed_&&get(elements_.owner(element))&&elements_.remove(element);
    }
    // Room initialization (0x41363e): forward layer traversal, then the same
    // backward inactive-to-tail pass used after merging. Invoke once after
    // serialized initialization and global-instance fallback, before Draw.
    // Predicate must reject null/non-instance elements and must not mutate
    // owners. This is deliberately NOT an idempotent stable partition.
    template<class Inactive> bool normalizeRoomInstances(Inactive inactive){
        if(failed_)return false;
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
        if(failed_||!get(slot))return false;
        auto& layer=layers_[slot];const auto old=layer.depth;
        if(depth==old)return true;
        layer.depth=depth;
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
                if(!elements_.visitForward(slot,[&](unsigned e){return publish(e,id);})){failed_=true;return false;}
                if(!elements_.concatenate(next,slot)||!elements_.normalize(next,inactive)){failed_=true;return false;}
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
private:
    void eraseEmpty(unsigned slot){unlink(slot);layers_[slot]=Layer{};}
    void unlink(unsigned slot){
        const auto& l=layers_[slot];
        if(l.previous==End)head_=l.next;else layers_[l.previous].next=l.next;
        if(l.next==End)tail_=l.previous;else layers_[l.next].previous=l.previous;
    }
    void linkAfter(unsigned slot,unsigned previous){
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
