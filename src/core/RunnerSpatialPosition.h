#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_POSITION_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_POSITION_H
#include "RunnerSpatialDirtyQueue.h"
#include "RunnerSpatialSplit.h"
namespace downwell {
// 0x46bc60/0x501e50. Coordinates have already been narrowed to binary32.
// specialSpriteState represents the non-null 0x46bff0 result, NOT simply
// existence of a sprite. Both intrusive queue memberships use stable slots.
struct RunnerSpatialPosition {
    template<class SpatialQueue,class CollisionQueue>
    static bool assign(unsigned slot,RunnerImageValue& x,RunnerImageValue& y,
        std::uint32_t& flags,RunnerImageValue newX,RunnerImageValue newY,
        bool specialSpriteState,bool hasObject,std::uint32_t objectFlags,
        SpatialQueue& spatial,CollisionQueue& collision){
        using Math=RunnerSpatialSplit;
        if(!Math::valid(newX)||!Math::valid(newY)||!Math::valid(x)||!Math::valid(y))return false;
        if(!specialSpriteState && Math::equal(x,newX) && Math::equal(y,newY))return true;
        const bool enqueue=SpatialQueue::shouldQueue(hasObject,objectFlags,flags);
        // Reject invalid stable-slot bindings before publishing any mutation.
        if(enqueue && (slot>=SpatialQueue::End || ((objectFlags&8u)&&slot>=CollisionQueue::End)))return false;
        x=newX;y=newY;flags|=8u;
        if(enqueue){
            spatial.mark(slot);
            if(objectFlags&8u)collision.mark(slot);
        }
        return true;
    }
};
}
#endif
