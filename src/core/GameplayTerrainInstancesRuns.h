#ifndef DOWNWELL_GAMEPLAY_TERRAIN_INSTANCES_RUNS_H
#define DOWNWELL_GAMEPLAY_TERRAIN_INSTANCES_RUNS_H
#include "GameplayTerrainInstancesScalar.h"
#include "TerrainOwnershipRuns.h"
#include <cstddef>
namespace downwell {
class GameplayTerrainInstancesRuns {
public:
    static constexpr unsigned capacity=2048;
    using Record=GameplayTerrainInstancesScalar::Record;
    using Binding=RecordLifecycleBinding<Record>;
    using Presentation=GameplayTerrainInstancesScalar::Presentation;
    using WallCollision=GameplayTerrainInstancesScalar::WallCollision;
    class Read {
        const GameplayTerrainInstancesRuns* owner_=nullptr;
        InstanceHandle handle_{};Record value_{};
    public:
        Read()=default;
        Read(decltype(nullptr)){}
        Read(const GameplayTerrainInstancesRuns* owner,InstanceHandle h,const Record& value):owner_(owner),handle_(h),value_(value){}
        explicit operator bool()const{return owner_!=nullptr;}
        const Record* operator->()const{return &value_;}
        const Record& operator*()const{return value_;}
        bool operator==(const Read& b)const{return owner_==b.owner_&&handle_.slot==b.handle_.slot&&handle_.generation==b.handle_.generation;}
        bool operator!=(const Read& b)const{return !(*this==b);}
    };
private:
    struct Mutable {
        int spriteId=-1,imageIndex=0,imageSpeed=1;
        WallEndStepState wall{};
        bool active=true,destroyRequested=false,wallNeedsReset=false;
        // Replaces the old ever-used byte: pristine, allocated, published,
        // spatially retired. Typed slot reuse resets all mutable state.
        std::uint8_t allocation=0;
    };
    using Ids=TerrainOwnershipRuns<capacity>;
    using Identity=typename Ids::Identity;
    Binding binding_;Presentation presentation_;WallCollision collision_;
    Ids identities_;
    Mutable mutable_[capacity]{};
    unsigned freeHint_=0;std::uint32_t next_=1;
    bool good_=true,dispatching_=false,unboundedWallArt_=false;
    Record record(const Identity& id)const{
        const auto& m=mutable_[id.handle.slot];
        Record r{id.object,id.x,id.y,m.active,id.handle.generation,id.source,id.creationCode};
        r.spriteId=m.spriteId;r.imageIndex=m.imageIndex;r.imageSpeed=m.imageSpeed;
        r.wall=m.wall;r.destroyRequested=m.destroyRequested;r.wallNeedsReset=m.wallNeedsReset;return r;
    }
    static bool visible(const Identity& id,const Mutable& m){return id.object==60&&m.active&&!m.destroyRequested;}
    static int drawSprite(const Identity& id,const Mutable& m){return visible(id,m)?(m.wall.checked?m.spriteId:51):-1;}
    bool note(const Identity& id,int before,int after,bool changed){
        if(dispatching_)return true;
        if(changed&&!presentation_.notify(id.x,id.y,before,after))return good_=false;
        return true;
    }
    bool admitted(InstanceHandle h,Identity& id){return good_&&identities_.read(h,id)?true:(good_=false);}
    static std::uint32_t bits(float value){std::uint32_t result;std::memcpy(&result,&value,sizeof(result));return result;}
public:
    explicit GameplayTerrainInstancesRuns(Binding b={},Presentation p={},WallCollision c={}):binding_(b),presentation_(p),collision_(c){}
    GameplayTerrainInstancesRuns(const GameplayTerrainInstancesRuns&)=delete;
    GameplayTerrainInstancesRuns& operator=(const GameplayTerrainInstancesRuns&)=delete;
    bool valid()const{return good_;}
    unsigned size()const{return identities_.size();}
    bool spatialCanAttach(InstanceHandle h,int object)const{
        return identities_.contains(h)&&mutable_[h.slot].allocation==1&&identities_.object(h.slot)==object;
    }
    void spatialPublish(unsigned slot){mutable_[slot].allocation=2;}
    void spatialRetire(unsigned slot){mutable_[slot].allocation=3;}
    void spatialClear(){for(auto& m:mutable_)if(m.allocation==2)m.allocation=3;}
    int spatialObject(unsigned slot)const{
        return slot<capacity&&mutable_[slot].allocation==2?identities_.object(slot):-1;
    }
    InstanceHandle spatialHandle(unsigned slot)const{
        return spatialObject(slot)>=0?identities_.currentHandle(slot):InstanceHandle{};
    }
    InstanceHandle handle(unsigned ordinal)const{return identities_.handle(ordinal);}
    bool read(InstanceHandle h,Record& out)const{
        Identity id;if(!identities_.read(h,id))return false;out=record(id);return true;
    }
    Read find(InstanceHandle h)const{Identity id;if(!identities_.read(h,id))return nullptr;return {this,h,record(id)};}
    bool ownsIdentity(InstanceHandle h,const Record& r)const{
        Identity id;return identities_.read(h,id)&&r.lifetimeId==h.generation&&
            r.objectId==id.object&&r.x==id.x&&r.y==id.y&&r.sourceInstanceId==id.source&&r.creationCodeId==id.creationCode;
    }
    template<class Visit> bool visitWallRows(int first,int end,Visit visit)const{
        if(first>=end)return true;
        const auto expand=[&](const Identity& id){return visit(record(id));};
        return unboundedWallArt_?identities_.visit(expand):identities_.visitRows(first,end,expand);
    }
    bool append(int object,int x,int y,bool active,std::uint32_t source=0,int code=-1){
        DW_PROFILE_SCOPE(profileTerrainRecord);
        if(!good_)return false;
        if((object!=60&&object!=93&&object!=99)||size()==capacity||!next_)return good_=false;
        unsigned slot=freeHint_;while(slot<capacity&&identities_.occupied(slot))++slot;
        if(slot==capacity)return good_=false;
        const InstanceHandle h{slot,next_};
        if(!identities_.append(h,object,x,y,source,code))return good_=false;
        ++next_;freeHint_=slot+1;
        auto& m=mutable_[slot];if(m.allocation)m=Mutable{};
        m.allocation=1;m.active=active;if(object==99)m.spriteId=46;
        if(!binding_.birth(h,record({h,object,x,y,code,source})))return good_=false;
        return true;
    }
    bool appendRoom(const PackedRoomInstance& r){
        if(!r.instanceId||bits(r.scaleX)!=0x3f800000u||bits(r.scaleY)!=0x3f800000u||
            (bits(r.rotation)&0x7fffffffu)||bits(r.imageSpeed)!=0x3f800000u||
            (bits(r.imageIndex)&0x7fffffffu)||r.preCreateCodeId!=-1||r.flags)return good_=false;
        return append(r.objectId,r.x,r.y,true,r.instanceId,r.creationCodeId);
    }
    bool createProperties(InstanceHandle h,int tile,bool glitch,GameMakerRandom& random){
        DW_PROFILE_SCOPE(profileTerrainCreate);
        Identity id;if(!admitted(h,id))return false;auto& m=mutable_[h.slot];
        if(id.object==99)return true;
        if(!terrainCreateProperties(id.object,tile,glitch,random,[&](int sprite){
            if(!binding_.sprite(h,record(id),sprite))return false;
            m.spriteId=sprite;
            if(id.object==60&&(wallRadius(sprite)<0||wallRadius(sprite)>16))unboundedWallArt_=true;
            return true;
        },[&](int index){
            if(!binding_.phaseIndex(h,record(id),unsigned(index)*256u))return false;
            m.imageIndex=index;return true;
        },[&](int speed){m.imageSpeed=speed;return true;}))return good_=false;
        if(id.object==60&&m.wallNeedsReset){m.wall={};m.wallNeedsReset=false;}return true;
    }
    bool roomCreation(InstanceHandle h){
        Identity id;if(!admitted(h,id))return false;auto& m=mutable_[h.slot];
        if(id.creationCode==-1)return true;m.wallNeedsReset=true;
        if(id.object!=60||!wallRoomCreation(id.creationCode,m.wall))return good_=false;return true;
    }
    bool requestDestroy(InstanceHandle h){
        Identity id;if(!admitted(h,id))return false;auto& m=mutable_[h.slot];
        if(m.destroyRequested)return true;
        if(!binding_.markDestroyed(h,record(id)))return good_=false;
        const int before=drawSprite(id,m);const bool wasVisible=visible(id,m);m.destroyRequested=true;
        if(wasVisible&&!collision_.notify(id.x,id.y,false))return good_=false;
        return note(id,before,-1,before>=0);
    }
    bool imageIndex(InstanceHandle h,int frame){
        Identity id;if(!admitted(h,id)||frame<0||frame>15)return good_=false;auto& m=mutable_[h.slot];
        const bool changed=m.imageIndex!=frame;
        if(!binding_.phaseIndex(h,record(id),unsigned(frame)*256u))return good_=false;
        m.imageIndex=frame;return note(id,drawSprite(id,m),drawSprite(id,m),changed&&m.wall.checked&&visible(id,m));
    }
    template<class Ops> bool wallEvent(InstanceHandle h,int area,GameMakerRandom& random,Ops& ops){
        Identity id;if(dispatching_||!admitted(h,id))return good_=false;auto& m=mutable_[h.slot];
        if(!m.active||m.destroyRequested||id.object==99)return true;
        if(id.object!=60)return good_=false;
        const bool selected=m.wall.checked!=0,wasVisible=visible(id,m);
        const int sprite=m.spriteId,frame=m.imageIndex,before=drawSprite(id,m);
        dispatching_=true;m.wallNeedsReset=true;
        const bool succeeded=wallEndStep(m.wall,area,random,ops);dispatching_=false;
        if(!succeeded||!good_)return good_=false;
        return note(id,before,drawSprite(id,m),wasVisible!=visible(id,m)||(visible(id,m)&&
            (selected!=(m.wall.checked!=0)||sprite!=m.spriteId||frame!=m.imageIndex)));
    }
    bool activation(InstanceHandle h,bool active){
        Identity id;if(!admitted(h,id))return false;auto& m=mutable_[h.slot];
        const int before=drawSprite(id,m);const bool wasVisible=visible(id,m);
        if(!binding_.activation(h,record(id),active))return good_=false;
        m.active=active;
        if(wasVisible!=visible(id,m)&&!collision_.notify(id.x,id.y,visible(id,m)))return good_=false;
        return note(id,before,drawSprite(id,m),before!=drawSprite(id,m));
    }
    bool retireMarked(InstanceHandle h){
        Identity id;if(!admitted(h,id)||!mutable_[h.slot].destroyRequested)return good_=false;
        if(!binding_.retire(h,record(id)))return good_=false;
        if(!identities_.retire(h))return good_=false;
        if(h.slot<freeHint_)freeHint_=h.slot;return true;
    }
    bool clear(){
        if(!good_)return false;
        while(size()){
            const auto h=handle(0);Identity id;identities_.read(h,id);
            if(!binding_.retire(h,record(id)))return good_=false;
            if(!identities_.retire(h))return good_=false;
            if(h.slot<freeHint_)freeHint_=h.slot;
        }
        unboundedWallArt_=false;return true;
    }
};
}
#endif
