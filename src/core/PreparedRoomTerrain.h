#ifndef DOWNWELL_CORE_PREPARED_ROOM_TERRAIN_H
#define DOWNWELL_CORE_PREPARED_ROOM_TERRAIN_H
#include "RoomPack.h"
namespace downwell {
// Resource admission only. No instances, views or random state are published.
// The borrowed pack must remain immutable and outlive this plan. Unsupported
// resources fall back to the generic loader and its original failure prefix.
class PreparedRoomTerrain {
    static constexpr unsigned capacity=256;
    PackedRoomInstance instances_[capacity];
    const RoomPack* pack_=nullptr;
    std::int32_t roomId_=0;
    unsigned first_=0,total_=0,count_=0;
    int wallEnd_=0;
public:
    bool prepare(const RoomPack& pack,const PackedRoom& room){
        pack_=nullptr;count_=0;wallEnd_=0;
        if(!pack.valid())return false;
        for(unsigned i=0;i<room.instanceCount;++i){
            PackedRoomInstance instance;
            if(!pack.roomInstance(room,i,instance))return false;
            if(instance.objectId==60){
                if(instance.y>2147483647-32)return false;
                if(instance.y+32>wallEnd_)wallEnd_=instance.y+32;
            }
            if(instance.objectId!=35&&instance.objectId!=60&&
               instance.objectId!=93&&instance.objectId!=99)continue;
            if(count_==capacity)return false;
            instances_[count_++]=instance;
        }
        roomId_=room.roomId;first_=room.firstInstance;total_=room.instanceCount;
        pack_=&pack;return true;
    }
    bool matches(const RoomPack& pack,const PackedRoom& room)const{
        return pack_==&pack&&roomId_==room.roomId&&first_==room.firstInstance&&total_==room.instanceCount;
    }
    unsigned count()const{return count_;}
    const PackedRoomInstance& at(unsigned i)const{return instances_[i];}
    int wallEnd()const{return wallEnd_;}
};
}
#endif
