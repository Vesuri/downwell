#ifndef DOWNWELL_CORE_DRONE_BULLET_INSTANCES_H
#define DOWNWELL_CORE_DRONE_BULLET_INSTANCES_H
#include "DroneBulletSourceStep.h"
#include "InstanceLifecycle.h"
#include "CallbackFailureSignal.h"
namespace downwell {
// Shared fixed storage. Source IDs are supplied by the room allocator and
// never inferred from local generation handles. Bindings must not relocate
// this owner. Room scheduling/order and collision selectors remain external.
template<unsigned Capacity> class DroneBulletInstances {
    static_assert(Capacity>0,"nonempty projectile store required");
public:
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        static constexpr int objectId=266;
        int spriteId=458;
        RunnerImageValue x{},y{};
        DroneBulletSourceStep state;
        unsigned hitMemCount=0;int hitMemory0=-1;bool hitIgnore=false;
        bool active=true,destroyRequested=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit DroneBulletInstances(Binding binding):binding_(binding){}
    DroneBulletInstances(const DroneBulletInstances&)=delete;
    DroneBulletInstances& operator=(const DroneBulletInstances&)=delete;
    bool valid()const{return good_;}
    void observeFailure(bool& owner,CallbackFailureScopes& scopes){good_.observe(owner,scopes);}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle h)const{
        return h.generation&&h.slot<Capacity&&records_[h.slot].lifetimeId==h.generation?&records_[h.slot]:nullptr;
    }
    bool create(unsigned sourceId,MotionBinary64 x,MotionBinary64 y,InstanceHandle& result){
        if(!good_)return false;
        if(!binding_.hasOwner()||!sourceId||sourceId>0x7fffffffu||!next_||executing_.generation)return fail();
        unsigned slot=Capacity;
        for(unsigned i=0;i<Capacity;++i){
            if(records_[i].lifetimeId&&records_[i].sourceId==sourceId)return fail();
            if(!records_[i].lifetimeId&&slot==Capacity)slot=i;
        }
        if(slot==Capacity)return fail();
        Record birth;
        if(!birth.x.assign(x)||!birth.y.assign(y)||!birth.state.reset(x,y))return fail();
        // Constructor registration precedes the Create event and its sprite
        // override. These are construction-time fields, not a finished shot.
        birth.state.imageSpeed={0x3f800000u};birth.state.speed={};birth.state.damage={};
        birth.state.decline={};birth.state.alarm0=-1;
        birth.lifetimeId=next_++;birth.sourceId=sourceId;
        records_[slot]=birth;++count_;const InstanceHandle key{slot,birth.lifetimeId};result=key;
        auto& r=records_[slot];
        executing_=key;
        if(!binding_.birthAt(key,r,r.x,r.y)||!good_){executing_={};return fail();}
        // event_inherited sets parentPBullet defaults; none consumes RNG.
        r.state.imageSpeed={};
        if(!binding_.sprite(key,r,459)||!good_){executing_={};return fail();}
        r.spriteId=459;r.state.damage=MotionBinary64::fromInteger(4);
        r.state.speed=MotionBinary64::fromInteger(7);
        r.state.decline={0x3fe99999u,0x9999999au};r.state.alarm0=16;
        executing_={};return true;
    }
    bool direction(InstanceHandle key,MotionBinary64 value){
        auto* r=resolve(key);if(!r||!value.valid())return fail();
        r->state.motion.direction=value;return true;
    }
    bool damage(InstanceHandle key,MotionBinary64 value){
        auto* r=resolve(key);if(!r||!value.valid())return fail();
        r->state.damage=value;return true;
    }
    bool markDestroyed(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();
        if(r->destroyRequested)return true;
        if(!binding_.markDestroyed(key,*r))return fail();
        r->destroyRequested=true;return true;
    }
    bool activate(InstanceHandle key,bool active){
        auto* r=resolve(key);if(!r||r->destroyRequested||!binding_.activation(key,*r,active))return fail();
        r->active=active;return true;
    }
    bool advanceAlarm(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();
        if(r->active&&!r->destroyRequested)r->state.advanceAlarm();
        return true;
    }
    class Publisher {
        DroneBulletInstances& store_;InstanceHandle key_;
    public:
        Publisher(DroneBulletInstances& store,InstanceHandle key):store_(store),key_(key){}
        bool publishAngle(RunnerImageValue angle){
            auto* r=store_.resolve(key_);
            return (r&&store_.binding_.transform(key_,*r,RunnerTransformField::angle,angle,{0x3f800000u},0xffffffu))||store_.fail();
        }
        bool publishImageSpeed(RunnerImageValue){return store_.resolve(key_)!=nullptr||store_.fail();}
        bool publishImageIndex(RunnerImageValue index){
            auto* r=store_.resolve(key_);return (r&&store_.binding_.imageIndex(key_,*r,index))||store_.fail();
        }
        bool publishPosition(RunnerImageValue x,RunnerImageValue y){
            auto* r=store_.resolve(key_);
            if(!r||!store_.binding_.publishedPosition(key_,*r,x,y))return store_.fail();
            r->x=x;r->y=y;return true;
        }
        bool destroySelf(){return store_.markDestroyed(key_);}
    };
    // A room scheduler invokes one retained lifetime at a time. Destroying
    // that lifetime does not interrupt this event or invalidate its Publisher.
    template<class Event> bool dispatch(InstanceHandle key,Event event){
        auto* r=resolve(key);if(!r||executing_.generation)return fail();
        if(!r->active||r->destroyRequested)return true;
        executing_=key;Publisher publisher(*this,key);
        auto status=good_.borrow();
        const bool ok=event(r->state,publisher,status.value());executing_={};
        return good_=good_&&ok;
    }
    bool advanceAnimation(InstanceHandle key){
        return dispatch(key,[](auto& state,auto& publisher,bool& good){return state.advanceAnimation(publisher,good);});
    }
    bool endStep(InstanceHandle key,MotionBinary64 viewY,MotionBinary64 height,bool mainRoom){
        return dispatch(key,[&](auto& state,auto& publisher,bool& good){
            struct Ops {
                DroneBulletInstances& store;InstanceHandle key;Publisher& publisher;
                MotionBinary64 top,height;bool main;
                bool readY(MotionBinary64& out){const auto* r=store.find(key);if(!r)return false;out=r->y.promoted();return true;}
                bool viewY(MotionBinary64& out){out=top;return true;}
                bool viewHeight(MotionBinary64& out){out=height;return true;}
                bool mainRoom(bool& out){out=main;return true;}
                bool destroySelf(){return publisher.destroySelf();}
            } ops{*this,key,publisher,viewY,height,mainRoom};
            return state.endStepWithView(ops,good);
        });
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);
        if(!r||!r->destroyRequested||same(key,executing_)||!binding_.retire(key,*r))return fail();
        *r={};--count_;return true;
    }
    // Explicit room teardown, including retained partial births after failure.
    // Never restart the local generation stream when reusing this owner.
    bool clear(){
        if(executing_.generation)return fail();
        for(unsigned i=0;i<Capacity;++i)if(records_[i].lifetimeId){
            const InstanceHandle key{i,records_[i].lifetimeId};
            if(!binding_.retire(key,records_[i]))return fail();
            records_[i]={};--count_;
        }
        good_=true;return true;
    }
private:
    static bool same(InstanceHandle a,InstanceHandle b){return a.slot==b.slot&&a.generation==b.generation;}
    bool fail(){good_=false;return false;}
    Record* resolve(InstanceHandle h){return good_?const_cast<Record*>(find(h)):nullptr;}
    Record records_[Capacity]{};Binding binding_;
    unsigned next_=1,count_=0;CallbackFailureSignal good_=true;InstanceHandle executing_{};
};
}
#endif
