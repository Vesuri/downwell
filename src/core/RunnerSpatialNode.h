#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_NODE_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_NODE_H
#include "RunnerSpatialSplit.h"
namespace downwell {
// Fixed node representation for the runner's six-entry spatial tree.
// Links are caller-owned indices, never native pointers. Tree allocation,
// traversal, condensation and scheduling belong to the index owner.
template<class Box> struct RunnerSpatialNodeStorage {
    struct Entry {
        Box bounds;
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
    bool split(const Entry& incoming,RunnerSpatialNodeStorage& sibling){
        return splitUsing(incoming,sibling,[](const Box (&bounds)[7],RunnerSpatialSplit::Result& groups){
            return RunnerSpatialSplit::partition(bounds,groups);
        });
    }
    template<class Partition>
    bool splitUsing(const Entry& incoming,RunnerSpatialNodeStorage& sibling,Partition partition){
        if(count!=6 || &sibling==this)return false;
        Box bounds[7];
        for(unsigned i=0;i<6;++i)bounds[i]=entries[i].bounds;
        bounds[6]=incoming.bounds;RunnerSpatialSplit::Result groups;
        if(!partition(bounds,groups))return false;
        // All fallible work is complete. Compact group zero in place while
        // appending group one to the sibling; retain source array order.
        // Copy incoming first because callers may pass one of our entries.
        const Entry last=incoming;
        sibling.level=level;sibling.count=0;count=0;
        for(unsigned i=0;i<6;++i){
            if(groups.group[i])sibling.append(entries[i]);
            else if(count==i)++count; // This entry is already in its final slot.
            else append(entries[i]); // Compaction writes strictly behind i.
        }
        if(groups.group[6])sibling.append(last);else append(last);
        // Only [0,count) is live. Future append overwrites its destination;
        // readers/conversion must not inspect or require cleared tail slots.
        return true;
    }
};
using RunnerSpatialNode=RunnerSpatialNodeStorage<RunnerSpatialSplit::Box>;
}
#endif
