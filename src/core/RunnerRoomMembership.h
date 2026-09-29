#ifndef DOWNWELL_CORE_RUNNER_ROOM_MEMBERSHIP_H
#define DOWNWELL_CORE_RUNNER_ROOM_MEMBERSHIP_H
#include "RunnerSpatialSplit.h"
#include "ProfileCounters.h"
namespace downwell {
// Room lists in 0x48d630. Requests change external flags; reconciliation scans
// inactive order first, then active order, never the request queue's order.
template<unsigned Capacity> class RunnerRoomMembership {
    static_assert(Capacity<65534,"Compact cleanup links require 16-bit slots");
    static constexpr unsigned Unmarked=Capacity+1;
    struct Entry {std::uint16_t next=Capacity,previous=Capacity;int list=-1;RunnerImageValue key;
        std::uint16_t markedNext=Unmarked,markedPrevious=Capacity;};
public:
    static constexpr unsigned End=Capacity;
    struct AnimationProjection {
        static_assert(Capacity<65535,"Compact animation links require 16-bit slots");
        std::uint16_t next[Capacity?Capacity:1]{};
        const RunnerRoomMembership* owner=nullptr;
        unsigned revision=0,first=0,last=0;
    };
    // Prepare once at room entry; mutations repair only adjacent omitted runs.
    // This is owned storage, not a pointer to a caller's temporary projection.
    bool prepareRetainedAnimation(unsigned first,unsigned last)const{
        if(first>last||last>Capacity)return false;
        rebuildAnimation(retainedAnimation_,first,last);return true;
    }
    template<class Visit> bool visitAnimationRetainedChecked(
        unsigned first,unsigned last,bool& dirty,Visit visit)const{
        return visitAnimationProjectedChecked(retainedAnimation_,first,last,dirty,visit);
    }
    bool contains(unsigned slot)const{return slot<Capacity && entries_[slot].list>=0;}
    bool active(unsigned slot)const{return slot<Capacity && entries_[slot].list==0;}
    bool marked(unsigned slot)const{return contains(slot)&&entries_[slot].markedNext!=Unmarked;}
    // Destruction request order is NOT retirement order. Insert into a sparse
    // projection of the current room list; callbacks run only at Draw entry.
    bool mark(unsigned slot){
        if(!contains(slot))return false;
        if(marked(slot))return true;
        auto& e=entries_[slot];unsigned previous=e.previous;
        while(previous!=End&&!marked(previous))previous=entries_[previous].previous;
        const unsigned next=previous==End?markedHead_[e.list]:entries_[previous].markedNext;
        e.markedPrevious=std::uint16_t(previous);e.markedNext=std::uint16_t(next);
        if(previous==End)markedHead_[e.list]=slot;else entries_[previous].markedNext=std::uint16_t(slot);
        if(next!=End)entries_[next].markedPrevious=std::uint16_t(slot);
        return true;
    }
    // Animation's active-room traversal. Visitor may update instance fields,
    // but must not create/remove/rebind memberships while visiting this list.
    template<class Visit> bool visitAnimationChecked(Visit visit)const{
        for(unsigned slot=head_[0];slot!=End;slot=entries_[slot].next)
            if(!visit(slot))return false;
        return true;
    }
    // The omitted range only snapshots unchanged positions in the adapter.
    // A coordinate-write observer sets dirty, including temporary query moves.
    // Mid-dispatch writes resume the full list at the saved raw successor;
    // already visited slots retain their old snapshot until the next frame.
    template<class Visit> bool visitAnimationProjectedChecked(AnimationProjection& projection,
        unsigned first,unsigned last,bool& dirty,Visit visit)const{
        if(first>last||last>Capacity)return false;
        bool full=dirty;dirty=false;
        if(full){DW_PROFILE_COUNT(counterAnimationFullPass);}
        else{DW_PROFILE_COUNT(counterAnimationProjectedPass);}
        if(!full&&(projection.owner!=this||projection.revision!=revision_||
            projection.first!=first||projection.last!=last)){
            DW_PROFILE_COUNT(counterAnimationProjectionBuild);
            rebuildAnimation(projection,first,last);
        }
        for(unsigned slot=head_[0];slot!=End;){
            if(dirty)full=true;
            if(!full)slot=projection.next[slot];
            if(slot==End)break;
            const unsigned next=entries_[slot].next;
            if(!visit(slot))return false;
            slot=next;
        }
        return true;
    }
    // Legacy room-list Draw walks the SAME active list in the opposite direction:
    // room+0x84, next+0x150 (0x490f11), versus animation's room+0x80,
    // next+0x14c. Higher depths and newly inserted equal-depth peers go first.
    // This is NOT the layer path used when room+0xdc is positive (0x490db8).
    // Current adapters still use this pending layer-owner integration.
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
        const bool retained=retainedAnimationReady();
        if(retained&&e.list==0&&!animationOmitted(slot))
            repairAnimationBefore(e.previous,e.next==End?End:retainedAnimation_.next[e.next]);
        if(e.markedNext!=Unmarked){
            if(e.markedPrevious==End)markedHead_[e.list]=e.markedNext;
            else entries_[e.markedPrevious].markedNext=e.markedNext;
            if(e.markedNext!=End)entries_[e.markedNext].markedPrevious=e.markedPrevious;
            entries_[slot].markedNext=Unmarked;entries_[slot].markedPrevious=End;
        }
        if(e.previous==End)head_[e.list]=e.next;else entries_[e.previous].next=e.next;
        if(e.next==End)tail_[e.list]=e.previous;else entries_[e.next].previous=e.previous;
        entries_[slot].next=entries_[slot].previous=End;entries_[slot].list=-1;++revision_;
        if(retained)retainedAnimation_.revision=revision_;
        return true;
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
    // Same ordering as visitCleanupChecked filtered by mark(). Visitor may
    // remove only its current entry, and must not publish new marks or births.
    template<class Visit> bool visitMarkedCleanupChecked(Visit visit){
        for(unsigned list=0;list<2;++list)for(unsigned slot=markedHead_[list];slot!=End;){
            const unsigned next=entries_[slot].markedNext;
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
                const bool wasMarked=marked(slot);
                remove(slot);insertActive(slot);
                if(wasMarked&&!mark(slot))return false;
                if(!publish(slot,true))return false;
            }
            slot=next;
        }
        for(unsigned slot=head_[0];slot!=End;){
            const unsigned next=entries_[slot].next;
            if(inactive(slot)){
                const bool wasMarked=marked(slot);
                remove(slot);appendInactive(slot);
                if(wasMarked&&!mark(slot))return false;
                if(!publish(slot,false))return false;
            }
            slot=next;
        }
        return true;
    }
private:
    bool retainedAnimationReady()const{
        return retainedAnimation_.owner==this&&retainedAnimation_.revision==revision_;
    }
    bool animationOmitted(unsigned slot)const{
        return slot>=retainedAnimation_.first&&slot<retainedAnimation_.last;
    }
    void repairAnimationBefore(unsigned previous,unsigned target){
        for(unsigned slot=previous;slot!=End&&animationOmitted(slot);slot=entries_[slot].previous)
            retainedAnimation_.next[slot]=std::uint16_t(target);
    }
    void rebuildAnimation(AnimationProjection& projection,unsigned first,unsigned last)const{
        unsigned next=End;
        for(unsigned slot=tail_[0];slot!=End;slot=entries_[slot].previous){
            if(slot<first||slot>=last)next=slot;
            projection.next[slot]=std::uint16_t(next);
        }
        projection.owner=this;projection.revision=revision_;
        projection.first=first;projection.last=last;
    }
    void insertActive(unsigned slot){
        const bool retained=retainedAnimationReady();
        ++revision_;
        auto& e=entries_[slot];unsigned previous=tail_[0];
        unsigned forward=head_[0];
        // Find the same upper-bound position as the source tail scan, but
        // approach it from both ends. Equal keys stay BEFORE the new entry.
        // Foreground births must not walk past every retained terrain entry.
        while(previous!=End && RunnerSpatialSplit::less(e.key,entries_[previous].key)){
            if(RunnerSpatialSplit::less(e.key,entries_[forward].key)){
                previous=entries_[forward].previous;break;
            }
            forward=entries_[forward].next;
            previous=entries_[previous].previous;
        }
        e.list=0;e.previous=previous;e.next=previous==End?head_[0]:entries_[previous].next;
        if(e.next==End)tail_[0]=slot;else entries_[e.next].previous=slot;
        if(previous==End)head_[0]=slot;else entries_[previous].next=slot;
        if(retained){
            retainedAnimation_.next[slot]=std::uint16_t(animationOmitted(slot)?
                (e.next==End?End:retainedAnimation_.next[e.next]):slot);
            if(!animationOmitted(slot))repairAnimationBefore(previous,slot);
            retainedAnimation_.revision=revision_;
        }
    }
    void appendInactive(unsigned slot){
        auto& e=entries_[slot];e.list=1;e.previous=tail_[1];e.next=End;
        if(tail_[1]==End)head_[1]=slot;else entries_[tail_[1]].next=slot;
        tail_[1]=slot;
    }
    Entry entries_[Capacity?Capacity:1]{};
    unsigned head_[2]={End,End},tail_[2]={End,End};
    unsigned markedHead_[2]={End,End};
    unsigned revision_=0;
    mutable AnimationProjection retainedAnimation_;
};
}
#endif
