#ifndef DOWNWELL_CORE_STORED_BOX_CHAIN_PUBLICATION_H
#define DOWNWELL_CORE_STORED_BOX_CHAIN_PUBLICATION_H
#include "InstanceStore.h"
#include "StoredDroneBulletRegistration.h"
#include "InstancePlaceResult.h"
namespace downwell {
// Object-92 chain operations for the registered stored world. Create returns
// a world lifetime; the drone registry's local key never becomes a VM value.
template<unsigned WorldCapacity,unsigned Bullets,class Collision>
class StoredBoxChainPublication {
    InstanceStore<WorldCapacity>& records_;
    StoredDroneBulletRegistration<WorldCapacity,Bullets>& bullets_;
    Collision& collision_;InstanceHandle self_;bool& good_;
    bool fail(){good_=false;return false;}
    InstanceRecord* caller(){
        auto* r=records_.resolve(self_);
        return good_&&bullets_.belongsTo(records_)&&r&&r->objectId==92&&collision_.resolve(self_)==r?r:nullptr;
    }
    template<class Write> bool bulletWrite(InstanceHandle h,Write write){
        const auto* r=records_.resolve(h);
        if(!caller()||!r||r->objectId!=266)return fail();
        const auto local=bullets_.handle(*r);
        return (local.generation&&bullets_.dispatch(local,write))||fail();
    }
public:
    using Handle=InstanceHandle;
    StoredBoxChainPublication(InstanceStore<WorldCapacity>& records,
        StoredDroneBulletRegistration<WorldCapacity,Bullets>& bullets,Collision& collision,InstanceHandle self,bool& good)
        :records_(records),bullets_(bullets),collision_(collision),self_(self),good_(good){}
    bool create(int object,MotionBinary64 x,MotionBinary64 y,Handle& result){
        if(!caller()||object!=266)return fail();
        Handle local;
        if(!bullets_.create(x,y,local))return fail();
        const auto h=bullets_.worldHandle(local);
        if(!h.generation)return fail();
        result=h;return true;
    }
    bool myBullet(Handle h){
        auto* r=caller();const auto* child=records_.resolve(h);
        if(!r||!child||child->objectId!=266||!bullets_.handle(*child).generation)return fail();
        r->boxMyBullet=static_cast<std::int32_t>(child->instanceId);return true;
    }
    bool bulletDirection(Handle h,int value){return bulletWrite(h,[&](auto& state,auto&,bool&){
        state.motion.direction=MotionBinary64::fromInteger(value);return true;});}
    bool bulletDamage(Handle h,int value){return bulletWrite(h,[&](auto& state,auto&,bool&){
        state.damage=MotionBinary64::fromInteger(value);return true;});}
    bool place(int object,MotionBinary64 x,MotionBinary64 y,Handle& target,bool& truth){
        auto* r=caller();InstancePlaceResult selected;
        if(!r||object!=92||!collision_.instancePlace(self_,*r,object,x,y,selected))return fail();
        target=selected.handle;truth=selected.id!=-4;return true;
    }
    bool target(Handle h){
        auto* r=caller();if(!r)return fail();
        if(!h.generation&&h.slot==Handle{}.slot){r->boxTarget=-4;return true;}
        const auto* selected=records_.resolve(h);
        if(!selected||selected->objectId!=92)return fail();
        r->boxTarget=static_cast<std::int32_t>(selected->instanceId);return true;
    }
    bool targetAlarm(Handle h,int alarm,int value){
        auto* r=records_.resolve(h);
        if(!caller()||!r||r->objectId!=92||alarm!=0)return fail();
        r->alarms.set(0,value);return true;
    }
    bool targetChecked(Handle h,bool value){
        auto* r=records_.resolve(h);
        if(!caller()||!r||r->objectId!=92)return fail();
        r->box.checked=value;return true;
    }
};
}
#endif
