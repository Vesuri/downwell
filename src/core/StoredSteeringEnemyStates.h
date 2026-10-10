#ifndef DOWNWELL_CORE_STORED_STEERING_ENEMY_STATES_H
#define DOWNWELL_CORE_STORED_STEERING_ENEMY_STATES_H
#include "InstanceStore.h"
#include "RunnerImageValue.h"
#include <cstring>
namespace downwell {
// Own recovered state separately from generic room records. Handles, not record
// addresses, survive synchronous child creation and room storage compaction.
// Ops publish all builtin writes at their source event boundary. Do not copy
// state wholesale to a record afterwards: that would reorder those writes.
template<unsigned WorldCapacity,unsigned Capacity,class State,int Object> class StoredSteeringEnemyStates {
    struct Entry {InstanceHandle key{};unsigned id=0;State state{};bool busy=false;};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;
    bool good_=true;
    bool alive(const Entry& e)const{
        const auto* r=records_.resolve(e.key);
        return e.id&&r&&r->objectId==Object&&r->instanceId==e.id;
    }
    Entry* find(InstanceHandle key){
        for(auto& e:entries_)if(e.key.slot==key.slot&&e.key.generation==key.generation&&alive(e))return &e;
        return nullptr;
    }
    bool fail(){good_=false;return false;}
public:
    explicit StoredSteeringEnemyStates(InstanceStore<WorldCapacity>& records):records_(records){}
    StoredSteeringEnemyStates(const StoredSteeringEnemyStates&)=delete;
    StoredSteeringEnemyStates& operator=(const StoredSteeringEnemyStates&)=delete;
    bool valid()const{return good_;}
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    bool clear(){
        for(const auto& e:entries_)if(e.busy)return fail();
        for(auto& e:entries_)e={};good_=true;return true;
    }
    const State* state(unsigned id)const{
        for(const auto& e:entries_)if(e.id==id&&alive(e))return &e.state;
        return nullptr;
    }
    template<class Random,class Ops> bool create(InstanceHandle key,Random& random,Ops& ops){
        const auto* r=records_.resolve(key);
        if(!good_||!r||r->objectId!=Object||r->destroyRequested)return fail();
        Entry* free=nullptr;
        for(auto& e:entries_){
            if(alive(e)){if(e.id==r->instanceId)return fail();}
            else if(!e.busy&&!free)free=&e;
        }
        if(!free)return fail();
        RunnerImageValue x,y;std::memcpy(&x.bits,&r->preciseX,4);std::memcpy(&y.bits,&r->preciseY,4);
        *free={};free->key=key;free->id=r->instanceId;free->busy=true;
        const bool ok=free->state.create(x,y,random,ops);
        free->busy=false;
        return (good_&&ok&&alive(*free))||fail();
    }
    template<class Event> bool dispatch(InstanceHandle key,Event event){
        auto* e=find(key);const auto* r=records_.resolve(key);
        if(!good_||!e||e->busy||!e->state.initialized)return fail();
        if(!r->active||r->destroyRequested)return true;
        e->busy=true;const bool ok=event(e->state);e->busy=false;
        // Destruction is a request; storage must survive the current event.
        return (good_&&ok&&alive(*e))||fail();
    }
    template<class Random,class Ops> bool step(InstanceHandle key,bool stop,Random& random,Ops& ops){
        return dispatch(key,[&](State& s){return s.step(stop,random,ops);});
    }
    bool hitStunAlarm(InstanceHandle key){
        // Alarm 1 has no pTimeStop guard. The room scheduler owns countdown.
        return dispatch(key,[](State& s){s.hitStun=false;return true;});
    }
};
}
#endif
