#ifndef DOWNWELL_AMIGA_SPATIAL_TREE_STORAGE_H
#define DOWNWELL_AMIGA_SPATIAL_TREE_STORAGE_H
#include "core/RunnerSpatialTreeLease.h"
#include <exec/memory.h>
#include <proto/exec.h>
#include "core/PlacementNew.h"

// CPU-only derived state, acquired only when a journal must materialize.
// The scope outlives the placement-constructed collision owner. It also
// releases the lease on early returns where that owner's destructor is not run.
template<unsigned Nodes> class AmigaSpatialTreeStorage {
    using Lease=downwell::RunnerSpatialTreeLease<Nodes>;
    using Tree=typename Lease::Tree;
    Tree* tree_=nullptr;
    static Tree* acquire(void* context){
        auto& self=*static_cast<AmigaSpatialTreeStorage*>(context);
        if(self.tree_)return nullptr; // One borrower, no implicit shared tree.
        auto* memory=AllocMem(sizeof(Tree),MEMF_FAST);
        if(!memory)return nullptr;
        return self.tree_=new(memory) Tree;
    }
    static void release(void* context,Tree* tree){
        auto& self=*static_cast<AmigaSpatialTreeStorage*>(context);
        if(tree!=self.tree_)__builtin_trap();
        tree->~Tree();FreeMem(tree,sizeof(Tree));self.tree_=nullptr;
    }
public:
    typename Lease::Provider provider(){return {this,acquire,release};}
    AmigaSpatialTreeStorage()=default;
    ~AmigaSpatialTreeStorage(){if(tree_)release(this,tree_);}
    AmigaSpatialTreeStorage(const AmigaSpatialTreeStorage&)=delete;
    AmigaSpatialTreeStorage& operator=(const AmigaSpatialTreeStorage&)=delete;
};
#endif
