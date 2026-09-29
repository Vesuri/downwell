#ifndef DOWNWELL_CORE_GAMEPLAY_TERRAIN_INSTANCES_H
#define DOWNWELL_CORE_GAMEPLAY_TERRAIN_INSTANCES_H
#include "InstanceLifecycle.h"
#include "RoomPack.h"
#include "TerrainCreate.h"
#include "WallEndStep.h"
#include "WallFootprint.h"
#include "Profile.h"
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
        // Owner bookkeeping, not a GML field. Free-slot preparation supplies
        // zero wall state until a room creation script or End Step writes it.
        bool wallNeedsReset=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    struct Presentation {
        void* context;
        bool (*changed)(void*,int,int,int,int); // Center, old/new sprite (-1 means hidden).
        Presentation(void* p=nullptr,bool (*fn)(void*,int,int,int,int)=nullptr):context(p),changed(fn){}
        template<class Observer> static Presentation bind(Observer& observer){
            return {&observer,[](void* p,int x,int y,int before,int after){return static_cast<Observer*>(p)->changed(x,y,before,after);}};
        }
        bool notify(int x,int y,int before,int after)const{return !changed||changed(context,x,y,before,after);}
    };
    // Unlike presentation, collision exclusion is published immediately, even
    // inside a coalesced End Step event. Birth/teardown remain the generator's
    // responsibility; this observes changes to an existing wall contribution.
    struct WallCollision {
        void* context;
        bool (*changed)(void*,int,int,bool);
        WallCollision(void* p=nullptr,bool (*fn)(void*,int,int,bool)=nullptr):context(p),changed(fn){}
        template<class Observer> static WallCollision bind(Observer& observer){
            return {&observer,[](void* p,int x,int y,bool enabled){return static_cast<Observer*>(p)->changed(x,y,enabled);}};
        }
        bool notify(int x,int y,bool enabled)const{return !changed||changed(context,x,y,enabled);}
    };
    explicit GameplayTerrainInstances(Binding binding={},Presentation presentation={},WallCollision collision={}):
        binding_(binding),presentation_(presentation),collision_(collision){}
    GameplayTerrainInstances(const GameplayTerrainInstances&)=delete;
    GameplayTerrainInstances& operator=(const GameplayTerrainInstances&)=delete;
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    // Conservative presentation candidates, in unchanged birth order. The
    // caller still tests the current sprite footprint/visibility. Coordinates
    // are immutable for these terrain lifetimes; callbacks must not mutate us.
    template<class Visit> bool visitWallRows(int first,int end,Visit visit)const{
        if(first>=end)return true;
        for(unsigned base=0;base<count_;base+=rowGroupSize){
            const auto& range=rowRanges_[base/rowGroupSize];
            if(!unboundedWallArt_ &&
                ((range.last<first&&unsigned(first)-unsigned(range.last)>=16u)||
                 (range.first>=end&&unsigned(range.first)-unsigned(end)>=16u)))continue;
            const unsigned limit=base+rowGroupSize<count_?base+rowGroupSize:count_;
            for(unsigned i=base;i<limit;++i)if(!visit(records_[order_[i]]))return false;
        }
        return true;
    }
    const Record* find(InstanceHandle h)const{
        return h.slot<capacity&&h.generation&&records_[h.slot].lifetimeId==h.generation?&records_[h.slot]:nullptr;
    }
    InstanceHandle handle(unsigned index)const{
        return index<count_?InstanceHandle{order_[index],records_[order_[index]].lifetimeId}:InstanceHandle{};
    }
    bool append(int object,int x,int y,bool active,std::uint32_t sourceId=0,int creationCode=-1){
        DW_PROFILE_SCOPE(profileTerrainRecord);
        if(!good_)return false;
        if((object!=60&&object!=93&&object!=99)||count_==capacity||!next_)return good_=false;
        unsigned slot=freeHint_;
        while(slot<capacity&&records_[slot].lifetimeId)++slot;
        if(slot==capacity)return good_=false;
        freeHint_=slot+1;order_[count_++]=slot;
        auto& r=records_[slot];
        if(!r.objectId){
            // The pool's constructor already prepared every unused slot.
            // Only these birth-specific fields differ from Record defaults.
            // Keep used slots distinguishable after retirement: recycled
            // records may contain arbitrary Create/End Step state.
            r.objectId=object;r.x=x;r.y=y;r.active=active;
            r.lifetimeId=next_++;r.sourceInstanceId=sourceId;r.creationCodeId=creationCode;
        }else r={object,x,y,active,next_++,sourceId,creationCode};
        extendRowRange(count_-1,y,(count_-1)%rowGroupSize==0);
        // Object 99 overrides Create with an empty event, retaining its
        // serialized sprite rather than inheriting objWall_n's tile setter.
        if(object==99)r.spriteId=46;
        // Expose the actual record to synchronous birth callbacks.
        if(!binding_.birth({slot,r.lifetimeId},r))return good_=false;
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
        DW_PROFILE_SCOPE(profileTerrainCreate);
        if(!good_||!find(h))return good_=false;
        auto& r=records_[h.slot];
        if(r.objectId==99)return true;
        if(!terrainCreateProperties(r.objectId,wallTile,glitch,random,
            [&](int sprite){
                if(!binding_.sprite(h,r,sprite))return false;
                r.spriteId=sprite;
                // Unknown art must retain the original uncullable path.
                if(r.objectId==60&&(wallRadius(sprite)<0||wallRadius(sprite)>16))unboundedWallArt_=true;
                return true;
            },
            [&](int index){
                if(!binding_.phaseIndex(h,r,unsigned(index)*256u))return false;
                r.imageIndex=index;return true;
            },[&](int speed){r.imageSpeed=speed;return true;}))return good_=false;
        if(r.objectId==60&&r.wallNeedsReset){r.wall={};r.wallNeedsReset=false;}
        return true;
    }
    bool roomCreation(InstanceHandle h){
        if(!good_||!find(h))return good_=false;
        auto& r=records_[h.slot];
        if(r.creationCodeId==-1)return true;
        r.wallNeedsReset=true;
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
        const int before=drawSprite(r);const bool wasVisible=visible(r);r.destroyRequested=true;
        if(wasVisible&&!collision_.notify(r.x,r.y,false))return good_=false;
        return note(r,before,-1,before>=0);
    }
    bool imageIndex(InstanceHandle h,int frame){
        if(!good_||!find(h)||frame<0||frame>15)return good_=false;
        auto& r=records_[h.slot];
        const bool changed=r.imageIndex!=frame;
        if(!binding_.phaseIndex(h,r,unsigned(frame)*256u))return good_=false;
        r.imageIndex=frame;return note(r,drawSprite(r),drawSprite(r),changed&&r.wall.checked&&visible(r));
    }
    template<class Ops> bool wallEvent(InstanceHandle h,int area,GameMakerRandom& random,Ops& ops){
        if(!good_||dispatching_||!find(h))return good_=false;
        auto& r=records_[h.slot];
        if(!r.active||r.destroyRequested)return true;
        if(r.objectId==99)return true; // Explicit empty End Step override.
        if(r.objectId!=60)return good_=false;
        const bool selected=r.wall.checked!=0;
        const bool wasVisible=visible(r);const int sprite=r.spriteId,frame=r.imageIndex,before=drawSprite(r);
        dispatching_=true;
        r.wallNeedsReset=true;
        const bool succeeded=wallEndStep(r.wall,area,random,ops);
        dispatching_=false;
        if(!succeeded||!good_)return good_=false;
        return note(r,before,drawSprite(r),wasVisible!=visible(r)||(visible(r)&&
            (selected!=(r.wall.checked!=0)||sprite!=r.spriteId||frame!=r.imageIndex)));
    }
    bool activation(InstanceHandle h,bool active){
        if(!good_||!find(h))return good_=false;
        auto& r=records_[h.slot];
        const int before=drawSprite(r);
        const bool wasVisible=visible(r);
        if(!binding_.activation(h,r,active))return good_=false;
        r.active=active;
        if(wasVisible!=visible(r)&&!collision_.notify(r.x,r.y,visible(r)))return good_=false;
        return note(r,before,drawSprite(r),before!=drawSprite(r));
    }
    // The world scheduler supplies the runner's cross-family cleanup order.
    // Do not invent a terrain-only sweep or retire during the game callback.
    bool retireMarked(InstanceHandle h){
        if(!good_||!find(h)||!records_[h.slot].destroyRequested)return good_=false;
        unsigned index=0;while(index<count_&&order_[index]!=h.slot)++index;
        if(index==count_)return good_=false;
        if(!binding_.retire(h,records_[h.slot]))return good_=false;
        records_[h.slot].lifetimeId=0;
        if(h.slot<freeHint_)freeHint_=h.slot;
        for(unsigned i=index+1;i<count_;++i){
            order_[i-1]=order_[i];
            // Only a group boundary can bring a new coordinate into a range.
            // Leaving stale extremes is conservative and avoids rescanning.
            if(i%rowGroupSize==0)extendRowRange(i-1,records_[order_[i]].y,false);
        }
        --count_;return true;
    }
    bool clear(){
        if(!good_)return false;
        unsigned retired=0;
        for(unsigned i=0;i<count_;++i){
            const auto h=handle(i);auto& r=records_[h.slot];
            if(!binding_.retire(h,r)){good_=false;break;}
            r.lifetimeId=0;++retired;
            if(h.slot<freeHint_)freeHint_=h.slot;
        }
        for(unsigned i=retired;i<count_;++i)order_[i-retired]=order_[i];
        count_-=retired;
        if(!count_)unboundedWallArt_=false;
        // A failed clear retains a compacted suffix. Preserve readable ranges
        // even for that terminal state, without changing teardown callbacks.
        for(unsigned i=0;i<count_;++i)extendRowRange(i,records_[order_[i]].y,i%rowGroupSize==0);
        return good_; // Never reset the generation sequence on Retry.
    }
private:
    static constexpr unsigned rowGroupSize=32;
    struct RowRange{int first=0,last=0;};
    void extendRowRange(unsigned index,int y,bool reset){
        auto& range=rowRanges_[index/rowGroupSize];
        if(reset)range.first=range.last=y;
        else{if(y<range.first)range.first=y;if(y>range.last)range.last=y;}
    }
    static bool visible(const Record& r){return r.objectId==60&&r.active&&!r.destroyRequested;}
    static int drawSprite(const Record& r){return visible(r)?(r.wall.checked?r.spriteId:51):-1;}
    bool note(const Record& r,int before,int after,bool changed){
        if(dispatching_)return true; // Coalesce the complete event's visual change.
        if(changed&&!presentation_.notify(r.x,r.y,before,after))return good_=false;
        return true;
    }
    // Metadata comparison only: do not pull floating-point helpers into the
    // freestanding 68000 backend to test known binary32 defaults.
    static std::uint32_t bits(float value){
        std::uint32_t result;static_assert(sizeof(value)==sizeof(result),"binary32 required");
        std::memcpy(&result,&value,sizeof(result));return result;
    }
    Binding binding_;
    Presentation presentation_;
    WallCollision collision_;
    Record records_[capacity]{};
    unsigned order_[capacity]{}; // Birth order, independent of reusable identity slots.
    RowRange rowRanges_[(capacity+rowGroupSize-1)/rowGroupSize]{};
    bool unboundedWallArt_=false;
    unsigned count_=0;
    unsigned freeHint_=0;
    std::uint32_t next_=1;
    bool good_=true;
    bool dispatching_=false;
};
}
#endif
