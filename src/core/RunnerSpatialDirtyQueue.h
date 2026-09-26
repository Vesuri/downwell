#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_DIRTY_QUEUE_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_DIRTY_QUEUE_H
#include <cstdint>
namespace downwell {
// 0x501e50: remove existing membership, then prepend. Slot indices are owned
// by the instance registry, not resource IDs. Retire membership before reuse.
// No simulation or callback scheduling is implied by this container.
template<unsigned Capacity> class RunnerSpatialDirtyQueue {
    static_assert(Capacity>0,"Dirty queue needs instance slots");
    struct Link {unsigned next=Capacity,previous=Capacity;bool queued=false;};
public:
    static constexpr unsigned End=Capacity;
    unsigned first()const{return head_;}
    unsigned size()const{return count_;}
    unsigned next(unsigned slot)const{return contains(slot)?links_[slot].next:End;}
    bool contains(unsigned slot)const{return slot<Capacity && links_[slot].queued;}
    void clear(){while(head_!=End)remove(head_);}
    bool mark(unsigned slot){
        if(slot>=Capacity)return false;
        remove(slot);
        auto& link=links_[slot];link.next=head_;link.previous=End;link.queued=true;
        if(head_!=End)links_[head_].previous=slot;
        head_=slot;++count_;return true;
    }
    bool remove(unsigned slot){
        if(!contains(slot))return false;
        auto& link=links_[slot];
        if(link.previous==End)head_=link.next;else links_[link.previous].next=link.next;
        if(link.next!=End)links_[link.next].previous=link.previous;
        link=Link{};--count_;return true;
    }
    // Callers must separately set the runner's bounds-dirty bit when a
    // position/image/mask mutation requires it. Queue eligibility is distinct.
    static bool shouldQueue(bool hasObject,std::uint32_t objectFlags,std::uint32_t instanceFlags){
        return hasObject && ((objectFlags&0x28u) || (instanceFlags&0x20u));
    }
    // Audited sprite-bound path of 0x502ca0 only. Recalculation/fallback mask
    // handling and source object-flag construction remain scheduler work.
    static bool shouldUpdateSpriteIndex(std::uint32_t objectFlags,std::uint32_t instanceFlags,bool allPrepared){
        return !(instanceFlags&0x100003u) && (objectFlags&0x10u) &&
            (allPrepared || (objectFlags&0x28u) || (instanceFlags&0x20u));
    }
private:
    Link links_[Capacity]={};unsigned head_=End,count_=0;
};
}
#endif
