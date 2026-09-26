#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_OBJECT_PREPARATION_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_OBJECT_PREPARATION_H
#include "RunnerSpatialSplit.h"
namespace downwell {
// Object-selector path 0x500f85..0x5010d4, after object resolution. This is
// neither the `all` selector nor an instance-ID query. Preparation may mutate
// the shared index before precise traversal; it is not a pure branch test.
struct RunnerSpatialObjectPreparation {
    using Math=RunnerSpatialSplit;
    using Box=Math::Box;
    enum class Path {none,list,spatial,failed};
    // markFamily implements 0x4798f0 against the caller's object graph.
    // members visits the ancestor-inclusive registration list in source order;
    // its callback receives (identity, flags&, cachedBounds&). Refresh performs
    // 0x466010(update=0), not the ordinary dirty-queue publication operation.
    // Any failure forbids querying the partially prepared state until recovery.
    template<class MarkFamily,class Members,class Refresh,class Insert>
    static Path prepare(unsigned population,unsigned rootHeight,
        std::uint32_t objectFlags,bool allPrepared,MarkFamily markFamily,
        Members members,Refresh refresh,Insert insert){
        if(!population)return Path::none;
        if(population<rootHeight)return Path::list;
        if((objectFlags&0x28u) || allPrepared)return Path::spatial;
        if(!markFamily())return Path::failed;
        bool good=true;
        members([&](std::uint32_t identity,std::uint32_t& flags,Box& cached){
            if(!good)return false;
            // Includes already-indexed 0x8000, unlike ordinary hit filtering.
            if(flags&0x108003u)return true;
            if((flags&8u) && !refresh(identity,flags,cached)){good=false;return false;}
            Box box=cached;
            if(!Math::valid(box.left)||!Math::valid(box.right)||
               !Math::valid(box.top)||!Math::valid(box.bottom)){good=false;return false;}
            if(Math::less(box.right,box.left)){auto n=box.left;box.left=box.right;box.right=n;}
            if(Math::less(box.bottom,box.top)){auto n=box.top;box.top=box.bottom;box.bottom=n;}
            if(!insert(box,identity)){good=false;return false;}
            flags|=0x8000u;
            return true;
        });
        return good?Path::spatial:Path::failed;
    }
};
}
#endif
