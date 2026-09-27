#ifndef DOWNWELL_CORE_STORED_LAYER_LIFECYCLE_H
#define DOWNWELL_CORE_STORED_LAYER_LIFECYCLE_H
#include "InstanceStore.h"
#include "RunnerLayerLifetimes.h"
namespace downwell {
// Borrowed observer for stored instances. Room resources must be prepared
// before births; surviving states belong to the same lifetime owner.
template<unsigned L,unsigned E,unsigned I> struct StoredLayerLifecycle {
    RunnerLayerLifetimes<L,E,I>& owner;
    bool birth(InstanceHandle h,const InstanceRecord& record){
        RunnerImageValue depth;
        return depth.assign(MotionBinary64::fromInteger(record.depth))&&
            owner.birth(h,record.instanceId,depth,!record.active);
    }
    bool retire(InstanceHandle h,const InstanceRecord&){return owner.retire(h);}
    bool activation(InstanceHandle h,const InstanceRecord&,bool active){return owner.publishInactive(h,!active);}
    // Source 0x48d630 reconciles object membership before layer relinking.
    // The published inactive flag can precede this boundary by several events.
    bool membership(InstanceHandle h,const InstanceRecord&,bool registered){
        const auto* state=owner.resolve(h);
        return state&&state->inactive==!registered&&owner.reconcileActivation(h);
    }
};
}
#endif
