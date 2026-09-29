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
template<unsigned I,bool CompactUnnumbered=false> class GameplayRoomLayers {
    static constexpr unsigned L=64,E=I+256,B=8;
    using Registry=RunnerLayerInstances<L,E,I>;
    using Layers=typename Registry::Layers;
    RunnerLayerIds ids_;
    Layers layers_;
    Registry registry_;
    RunnerLayerBackgrounds<L,E,B> backgrounds_;
    RunnerLayerLifetimes<L,E,I,true,CompactUnnumbered> lifetimes_{layers_,registry_,ids_};
    const RoomPack* rooms_=nullptr;
    bool visible_[L]{};
    // Actor projection only. Nonprojected scenery/fields still occupy their
    // original elements and can terminate a layer when inactive.
    std::uint8_t projected_[(I+7)/8]{};
    // 0 = resolve actor/placeholder, 1 = retained active scenery (skip),
    // 2 = retained inactive scenery (end layer). Published at mutations, not
    // guessed from visibility. Element identity survives depth moves/merges.
    std::uint8_t retainedDraw_[E]{};
    static_assert(E<65535,"Compact scenery-run links require 16-bit elements");
    mutable std::uint16_t nextRelevant_[E]{};
    mutable bool projectionValid_[L]{};
    unsigned live_=0,playerId_=0;
    bool good_=true,prepared_=false;
    bool fail(){good_=false;return false;}
    template<class Visit> bool visitProjectedElements(unsigned layer,Visit visit)const{
        if(!layers_.get(layer))return false;
        for(unsigned e=layers_.firstDrawElement(layer);e!=Registry::End;){
            if(!projectionValid_[layer]){
                unsigned next=Registry::End;
                if(!layers_.visitElementsBackward(layer,[&](unsigned element){
                    if(retainedDraw_[element]!=1)next=element;
                    nextRelevant_[element]=std::uint16_t(next);return true;
                }))return false;
                projectionValid_[layer]=true;
            }
            e=nextRelevant_[e];if(e==Registry::End)break;
            // Save the raw successor, NOT the projected one: a callback can
            // activate scenery or insert after an upcoming skipped element.
            const unsigned next=layers_.nextDrawElement(e);
            if(!visit(e))return false;
            e=next;
        }
        return true;
    }
    struct DrawProjection {
        const GameplayRoomLayers& owner;
        bool succeeded()const{return owner.layers_.succeeded();}
        template<class Visit> bool visitLayersReverse(Visit visit)const{return owner.layers_.visitLayersReverse(visit);}
        template<class Visit> bool visitDrawElements(unsigned layer,Visit visit)const{return owner.visitProjectedElements(layer,visit);}
    };
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
        for(auto& valid:projectionValid_)valid=false;
        if(prepared_&&!lifetimes_.uninitializeRoom([&](unsigned e){return backgrounds_.uninitialize(e);}))return fail();
        if(!registry_.discardUninitialized()||!backgrounds_.discardUninitialized()||!layers_.discardUninitialized())return fail();
        for(auto& state:retainedDraw_)state=0;
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
            lifetimes_.birthUnnumbered(h,depth,inactive,!projected)))return fail();
        const unsigned bit=1u<<(h.slot&7);
        if(projected)projected_[h.slot>>3]|=bit;
        else projected_[h.slot>>3]&=~bit;
        const unsigned element=registry_.find(h);
        if(element==Registry::End)return fail();
        const unsigned layer=layers_.elementOwner(element);
        if(layer==Layers::End)return fail();
        retainedDraw_[element]=projected?0:(inactive?2:1);
        // A head insertion changes no existing element's forward skip link.
        // Extend the warmed projection instead of walking all retained terrain
        // again. Reserved elements elsewhere retain the general rebuild path.
        if(projectionValid_[layer]&&layers_.firstDrawElement(layer)==element){
            const unsigned next=layers_.nextDrawElement(element);
            nextRelevant_[element]=std::uint16_t(retainedDraw_[element]!=1?element:
                next==Registry::End?Registry::End:nextRelevant_[next]);
        }else projectionValid_[layer]=false;
        ++live_;return true;
    }
    bool retire(InstanceHandle h){
        if(!enabled())return true;
        if(!valid()||!live_)return fail();
        const unsigned element=registry_.find(h);
        const unsigned layer=layers_.elementOwner(element);
        const unsigned previous=layers_.previousDrawElement(element);
        const unsigned next=layers_.nextDrawElement(element);
        const bool skipped=element!=Registry::End&&retainedDraw_[element]==1;
        if(element==Registry::End||layer==Layers::End||!lifetimes_.retire(h))return fail();
        retainedDraw_[element]=0;
        projected_[h.slot>>3]&=~(1u<<(h.slot&7));
        // Removing skipped scenery changes no cached target. Removing a
        // relevant element changes only the preceding contiguous scenery run;
        // repair that run instead of invalidating every retained wall in the
        // layer. A cold projection remains cold. Disposed dynamic slots must
        // be invalidated before reuse, even when this was the last element.
        if(!layers_.get(layer))projectionValid_[layer]=false;
        else if(projectionValid_[layer]&&!skipped){
            const auto target=std::uint16_t(next==Registry::End?Registry::End:nextRelevant_[next]);
            for(unsigned e=previous;e!=Registry::End&&retainedDraw_[e]==1;e=layers_.previousDrawElement(e))
                nextRelevant_[e]=target;
        }
        --live_;return true;
    }
    bool activation(InstanceHandle h,bool active){
        if(!enabled())return true;
        if(!valid()||!lifetimes_.publishInactive(h,!active))return false;
        if(!(projected_[h.slot>>3]&(1u<<(h.slot&7)))){
            const unsigned element=registry_.find(h);
            if(element==Registry::End)return fail();
            const unsigned layer=layers_.elementOwner(element);
            if(layer==Layers::End)return fail();
            const unsigned state=active?1:2;
            if(retainedDraw_[element]!=state){
                retainedDraw_[element]=std::uint8_t(state);
                projectionValid_[layer]=false;
            }
        }
        return true;
    }
    bool reconcile(InstanceHandle h){
        if(!enabled())return true;
        if(!valid())return false;
        int id;
        const unsigned layer=lifetimes_.attachedLayer(h,id)?layers_.find(id):Layers::End;
        if(layer==Layers::End)return fail();
        projectionValid_[layer]=false;return lifetimes_.reconcileActivation(h);
    }
    bool serviceDepths(){
        if(!enabled())return true;
        // Depth service can merge whole layers and dispose dynamic IDs.
        if(lifetimes_.pendingDepths())for(auto& valid:projectionValid_)valid=false;
        return valid()&&lifetimes_.serviceDepths();
    }
    bool writeDepth(InstanceHandle h,MotionBinary64 value){
        return enabled()&&valid()&&lifetimes_.writeDepth(h,value);
    }
    template<class Lookup,class Draw> bool visit(Lookup lookup,Draw draw)const{
        if(!enabled()||!valid()||!prepared_)return false;
        return RunnerLayerDraw::visit(DrawProjection{*this},[&](unsigned slot,const auto&){return visible_[slot];},
            [](unsigned,const auto&){return true;},[&](unsigned e,RunnerLayerDraw::Element& out){
                if(retainedDraw_[e]){
                    out.type=2;out.instance=true;
                    out.flags=retainedDraw_[e]==2?2u:0u;return true;
                }
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
