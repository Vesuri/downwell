#ifndef DOWNWELL_CORE_STORED_DITHER_COLLISION_PASS_H
#define DOWNWELL_CORE_STORED_DITHER_COLLISION_PASS_H
#include "StoredWorldCollision.h"
namespace downwell {
// Migration boundary for the authored wall-pruning controller. Collection uses
// the complete owner queue/tree order; the implemented 61/60 pair and, when
// explicitly bound, registered 225/60 and 223/60 pairs are executed here. Other families
// still use the existing gameplay adapters.
// This is not a second pass to run alongside a complete collision dispatcher.
class StoredDitherCollisionPass {
public:
    explicit StoredDitherCollisionPass(StoredWorldCollision& owner):owner_(owner){}
    bool run(StartupRuntime& world){
        return runSelected(world,[](const auto&,const auto&){return false;},
            [&](const auto& event){return world.dispatchDitherAboveCollision(event);});
    }
    // Admit registered smoke in the SAME pair stream, not a later smoke-only
    // pass. Broader wall families still need their reverse handlers/motion.
    template<unsigned Capacity> bool run(StartupRuntime& world,
        StoredSmokeRegistration<StartupRuntime::instanceCapacity,Capacity>& smoke){
        if(!smoke.valid())return false;
        return runSelected(world,[&](const auto& a,const auto& b){
            return (a.objectId==225&&b.objectId==60&&smoke.state(a))||
                   (b.objectId==225&&a.objectId==60&&smoke.state(b));
        },[&](const auto& event){
            const auto* self=owner_.resolve(event.self);
            return self&&(self->objectId==225?world.dispatchSmokeCollision(smoke,event):
                         world.dispatchDitherAboveCollision(event));
        });
    }
    template<unsigned SmokeCapacity,unsigned ParticleCapacity> bool run(StartupRuntime& world,
        StoredSmokeRegistration<StartupRuntime::instanceCapacity,SmokeCapacity>& smoke,
        StoredConsumptionBloodRegistration<StartupRuntime::instanceCapacity,ParticleCapacity>& particles){
        if(!smoke.valid()||!particles.valid())return false;
        const auto effect=[&](const auto& r){
            return (r.objectId==225&&smoke.state(r))||(r.objectId==223&&particles.state(r));
        };
        return runSelected(world,[&](const auto& a,const auto& b){
            return (b.objectId==60&&effect(a))||(a.objectId==60&&effect(b));
        },[&](const auto& event){
            const auto* self=owner_.resolve(event.self);if(!self)return false;
            if(self->objectId==223)return world.dispatchConsumptionBloodCollision(particles,event);
            if(self->objectId==225)return world.dispatchSmokeCollision(smoke,event);
            return world.dispatchDitherAboveCollision(event);
        });
    }
private:
    template<class Admit,class Execute> bool runSelected(StartupRuntime& world,Admit admit,Execute execute){
        if(!world.effectsSucceeded())return false;
        if(world.roomId()!=11)return true;
        if(!owner_.collectCollisionPairs(pairs_,true))return false;
        RunnerCollisionEventContext context;
        struct Operations {
            StoredWorldCollision& owner;StartupRuntime& world;RunnerCollisionEventContext& context;
            Execute& execute;
            bool overlap(InstanceHandle a,InstanceHandle b,bool& hit){return owner.collisionOverlap(a,b,hit);}
            bool flags(InstanceHandle h,std::uint32_t& value){return owner.collisionFlags(h,value);}
            bool queueCollision(InstanceHandle h){return owner.queueCollision(h);}
            bool dispatch(InstanceHandle a,InstanceHandle b){
                return owner.dispatchCollisionEvent(context,a,b,{world.roomId()==11,false},
                    execute);
            }
            // Both admitted resource objects are non-solid in runner terms.
            // Never invent engine/path motion if this contract changes.
            bool rollback(InstanceHandle){return false;}
            bool prepareMotion(InstanceHandle){return false;}
            bool advanceMotion(InstanceHandle){return false;}
        } operations{owner_,world,context,execute};
        RunnerCollisionDispatch dispatch;
        return pairs_.dispatch([&](InstanceHandle a,InstanceHandle b){
            const auto* first=owner_.resolve(a);const auto* second=owner_.resolve(b);
            if(!first||!second)return false;
            const bool selected=(first->objectId==61&&second->objectId==60)||
                (first->objectId==60&&second->objectId==61)||admit(*first,*second);
            return !selected||dispatch.pair(a,b,operations);
        });
    }
private:
    StoredWorldCollision& owner_;
    RunnerCollisionPairs<StartupRuntime::instanceCapacity,16384> pairs_;
};
}
#endif
