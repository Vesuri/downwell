#ifndef DOWNWELL_CORE_STORED_SHOOTABLE_TARGETS_H
#define DOWNWELL_CORE_STORED_SHOOTABLE_TARGETS_H
#include "StoredWorldCollision.h"
namespace downwell {
// Dispatch fields only after the collision query has selected a world
// identity. Never probe a registry and fall through on failure: its failure
// may be terminal, and the same family-local slot can exist in both owners.
template<class Bones,class Corpses> struct StoredShootableTargets {
    StoredWorldCollision& world;Bones& bones;Corpses& corpses;
    template<class Operation> bool selected(InstanceHandle key,Operation operation){
        const auto* record=world.resolve(key);if(!record)return false;
        if(record->objectId==269)return operation(bones);
        if(record->objectId==263)return operation(corpses);
        return false; // Other parent-90 descendants require their actual owner.
    }
    bool readShootable(InstanceHandle key,MotionBinary64& hp,bool& effect){
        return selected(key,[&](auto& owner){return owner.readShootable(key,hp,effect);});
    }
    bool writeShootableHealth(InstanceHandle key,MotionBinary64 hp){
        return selected(key,[&](auto& owner){return owner.writeShootableHealth(key,hp);});
    }
    bool writeShootableHit(InstanceHandle key){
        return selected(key,[&](auto& owner){return owner.writeShootableHit(key);});
    }
};
template<class Bones,class Corpses>
auto storedShootableTargets(StoredWorldCollision& world,Bones& bones,Corpses& corpses){
    return StoredShootableTargets<Bones,Corpses>{world,bones,corpses};
}
}
#endif
