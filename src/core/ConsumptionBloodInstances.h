#ifndef DOWNWELL_CORE_CONSUMPTION_BLOOD_INSTANCES_H
#define DOWNWELL_CORE_CONSUMPTION_BLOOD_INSTANCES_H
#include "ConsumptionBloodEffect.h"
#include "InstanceLifecycle.h"
#include "FailureSignal.h"
#include "AlarmBank.h"
namespace downwell {
// Object 223 ownership only; shared events implement all game behavior.
template<unsigned Capacity> class ConsumptionBloodInstances {
    static_assert(Capacity>0,"nonempty particle capacity required");
public:
    struct Record {
        std::uint32_t lifetimeId=0,sourceId=0;
        RunnerImageValue x{},y{};
        ConsumptionBloodEffect state;
        bool active=true,destroyRequested=false;
        static constexpr int objectId=223;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit ConsumptionBloodInstances(Binding binding):binding_(binding){}
    ConsumptionBloodInstances(const ConsumptionBloodInstances&)=delete;
    ConsumptionBloodInstances& operator=(const ConsumptionBloodInstances&)=delete;
    bool valid()const{return good_;}
    void observeFailure(bool& owner){good_.observe(owner);}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle key)const{
        return key.generation&&key.slot<Capacity&&records_[key.slot].lifetimeId==key.generation?&records_[key.slot]:nullptr;
    }
    InstanceHandle sourceHandle(std::uint32_t id)const{
        for(unsigned i=0;id&&i<Capacity;++i)if(records_[i].lifetimeId&&records_[i].sourceId==id)return {i,records_[i].lifetimeId};
        return {};
    }
private:
    struct Publish {
        ConsumptionBloodInstances& owner;InstanceHandle key;Record& record;
        bool imageSpeed(RunnerImageValue){return true;}
        bool imageIndex(RunnerImageValue value){return owner.binding_.imageIndex(key,record,value)||owner.fail();}
        bool position(RunnerImageValue x,RunnerImageValue y){
            if(!owner.binding_.publishedPosition(key,record,x,y))return owner.fail();
            record.x=x;record.y=y;return true;
        }
        bool destroy(){
            if(!owner.binding_.markDestroyed(key,record))return owner.fail();
            record.destroyRequested=true;return true;
        }
    };
    template<class Globals> struct Environment:Publish {
        Globals& globals;
        Environment(ConsumptionBloodInstances& o,InstanceHandle h,Record& r,Globals& g):Publish{o,h,r},globals(g){}
        bool gravity(MotionBinary64 value){return globals.gravity(value);}
        MotionBinary64 gravity(){return globals.gravity();}
        MotionBinary64 fallSpeed(){return globals.fallSpeed();}
    };
public:
    template<class Random,class Globals> bool create(std::uint32_t id,MotionBinary64 x,MotionBinary64 y,
        Random& random,Globals& globals,InstanceHandle& output){
        output={};if(!good_||!next_||!id||id>0x7fffffffu||sourceHandle(id).generation)return fail();
        unsigned slot=0;while(slot<Capacity&&records_[slot].lifetimeId)++slot;
        RunnerImageValue px,py;if(slot==Capacity||!px.assign(x)||!py.assign(y))return fail();
        auto& r=records_[slot];r={};r.lifetimeId=next_++;r.sourceId=id;r.x=px;r.y=py;
        r.state.x=px;r.state.y=py;r.state.imageSpeed={0x3f800000u};++count_;
        const InstanceHandle key{slot,r.lifetimeId};Environment<Globals> env{*this,key,r,globals};
        if(!binding_.birthAt(key,r,px,py)||!r.state.create(x,y,random,env)||!good_)return fail();
        output=key;return true;
    }
    template<class Globals> bool step(InstanceHandle key,Globals& globals){
        auto* r=resolve(key);if(!r)return fail();if(!r->active||r->destroyRequested)return true;
        Environment<Globals> env{*this,key,*r,globals};return (r->state.step(env)&&good_)||fail();
    }
    bool advanceAnimation(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();if(!r->active||r->destroyRequested)return true;
        Publish publish{*this,key,*r};return (r->state.advanceImage(publish)&&good_)||fail();
    }
    bool kill(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();Publish publish{*this,key,*r};return r->state.kill(publish)||fail();
    }
    bool advanceAlarm(InstanceHandle key,unsigned index){
        auto* r=resolve(key);if(!r||index!=0)return fail();
        if(r->active&&!r->destroyRequested&&AlarmBank::advanceValue(r->state.alarm0))return kill(key);
        return true;
    }
    bool activate(InstanceHandle key,bool active){
        auto* r=resolve(key);if(!r||r->destroyRequested||!binding_.activation(key,*r,active))return fail();
        r->active=active;return true;
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);if(!r||!r->destroyRequested||!r->state.destroyRequested||!binding_.retire(key,*r))return fail();
        *r={};--count_;return true;
    }
    bool clear(){
        for(unsigned i=0;i<Capacity;++i)if(records_[i].lifetimeId){
            if(!binding_.retire({i,records_[i].lifetimeId},records_[i]))return fail();records_[i]={};--count_;
        }
        return true;
    }
private:
    Record* resolve(InstanceHandle key){return good_?const_cast<Record*>(find(key)):nullptr;}
    bool fail(){good_=false;return false;}
    Binding binding_;Record records_[Capacity]{};std::uint32_t next_=1;unsigned count_=0;FailureSignal<> good_=true;
};
}
#endif
