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
};
}
#endif
