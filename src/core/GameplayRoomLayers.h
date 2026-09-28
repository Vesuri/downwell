#ifndef DOWNWELL_CORE_GAMEPLAY_ROOM_LAYERS_H
#define DOWNWELL_CORE_GAMEPLAY_ROOM_LAYERS_H
#include "PackedRoomLayerLoader.h"
#include "RunnerLayerLifetimes.h"
#include "RunnerLayerBackgrounds.h"
#include "RunnerLayerDraw.h"
namespace downwell {
// Native room layer ownership, separate from the animation/membership list.
// Current gameplay rooms have no surviving native actors between entries.
// Unrecovered dynamic numeric IDs remain unknown, not slot-derived aliases.
template<unsigned I> class GameplayRoomLayers {
    static constexpr unsigned L=64,E=I+256,B=8;
    using Registry=RunnerLayerInstances<L,E>;
    using Layers=typename Registry::Layers;
    RunnerLayerIds ids_;
    Layers layers_;
    Registry registry_;
    RunnerLayerBackgrounds<L,E,B> backgrounds_;
    RunnerLayerLifetimes<L,E,I> lifetimes_{layers_,registry_,ids_};
    const RoomPack* rooms_=nullptr;
    bool visible_[L]{};
    // Actor projection only. Nonprojected scenery/fields still occupy their
    // original elements and can terminate a layer when inactive.
    std::uint8_t projected_[(I+7)/8]{};
    unsigned live_=0,playerId_=0;
    bool good_=true,prepared_=false;
    bool fail(){good_=false;return false;}
public:
    explicit GameplayRoomLayers(const RoomPack* rooms):rooms_(rooms){
        if(rooms_&&!PackedRoomLayerLoader::loadCatalogIds(*rooms_,ids_))good_=false;
    }
    bool enabled()const{return rooms_!=nullptr;}
    bool valid()const{return good_&&lifetimes_.succeeded();}
    // Called before the new native player birth, after old lifetimes retire.
    bool prepare(){
        if(!enabled())return true;
        if(!valid()||live_)return fail();
        if(prepared_&&!lifetimes_.uninitializeRoom([&](unsigned e){return backgrounds_.uninitialize(e);}))return fail();
        if(!registry_.discardUninitialized()||!backgrounds_.discardUninitialized()||!layers_.discardUninitialized())return fail();
        for(auto& v:visible_)v=true;
        if(!PackedRoomLayerLoader::load(*rooms_,11,layers_,registry_,[&](unsigned slot,const PackedRoomLayer& layer){
            visible_[slot]=layer.visible();
            return layer.type!=1||backgrounds_.create(layers_,slot,layer,[](unsigned){return false;});
        }))return fail();
        PackedRoom room;if(!rooms_->findRoom(11,room))return fail();
        playerId_=0;
        for(unsigned i=0;i<room.instanceCount;++i){
            PackedRoomInstance r;if(!rooms_->roomInstance(room,i,r))return fail();
            if(r.objectId==35){if(playerId_||!r.instanceId)return fail();playerId_=r.instanceId;}
        }
        if(!playerId_||!lifetimes_.initializeRoom([](auto){return true;},
            [&](unsigned e){return backgrounds_.initialize(e);}))return fail();
        prepared_=true;return true;
    }
    unsigned playerSourceId()const{return playerId_;}
    bool birth(InstanceHandle h,unsigned sourceId,RunnerImageValue depth,bool inactive,bool projected=true){
        if(!enabled())return true;
        if(!valid()||!prepared_||!(sourceId?lifetimes_.birth(h,sourceId,depth,inactive):
            lifetimes_.birthUnnumbered(h,depth,inactive)))return fail();
        const unsigned bit=1u<<(h.slot&7);
        if(projected)projected_[h.slot>>3]|=bit;
        else projected_[h.slot>>3]&=~bit;
        ++live_;return true;
    }
    bool retire(InstanceHandle h){
        if(!enabled())return true;
        if(!valid()||!live_||!lifetimes_.retire(h))return fail();
        projected_[h.slot>>3]&=~(1u<<(h.slot&7));
        --live_;return true;
    }
    bool activation(InstanceHandle h,bool active){return !enabled()||(valid()&&lifetimes_.publishInactive(h,!active));}
    bool reconcile(InstanceHandle h){return !enabled()||(valid()&&lifetimes_.reconcileActivation(h));}
    bool serviceDepths(){return !enabled()||(valid()&&lifetimes_.serviceDepths());}
    bool writeDepth(InstanceHandle h,MotionBinary64 value){
        return enabled()&&valid()&&lifetimes_.writeDepth(h,value);
    }
    template<class Lookup,class Draw> bool visit(Lookup lookup,Draw draw)const{
        if(!enabled()||!valid()||!prepared_)return false;
        return RunnerLayerDraw::visit(layers_,[&](unsigned slot,const auto&){return visible_[slot];},
            [](unsigned,const auto&){return true;},[&](unsigned e,RunnerLayerDraw::Element& out){
                const auto* entry=registry_.get(e);
                if(!entry){out.type=1;return backgrounds_.get(e)!=nullptr;}
                out.type=2;if(!entry->instance.generation)return true;
                const auto* live=lifetimes_.resolve(entry->instance);
                if(!live||!live->depth.attached||live->sourceId!=entry->sourceId)return false;
                if(!(projected_[entry->instance.slot>>3]&(1u<<(entry->instance.slot&7)))){
                    // With object=false only the inactive flag affects the
                    // shared dispatch decision. Keep that flag live, including
                    // the interval before activation relinks this element.
                    out.instance=true;out.flags=live->inactive?2u:0u;return true;
                }
                if(!lookup(entry->instance,out))return false;
                out.type=2;out.instance=true;return true;
            },[&](unsigned slot,unsigned e,auto){
                const auto* entry=registry_.get(e);
                // Retained terrain/background composition is still a separate
                // renderer pass; this visitor projects owned actor elements.
                return !entry||draw(entry->instance,layers_.get(slot)->depth);
            },[](unsigned,const auto&){return true;});
    }
};
}
#endif
