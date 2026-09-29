#ifndef DOWNWELL_CORE_RUNNER_LAYER_ELEMENTS_H
#define DOWNWELL_CORE_RUNNER_LAYER_ELEMENTS_H
#include "LowestFreeSlotMap.h"
namespace downwell {
// Selected GMS2 layer element links, independent of the room animation list.
// Slots are borrowed identities: their owner must detach before reuse. This
// primitive does not allocate layers, publish source IDs, or define Draw order.
template<unsigned Layers,unsigned Elements> class RunnerLayerElements {
    struct Links {unsigned next=Elements,previous=Elements,layer=Layers;};
    struct Layer {unsigned head=Elements,tail=Elements,count=0;};
public:
    static constexpr unsigned End=Elements,NoLayer=Layers;
    // Storage disposal only; caller has already unpublished all payloads.
    void clear(){for(auto& link:links_)link=Links{};for(auto& layer:layers_)layer=Layer{};free_.clear();}
    unsigned firstFree(unsigned start=0)const{return free_.first(start);}
    unsigned owner(unsigned element)const{return element<Elements?links_[element].layer:NoLayer;}
    unsigned count(unsigned layer)const{return layer<Layers?layers_[layer].count:0;}
    unsigned first(unsigned layer)const{return layer<Layers?layers_[layer].head:End;}
    unsigned successor(unsigned element)const{return element<Elements?links_[element].next:End;}
    unsigned predecessor(unsigned element)const{return element<Elements?links_[element].previous:End;}
    // 0x4128d0: explicit append wins; otherwise type-2 elements enter at
    // head, other types follow only the contiguous active-instance prefix.
    // activeInstance must reject non-type-2 and null instance references.
    // Predicates and raw visitors must not mutate these links. visitDraw has
    // a separate saved-successor contract for insertion during dispatch.
    template<class ActiveInstance> bool insert(unsigned layer,unsigned element,
        bool instanceElement,bool append,ActiveInstance activeInstance){
        if(layer>=Layers || element>=Elements || owner(element)!=NoLayer)return false;
        unsigned previous=End;
        if(append)previous=layers_[layer].tail;
        else if(!instanceElement)
            for(unsigned i=layers_[layer].head;i!=End && activeInstance(i);i=links_[i].next)
                previous=i;
        insertAfter(layer,element,previous);return true;
    }
    bool remove(unsigned element){
        if(element>=Elements || owner(element)==NoLayer)return false;
        const Links e=links_[element];auto& layer=layers_[e.layer];
        if(e.previous==End)layer.head=e.next;else links_[e.previous].next=e.next;
        if(e.next==End)layer.tail=e.previous;else links_[e.next].previous=e.previous;
        --layer.count;links_[element]=Links{};free_.release(element);return true;
    }
    // 0x41487f: append the whole source chain, preserving its order and
    // updating element ownership. Source layer disposal belongs to the caller.
    bool concatenate(unsigned destination,unsigned source){
        if(destination>=Layers || source>=Layers || destination==source)return false;
        auto& from=layers_[source];auto& to=layers_[destination];
        if(from.head==End)return true;
        for(unsigned i=from.head;i!=End;i=links_[i].next)links_[i].layer=destination;
        links_[from.head].previous=to.tail;
        if(to.tail==End)to.head=from.head;else links_[to.tail].next=from.head;
        to.tail=from.tail;to.count+=from.count;from=Layer{};return true;
    }
    // 0x426630: walk backwards using saved predecessors, moving inactive
    // instance elements to the tail. This reverses their forward order; it is
    // NOT a stable partition. Null/non-instance elements must return false.
    template<class InactiveInstance> bool normalize(unsigned layer,InactiveInstance inactiveInstance){
        if(layer>=Layers)return false;
        for(unsigned i=layers_[layer].tail;i!=End;){
            const unsigned previous=links_[i].previous;
            if(inactiveInstance(i)){remove(i);insertAfter(layer,i,layers_[layer].tail);}
            i=previous;
        }
        return true;
    }
    // Raw link order only, not a claim about the runner's selected Draw walk.
    template<class Visit> bool visitForward(unsigned layer,Visit visit)const{
        if(layer>=Layers)return false;
        for(unsigned i=layers_[layer].head;i!=End;i=links_[i].next)if(!visit(i))return false;
        return true;
    }
    // Draw (0x4904c6): save the successor BEFORE dispatch. Insertions are
    // observed only when a later visited element reads its own successor.
    // The owner must keep the current layer and saved successor alive; this
    // does not establish safe retirement/reuse semantics for Draw callbacks.
    template<class Visit> bool visitDraw(unsigned layer,Visit visit)const{
        if(layer>=Layers)return false;
        for(unsigned i=layers_[layer].head;i!=End;){
            const unsigned next=links_[i].next;
            if(!visit(i))return false;
            i=next;
        }
        return true;
    }
    template<class Visit> bool visitBackward(unsigned layer,Visit visit)const{
        if(layer>=Layers)return false;
        for(unsigned i=layers_[layer].tail;i!=End;i=links_[i].previous)if(!visit(i))return false;
        return true;
    }
private:
    void insertAfter(unsigned layer,unsigned element,unsigned previous){
        free_.occupy(element);
        auto& l=layers_[layer];auto& e=links_[element];
        e.layer=layer;e.previous=previous;e.next=previous==End?l.head:links_[previous].next;
        if(e.next==End)l.tail=element;else links_[e.next].previous=element;
        if(previous==End)l.head=element;else links_[previous].next=element;
        ++l.count;
    }
    Links links_[Elements?Elements:1]{};
    Layer layers_[Layers?Layers:1]{};
    LowestFreeSlotMap<Elements> free_;
};
}
#endif
