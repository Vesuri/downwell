#ifndef DOWNWELL_CORE_RUN_CONTROLLER_INSTANCES_H
#define DOWNWELL_CORE_RUN_CONTROLLER_INSTANCES_H
#include "InstanceLifecycle.h"
#include "MomentDelay.h"
#include "AlarmBank.h"
#include "PlayerDamage.h"
namespace downwell {
// Persistent object-37 lifetimes. The enclosing world supplies source IDs and
// synchronous global initialization. Room changes must retain these records;
// neither momentDelay nor this local pool allocates numeric source IDs.
template<unsigned Capacity> class RunControllerInstances {
    static_assert(Capacity>0,"controller ownership requires nonzero capacity");
public:
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        RunnerImageValue x{},y{};
        AlarmBank alarms;
        int controllerSlow=0;
        bool active=true,destroyRequested=false;
        static constexpr int objectId=37;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit RunControllerInstances(Binding binding):binding_(binding){}
    RunControllerInstances(const RunControllerInstances&)=delete;
    RunControllerInstances& operator=(const RunControllerInstances&)=delete;
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle h)const{
        return h.generation&&h.slot<Capacity&&records_[h.slot].lifetimeId==h.generation?&records_[h.slot]:nullptr;
    }
    InstanceHandle sourceHandle(unsigned source)const{
        for(unsigned i=0;source&&i<Capacity;++i)if(records_[i].lifetimeId&&records_[i].sourceId==source)
            return {i,records_[i].lifetimeId};
        return {};
    }
    template<class Initialize> bool create(unsigned source,RunnerImageValue x,RunnerImageValue y,
        Initialize initialize,InstanceHandle& result){
        result={};if(!good_||!source||source>0x7fffffffu||!next_||sourceHandle(source).generation)return fail();
        unsigned slot=0;while(slot<Capacity&&records_[slot].lifetimeId)++slot;
        if(slot==Capacity)return fail();
        auto& record=records_[slot];record={};record.lifetimeId=next_++;record.sourceId=source;
        record.x=x;record.y=y;++count_;const InstanceHandle key{slot,record.lifetimeId};
        if(!binding_.birthAt(key,record,x,y)||!initialize(key)||!good_)return fail();
        result=key;return true;
    }
    // Visit must enumerate the source membership order and supply local
    // handles. Do not replace it with slot order or stop after the first match.
    template<class Visit> bool momentDelay(int& speed,Visit visit){
        if(!good_)return false;
        return MomentDelay::request(speed,[&](auto apply){
            return visit([&](InstanceHandle key){
                auto* r=resolve(key);if(!r)return fail();
                return !r->active||r->destroyRequested||apply(*r);
            });
        })||fail();
    }
    template<class Visit,class Apply> bool visitDamageControllers(Visit visit,Apply apply){
        if(!good_)return false;
        return visit([&](InstanceHandle key){
            auto* record=resolve(key);if(!record)return fail();
            if(!record->active||record->destroyRequested)return true;
            return withPlayerDamageController([&]{return resolve(key);},apply)||fail();
        })||fail();
    }
    bool advanceAlarm(InstanceHandle key,unsigned alarm,int& speed){
        auto* r=resolve(key);if(!r||alarm>=AlarmBank::count)return fail();
        if(!r->active||r->destroyRequested)return true;
        if(!r->alarms.advanceOne(alarm))return true;
        // Other controller events require their own source implementations.
        if(alarm!=0)return fail();
        MomentDelay::alarm0(speed,*r);return true;
    }
    bool setAlarm(InstanceHandle key,unsigned alarm,int value){
        auto* r=resolve(key);if(!r||alarm>=AlarmBank::count)return fail();
        r->alarms.set(alarm,value);return true;
    }
    bool activate(InstanceHandle key,bool active){
        auto* r=resolve(key);if(!r||!binding_.activation(key,*r,active))return fail();
        r->active=active;return true;
    }
    bool clear(){
        for(unsigned i=0;i<Capacity;++i)if(records_[i].lifetimeId){
            if(!binding_.retire({i,records_[i].lifetimeId},records_[i]))return fail();
            records_[i]={};--count_;
        }
        return true;
    }
private:
    Record* resolve(InstanceHandle h){return good_?const_cast<Record*>(find(h)):nullptr;}
    bool fail(){good_=false;return false;}
    Binding binding_;Record records_[Capacity]{};unsigned next_=1,count_=0;bool good_=true;
};
}
#endif
