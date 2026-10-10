#ifndef DOWNWELL_CORE_LEVEL_NUMBER_INSTANCES_H
#define DOWNWELL_CORE_LEVEL_NUMBER_INSTANCES_H
#include "LevelNumber.h"
#include "InstanceLifecycle.h"
#include "FailureSignal.h"
namespace downwell {
// Native object-372 storage. The enclosing world supplies source IDs and
// resource/layer registration; local generations are never source IDs.
// Alarm and Draw visits must come from mixed room/layer order, not pool order.
template<unsigned Capacity,unsigned TextCapacity=64> class LevelNumberInstances {
    static_assert(Capacity>0,"level-number ownership requires capacity");
public:
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        RunnerImageValue x{},y{};
        LevelNumber<TextCapacity> state;
        bool active=true,destroyRequested=false;
        static constexpr int objectId=372;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit LevelNumberInstances(Binding binding):binding_(binding){}
    LevelNumberInstances(const LevelNumberInstances&)=delete;
    LevelNumberInstances& operator=(const LevelNumberInstances&)=delete;
    bool valid()const{return good_;}
    void observeFailure(bool& owner){good_.observe(owner);}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle h)const{
        return h.generation&&h.slot<Capacity&&records_[h.slot].lifetimeId==h.generation?&records_[h.slot]:nullptr;
    }
    InstanceHandle sourceHandle(unsigned source)const{
        for(unsigned i=0;source&&i<Capacity;++i)if(records_[i].lifetimeId&&records_[i].sourceId==source)
            return {i,records_[i].lifetimeId};
        return {};
    }
    template<class Initialize> bool create(unsigned source,MotionBinary64 x,MotionBinary64 y,
        Initialize initialize,InstanceHandle& output){
        output={};
        if(!good_||busy_||!source||source>0x7fffffffu||!next_||sourceHandle(source).generation)return fail();
        unsigned slot=0;while(slot<Capacity&&records_[slot].lifetimeId)++slot;
        RunnerImageValue px,py;
        if(slot==Capacity||!px.assign(x)||!py.assign(y))return fail();
        auto& r=records_[slot];r={};r.lifetimeId=next_++;r.sourceId=source;r.x=px;r.y=py;++count_;
        const InstanceHandle key{slot,r.lifetimeId};
        if(!binding_.birthAt(key,r,px,py))return fail();
        busy_=true;const bool okay=initialize(key,r.state);busy_=false;
        if(!okay||!r.state.good||!r.state.created||!good_)return fail();
        output=key;return true;
    }
    bool advanceAlarm(InstanceHandle key,unsigned index){
        auto* r=resolve(key);if(!r||index>=AlarmBank::count)return fail();
        if(!r->active||r->destroyRequested)return true;
        return r->state.advanceAlarm(index,[&]{
            if(!binding_.markDestroyed(key,*r)||!good_)return false;
            r->destroyRequested=true;return true;
        })||fail();
    }
    template<class Ops> bool draw(InstanceHandle key,Ops& ops){
        auto* r=resolve(key);if(!r)return fail();
        if(!r->active||r->destroyRequested)return true;
        busy_=true;const bool okay=r->state.draw(ops);busy_=false;
        return (okay&&good_)||fail();
    }
    bool activate(InstanceHandle key,bool active){
        auto* r=resolve(key);
        if(!r||r->destroyRequested||!binding_.activation(key,*r,active)||!good_)return fail();
        r->active=active;return true;
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);
        if(!r||!r->destroyRequested||!r->state.destroyRequested)return fail();
        return erase(key,*r);
    }
    bool clear(){
        if(!good_||busy_)return fail();
        for(unsigned i=0;i<Capacity;++i)if(records_[i].lifetimeId&&!erase({i,records_[i].lifetimeId},records_[i]))return false;
        return true;
    }
private:
    bool erase(InstanceHandle key,Record& r){
        if(!binding_.retire(key,r)||!good_)return fail();
        r={};--count_;return true;
    }
    Record* resolve(InstanceHandle h){return good_&&!busy_?const_cast<Record*>(find(h)):nullptr;}
    bool fail(){good_=false;return false;}
    Binding binding_;Record records_[Capacity]{};unsigned next_=1,count_=0;
    FailureSignal<> good_=true;bool busy_=false;
};
}
#endif
