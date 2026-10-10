#ifndef DOWNWELL_CORE_GAMEPLAY_SPAWNERS_H
#define DOWNWELL_CORE_GAMEPLAY_SPAWNERS_H
#include "InstanceLifecycle.h"
#include "SpawnerStep.h"
#include "FailureSignal.h"
namespace downwell {
class GameplaySpawners {
public:
    static constexpr unsigned capacity=8;
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        RunnerImageValue x{},y{};
        SpawnerStep state;
        bool active=true,destroyRequested=false,initialized=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit GameplaySpawners(Binding binding={}):binding_(binding){}
    bool valid()const{return good_;}
    void observeFailure(bool& owner){good_.observe(owner);}
    unsigned size()const{return count_;}
    InstanceHandle handle(unsigned slot)const{return slot<capacity?InstanceHandle{slot,records_[slot].lifetimeId}:InstanceHandle{};}
    const Record* find(InstanceHandle key)const{
        return key.generation&&key.slot<capacity&&records_[key.slot].lifetimeId==key.generation?&records_[key.slot]:nullptr;
    }
    bool createCaverns(unsigned source,int x,int y,int level,bool hard,InstanceHandle& output){
        output={};
        if(!good_||!next_||count_==capacity||!source||source>0x7fffffffu||level<1||level>3)return fail();
        for(const auto& r:records_)if(r.lifetimeId&&r.sourceId==source)return fail();
        unsigned slot=0;while(records_[slot].lifetimeId)++slot;
        auto& r=records_[slot];r={};r.lifetimeId=next_++;r.sourceId=source;++count_;
        if(!r.x.assignInteger(x)||!r.y.assignInteger(y)||!binding_.birth(handle(slot),r))return fail();
        if(!r.state.createCaverns(level,hard))return fail();
        r.initialized=true;output=handle(slot);return true;
    }
    bool mask(InstanceHandle key,int value){
        if(!good_||!find(key)||!binding_.mask(key,records_[key.slot],value))return fail();
        records_[key.slot].state.maskIndex=value;return true;
    }
    template<class Random,class Ops> bool step(InstanceHandle key,Random& random,Ops& ops){
        if(!good_||!find(key)||!records_[key.slot].initialized)return fail();
        auto& r=records_[key.slot];if(!r.active||r.destroyRequested)return true;
        return r.state.step(r.y.promoted(),random,ops)||fail();
    }
    bool meeting(InstanceHandle key,MotionBinary64 x,MotionBinary64 y,int object,bool& hit){
        if(!good_||!find(key))return fail();
        return binding_.placeMeeting(key,records_[key.slot],object,x,y,hit)||fail();
    }
    bool markDestroyed(InstanceHandle key){
        if(!good_||!find(key))return fail();
        auto& r=records_[key.slot];
        if(r.destroyRequested)return true;
        if(!binding_.markDestroyed(key,r))return fail();
        r.destroyRequested=true;return true;
    }
    bool retireMarked(InstanceHandle key){
        if(!good_||!find(key)||!records_[key.slot].destroyRequested||!binding_.retire(key,records_[key.slot]))return fail();
        records_[key.slot]={};--count_;return true;
    }
    bool clear(){
        if(!good_)return false;
        for(unsigned i=0;i<capacity;++i)if(records_[i].lifetimeId){
            if(!binding_.retire(handle(i),records_[i]))return fail();
            records_[i]={};--count_;
        }
        return true;
    }
private:
    bool fail(){good_=false;return false;}
    Record records_[capacity]{};Binding binding_;unsigned count_=0,next_=1;FailureSignal<> good_=true;
};
}
#endif
