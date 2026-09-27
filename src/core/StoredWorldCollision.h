#ifndef DOWNWELL_CORE_STORED_WORLD_COLLISION_H
#define DOWNWELL_CORE_STORED_WORLD_COLLISION_H
#include "StartupRuntime.h"
#include "StoredCollisionLifecycle.h"
#include "RunnerObjectFamilies.h"
#include "SmokeDirection.h"
#include "SmokeTrig.h"
#include "RunnerCollisionEventContext.h"
#include "RunnerCollisionDispatch.h"
namespace downwell {
// Resource-backed owner for the stored world. Construct before the runtime,
// bind its lifecycle before room births, then attach the runtime before Step.
// Resource buffers, this owner and its attached runtime must remain alive and
// at fixed addresses. No per-frame population pass or copied instance records.
class StoredWorldCollision {
    struct Trig {
        bool operator()(RunnerImageValue angle,RunnerImageValue& c,RunnerImageValue& s)const{
            RunnerImageValue radians;MotionBinary64 cosine,sine;
            return SmokeDirection::radians(angle,radians)&&smokeTrig(radians.promoted(),cosine,sine)&&
                c.assign(cosine)&&s.assign(sine);
        }
    };
    struct Round {bool operator()(RunnerImageValue v,std::int32_t& out)const{return v.promoted().roundPixel(out);}};
    static constexpr unsigned capacity=StartupRuntime::instanceCapacity;
    using Owner=RunnerSpatialInstances<capacity,4096>;
    using Families=RunnerObjectFamilies<386>;
    using Adapter=StoredCollisionLifecycle<capacity,4096,Families,Trig,Round>;
    Owner owner_;
    Owner::Properties properties_[capacity]{};
    Families families_;
    std::uint32_t ids_[capacity]{};
    const SpriteMaskPack& masks_;
    Adapter adapter_;
    const StartupRuntime* runtime_=nullptr;
    bool valid_=false;
public:
    StoredWorldCollision(const ObjectPack& objects,const SpriteCollisionPack& sprites,const SpriteMaskPack& masks)
        :masks_(masks),adapter_{owner_,properties_,objects,sprites,families_,{}, {}}{
        valid_=sprites.valid()&&masks.valid()&&families_.bindInstanceFlags(objects);
    }
    StoredWorldCollision(const StoredWorldCollision&)=delete;
    StoredWorldCollision& operator=(const StoredWorldCollision&)=delete;
    bool valid()const{return valid_&&owner_.valid();}
    bool attach(const StartupRuntime& runtime){
        if(!valid()||(runtime_&&runtime_!=&runtime))return false;
        runtime_=&runtime;return true;
    }
    InstanceLifecycleBinding binding(){return InstanceLifecycleBinding::bindPointQueries(*this);}
    bool capturePrevious(InstanceHandle h,const InstanceRecord& r){
        return resolve(h)==&r&&adapter_.capturePrevious(h,r);
    }
    bool collisionPreviousPosition(InstanceHandle h,RunnerCollisionMotion::Previous& previous)const{
        if(!valid()||!resolve(h))return false;
        const auto* state=owner_.resolve(h);if(!state)return false;
        previous=state->previous;return true;
    }
    bool collisionPosition(InstanceHandle h,RunnerImageValue& x,RunnerImageValue& y)const{
        if(!valid()||!resolve(h))return false;
        const auto* state=owner_.resolve(h);if(!state)return false;
        x=state->x;y=state->y;return true;
    }
    // Position portion only; path progress is owned separately. Call these
    // with the attached mutable world, never with a copy sharing this owner.
    bool rollbackCollisionPosition(StartupRuntime& world,InstanceHandle h){
        RunnerCollisionMotion::Previous previous;
        return runtime_==&world&&collisionPreviousPosition(h,previous)&&
            RunnerCollisionMotion::rollback(previous,nullptr,[&](auto x,auto y){
                return world.publishCollisionPosition(h,x,y);
            });
    }
    bool advanceCollisionPosition(StartupRuntime& world,InstanceHandle h,RunnerCollisionMotion::Velocity engine){
        RunnerImageValue x,y;
        return runtime_==&world&&collisionPosition(h,x,y)&&
            RunnerCollisionMotion::advance(x,y,engine,[&](auto nextX,auto nextY){
                return world.publishCollisionPosition(h,nextX,nextY);
            });
    }
    const InstanceRecord* resolve(InstanceHandle h)const{
        if(!runtime_||!owner_.resolve(h))return nullptr;
        const auto* record=runtime_->resolveInstance(h);
        return record&&record->instanceId==ids_[h.slot]&&
            record->objectId==owner_.objectId(h)?record:nullptr;
    }
    bool birth(InstanceHandle h,const InstanceRecord& r){
        if(!valid()||h.slot>=capacity||ids_[h.slot]||!r.instanceId||!adapter_.birth(h,r))return false;
        ids_[h.slot]=r.instanceId;return true;
    }
    bool birthAt(InstanceHandle h,const InstanceRecord& r,RunnerImageValue x,RunnerImageValue y){
        if(!valid()||h.slot>=capacity||ids_[h.slot]||!r.instanceId||!adapter_.birthAt(h,r,x,y))return false;
        ids_[h.slot]=r.instanceId;return true;
    }
    bool retire(InstanceHandle h,const InstanceRecord& r){
        if(h.slot>=capacity||ids_[h.slot]!=r.instanceId||!adapter_.retire(h,r))return false;
        ids_[h.slot]=0;return true;
    }
    bool markDestroyed(InstanceHandle h,const InstanceRecord& r){return resolve(h)==&r&&adapter_.markDestroyed(h,r);}
    bool instanceNumber(int object,unsigned& result)const{
        return valid()&&owner_.instanceNumber(object,families_,result);
    }
    template<unsigned Pairs>
    bool collectCollisionPairs(RunnerCollisionPairs<capacity,Pairs>& pairs,bool indexAvailable){
        return valid()&&runtime_&&adapter_.collectCollisionPairs(*this,pairs,indexAvailable);
    }
    // Resolve at dispatch time, not collection time: an earlier handler can
    // change object identity. No destruction/activation filter belongs here;
    // the paired-dispatch wrapper controls those source-specific checks.
    bool collisionEvent(InstanceHandle self,InstanceHandle other,bool& found,PackedObjectEvent& event)const{
        if(!valid())return false;
        const auto* a=resolve(self);const auto* b=resolve(other);
        return a&&b&&adapter_.objects.collisionEvent(a->objectId,b->objectId,found,event);
    }
    template<class Execute>
    bool dispatchCollisionEvent(RunnerCollisionEventContext& context,InstanceHandle self,InstanceHandle other,
                                RunnerCollisionEventContext::RoomState room,Execute execute)const{
        if(!valid())return false;
        return context.dispatch(self,other,room,
            [&](InstanceHandle a,InstanceHandle b,int& subtype,bool& found,PackedObjectEvent& event){
                const auto* opposite=resolve(b);if(!opposite)return false;
                subtype=opposite->objectId;return collisionEvent(a,b,found,event);
            },execute);
    }
    bool collisionFlags(InstanceHandle h,std::uint32_t& flags)const{
        if(!valid()||!resolve(h))return false;
        const auto* state=owner_.resolve(h);if(!state)return false;
        flags=state->flags;return true;
    }
    bool queueCollision(InstanceHandle h){
        if(!valid()||!resolve(h))return false;
        const auto* flags=families_.flags(unsigned(owner_.objectId(h)));
        return flags&&owner_.queueCollision(h,*flags);
    }
    bool collisionOverlap(InstanceHandle self,InstanceHandle other,bool& hit){
        if(!valid())return false;
        return adapter_.collide(*this,self,other,false,masks_,RunnerMaskDispatch{},
            [](RunnerImageValue angle,RunnerImageValue& c,RunnerImageValue& s){
                angle.bits^=0x80000000u;return Trig{}(angle,c,s);
            },hit);
    }
    // Execute a sealed source-ordered pass against live lifetime identities.
    // Motion supplies checked path restoration/preparation and engine velocity;
    // roomState is reread for EACH event direction. Neither execution callback
    // may physically retire instances until this captured pass finishes.
    template<unsigned Pairs,class Motion,class RoomState,class Execute>
    bool executeCollisionPairs(StartupRuntime& world,RunnerCollisionPairs<capacity,Pairs>& pairs,
        RunnerCollisionEventContext& context,Motion& motion,RoomState roomState,Execute execute){
        if(!valid()||runtime_!=&world||!world.effectsSucceeded()||!context.succeeded())return false;
        struct Ops {
            StoredWorldCollision& owner;StartupRuntime& world;RunnerCollisionEventContext& context;
            Motion& motion;RoomState& roomState;Execute& execute;
            bool overlap(InstanceHandle a,InstanceHandle b,bool& hit){return owner.collisionOverlap(a,b,hit);}
            bool flags(InstanceHandle h,std::uint32_t& flags){return owner.collisionFlags(h,flags);}
            bool queueCollision(InstanceHandle h){return owner.queueCollision(h);}
            bool rollback(InstanceHandle h){return owner.rollbackCollisionPosition(world,h)&&motion.restorePath(h);}
            bool dispatch(InstanceHandle a,InstanceHandle b){
                return owner.dispatchCollisionEvent(context,a,b,roomState(),execute)&&world.effectsSucceeded();
            }
            bool prepareMotion(InstanceHandle h){return motion.preparePath(h);}
            bool advanceMotion(InstanceHandle h){RunnerCollisionMotion::Velocity speed;
                return motion.engineVelocity(h,speed)&&owner.advanceCollisionPosition(world,h,speed);}
        } ops{*this,world,context,motion,roomState,execute};
        RunnerCollisionDispatch dispatch;
        return pairs.dispatch([&](InstanceHandle a,InstanceHandle b){return dispatch.pair(a,b,ops);});
    }
    bool activation(InstanceHandle h,const InstanceRecord& r,bool active){return adapter_.activation(h,r,active);}
    bool membership(InstanceHandle h,const InstanceRecord& r,bool registered){return adapter_.membership(h,r,registered);}
    bool move(InstanceHandle h,const InstanceRecord& r,RunnerImageValue x,RunnerImageValue y){return adapter_.move(h,r,x,y);}
    bool sprite(InstanceHandle h,const InstanceRecord& r,int sprite){return adapter_.sprite(h,r,sprite);}
    bool mask(InstanceHandle h,const InstanceRecord& r,int mask){return adapter_.mask(h,r,mask);}
    bool imageIndex(InstanceHandle h,const InstanceRecord& r,RunnerImageValue index){return adapter_.imageIndex(h,r,index);}
    bool transform(InstanceHandle h,const InstanceRecord& r,RunnerTransformField field,
                   RunnerImageValue value,RunnerImageValue alpha,std::uint32_t blend){
        return adapter_.transform(h,r,field,value,alpha,blend);
    }
    bool placeMeeting(InstanceHandle h,const InstanceRecord& r,int object,MotionBinary64 x,MotionBinary64 y,bool& hit){
        if(!valid()||resolve(h)!=&r)return false;
        return adapter_.meetingVm(*this,h,object,x,y,masks_,RunnerMaskDispatch{},
            [](RunnerImageValue angle,RunnerImageValue& c,RunnerImageValue& s){
                angle.bits^=0x80000000u;return Trig{}(angle,c,s);
            },hit);
    }
    bool instancePlace(InstanceHandle h,const InstanceRecord& r,int object,MotionBinary64 x,MotionBinary64 y,
                       InstancePlaceResult& result){
        if(!valid()||resolve(h)!=&r)return false;
        Owner::Result selected;
        if(!adapter_.placeVm(*this,h,object,x,y,masks_,RunnerMaskDispatch{},
            [](RunnerImageValue angle,RunnerImageValue& c,RunnerImageValue& s){
                angle.bits^=0x80000000u;return Trig{}(angle,c,s);
            },selected))return false;
        InstancePlaceResult next;
        if(selected.found){
            const auto* record=resolve(selected.handle);
            const auto* state=owner_.resolve(selected.handle);
            if(!record||!state||!record->instanceId||record->instanceId>0x7fffffffu)return false;
            next.id=static_cast<std::int32_t>(record->instanceId);
            next.handle=selected.handle;next.x=state->x.promoted();next.y=state->y.promoted();
        }
        result=next;return true;
    }
    bool positionMeeting(int object,MotionBinary64 x,MotionBinary64 y,bool& hit){
        if(!valid()||!runtime_)return false;
        return adapter_.pointVm(*this,object,x,y,masks_,
            [](RunnerImageValue angle,RunnerImageValue& c,RunnerImageValue& s){
                angle.bits^=0x80000000u;return Trig{}(angle,c,s);
            },hit);
    }
};
}
#endif
