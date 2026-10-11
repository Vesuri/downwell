#ifndef DOWNWELL_CORE_BLOOD_INSTANCES_H
#define DOWNWELL_CORE_BLOOD_INSTANCES_H
#include "BloodEffect.h"
#include "InstanceLifecycle.h"
#include "CallbackFailureSignal.h"
namespace downwell {
// Registered object-220 blood specialization. Room numeric IDs and local
// generations are distinct. Keep BloodEffects as the scalar owner oracle.
template<unsigned Capacity> class BloodInstances {
    static_assert(Capacity>0,"nonempty blood storage required");
public:
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        static constexpr int objectId=220;
        int spriteId=-1;
        RunnerImageValue x{},y{};
        BloodEffect state;
        bool active=true,destroyRequested=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    struct Builtins {void* context=nullptr;bool (*speed)(void*,InstanceHandle,const Record&)=nullptr;};
    explicit BloodInstances(Binding binding,Builtins builtins={}):binding_(binding),builtins_(builtins){}
    BloodInstances(const BloodInstances&)=delete;
    BloodInstances& operator=(const BloodInstances&)=delete;
    bool valid()const{return good_;}
    void observeFailure(bool& owner,CallbackFailureScopes& scopes){good_.observe(owner,scopes);}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle h)const{
        return h.generation&&h.slot<Capacity&&records_[h.slot].lifetimeId==h.generation?&records_[h.slot]:nullptr;
    }
    template<class Caller> bool create(unsigned sourceId,MotionBinary64 x,MotionBinary64 y,
        MotionBinary64 angle,Caller caller,InstanceHandle& result){
        if(!good_)return false;
        if(!binding_.hasOwner()||!sourceId||sourceId>0x7fffffffu||!next_||executing_.generation)return fail();
        unsigned slot=Capacity;
        for(unsigned i=0;i<Capacity;++i){
            if(records_[i].lifetimeId&&records_[i].sourceId==sourceId)return fail();
            if(!records_[i].lifetimeId&&slot==Capacity)slot=i;
        }
        if(slot==Capacity)return fail();
        Record birth;
        if(!birth.x.assign(x)||!birth.y.assign(y))return fail();
        birth.lifetimeId=next_++;birth.sourceId=sourceId;
        birth.state.x=birth.x;birth.state.y=birth.y;
        birth.state.imageSpeed={0x3f800000u}; // Serialized instance default, before emitMovingFx.
        records_[slot]=birth;++count_;result={slot,birth.lifetimeId};executing_=result;
        auto& record=records_[slot];
        const auto complete=[&](){
            if(!binding_.birthAt(result,record,record.x,record.y)||!good_)return false;
            // Synchronous parentMovingFx Create has no random draws.
            record.state.xx=record.x.promoted();record.state.yy=record.y.promoted();
            if(!caller(result))return false; // self.myFx precedes all child setters.
            if(!binding_.sprite(result,record,84))return false;
            record.spriteId=84;record.state.imageIndex={};record.state.timeline={};
            record.state.imageSpeed={0x3f000000u};
            if(!publishSpeed(result,record))return false;
            RunnerImageValue narrowed;
            if(!narrowed.assign(angle)||!binding_.transform(result,record,RunnerTransformField::angle,
                narrowed,{0x3f800000u},0xffffffu))return false;
            record.state.imageAngle=narrowed;
            return SmokeDirection::vector(MotionBinary64::fromInteger(2),angle,
                record.state.xsp,record.state.ysp,smokeTrig);
        };
        const bool ok=complete();executing_={};return good_=good_&&ok;
    }
    bool markDestroyed(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();
        if(r->destroyRequested)return true;
        if(!binding_.markDestroyed(key,*r))return fail();
        r->destroyRequested=true;r->state.destroyRequested=true;return true;
    }
    bool activate(InstanceHandle key,bool active){
        auto* r=resolve(key);
        if(!r||r->destroyRequested||!binding_.activation(key,*r,active))return fail();
        r->active=active;return true;
    }
    template<class Query> bool step(InstanceHandle key,Query query){
        auto* r=resolve(key);if(!r||executing_.generation)return fail();
        if(!r->active||r->destroyRequested)return true;
        executing_=key;
        struct Ops {
            BloodInstances& store;InstanceHandle key;Record& record;Query& query;
            bool timeAllowed(bool& allowed){return query(static_cast<const Record&>(record),allowed);}
            bool speed(RunnerImageValue value){record.state.imageSpeed=value;return store.publishSpeed(key,record);}
            bool position(RunnerImageValue x,RunnerImageValue y){
                if(!store.binding_.publishedPosition(key,record,x,y))return false;
                record.x=x;record.y=y;return true;
            }
        } ops{*this,key,*r,query};
        auto status=good_.borrow();
        const bool ok=r->state.stepPublished(ops,status.value());executing_={};return good_=good_&&ok;
    }
    bool advanceAnimation(InstanceHandle key){
        auto* r=resolve(key);if(!r||executing_.generation)return fail();
        if(r->destroyRequested)return true;
        executing_=key;auto next=r->state;
        bool ok=next.advanceImage()&&binding_.imageIndex(key,*r,next.imageIndex);
        if(ok){
            r->state.timeline=next.timeline;r->state.imageIndex=next.imageIndex;
            if(next.destroyRequested)ok=markDestroyed(key);
        }
        executing_={};return good_=good_&&ok;
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);
        if(!r||!r->destroyRequested||same(key,executing_)||!binding_.retire(key,*r))return fail();
        *r={};--count_;return true;
    }
    bool clear(){
        if(executing_.generation)return fail();
        for(unsigned i=0;i<Capacity;++i)if(records_[i].lifetimeId){
            if(!binding_.retire({i,records_[i].lifetimeId},records_[i]))return fail();
            records_[i]={};--count_;
        }
        good_=true;return true;
    }
private:
    bool publishSpeed(InstanceHandle key,const Record& record){
        return !builtins_.speed||builtins_.speed(builtins_.context,key,record);
    }
    static bool same(InstanceHandle a,InstanceHandle b){return a.slot==b.slot&&a.generation==b.generation;}
    bool fail(){good_=false;return false;}
    Record* resolve(InstanceHandle h){return good_?const_cast<Record*>(find(h)):nullptr;}
    Record records_[Capacity]{};Binding binding_;Builtins builtins_;
    unsigned next_=1,count_=0;CallbackFailureSignal good_=true;InstanceHandle executing_{};
};
}
#endif
