#ifndef DOWNWELL_CORE_GAMEPLAY_ROOM_LAYERS_H
#define DOWNWELL_CORE_GAMEPLAY_ROOM_LAYERS_H
#include "PackedRoomLayerLoader.h"
#include "PreparedEmptyRoomLayers.h"
#include "RunnerLayerLifetimes.h"
#include "RunnerLayerBackgrounds.h"
#include "RunnerLayerDraw.h"
#include "RunnerPreparedDrawList.h"
#include "RunnerLogicalLayerOrder.h"
namespace downwell {
template<unsigned I,class Terrain> struct GameplayLayerStorage {
    using Type=RunnerLogicalLayerOrder<64,I+256,Terrain>;
    static constexpr bool retained=true;
};
template<unsigned I> struct GameplayLayerStorage<I,void> {
    using Type=RunnerLayers<64,I+256,true>;
    static constexpr bool retained=false;
};
// Native room layer ownership, separate from the animation/membership list.
// Current gameplay rooms have no surviving native actors between entries.
// Unrecovered dynamic numeric IDs remain unknown, not slot-derived aliases.
#ifdef DOWNWELL_REFERENCE_ROOM_PREPARE
inline constexpr bool referenceRoomPreparation=true;
#else
inline constexpr bool referenceRoomPreparation=false;
#endif
template<unsigned I,bool CompactUnnumbered=false,class Terrain=void,
    bool ReferencePrepare=referenceRoomPreparation> class GameplayRoomLayers {
    static constexpr unsigned L=64,E=I+256,B=8;
    using Storage=GameplayLayerStorage<I,Terrain>;
    using Layers=typename Storage::Type;
    using Registry=RunnerLayerInstances<L,E,I,Layers>;
    RunnerLayerIds ids_;
    Layers layers_;
    Registry registry_;
    RunnerLayerBackgrounds<L,E,B,Layers> backgrounds_;
    RunnerLayerLifetimes<L,E,I,true,CompactUnnumbered,Layers> lifetimes_{layers_,registry_,ids_};
    Terrain* terrain_=nullptr;
    unsigned terrainFirst_=0;
    RunnerRetainedLayerRead retainedState(InstanceHandle h)const{
        if constexpr(Storage::retained){
            if(terrain_&&h.slot>=terrainFirst_&&h.slot-terrainFirst_<Terrain::capacity)
                return terrain_->layerState({h.slot-terrainFirst_,h.generation});
        }
        return nullptr;
    }
    const RoomPack* rooms_=nullptr;
    PreparedEmptyRoomLayers<L,256> emptyRoom_;
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
    using PreparedDraw=RunnerPreparedDrawList<L,E>;
    PreparedDraw preparedDraw_;
    bool preparedDrawEnabled_=false;
    unsigned preparedPredecessor(unsigned element)const{
        unsigned previous=layers_.previousDrawElement(element);
        while(previous!=Registry::End&&!preparedDraw_.contains(previous))
            previous=layers_.previousDrawElement(previous);
        return previous;
    }
    bool relinkPreparedDraw(){
        preparedDraw_.beginRelink();
        return layers_.visitLayersReverse([&](unsigned layer,const auto&){
            return layers_.visitElementsBackward(layer,[&](unsigned element){
                return !preparedDraw_.contains(element)||preparedDraw_.prependExisting(layer,element);
            });
        });
    }
    unsigned live_=0,playerId_=0;
    FailureSignal<> good_=true;bool prepared_=false;
    std::uint32_t drawRevision_=0;
    void changedDraw(){if(drawRevision_!=0xffffffffu)++drawRevision_;
        if constexpr(Storage::retained)if(preparedDrawEnabled_)preparedDraw_.invalidateOrder();}
    bool fail(){good_=false;return false;}
    template<class Visit> bool visitProjectedElements(unsigned layer,Visit visit)const{
        if(!layers_.get(layer))return false;
        RunnerLayerOrderKey after;bool haveAfter=false;
        for(unsigned e=layers_.firstDrawElement(layer);e!=Registry::End;){
            if(!projectionValid_[layer]){
                unsigned next=Registry::End;
                if(!layers_.visitElementsBackward(layer,[&](unsigned element){
                    // Unborn placed instances (unregistered room terrain) are
                    // skipped by Draw exactly like active retained scenery.
                    // A birth invalidates or extends this projection.
                    const auto* entry=registry_.get(element);
                    if(retainedDraw_[element]!=1&&!(entry&&!entry->instance.generation))next=element;
                    nextRelevant_[element]=std::uint16_t(next);return true;
                }))return false;
                projectionValid_[layer]=true;
            }
            e=nextRelevant_[e];if(e==Registry::End)break;
            RunnerLayerOrderKey key;
            if constexpr(Storage::retained){
                key=layers_.physicalOrder(e);
                if(layers_.cutsOff(layer,haveAfter?&after:nullptr,key))return true;
            }
            // Save the raw successor, NOT the projected one: a callback can
            // activate scenery or insert after an upcoming skipped element.
            const unsigned next=layers_.nextDrawElement(e);
            if(!visit(e))return false;
            if constexpr(Storage::retained){after=key;haveAfter=true;}
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
    template<class T> bool bindTerrain(T& terrain,unsigned first){
        static_assert(Storage::retained,"Retained layer storage required");
        if(terrain_||first>I||Terrain::capacity>I-first||!layers_.bindTerrain(terrain))return false;
        terrain_=&terrain;terrainFirst_=first;return true;
    }
    explicit GameplayRoomLayers(const RoomPack* rooms):rooms_(rooms){
        if(rooms_&&!PackedRoomLayerLoader::loadCatalogIds(*rooms_,ids_))good_=false;
        if constexpr(!ReferencePrepare)if(rooms_&&good_)emptyRoom_.prepare(*rooms_,11,35);
    }
    bool enabled()const{return rooms_!=nullptr;}
    void enablePreparedDraw(){preparedDrawEnabled_=true;}
    std::uint32_t preparedDrawRevision()const{return preparedDraw_.revision();}
    bool publishPreparedDraw(InstanceHandle h,unsigned token,RunnerLayerDraw::Action action){
        if(!preparedDrawEnabled_)return true;
        if constexpr(Storage::retained)if(auto state=retainedState(h))if(state->bound())
            return action==(state->inactive?RunnerLayerDraw::Action::endLayer:RunnerLayerDraw::Action::skip);
        const unsigned element=registry_.find(h);
        const auto prepared=action==RunnerLayerDraw::Action::endLayer?PreparedDraw::endLayer:
            action==RunnerLayerDraw::Action::drawEvent||action==RunnerLayerDraw::Action::sprite||
            action==RunnerLayerDraw::Action::alternateSprite?PreparedDraw::draw:PreparedDraw::skip;
        return preparedDraw_.publish(element,token,prepared);
    }
    template<class Draw> bool visitPreparedDraw(Draw draw)const{
        if(!preparedDrawEnabled_||!enabled()||!valid()||!prepared_)return false;
        return layers_.visitLayersReverse([&](unsigned slot,const auto& layer){
            if(!visible_[slot])return true;
            RunnerLayerOrderKey after;bool haveAfter=false;
            return preparedDraw_.visitOrdered(slot,[&](unsigned element){
                if constexpr(Storage::retained){
                    const auto key=layers_.physicalOrder(element);
                    if(layers_.cutsOff(slot,haveAfter?&after:nullptr,key))return false;
                    // Save before Draw: a callback can insert a newer head.
                    after=key;haveAfter=true;
                }
                return true;
            },[&](unsigned token){
                return draw(token,layer.encodedDepth);
            });
        });
    }
    std::uint32_t drawRevision()const{return drawRevision_;}
    bool valid()const{return good_&&lifetimes_.succeeded();}
    void observeFailure(bool& owner){good_.observe(owner);lifetimes_.observeFailure(owner);}
    bool pendingDepths()const{return enabled()&&lifetimes_.pendingDepths()!=0;}
    // Called before the new native player birth, after old lifetimes retire.
    bool prepare(){
        changedDraw();
        if(!enabled())return true;
        if(!valid()||live_)return fail();
        if(preparedDrawEnabled_)preparedDraw_.clear();
        for(auto& valid:projectionValid_)valid=false;
        if(prepared_&&!lifetimes_.uninitializeRoom([&](unsigned e){return backgrounds_.uninitialize(e);}))return fail();
        if(!registry_.discardUninitialized()||!backgrounds_.discardUninitialized()||!layers_.discardUninitialized())return fail();
        for(auto& state:retainedDraw_)state=0;
        for(auto& v:visible_)v=true;
        if constexpr(!ReferencePrepare)if(emptyRoom_.ready()){
            if(!emptyRoom_.publish(layers_,registry_,[&](unsigned slot,const PackedRoomLayer& layer){
                visible_[slot]=layer.visible();
                return layer.type!=1||backgrounds_.create(layers_,slot,layer,[](unsigned){return false;});
            }))return fail();
            playerId_=emptyRoom_.playerSourceId();
            if(!lifetimes_.initializeEmptyRoom([&](unsigned e){return backgrounds_.initialize(e);}))return fail();
            prepared_=true;return true;
        }
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
    // Either the authored builder or a compatibility-fixture player may be
    // first. Never rebuild a room that already contains the other lifetime.
    bool prepareFirstBirth(){return live_?valid()&&prepared_:prepare();}
    unsigned playerSourceId()const{return playerId_;}
    // A native always-active family has no per-instance layer elements.
    // Its first live birth resolves the layer at the source creation point;
    // the family population pins that layer until the last retirement. An
    // empty lease must resolve again: another family may erase/reuse a slot.
    struct SceneryFamily {
        unsigned layer=Layers::End,population=0;
        std::int32_t depth=0;
    };
    bool borrowSceneryFamily(SceneryFamily& family,NativeFamilyBirth& birth){
        if(!enabled()){birth.population={};return true;}
        if(!valid()||!prepared_)return fail();
        if(!family.population){
            family.layer=layers_.findDynamic(family.depth);
            if(family.layer==Layers::End){
                if(!ids_.createDynamic(layers_,family.depth,family.layer))return fail();
                projectionValid_[family.layer]=false;
            }
        }
        if(!layers_.borrowRetainedPopulation(family.layer,birth.population))return fail();
        birth.population.family=&family.population;birth.population.live=&live_;
        return true;
    }
    bool birthSceneryFamily(SceneryFamily& family){
        if(!enabled())return true;
        if(!good_||!prepared_)return fail();
        if(!family.population){
            family.layer=layers_.findDynamic(family.depth);
            if(family.layer==Layers::End){
                if(!ids_.createDynamic(layers_,family.depth,family.layer))return fail();
                projectionValid_[family.layer]=false;
            }
        }
        if(!layers_.retain(family.layer,1))return fail();
        ++family.population;++live_;return true;
    }
    bool retireSceneryFamily(SceneryFamily& family){
        if(!enabled())return true;
        if(!good_||!prepared_||!family.population||!live_||
           !layers_.releaseRetained(family.layer,1))return fail();
        if(!layers_.get(family.layer))projectionValid_[family.layer]=false;
        --family.population;--live_;return true;
    }
    // Always-active scenery has no projected Draw and never moves depth.
    // Keep its logical population (capacity and dynamic-layer lifetime), not
    // one physical element and lifetime object for every box/time field.
    bool birthScenery(RunnerImageValue depth){
        if(!enabled())return true;
        if(!valid()||!prepared_)return fail();
        std::int32_t integer;
        if(!RunnerDepthService::integerDepth(depth,integer))return fail();
        unsigned layer=layers_.findDynamic(integer);
        if(layer==Layers::End){
            if(!ids_.createDynamic(layers_,integer,layer))return fail();
            projectionValid_[layer]=false;
        }
        if(!layers_.retain(layer,1))return fail();
        ++live_;return true;
    }
    bool retireScenery(RunnerImageValue depth){
        if(!enabled())return true;
        if(!valid()||!prepared_||!live_)return fail();
        std::int32_t integer;
        if(!RunnerDepthService::integerDepth(depth,integer))return fail();
        const unsigned layer=layers_.findDynamic(integer);
        if(layer==Layers::End||!layers_.releaseRetained(layer,1))return fail();
        if(!layers_.get(layer))projectionValid_[layer]=false;
        --live_;return true;
    }
    // Borrow only during one callback-free, same-object generated terrain run.
    // Resolve/create its layer at the first cell's original attachment point.
    struct TerrainRunBirth {
        unsigned layer=Layers::End,available=0,pending=0,drawPending=0;
        bool admitted=false;
    };
    bool admitPreparedTerrainSpan(unsigned count,RunnerImageValue depth,unsigned& layer)const{
        if(!enabled()||!valid()||!prepared_)return false;
        std::int32_t integer;if(!RunnerDepthService::integerDepth(depth,integer))return false;
        layer=layers_.findDynamic(integer);
        return layer!=Layers::End&&count<=layers_.retainedAvailable(layer);
    }
    bool publishPreparedTerrainSpan(unsigned layer,unsigned count,RunnerRetainedLayerState& first){
        if(!layers_.retainConstructedSpan(layer,count,first))return fail();
        live_+=count;
        drawRevision_=drawRevision_>0xffffffffu-count?0xffffffffu:drawRevision_+count;
        if(preparedDrawEnabled_)preparedDraw_.invalidateOrder();
        return true;
    }
    bool birthInheritedTerrain(InstanceHandle h,TerrainRunBirth& run){
        ++run.drawPending;
        if(!enabled())return true;
        if(!run.admitted){run.available=layers_.retainedAvailable(run.layer);run.admitted=true;}
        // Consume logical capacity one cell at its original layer boundary;
        // no later Create runs when this cell exhausts the available prefix.
        if(!prepared_||run.pending==run.available)return fail();
        ++run.pending;terrain_->publishInheritedLayer(h.slot-terrainFirst_);return true;
    }
    bool completeInheritedTerrain(TerrainRunBirth& run){
        const unsigned count=run.pending,draw=run.drawPending;
        run.pending=run.drawPending=0;run.admitted=false;
        drawRevision_=drawRevision_>0xffffffffu-draw?0xffffffffu:drawRevision_+draw;
        if(draw&&preparedDrawEnabled_)preparedDraw_.invalidateOrder();
        if(count){
            if(!layers_.retainInheritedCount(run.layer,count))return fail();
            live_+=count;
        }
        return true;
    }
    // Only generated, unnumbered, nonprojected terrain inside its allocator's
    // fresh-birth transaction. Layer creation and ID allocation stay live.
    bool birthAllocatedTerrain(InstanceHandle h,RunnerImageValue depth,bool inactive,TerrainRunBirth* run=nullptr,
                               bool inherited=false){
        static_assert(Storage::retained,"Owned terrain layers required");
        changedDraw();if(!enabled())return true;
        if(!prepared_)return fail();
        unsigned layer=run?run->layer:Layers::End;
        if(layer==Layers::End){
            std::int32_t integer;if(!RunnerDepthService::integerDepth(depth,integer))return fail();
            layer=layers_.findDynamic(integer);
            if(layer==Layers::End&&!ids_.createDynamic(layers_,integer,layer))return fail();
            if(run)run->layer=layer;
        }
        if(!layers_.retainAllocated(h.slot-terrainFirst_,layer,inactive,inherited))return fail();
        ++live_;return true;
    }
    bool birth(InstanceHandle h,unsigned sourceId,RunnerImageValue depth,bool inactive,bool projected=true){
        changedDraw();
        if(!enabled())return true;
        if constexpr(Storage::retained)if(!sourceId&&!projected){
            if(auto state=retainedState(h)){
                if(!valid()||!prepared_||state->bound()||!RunnerSpatialSplit::valid(depth))return fail();
                std::int32_t integer;if(!RunnerDepthService::integerDepth(depth,integer))return fail();
                unsigned layer=layers_.findDynamic(integer);
                if(layer==Layers::End&&!ids_.createDynamic(layers_,integer,layer))return fail();
                if(!layers_.retainInstance({h.slot-terrainFirst_,h.generation},layer,inactive))return fail();
                ++live_;return true;
            }
        }
        if(!valid()||!prepared_||!(sourceId?lifetimes_.birth(h,sourceId,depth,inactive):
            lifetimes_.birthUnnumbered(h,depth,inactive,!projected)))return fail();
        const unsigned bit=1u<<(h.slot&7);
        if(projected)projected_[h.slot>>3]|=bit;
        else projected_[h.slot>>3]&=~bit;
        const unsigned element=registry_.find(h);
        if(element==Registry::End)return fail();
        const unsigned layer=layers_.elementOwner(element);
        if(layer==Layers::End)return fail();
        if(preparedDrawEnabled_&&!preparedDraw_.attach(layer,element,preparedPredecessor(element)))return fail();
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
        changedDraw();
        if(!enabled())return true;
        if(!valid()||!live_)return fail();
        if constexpr(Storage::retained)if(auto state=retainedState(h))if(state->bound()){
            if(!layers_.releaseInstance({h.slot-terrainFirst_,h.generation}))return fail();
            --live_;return true;
        }
        const unsigned element=registry_.find(h);
        const unsigned layer=layers_.elementOwner(element);
        const unsigned previous=layers_.previousDrawElement(element);
        const unsigned next=layers_.nextDrawElement(element);
        const bool skipped=element!=Registry::End&&retainedDraw_[element]==1;
        if(element==Registry::End||layer==Layers::End||!lifetimes_.retire(h))return fail();
        if(preparedDrawEnabled_&&!preparedDraw_.detach(layer,element))return fail();
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
        changedDraw();
        if(!enabled())return true;
        if constexpr(Storage::retained)if(auto state=retainedState(h))if(state->bound())
            return valid()&&layers_.publishInactive({h.slot-terrainFirst_,h.generation},!active);
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
        changedDraw();
        if(!enabled())return true;
        if(!valid())return false;
        if constexpr(Storage::retained)if(auto state=retainedState(h))if(state->bound())
            return layers_.reconcileRetained({h.slot-terrainFirst_,h.generation});
        int id;
        const unsigned layer=lifetimes_.attachedLayer(h,id)?layers_.find(id):Layers::End;
        if(layer==Layers::End)return fail();
        projectionValid_[layer]=false;
        if(!lifetimes_.reconcileActivation(h))return false;
        if(preparedDrawEnabled_){
            const unsigned element=registry_.find(h);
            if(!preparedDraw_.reposition(layer,element,preparedPredecessor(element)))return fail();
        }
        return true;
    }
    bool serviceDepths(){
        if(!enabled())return true;
        // Depth service can merge whole layers and dispose dynamic IDs.
        const bool pending=lifetimes_.pendingDepths()!=0;
        if(pending){
            changedDraw();for(auto& valid:projectionValid_)valid=false;
        }
        return valid()&&lifetimes_.serviceDepths()&&
            (!pending||!preparedDrawEnabled_||relinkPreparedDraw());
    }
    bool writeDepth(InstanceHandle h,MotionBinary64 value){
        // Generated terrain has no exposed depth setter in the typed owner.
        // Never route a retained handle into an unrelated physical lifetime.
        if constexpr(Storage::retained)if(auto state=retainedState(h))if(state->bound())return fail();
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
