#ifndef DOWNWELL_TEST_GENERATED_SOURCE_BIRTH_CHECKS_H
#define DOWNWELL_TEST_GENERATED_SOURCE_BIRTH_CHECKS_H
#include "core/GameplayCollisionOwner.h"
#include <cassert>
#include <memory>
#include <vector>
inline void checkGeneratedSourceBirths(const downwell::ObjectPack& objects,
    const downwell::SpriteCollisionPack& sprites){
    using namespace downwell;
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        const bool native=mode==GameplayCollisionOwner::Ownership::native;
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),native?nullptr:&owner->terrain());
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain)));
        struct Cursor {
            unsigned next=850000;std::vector<int> births;
            bool operator()(int object,unsigned& id){births.push_back(object);id=next++;return true;}
        } allocate;
        auto& next=allocate.next;auto& births=allocate.births;
        auto backend=[&](const DeferredCreateInstance& c,unsigned id){
            if(c.objectId!=60&&c.objectId!=93&&c.objectId!=99)return true;
            return !native||(terrain->append(c.objectId,c.x,c.y,c.active,id)&&
                world->finishNativeTerrainCreate(terrain->handle(terrain->size()-1)));
        };
        // Shop ownership is intentionally not faked by this fixture. There is
        // no native shop owner yet: the composed handoff must propagate its
        // eventual owner's failure, rather than fall back to compact stock.
        auto noShop=[](const auto&,auto&){assert(false);return false;};
        for(int object:{60,93,99,92,112,82,62}){
            DeferredCreateInstance command(object,240,512);
            command.setTimeFieldRadiusWord(12345);
            if(object==92)command.setBoxCreate(173,2);
            assert(world->createGeneratedInstanceWithSourceIds(command,objects,allocate,backend,noShop));
        }
        assert((births==std::vector<int>{60,93,99,92,112,82,62}));
        for(unsigned i=0;i<3;++i){
            if(native){GameplayWorld::NativeTerrain::Identity id;bool active,destroyed;
                assert(terrain->read(terrain->handle(i),id,active,destroyed)&&id.source==850000+i&&active&&!destroyed);
            }else assert(owner->terrain().find(owner->terrain().handle(i))->sourceInstanceId==850000+i);
        }
        assert(world->breakableSlot(0).sourceId==850003&&world->breakableSlot(0).state.breakSound==173&&
            world->breakableSlot(0).state.haveGem==2);
        assert(world->enemyAt(0).sourceId==850004);
        assert(world->sideCameraLockers().find(world->sideCameraLockers().handle(0))->sourceId==850006);
        const auto draws=world->randomDrawCount();
        DeferredCreateInstance field(82,256,600);field.setTimeFieldRadiusWord(0xdeadbeefu);
        assert(world->createGeneratedInstanceWithSourceIds(field,objects,allocate,backend,noShop)&&
            world->randomDrawCount()==draws);
        // Nested creation uses the same cursor and may interleave another
        // family between two parent/child allocations. No ID block is reserved.
        auto nested=[&](int object,unsigned& id){
            if(!allocate(object,id))return false;
            if(object==306){unsigned child;
                return allocate(62,child)&&world->registerSideCameraLocker(400,1000,child);
            }
            return true;
        };
        assert(world->createGeneratedInstanceWithSourceIds({306,240,2000},objects,nested,backend,noShop));
        assert(next==850012&&births[births.size()-4]==306&&births[births.size()-3]==62&&
            births[births.size()-2]==74&&births.back()==255);
        assert(world->exitControllers().find(world->exitControllers().handle(0))->sourceId==850008);
        assert(world->exitControllers().find(world->exitControllers().handle(1))->sourceId==850010);
    }
    for(unsigned failure=0;failure<5;++failure){
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false);
        unsigned allocations=0,publications=0,shops=0;
        auto allocate=[&](int,unsigned& id){++allocations;id=failure==1?0:850001;return failure!=0;};
        auto backend=[&](const auto&,unsigned){++publications;return false;};
        auto shop=[&](const auto&,auto&){++shops;return false;};
        const int object=failure==3?120:failure==4?277:92;
        assert(!world->createGeneratedInstanceWithSourceIds({object,240,512},objects,allocate,backend,shop));
        assert(!world->effectsSucceeded()&&world->breakableSlots()==0&&
            allocations==(failure<3?1u:0u)&&publications==(failure==2?1u:0u)&&shops==(failure==4?1u:0u));
        assert(!world->createGeneratedInstanceWithSourceIds({92,240,512},objects,allocate,backend,shop));
        assert(allocations==(failure<3?1u:0u));
    }
}
#endif
