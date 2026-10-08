#ifndef DOWNWELL_NATIVE_TERRAIN_LIFETIMES_H
#define DOWNWELL_NATIVE_TERRAIN_LIFETIMES_H
#include "TerrainOwnershipRuns.h"
#include "PlacementNew.h"
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
    bool good_=true;
public:
    struct Binding {
        void* context=nullptr;
        bool (*admit)(void*,unsigned)=nullptr;
        bool (*birth)(void*,unsigned,std::uint32_t&)=nullptr;
        bool (*retire)(void*,unsigned)=nullptr;
        bool (*mark)(void*,unsigned)=nullptr;
    };
    using Identity=typename Runs::Identity;
    bool bind(Binding binding){
        if(!good_)return false;
        if(!binding.context&&(binding.admit||binding.birth||binding.retire||binding.mark))return good_=false;
        if(binding_.context)return binding_.context==binding.context&&binding_.admit==binding.admit&&
            binding_.birth==binding.birth&&binding_.retire==binding.retire&&binding_.mark==binding.mark;
        if(binding.context&&(!binding.admit||!binding.birth||!binding.retire||!binding.mark||size()))return good_=false;
        binding_=binding;return true;
    }
    unsigned registrationOrder(InstanceHandle h)const{
        Order value;return good_&&runs_.publication(h,value)?value.stamp:0;
    }
    bool valid()const{return good_;}
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
        if(slot==Capacity||(binding_.birth&&!binding_.birth(binding_.context,slot,order.stamp))||
            !runs_.append({slot,next_},object,x,y,source,creation,order))return good_=false;
        ++next_;freeHint_=slot+1;flags_[slot]=active?0:2;return true;
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
        if(!good_||!runs_.contains(h)||!(flags_[h.slot]&1)||
            (binding_.retire&&!binding_.retire(binding_.context,h.slot))||!runs_.retire(h))return good_=false;
        if(h.slot<freeHint_)freeHint_=h.slot;
        return true;
    }
    // Room teardown invalidates every old key without resetting generations.
    // No bulk temporary on the Amiga task stack.
    bool clear(){
        if(!good_)return false;
        if(binding_.retire&&!runs_.visit([&](const Identity& id){return binding_.retire(binding_.context,id.handle.slot);}))return good_=false;
        runs_.~Runs();new(&runs_) Runs;freeHint_=0;return true;
    }
private:
    Binding binding_{};
};
}
#endif
