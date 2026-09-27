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
    bool active(unsigned slot)const{return slot<Capacity && entries_[slot].list==0;}
    // Animation's active-room traversal. Visitor may update instance fields,
    // but must not create/remove/rebind memberships while visiting this list.
    template<class Visit> bool visitAnimationChecked(Visit visit)const{
        for(unsigned slot=head_[0];slot!=End;slot=entries_[slot].next)
            if(!visit(slot))return false;
        return true;
    }
    // Ordinary Draw walks the SAME active list in the opposite direction:
    // room+0x84, next+0x150 (0x490f11), versus animation's room+0x80,
    // next+0x14c. Higher depths and newly inserted equal-depth peers go first.
    // The visitor owns visibility/destruction filtering. No list mutation.
    template<class Visit> bool visitDrawChecked(Visit visit)const{
        for(unsigned slot=tail_[0];slot!=End;slot=entries_[slot].previous)
            if(!visit(slot))return false;
        return true;
    }
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
    // Marked-record cleanup (0x48bbb0) differs from activation reconciliation:
    // active list first, inactive list second. The visitor may remove only its
    // current entry; save the successor before dispatch. No births/rebinding.
    template<class Visit> bool visitCleanupChecked(Visit visit){
        for(unsigned list=0;list<2;++list)for(unsigned slot=head_[list];slot!=End;){
            const unsigned next=entries_[slot].next;
            if(!visit(slot))return false;
            slot=next;
        }
        return true;
    }
    // Callbacks can publish external membership, but must not mutate this owner.
    template<class Inactive,class Publish> void reconcile(Inactive inactive,Publish publish){
        reconcileChecked(inactive,[&](unsigned slot,bool registered){
            publish(slot,registered);return true;
        });
    }
    // Failure is terminal for this reconciliation: prior moves/publications
    // are not rolled back. Do not invoke later callbacks after a failed owner.
    template<class Inactive,class Publish> bool reconcileChecked(Inactive inactive,Publish publish){
        for(unsigned slot=head_[1];slot!=End;){
            const unsigned next=entries_[slot].next;
            if(!inactive(slot)){
                remove(slot);insertActive(slot);
                if(!publish(slot,true))return false;
            }
            slot=next;
        }
        for(unsigned slot=head_[0];slot!=End;){
            const unsigned next=entries_[slot].next;
            if(inactive(slot)){
                remove(slot);appendInactive(slot);
                if(!publish(slot,false))return false;
            }
            slot=next;
        }
        return true;
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
