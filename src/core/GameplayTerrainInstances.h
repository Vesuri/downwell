#ifndef DOWNWELL_CORE_GAMEPLAY_TERRAIN_INSTANCES_H
#define DOWNWELL_CORE_GAMEPLAY_TERRAIN_INSTANCES_H
#include "InstanceLifecycle.h"
namespace downwell {
// Optional identity store, separate from backend occupancy/drawing caches.
// One record per generated wall/platform, including coincident instances.
// Coordinates are room coordinates. No source instance ID is synthesized.
class GameplayTerrainInstances {
public:
    static constexpr unsigned capacity=2048;
    struct Record {
        int objectId=0,x=0,y=0;
        bool active=true;
        std::uint32_t lifetimeId=0;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit GameplayTerrainInstances(Binding binding={}):binding_(binding){}
    GameplayTerrainInstances(const GameplayTerrainInstances&)=delete;
    GameplayTerrainInstances& operator=(const GameplayTerrainInstances&)=delete;
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle h)const{
        return h.slot<count_&&h.generation&&records_[h.slot].lifetimeId==h.generation?&records_[h.slot]:nullptr;
    }
    InstanceHandle handle(unsigned index)const{
        return index<count_?InstanceHandle{index,records_[index].lifetimeId}:InstanceHandle{};
    }
    bool append(int object,int x,int y,bool active){
        if(!good_)return false;
        if((object!=60&&object!=93)||count_==capacity||!next_)return good_=false;
        const unsigned slot=count_++;
        auto& r=records_[slot];r={object,x,y,active,next_++};
        // Expose the actual record to synchronous birth callbacks.
        if(!binding_.birth(handle(slot),r))return good_=false;
        return true;
    }
    bool clear(){
        if(!good_)return false;
        for(unsigned i=0;i<count_;++i){
            auto& r=records_[i];
            if(!binding_.retire(handle(i),r))return good_=false;
            r.lifetimeId=0;
        }
        count_=0;return true; // Never reset the generation sequence on Retry.
    }
private:
    Binding binding_;
    Record records_[capacity]{};
    unsigned count_=0;
    std::uint32_t next_=1;
    bool good_=true;
};
}
#endif
