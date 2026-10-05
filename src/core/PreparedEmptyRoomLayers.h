#ifndef DOWNWELL_CORE_PREPARED_EMPTY_ROOM_LAYERS_H
#define DOWNWELL_CORE_PREPARED_EMPTY_ROOM_LAYERS_H
#include "PackedRoomLayerLoader.h"
namespace downwell {
// Immutable resource admission, not room publication. The borrowed RoomPack
// must remain immutable. Rejected plans use the generic loader, preserving its
// exact terminal failure prefix. Publication callbacks may create backgrounds,
// but must not create instances or alter the reserved source-ID namespace.
template<unsigned L,unsigned M> class PreparedEmptyRoomLayers {
    PackedRoomLayer layers_[L]{};
    unsigned memberFirst_[L]{};
    std::uint32_t members_[M]{};
    unsigned layerCount_=0,memberCount_=0,playerId_=0;
    bool ready_=false;
public:
    bool prepare(const RoomPack& pack,int roomId,int playerObject){
        ready_=false;layerCount_=memberCount_=playerId_=0;
        PackedRoom room;
        if(!pack.valid()||!pack.findRoom(roomId,room)||room.layerCount>L||room.instanceCount>M)return false;
        for(unsigned i=0;i<room.layerCount;++i){
            auto& layer=layers_[i];
            if(!pack.roomLayer(room,i,layer)||layer.layerId>0x7fffffffu||
                layer.membershipCount>M-memberCount_)return false;
            const unsigned first=memberCount_;
            for(unsigned j=0;j<layer.membershipCount;++j){
                std::uint32_t id;
                if(!pack.membership(layer.firstMembership+j,id)||!id)return false;
                for(unsigned k=0;k<memberCount_;++k)if(members_[k]==id)return false;
                members_[memberCount_++]=id;
            }
            memberFirst_[i]=first;
            ++layerCount_;
        }
        if(memberCount_!=room.instanceCount)return false;
        for(unsigned i=0;i<room.instanceCount;++i){
            PackedRoomInstance instance;
            if(!pack.roomInstance(room,i,instance))return false;
            bool found=false;
            for(unsigned j=0;j<memberCount_;++j)if(members_[j]==instance.instanceId){found=true;break;}
            if(!found)return false;
            if(instance.objectId==playerObject){
                if(playerId_||!instance.instanceId)return false;
                playerId_=instance.instanceId;
            }
        }
        return ready_=playerId_!=0;
    }
    bool ready()const{return ready_;}
    unsigned playerSourceId()const{return playerId_;}
    template<unsigned E,unsigned I,class Storage,class Publish>
    bool publish(Storage& layers,RunnerLayerInstances<L,E,I,Storage>& instances,Publish publish)const{
        if(!ready_||!layers.succeeded()||memberCount_>E)return false;
        bool empty=true;layers.visitLayers([&](unsigned,const auto&){empty=false;return false;});
        if(!empty)return false;
        for(unsigned i=0;i<E;++i)if(instances.get(i))return false;
        for(unsigned i=0;i<layerCount_;++i){
            const auto& layer=layers_[i];unsigned slot=L;
            if(!layers.insertAfterEquals(std::int32_t(layer.layerId),layer.depth,false,slot)||
                !publish(slot,layer))return false;
            for(unsigned j=0;j<layer.membershipCount;++j)
                if(!instances.reservePrepared(layers,slot,members_[memberFirst_[i]+j]))return false;
        }
        return true;
    }
};
}
#endif
