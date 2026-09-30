#ifndef DOWNWELL_RUNNER_ROOM_ORDER_STATE_H
#define DOWNWELL_RUNNER_ROOM_ORDER_STATE_H
#include "RunnerRegistrationOrder.h"
#include "RunnerImageValue.h"
namespace downwell {
// Active: depth then insertion order. Inactive: deactivation order only.
// These are room order keys, independent of object registration and layer Draw.
struct RunnerRoomOrderState {
    RunnerRegistrationOrder order;
    RunnerImageValue depth{};
    std::int8_t list=-1;
    bool marked=false;
    RunnerRoomOrderState shifted(unsigned offset)const{
        auto result=*this;
        if(order.present()){
            result.order.low+=offset;if(result.order.low<order.low)++result.order.high;
        }
        return result;
    }
    bool same(const RunnerRoomOrderState& b)const{
        return order.low==b.order.low&&order.high==b.order.high&&depth.bits==b.depth.bits&&
            list==b.list&&marked==b.marked;
    }
};
}
#endif
