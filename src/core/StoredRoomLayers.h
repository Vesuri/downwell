#ifndef DOWNWELL_CORE_STORED_ROOM_LAYERS_H
#define DOWNWELL_CORE_STORED_ROOM_LAYERS_H
#include "StoredLayerLifecycle.h"
#include "PackedRoomLayerLoader.h"
#include "RunnerLayerBackgrounds.h"
#include "RunnerLayerDraw.h"
namespace downwell {
// Shared stored-world room ownership. Storage remains at a fixed address;
// disposal uses bounded in-place loops, never a room-sized stack temporary.
// Room resources are discarded only after all payloads are uninitialized.
template<unsigned I,unsigned L,unsigned E,unsigned B> class StoredRoomLayers {
public:
    using Registry=RunnerLayerInstances<L,E>;
    using Layers=typename Registry::Layers;
    explicit StoredRoomLayers(RunnerLayerIds& ids):lifetimes_(layers_,registry_,ids),observer_{lifetimes_}{}
    StoredRoomLayers(const StoredRoomLayers&)=delete;
    StoredRoomLayers& operator=(const StoredRoomLayers&)=delete;
    StoredLayerLifecycle<L,E,I>& observer(){return observer_;}
    const RunnerLayerLifetimes<L,E,I>& lifetimes()const{return lifetimes_;}
    const Layers& layers()const{return layers_;}
    const Registry& instances()const{return registry_;}
    // Caller supplies source instance flags/event/sprite availability; this
    // owner supplies type, checked identity, layer visibility and mixed order.
    // Draw callbacks must obey RunnerLayerDraw's structural-mutation contract.
    template<class Lookup,class DrawInstance,class DrawBackground>
    bool visitDraw(Lookup lookup,DrawInstance instance,DrawBackground background)const{
        if(!succeeded())return false;
        return RunnerLayerDraw::visit(layers_,
            [&](unsigned slot,const auto&){return visible_[slot];},
            [](unsigned,const auto&){return true;},
            [&](unsigned e,RunnerLayerDraw::Element& out){
                const auto* entry=registry_.get(e);
                if(!entry){out.type=1;return backgrounds_.get(e)!=nullptr;}
                out.type=2;
                if(!entry->instance.generation)return true;
                const auto* state=lifetimes_.resolve(entry->instance);
                if(!state||state->sourceId!=entry->sourceId||!state->depth.attached)return false;
                if(!lookup(entry->instance,entry->sourceId,out))return false;
                out.type=2;out.instance=true;return true;
            },[&](unsigned slot,unsigned e,RunnerLayerDraw::Action action){
                const auto* entry=registry_.get(e);
                if(entry)return instance(entry->instance,entry->sourceId,layers_.get(slot)->depth,action);
                const auto* owned=backgrounds_.get(e);
                return owned&&background(owned->background,layers_.get(slot)->depth);
            },[](unsigned,const auto&){return true;});
    }
    bool succeeded()const{return good_&&lifetimes_.succeeded();}
    bool leave(){
        if(!succeeded())return false;
        return lifetimes_.uninitializeRoom([&](unsigned e){return backgrounds_.uninitialize(e);})||fail();
    }
    bool prepare(const RoomPack& pack,int room,InstanceStore<I>& instances){
        if(!succeeded()||!registry_.discardUninitialized()||!backgrounds_.discardUninitialized()||
            !layers_.discardUninitialized())return fail();
        for(auto& visible:visible_)visible=true; // New dynamic layers are visible.
        if(!PackedRoomLayerLoader::load(pack,room,layers_,registry_,[&](unsigned slot,const PackedRoomLayer& layer){
            visible_[slot]=layer.visible();
            return layer.type!=1||backgrounds_.create(layers_,slot,layer,[](unsigned){return false;});
        })||!instances.captureGlobalInstanceOrder(globals_))return fail();
        return lifetimes_.initializeRoom([&](auto visit){
            for(unsigned i=0;i<globals_.size();++i)if(!visit(globals_.handle(i)))return false;
            return true;
        },[&](unsigned e){return backgrounds_.initialize(e);})||fail();
    }
private:
    bool fail(){good_=false;return false;}
    Layers layers_;
    Registry registry_;
    RunnerLayerBackgrounds<L,E,B> backgrounds_;
    RunnerLayerLifetimes<L,E,I> lifetimes_;
    StoredLayerLifecycle<L,E,I> observer_;
    RunnerStepSnapshot<I> globals_;
    bool visible_[L?L:1]{};
    bool good_=true;
};
}
#endif
