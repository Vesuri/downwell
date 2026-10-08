#ifndef DOWNWELL_NATIVE_TERRAIN_LIFETIMES_H
#define DOWNWELL_NATIVE_TERRAIN_LIFETIMES_H
#include "TerrainOwnershipRuns.h"
#include "PlacementNew.h"
namespace downwell {
// Identity only: no duplicate spatial tree, room list, animation state or
// raster cache. Generated source IDs remain unknown until the shared global
// allocation stream is connected; family-local generations are never VM IDs.
template<unsigned Capacity> class NativeTerrainLifetimes {
    using Runs=TerrainOwnershipRuns<Capacity>;
    Runs runs_;
    unsigned next_=1,freeHint_=0;
    unsigned char flags_[Capacity]{};
    bool good_=true;
public:
    using Identity=typename Runs::Identity;
    bool valid()const{return good_;}
    unsigned size()const{return runs_.size();}
    unsigned runCount()const{return runs_.runCount();}
    unsigned available()const{return Capacity-size();}
    bool read(InstanceHandle h,Identity& id,bool& active,bool& destroyed)const{
        if(!runs_.read(h,id))return false;
        active=(flags_[h.slot]&2)==0;destroyed=(flags_[h.slot]&1)!=0;return true;
    }
    InstanceHandle handle(unsigned ordinal)const{return runs_.handle(ordinal);}
    template<class Visit> bool visit(Visit visit)const{
        return good_&&runs_.visit([&](const Identity& id){return visit(id,(flags_[id.handle.slot]&2)==0,
            (flags_[id.handle.slot]&1)!=0);});
    }
    bool append(int object,int x,int y,bool active=true,unsigned source=0,int creation=-1){
        if(!good_||!next_||!available())return good_=false;
        unsigned slot=freeHint_;while(slot<Capacity&&runs_.occupied(slot))++slot;
        if(slot==Capacity||!runs_.append({slot,next_},object,x,y,source,creation))return good_=false;
        ++next_;freeHint_=slot+1;flags_[slot]=active?0:2;return true;
    }
    bool canAppend(unsigned count)const{
        return good_&&count<=available()&&next_&&count<=0xffffffffu-next_;
    }
    bool markDestroyed(InstanceHandle h){
        if(!good_||!runs_.contains(h))return good_=false;
        flags_[h.slot]|=1;return true;
    }
    bool retire(InstanceHandle h){
        if(!good_||!runs_.contains(h)||!(flags_[h.slot]&1)||!runs_.retire(h))return good_=false;
        if(h.slot<freeHint_)freeHint_=h.slot;
        return true;
    }
    // Room teardown invalidates every old key without resetting generations.
    // No bulk temporary on the Amiga task stack.
    bool clear(){
        if(!good_)return false;
        runs_.~Runs();new(&runs_) Runs;freeHint_=0;return true;
    }
};
}
#endif
