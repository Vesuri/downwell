#ifndef DOWNWELL_CORE_INSTANCE_LIFECYCLE_H
#define DOWNWELL_CORE_INSTANCE_LIFECYCLE_H

#include "InstanceHandle.h"

namespace downwell {

struct InstanceRecord;
// Default for stores without an external collision owner. An installed adapter
// must publish birth before game callbacks and retire before handle invalidation.
// Adapters must not mutate the store and should reference, not copy, their owner.
struct NoInstanceLifecycle {
    bool birth(InstanceHandle, const InstanceRecord&) const { return true; }
    bool retire(InstanceHandle, const InstanceRecord&) const { return true; }
};

} // namespace downwell

#endif
