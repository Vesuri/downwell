#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_TREE_LEASE_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_TREE_LEASE_H
#include "RunnerSpatialIndex.h"
namespace downwell {
// The journal owns chronology; this owns only its materialized derivative.
// Platform acquisition must return a constructed, empty tree. No allocation
// happens for validity checks or reset. A failed acquisition is terminal until
// clear(), just like a failed tree mutation.
template<unsigned Nodes> class RunnerSpatialTreeLease {
public:
    using Tree=RunnerSpatialIndex<Nodes>;
    using Box=typename Tree::Box;
    struct Provider {
        void* context=nullptr;
        Tree* (*acquire)(void*)=nullptr;
        void (*release)(void*,Tree*)=nullptr;
    };
    bool bind(Provider provider){
        if(provider_.acquire||tree_||!good_||!provider.acquire||!provider.release)return false;
        provider_=provider;return true;
    }
    // Exclusive ownership lets mutation boundaries latch tree failure. Do not
    // poll an optional derived tree from every journal/owner validity read.
    bool valid()const{return good_;}
    bool resident()const{return tree_!=nullptr;}
    void clear(){
        if(tree_)provider_.release(provider_.context,tree_);
        tree_=nullptr;good_=true;
    }
    unsigned nodeCount()const{return get()?tree_->nodeCount():0;}
    int height()const{return get()?tree_->height():0;}
    bool insert(Box box,std::uint32_t id){return get()&&checked(tree_->insert(box,id));}
    bool remove(Box box,std::uint32_t id,bool retry,bool& removed){
        return get()&&checked(tree_->remove(box,id,retry,removed));
    }
    template<class Visit> bool query(Box box,Visit visit)const{
        return get()&&checked(tree_->query(box,visit));
    }
    template<class Visit> bool queryLine(const RunnerLineArguments& args,Visit visit)const{
        return get()&&checked(tree_->queryLine(args,visit));
    }
    RunnerSpatialTreeLease()=default;
    ~RunnerSpatialTreeLease(){clear();}
    RunnerSpatialTreeLease(const RunnerSpatialTreeLease&)=delete;
    RunnerSpatialTreeLease& operator=(const RunnerSpatialTreeLease&)=delete;
private:
    bool checked(bool result)const{
        // Invalid query arguments can return false without poisoning the
        // original tree. Preserve that distinction from mutation failure.
        good_=tree_->valid();return result;
    }
    bool get()const{
        if(!valid())return false;
        if(!tree_){
            tree_=provider_.acquire?provider_.acquire(provider_.context):nullptr;
            if(!tree_||!tree_->valid()){good_=false;return false;}
        }
        return true;
    }
    Provider provider_{};
    mutable Tree* tree_=nullptr;
    mutable bool good_=true;
};
}
#endif
