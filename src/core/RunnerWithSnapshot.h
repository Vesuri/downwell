#ifndef DOWNWELL_CORE_RUNNER_WITH_SNAPSHOT_H
#define DOWNWELL_CORE_RUNNER_WITH_SNAPSHOT_H
#include "InstanceHandle.h"
namespace downwell {
// Object pushenv, not outer Step order. Caller supplies eligible ordinary
// registrations, then eligible pending-object and pending-activation entries.
// Admission (object matching and flags) occurs during capture, never visit.
// Pending entries reappend existing lifetimes without changing inline order.
// See verify-runner-with.py. Bounded storage; no heap or resource-sized arrays.
template<unsigned Capacity> class RunnerWithSnapshot {
    InstanceHandle entries_[Capacity?Capacity:1]{},inline_[2]{};
    unsigned count_=0;bool valid_=true,sealed_=false;
    static bool equal(InstanceHandle a,InstanceHandle b){
        return a.slot==b.slot&&a.generation==b.generation;
    }
    bool fail(){valid_=false;return false;}
public:
    bool append(InstanceHandle key){
        if(!valid_||sealed_||!key.generation)return fail();
        unsigned at=0;while(at<count_&&!equal(entries_[at],key))++at;
        if(at==count_){
            if(count_==Capacity)return fail();
            if(count_<2)inline_[count_]=key;
            entries_[count_++]=key;
        }else{
            for(unsigned i=at+1;i<count_;++i)entries_[i-1]=entries_[i];
            entries_[count_-1]=key;
        }
        return true;
    }
    // Seal only after all three sources have been captured successfully.
    bool seal(){if(!valid_||sealed_)return fail();sealed_=true;return true;}
    unsigned size()const{return count_;}
    template<class Visit> bool visit(Visit visit)const{
        if(!valid_||!sealed_)return false;
        for(unsigned i=0;i<count_;++i){
            const auto key=count_<=2?inline_[i]:entries_[count_-1-i];
            // The owner must resolve the captured generation, not replace it
            // with a reused slot or silently omit a subsequently flagged body.
            if(!visit(key))return false;
        }
        return true;
    }
};
}
#endif
