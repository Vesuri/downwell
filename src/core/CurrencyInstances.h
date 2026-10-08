#ifndef DOWNWELL_CORE_CURRENCY_INSTANCES_H
#define DOWNWELL_CORE_CURRENCY_INSTANCES_H
#include "CurrencyCreate.h"
#include "InstanceLifecycle.h"
namespace downwell {
// Native currency lifetimes. Numeric source IDs are supplied by the room birth
// stream; never manufacture them from family-local generation handles.
template<unsigned Capacity> class CurrencyInstances {
public:
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        int spriteId=133,maskId=-1;
        RunnerImageValue x{},y{},imageIndex{};
        CurrencyCreate state;
        bool active=true,destroyRequested=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit CurrencyInstances(Binding binding):binding_(binding){}
    CurrencyInstances(const CurrencyInstances&)=delete;
    CurrencyInstances& operator=(const CurrencyInstances&)=delete;
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle h)const{
        return h.generation&&h.slot<Capacity&&records_[h.slot].lifetimeId==h.generation?&records_[h.slot]:nullptr;
    }
    template<class Random,class Queries> bool create(unsigned sourceId,MotionBinary64 x,MotionBinary64 y,
        int area,bool attractor,Random& random,Queries& queries,InstanceHandle& output){
        if(!good_)return false;
        if(!binding_.hasOwner()||!sourceId||sourceId>0x7fffffffu||!next_||executing_)return fail();
        unsigned slot=Capacity;
        for(unsigned i=0;i<Capacity;++i){
            if(records_[i].lifetimeId&&records_[i].sourceId==sourceId)return fail();
            if(!records_[i].lifetimeId&&slot==Capacity)slot=i;
        }
        if(slot==Capacity)return fail();
        Record birth;if(!birth.x.assign(x)||!birth.y.assign(y))return fail();
        birth.state.x=birth.x;birth.state.y=birth.y;birth.state.imageSpeed={0x3f800000u};
        birth.sourceId=sourceId;birth.lifetimeId=next_++;
        records_[slot]=birth;++count_;const InstanceHandle key{slot,birth.lifetimeId};
        auto& r=records_[slot];executing_=true;
        struct Adapter {
            CurrencyInstances& owner;InstanceHandle key;Record& r;Queries& queries;
            bool wall(MotionBinary64 x,MotionBinary64 y,bool& hit){return queries.wall(key,x,y,hit);}
            bool tile(MotionBinary64 x,MotionBinary64 y,bool& hit){return queries.tile(key,100,x,y,hit);}
            bool publish(RunnerImageValue x,RunnerImageValue y){
                if(!owner.binding_.publishedPosition(key,r,x,y))return false;
                r.x=x;r.y=y;return true;
            }
        } adapter{*this,key,r,queries};
        const bool ok=binding_.birthAt(key,r,r.x,r.y)&&r.state.create(x,y,area,attractor,random,adapter);
        executing_=false;
        if(!ok||!good_)return fail();
        output=key;return true;
    }
    template<class Event> bool dispatch(InstanceHandle key,Event event){
        auto* r=resolve(key);if(!r||executing_)return fail();executing_=true;
        const bool ok=event(r->state,good_);executing_=false;return good_=good_&&ok;
    }
    bool markDestroyed(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();
        if(r->destroyRequested)return true;
        if(!binding_.markDestroyed(key,*r))return fail();r->destroyRequested=true;return true;
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);
        if(!r||executing_||!r->destroyRequested||!binding_.retire(key,*r))return fail();
        *r={};--count_;return true;
    }
    bool clear(){
        if(executing_)return fail();
        for(unsigned i=0;i<Capacity;++i)if(records_[i].lifetimeId){
            if(!binding_.retire({i,records_[i].lifetimeId},records_[i]))return fail();records_[i]={};--count_;
        }
        return true;
    }
private:
    Record* resolve(InstanceHandle h){return good_?const_cast<Record*>(find(h)):nullptr;}
    bool fail(){good_=false;return false;}
    Record records_[Capacity]{};unsigned next_=1,count_=0;bool good_=true,executing_=false;Binding binding_;
};
}
#endif
