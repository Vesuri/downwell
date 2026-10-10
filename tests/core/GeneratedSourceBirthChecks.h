#ifndef DOWNWELL_TEST_GENERATED_SOURCE_BIRTH_CHECKS_H
#define DOWNWELL_TEST_GENERATED_SOURCE_BIRTH_CHECKS_H
#include "core/GameplayCollisionOwner.h"
#include <cassert>
#include <memory>
#include <vector>
inline void checkGeneratedSourceBirths(const downwell::ObjectPack& objects,
    const downwell::SpriteCollisionPack& sprites){
    using namespace downwell;
    // Scattered16 event order, including child Create RNG and failure prefixes.
    // These are event tests, not admission of natural encounters.
    struct RandomAudit {
        std::vector<int> order;
        unsigned chooseIndexSmall(unsigned n){order.push_back(100+int(n));return n-1;}
    };
    struct ScatteredEffects {
        RandomAudit& random;unsigned failAt=0,calls=0;int style=0;
        bool knees=true;MotionBinary64 speed=MotionBinary64::fromInteger(1);
        bool op(int code){random.order.push_back(code);return ++calls!=failAt;}
        bool outOfView(){return op(1);}
        bool currency(int n){assert(n==2);return op(2);}
        bool attractGem(){return op(3);}
        bool createDebris(){return op(4);}
        bool debrisYSpeed(MotionBinary64 v){assert(v.high==0xbfe00000u&&!v.low);return op(5);}
        bool sound(int s,int volume,int pan,int pitch){
            assert(!pan&&pitch==1&&((s==174&&volume==50)||(s==93&&volume==60)));return op(6);
        }
        bool sprite(int s){assert(s==551);return op(7);}
        bool playerYSpeed(MotionBinary64& v){v=speed;return op(8);}
        bool playStyle(int& v){v=style;return op(9);}
        bool jump(int zero,MotionBinary64 v){assert(!zero&&!v.low&&v.high==(style==2?0x3ff80000u:0x40040000u));return op(10);}
        bool shake(int a,int b){assert(a==2&&b==4);return op(11);}
        bool explodingKnees(bool& v){v=knees;return op(12);}
        bool createBlast(){return op(13);}
    };
    for(unsigned failure=0;failure<=9;++failure){
        GameplayScattered store;DeferredCreateInstance command(289,240,700);
        assert(store.adopt(899998,command));const auto key=store.handle(0);
        RandomAudit random;ScatteredEffects effects{random};
        assert(store.playerContact(key,random,effects));
        assert((random.order==std::vector<int>{8,9,10,11,12,13,103,6}));
        assert(store.find(key)->objHp==0&&store.find(key)->stomped==1);
        random.order.clear();effects.calls=0;effects.failAt=failure;
        assert(store.step(key,random,effects)==!failure);
        const std::vector<int> expected{1,2,3,4,5,4,5,104,6,7};
        const unsigned length=failure?(failure>=8?failure+1:failure):10;
        assert(random.order==std::vector<int>(expected.begin(),expected.begin()+length));
        assert(store.find(key)->scriptActive==(failure?1:0));
        if(failure){assert(!store.step(key,random,effects)&&random.order.size()==length);}
        else {
            assert(store.find(key)->active); // Script active=0 is not deactivation.
            assert(store.find(key)->spriteId==551&&store.find(key)->maskId==550&&
                store.find(key)->imageIndex.bits==0);
            random.order.clear();assert(store.step(key,random,effects));
            assert((random.order==std::vector<int>{1}));
        }
    }
    for(unsigned failure=0;failure<=7;++failure){
        GameplayScattered::Record state;RandomAudit random;ScatteredEffects effects{random,failure};effects.style=2;
        assert(ScatteredEvents::playerContact(state,random,effects)==!failure);
        const std::vector<int> expected{8,9,10,11,12,13,103,6};
        const unsigned length=failure?(failure==7?8:failure):8;
        assert(random.order==std::vector<int>(expected.begin(),expected.begin()+length));
        assert(state.objHp==(failure==1?1:0)&&state.stomped==(failure==1?0:1));
    }
    {
        GameplayScattered::Record state;RandomAudit random;ScatteredEffects effects{random};
        for(auto speed:{MotionBinary64{},MotionBinary64::fromInteger(-1)}){
            effects.speed=speed;random.order.clear();
            assert(ScatteredEvents::playerContact(state,random,effects)&&state.objHp==1);
            assert((random.order==std::vector<int>{8}));
        }
        state.scriptActive=0;random.order.clear();
        assert(ScatteredEvents::playerContact(state,random,effects)&&random.order.empty());
        state.hitStun=5;ScatteredEvents::alarm0(state);assert(!state.hitStun);
    }
    for(unsigned failure=0;failure<=6;++failure){
        struct Audit {
            using Record=GameplayScattered::Record;unsigned calls=0,failure;
            bool emit(){return ++calls!=failure;}
            bool birth(InstanceHandle,const Record& r){assert(r.sourceId==899999&&!r.initialized&&r.hitEffect==0);return emit();}
            bool retire(InstanceHandle,const Record&){return true;}
            bool activation(InstanceHandle,const Record&,bool){return true;}
            bool membership(InstanceHandle,const Record&,bool){return true;}
            bool move(InstanceHandle,const Record&,RunnerImageValue,RunnerImageValue){assert(false);return false;}
            bool sprite(InstanceHandle,const Record&,int sprite){assert(calls==3&&sprite==550);return emit();}
            bool mask(InstanceHandle,const Record&,int mask){assert(calls==4&&mask==550);return emit();}
            bool imageIndex(InstanceHandle,const Record&,RunnerImageValue index){
                assert((calls==1&&!index.bits)||(calls==5&&index.bits==0x41800000u));return emit();
            }
            bool transform(InstanceHandle,const Record&,RunnerTransformField field,RunnerImageValue scale,
                RunnerImageValue alpha,std::uint32_t blend){
                assert(calls==2&&field==RunnerTransformField::scaleX&&scale.bits==0xbf800000u&&
                    alpha.bits==0x3f800000u&&blend==0xffffffu);return emit();
            }
        } audit{0,failure};
        GameplayScattered store(GameplayScattered::Binding::bindMasks(audit));
        DeferredCreateInstance command(289,240,700);command.integerScaleX=-1;command.integerImageIndex=16;
        assert(store.adopt(899999,command)==!failure&&audit.calls==(failure?failure:6));
        const auto key=store.handle(0);const auto* r=store.find(key);
        assert(r&&r->initialized==!failure&&store.valid()==!failure);
        if(failure)assert(!store.adopt(900000,command)&&audit.calls==failure&&store.size()==1);
        else assert(store.clear()&&!store.find(key)&&!store.size());
    }
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
        DeferredCreateInstance scenery(289,240,700);scenery.integerScaleX=-1;scenery.integerImageIndex=16;
        const auto beforeScenery=world->randomDrawCount();
        assert(world->createGeneratedInstanceWithSourceIds(scenery,objects,allocate,backend,noShop));
        const auto* store=world->scattered();assert(store&&store->size()==1);
        const auto local=store->handle(0);const auto* actor=store->find(local);InstanceHandle shared;
        assert(actor&&actor->sourceId==850012&&actor->initialized&&actor->imageIndex.bits==0x41800000u&&
            actor->scaleX.bits==0xbf800000u&&actor->maskId==550&&actor->hitEffect==1&&actor->objHp==1&&
            world->randomDrawCount()==beforeScenery&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::scattered,local,shared)&&
            GameplayCollisionView(*world).contains(shared));
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,2,300)!=ResultsDestination::none&&
            world->scattered()==store&&!store->find(local)&&!GameplayCollisionView(*world).contains(shared));
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
