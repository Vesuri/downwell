#ifndef DOWNWELL_CORE_PLAYER_CHILD_CREATION_H
#define DOWNWELL_CORE_PLAYER_CHILD_CREATION_H
#include <cstdint>
namespace downwell {
// scrPlayerInit's companion births, then objPlayer_n Create's meter/camera
// and room-11 entry wipe. Each create callback must complete child Create
// synchronously and return its captured source ID, not an allocator cursor.
// A rejected operation retains the accepted prefix and stops later births.
struct PlayerChildCreation {
    template<class Create,class DroneNumber,class ReverseDrone,class Camera,class Entry>
    static bool run(int room,int x,int y,unsigned balloon,unsigned drones,
        Create create,DroneNumber droneNumber,ReverseDrone reverseDrone,Camera camera,Entry entry){
        if(balloon&&!create(256,x,y))return false;
        if(!droneNumber(0))return false;
        if(drones){
            if(!create(249,x,y)||!droneNumber(1))return false;
            if(drones==2){
                const std::uint32_t id=create(249,x,y);
                if(!id||!reverseDrone(id))return false;
            }
        }
        if(!create(66,-100,y))return false;
        const std::uint32_t id=create(64,x,y);
        if(!id||!camera(id,(room>=20&&room<=24)||room==10||room==17,room==11))return false;
        return room!=11||entry(room);
    }
};
}
#endif
