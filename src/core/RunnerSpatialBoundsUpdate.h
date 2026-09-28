#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_BOUNDS_UPDATE_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_BOUNDS_UPDATE_H
#include "RunnerSpatialDirtyQueue.h"
#include "RunnerSpatialSplit.h"
#include "ProfileCounters.h"
namespace downwell {
// 0x5020a0: direct publication after bounds recalculation. Sprite/mask lookup
// and the caller's update-enable argument determine whether this is invoked.
// Old/new bounds can have reversed endpoints; normalize as the runner does.
struct RunnerSpatialBoundsUpdate {
    using Math=RunnerSpatialSplit;
    using Box=Math::Box;
    // Finalize 0x466010 after resource lookup/bounds calculation. A null
    // resourceBounds means lookup failed, NOT an empty collision rectangle.
    // Missing resources suppress publication even for already indexed instances.
    // The caller supplies source-calculated bounds; this does not calculate masks.
    template<class Index> static bool finishRefresh(Index& index,std::uint32_t id,
        std::uint32_t objectFlags,std::uint32_t& instanceFlags,bool allPrepared,
        Box& cached,const Box* resourceBounds,RunnerImageValue x,RunnerImageValue y,
        bool precise,bool update){
        if(!resourceBounds){
            if(!Math::valid(x)||!Math::valid(y))return false;
            cached={x,y,x,y};
            instanceFlags&=~0x208u; // Clear precise and bounds-dirty, keep membership.
            return true;
        }
        const Box old=cached,newBox=*resourceBounds;
        cached=newBox;
        instanceFlags=(instanceFlags&~0x208u)|(precise?0x200u:0u);
        return !update || publish(index,id,objectFlags,instanceFlags,allPrepared,old,newBox);
    }
    template<class Index> static bool publish(Index& index,std::uint32_t id,
        std::uint32_t objectFlags,std::uint32_t& instanceFlags,bool allPrepared,Box oldBox,Box newBox){
        if(!RunnerSpatialDirtyQueue<1>::shouldPublishBounds(objectFlags,instanceFlags,allPrepared))return true;
        if(!normalize(oldBox) || !normalize(newBox))return false;
#ifdef DOWNWELL_PROBES
        // Count actual admitted publications, not geometry refreshes or queue
        // marks. Equal bounds are diagnostic only: source removal/insertion
        // order remains observable and is never skipped by this counter.
        if(!(instanceFlags&0x8000u))DW_PROFILE_COUNT(counterSpatialFirstPublication);
        else if(Math::equal(oldBox.left,newBox.left)&&Math::equal(oldBox.top,newBox.top)&&
            Math::equal(oldBox.right,newBox.right)&&Math::equal(oldBox.bottom,newBox.bottom))
            DW_PROFILE_COUNT(counterSpatialEqualRepublication);
        else DW_PROFILE_COUNT(counterSpatialChangedRepublication);
#endif
        if(instanceFlags&0x8000u){
            bool removed=false;
            if(!index.remove(oldBox,id,true,removed))return false;
            // A failed lookup does not prevent the source's insertion.
        }
        if(!index.insert(newBox,id))return false;
        instanceFlags|=0x8000u;return true;
    }
private:
    static bool normalize(Box& box){
        if(!Math::valid(box.left)||!Math::valid(box.top)||!Math::valid(box.right)||!Math::valid(box.bottom))return false;
        if(Math::less(box.right,box.left)){auto a=box.left;box.left=box.right;box.right=a;}
        if(Math::less(box.bottom,box.top)){auto a=box.top;box.top=box.bottom;box.bottom=a;}
        return true;
    }
};
}
#endif
