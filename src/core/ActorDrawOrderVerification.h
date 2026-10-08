#ifndef DOWNWELL_CORE_ACTOR_DRAW_ORDER_VERIFICATION_H
#define DOWNWELL_CORE_ACTOR_DRAW_ORDER_VERIFICATION_H
#include "GameplayWorld.h"
namespace downwell {
// Caller-owned scratch: never put this maximum-size snapshot on the Amiga
// task stack. The callbacks observe ordering only, without executing Draw.
struct ActorDrawOrderVerification {
    static constexpr unsigned capacity=1+GameplayWorld::enemyCapacity+GameplayWorld::bulletCapacity+
        GameplayScriptSpawnInstances::capacity+GameplayDebrisInstances::capacity+GameplayEntryDithers::capacity+GameplayWorld::boneCapacity;
    struct Entry {GameplayWorld::DrawActor kind;InstanceHandle key;RunnerImageValue depth;};
    Entry expected[capacity]{};
    bool verify(const GameplayWorld& world){
        unsigned count=0;
        if(!world.visitRoomActors([&](auto kind,auto key,auto depth){
            if(count==capacity)return false;
            expected[count++]={kind,key,depth};return true;
        }))return false;
        unsigned index=0;
        return world.visitRoomActorsReadOnly([&](auto kind,auto key,auto depth){
            if(index==count)return false;
            const auto& e=expected[index++];
            return e.kind==kind&&e.key.slot==key.slot&&e.key.generation==key.generation&&e.depth.bits==depth.bits;
        })&&index==count;
    }
};
}
#endif
