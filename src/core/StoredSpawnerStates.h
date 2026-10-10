#ifndef DOWNWELL_CORE_STORED_SPAWNER_STATES_H
#define DOWNWELL_CORE_STORED_SPAWNER_STATES_H
#include "InstanceStore.h"
#include "SpawnerStep.h"
#include <cstring>
namespace downwell {
// State for actual stored-room Spawner lifetimes. Does not allocate source IDs
// or room records: synchronous Create supplies the already-published handle.
// Keep the large channel arrays out of every generic InstanceRecord.
template<unsigned WorldCapacity,unsigned Capacity> class StoredSpawnerStates {
    struct Entry {InstanceHandle key{};unsigned sourceId=0;SpawnerStep state;};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;
    bool alive(const Entry& entry)const{
        const auto* r=records_.resolve(entry.key);
        return r&&r->objectId==78&&r->instanceId==entry.sourceId;
    }
public:
    explicit StoredSpawnerStates(InstanceStore<WorldCapacity>& records):records_(records){}
    StoredSpawnerStates(const StoredSpawnerStates&)=delete;
    StoredSpawnerStates& operator=(const StoredSpawnerStates&)=delete;
    void clear(){for(auto& e:entries_)e={};}
    const SpawnerStep* state(unsigned id)const{
        for(const auto& e:entries_)if(id&&e.sourceId==id&&alive(e))return &e.state;
        return nullptr;
    }
    bool createCaverns(InstanceHandle key,int level,bool hard){
        const auto* r=records_.resolve(key);
        if(!r||r->objectId!=78||level<1||level>3)return false;
        Entry* free=nullptr;
        for(auto& e:entries_){
            if(!alive(e)){if(!free)free=&e;continue;}
            if(e.sourceId==r->instanceId)return false;
        }
        if(!free)return false;
        *free={};free->key=key;free->sourceId=r->instanceId;
        return free->state.createCaverns(level,hard);
    }
    template<class Random,class Ops> bool step(InstanceHandle key,Random& random,Ops& ops){
        for(auto& e:entries_)if(e.key.slot==key.slot&&e.key.generation==key.generation&&alive(e)){
            const auto* r=records_.resolve(key);
            if(!r->active||r->destroyRequested)return true;
            RunnerImageValue y;std::memcpy(&y.bits,&r->preciseY,4);
            return e.state.step(y.promoted(),random,ops);
        }
        return false;
    }
};
}
#endif
