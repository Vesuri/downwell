#ifndef DOWNWELL_TEST_GENERATED_SOURCE_BIRTH_CHECKS_H
#define DOWNWELL_TEST_GENERATED_SOURCE_BIRTH_CHECKS_H
#include "core/GameplayCollisionOwner.h"
#include "core/StoredShopCreationOwner.h"
#include "core/GameplayShopInstances.h"
#include <cassert>
#include <memory>
#include <vector>
inline void checkGeneratedSourceBirths(const downwell::ObjectPack& objects,
    const downwell::SpriteCollisionPack& sprites,const downwell::SpriteMaskPack& masks){
    using namespace downwell;
    for(unsigned failure=0;failure<=12;++failure){
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false);
        auto reference=std::make_unique<GameplayWorld>(300,false,1000,false);
        assert(world->grantUpgrade(14)&&reference->grantUpgrade(14));
        struct Publication {
            const GameplayShopInstances* store=nullptr;
            unsigned failure,births=0,images=0,retired=0;
            bool birth(InstanceHandle,const GameplayShopInstances::Record&){assert(false);return false;}
            bool birthAt(InstanceHandle h,const GameplayShopInstances::Record& r,RunnerImageValue x,RunnerImageValue y){
                assert(store&&store->find(h)==&r&&!r.initialized&&!r.state.initialized&&
                    r.imageIndex.bits==0&&r.imageSpeed.bits==0x3f800000u&&
                    r.x.bits==x.bits&&r.y.bits==y.bits&&r.alarms.get(0)==-1);
                ++births;return births!=failure;
            }
            bool imageIndex(InstanceHandle h,const GameplayShopInstances::Record& r,RunnerImageValue v){
                assert(store->find(h)==&r&&r.objectId==283&&!r.initialized&&
                    !r.imageIndex.bits&&r.imageSpeed.bits==0x3f800000u&&v.bits==0x3f800000u);
                ++images;return failure!=12;
            }
            bool retire(InstanceHandle h,const GameplayShopInstances::Record& r){assert(store->find(h)==&r);++retired;return true;}
            bool activation(InstanceHandle,const GameplayShopInstances::Record&,bool){return true;}
            bool membership(InstanceHandle,const GameplayShopInstances::Record&,bool){return true;}
            bool move(InstanceHandle,const GameplayShopInstances::Record&,RunnerImageValue,RunnerImageValue){return true;}
            bool sprite(InstanceHandle,const GameplayShopInstances::Record&,int){return true;}
            bool mask(InstanceHandle,const GameplayShopInstances::Record&,int){return true;}
            bool transform(InstanceHandle,const GameplayShopInstances::Record&,RunnerTransformField,RunnerImageValue,RunnerImageValue,std::uint32_t){return true;}
            bool markDestroyed(InstanceHandle,const GameplayShopInstances::Record&){return true;}
        } publication{nullptr,failure};
        auto store=std::make_unique<GameplayShopInstances>(objects,GameplayShopInstances::Binding::bindPositionedActor(publication));
        publication.store=store.get();unsigned next=950000;std::vector<int> order;
        auto allocate=[&](int object,unsigned& id){order.push_back(object);id=next++;return true;};
        struct Audio {
            unsigned calls=0;
            bool play(int sound,int priority,bool loop,int& voice){assert(sound==192&&priority==100&&loop);++calls;voice=88;return true;}
            bool setGain(int voice,MotionBinary64 gain,int duration){assert(voice==88&&gain.zero()&&!duration);++calls;return true;}
        } audio;
        auto creation=store->creation(allocate,audio);
        auto* shop=world->registerShopWithOwner(480,2000,creation);
        assert(bool(shop)==!failure&&store->valid()==!failure&&world->effectsSucceeded()==!failure);
        const unsigned count=failure?(failure==12?9:failure):11;
        const int expected[]={277,278,279,279,279,98,280,212,283,283,283};
        assert(store->size()==count&&next==950000+count&&order.size()==count&&publication.births==count);
        for(unsigned i=0;i<count;++i){
            const auto* r=store->find(store->sourceHandle(950000+i));
            assert(order[i]==expected[i]&&r&&r->objectId==expected[i]&&
                r->initialized==(!failure||i+1<count));
        }
        const auto old=store->sourceHandle(950000);
        if(shop){
            const auto* control=reference->registerShop(480,2000);
            assert(control&&publication.images==3&&audio.calls==2&&shop->sourceId==950000&&
                shop->children.items[2]==950010&&shop->farewellVariant==control->farewellVariant);
            assert(world->randomState().stateIndex()==reference->randomState().stateIndex());
            for(unsigned i=0;i<16;++i)assert(world->randomState().stateWord(i)==reference->randomState().stateWord(i));
            for(unsigned i=0;i<3;++i){
                const auto* item=store->find(store->sourceHandle(shop->children.items[i]));
                const auto* box=store->find(store->sourceHandle(shop->children.cases[i]));
                assert(item&&box&&item->state.whichUg==control->stock.items[i].upgrade&&
                    shop->stock.items[i].price==control->stock.items[i].price&&box->state.storedItem==1&&
                    box->state.wallHp==1000000&&item->imageIndex.bits==0x3f800000u&&!item->imageSpeed.bits);
            }
            const auto* text=store->find(store->sourceHandle(shop->children.risingText));
            const auto* keeper=store->find(store->sourceHandle(shop->children.keeper));
            assert(text&&keeper&&text->alarms.get(0)==90&&text->alarms.get(1)==60&&
                text->state.ysp.high==0xc0080000u&&text->state.drawing==1&&keeper->imageSpeed.bits==0x3db851ecu);
        }else{
            assert(!world->registerShopWithOwner(0,0,creation)&&next==950000+count);
        }
        assert(store->clear()&&!store->size()&&!store->find(old)&&publication.retired==count);
        if(!failure){
            const unsigned id=creation.create(278,0,0);const auto h=store->sourceHandle(id);
            assert(id==950011&&h.generation!=old.generation&&!store->find(old)&&store->clear());
        }
    }
    // The generated Shop callback now enters the actual GameplayWorld run
    // state. Use a concrete retained owner here, not successful ID-only child
    // callbacks. Native family ownership/front-end installation is separate.
    for(bool member:{false,true})for(unsigned failure=0;failure<4;++failure){
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false);
        auto reference=std::make_unique<GameplayWorld>(300,false,1000,false);
        if(member)assert(world->grantUpgrade(14)&&reference->grantUpgrade(14));
        InstanceStore<16> records;unsigned next=940000;const int room=11;bool good=true;
        if(failure==3)for(unsigned i=0;i<8;++i)
            assert(records.create(930000+i,62,room,false,0)==CreateInstanceResult::success);
        struct Audio {
            unsigned failure,calls=0;
            bool play(int sound,int priority,bool loop,int& handle){
                assert(sound==192&&priority==100&&loop);++calls;handle=77;return failure!=1;
            }
            bool setGain(int handle,MotionBinary64 gain,int duration){
                assert(handle==77&&gain.zero()&&!duration);++calls;return failure!=2;
            }
        } audio{failure};
        struct Publication {
            const InstanceStore<16>& records;unsigned births=0;
            bool birth(InstanceHandle,const InstanceRecord&){assert(false);return false;}
            bool birthAt(InstanceHandle h,const InstanceRecord& r,RunnerImageValue x,RunnerImageValue y){
                std::int32_t px,py;
                assert(records.resolve(h)==&r&&x.roundPixel(px)&&y.roundPixel(py)&&
                    px==r.x&&py==r.y&&r.imageIndex==0&&r.imageSpeed==1);
                ++births;return true;
            }
            bool retire(InstanceHandle,const InstanceRecord&){return true;}
            bool activation(InstanceHandle,const InstanceRecord&,bool){return true;}
            bool membership(InstanceHandle,const InstanceRecord&,bool){return true;}
        } publication{records};
        StoredShopCreationOwner<16,1,Audio> owner(records,objects,
            InstanceLifecycleBinding::bindPositionedLifetime(publication),next,room,good,audio);
        auto allocate=[&](int,unsigned& id){id=next++;return true;};
        auto backend=[](const auto&,unsigned){assert(false);return false;};
        ShopWorld::Instance* shop=nullptr;
        auto createShop=[&](const auto& command,auto& cursor){
            assert(&cursor==&allocate);
            shop=world->registerShopWithOwner(command.x,command.y,owner);return shop!=nullptr;
        };
        assert(world->createGeneratedInstanceWithSourceIds({62,100,700},objects,allocate,backend,createShop));
        const auto beforeRandom=world->randomState();
        const bool created=world->createGeneratedInstanceWithSourceIds({277,480,2000},objects,allocate,backend,createShop);
        assert(created==!failure&&world->effectsSucceeded()==!failure&&good==!failure);
        if(!failure){
            const auto* control=reference->registerShop(480,2000);
            assert(control&&shop&&records.size()==11&&next==940012&&shop->sourceId==940001&&
                shop->children.table==940002&&shop->children.items[2]==940011&&audio.calls==2&&publication.births==11);
            for(unsigned i=0;i<3;++i)assert(shop->stock.items[i].upgrade==control->stock.items[i].upgrade&&
                shop->stock.items[i].price==control->stock.items[i].price);
            assert(shop->farewellVariant==control->farewellVariant&&
                world->randomState().stateIndex()==reference->randomState().stateIndex());
            for(unsigned i=0;i<16;++i)assert(world->randomState().stateWord(i)==reference->randomState().stateWord(i));
            assert(world->createGeneratedInstanceWithSourceIds({62,200,700},objects,allocate,backend,createShop)&&next==940013);
        }else{
            assert(!shop&&next==(failure==3?940009u:940002u)&&audio.calls==(failure==1?1u:2u));
            if(failure<3){
                assert(world->randomState().stateIndex()==beforeRandom.stateIndex());
                for(unsigned i=0;i<16;++i)assert(world->randomState().stateWord(i)==beforeRandom.stateWord(i));
            }
            // Failure retains its published prefix and cannot be hidden by a
            // subsequent compact shop or by restarting registered Create.
            const unsigned consumed=next,count=records.size();const auto random=world->randomState();
            assert(!world->registerShop(0,0)&&!world->registerShopWithOwner(0,0,owner)&&
                next==consumed&&records.size()==count&&world->shops().allocatedCount==1&&
                world->randomState().stateIndex()==random.stateIndex());
            for(unsigned i=0;i<16;++i)assert(world->randomState().stateWord(i)==random.stateWord(i));
        }
    }
    // Exercise the production room animation/alarm callbacks through a whole
    // tick, rather than directly advancing the actor from the test.
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        const bool native=mode==GameplayCollisionOwner::Ownership::native;
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),native?nullptr:&owner->terrain());
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain)));
        auto& store=owner->whiteJellies();
        struct Ops {
            GameplayWhiteJellies& store;InstanceHandle key;GameplayCollisionOwner& owner;
            unsigned* decoyReads=nullptr;
            bool position(bool y,RunnerImageValue v,RunnerImageValue& out){return store.position(key,y,v,out);}
            bool playerY(MotionBinary64& v){v={};return true;}
            bool playerX(MotionBinary64& v){v={};return true;}
            bool atan(MotionBinary64 y,MotionBinary64 x,MotionBinary64& v){return portableAtan(y,x,v);}
            bool pointDirection(MotionBinary64 x,MotionBinary64 y,MotionBinary64 tx,MotionBinary64 ty,MotionBinary64& v){
                return RunnerPointDirection::evaluate(x,y,tx,ty,v,portableAtan);
            }
            bool direction(MotionBinary64 v,MotionBinary64& out){return owner.whiteJellyDirection(key,v,out);}
            bool imageSpeed(RunnerImageValue v,RunnerImageValue& out){out=v;return true;}
            bool enemyPlayerY(MotionBinary64& v){assert(decoyReads);++*decoyReads;v=MotionBinary64::fromInteger(2345);return true;}
            bool enemyPlayerX(MotionBinary64& v){assert(decoyReads);++*decoyReads;v=MotionBinary64::fromInteger(1234);return true;}
            bool lengthdir(bool y,MotionBinary64 speed,MotionBinary64 angle,MotionBinary64& out){
                assert(angle.zero());out=y?MotionBinary64{}:speed;return true;
            }
            bool meeting(MotionBinary64 x,MotionBinary64 y,int target,bool& hit){const bool ok=store.meeting(key,x,y,target,hit);assert(ok);return ok;}
            bool imageIndex(int v){assert(!v);return store.imageIndex(key,{});}
            bool alarm(int n,int v){return store.setAlarm(key,n,v);}
            bool sprite(int v){return store.sprite(key,v);}
            bool destroy(){return store.markDestroyed(key);}
            bool sound(int,int,int,int,const char*){assert(false);return false;}
            bool enemyDeath(){assert(false);return false;}
            bool smoke(MotionBinary64,MotionBinary64,int,int){assert(false);return false;}
            bool movingFx(MotionBinary64,MotionBinary64,int,MotionBinary64,MotionBinary64,MotionBinary64){assert(false);return false;}
            bool allocateMovingFx(unsigned&){assert(false);return false;}
            bool flash(MotionBinary64,MotionBinary64,int,int,int){assert(false);return false;}
            bool currency(int){assert(false);return false;}
        };
        auto expectedRandom=world->randomState();InstanceHandle key;
        MotionBinary64 expectedVelocity;
        assert(randomBinary64(expectedRandom.nextU32(),{0x3fe00000u,0},expectedVelocity)&&
            expectedVelocity.add(MotionBinary64::fromInteger(1)));
        assert(world->createWhiteJellyWithSourceId(899000,{0x43800000u},{0x44300000u},
            [&](auto h){return Ops{store,h,*owner};},key));
        MotionBinary64 heading;
        assert(owner->whiteJellyDirection(key,MotionBinary64::fromInteger(360),heading)&&heading.zero());
        assert(owner->whiteJellyDirection(key,heading,heading)&&heading.zero());
        assert(store.find(key)->initialized&&store.find(key)->active&&
            store.find(key)->velocity.high==expectedVelocity.high&&store.find(key)->velocity.low==expectedVelocity.low&&
            world->randomState().stateIndex()==expectedRandom.stateIndex());
        for(unsigned word=0;word<16;++word)assert(world->randomState().stateWord(word)==expectedRandom.stateWord(word));
        assert(store.dispatch(key,[](auto& r){r.hitStun=true;return true;}));
        assert(store.setAlarm(key,1,2));
        world->step(0);
        assert(world->effectsSucceeded()&&owner->valid()&&store.find(key)->imageIndex.bits==0x3e4ccccdu&&
            store.find(key)->hitStun&&store.find(key)->alarms.get(1)==1);
        world->step(0);
        assert(world->effectsSucceeded()&&owner->valid()&&!store.find(key)->hitStun&&
            store.find(key)->imageIndex.bits==0x3ecccccdu);
        assert(owner->configureCorpseMasks(masks));
        assert(store.dispatch(key,[](auto& r){r.xx=MotionBinary64::fromInteger(-1000);return true;}));
        GameplayCollisionOwner::EncounterStepOrder phase;Ops ops{store,key,*owner};unsigned steps=0;
        auto beforeX=store.find(key)->xx,beforeY=store.find(key)->yy;
        assert(owner->withEncounterSteps(phase,[&](InstanceHandle h,int object){
            assert(object==115);++steps;const bool ok=owner->stepRegisteredWhiteJellyWithWorld(h,false,ops);assert(ok);return ok;
        },[&]{world->step(0);return world->effectsSucceeded();}));
        auto expectedX=MotionBinary64::fromInteger(-1000);
        assert(expectedX.add(store.find(key)->xsp));
        assert(steps==1&&!store.find(key)->xsp.zero()&&store.find(key)->xx.high==expectedX.high&&store.find(key)->xx.low==expectedX.low&&
            store.find(key)->x.bits==0xc47a0000u);
        MotionBinary64 targetX,targetY,expectedDirection;
        assert(ContactCoordinates::playerGlobals(world->player().frameStartXFixed(),world->player().frameStartYFixed(),
            240,176,targetX,targetY)&&RunnerPointDirection::evaluate(beforeX,beforeY,targetX,targetY,expectedDirection,portableAtan));
        assert(store.find(key)->playerDirection.high==expectedDirection.high&&store.find(key)->playerDirection.low==expectedDirection.low);
        assert(world->grantUpgrade(15));unsigned decoyReads=0;ops.decoyReads=&decoyReads;
        beforeX=store.find(key)->xx;beforeY=store.find(key)->yy;
        assert(owner->withEncounterSteps(phase,[&](InstanceHandle h,int object){
            assert(object==115);return owner->stepRegisteredWhiteJellyWithWorld(h,false,ops);
        },[&]{world->step(0);return world->effectsSucceeded();}));
        assert(decoyReads==2); // Target selection is not silently replaced with the player.
        assert(RunnerPointDirection::evaluate(beforeX,beforeY,MotionBinary64::fromInteger(1234),
            MotionBinary64::fromInteger(2345),expectedDirection,portableAtan));
        assert(store.find(key)->playerDirection.high==expectedDirection.high&&store.find(key)->playerDirection.low==expectedDirection.low);
        RunnerImageValue outside,accepted;
        unsigned allocations=0,timerReads=0;
        for(bool stopped:{true,false}){
            assert(store.setAlarm(key,0,1));
            assert(owner->withWhiteJellyEffectAlarms(
                [&](bool& value){value=stopped;return true;},
                [&](InstanceHandle caller,MotionBinary64& value){
                    assert(caller.slot==key.slot&&caller.generation==key.generation);
                    ++timerReads;value={0x40298000u,0};return true; // 12.75 truncates to 12.
                },
                [&](unsigned& source){assert(!stopped);source=900000+allocations++;return true;},
                [&]{world->step(0);return world->effectsSucceeded();}));
            assert(store.find(key)->alarms.get(0)==12&&allocations==(stopped?0u:1u));
        }
        assert(timerReads==2&&store.find(key)->myFx==900000&&owner->movingEffects().size()==1);
        unsigned emitted=0;
        assert(owner->visitRegistrationOrder([&](InstanceHandle h,int object,std::uint32_t){
            if(object!=220)return true;
            GameplayCollisionView::Record child;
            assert(GameplayCollisionView(*world).resolve(h,child)&&child.movingFx&&
                child.movingFx->sourceId==900000&&child.movingFx->spriteId==704);
            ++emitted;return true;
        })&&emitted==1);
        assert(outside.assignInteger(-1000)&&store.position(key,true,outside,accepted));
        world->step(0);
        assert(world->effectsSucceeded()&&owner->valid()&&store.find(key)&&store.find(key)->destroyRequested);
        assert(owner->prepareDraw()&&!store.find(key));
        InstanceHandle spawner;
        assert(owner->spawners().createCaverns(899001,-116,100,1,false,spawner));
        struct SpawnOps {
            GameplayWorld& world;GameplayCollisionOwner& owner;const ObjectPack& objects;
            InstanceHandle spawner,child{};unsigned next=899002,births=0,deactivations=0;
            bool playerY(MotionBinary64& v){v=MotionBinary64::fromInteger(101);return true;}
            bool area(int& v){v=1;return true;}
            bool fallDepth(MotionBinary64& v){v=MotionBinary64::fromInteger(160);return true;}
            bool death(bool& v){v=false;return true;}
            bool endReached(bool& v){v=false;return true;}
            bool enemyCount(int& v){unsigned n;if(!owner.population(83,n))return false;v=int(n);return true;}
            bool objectMask(int object,int& mask){PackedObject r;if(!objects.findObject(object,r))return false;mask=r.textureMaskId;return true;}
            bool mask(int v){return owner.spawners().mask(spawner,v);}
            bool viewY(MotionBinary64& v){v=MotionBinary64::fromInteger(34);return true;}
            bool viewHeight(MotionBinary64& v){v=MotionBinary64::fromInteger(284);return true;}
            bool meeting(MotionBinary64 x,MotionBinary64 y,int object,bool& hit){return owner.spawners().meeting(spawner,x,y,object,hit);}
            bool tile(int depth,MotionBinary64 x,MotionBinary64 y,bool& hit){
                return depth==100&&world.wallBackgroundTiles().contains(x,y,hit);
            }
            bool create(int object,MotionBinary64 x,MotionBinary64 y,unsigned& id){
                assert(object==115&&!births&&!deactivations);id=next++;
                InstanceHandle shared;GameplayCollisionView::Kind kind;
                const bool ok=owner.createRegisteredSteeringEnemy(object,id,x,y,portableAtan,shared)&&
                    GameplayCollisionView::decode(shared,kind,child)&&kind==GameplayCollisionView::Kind::whiteJelly;
                assert(ok&&owner.whiteJellies().find(child)->initialized&&owner.whiteJellies().find(child)->active);
                unsigned count;assert(owner.population(83,count)&&count==1);
                ++births;return ok;
            }
            bool deactivate(unsigned id){
                const auto* r=owner.whiteJellies().find(child);
                assert(births==1&&!deactivations&&r&&r->initialized&&r->sourceId==id);
                ++deactivations;const bool ok=owner.whiteJellies().activate(child,false);
                unsigned count;assert(ok&&owner.population(83,count)&&count==0);return ok;
            }
        } spawnOps{*world,*owner,objects,spawner};
        auto oracle=world->randomState();
        oracle.irandomSmall(32);oracle.chooseIndexSmall(2);oracle.irandomSmall(0);oracle.irandomSmall(0);
        const int childX=160+16*(2+int(oracle.irandomSmall(6)));
        const int childY=336+16*(1+int(oracle.irandomSmall(4)));
        oracle.nextU32(); // Synchronous white-jelly Create, before deactivation.
        unsigned spawnerSteps=0,particleSteps=0;
        assert(owner->withEncounterSteps(phase,[&](InstanceHandle h,int object){
            if(object==220){++particleSteps;return owner->stepMovingFx(h,false,480,masks);}
            assert(object==78);++spawnerSteps;return owner->stepRegisteredSpawner(h,spawnOps);
        },[&]{world->step(0);return world->effectsSucceeded();}));
        const auto* child=store.find(spawnOps.child);int actualX,actualY;
        assert(spawnerSteps==1&&particleSteps==1&&spawnOps.births==1&&spawnOps.deactivations==1&&child&&!child->active&&
            child->scriptActive&&child->imageIndex.bits==0&&child->x.roundPixel(actualX)&&child->y.roundPixel(actualY)&&
            actualX==childX&&actualY==childY&&world->randomState().stateIndex()==oracle.stateIndex());
        for(unsigned word=0;word<16;++word)assert(world->randomState().stateWord(word)==oracle.stateWord(word));
    }
    for(int style:{0,2})for(bool knees:{false,true})for(bool rejectSound:{false,true}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,GameplayCollisionOwner::Ownership::native);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding());
        assert(owner->attach(*world));
        DeferredCreateInstance command(289,240,700);assert(owner->scattered().adopt(890000,command));
        const auto key=owner->scattered().handle(0);
        if(knees)assert(world->grantUpgrade(0));
        struct Player {
            int style;unsigned spins=0,recharges=0,writes=0;bool grounded=true;MotionBinary64 velocity{};
            bool playerYSpeed(MotionBinary64& v){v=MotionBinary64::fromInteger(1);return true;}
            int playStyle(){return style;}
            void setSpinJumping(){assert(!spins&&!recharges&&!writes);++spins;}
            bool recharge(){assert(spins==1&&grounded&&!writes);++recharges;return true;}
            bool setGrounded(bool v){assert(recharges==1&&!v&&!writes);grounded=v;++writes;return true;}
            bool setHorizontal(MotionBinary64){assert(false);return false;}
            bool setVertical(MotionBinary64 v){assert(!grounded&&writes==1);velocity=v;++writes;return true;}
        } player{style};
        struct Audio {
            bool reject;int chosen=-1;unsigned calls=0;
            bool isPlaying(int id,bool& v){chosen=id;v=false;++calls;return true;}
            bool stop(int){assert(false);return false;}
            bool play(int id,int priority,bool loop,int& voice){assert(id==chosen&&priority==60&&!loop);voice=1;++calls;return !reject;}
            bool getGain(int,MotionBinary64& v){v=MotionBinary64::fromInteger(1);++calls;return true;}
            bool setGain(int,MotionBinary64,int){++calls;return true;}
            bool setPitch(int,MotionBinary64){++calls;return true;}
        } audio{rejectSound};
        unsigned next=890001,delays=0;
        auto allocate=[&](unsigned& id){id=next++;return true;};
        auto expected=world->randomState();unsigned flip=0;int sound=-1;
        if(knees){flip=expected.chooseIndexSmall(2);sound=91+int(expected.chooseIndexSmall(3));}
        SoundPlayEnvironment env;
        const bool failed=knees&&rejectSound;
        InstanceHandle self,other;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::scattered,key,self)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::player,world->playerHandle(),other));
        const RunnerCollisionEventContext::Frame event{self,other,4,35,289,2388};
        assert(owner->dispatchScatteredPlayerCollision(event,allocate,[&]{++delays;return true;},player,env,audio)==!failed);
        const auto* actor=owner->scattered().find(key);
        assert(actor&&actor->objHp==0&&actor->stomped&&actor->scriptActive&&player.spins==1&&player.recharges==1&&
            player.writes==2&&!player.grounded&&player.velocity.high==(style==2?0xbff80000u:0xc0040000u)&&!player.velocity.low);
        assert(actor->jump.horizontal.zero()&&actor->jump.vertical.high==(style==2?0x3ff80000u:0x40040000u)&&
            actor->shake.shakeAmt==2&&actor->shake.shakeDur==4&&delays==unsigned(knees)&&next==890001+unsigned(knees));
        assert(world->randomState().stateIndex()==expected.stateIndex());
        for(unsigned i=0;i<16;++i)assert(world->randomState().stateWord(i)==expected.stateWord(i));
        assert(audio.calls==(knees?(rejectSound?2u:5u):0u));
        if(knees){
            assert(audio.chosen==sound&&actor->sound.sfxIndex==sound);
            const auto* blast=world->registeredBlasts()->find(world->registeredBlasts()->sourceHandle(890001));
            assert(blast&&blast->state.xscale.bits==(flip?0xbf800000u:0x3f800000u));
        }
    }
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
        for(auto speed:{MotionBinary64{},MotionBinary64::fromInteger(-1),MotionBinary64{0x3e700000u,0}}){
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
        // no shop collision/layer binding in this fixture: the composed
        // handoff must not fall back to compact stock.
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
        struct Queries {
            unsigned walls=0,tiles=0;
            bool wall(InstanceHandle,MotionBinary64,MotionBinary64,bool& hit){++walls;hit=false;return true;}
            bool tile(InstanceHandle,int depth,MotionBinary64,MotionBinary64,bool& hit){assert(depth==100);++tiles;hit=false;return true;}
        } queries;
        struct Audio {
            std::vector<int> calls;int sound=-1;
            bool isPlaying(int id,bool& playing){sound=id;calls.push_back(1);playing=false;return true;}
            bool stop(int){assert(false);return false;}
            bool play(int id,int priority,bool loop,int& voice){assert(id==sound&&priority==50&&!loop);calls.push_back(2);voice=123;return true;}
            bool getGain(int id,MotionBinary64& gain){assert(id==123);calls.push_back(3);gain=MotionBinary64::fromInteger(1);return true;}
            bool setGain(int id,MotionBinary64 gain,int time){assert(id==123&&gain.high==0x3ff00000u&&!time);calls.push_back(4);return true;}
            bool setPitch(int id,MotionBinary64 pitch){assert(id==123&&pitch.high==0x3ff00000u);calls.push_back(5);return true;}
        } audio;
        SoundPlayEnvironment environment;
        auto childId=[&](std::uint32_t& id){id=next++;return true;};
        assert(owner->scattered().dispatch(local,[](auto& r){r.objHp=0;r.stomped=1;return true;}));
        assert(owner->stepRegisteredScattered(shared,11,MotionBinary64::fromInteger(600),childId,queries,environment,audio));
        actor=store->find(local);
        assert(actor&&actor->spriteId==551&&actor->maskId==550&&actor->imageIndex.bits==0x41800000u&&
            !actor->scriptActive&&actor->active&&actor->reward.myGem==850013&&next==850016);
        assert((audio.calls==std::vector<int>{1,2,3,4,5})&&audio.sound>=171&&audio.sound<=174);
        const auto* gem=world->currencyInstances()->find(world->currencyInstances()->handleForSourceId(actor->reward.myGem));
        assert(gem&&gem->state.attracted&&gem->state.getAmount==2&&queries.walls==1&&queries.tiles==1);
        unsigned debrisCount=0;
        for(unsigned slot=0;slot<GameplayDebrisInstances::capacity;++slot){
            for(unsigned generation=1;generation<=2;++generation){
                const auto* d=world->debrisInstances()->find({slot,generation});
                if(!d)continue;++debrisCount;
                assert(d->sourceId==850013+generation&&d->effect->ysp.high==0xbfe00000u&&!d->effect->ysp.low);
            }
        }
        assert(debrisCount==2);
        const auto afterBreak=world->randomDrawCount();
        assert(owner->stepRegisteredScattered(shared,11,MotionBinary64::fromInteger(600),childId,queries,environment,audio)&&
            world->randomDrawCount()==afterBreak&&next==850016);
        assert(owner->stepRegisteredScattered(shared,11,MotionBinary64::fromInteger(900),childId,queries,environment,audio));
        assert(store->find(local)->destroyRequested&&owner->prepareDraw()&&!store->find(local));
        world->confirmFirstStage();
        const auto spawnerId=next;const auto beforeSpawner=world->randomState();
        assert(world->createGeneratedInstanceWithSourceIds({78,-116,704},objects,allocate,backend,noShop));
        auto& spawners=owner->spawners();const auto spawnerKey=spawners.handle(0);
        const auto* spawner=spawners.find(spawnerKey);
        assert(spawner&&spawner->sourceId==spawnerId&&next==spawnerId+1&&births.back()==78&&
            spawner->initialized&&spawner->active&&!spawner->state.scriptActive&&
            spawner->state.channelCount==1&&spawner->state.channels[0].enemies[0]==115);
        int spawnerX,spawnerY;
        assert(spawner->x.roundPixel(spawnerX)&&spawner->y.roundPixel(spawnerY)&&spawnerX==-116&&spawnerY==704);
        assert(world->randomState().stateIndex()==beforeSpawner.stateIndex()&&
            world->randomDrawCount()==beforeSpawner.drawCount());
        for(unsigned word=0;word<16;++word)assert(world->randomState().stateWord(word)==beforeSpawner.stateWord(word));
        InstanceHandle spawnerShared;GameplayCollisionView::Record borrowed;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::spawner,spawnerKey,spawnerShared)&&
            GameplayCollisionView(*world).resolve(spawnerShared,borrowed)&&borrowed.spawner==spawner);
        assert(spawners.mask(spawnerKey,174)&&owner->prepareDraw());
        assert(spawners.markDestroyed(spawnerKey)&&owner->prepareDraw()&&!spawners.find(spawnerKey)&&
            !GameplayCollisionView(*world).contains(spawnerShared));
        assert(world->createGeneratedInstanceWithSourceIds({78,-116,800},objects,allocate,backend,noShop));
        const auto replacement=spawners.handle(0);
        assert(replacement.slot==spawnerKey.slot&&replacement.generation!=spawnerKey.generation&&
            spawners.find(replacement)->sourceId==spawnerId+1);
        auto& jellies=owner->whiteJellies();
        assert(owner->configureCorpseMasks(masks));
        struct JellyOps {
            GameplayWhiteJellies& store;InstanceHandle key;
            bool position(bool y,RunnerImageValue v,RunnerImageValue& out){return store.position(key,y,v,out);}
            bool playerY(MotionBinary64& v){v={};return true;}
            bool playerX(MotionBinary64& v){v={};return true;}
            bool pointDirection(MotionBinary64,MotionBinary64,MotionBinary64,MotionBinary64,MotionBinary64& v){v=MotionBinary64::fromInteger(90);return true;}
            bool direction(MotionBinary64 v,MotionBinary64& out){out=v;return true;}
            bool imageSpeed(RunnerImageValue v,RunnerImageValue& out){out=v;return true;}
        };
        GameMakerRandom jellyRandom(19);InstanceHandle jelly;
        assert(jellies.create(899000,{0x43788000u},{0x442f0000u},jellyRandom,
            [&](auto h){return JellyOps{jellies,h};},jelly)); // 248.5,700 ->256,704
        InstanceHandle jellyShared;GameplayCollisionView::Record jellyRecord;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::whiteJelly,jelly,jellyShared)&&
            GameplayCollisionView(*world).resolve(jellyShared,jellyRecord)&&jellyRecord.whiteJelly==jellies.find(jelly));
        assert(jellyRecord.whiteJelly->sourceId==899000&&jellyRecord.whiteJelly->x.bits==0x43800000u&&
            jellyRecord.whiteJelly->y.bits==0x44300000u&&owner->prepareDraw());
        unsigned familyCount;
        assert(owner->population(83,familyCount)&&familyCount==2); // Generated enemy 112 and registered jelly.
        assert(owner->population(115,familyCount)&&familyCount==1);
        assert(spawners.mask(replacement,174));bool candidateHit=false;
        assert(spawners.meeting(replacement,MotionBinary64::fromInteger(256),MotionBinary64::fromInteger(704),83,candidateHit)&&candidateHit);
        assert(jellies.activate(jelly,false));
        assert(owner->population(83,familyCount)&&familyCount==1);
        assert(spawners.meeting(replacement,MotionBinary64::fromInteger(256),MotionBinary64::fromInteger(704),83,candidateHit)&&!candidateHit);
        assert(jellies.activate(jelly,true));
        assert(owner->population(83,familyCount)&&familyCount==2);
        assert(spawners.meeting(replacement,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),57,candidateHit)&&candidateHit);
        assert(spawners.meeting(replacement,MotionBinary64::fromInteger(-1000),MotionBinary64::fromInteger(-1000),57,candidateHit)&&!candidateHit);
        int retainedX,retainedY;
        assert(spawners.find(replacement)->x.roundPixel(retainedX)&&spawners.find(replacement)->y.roundPixel(retainedY)&&
            retainedX==-116&&retainedY==800);
        bool wallHit=false;
        assert(jellies.meeting(jelly,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),57,wallHit)&&wallHit);
        assert(jellies.meeting(jelly,MotionBinary64::fromInteger(-1000),MotionBinary64::fromInteger(-1000),57,wallHit)&&!wallHit);
        assert(jellies.find(jelly)->x.bits==0x43800000u&&jellies.find(jelly)->y.bits==0x44300000u);
        WhiteJellyStep collisionProbe=*jellies.find(jelly);
        collisionProbe.xx=MotionBinary64::fromInteger(240);collisionProbe.yy=MotionBinary64::fromInteger(512);
        collisionProbe.xsp=collisionProbe.ysp=MotionBinary64::fromInteger(1);
        assert(collisionProbe.wallCollision([&](auto x,auto y,int target,bool& hit){
            return jellies.meeting(jelly,x,y,target,hit);
        })&&collisionProbe.xcollision==1&&collisionProbe.ycollision==1);
        InstanceHandle playerShared;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::player,world->playerHandle(),playerShared));
        struct JellyPlayer {
            MotionBinary64 y;unsigned& stomps;unsigned& damages;
            bool globalY(MotionBinary64& out){out=y;return true;}
            bool stomp(){++stomps;return true;}
            bool damage(int a,int b,int c){assert(a==1&&b==3&&c==2);++damages;return true;}
        };
        unsigned stomps=0,damages=0;
        const RunnerCollisionEventContext::Frame playerContact{jellyShared,playerShared,4,35,115,1227};
        for(int playerY:{700,702})assert(owner->dispatchWhiteJellyPlayerCollision(playerContact,
            [&](auto& actor){assert(actor.sourceId==899000);return JellyPlayer{MotionBinary64::fromInteger(playerY),stomps,damages};}));
        assert(stomps==1&&damages==1);
        InstanceHandle peer,peerShared;
        assert(jellies.create(899002,{0x43800000u},{0x44300000u},jellyRandom,
            [&](auto h){return JellyOps{jellies,h};},peer));
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::whiteJelly,peer,peerShared));
        const RunnerCollisionEventContext::Frame enemyContact{jellyShared,peerShared,4,115,115,1226};
        auto expectedRandom=world->randomState();
        const auto expectedDirection=MotionBinary64::fromInteger(int(expectedRandom.irandomSmall(359)));
        unsigned pointCalls=0,setterCalls=0;
        auto point=[&](MotionBinary64 x,MotionBinary64 y,MotionBinary64 tx,MotionBinary64 ty,MotionBinary64& out){
            ++pointCalls;
            assert(x.high==MotionBinary64::fromInteger(272).high&&y.high==MotionBinary64::fromInteger(704).high&&
                tx.high==MotionBinary64::fromInteger(256).high&&ty.high==y.high);
            out=MotionBinary64::fromInteger(180);return true;
        };
        auto setter=[&](InstanceHandle self,MotionBinary64 wanted,MotionBinary64& accepted){
            assert(self.slot==jellyShared.slot&&self.generation==jellyShared.generation);
            ++setterCalls;accepted=wanted;return true;
        };
        assert(owner->dispatchWhiteJellyEnemyCollision(enemyContact,point,setter)&&!pointCalls&&setterCalls==1);
        assert(jellies.find(jelly)->direction.high==expectedDirection.high&&
            jellies.find(jelly)->direction.low==expectedDirection.low);
        assert(world->randomState().stateIndex()==expectedRandom.stateIndex());
        for(unsigned word=0;word<16;++word)assert(world->randomState().stateWord(word)==expectedRandom.stateWord(word));
        RunnerImageValue acceptedPosition;
        assert(jellies.position(peer,false,{0x43880000u},acceptedPosition));
        assert(owner->dispatchWhiteJellyEnemyCollision(enemyContact,point,setter)&&pointCalls==1&&setterCalls==2&&
            jellies.find(jelly)->direction.high==MotionBinary64::fromInteger(180).high);
        assert(world->randomState().stateIndex()==expectedRandom.stateIndex());
        for(unsigned word=0;word<16;++word)assert(world->randomState().stateWord(word)==expectedRandom.stateWord(word));
        assert(jellies.markDestroyed(peer)&&owner->prepareDraw()&&!jellies.find(peer));
        InstanceHandle fieldShared;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::timeField,
            {0,world->timeFields().identityAt(0)},fieldShared));
        const RunnerCollisionEventContext::Frame fieldContact{jellyShared,fieldShared,4,82,83,1066};
        assert(jellies.dispatch(jelly,[](auto& r){r.timefieldImmune=true;return true;}));
        assert(owner->dispatchWhiteJellyFieldCollision(fieldContact)&&jellies.find(jelly)->health==25);
        assert(jellies.dispatch(jelly,[](auto& r){r.timefieldImmune=false;return true;}));
        assert(owner->dispatchWhiteJellyFieldCollision(fieldContact)&&jellies.find(jelly)->health==0&&
            jellies.find(jelly)->alive&&!jellies.find(jelly)->destroyRequested);
        for(unsigned frame=0;frame<8;++frame)assert(jellies.advanceAnimation(jelly));
        assert(jellies.sprite(jelly,175)&&jellies.find(jelly)->imageIndex.bits==0&&owner->prepareDraw());
        assert(jellies.find(jelly)->maskId==174&&jellies.sprite(jelly,176)&&
            jellies.imageIndex(jelly,{})&&jellies.advanceAnimation(jelly)&&owner->prepareDraw());
        assert(jellies.activate(jelly,false)&&owner->prepareDraw()&&jellies.activate(jelly,true));
        assert(jellies.markDestroyed(jelly)&&jellies.find(jelly)&&owner->population(83,familyCount)&&familyCount==1);
        assert(owner->population(115,familyCount)&&familyCount==0);
        familyCount=123;
        assert(!owner->population(-1,familyCount)&&familyCount==123);
        assert(owner->prepareDraw()&&!jellies.find(jelly)&&
            !GameplayCollisionView(*world).contains(jellyShared));
        InstanceHandle jellyReplacement;
        assert(jellies.create(899001,{0x43788000u},{0x442f0000u},jellyRandom,
            [&](auto h){return JellyOps{jellies,h};},jellyReplacement));
        assert(jellyReplacement.slot==jelly.slot&&jellyReplacement.generation!=jelly.generation);
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,2,300)!=ResultsDestination::none&&
            world->scattered()==store&&!store->find(local)&&!GameplayCollisionView(*world).contains(shared));
        assert(world->spawners()==&spawners&&!spawners.size()&&!spawners.find(spawnerKey)&&
            !GameplayCollisionView(*world).contains(spawnerShared));
        assert(world->whiteJellies()==&jellies&&!jellies.size()&&!jellies.find(jellyReplacement));
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
