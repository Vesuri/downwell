#ifndef DOWNWELL_RUNNER_STEP_SNAPSHOT_H
#define DOWNWELL_RUNNER_STEP_SNAPSHOT_H
#include "InstanceHandle.h"
namespace downwell {
// Capture registrations before dispatch, then order exact objects by resource
// ID while retaining their registration order. The basic sorter uses no heap,
// recursion or resource ID-sized array; hosts can opt into dense sorting.
// Owners must recheck flags and lifetime at every dispatch.
// Storage is 16 bytes per entry on host/68000; large native owners must keep
// this workspace off the task stack. See verify-runner-step-order.py.
template<unsigned Capacity> class RunnerStepSnapshot {
    struct Entry {InstanceHandle handle;int object;unsigned order;};
    Entry entries_[Capacity?Capacity:1];unsigned count_=0;
    static bool less(const Entry& a,const Entry& b){
        return a.object<b.object||(a.object==b.object&&a.order<b.order);
    }
    void swap(unsigned a,unsigned b){const auto value=entries_[a];entries_[a]=entries_[b];entries_[b]=value;}
    void sift(unsigned root,unsigned size){
        while(root<size/2){
            unsigned child=root*2+1;
            if(child+1<size&&less(entries_[child],entries_[child+1]))++child;
            if(!less(entries_[root],entries_[child]))break;
            swap(root,child);root=child;
        }
    }
public:
    void clear(){count_=0;}
    bool append(unsigned slot,unsigned generation,int object){
        if(count_==Capacity||!generation||object<0)return false;
        entries_[count_]={{slot,generation},object,count_};++count_;return true;
    }
    void sort(){
        for(unsigned i=count_/2;i;--i)sift(i-1,count_);
        for(unsigned i=count_;i>1;--i){swap(0,i-1);sift(0,i-1);}
    }
    // Host stored-world fast path: resource IDs are dense and small. Stable
    // counting sort preserves registration order without four O(n log n)
    // sorts per tick. Call immediately after capture (append order), not on
    // a reordered snapshot. Unusual resource IDs retain the general sorter.
    // Explicitly opt-in: this temporary workspace is NOT for the Amiga task
    // stack. Native owners continue to use the constant-workspace sort().
    template<unsigned ObjectLimit> void sortDenseObjects(){
        static_assert(ObjectLimit > 0,"nonempty object range");
        if(count_<64){sort();return;}
        for(unsigned i=0;i<count_;++i)
            if(static_cast<unsigned>(entries_[i].object)>=ObjectLimit){sort();return;}
        unsigned offsets[ObjectLimit]{};
        for(unsigned i=0;i<count_;++i)++offsets[entries_[i].object];
        unsigned total=0;
        for(unsigned object=0;object<ObjectLimit;++object){
            const unsigned count=offsets[object];offsets[object]=total;total+=count;
        }
        Entry ordered[Capacity?Capacity:1];
        for(unsigned i=0;i<count_;++i)ordered[offsets[entries_[i].object]++]=entries_[i];
        for(unsigned i=0;i<count_;++i)entries_[i]=ordered[i];
    }
    unsigned size()const{return count_;}
    int object(unsigned i)const{return i<count_?entries_[i].object:-1;}
    InstanceHandle handle(unsigned i)const{return i<count_?entries_[i].handle:InstanceHandle{};}
};
}
#endif
