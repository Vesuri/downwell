#ifndef DOWNWELL_RUNNER_SPATIAL_LIFETIMES_H
#define DOWNWELL_RUNNER_SPATIAL_LIFETIMES_H
#include "InstanceHandle.h"
namespace downwell {
// Ordinary objects own their identities here. A typed terrain owner can supply
// its authoritative identities instead; no shadow object/generation array is
// reserved for that slot range. Publication is separate from typed allocation.
template<unsigned Capacity,unsigned First=0,unsigned Last=0,class Terrain=void>
class RunnerSpatialLifetimes;
template<unsigned Capacity> class RunnerSpatialLifetimes<Capacity,0,0,void> {
    struct Entry {int object=-1;std::uint32_t generation=0;};
    Entry entries_[Capacity?Capacity:1]{};
public:
    bool canAttach(InstanceHandle h,int object)const{
        return h.slot<Capacity&&h.generation&&object>=0&&entries_[h.slot].object<0&&
            h.generation>entries_[h.slot].generation;
    }
    void publish(InstanceHandle h,int object){entries_[h.slot]={object,h.generation};}
    int object(unsigned slot)const{return slot<Capacity?entries_[slot].object:-1;}
    InstanceHandle handle(unsigned slot)const{return slot<Capacity?InstanceHandle{slot,entries_[slot].generation}:InstanceHandle{};}
    bool contains(InstanceHandle h)const{return h.generation&&object(h.slot)>=0&&handle(h.slot).generation==h.generation;}
    bool rebind(unsigned slot,int object){entries_[slot].object=object;return true;}
    void retire(unsigned slot){entries_[slot].object=-1;}
    void clear(){for(auto& entry:entries_)entry.object=-1;}
};
template<unsigned Capacity,unsigned First,unsigned Last,class Terrain>
class RunnerSpatialLifetimes {
    static_assert(First<Last&&Last<=Capacity,"External terrain lifetime range");
    RunnerSpatialLifetimes<Capacity-(Last-First)> actors_;
    Terrain* terrain_=nullptr;
    static bool external(unsigned slot){return slot>=First&&slot<Last;}
    static unsigned actorSlot(unsigned slot){return slot<First?slot:slot-(Last-First);}
public:
    bool bind(Terrain& terrain){if(terrain_)return false;terrain_=&terrain;return true;}
    bool canAttach(InstanceHandle h,int object)const{
        if(h.slot>=Capacity)return false;
        return external(h.slot)?terrain_&&terrain_->spatialCanAttach({h.slot-First,h.generation},object):
            actors_.canAttach({actorSlot(h.slot),h.generation},object);
    }
    void publish(InstanceHandle h,int object){
        if(external(h.slot))terrain_->spatialPublish(h.slot-First);
        else actors_.publish({actorSlot(h.slot),h.generation},object);
    }
    int object(unsigned slot)const{
        if(slot>=Capacity)return -1;
        return external(slot)?(terrain_?terrain_->spatialObject(slot-First):-1):actors_.object(actorSlot(slot));
    }
    InstanceHandle handle(unsigned slot)const{
        if(slot>=Capacity)return {};
        const auto h=external(slot)?(terrain_?terrain_->spatialHandle(slot-First):InstanceHandle{}):actors_.handle(actorSlot(slot));
        return {slot,h.generation};
    }
    bool contains(InstanceHandle h)const{return h.generation&&object(h.slot)>=0&&handle(h.slot).generation==h.generation;}
    bool rebind(unsigned slot,int value){
        // Typed terrain objects cannot change family without changing their
        // typed identity. All current terrain binding callbacks are same-object.
        return external(slot)?object(slot)==value:actors_.rebind(actorSlot(slot),value);
    }
    void retire(unsigned slot){
        if(external(slot))terrain_->spatialRetire(slot-First);else actors_.retire(actorSlot(slot));
    }
    void clear(){actors_.clear();if(terrain_)terrain_->spatialClear();}
};
}
#endif
