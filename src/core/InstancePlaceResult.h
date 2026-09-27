#ifndef DOWNWELL_CORE_INSTANCE_PLACE_RESULT_H
#define DOWNWELL_CORE_INSTANCE_PLACE_RESULT_H
#include "InstanceHandle.h"
#include "MotionBinary64.h"
namespace downwell {
// A successful query returns either noone (-4), or a live source instance ID
// and a snapshot of its published coordinates. Never retain a record pointer
// across synchronous Create/Destroy; the handle is generation checked.
struct InstancePlaceResult {
    std::int32_t id=-4;
    InstanceHandle handle{};
    MotionBinary64 x{},y{};
};
}
#endif
