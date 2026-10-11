#ifndef DOWNWELL_TEST_COLLISION_OWNER_FIXTURE_H
#define DOWNWELL_TEST_COLLISION_OWNER_FIXTURE_H
#include "core/GameplayCollisionOwner.h"
#include <memory>
#include <utility>
namespace downwell {
struct CollisionOwnerFixtureDeleter {
    std::unique_ptr<GameplayCollisionOwner::ReferenceStorage> reference;
    std::unique_ptr<GameplayCollisionOwner::NativeCollisionStorage> collision;
    std::unique_ptr<GameplayCollisionOwner::SkeletonEncounterStorage> encounter;
    void operator()(GameplayCollisionOwner* owner)const {delete owner;}
};
using CollisionOwnerFixture=std::unique_ptr<GameplayCollisionOwner,CollisionOwnerFixtureDeleter>;
// The deleter retains the backing until after the borrower is destroyed. Keep
// the actual owner pointer (including void* callback contexts) unchanged.
inline auto makeCollisionOwner(const ObjectPack& objects,const SpriteCollisionPack& sprites,
        GameplayTerrainInstances::Presentation presentation={},GameplayTerrainInstances::WallCollision collision={},
        const RoomPack* rooms=nullptr,GameplayCollisionOwner::Ownership mode=GameplayCollisionOwner::Ownership::reference,
        bool nativeBoxes=true,bool nativeFields=true,bool collisionStorage=true,bool encounterStorage=true) {
    CollisionOwnerFixtureDeleter backing;
    if(mode==GameplayCollisionOwner::Ownership::reference||mode==GameplayCollisionOwner::Ownership::nativeTerrain)
        backing.reference=std::make_unique<GameplayCollisionOwner::ReferenceStorage>();
    else if(collisionStorage)
        backing.collision=std::make_unique<GameplayCollisionOwner::NativeCollisionStorage>();
    if(encounterStorage)backing.encounter=std::make_unique<GameplayCollisionOwner::SkeletonEncounterStorage>();
    auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,presentation,collision,rooms,mode,
        nativeBoxes,nativeFields,backing.reference.get(),backing.collision.get(),backing.encounter.get());
    return CollisionOwnerFixture(owner.release(),std::move(backing));
}
}
#endif
