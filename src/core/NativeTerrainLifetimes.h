#ifndef DOWNWELL_NATIVE_TERRAIN_LIFETIMES_H
#define DOWNWELL_NATIVE_TERRAIN_LIFETIMES_H
#include "TerrainOwnershipRuns.h"
#include "PlacementNew.h"
#include "WallEndStep.h"
#include "PackedWallState.h"
namespace downwell {
// Identity and birth chronology: no duplicate spatial tree, room list, animation state or
// raster cache. Generated source IDs remain unknown until the shared global
// allocation stream is connected; family-local generations are never VM IDs.
template<unsigned Capacity> class NativeTerrainLifetimes {
    struct Order {
        std::uint32_t stamp=0;
        Order shifted(unsigned n)const{return {stamp?stamp+n:0};}
        bool same(const Order& other)const{return stamp==other.stamp;}
    };
    using Runs=TerrainOwnershipRuns<Capacity,Order>;
    Runs runs_;
    unsigned next_=1,freeHint_=0;
    unsigned char flags_[Capacity]{};
    std::int16_t createSprite_[Capacity]{};
    PackedWallState walls_[Capacity]{};
    WallEndStepState* stepping_=nullptr;
    InstanceHandle steppingHandle_{};
    bool good_=true;
public:
    using Identity=typename Runs::Identity;
    struct Binding {
        void* context=nullptr;
        bool (*admit)(void*,unsigned)=nullptr;
        bool (*birth)(void*,const Identity&,bool,std::uint32_t&,std::uint32_t&)=nullptr;
        bool (*retire)(void*,unsigned)=nullptr;
        bool (*mark)(void*,unsigned)=nullptr;
        bool (*create)(void*,const Identity&,int,std::uint32_t&)=nullptr;
    };
    bool spatialFlags(InstanceHandle h,std::uint32_t& out)const{
        if(!contains(h))return false;
        const unsigned value=flags_[h.slot];
        out=(value&3u)|((value&4u)?8u:0u)|((value&8u)?0x200u:0u)|((value&16u)?0x8000u:0u);
        return true;
    }
    bool bind(Binding binding){
        if(!good_)return false;
        if(!binding.context&&(binding.admit||binding.birth||binding.retire||binding.mark||binding.create))return good_=false;
        if(binding_.context)return binding_.context==binding.context&&binding_.admit==binding.admit&&
            binding_.birth==binding.birth&&binding_.retire==binding.retire&&binding_.mark==binding.mark&&binding_.create==binding.create;
        if(binding.context&&(!binding.admit||!binding.birth||!binding.retire||!binding.mark||size()))return good_=false;
        binding_=binding;return true;
    }
    unsigned registrationOrder(InstanceHandle h)const{
        Order value;return good_&&runs_.publication(h,value)?value.stamp:0;
    }
    bool valid()const{return good_;}
    // -2 means constructor defaults, not the runner's missing-sprite value.
    bool appearance(InstanceHandle h,int& sprite,unsigned& index,bool& created)const{
        if(!contains(h))return false;
        created=(flags_[h.slot]&32u)!=0;sprite=createSprite_[h.slot];
        index=created&&runs_.object(h.slot)==93?23u:0u;
        if(created&&runs_.object(h.slot)==60){WallEndStepState state;if(!wallState(h,state))return false;index=state.imageIndex;}
        return true;
    }
    bool cachedSprite(InstanceHandle h,int& sprite)const{
        if(!contains(h))return false;
        sprite=(flags_[h.slot]&64u)?createSprite_[h.slot]:-2;return true;
    }
    // Geometry is immutable within either constructor or post-Create state.
    // One existing flags bit selects which resource produced cached bounds.
    bool publishRefreshedBounds(InstanceHandle h,std::uint32_t spatial){
        if(!contains(h))return good_=false;
        const unsigned previous=flags_[h.slot];
        flags_[h.slot]=(previous&163u)|((previous&32u)?64u:0u)|((spatial&8u)?4u:0u)|
            ((spatial&0x200u)?8u:0u)|((spatial&0x8000u)?16u:0u);
        return true;
    }
    bool createProperties(InstanceHandle h,int tile){
        Identity id;std::uint32_t spatial;
        if(!readIdentity(h,id)||!spatialFlags(h,spatial)||(flags_[h.slot]&32u))return good_=false;
        if(id.object!=99){
            if(tile<0||tile>32767||(binding_.context&&
                (!binding_.create||!binding_.create(binding_.context,id,tile,spatial))))return good_=false;
            createSprite_[h.slot]=static_cast<std::int16_t>(tile);
        }
        flags_[h.slot]=(flags_[h.slot]&3u)|32u|((spatial&8u)?4u:0u)|
            ((spatial&0x200u)?8u:0u)|((spatial&0x8000u)?16u:0u);
        walls_[h.slot].store(WallEndStepState{});
        return true;
    }
    // Serialized creation code runs after object Create, before the next
    // room birth. Keep its application stage in the spare flags bit and its
    // immutable operand in the existing run identity, not a per-cell record.
    bool roomCreation(InstanceHandle h){
        Identity id;
        if(stepping_||!readIdentity(h,id)||!(flags_[h.slot]&32u))return good_=false;
        WallEndStepState state=walls_[h.slot].load();
        if(id.creationCode!=-1&&(id.object!=60||!wallRoomCreation(id.creationCode,state)))return good_=false;
        if(!walls_[h.slot].store(state))return good_=false;
        flags_[h.slot]|=128u;return true;
    }
    // Initial event state only; wallState exposes retained End Step mutations.
    // Never reconstruct a previously stepped wall with this initial snapshot.
    bool initialWallState(InstanceHandle h,WallEndStepState& state)const{
        Identity id;
        if(!readIdentity(h,id)||id.object!=60||!(flags_[h.slot]&32u))return false;
        state={};
        return !(flags_[h.slot]&128u)||wallRoomCreation(id.creationCode,state);
    }
    // Read-only inspection also exposes the retained failure prefix. Execution
    // remains terminal after failure; stale handles never read a reused slot.
    bool wallState(InstanceHandle h,WallEndStepState& state)const{
        Identity id;if(!runs_.read(h,id)||id.object!=60||!(flags_[h.slot]&32u))return false;
        state=stepping_&&h.slot==steppingHandle_.slot&&h.generation==steppingHandle_.generation?
            *stepping_:walls_[h.slot].load();return true;
    }
    bool setWallDeactivation(InstanceHandle h,bool value){
        WallEndStepState state;if(!good_||!wallState(h,state))return good_=false;
        state.deactivate=value;
        if(stepping_&&h.slot==steppingHandle_.slot&&h.generation==steppingHandle_.generation)*stepping_=state;
        return walls_[h.slot].store(state)||(good_=false);
    }
    template<class Ops> bool wallEvent(InstanceHandle h,int area,GameMakerRandom& random,Ops& ops){
        Identity id;WallEndStepState state;
        if(stepping_||!readIdentity(h,id)||id.object!=60||!wallState(h,state))return good_=false;
        if(flags_[h.slot]&3u)return true;
        stepping_=&state;steppingHandle_=h;
        const bool ok=wallEndStep(state,area,random,ops);
        stepping_=nullptr;steppingHandle_={};
        const bool retained=walls_[h.slot].store(state);
        if(!ok||!retained||!good_)return good_=false;
        return true;
    }
    unsigned size()const{return runs_.size();}
    unsigned runCount()const{return runs_.runCount();}
    unsigned available()const{return Capacity-size();}
    bool contains(InstanceHandle h)const{return good_&&runs_.contains(h);}
    bool read(InstanceHandle h,Identity& id,bool& active,bool& destroyed)const{
        if(!good_||!runs_.read(h,id))return false;
        active=(flags_[h.slot]&2)==0;destroyed=(flags_[h.slot]&1)!=0;return true;
    }
    InstanceHandle handle(unsigned ordinal)const{return runs_.handle(ordinal);}
    InstanceHandle currentHandle(unsigned slot)const{return good_?runs_.currentHandle(slot):InstanceHandle{};}
    template<class Visit> bool visit(Visit visit)const{
        return good_&&runs_.visit([&](const Identity& id){return visit(id,(flags_[id.handle.slot]&2)==0,
            (flags_[id.handle.slot]&1)!=0);});
    }
    bool append(int object,int x,int y,bool active=true,unsigned source=0,int creation=-1){
        if(!canAppend(1)||(object!=60&&object!=93&&object!=99))return good_=false;
        unsigned slot=freeHint_;while(slot<Capacity&&runs_.occupied(slot))++slot;
        Order order;
        const Identity identity{{slot,next_},object,x,y,creation,source};
        std::uint32_t spatial=8u;
        if(slot==Capacity||(binding_.birth&&!binding_.birth(binding_.context,identity,active,order.stamp,spatial))||
            !runs_.append({slot,next_},object,x,y,source,creation,order))return good_=false;
        ++next_;freeHint_=slot+1;
        flags_[slot]=(active?0:2)|((spatial&8u)?4u:0u)|((spatial&0x200u)?8u:0u)|((spatial&0x8000u)?16u:0u);
        createSprite_[slot]=-2;
        walls_[slot].store(WallEndStepState{});
        return true;
    }
    bool canAppend(unsigned count)const{
        return good_&&count<=available()&&next_&&(!count||count-1<=0xffffffffu-next_)&&
            (!binding_.admit||binding_.admit(binding_.context,count));
    }
    bool markDestroyed(InstanceHandle h){
        if(!good_||!runs_.contains(h))return good_=false;
        if(flags_[h.slot]&1)return true;
        if(binding_.mark&&!binding_.mark(binding_.context,h.slot))return good_=false;
        flags_[h.slot]|=1;return true;
    }
    bool retire(InstanceHandle h){
        if(stepping_||!good_||!runs_.contains(h)||!(flags_[h.slot]&1)||
            (binding_.retire&&!binding_.retire(binding_.context,h.slot))||!runs_.retire(h))return good_=false;
        if(h.slot<freeHint_)freeHint_=h.slot;
        return true;
    }
    // Room teardown invalidates every old key without resetting generations.
    // No bulk temporary on the Amiga task stack.
    bool clear(){
        if(stepping_)return good_=false;
        if(!good_)return false;
        if(binding_.retire&&!runs_.visit([&](const Identity& id){return binding_.retire(binding_.context,id.handle.slot);}))return good_=false;
        runs_.~Runs();new(&runs_) Runs;freeHint_=0;return true;
    }
private:
    bool readIdentity(InstanceHandle h,Identity& id)const{return good_&&runs_.read(h,id);}
    Binding binding_{};
};
}
#endif
