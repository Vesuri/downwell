#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_CAPACITY_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_CAPACITY_H
namespace downwell {
// Bound for RunnerSpatialIndex with at most Instances live leaf entries.
// Splits leave at least two entries on each side; removal detaches children
// below two. Thus a settled tree has <= Instances/2 leaves and <= leaves-1
// internal nodes (an empty/single-entry root is the exception).
// Condensation retains at most one detached node per level. Reinsertion can
// temporarily allocate one sibling per level and a new root before releasing
// the detached node. Two bit-widths plus three cover both transient chains,
// the root exception, and the incoming entry. No traversal/layout rule changes.
template<unsigned Instances> struct RunnerSpatialCapacity {
    static_assert(Instances>0,"Nonempty spatial identity domain required");
    static constexpr unsigned levels=[](){unsigned n=Instances,bits=0;
        do{++bits;n>>=1;}while(n);return bits;}();
    static constexpr unsigned nodes=Instances+2*levels+3;
};
}
#endif
