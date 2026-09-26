#ifndef DOWNWELL_CORE_RUNNER_PLACE_QUERY_H
#define DOWNWELL_CORE_RUNNER_PLACE_QUERY_H
#include "RunnerSpatialObjectPreparation.h"
namespace downwell {
// 0x4fe560: object-selector instance_place orchestration. The owner resolves
// lifetimes and applies source candidate eligibility in list/spatial visitors.
// This does not implement all/instance selectors or the collect-all variant.
struct RunnerPlaceQuery {
    using Path=RunnerSpatialObjectPreparation::Path;
    using Box=RunnerSpatialSplit::Box;
    using Real=RunnerImageValue;
    struct Result {bool found=false;std::uint32_t identity=0;};
    // prepare includes family preparation and returns the selected branch.
    // move must implement the position setter's dirty-queue side effects.
    // precise(identity,publishBounds,hit) performs the candidate collision.
    // Enumerators stop when visitor returns false; return false on failure.
    // Failures attempt position restoration, but require the owner to latch
    // failure: index/candidate mutations are not rolled back.
    template<class Prepare,class Flush,class Refresh,class Move,class List,class Spatial,class Precise>
    static bool run(Real& x,Real& y,std::uint32_t& flags,Box& cached,Real targetX,Real targetY,
        Prepare prepare,Flush flush,Refresh refresh,Move move,List list,Spatial spatial,
        Precise precise,Result& result){
        const auto path=prepare();
        if(path==Path::failed)return false;
        if(path==Path::none){result={};return true;}
        const bool indexed=path==Path::spatial;
        if(indexed && (!flush()||((flags&8u)&&!refresh(true))))return false;
        const Real oldX=x,oldY=y;const Box oldBox=cached;
        bool good=move(targetX,targetY);
        Result next;
        auto visit=[&](std::uint32_t identity){
            if(!good||next.found)return false;
            bool hit=false;
            if(!precise(identity,!indexed,hit)){good=false;return false;}
            if(hit){next={true,identity};return false;}
            return true;
        };
        if(good && indexed){
            if((flags&8u)&&!refresh(false))good=false;
            Box box=cached;
            if(good){
                using Math=RunnerSpatialSplit;
                good=Math::valid(box.left)&&Math::valid(box.top)&&Math::valid(box.right)&&Math::valid(box.bottom);
                if(good){
                    if(Math::less(box.right,box.left)){const auto n=box.left;box.left=box.right;box.right=n;}
                    if(Math::less(box.bottom,box.top)){const auto n=box.top;box.top=box.bottom;box.bottom=n;}
                    if(!spatial(box,visit))good=false;
                }
            }
        }else if(good && !list(visit))good=false;
        // Restore through the setter, NOT a raw assignment: dirty queue order
        // remains observable. Only the spatial branch restores cached bounds.
        const bool restored=move(oldX,oldY);
        if(indexed)cached=oldBox;
        if(!good||!restored)return false;
        result=next;return true;
    }
};
}
#endif
