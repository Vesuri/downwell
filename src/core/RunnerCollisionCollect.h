#ifndef DOWNWELL_RUNNER_COLLISION_COLLECT_H
#define DOWNWELL_RUNNER_COLLISION_COLLECT_H
#include "RunnerCollisionPairs.h"
#include "RunnerSpatialDirtyQueue.h"
#include "RunnerSpatialSplit.h"
namespace downwell {
// 0x5022c0: shared collection order, independent of lifetime/property storage.
// All callbacks are read-only except flush. No handlers run before sealing.
struct RunnerCollisionCollect {
    template<unsigned Instances,unsigned Pairs,class Queue,class Flush,class Handle,class Flags,
        class Bounds,class Query,class Events,class Frames>
    static bool run(RunnerCollisionPairs<Instances,Pairs>& pairs,Queue& collision,
        unsigned (&retained)[Instances],bool indexAvailable,Flush flush,Handle handle,Flags flags,
        Bounds bounds,Query query,Events events,Frames frames){
        using Math=RunnerSpatialSplit;
        if(!pairs.beginPass())return false;
        unsigned count=0;
        if(indexAvailable){
            if(!flush())return false;
            unsigned slot=collision.first();
            while(slot!=Queue::End){
                const auto self=handle(slot);std::uint32_t stateFlags;
                if(!self.generation||self.slot!=slot||!flags(self,stateFlags))return false;
                const bool eligible=RunnerSpatialDirtyQueue<1>::shouldRefreshQueued(stateFlags);
                if(eligible){
                    Math::Box box;
                    // Cached endpoints may be reversed. Validate the scalars
                    // before normalizing, not the ordered query rectangle.
                    if(!bounds(self,box)||!Math::valid(box.left)||!Math::valid(box.top)||
                        !Math::valid(box.right)||!Math::valid(box.bottom))return false;
                    if(Math::less(box.right,box.left)){const auto v=box.left;box.left=box.right;box.right=v;}
                    if(Math::less(box.bottom,box.top)){const auto v=box.top;box.top=box.bottom;box.bottom=v;}
                    bool callbacks=true;
                    const bool queried=query(box,[&](unsigned candidate){
                        if(candidate>=Instances){callbacks=false;return false;}
                        const auto other=handle(candidate);
                        callbacks=other.generation&&other.slot==candidate&&pairs.candidate(self,other,events);
                        return callbacks;
                    });
                    if(!queried||!callbacks)return false;
                }
                if(!collision.contains(slot))return false;
                const unsigned successor=collision.next(slot);collision.remove(slot);
                if(eligible){
                    if(!flags(self,stateFlags))return false;
                    if(stateFlags&0x200u){
                        unsigned n=0;if(!frames(self,n))return false;
                        if(n>1){if(count==Instances)return false;retained[count++]=slot;}
                    }
                    if(!pairs.finishInstance(self))return false;
                }
                slot=successor;
            }
        }
        collision.clear();
        // Two original prepend passes preserve retained query order.
        while(count)collision.mark(retained[--count]);
        return pairs.seal();
    }
};
}
#endif
