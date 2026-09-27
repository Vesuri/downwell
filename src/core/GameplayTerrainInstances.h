#ifndef DOWNWELL_CORE_GAMEPLAY_TERRAIN_INSTANCES_H
#define DOWNWELL_CORE_GAMEPLAY_TERRAIN_INSTANCES_H
#include "InstanceLifecycle.h"
#include "RoomPack.h"
#include "TerrainCreate.h"
#include "WallEndStep.h"
#include <cstring>
namespace downwell {
// Optional identity store, separate from backend occupancy/drawing caches.
// One record per wall/platform, including coincident instances. Room births
// retain authored IDs; generated births do not synthesize source instance IDs.
// Coordinates are room coordinates.
class GameplayTerrainInstances {
public:
    static constexpr unsigned capacity=2048;
    struct Record {
        int objectId=0,x=0,y=0;
        bool active=true;
        std::uint32_t lifetimeId=0;
        std::uint32_t sourceInstanceId=0; // Authored ID only; zero for generated records.
        int creationCodeId=-1;
        int spriteId=-1,imageIndex=0,imageSpeed=1;
        WallEndStepState wall{};
        bool destroyRequested=false;
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
    bool append(int object,int x,int y,bool active,std::uint32_t sourceId=0,int creationCode=-1){
        if(!good_)return false;
        if((object!=60&&object!=93)||count_==capacity||!next_)return good_=false;
        const unsigned slot=count_++;
        auto& r=records_[slot];r={object,x,y,active,next_++,sourceId,creationCode};
        // Expose the actual record to synchronous birth callbacks.
        if(!binding_.birth(handle(slot),r))return good_=false;
        return true;
    }
    bool appendRoom(const PackedRoomInstance& r){
        // These are the supported rmMain terrain birth properties. Reject
        // new transformed/pre-create variants rather than silently lose
        // their source shape. Object and room Create execute after birth.
        if(!r.instanceId||bits(r.scaleX)!=0x3f800000u||bits(r.scaleY)!=0x3f800000u||
            (bits(r.rotation)&0x7fffffffu)||bits(r.imageSpeed)!=0x3f800000u||
            (bits(r.imageIndex)&0x7fffffffu)||r.preCreateCodeId!=-1||r.flags)
            return good_=false;
        return append(r.objectId,r.x,r.y,true,r.instanceId,r.creationCodeId);
    }
    bool createProperties(InstanceHandle h,int wallTile,bool glitch,GameMakerRandom& random){
        if(!good_||!find(h))return good_=false;
        auto& r=records_[h.slot];
        if(!terrainCreateProperties(r.objectId,wallTile,glitch,random,
            [&](int sprite){if(!binding_.sprite(h,r,sprite))return false;r.spriteId=sprite;return true;},
            [&](int index){
                if(!binding_.phaseIndex(h,r,unsigned(index)*256u))return false;
                r.imageIndex=index;return true;
            },[&](int speed){r.imageSpeed=speed;return true;}))return good_=false;
        if(r.objectId==60)r.wall={};
        return true;
    }
    bool roomCreation(InstanceHandle h){
        if(!good_||!find(h))return good_=false;
        auto& r=records_[h.slot];
        if(r.creationCodeId==-1)return true;
        if(r.objectId!=60||!wallRoomCreation(r.creationCodeId,r.wall))return good_=false;
        return true;
    }
    bool requestDestroy(InstanceHandle h){
        if(!good_||!find(h))return good_=false;
        auto& r=records_[h.slot];
        if(r.destroyRequested)return true;
        // Publish collision exclusion before marking the local record. Keep
        // its address and identity alive through the remainder of the event.
        if(!binding_.markDestroyed(h,r))return good_=false;
        r.destroyRequested=true;return true;
    }
    bool activation(InstanceHandle h,bool active){
        if(!good_||!find(h))return good_=false;
        auto& r=records_[h.slot];
        if(!binding_.activation(h,r,active))return good_=false;
        r.active=active;return true;
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
    // Metadata comparison only: do not pull floating-point helpers into the
    // freestanding 68000 backend to test known binary32 defaults.
    static std::uint32_t bits(float value){
        std::uint32_t result;static_assert(sizeof(value)==sizeof(result),"binary32 required");
        std::memcpy(&result,&value,sizeof(result));return result;
    }
    Binding binding_;
    Record records_[capacity]{};
    unsigned count_=0;
    std::uint32_t next_=1;
    bool good_=true;
};
}
#endif
