#ifndef DOWNWELL_CORE_STORED_ROOM_LAYERS_H
#define DOWNWELL_CORE_STORED_ROOM_LAYERS_H
#include "StoredLayerLifecycle.h"
#include "PackedRoomLayerLoader.h"
#include "RunnerLayerBackgrounds.h"
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
    bool succeeded()const{return good_&&lifetimes_.succeeded();}
    bool leave(){
        if(!succeeded())return false;
        return lifetimes_.uninitializeRoom([&](unsigned e){return backgrounds_.uninitialize(e);})||fail();
    }
    bool prepare(const RoomPack& pack,int room,InstanceStore<I>& instances){
        if(!succeeded()||!registry_.discardUninitialized()||!backgrounds_.discardUninitialized()||
            !layers_.discardUninitialized())return fail();
        if(!PackedRoomLayerLoader::load(pack,room,layers_,registry_,[&](unsigned slot,const PackedRoomLayer& layer){
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
    bool good_=true;
};
}
#endif
