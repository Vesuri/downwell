#ifndef DOWNWELL_CORE_BLAST_INSTANCES_H
#define DOWNWELL_CORE_BLAST_INSTANCES_H
#include "BlastActor.h"
#include "InstanceLifecycle.h"
#include "CallbackFailureSignal.h"
namespace downwell {
template<unsigned Capacity,unsigned HitCapacity=64> class BlastInstances {
public:
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        static constexpr int objectId=276,spriteId=471;
        RunnerImageValue x{},y{},publishedScale{0x3f800000u};
        BlastActor<HitCapacity> state;
        bool active=true,destroyRequested=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    struct Builtins {void* context=nullptr;bool (*speed)(void*,InstanceHandle,const Record&,RunnerImageValue)=nullptr;};
    explicit BlastInstances(Binding binding,Builtins builtins={}):binding_(binding),builtins_(builtins){}
    BlastInstances(const BlastInstances&)=delete;
    bool valid()const{return good_;}unsigned size()const{return count_;}
    void observeFailure(bool& owner,CallbackFailureScopes& scopes){good_.observe(owner,scopes);}
    const Record* find(InstanceHandle key)const{
        return key.generation&&key.slot<Capacity&&records_[key.slot].lifetimeId==key.generation?&records_[key.slot]:nullptr;
    }
    InstanceHandle sourceHandle(unsigned id)const{
        for(unsigned i=0;id&&i<Capacity;++i)if(records_[i].lifetimeId&&records_[i].sourceId==id)return {i,records_[i].lifetimeId};
        return {};
    }
    class Publisher {
        BlastInstances& store_;InstanceHandle key_;
    public:
        Publisher(BlastInstances& store,InstanceHandle key):store_(store),key_(key){}
        bool imageIndex(RunnerImageValue value){
            auto* r=store_.resolve(key_);if(!r||!store_.binding_.imageIndex(key_,*r,value))return store_.fail();
            r->state.imageIndex=value;return true;
        }
        bool imageSpeed(RunnerImageValue value){
            auto* r=store_.resolve(key_);if(!r)return store_.fail();
            r->state.imageSpeed=value;
            return !store_.builtins_.speed||store_.builtins_.speed(store_.builtins_.context,key_,*r,value)||store_.fail();
        }
        bool xscale(RunnerImageValue value){
            auto* r=store_.resolve(key_);
            if(!r||!store_.binding_.transform(key_,*r,RunnerTransformField::scaleX,value,{0x3f800000u},0xffffffu))return store_.fail();
            r->publishedScale=value;r->state.xscale=value;return true;
        }
        bool destroySelf(){return store_.markDestroyed(key_);}
    };
    // Publish constructor geometry before running any inherited/child Create.
    // Services provide real camera, controller and shared RNG dependencies.
    template<class Services> bool create(unsigned id,MotionBinary64 x,MotionBinary64 y,Services& services,InstanceHandle& out){
        out={};if(!good_||!id||id>0x7fffffffu||!next_||executing_.generation||!binding_.hasOwner()||sourceHandle(id).generation)return fail();
        unsigned slot=0;while(slot<Capacity&&records_[slot].lifetimeId)++slot;
        Record birth;if(slot==Capacity||!birth.x.assign(x)||!birth.y.assign(y))return fail();
        birth.sourceId=id;birth.lifetimeId=next_++;records_[slot]=birth;++count_;
        out={slot,birth.lifetimeId};executing_=out;auto& r=records_[slot];Publisher publisher(*this,out);
        struct Events {
            Services& services;Publisher& publish;
            bool shake(int a,int d){return services.shake(a,d);}bool momentDelay(){return services.momentDelay();}
            bool choose(unsigned count,unsigned& choice){return services.choose(count,choice);}
            bool imageIndex(RunnerImageValue v){return publish.imageIndex(v);}
            bool imageSpeed(RunnerImageValue v){return publish.imageSpeed(v);}
            bool xscale(RunnerImageValue v){return publish.xscale(v);}
        } events{services,publisher};
        auto status=good_.borrow();
        const bool ok=binding_.birthAt(out,r,r.x,r.y)&&good_&&r.state.create(events,status.value());
        executing_={};return good_=good_&&ok;
    }
    template<class Event> bool dispatch(InstanceHandle key,Event event){
        auto* r=resolve(key);if(!r||executing_.generation)return fail();
        if(!r->active||r->destroyRequested)return true;
        executing_=key;Publisher publish(*this,key);auto status=good_.borrow();
        const bool ok=event(r->state,publish,status.value());
        executing_={};return good_=good_&&ok;
    }
    bool advanceAnimation(InstanceHandle key){return dispatch(key,[](auto& state,auto& publish,bool& good){return state.advanceAnimation(publish,good);});}
    bool step(InstanceHandle key){return dispatch(key,[](auto& state,auto&,bool& good){return state.step(good);});}
    bool alarm(InstanceHandle key,unsigned index){return dispatch(key,[&](auto& state,auto&,bool& good){
        if(index!=0)return good=false;state.alarm0();return true;
    });}
    bool activate(InstanceHandle key,bool active){
        auto* r=resolve(key);if(!r||r->destroyRequested||!binding_.activation(key,*r,active))return fail();r->active=active;return true;
    }
    bool markDestroyed(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();if(r->destroyRequested)return true;
        if(!binding_.markDestroyed(key,*r))return fail();r->destroyRequested=true;r->state.destroyRequested=true;return true;
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);if(!r||!r->destroyRequested||same(key,executing_)||!binding_.retire(key,*r))return fail();
        *r={};--count_;return true;
    }
    bool clear(){
        if(executing_.generation)return fail();
        for(unsigned i=0;i<Capacity;++i)if(records_[i].lifetimeId){
            if(!binding_.retire({i,records_[i].lifetimeId},records_[i]))return fail();records_[i]={};--count_;
        }
        return true;
    }
private:
    static bool same(InstanceHandle a,InstanceHandle b){return a.generation&&a.slot==b.slot&&a.generation==b.generation;}
    Record* resolve(InstanceHandle key){return good_?const_cast<Record*>(find(key)):nullptr;}
    bool fail(){good_=false;return false;}
    Binding binding_;Builtins builtins_;Record records_[Capacity]{};
    unsigned next_=1,count_=0;InstanceHandle executing_{};CallbackFailureSignal good_=true;
};
}
#endif
