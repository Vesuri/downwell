#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_DIRTY_QUEUE_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_DIRTY_QUEUE_H
#include <cstdint>
#include "SlotIndex.h"
namespace downwell {
// 0x501e50: remove existing membership, then prepend. Slot indices are owned
// by the instance registry, not resource IDs. Retire membership before reuse.
// Refresh callbacks bind this container to the instance registry.
template<unsigned Capacity> class RunnerSpatialDirtyQueue {
    static_assert(Capacity>0,"Dirty queue needs instance slots");
    using Index=typename SlotIndex<Capacity>::Type;
    struct Link {Index next=Capacity,previous=Capacity;};
public:
    static constexpr unsigned End=Capacity;
    unsigned first()const{return head_;}
    unsigned size()const{return count_;}
    unsigned next(unsigned slot)const{return contains(slot)?links_[slot].next:End;}
    // Only the head may be queued without a predecessor. Removed slots have
    // both links reset, including the singleton that used to be the head.
    bool contains(unsigned slot)const{return slot<Capacity && (slot==head_||links_[slot].previous!=End);}
    void clear(){while(head_!=End)remove(head_);}
    bool mark(unsigned slot){
        if(slot>=Capacity)return false;
        // Removing and prepending the current head leaves every link and the
        // count unchanged. Consecutive setters need no queue writes here.
        if(slot==head_)return true;
        remove(slot);
        auto& link=links_[slot];link.next=Index(head_);link.previous=End;
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
    static bool shouldRefreshQueued(std::uint32_t instanceFlags){
        return !(instanceFlags&0x100003u);
    }
    // 0x502ca0: inspect flags, refresh, read successor, then unlink. In the
    // ordinary sprite path refresh MUST calculate bounds with update=1, even
    // when bit 8 is clear or publication is ineligible. Special resources need
    // their own refresh/publication path; reject unsupported ones.
    // Callbacks may reorder other queued slots, but must not retire/reuse the
    // current slot. New heads are not revisited automatically. On failure the
    // current membership remains; prior updates are not rolled back. The owner
    // must latch failure rather than retry a partially mutated simulation.
    template<class Flags,class Refresh> bool flush(Flags flags,Refresh refresh){
        unsigned slot=head_;
        while(slot!=End){
            if(shouldRefreshQueued(flags(slot)) && !refresh(slot))return false;
            if(!contains(slot))return false;
            const unsigned successor=next(slot);
            remove(slot);slot=successor;
        }
        return true;
    }
    // Direct bounds publication (0x5020a0) has no outer dirty-pass skip:
    // membership bit 0x8000 bypasses the 0x100003 exclusion.
    static bool shouldPublishBounds(std::uint32_t objectFlags,std::uint32_t instanceFlags,bool allPrepared){
        return ((instanceFlags&0x8000u) || !(instanceFlags&0x100003u)) &&
            (objectFlags&0x10u) && (allPrepared || (objectFlags&0x28u) || (instanceFlags&0x20u));
    }
private:
    Link links_[Capacity]={};unsigned head_=End,count_=0;
};
}
#endif
