#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_NODE_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_NODE_H
#include "RunnerSpatialSplit.h"
namespace downwell {
// Fixed node representation for the runner's six-entry spatial tree.
// Links are caller-owned indices, never native pointers. Tree allocation,
// traversal, condensation and scheduling belong to the index owner.
struct RunnerSpatialNode {
    struct Entry {
        RunnerSpatialSplit::Box bounds;
        std::uint32_t child=0,instance=0;
    };
    int level=0;
    unsigned count=0;
    Entry entries[6]={};
    bool append(const Entry& entry){
        if(count>=6)return false;
        entries[count++]=entry;return true;
    }
    // 0x502c68 leaf removal and 0x502c15 child removal: NOT stable erase.
    bool erase(unsigned index){
        if(count>6 || index>=count)return false;
        entries[index]=entries[count-1];--count;return true;
    }
    bool eraseInstance(std::uint32_t instance){
        if(level!=0 || count>6)return false;
        for(unsigned i=0;i<count;++i)if(entries[i].instance==instance)return erase(i);
        return false;
    }
    // 0x501420 redistributes in original array order, independently of the
    // quadratic assignment order. Both outputs remain intact on failure.
    bool split(const Entry& incoming,RunnerSpatialNode& sibling){
        if(count!=6 || &sibling==this)return false;
        RunnerSpatialSplit::Box bounds[7];
        for(unsigned i=0;i<6;++i)bounds[i]=entries[i].bounds;
        bounds[6]=incoming.bounds;RunnerSpatialSplit::Result groups;
        if(!RunnerSpatialSplit::partition(bounds,groups))return false;
        RunnerSpatialNode next[2];next[0].level=next[1].level=level;
        for(unsigned i=0;i<7;++i)next[groups.group[i]].append(i<6?entries[i]:incoming);
        *this=next[0];sibling=next[1];return true;
    }
};
}
#endif
