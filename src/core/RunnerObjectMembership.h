#ifndef DOWNWELL_CORE_RUNNER_OBJECT_MEMBERSHIP_H
#define DOWNWELL_CORE_RUNNER_OBJECT_MEMBERSHIP_H
#include <cstdint>
namespace downwell {
// 0x478380/0x479880: append registration, stable unlink. With an immutable
// object-parent graph, filtering this common registration list gives exactly
// the relative order of each own/ancestor-inclusive list. It is NOT Draw order.
// Slots belong to an external lifetime registry. The opaque identity is not
// interpreted as a resource ID. Active flags and phase boundaries are external:
// deactivation requests must not eagerly unbind before source reconciliation.
template<unsigned Capacity> class RunnerObjectMembership {
public:
    static constexpr unsigned End = Capacity;
    bool contains(unsigned slot) const {return slot<Capacity && entries_[slot].object>=0;}
    unsigned size() const {return count_;}
    // Borrow the current registration sequence without permitting mutation.
    // Exact-object Step ordering is applied after capture, not by traversing
    // ancestor-inclusive collision lists.
    template<class Visit> bool visitRegistrations(Visit visit)const{
        return visitRegistrationRange(0,Capacity,visit);
    }
    // Slot ranges select an internal owner domain, never traversal order.
    // Retain registration order across rebinds; omitted slots invoke no
    // callback and need no caller-side record resolution.
    template<class Visit> bool visitRegistrationRange(unsigned first,unsigned last,Visit visit)const{
        if(first>last||last>Capacity)return false;
        if(first==last)return true;
        for(unsigned slot=head_;slot!=End;slot=entries_[slot].next){
            if(slot<first||slot>=last)continue;
            const auto& e=entries_[slot];
            if(!visit(slot,e.identity,e.object))return false;
        }
        return true;
    }
    void clear() {while(head_!=End)unbind(head_);}
    bool bind(unsigned slot,int object,std::uint32_t identity) {
        if(slot>=Capacity || object<0)return false;
        unbind(slot);
        auto& entry=entries_[slot];
        entry.object=object;entry.identity=identity;entry.previous=tail_;entry.next=End;
        if(tail_!=End)entries_[tail_].next=slot;else head_=slot;
        tail_=slot;++count_;return true;
    }
    bool unbind(unsigned slot) {
        if(!contains(slot))return false;
        const auto entry=entries_[slot];
        if(entry.previous==End)head_=entry.next;else entries_[entry.previous].next=entry.next;
        if(entry.next==End)tail_=entry.previous;else entries_[entry.next].previous=entry.previous;
        entries_[slot]=Entry{};--count_;return true;
    }
    // isA(child,parent) uses the immutable resource graph; exact matches need
    // no callback. visit(slot,identity) returns false to stop at the first hit.
    // Callbacks must not mutate this registry. Missing/unsupported graphs must
    // be rejected by the owner before querying, not silently treated as no hit.
    template<class IsA,class Visit> void visit(int object,IsA isA,Visit callback) const {
        if(object<0)return;
        for(unsigned slot=head_;slot!=End;slot=entries_[slot].next){
            const auto& entry=entries_[slot];
            if((entry.object==object || isA(entry.object,object)) && !callback(slot,entry.identity))return;
        }
    }
    template<class IsA> unsigned count(int object,IsA isA) const {
        unsigned result=0;
        visit(object,isA,[&](unsigned,std::uint32_t){++result;return true;});
        return result;
    }
private:
    struct Entry {int object=-1;std::uint32_t identity=0;unsigned next=End,previous=End;};
    Entry entries_[Capacity ? Capacity : 1]{};
    unsigned head_=End,tail_=End,count_=0;
};
}
#endif
