#ifndef DOWNWELL_CORE_STORED_WORLD_COLLISION_H
#define DOWNWELL_CORE_STORED_WORLD_COLLISION_H
#include "StartupRuntime.h"
#include "StoredCollisionLifecycle.h"
#include "RunnerObjectFamilies.h"
#include "SmokeDirection.h"
#include "SmokeTrig.h"
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
    InstanceLifecycleBinding binding(){return InstanceLifecycleBinding::bindPositionedBirths(*this);}
    const InstanceRecord* resolve(InstanceHandle h)const{
        if(!runtime_||!owner_.resolve(h))return nullptr;
        const auto* record=runtime_->instance(ids_[h.slot]);
        return record&&record->objectId==owner_.objectId(h)?record:nullptr;
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
};
}
#endif
