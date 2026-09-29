#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_JOURNAL_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_JOURNAL_H
#include "RunnerSpatialIndex.h"
namespace downwell {
// Derived-tree scheduling only. The caller still publishes instance fields,
// flags, dirty queues and lifetime changes synchronously. Never collapse equal
// updates or reconstruct from current positions: tree order depends on history.
// All reads catch up. service() permits bounded work between publication bursts.
template<unsigned Nodes,unsigned Slots,unsigned Pending=512> class RunnerSpatialJournal {
    static_assert(Pending && !(Pending&(Pending-1)),"Journal capacity must be a power of two");
    using Math=RunnerSpatialSplit;
    static constexpr unsigned depthLimit=[](){unsigned n=Nodes,bits=0;do{++bits;n>>=1;}while(n);return bits;}();
    // A valid tree has at most N leaves and N-1 internal nodes for N live
    // entries. Removal retains at most one detached node per level while
    // reinserting. Reserve a second depth allowance for transient root splits.
    // This intentionally loose bound keeps deferred operations away from pool
    // exhaustion; near the bound, run the original mutation synchronously.
    static constexpr unsigned reserve=2*depthLimit+1;
    static constexpr unsigned safePopulation=Nodes>reserve?(Nodes-reserve)/2:0;
public:
    using Box=typename Math::Box;
    bool valid()const{return good_&&tree_.valid();}
    bool deferred()const{return deferred_;}
    unsigned pending()const{return count_;}
    void clear(){
        tree_.clear();head_=count_=population_=0;good_=deferred_=true;haveBounds_=false;
        for(auto& word:present_)word=0;
    }
    unsigned nodeCount()const{catchUp();return tree_.nodeCount();}
    int height()const{catchUp();return tree_.height();}
    // Borrow during admission, then take the journal's own snapshot. Neither
    // admission nor catch-up mutates caller storage; no temporary value copy
    // is needed at the API boundary (notably on the 68000 stack).
    bool insert(const Box& box,std::uint32_t instance){
        if(!valid())return false;
        Math::PixelBox next;
        if(!deferred_ || instance>=Slots || contains(instance) ||
            population_>=safePopulation || !admit(box,next)){
            if(!eager())return false;
            return tree_.insert(box,instance);
        }
        if(!makeRoom())return false;
        operations_[(head_+count_)&(Pending-1)]={box,instance,true};++count_;
        DW_PROFILE_COUNT(counterSpatialJournalQueued);
        present_[instance>>5]|=1u<<(instance&31);++population_;
        bounds_=next;haveBounds_=true;return true;
    }
    bool remove(const Box& oldBounds,std::uint32_t instance,bool retry,bool& removed){
        if(!valid())return false;
        // Full-root retry with a unique identity has an unambiguous result.
        // Unbounded IDs, duplicates and no-retry searches use the eager tree.
        if(!deferred_ || instance>=Slots || !retry || !Math::valid(oldBounds)){
            if(!eager())return false;
            return tree_.remove(oldBounds,instance,retry,removed);
        }
        if(!contains(instance)){removed=false;return true;}
        if(!makeRoom())return false;
        operations_[(head_+count_)&(Pending-1)]={oldBounds,instance,false};++count_;
        DW_PROFILE_COUNT(counterSpatialJournalQueued);
        present_[instance>>5]&=~(1u<<(instance&31));--population_;
        removed=true;return true;
    }
    template<class Visit> bool query(Box box,Visit visit)const{
        return catchUp()&&tree_.query(box,visit);
    }
    template<class Visit> bool queryLine(const RunnerLineArguments& args,Visit visit)const{
        return catchUp()&&tree_.queryLine(args,visit);
    }
    bool service(unsigned budget)const{
        if(!valid())return false;
        while(count_&&budget--){
            const auto& operation=operations_[head_];
            bool removed=false;
            const bool ok=operation.insert?tree_.insert(operation.box,operation.instance):
                tree_.remove(operation.box,operation.instance,true,removed)&&removed;
            if(!ok){good_=false;return false;}
            DW_PROFILE_COUNT(counterSpatialJournalServiced);
            head_=(head_+1)&(Pending-1);--count_;
        }
        return true;
    }
private:
    struct Operation{Box box;std::uint32_t instance;bool insert;};
    mutable RunnerSpatialIndex<Nodes> tree_;
    Operation operations_[Pending]{};
    std::uint32_t present_[(Slots+31)/32? (Slots+31)/32:1]{};
    mutable unsigned head_=0,count_=0;
    unsigned population_=0;
    mutable bool good_=true;
    bool deferred_=true,haveBounds_=false;
    Math::PixelBox bounds_{};
    bool contains(unsigned instance)const{return (present_[instance>>5]>>(instance&31))&1u;}
    bool catchUp()const{return service(count_);}
    bool makeRoom(){
        if(count_<Pending)return true;
        DW_PROFILE_COUNT(counterSpatialJournalPressure);
        return catchUp();
    }
    bool eager(){
        if(!catchUp())return false;
        if(deferred_)DW_PROFILE_COUNT(counterSpatialJournalFallback);
        deferred_=false;return true;
    }
    bool admit(const Box& box,Math::PixelBox& combined)const{
        // These conditions also exclude numeric failures during later replay:
        // every historical union has exact finite binary32 area arithmetic.
        if(box.left.bits==0x80000000u||box.top.bits==0x80000000u||
           box.right.bits==0x80000000u||box.bottom.bits==0x80000000u||
           !Math::decodePixels(box,combined))return false;
        if(haveBounds_){
            if(bounds_.left<combined.left)combined.left=bounds_.left;
            if(bounds_.top<combined.top)combined.top=bounds_.top;
            if(bounds_.right>combined.right)combined.right=bounds_.right;
            if(bounds_.bottom>combined.bottom)combined.bottom=bounds_.bottom;
        }
        const unsigned width=unsigned(combined.right-combined.left),height=unsigned(combined.bottom-combined.top);
        return width<=65535&&height<=65535&&
            std::uint32_t(std::uint16_t(width))*std::uint32_t(std::uint16_t(height))<=0x1000000u;
    }
};
}
#endif
