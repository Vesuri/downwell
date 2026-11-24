#ifndef DOWNWELL_CORE_SMOKE_INSTANCES_H
#define DOWNWELL_CORE_SMOKE_INSTANCES_H
#include "SmokeEffect.h"
#include "SmokeDirection.h"
#include "SmokeTrigNarrow.h"
#include "InstanceLifecycle.h"
#include "FailureSignal.h"
#include "AlarmBank.h"
namespace downwell {
// Native smoke lifetimes: source IDs are supplied by the room, never inferred
// from the family's generation handles. The room owns phase ordering.
template<unsigned Capacity> class SmokeInstances {
    static_assert(Capacity>0,"nonempty smoke instance capacity required");
public:
    struct Record {
        std::uint32_t lifetimeId=0,sourceId=0;
        RunnerImageValue x{},y{};
        SmokeEffect state;
        int alarm0=-1;
        bool active=true,destroyRequested=false;
        static constexpr int objectId=225;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit SmokeInstances(Binding binding={}):binding_(binding){}
    SmokeInstances(const SmokeInstances&)=delete;
    SmokeInstances& operator=(const SmokeInstances&)=delete;
    bool valid()const{return good_;}
    void observeFailure(bool& owner){good_.observe(owner);}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle key)const{
        return key.generation&&key.slot<Capacity&&records_[key.slot].lifetimeId==key.generation?&records_[key.slot]:nullptr;
    }
    InstanceHandle sourceHandle(std::uint32_t id)const{
        for(unsigned i=0;id&&i<Capacity;++i)if(records_[i].lifetimeId&&records_[i].sourceId==id)
            return {i,records_[i].lifetimeId};
        return {};
    }
    template<class Random> bool create(std::uint32_t id,MotionBinary64 x,MotionBinary64 y,Random& random,InstanceHandle& output){
        output={};if(!good_||!next_||!id||id>0x7fffffffu||sourceHandle(id).generation)return fail();
        unsigned slot=0;while(slot<Capacity&&records_[slot].lifetimeId)++slot;
        RunnerImageValue px,py;if(slot==Capacity||!px.assign(x)||!py.assign(y))return fail();
        auto& r=records_[slot];r={};r.lifetimeId=next_++;r.sourceId=id;r.x=px;r.y=py;
        r.state.x=px;r.state.y=py;r.state.imageSpeed={0x3f800000u};++count_;
        const InstanceHandle key{slot,r.lifetimeId};
        if(!binding_.birthAt(key,r,px,py)||!r.state.create(x,y,random))return fail();
        output=key;return true;
    }
    template<class Random> bool step(InstanceHandle key,Random& random){
        auto* r=resolve(key);if(!r)return fail();if(!r->active||r->destroyRequested)return true;
        struct Publish {
            SmokeInstances& owner;InstanceHandle key;Record& r;
            bool imageSpeed(RunnerImageValue){return true;} // State already owns the builtin.
            bool position(RunnerImageValue x,RunnerImageValue y){
                if(!owner.binding_.publishedPosition(key,r,x,y))return owner.fail();
                r.x=x;r.y=y;return true;
            }
        } publish{*this,key,*r};
        bool okay=true;
        return r->state.stepPublished(random,[](auto speed,auto direction,auto& x,auto& y){
            return SmokeDirection::vector(speed,direction,x,y,smokeTrigNarrowed);
        },publish,smokeEscapedDistance,okay)||fail();
    }
    bool setDirection(InstanceHandle key,MotionBinary64 value){
        auto* r=resolve(key);if(!r||!value.valid())return fail();
        r->state.direction=value;return true;
    }
    bool setSpeed(InstanceHandle key,MotionBinary64 value){
        auto* r=resolve(key);if(!r||!value.valid())return fail();
        r->state.speed=value;return true;
    }
    bool advanceAnimation(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();if(!r->active||r->destroyRequested)return true;
        if(!r->state.advanceImage()||!binding_.imageIndex(key,*r,r->state.imageIndex))return fail();
        if(r->state.destroyRequested){
            if(!binding_.markDestroyed(key,*r))return fail();r->destroyRequested=true;
        }
        return true;
    }
    bool alarmEvent(InstanceHandle key,unsigned index){
        auto* r=resolve(key);if(!r||index!=0)return fail();r->state.alarm0();return true;
    }
    bool setAlarm0(InstanceHandle key,int value){auto* r=resolve(key);if(!r)return fail();r->alarm0=value;return true;}
    bool advanceAlarm(InstanceHandle key,unsigned index){
        auto* r=resolve(key);if(!r||index!=0)return fail();
        if(r->active&&!r->destroyRequested&&AlarmBank::advanceValue(r->alarm0))r->state.alarm0();
        return true;
    }
    bool wallContact(InstanceHandle key){auto* r=resolve(key);return (r&&r->state.collideWall())||fail();}
    bool activate(InstanceHandle key,bool active){
        auto* r=resolve(key);if(!r||r->destroyRequested||!binding_.activation(key,*r,active))return fail();
        r->active=active;return true;
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);
        if(!r||!r->destroyRequested||!r->state.destroyRequested||!binding_.retire(key,*r))return fail();
        *r={};--count_;return true;
    }
    bool clear(){
        for(unsigned i=0;i<Capacity;++i)if(records_[i].lifetimeId){
            if(!binding_.retire({i,records_[i].lifetimeId},records_[i]))return fail();
            records_[i]={};--count_;
        }
        return true;
    }
private:
    Record* resolve(InstanceHandle key){return good_?const_cast<Record*>(find(key)):nullptr;}
    bool fail(){good_=false;return false;}
    Binding binding_;Record records_[Capacity]{};std::uint32_t next_=1;
    unsigned count_=0;FailureSignal<> good_=true;
};
}
#endif
