#ifndef DOWNWELL_CORE_STORED_DITHER_COLLISION_PASS_H
#define DOWNWELL_CORE_STORED_DITHER_COLLISION_PASS_H
#include "StoredWorldCollision.h"
namespace downwell {
// Migration boundary for the authored wall-pruning controller. Collection uses
// the complete owner queue/tree order; only the implemented 61/60 pair is
// executed here. Other families still use the existing gameplay adapters.
// This is not a second pass to run alongside a complete collision dispatcher.
class StoredDitherCollisionPass {
public:
    explicit StoredDitherCollisionPass(StoredWorldCollision& owner):owner_(owner){}
    bool run(StartupRuntime& world){
        if(!world.effectsSucceeded())return false;
        if(world.roomId()!=11)return true;
        if(!owner_.collectCollisionPairs(pairs_,true))return false;
        RunnerCollisionEventContext context;
        struct Operations {
            StoredWorldCollision& owner;StartupRuntime& world;RunnerCollisionEventContext& context;
            bool overlap(InstanceHandle a,InstanceHandle b,bool& hit){return owner.collisionOverlap(a,b,hit);}
            bool flags(InstanceHandle h,std::uint32_t& value){return owner.collisionFlags(h,value);}
            bool queueCollision(InstanceHandle h){return owner.queueCollision(h);}
            bool dispatch(InstanceHandle a,InstanceHandle b){
                return owner.dispatchCollisionEvent(context,a,b,{world.roomId()==11,false},
                    [&](const auto& event){return world.dispatchDitherAboveCollision(event);});
            }
            // Both admitted resource objects are non-solid in runner terms.
            // Never invent engine/path motion if this contract changes.
            bool rollback(InstanceHandle){return false;}
            bool prepareMotion(InstanceHandle){return false;}
            bool advanceMotion(InstanceHandle){return false;}
        } operations{owner_,world,context};
        RunnerCollisionDispatch dispatch;
        return pairs_.dispatch([&](InstanceHandle a,InstanceHandle b){
            const auto* first=owner_.resolve(a);const auto* second=owner_.resolve(b);
            if(!first||!second)return false;
            const bool selected=(first->objectId==61&&second->objectId==60)||
                (first->objectId==60&&second->objectId==61);
            return !selected||dispatch.pair(a,b,operations);
        });
    }
private:
    StoredWorldCollision& owner_;
    RunnerCollisionPairs<StartupRuntime::instanceCapacity,16384> pairs_;
};
}
#endif
