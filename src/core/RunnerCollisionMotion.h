#ifndef DOWNWELL_CORE_RUNNER_COLLISION_MOTION_H
#define DOWNWELL_CORE_RUNNER_COLLISION_MOTION_H
#include "RunnerSpatialSplit.h"
namespace downwell {
// 0x50312a..0x50319c and 0x5031e3..0x5032cd. Engine hspeed/vspeed
// (instance +0xdc/+0xe0) are not actor-specific GML xsp/ysp or velocities.
// The scheduler supplies previous coordinates (+0xc0/+0xc4) at the original
// snapshot boundary. Position publication must use the ordinary setter so
// bounds dirtiness and both queue memberships remain intact.
struct RunnerCollisionMotion {
    using Real=RunnerImageValue;
    struct Previous {Real x{},y{};};
    struct Velocity {Real horizontal{},vertical{};};
    struct Path {Real position{},previousPosition{};};
    template<class Publish>
    static bool rollback(Previous previous,Path* path,Publish publish){
        if(!RunnerSpatialSplit::valid(previous.x)||!RunnerSpatialSplit::valid(previous.y))return false;
        if(!publish(previous.x,previous.y))return false;
        // Source restores path progress AFTER publishing the position.
        if(path)path->position=path->previousPosition;
        return true;
    }
    template<class Publish>
    static bool advance(Real x,Real y,Velocity velocity,Publish publish){
        Real nextX,nextY;
        // Match the two independent ADDSS instructions and their source order.
        if(!add(velocity.vertical,y,nextY)||!add(velocity.horizontal,x,nextX))return false;
        return publish(nextX,nextY);
    }
private:
    static bool add(Real a,Real b,Real& result){
        if(!RunnerSpatialSplit::valid(a)||!RunnerSpatialSplit::valid(b))return false;
        auto sum=a.promoted();
        return sum.add(b.promoted())&&result.assign(sum);
    }
};
}
#endif
