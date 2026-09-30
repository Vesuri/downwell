#ifndef DOWNWELL_RUNNER_SPATIAL_STATE_H
#define DOWNWELL_RUNNER_SPATIAL_STATE_H
#include "RunnerCollisionMotion.h"
#include "RunnerSpatialSplit.h"
namespace downwell {
struct RunnerSpatialState {
    RunnerImageValue x{},y{};
    RunnerSpatialSplit::Box bounds{};
    std::uint32_t flags=8;
    unsigned kind=1;
    bool special=false;
    RunnerCollisionMotion::Previous previous{};
    RunnerSpatialState shifted(unsigned)const{return *this;}
    bool same(const RunnerSpatialState& b)const{
        return x.bits==b.x.bits&&y.bits==b.y.bits&&flags==b.flags&&kind==b.kind&&special==b.special&&
            previous.x.bits==b.previous.x.bits&&previous.y.bits==b.previous.y.bits&&
            bounds.left.bits==b.bounds.left.bits&&bounds.top.bits==b.bounds.top.bits&&
            bounds.right.bits==b.bounds.right.bits&&bounds.bottom.bits==b.bounds.bottom.bits;
    }
};
}
#endif
