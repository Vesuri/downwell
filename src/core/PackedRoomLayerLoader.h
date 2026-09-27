#ifndef DOWNWELL_CORE_PACKED_ROOM_LAYER_LOADER_H
#define DOWNWELL_CORE_PACKED_ROOM_LAYER_LOADER_H
#include "RoomPack.h"
#include "RunnerLayerInstances.h"
#include "RunnerLayerIds.h"
namespace downwell {
// Prepare fixed layers and their empty instance elements BEFORE room instance
// creation. This loads the instance-membership projection in DWRM, not omitted
// background/tile/shader payloads. Use fresh owners; a failed load is terminal
// for those owners and may leave a published prefix (never retry it in place).
struct PackedRoomLayerLoader {
    // The original loads the entire room resource catalog before gameplay.
    // For this pinned build all room sequence lists are empty: catalog loading
    // raises only the loaded-layer maximum, not the dynamic-layer counter.
    // This seeds identities, not room elements or their separate ID counter.
    static bool loadCatalogIds(const RoomPack& pack,RunnerLayerIds& ids){
        if(!pack.valid())return false;
        for(unsigned i=0;i<pack.layerCount();++i){
            PackedRoomLayer layer;
            if(!pack.layer(i,layer)||layer.layerId>0x7fffffffu||
                !ids.observeLoaded(std::int32_t(layer.layerId)))return false;
        }
        return true;
    }
    template<unsigned L,unsigned E,class Publish>
    static bool load(const RoomPack& pack,int roomId,RunnerLayers<L,E>& layers,
        RunnerLayerInstances<L,E>& instances,Publish publish,RunnerLayerIds* ids=nullptr){
        if(!pack.valid()||!layers.succeeded())return false;
        bool empty=true;layers.visitLayers([&](unsigned,const auto&){empty=false;return false;});
        if(!empty)return false;
        for(unsigned i=0;i<E;++i)if(instances.get(i))return false;
        PackedRoom room;if(!pack.findRoom(roomId,room)||room.layerCount>L||room.instanceCount>E)return false;
        unsigned members=0;
        for(unsigned i=0;i<room.layerCount;++i){
            PackedRoomLayer layer;
            if(!pack.roomLayer(room,i,layer)||layer.layerId>0x7fffffffu||layer.membershipCount>E-members)return false;
            unsigned slot=L;
            if(!layers.insertAfterEquals(std::int32_t(layer.layerId),layer.depth,false,slot)||!publish(slot,layer))return false;
            for(unsigned j=0;j<layer.membershipCount;++j){
                std::uint32_t id;
                if(!pack.membership(layer.firstMembership+j,id)||!instances.reserve(layers,slot,id))return false;
                ++members;
            }
            // Original reader raises the process maximum after completing
            // each serialized layer, not by pre-scanning this room's IDs.
            if(ids&&!ids->observeLoaded(std::int32_t(layer.layerId)))return false;
        }
        if(members!=room.instanceCount)return false;
        // Every packed birth must have one reservation; reserve already rejects
        // duplicates. This also rejects dangling IDs and omitted memberships.
        for(unsigned i=0;i<room.instanceCount;++i){
            PackedRoomInstance instance;
            if(!pack.roomInstance(room,i,instance)||instances.find(instance.instanceId)==instances.End)return false;
        }
        return true;
    }
};
}
#endif
