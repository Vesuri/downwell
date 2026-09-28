#ifndef DOWNWELL_CORE_DRONE_BULLET_INSTANCES_VERIFICATION_H
#define DOWNWELL_CORE_DRONE_BULLET_INSTANCES_VERIFICATION_H
#include "DroneBulletInstances.h"
#include "DroneBulletVerification.h"
namespace downwell {
struct DroneBulletInstancesVerification {
    using Store=DroneBulletInstances<2>;
    struct Owner {
        unsigned calls=0,fail=0,births=0,sprites=0,moves=0,marks=0,retirements=0;
        bool good=true;
        bool call(){return good&&++calls!=fail;}
        bool birth(InstanceHandle,const Store::Record&){return false;}
        bool birthAt(InstanceHandle h,const Store::Record& r,RunnerImageValue x,RunnerImageValue y){
            ++births;good=good&&h.generation==r.lifetimeId&&r.sourceId>=100000&&r.spriteId==458&&
                r.state.imageSpeed.bits==0x3f800000u&&r.state.speed.zero()&&r.state.damage.zero()&&
                x.bits==r.x.bits&&y.bits==r.y.bits;
            return call();
        }
        bool sprite(InstanceHandle,const Store::Record& r,int sprite){
            ++sprites;good=good&&r.spriteId==458&&sprite==459&&r.state.imageSpeed.bits==0&&r.state.speed.zero();return call();
        }
        bool move(InstanceHandle,const Store::Record&,RunnerImageValue,RunnerImageValue){++moves;return call();}
        bool imageIndex(InstanceHandle,const Store::Record&,RunnerImageValue){return call();}
        bool transform(InstanceHandle,const Store::Record&,RunnerTransformField,RunnerImageValue,RunnerImageValue,std::uint32_t){return call();}
        bool retire(InstanceHandle,const Store::Record&){++retirements;return call();}
        bool markDestroyed(InstanceHandle,const Store::Record& r){++marks;good=good&&!r.destroyRequested;return call();}
        bool activation(InstanceHandle,const Store::Record&,bool){return call();}
        bool membership(InstanceHandle,const Store::Record&,bool){return call();}
        template<class... Args> bool mask(Args&&...){return false;}
        template<class... Args> bool placeMeeting(Args&&...){return false;}
        template<class... Args> bool instancePlace(Args&&...){return false;}
        template<class... Args> bool instanceNumber(Args&&...){return false;}
    };
    static bool run(){
        Owner owner;Store store(Store::Binding::bindPositionedBirths(owner));InstanceHandle first;
        if(!store.create(105187,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),first)||
            owner.births!=1||owner.sprites!=1||store.find(first)->spriteId!=459||
            store.find(first)->state.alarm0!=16||store.find(first)->hitMemory0!=-1)return false;
        unsigned events=0;
        if(!store.activate(first,false)||!store.advanceAlarm(first)||!store.advanceAnimation(first)||
            !store.dispatch(first,[&](auto&,auto&,bool&){++events;return true;})||events||store.find(first)->state.alarm0!=16||
            !store.activate(first,true)||!store.direction(first,MotionBinary64::fromInteger(90))||
            !store.damage(first,MotionBinary64::fromInteger(20)))return false;
        DroneBulletVerification::Random random;
        if(!store.dispatch(first,[&](auto& state,auto& publisher,bool& good){
            struct Ops:Store::Publisher {
                explicit Ops(Store::Publisher p):Store::Publisher(p){}
                bool solid(DroneBulletSourceStep&){return destroySelf();}
            } ops(publisher);
            return state.step(random,ops,good);
        })||owner.moves!=2||owner.marks!=1||!store.find(first)->destroyRequested||
            store.find(first)->y.bits!=0x43fc8000u||!store.markDestroyed(first)||owner.marks!=1)return false;
        if(!store.advanceAlarm(first)||!store.advanceAnimation(first)||
            !store.dispatch(first,[&](auto&,auto&,bool&){++events;return true;})||events||
            !store.retire(first)||store.find(first)||store.size())return false;
        InstanceHandle second;
        if(!store.create(105188,{}, {},second)||second.slot!=first.slot||second.generation<=first.generation||
            store.direction(first,MotionBinary64::fromInteger(180))||store.valid()||
            !store.find(second)->state.motion.direction.zero()||!store.clear()||!store.valid()||store.size())return false;
        InstanceHandle third;
        if(!store.create(105189,{}, {},third)||third.generation<=second.generation)return false;
        if(store.dispatch(third,[&](auto&,auto&,bool&){return store.clear();})||store.size()!=1||
            !store.find(third)||!store.clear()||store.size())return false;
        for(unsigned fail=1;fail<=2;++fail){
            Owner rejecting;rejecting.fail=fail;Store partial(Store::Binding::bindPositionedBirths(rejecting));InstanceHandle key;
            if(partial.create(105187,{}, {},key)||partial.valid()||partial.size()!=1||!partial.find(key)||
                partial.find(key)->spriteId!=458||partial.find(key)->state.imageSpeed.bits!=(fail==1?0x3f800000u:0u)||
                partial.create(105188,{}, {},key)||rejecting.calls!=fail||!partial.clear()||partial.size())return false;
        }
        for(unsigned limit=0;limit<2;++limit){
            Owner bounded;Store full(Store::Binding::bindPositionedBirths(bounded));InstanceHandle one,two,rejected{99,99};
            if(!full.create(105187,{}, {},one))return false;
            if(limit&&!full.create(105188,{}, {},two))return false;
            const auto calls=bounded.calls;
            if(full.create(limit?105189:105187,{}, {},rejected)||full.valid()||bounded.calls!=calls||
                full.size()!=limit+1||rejected.slot!=99||rejected.generation!=99||!full.clear())return false;
        }
        Store unbound({});InstanceHandle missing;
        if(unbound.create(105187,{}, {},missing)||unbound.size()||missing.generation)return false;
        return true;
    }
};
}
#endif
