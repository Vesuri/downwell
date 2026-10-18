#ifndef DOWNWELL_RUNNER_SPATIAL_NODE_POOL_H
#define DOWNWELL_RUNNER_SPATIAL_NODE_POOL_H
#include "PlacementNew.h"
namespace downwell {
// Reserve capacity without constructing unreachable nodes. The index starts
// node zero and each fresh high-water slot before reading it. Recycled slots
// retain their entries, exactly as in the eager pool.
template<class Node,unsigned Capacity,bool Lazy> class RunnerSpatialNodePool;
template<class Node,unsigned Capacity> class RunnerSpatialNodePool<Node,Capacity,true> {
    static_assert(__has_trivial_destructor(Node),"Pool nodes must not own resources");
    union Slot {Node value;Slot(){};~Slot(){};};
    Slot slots_[Capacity];
public:
    void start(unsigned n){new(&slots_[n].value) Node;}
    Node& operator[](unsigned n){return slots_[n].value;}
    const Node& operator[](unsigned n)const{return slots_[n].value;}
};
template<class Node,unsigned Capacity> class RunnerSpatialNodePool<Node,Capacity,false> {
    Node nodes_[Capacity]{};
public:
    void start(unsigned){}
    Node& operator[](unsigned n){return nodes_[n];}
    const Node& operator[](unsigned n)const{return nodes_[n];}
};
}
#endif
