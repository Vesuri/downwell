#ifndef DOWNWELL_CORE_RUNNER_COLLISION_PAIRS_H
#define DOWNWELL_CORE_RUNNER_COLLISION_PAIRS_H
#include "InstanceHandle.h"
#include "SlotIndex.h"
namespace downwell {
// Candidate callback 0x503000 and append-order dispatch 0x5025b6. The owner
// supplies ORIGINAL dirty-list/spatial traversal order and live handles. It
// must not mutate lifetimes from event-lookup callbacks. Precise overlap and
// destruction checks happen later, when processing the sealed pairs.
template<unsigned Instances,unsigned Capacity,bool SparseReset=true> class RunnerCollisionPairs {
    static_assert(Instances>0&&Capacity>0,"Collision storage must be nonempty");
public:
    struct Pair {InstanceHandle self,other;};
    bool succeeded()const{return good_;}
    unsigned size()const{return count_;}
    const Pair* at(unsigned index)const{return index<count_?&pairs_[index]:nullptr;}
    bool beginPass(){
        if(!good_||collecting_||executing_)return fail();
        count_=0;
        if constexpr(SparseReset){
            while(touchedCount_)processed_[touched_[--touchedCount_]]=0;
        }else for(auto& generation:processed_)generation=0; // Retained oracle.
        collecting_=true;ready_=false;return true;
    }
    template<class HasEvent> bool candidate(InstanceHandle self,InstanceHandle other,HasEvent hasEvent){
        if(!good_)return false;
        if(!collecting_||!valid(self)||!valid(other))return fail();
        if((self.slot==other.slot&&self.generation==other.generation)||processed_[other.slot]==other.generation)return true;
        bool found=false;
        if(!hasEvent(self,other,found))return fail();
        if(!found&&!hasEvent(other,self,found))return fail();
        if(!found)return true;
        if(count_==Capacity)return fail();
        pairs_[count_++]={self,other};return true;
    }
    // 0x502474 stamps the current instance only AFTER its spatial query.
    bool finishInstance(InstanceHandle self){
        if(!good_)return false;
        if(!collecting_||!valid(self))return fail();
        if constexpr(SparseReset){
            // A slot may be stamped repeatedly, including with a replacement
            // generation. Remember it once; candidate suppression still reads
            // the exact generation rather than merely a per-slot visited bit.
            if(!processed_[self.slot])touched_[touchedCount_++]=self.slot;
        }
        processed_[self.slot]=self.generation;return true;
    }
    bool seal(){
        if(!good_)return false;
        if(!collecting_)return fail();
        collecting_=false;ready_=true;return true;
    }
    // 0x5025b6 captures the count and processes append order. Keep the sealed
    // buffer stable while handlers run; recursive collection is unsupported.
    // Successful consumption is once-only, and any failure retains its prefix.
    template<class Dispatch> bool dispatch(Dispatch invoke){
        if(!good_||!ready_||executing_)return fail();
        ready_=false;executing_=true;
        const unsigned limit=count_;
        for(unsigned i=0;i<limit;++i){
            const Pair pair=pairs_[i];
            if(!invoke(pair.self,pair.other)||!good_){executing_=false;return fail();}
        }
        executing_=false;return true;
    }
private:
    bool valid(InstanceHandle h)const{return h.slot<Instances&&h.generation!=0;}
    bool fail(){good_=false;return false;}
    Pair pairs_[Capacity]{};std::uint32_t processed_[Instances]{};
    typename SlotIndex<Instances>::Type touched_[SparseReset?Instances:1];
    unsigned touchedCount_=0;
    unsigned count_=0;bool collecting_=false,ready_=false,executing_=false,good_=true;
};
}
#endif
