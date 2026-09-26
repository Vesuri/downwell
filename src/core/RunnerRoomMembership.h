#ifndef DOWNWELL_CORE_RUNNER_ROOM_MEMBERSHIP_H
#define DOWNWELL_CORE_RUNNER_ROOM_MEMBERSHIP_H
#include "RunnerSpatialSplit.h"
namespace downwell {
// Room lists in 0x48d630. Requests change external flags; reconciliation scans
// inactive order first, then active order, never the request queue's order.
template<unsigned Capacity> class RunnerRoomMembership {
    struct Entry {unsigned next=Capacity,previous=Capacity;int list=-1;RunnerImageValue key;};
public:
    static constexpr unsigned End=Capacity;
    bool contains(unsigned slot)const{return slot<Capacity && entries_[slot].list>=0;}
    bool bind(unsigned slot,RunnerImageValue key){
        if(slot>=Capacity || !RunnerSpatialSplit::valid(key))return false;
        remove(slot);entries_[slot].key=key;insertActive(slot);return true;
    }
    bool remove(unsigned slot){
        if(!contains(slot))return false;
        const auto e=entries_[slot];
        if(e.previous==End)head_[e.list]=e.next;else entries_[e.previous].next=e.next;
        if(e.next==End)tail_[e.list]=e.previous;else entries_[e.next].previous=e.previous;
        entries_[slot].next=entries_[slot].previous=End;entries_[slot].list=-1;return true;
    }
    void clear(){for(unsigned list=0;list<2;++list)while(head_[list]!=End)remove(head_[list]);}
    // Callbacks can publish external membership, but must not mutate this owner.
    template<class Inactive,class Publish> void reconcile(Inactive inactive,Publish publish){
        for(unsigned slot=head_[1];slot!=End;){
            const unsigned next=entries_[slot].next;
            if(!inactive(slot)){remove(slot);insertActive(slot);publish(slot,true);}
            slot=next;
        }
        for(unsigned slot=head_[0];slot!=End;){
            const unsigned next=entries_[slot].next;
            if(inactive(slot)){
                remove(slot);appendInactive(slot);publish(slot,false);
            }
            slot=next;
        }
    }
private:
    void insertActive(unsigned slot){
        auto& e=entries_[slot];unsigned previous=tail_[0];
        while(previous!=End && RunnerSpatialSplit::less(e.key,entries_[previous].key))
            previous=entries_[previous].previous;
        e.list=0;e.previous=previous;e.next=previous==End?head_[0]:entries_[previous].next;
        if(e.next==End)tail_[0]=slot;else entries_[e.next].previous=slot;
        if(previous==End)head_[0]=slot;else entries_[previous].next=slot;
    }
    void appendInactive(unsigned slot){
        auto& e=entries_[slot];e.list=1;e.previous=tail_[1];e.next=End;
        if(tail_[1]==End)head_[1]=slot;else entries_[tail_[1]].next=slot;
        tail_[1]=slot;
    }
    Entry entries_[Capacity?Capacity:1]{};
    unsigned head_[2]={End,End},tail_[2]={End,End};
};
}
#endif
