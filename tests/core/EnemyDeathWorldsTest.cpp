#include "core/GameplayWorld.h"
#include "core/StartupRuntime.h"
#include "core/StoredWorldCollision.h"
#include "core/GameplayCollisionOwner.h"
#include "../../generated/downwell-area-titles.h"
#include <cassert>
#include <memory>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
static std::vector<unsigned char> load(const char* path){
    std::ifstream input(path,std::ios::binary);return {std::istreambuf_iterator<char>(input),{}};
}
template<class World,class Kills> void check(World& world,Kills kills,unsigned failure){
    const auto before=world.runCombo().state.count;const auto killed=kills();
    unsigned calls=0;bool noKill=true;
    struct Audio {
        unsigned& calls;unsigned failure;
        bool isPlaying(int id,bool& result){assert(id==0);++calls;result=true;return failure!=1;}
        bool stop(int id){assert(id==0&&calls==1);++calls;return failure!=2;}
    } audio{calls,failure};
    const bool okay=world.recordEnemyDeath(true,audio,[&]{
        assert(kills()==killed+1&&calls==2);++calls;noKill=false;return failure!=3;
    },[&]{assert(!noKill&&calls==3);++calls;return failure!=4;
    },[&]{assert(calls==4);++calls;return failure!=5;});
    assert(okay==(failure==0)&&world.effectsSucceeded()==okay&&
        world.runCombo().state.count==before+1&&kills()==killed+(failure&&failure<3?0:1)&&
        calls==(failure?failure:5));
}
template<class World,class Kills> void checkOwnedAchievement(World& world,Kills kills,unsigned failure){
    assert(world.noKillAchievement());const auto before=kills();unsigned calls=0;
    struct Audio {
        unsigned& calls;unsigned failure;
        bool isPlaying(int id,bool& result){assert(id==0);++calls;result=true;return failure!=1;}
        bool stop(int id){assert(id==0&&calls==1);++calls;return failure!=2;}
    } audio{calls,failure};
    const bool okay=world.recordEnemyDeath(true,audio,[&]{
        assert(kills()==before+1&&!world.noKillAchievement()&&calls==2);++calls;return failure!=3;
    },[&]{assert(!world.noKillAchievement()&&calls==3);++calls;return failure!=4;});
    assert(okay==(failure==0)&&world.effectsSucceeded()==okay&&
        world.noKillAchievement()==(failure==1||failure==2)&&
        kills()==before+(failure==1||failure==2?0:1)&&calls==(failure?failure:4));
}
template<class World> void checkCue(World& world,unsigned failure,bool ending=false,bool changeArea=false,bool musicPlaying=false){
    using namespace downwell;
    const_cast<RunStage&>(world.runStage()).area=ending?5:1;
    bool firstLand=false;int bgm=10;unsigned calls=0;
    std::vector<int> trace;
    auto operation=[&](int tag){trace.push_back(tag);return ++calls!=failure;};
    struct Audio {
        decltype(operation)& op;bool& firstLand;bool ending,musicPlaying;
        bool isPlaying(int id,bool& playing){
            assert(firstLand);playing=id==186||id==900||(ending&&id==10)||(musicPlaying&&id==20);
            return op(id==186?2:id==0?7:id==900?12:5);
        }
        bool stop(int id){assert(id==186||id==10);return op(id==186?3:13);}
        bool setGain(int id,MotionBinary64 gain,unsigned time){
            assert((id==20||id==900)&&gain.high==0x3ff00000u&&!gain.low&&!time);
            return op(id==20?6:10);
        }
        bool play(int id,int priority,bool loop,int& voice){
            assert(id==20&&priority==100&&loop);voice=900;return op(8);
        }
        bool getGain(int id,MotionBinary64& gain){assert(id==900);gain=MotionBinary64::fromInteger(1);return op(9);}
        bool setPitch(int id,MotionBinary64 pitch){assert(id==900&&pitch.high==0x3ff00000u&&!pitch.low);return op(11);}
    } audio{operation,firstLand,ending,musicPlaying};
    SoundPlayState caller;SoundPlayEnvironment env;
    const auto x=MotionBinary64::fromInteger(241),y=MotionBinary64::fromInteger(603);
    assert(world.beginLevelCue(firstLand,true,bgm,x,y,[](auto...){std::abort();return false;},
        [](int,int&){std::abort();return false;},caller,env,audio)&&!firstLand&&bgm==10&&!calls);
    const bool ok=world.beginLevelCue(firstLand,false,bgm,x,y,[&](int object,auto px,auto py){
        assert(!firstLand&&!ending&&object==372&&px.high==x.high&&px.low==x.low&&py.high==y.high&&py.low==y.low);
        if(changeArea)const_cast<RunStage&>(world.runStage()).area=2;
        return operation(1);
    },[&](int area,int& id){assert(firstLand&&area==(changeArea?2:ending?5:1));id=20;return operation(4);},caller,env,audio);
    const std::vector<int> expected=ending?std::vector<int>{2,3,5,13,4,4}:musicPlaying?std::vector<int>{1,2,3,4,4,5}:
        std::vector<int>{1,2,3,4,4,5,6,7,8,9,10,11,12};
    assert(ok==(failure==0)&&world.effectsSucceeded()==ok&&trace.size()==(failure?failure:expected.size()));
    for(unsigned i=0;i<trace.size();++i)assert(trace[i]==expected[i]);
    assert(firstLand==(ending||failure!=1));
    assert(bgm==((!failure||failure>(ending?6u:5u))?20:10));
    if(ok){
        const auto before=calls;
        assert(world.beginLevelCue(firstLand,false,bgm,x,y,[](auto...){std::abort();return false;},
            [](int,int&){std::abort();return false;},caller,env,audio)&&calls==before);
    }
}
template<class World> void checkDamageDuck(World& world,unsigned failure){
    downwell::ContactDamageScripts caller;
    struct Audio {
        int music;unsigned failure,calls=0;
        bool call(int id){assert(id==music);return ++calls!=failure;}
        bool isPlaying(int id,bool& playing){playing=true;return call(id);}
        bool getGain(int id,downwell::MotionBinary64& value){value=downwell::MotionBinary64::fromInteger(1);return call(id);}
        bool setGain(int id,downwell::MotionBinary64 value,int duration){
            assert(duration==(calls==2?0:500)&&value.high==(calls==2?0x3fe66666u:0x3ff00000u));
            return call(id);
        }
    } audio{world.backgroundMusicId(),failure};
    assert(world.duckMusic(caller.music,500,{0x3fe66666u,0x66666666u},audio)==!failure&&
        world.effectsSucceeded()==!failure&&audio.calls==(failure?failure:4)&&caller.music.duckDuration==500);
    if(failure){const auto count=audio.calls;
        assert(!world.duckMusic(caller.music,123,{},audio)&&audio.calls==count&&caller.music.duckDuration==500);}
}
int main(){
    for(unsigned failure=0;failure<=4;++failure){
        auto native=std::make_unique<GameplayWorld>(300,true,0,false);checkDamageDuck(*native,failure);
        auto stored=std::make_unique<downwell::StartupRuntime>();checkDamageDuck(*stored,failure);
    }
    for(int area=0;area<=6;++area){
        int music=-1;const bool valid=downwell::RunSession::areaMusic(area,music);
        assert(valid==(area>=1&&area<=5)&&music==(valid?193+area:-1));
    }
    {
        auto native=std::make_unique<GameplayWorld>(300,true,0,false);
        checkCue(*native,0,false,false,true);
        auto stored=std::make_unique<downwell::StartupRuntime>();
        checkCue(*stored,0,false,false,true);
    }
    {
        auto native=std::make_unique<GameplayWorld>(300,true,0,false);
        checkCue(*native,0,false,true);
        auto stored=std::make_unique<downwell::StartupRuntime>();
        checkCue(*stored,0,false,true);
    }
    for(bool ending:{false,true})for(unsigned failure=0;failure<=(ending?6u:13u);++failure){
        auto native=std::make_unique<GameplayWorld>(300,true,0,false);
        checkCue(*native,failure,ending);
        auto stored=std::make_unique<downwell::StartupRuntime>();
        checkCue(*stored,failure,ending);
    }
    {
        downwell::InstanceRecord controllers[2];int speed=60;
        assert(downwell::MomentDelay::request(speed,[&](auto apply){
            return apply(controllers[0])&&apply(controllers[1]);
        }));
        assert(speed==30&&controllers[0].controllerSlow==30&&controllers[1].controllerSlow==30&&
            controllers[0].alarms.get(0)==1&&controllers[1].alarms.get(0)==1);
        speed=60;assert(downwell::MomentDelay::request(speed,[](auto){return true;})&&speed==60);
    }
    const auto roomsData=load("generated/downwell-rooms.dwrm"),objectsData=load("generated/downwell-objects.dwob");
    downwell::RoomPack rooms(roomsData.data(),roomsData.size());
    downwell::ObjectPack objects(objectsData.data(),objectsData.size());
    assert(rooms.valid()&&objects.valid());
    const auto spriteData=load("generated/downwell-sprites.dwsc"),maskData=load("generated/downwell-sprite-masks.dwsm");
    downwell::SpriteCollisionPack sprites(spriteData.data(),spriteData.size());
    downwell::SpriteMaskPack masks(maskData.data(),maskData.size());
    assert(sprites.valid()&&masks.valid());
    const auto fontData=load("generated/downwell-fonts.dwfn");downwell::FontPack fonts(fontData.data(),fontData.size());
    assert(fonts.valid());
    for(unsigned seed=0;seed<17;++seed){
        using namespace downwell;
        auto storedOwner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto stored=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,seed,nullptr,storedOwner->binding());
        assert(storedOwner->attach(*stored));stored->skipToGameForTesting();
        auto skeletons=stored->skeletonRegistration<1>();auto bones=stored->boneRegistration<2>();
        auto blood=stored->bloodRegistration<2>();auto smoke=stored->smokeRegistration<8>();
        auto flash=stored->flashBallRegistration<8>();auto currency=stored->currencyRegistration<3>();
        auto corpses=stored->corpseRegistration<1>();auto titles=stored->levelNumberRegistration<1>();
        InstanceHandle skeleton;assert(stored->spawnSkeleton(skeletons,240,40000,skeleton));
        const auto initialRandom=stored->randomState();auto first=stored->nextRuntimeInstanceId();
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto native=std::make_unique<GameplayWorld>(300,true,0,seed==15,owner->enemyBinding(),owner->breakableBinding(),
            owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*native)&&owner->configureCorpseMasks(masks));
        int storedSpeed=60,nativeSpeed=60;InstanceHandle controller,other,shared;
        assert(native->bindControllerRoomSpeed(nativeSpeed)&&owner->runControllers().create(105500,{}, {},
            [](InstanceHandle){return true;},controller));
        assert(native->createSkeletonWithSourceId(first-1,240,40000,other));
        native->configureEnemyRandom(initialRandom);
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::skeleton,other,shared));
        auto prepare=[&](auto& actor,auto&,bool&){actor.money=14;
            if(seed!=15){actor.health=0;actor.takenImpact=MotionBinary64::fromInteger(2);}return true;};
        assert(skeletons.dispatch(skeleton,prepare)&&owner->skeletons().dispatch(other,prepare));
        struct Audio {
            std::vector<int> plays;
            bool isPlaying(int,bool& playing){playing=false;return true;}
            bool stop(int){return true;}
            bool play(int id,int,bool,int& voice){plays.push_back(id);voice=1000+id;return true;}
            bool getGain(int,MotionBinary64& gain){gain=MotionBinary64::fromInteger(1);return true;}
            bool setGain(int,MotionBinary64,int){return true;}
            bool setPitch(int,MotionBinary64){return true;}
        } storedAudio,nativeAudio;
        struct Queries {
            bool wall(InstanceHandle,MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
            bool tile(InstanceHandle,int layer,MotionBinary64,MotionBinary64,bool& hit){assert(layer==100);hit=false;return true;}
        } queries;
        auto tile=[&](auto key,int layer,auto x,auto y,bool& hit){return queries.tile(key,layer,x,y,hit);};
        const char* language="english";unsigned font=2;bool hard=false;
        LevelNumberTextContext text{fonts,area_titles::entries,area_titles::count,language,font,hard};
        TimeStopEffects<4> stop;SoundPlayEnvironment storedEnv,nativeEnv;
        // Reach death through resource-bound contacts, not a health override.
        // Each blast owns separate hit memory; its impact Create consumes an
        // ID before the next blast. Both worlds then run the same death Step.
        auto blasts=stored->blastRegistration<2>();
        if(seed==15){
            unsigned next=first;
            for(unsigned hit=0;hit<2;++hit){
                InstanceHandle a,b,bShared;
                assert(stored->withMomentDelayRoomSpeed(storedSpeed,[&]{
                    return stored->spawnBlast(blasts,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(40000),a);
                }));
                assert(owner->createBlast(next++,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(40000),b)&&
                    GameplayCollisionView::encode(GameplayCollisionView::Kind::blast,b,bShared));
                RunnerCollisionEventContext sc,nc;
                assert(stored->withMomentDelayRoomSpeed(storedSpeed,[&]{
                    return storedOwner->dispatchCollisionEvent(sc,blasts.worldHandle(a),skeletons.worldHandle(skeleton),{true,false},
                        [&](const auto& event){return stored->dispatchBlastSkeletonCollisionWithEffects(blasts,skeletons,event,
                            [&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);},storedEnv,storedAudio);});
                }));
                assert(owner->dispatchCollisionEvent(nc,bShared,shared,{true,false},[&](const auto& event){
                    return owner->dispatchBlastSkeletonCollisionWithEffects(event,[&](unsigned& id){id=next++;return true;},
                        [&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);},nativeEnv,nativeAudio);
                }));
                assert(skeletons.instances().find(skeleton)->state.health==(hit?-40:10)&&
                    owner->skeletons().find(other)->state.health==(hit?-40:10)&&next==stored->nextRuntimeInstanceId());
            }
            first=next;
        }
        if(seed==16){
            // Children born during this ordinary Step must not receive an
            // ordinary Step from the snapshot already being dispatched.
            auto noChildStep=[](auto&&...)->bool{std::abort();};
            assert(stored->withMomentDelayRoomSpeed(storedSpeed,[&]{
                return stored->withBloodEvents(blood,noChildStep,[&]{
                    return stored->withSmokeEvents(smoke,[&]{
                        return stored->withFlashBallEvents(flash,[&]{
                            return stored->withCurrencyEvents(currency,noChildStep,[&]{
                                return stored->withCorpseEvents(corpses,noChildStep,[&]{
                                    return stored->withLevelNumberEvents(titles,[&]{
                                        return stored->withRegisteredSkeletonEvents(skeletons,bones,blood,smoke,flash,
                                            currency,corpses,titles,stop,true,text,tile,storedEnv,storedAudio,[&]{
                                                return stored->stepWithOwnedDebris(Input{},noChildStep);
                                            });
                                    });
                                });
                            });
                        });
                    });
                });
            }));
            assert(stored->mainEnemiesDefeated()==1&&skeletons.instances().size()==0&&
                stored->debrisEffects().size()==2&&blood.instances().size()==2&&flash.instances().size()>=4);
            // Native death also enters through the ordinary gameplay tick,
            // rather than a manually invoked Step after that tick finishes.
            auto phase=std::make_unique<GameplayCollisionOwner::SkeletonBoneStepOrder>();
            auto next=first;unsigned selected=0;
            assert(owner->withSkeletonBoneSteps(*phase,[&](InstanceHandle h){
                ++selected;
                return owner->stepRegisteredSkeleton(h,stop,true,[&](std::uint32_t& id){id=next++;return true;},
                    tile,queries,text,nativeEnv,nativeAudio);
            },noChildStep,[&]{native->step(0);return native->effectsSucceeded();}));
            assert(selected==1&&native->debrisEffects().size()==2&&owner->blood().size()==2&&
                owner->registeredFlash().size()>=4);
            continue;
        }
        assert(stored->withMomentDelayRoomSpeed(storedSpeed,[&]{
            return skeletons.dispatchPublished(skeleton,[&](auto& actor,auto& throwing,auto& publish,bool& good){
                auto effects=stored->registeredSkeletonEffects(actor,throwing,publish,bones,blood,smoke,flash,currency,
                    corpses,titles,stop,true,text,tile,storedEnv,storedAudio);
                return actor.stepPublished(false,{}, {},publish,effects,good);
            });
        }));
        auto next=first;
        assert(owner->stepRegisteredSkeleton(shared,stop,true,[&](std::uint32_t& id){id=next++;return true;},
            tile,queries,text,nativeEnv,nativeAudio));
        assert(next==stored->nextRuntimeInstanceId()&&storedSpeed==nativeSpeed&&storedAudio.plays==nativeAudio.plays);
        assert(stored->randomDrawCount()==native->randomDrawCount());
        for(unsigned i=0;i<16;++i)assert(stored->randomState().stateWord(i)==native->randomState().stateWord(i));
        const auto& a=skeletons.instances().find(skeleton)->state;const auto& b=owner->skeletons().find(other)->state;
        assert(a.destroyRequested&&b.destroyRequested&&a.myFx==b.myFx&&a.myDeadBody==b.myDeadBody&&
            a.reward.myGem==b.reward.myGem&&a.smokeScript.smokenum==b.smokeScript.smokenum);
        unsigned compared=0;
        auto child=[&](std::uint32_t id,int object,RunnerImageValue x,RunnerImageValue y){
            const auto* r=stored->instance(id);std::uint32_t sx,sy;
            assert(r&&id>=first&&id<next&&r->objectId==object);
            std::memcpy(&sx,&r->preciseX,4);std::memcpy(&sy,&r->preciseY,4);
            assert(sx==x.bits&&sy==y.bits);++compared;
        };
        GameplayCollisionView view(*native);
        assert(owner->visitRegistrationOrder([&](auto key,int,auto){
            GameplayCollisionView::Record r;assert(view.resolve(key,r));
            if(r.blood)child(r.blood->sourceId,220,r.blood->x,r.blood->y);
            if(r.smoke)child(r.smoke->sourceId,225,r.smoke->x,r.smoke->y);
            if(r.flashBall)child(r.flashBall->sourceId,224,r.flashBall->x,r.flashBall->y);
            if(r.currency)child(r.currency->sourceId,248,r.currency->x,r.currency->y);
            if(r.corpse)child(r.corpse->sourceId,263,r.corpse->x,r.corpse->y);
            if(r.levelNumber)child(r.levelNumber->sourceId,372,r.levelNumber->x,r.levelNumber->y);
            if(r.debris)child(r.debris->sourceId,370,r.debris->effect->x,r.debris->effect->y);
            return true;
        }));
        assert(compared==next-first);
    }
    for(auto mode:{downwell::GameplayCollisionOwner::Ownership::reference,
                  downwell::GameplayCollisionOwner::Ownership::native,
                  downwell::GameplayCollisionOwner::Ownership::nativeScenery})for(unsigned failure=0;failure<=19;++failure){
        using namespace downwell;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),owner->breakableBinding(),
            owner->playerBinding(),mode==GameplayCollisionOwner::Ownership::reference?&owner->terrain():nullptr);
        assert(owner->attach(*world)&&owner->configureCorpseMasks(masks));
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        if(mode!=GameplayCollisionOwner::Ownership::reference)assert(world->bindNativeTerrain(*terrain));
        int speed=60;InstanceHandle controller,skeleton,shared;
        assert(world->bindControllerRoomSpeed(speed)&&owner->runControllers().create(105500,{}, {},
            [](InstanceHandle){return true;},controller));
        assert(world->createSkeletonWithSourceId(105501,240,200,skeleton));
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::skeleton,skeleton,shared));
        assert(owner->skeletons().dispatch(skeleton,[](auto& actor,auto&,bool&){
            actor.health=0;actor.money=14;actor.takenImpact=MotionBinary64::fromInteger(2);return true;
        }));
        struct Audio {
            std::vector<int> played;
            bool isPlaying(int,bool& playing){playing=false;return true;}
            bool stop(int){return true;}
            bool play(int id,int,bool,int& voice){played.push_back(id);voice=1000+id;return true;}
            bool getGain(int,MotionBinary64& gain){gain=MotionBinary64::fromInteger(1);return true;}
            bool setGain(int,MotionBinary64,int){return true;}
            bool setPitch(int,MotionBinary64){return true;}
        } audio;
        struct Queries {
            bool wall(InstanceHandle,MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
            bool tile(InstanceHandle,int layer,MotionBinary64,MotionBinary64,bool& hit){assert(layer==100);hit=false;return true;}
        } queries;
        const char* language="english";unsigned font=2;bool hard=false;
        LevelNumberTextContext text{fonts,area_titles::entries,area_titles::count,language,font,hard};
        TimeStopEffects<4> stop;SoundPlayEnvironment environment;std::uint32_t next=106000;unsigned allocations=0;
        const bool complete=owner->stepRegisteredSkeleton(shared,stop,true,[&](std::uint32_t& id){
            ++allocations;if(allocations==failure)return false;id=next++;return true;},
            [&](auto key,int layer,auto x,auto y,bool& hit){return queries.tile(key,layer,x,y,hit);},
            queries,text,environment,audio);
        assert(complete==!failure);
        if(failure){
            assert(allocations==failure&&!world->effectsSucceeded()&&!owner->valid());
            const auto* retained=owner->skeletons().find(skeleton);
            assert(retained&&!retained->destroyRequested&&retained->state.takenImpact.high==0x40000000u);
            assert(!owner->stepRegisteredSkeleton(shared,stop,true,
                [&](std::uint32_t&){std::abort();return false;},
                [&](auto key,int layer,auto x,auto y,bool& hit){return queries.tile(key,layer,x,y,hit);},
                queries,text,environment,audio));
            continue;
        }
        assert(allocations==19);
        const auto* record=owner->skeletons().find(skeleton);assert(record&&record->destroyRequested);
        assert(record->state.myFx>=106001&&record->state.reward.myGem>record->state.myFx&&
            record->state.myDeadBody>record->state.reward.myGem&&!record->state.takenImpact.high);
        assert(world->enemiesDefeated()==1&&!world->noKillAchievement()&&speed==30);
        assert(owner->levelNumbers().size()==1&&owner->currency().size()==3&&
            owner->registeredCorpses().size()==1&&owner->debris().size()==2);
        assert(owner->registeredSmoke().size()>=4&&owner->registeredFlash().size()>=4);
        assert((audio.played==std::vector<int>{194,132}));
        assert(next==record->state.myDeadBody+3&&owner->prepareDraw()&&!owner->skeletons().find(skeleton));
    }
    for(unsigned failure:{0u,1u,2u})for(bool native:{false,true}){
        if(!native&&failure==1)continue; // Explicit allocator failure is native-only.
        using namespace downwell;
        auto prepare=[](auto& actor,auto& publish){
            actor.xx=actor.yy=MotionBinary64::fromInteger(-999);
            actor.xsp={0x3fb99999u,0x9999999au};return publish.facing(-1);
        };
        auto verify=[&](const SkeletonActor& actor,unsigned id,const CorpseAnimation* child){
            assert(actor.bodySprite==206&&actor.impactSp.high==0x40100000u&&!actor.impactSp.low);
            assert(actor.myDeadBody==(failure?0:id));
            assert(actor.xsp.high==(failure?0x3fb99999u:0xbff80000u));
            if(failure==1){assert(!child);return;}
            assert(child&&child->x.bits==0x43700000u&&child->y.bits==0x471c4000u);
            if(!failure)assert(child->spriteId==206&&child->imageXscale.bits==0xbf800000u&&
                child->ysp.high==0x40100000u&&child->xsp.high==0xc0180000u&&child->explosionFx==102);
        };
        if(native){
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
            auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),owner->breakableBinding(),
                owner->playerBinding(),&owner->terrain());assert(owner->attach(*world)&&owner->configureCorpseMasks(masks));
            InstanceHandle skeleton;assert(world->createSkeletonWithSourceId(505001,240,40000,skeleton));
            auto random=world->randomState();unsigned allocations=0;
            assert(owner->skeletons().dispatchPublished(skeleton,[&](auto& actor,auto&,auto& publish,bool&){
                assert(prepare(actor,publish));
                return world->emitSkeletonCorpse(publish,MotionBinary64::fromInteger(9),[&](std::uint32_t& id){
                    assert(actor.bodySprite==206&&actor.impactSp.high==0x40100000u&&!actor.myDeadBody);
                    ++allocations;id=506000;return failure!=1;
                },[&](auto,int layer,auto,auto,bool& hit){assert(layer==100);hit=false;return failure!=2;});
            })==!failure);
            const auto* corpse=owner->registeredCorpses().find(owner->registeredCorpses().sourceHandle(506000));
            verify(owner->skeletons().find(skeleton)->state,506000,corpse?&corpse->state:nullptr);
            assert(allocations==1&&world->effectsSucceeded()==!failure);
            if(failure!=1)for(unsigned i=0;i<3;++i)random.nextU32();
            for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==world->randomState().stateWord(i));
        }else{
            auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
            auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,nullptr,owner->binding());
            assert(owner->attach(*world));world->skipToGameForTesting();
            auto skeletons=world->skeletonRegistration<1>();auto corpses=world->corpseRegistration<1>();InstanceHandle skeleton;
            assert(world->spawnSkeleton(skeletons,240,40000,skeleton));
            const auto id=world->nextRuntimeInstanceId();auto random=world->randomState();
            assert(skeletons.dispatchPublished(skeleton,[&](auto& actor,auto&,auto& publish,bool&){
                assert(prepare(actor,publish));
                return world->emitSkeletonCorpse(corpses,publish,MotionBinary64::fromInteger(9),
                    [&](auto,int layer,auto,auto,bool& hit){assert(layer==100);hit=false;return failure!=2;});
            })==!failure);
            verify(skeletons.instances().find(skeleton)->state,id,corpses.state(corpses.sourceHandle(id)));
            assert(world->effectsSucceeded()==!failure);
            for(unsigned i=0;i<3;++i)random.nextU32();
            for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==world->randomState().stateWord(i));
        }
    }
    for(bool stomped:{false,true})for(bool failSecond:{false,true})for(bool native:{false,true}){
        using namespace downwell;
        auto verify=[&](const SkeletonActor& actor,unsigned first,unsigned calls,const auto& world,auto child){
            assert(calls==(failSecond?2u:3u)&&world.effectsSucceeded()==!failSecond);
            assert(actor.reward.atrState==stomped&&actor.reward.myGem==first+(failSecond?0:2));
            assert(actor.reward.moniAmount.high==(failSecond?0x40100000u:0u)&&!actor.reward.moniAmount.low);
            for(unsigned i=0;i<calls;++i){
                const auto* state=child(first+i);assert(state);
                const bool complete=!failSecond||i==0;
                assert(state->getAmount==(i==0?10:2)&&state->size==(i==0?1:0));
                assert(state->attracted==(complete&&stomped));
                assert(state->x.bits==0x43700000u&&state->y.bits==0x471c4000u); // published 240,40000
            }
        };
        if(native){
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
            auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),owner->breakableBinding(),
                owner->playerBinding(),&owner->terrain());assert(owner->attach(*world));
            InstanceHandle skeleton;assert(world->createSkeletonWithSourceId(505001,240,40000,skeleton));
            struct Queries {
                bool fail;unsigned calls=0;
                bool wall(InstanceHandle,MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
                bool tile(InstanceHandle,int layer,MotionBinary64,MotionBinary64,bool& hit){
                    assert(layer==100);hit=false;return !(++calls==2&&fail);
                }
            } queries{failSecond};
            auto random=world->randomState();std::uint32_t next=506000;
            assert(owner->skeletons().dispatch(skeleton,[&](auto& actor,auto&,bool&){
                actor.money=14;actor.stomped=stomped;actor.xx=actor.yy=MotionBinary64::fromInteger(999);
                return world->emitSkeletonCurrency(objects,actor,[&](std::uint32_t& id){id=next++;return true;},queries);
            })==!failSecond);
            verify(owner->skeletons().find(skeleton)->state,506000,queries.calls,*world,[&](unsigned id){
                const auto* r=owner->currency().find(owner->currency().handleForSourceId(id));return r?&r->state:nullptr;
            });
            for(unsigned i=0;i<queries.calls*8;++i)random.nextU32();
            for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==world->randomState().stateWord(i));
        }else{
            auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
            auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,nullptr,owner->binding());
            assert(owner->attach(*world));world->skipToGameForTesting();
            auto skeletons=world->skeletonRegistration<1>();auto gems=world->currencyRegistration<3>();InstanceHandle skeleton;
            assert(world->spawnSkeleton(skeletons,240,40000,skeleton));
            const auto first=world->nextRuntimeInstanceId();auto random=world->randomState();unsigned calls=0;
            assert(skeletons.dispatch(skeleton,[&](auto& actor,auto&,bool&){
                actor.money=14;actor.stomped=stomped;actor.xx=actor.yy=MotionBinary64::fromInteger(999);
                return world->emitSkeletonCurrency(gems,actor,[&](InstanceHandle,int layer,auto,auto,bool& hit){
                    assert(layer==100);hit=false;return !(++calls==2&&failSecond);
                });
            })==!failSecond);
            verify(skeletons.instances().find(skeleton)->state,first,calls,*world,[&](unsigned id){
                const auto* r=world->instance(id);return r?gems.find(gems.handle(*r)):nullptr;
            });
            for(unsigned i=0;i<calls*8;++i)random.nextU32();
            for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==world->randomState().stateWord(i));
        }
    }
    for(auto mode:{downwell::GameplayCollisionOwner::Ownership::reference,
                  downwell::GameplayCollisionOwner::Ownership::native,
                  downwell::GameplayCollisionOwner::Ownership::nativeScenery}){
        using namespace downwell;
        const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),
            owner->playerBinding(),native?nullptr:&owner->terrain(),&rooms);
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain)));
        SkeletonActor actor;actor.xx=MotionBinary64::fromInteger(240);actor.yy=MotionBinary64::fromInteger(704);
        auto random=world->randomState();SmokeBurstCaller expected;std::vector<FlashBallEffect> children;
        assert((flashBurstPublished<6,2,10>(expected,actor.xx,actor.yy,random,[&](int,auto x,auto y){
            FlashBallEffect child;if(!child.create(x,y,random))return false;children.push_back(child);return true;
        })));
        unsigned next=510000;
        assert(world->emitSkeletonFlash(actor,[&](unsigned& id){id=next++;return true;}));
        assert(owner->registeredFlash().size()==children.size()&&actor.smokeScript.smokenum==int(children.size()));
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==world->randomState().stateWord(i));
        unsigned drawn=0;
        assert(world->visitRoomActorsReadOnly([&](auto kind,auto key,auto depth){
            if(kind==GameplayWorld::DrawActor::flashBall){
                const auto* r=owner->registeredFlash().find(key);RunnerImageValue expectedDepth;
                assert(expectedDepth.assignInteger(-200000)&&r&&depth.bits==expectedDepth.bits);
                const auto& expectedChild=children[r->sourceId-510000];
                assert(r->x.bits==expectedChild.x.bits&&r->y.bits==expectedChild.y.bits);
                GameplayCollisionView::Record resolved;InstanceHandle shared;
                assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::flashBall,key,shared)&&
                    GameplayCollisionView(*world).resolve(shared,resolved)&&resolved.flashBall==r);
                ++drawn;
            }
            return true;
        })&&drawn==children.size());
        for(unsigned frame=0;frame<7;++frame){
            world->step(0);assert(world->effectsSucceeded()&&owner->valid()&&owner->prepareDraw());
            assert(owner->registeredFlash().size()==(frame==6?0u:children.size()));
        }
        assert(world->emitSkeletonFlash(actor,[&](unsigned& id){id=next++;return true;}));
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,2,0)==ResultsDestination::gameplay&&
            owner->registeredFlash().size()==0&&owner->valid());
    }
    for(unsigned failAllocation=0;failAllocation<=2;++failAllocation){
        using namespace downwell;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));InstanceHandle skeleton;
        assert(world->createSkeletonWithSourceId(505001,240,704,skeleton));
        auto random=world->randomState();const auto draws=world->randomDrawCount();unsigned calls=0;
        const bool okay=owner->skeletons().dispatch(skeleton,[&](auto& actor,auto&,bool&){
            actor.xx=actor.yy=MotionBinary64::fromInteger(999); // Script reads published x/y, not private accumulators.
            return world->emitSkeletonBlood(actor,[&](unsigned& id){++calls;id=506000+calls;return calls!=failAllocation;});
        });
        const unsigned births=calls-unsigned(failAllocation!=0);
        const auto* actor=owner->skeletons().find(skeleton);
        assert(okay==!failAllocation&&actor&&actor->state.myFx==(births?506000+births:0)&&
            world->bloodInstances()->size()==births&&world->randomDrawCount()==draws+calls);
        for(unsigned i=0;i<calls;++i)random.nextU32();
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==world->randomState().stateWord(i));
        for(unsigned i=0;i<births;++i){const auto* child=world->bloodInstances()->find({i,i+1});
            assert(child&&child->sourceId==506001+i&&child->x.bits==0x43700000u&&child->y.bits==0x44300000u&&child->spriteId==84);}
        if(okay){
            SmokeBurstCaller expected;SmokeEffect children[6];unsigned count=0,allocated=0;
            assert((smokeBurstPublished<5,1>(expected,MotionBinary64::fromInteger(999),MotionBinary64::fromInteger(999),random,
                [&](int object,auto x,auto y){return object==225&&count<6&&children[count++].create(x,y,random);})));
            assert(owner->skeletons().dispatch(skeleton,[&](auto& state,auto&,bool&){
                return world->emitSkeletonSmoke(state,[&](unsigned& id){id=506003+allocated++;return true;});
            }));
            assert(allocated==count&&actor->state.myFx==506002&&actor->state.smokeScript.smokenum==int(count)&&
                actor->state.smokeScript.smx.high==expected.smx.high&&actor->state.smokeScript.smx.low==expected.smx.low);
            for(unsigned i=0;i<count;++i){const auto* child=world->registeredSmoke()->find(world->registeredSmoke()->sourceHandle(506003+i));
                assert(child&&child->x.bits==children[i].x.bits&&child->y.bits==children[i].y.bits);}
            for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==world->randomState().stateWord(i));
        }
    }
    for(bool small:{false,true}){
        using namespace downwell;
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,nullptr,owner->binding());
        assert(owner->attach(*world));world->skipToGameForTesting();
        auto skeletons=world->skeletonRegistration<1>();InstanceHandle skeleton;
        assert(world->spawnSkeleton(skeletons,240,704,skeleton));
        auto run=[&](auto& blood){
            const auto first=world->nextRuntimeInstanceId();const auto draws=world->randomDrawCount();
            const bool okay=skeletons.dispatch(skeleton,[&](auto& actor,auto&,bool&){
                actor.xx=actor.yy=MotionBinary64::fromInteger(999);
                return world->emitSkeletonBlood(blood,actor);
            });
            assert(okay==!small&&skeletons.instances().find(skeleton)->state.myFx==first+unsigned(!small)&&
                blood.instances().size()==(small?1u:2u)&&world->randomDrawCount()==draws+2);
            for(unsigned i=0;i<(small?1u:2u);++i){const auto* child=world->instance(first+i);
                assert(child&&child->x==240&&child->y==704&&child->spriteId==84);}
            if(okay){
                auto smoke=world->smokeRegistration<6>();auto random=world->randomState();
                SmokeBurstCaller expected;unsigned count=0;
                assert((smokeBurstPublished<5,1>(expected,MotionBinary64::fromInteger(999),MotionBinary64::fromInteger(999),random,
                    [&](int object,auto x,auto y){SmokeEffect child;++count;return object==225&&child.create(x,y,random);})));
                assert(skeletons.dispatch(skeleton,[&](auto& actor,auto&,bool&){return world->emitSkeletonSmoke(smoke,actor);}));
                assert(skeletons.instances().find(skeleton)->state.myFx==first+1&&
                    skeletons.instances().find(skeleton)->state.smokeScript.smokenum==int(count));
                for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==world->randomState().stateWord(i));
                for(unsigned i=0;i<count;++i){const auto* child=world->instance(first+2+i);assert(child&&child->objectId==225);}
                assert(smoke.clear()&&blood.clear());
                auto flash=world->flashBallRegistration<8>();random=world->randomState();
                const auto flashFirst=world->nextRuntimeInstanceId();count=0;
                std::vector<FlashBallEffect> expectedFlashes;
                assert((flashBurstPublished<6,2,10>(expected,MotionBinary64::fromInteger(999),MotionBinary64::fromInteger(999),random,
                    [&](int object,auto x,auto y){FlashBallEffect child;++count;
                        if(object!=224||!child.create(x,y,random))return false;
                        expectedFlashes.push_back(child);return true;})));
                assert(skeletons.dispatch(skeleton,[&](auto& actor,auto&,bool&){return world->emitSkeletonFlash(flash,actor);}));
                assert(skeletons.instances().find(skeleton)->state.smokeScript.pvary==10);
                for(unsigned i=0;i<count;++i){
                    const auto* child=world->instance(flashFirst+i);
                    assert(child&&child->objectId==224&&child->spriteId==96&&child->imageIndex==1&&child->imageSpeed==0.3F);
                    const auto* record=flash.instances().find(flash.handle(*child));assert(record);
                    assert(record->x.bits==expectedFlashes[i].x.bits&&record->y.bits==expectedFlashes[i].y.bits);
                }
                for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==world->randomState().stateWord(i));
                assert(skeletons.clear());
                for(unsigned frame=0;frame<7;++frame)assert(world->withFlashBallEvents(flash,[&]{world->step(false);return world->effectsSucceeded();}));
                assert(flash.instances().size()==0&&flash.valid());
                for(unsigned i=0;i<count;++i)assert(!world->instance(flashFirst+i));
            }
        };
        if(small){auto blood=world->bloodRegistration<1>();run(blood);}
        else{auto blood=world->bloodRegistration<2>();run(blood);}
    }
    for(bool native:{false,true})for(bool stopped:{false,true})for(bool badFont:{false,true})for(bool musicPlaying:{false,true}){
        using namespace downwell;
        auto storedOwner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto stored=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,nullptr,storedOwner->binding());
        assert(storedOwner->attach(*stored));stored->skipToGameForTesting();
        auto titles=stored->levelNumberRegistration<1>();auto skeletons=stored->skeletonRegistration<1>();
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));int speed=60;InstanceHandle controller,skeleton;
        // Explicit native source-ID fixture; production startup admission is separate.
        if(native){
            assert(world->bindControllerRoomSpeed(speed)&&owner->runControllers().create(StartupRuntime::controllerInstanceId,{}, {},
                [&](InstanceHandle key){const auto* record=owner->runControllers().find(key);
                    return record&&record->controllerSlow==0&&record->alarms.get(0)==-1;},controller));
            assert(world->createSkeletonWithSourceId(505001,240,704,skeleton));
        }else assert(stored->spawnSkeleton(skeletons,240,704,skeleton));
        const unsigned titleId=native?505002:stored->nextRuntimeInstanceId();unsigned allocated=0,queries=0;
        const char* language="english";unsigned font=badFont?999:2;bool hard=false;
        LevelNumberTextContext text{fonts,area_titles::entries,area_titles::count,language,font,hard};
        TimeStopEffects<4> stop;stop.player.stopped=stopped;SoundPlayEnvironment environment;
        struct Audio {
            unsigned& queries;bool musicPlaying;unsigned plays=0;
            bool isPlaying(int id,bool& playing){
                ++queries;assert(id==(queries==1?0:queries==2?186:queries==3?194:queries==4?0:900));
                playing=id==900||(id==194&&musicPlaying);return true;
            }
            bool stop(int){std::abort();}
            bool play(int id,int priority,bool loop,int& voice){assert(id==194&&priority==100&&loop);++plays;voice=900;return true;}
            bool getGain(int id,MotionBinary64& gain){assert(id==900);gain=MotionBinary64::fromInteger(1);return true;}
            bool setGain(int id,MotionBinary64 gain,int time){assert((id==194||id==900)&&gain.high==0x3ff00000u&&!gain.low&&!time);return true;}
            bool setPitch(int id,MotionBinary64 pitch){assert(id==900&&pitch.high==0x3ff00000u&&!pitch.low);return true;}
        } audio{queries,musicPlaying};
        auto death=[&](SkeletonActor& actor){
            if(native)return owner->recordSkeletonDeath(actor,true,stop,
                [&](unsigned& id){++allocated;id=titleId;return true;},text,environment,audio);
            return stored->recordSkeletonDeath(actor,true,stop,titles,text,environment,audio);
        };
        const bool okay=native?owner->skeletons().dispatch(skeleton,[&](auto& actor,auto&,bool&){return death(actor);}):
            stored->withMomentDelayRoomSpeed(speed,[&]{return skeletons.dispatch(skeleton,[&](auto& actor,auto&,bool&){return death(actor);});});
        assert(okay==(stopped||!badFont)&&speed==30&&queries==(stopped||badFont?1u:musicPlaying?3u:5u));
        assert(audio.plays==unsigned(!stopped&&!badFont&&!musicPlaying));
        const auto* actor=native?&owner->skeletons().find(skeleton)->state:&skeletons.instances().find(skeleton)->state;
        if(audio.plays)assert(actor->sound.sfxIndex==194&&actor->sound.sndsnd==900&&actor->sound.chanNum==5);
        assert(native?!world->noKillAchievement():!stored->noKillAchievement());
        assert((native?world->enemiesDefeated():stored->mainEnemiesDefeated())==1);
        assert((native?world->firstLand():stored->firstLand())==(!stopped&&!badFont));
        assert((native?world->backgroundMusicId():stored->backgroundMusicId())==(!stopped&&!badFont?194:193));
        if(native){
            const auto* control=owner->runControllers().find(controller);
            assert(control&&control->controllerSlow==30&&control->alarms.get(0)==1&&allocated==unsigned(!stopped));
            const auto* child=owner->levelNumbers().find(owner->levelNumbers().sourceHandle(titleId));
            assert(bool(child)==!stopped);
            if(child)assert(child->x.bits==0x43700000u&&child->y.bits==0x44300000u);
        }else{
            const auto* control=stored->instance(StartupRuntime::controllerInstanceId);
            assert(control&&control->controllerSlow==30&&control->alarms.get(0)==1);
            const auto* child=stored->instance(titleId);assert(bool(child)==!stopped);
            if(child)assert(child->x==240&&child->y==704);
        }
    }
    for(bool native:{false,true})for(bool badFont:{false,true}){
        using namespace downwell;
        auto storedOwner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto stored=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,nullptr,storedOwner->binding());
        assert(storedOwner->attach(*stored));stored->skipToGameForTesting();auto registration=stored->levelNumberRegistration<1>();
        auto nativeOwner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,nativeOwner->enemyBinding(),
            nativeOwner->breakableBinding(),nativeOwner->playerBinding(),&nativeOwner->terrain());
        assert(nativeOwner->attach(*world));
        const char* language="japanese";unsigned font=badFont?999:2;bool hard=true;
        auto firstLand=[&]{return native?world->firstLand():stored->firstLand();};
        auto musicId=[&]{return native?world->backgroundMusicId():stored->backgroundMusicId();};
        TimeStopEffects<4> stop;
        LevelNumberTextContext text{fonts,area_titles::entries,area_titles::count,language,font,hard};
        unsigned source=native?105187:stored->nextRuntimeInstanceId(),allocations=0,queries=0;
        auto title=[&]()->const LevelNumber<64>*{
            if(native){auto* r=nativeOwner->levelNumbers().find(nativeOwner->levelNumbers().sourceHandle(source));return r?&r->state:nullptr;}
            const auto* r=stored->instance(source);return r?registration.state(registration.handle(*r)):nullptr;
        };
        struct Audio {
            decltype(title)& child;decltype(firstLand)& first;unsigned& queries;
            bool isPlaying(int id,bool& playing){assert(first()&&child()&&child()->created);++queries;playing=id==194;return true;}
            bool stop(int){std::abort();}
            bool setGain(int,MotionBinary64,unsigned){std::abort();}
            bool play(int,int,bool,int&){std::abort();}
            bool getGain(int,MotionBinary64&){std::abort();}
            bool setPitch(int,MotionBinary64){std::abort();}
        } audio{title,firstLand,queries};
        SoundPlayState caller;SoundPlayEnvironment env;
        auto invoke=[&](bool stopped){
            stop.player.stopped=stopped;
            if(native)return world->beginLevelCueWithTitle(stop,{}, {},
                [&](unsigned& id){++allocations;id=source;return true;},text,caller,env,audio);
            return stored->beginLevelCueWithTitle(stop,{}, {},registration,text,caller,env,audio);
        };
        assert(musicId()==193&&invoke(true)&&!title()&&!firstLand()&&!queries&&!allocations);
        assert(invoke(false)==!badFont&&title()&&firstLand()==!badFont&&queries==(badFont?0u:2u));
        assert(musicId()==(badFont?193:194));
        assert(native?allocations==1:stored->nextRuntimeInstanceId()==source+1);
        if(badFont){assert(!title()->created);continue;}
        LevelNumber<64> expected;
        assert(text.initialize(expected,native?world->runStage():stored->runStage())&&title()->length==expected.length&&
            title()->width.high==expected.width.high&&title()->width.low==expected.width.low);
        for(unsigned i=1;i<=expected.length;++i)assert(title()->characters[i].token==expected.characters[i].token);
        assert(invoke(false)&&queries==2); // No second child/audio after firstLand.
        if(native){
            assert(world->restartFromResults(DeathMenuSequence::Action::Retry,2,300)==ResultsDestination::gameplay);
            assert(!world->firstLand()&&world->backgroundMusicId()==194);
        }else{
            assert(registration.clear());
            assert(stored->restartFromResults(DeathMenuSequence::Action::Retry,2));
            assert(!stored->firstLand()&&stored->backgroundMusicId()==194);
        }
    }
    for(bool incomplete:{false,true}){
        auto owner=std::make_unique<downwell::StoredWorldCollision>(objects,sprites,masks);
        auto stored=std::make_unique<downwell::StartupRuntime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,
            2,nullptr,owner->binding());
        assert(owner->attach(*stored));stored->skipToGameForTesting();
        auto numbers=stored->levelNumberRegistration<1>();
        const auto id=stored->nextRuntimeInstanceId();downwell::InstanceHandle key;
        const bool created=numbers.create(downwell::MotionBinary64::fromInteger(241),downwell::MotionBinary64::fromInteger(603),
            [&](auto live,auto& state){
                assert(live.generation&&stored->instance(id)&&stored->instance(id)->objectId==372&&
                    stored->instance(id)->x==241&&stored->instance(id)->alarms.get(0)==-1);
                if(incomplete)return true; // A successful placeholder is not a completed Create.
                return state.create(3,downwell::MotionBinary64::fromInteger(24),[](unsigned i,unsigned& token){token=i;return true;});
            },key);
        assert(stored->nextRuntimeInstanceId()==id+1&&created==!incomplete);
        if(incomplete){assert(!stored->effectsSucceeded()&&!key.generation&&stored->instance(id)->alarms.get(0)==-1);continue;}
        assert(stored->instance(id)->alarms.get(0)==4);
        struct AbsentSurface {
            bool color(unsigned){std::abort();}
            bool exists(bool& present){present=false;return true;}
            bool begin(){std::abort();}bool end(){std::abort();}
            bool view(unsigned,downwell::MotionBinary64&){std::abort();}
            bool character(unsigned,downwell::MotionBinary64,downwell::MotionBinary64){std::abort();}
        } surface;
        for(unsigned tick=1;tick<=142;++tick){
            assert(stored->withLevelNumberEvents(numbers,[&]{stored->step({});return stored->effectsSucceeded();}));
            if(tick==4)assert(numbers.state(key)->shown==2&&stored->instance(id)->alarms.get(0)==4);
            if(tick==12)assert(stored->instance(id)->alarms.get(1)==119);
            if(tick==131)assert(numbers.state(key)->exit==1&&stored->instance(id)->alarms.get(2)==3);
            if(tick==1){
                unsigned draws=0;
                assert(!stored->drawLevelNumber(*stored->instance(id),surface));
                assert(stored->withLevelNumberDraw(numbers,[&]{
                    unsigned nested=0;
                    assert(!stored->withLevelNumberDraw(numbers,[&]{++nested;return true;})&&!nested);
                    return stored->visitUnconditionalSurfaceDraw([&](auto live,const auto& record,int depth){
                        if(record.objectId!=372)return true;
                        ++draws;return live.slot==key.slot&&live.generation==key.generation&&depth==-100000&&
                            stored->drawLevelNumber(record,surface);
                    });
                })&&draws==1);
                assert(!stored->drawLevelNumber(*stored->instance(id),surface));
                assert(!stored->withLevelNumberDraw(numbers,[]{return false;}));
                assert(!stored->drawLevelNumber(*stored->instance(id),surface));
            }
            if(tick==2){
                struct PresentSurface {
                    unsigned glyphs=0,views=0,colors=0,ends=0,colorValue=123;
                    bool exists(bool& value){value=true;return true;}bool begin(){return true;}
                    bool end(){++ends;return true;}
                    bool color(unsigned value){++colors;colorValue=value;return true;}
                    bool view(unsigned,downwell::MotionBinary64& value){++views;value={};return true;}
                    bool character(unsigned token,downwell::MotionBinary64,downwell::MotionBinary64){
                        assert(token==1&&colorValue==(glyphs==4?0xffffffu:0u));++glyphs;return true;
                    }
                } present;
                assert(stored->withLevelNumberDraw(numbers,[&]{
                    return stored->visitUnconditionalSurfaceDraw([&](auto,const auto& record,int){
                        return record.objectId!=372||stored->drawLevelNumber(record,present);
                    });
                }));
                assert(present.glyphs==5&&present.views==4&&present.colors==2&&present.ends==1&&present.colorValue==0xffffff);
            }
            if(tick<142)assert(numbers.state(key)->characters[1].offset==(tick<2?-2:-1)); // Only Draw moves letters.
        }
        assert(!stored->instance(id)&&!numbers.state(key)&&numbers.valid());
        downwell::InstanceHandle replacement;
        assert(numbers.create({}, {},[](auto,auto& state){return state.create(1,{},[](unsigned,unsigned& token){token=1;return true;});},replacement));
        assert((replacement.slot!=key.slot||replacement.generation!=key.generation)&&
            !numbers.state(key)&&numbers.clear()&&numbers.valid());
    }
    {
        auto owner=std::make_unique<downwell::StoredWorldCollision>(objects,sprites,masks);
        auto stored=std::make_unique<downwell::StartupRuntime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,
            2,nullptr,owner->binding());
        assert(owner->attach(*stored));stored->skipToGameForTesting();
        unsigned visited=0;int speed=60;
        assert(stored->visitDamageControllers([&](auto& controller){
            ++visited;speed=40;controller.setSlow(23);return controller.setAlarm(0,40);
        }));
        const auto* controller=stored->instance(downwell::StartupRuntime::controllerInstanceId);
        assert(visited==1&&speed==40&&controller&&controller->controllerSlow==23&&controller->alarms.get(0)==40);
        assert(!stored->visitDamageControllers([&](auto& controller){
            controller.setSlow(17);return controller.setAlarm(downwell::AlarmBank::count,40);
        })&&!stored->effectsSucceeded()&&controller->controllerSlow==17&&controller->alarms.get(0)==40);
    }
    for(unsigned binding=0;binding<3;++binding){
        auto owner=std::make_unique<downwell::StoredWorldCollision>(objects,sprites,masks);
        auto stored=std::make_unique<downwell::StartupRuntime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,
            2,nullptr,owner->binding());
        assert(owner->attach(*stored));
        stored->skipToGameForTesting();
        downwell::TimeStopEffects<4> state;
        auto shards=stored->timeShardRegistration<4>();auto changes=stored->timeChangeRegistration<4>();
        assert(state.shards.bindPhases(shards)&&state.changes.bind(changes));
        if(binding==1){
            int otherSpeed=60;unsigned entered=0;
            assert(!stored->withMomentDelayRoomSpeed(otherSpeed,[&]{
                return stored->stepWithTimeStopEffects(shards,changes,state,{},false,
                    downwell::StartupRuntime::NoTimeTransition{},[&]{++entered;return true;});
            })&&!entered&&otherSpeed==60&&state.roomSpeed==60);
            continue;
        }
        unsigned requests=0;
        const auto tick=[&]{return stored->stepWithTimeStopEffects(shards,changes,state,{},false,
            downwell::StartupRuntime::NoTimeTransition{},[&]{
                ++requests;
                return stored->requestMomentDelay()&&state.roomSpeed==30&&
                    state.changes.createAt({}, {},state.roomSpeed,[](unsigned ms){return ms==30;});
            });};
        if(binding==2){
            assert(stored->withMomentDelayRoomSpeed(state.roomSpeed,[&]{
                return tick()&&stored->requestMomentDelay();
            }));
        }else assert(tick());
        const auto changeId=state.changes.sourceId(1);
        const auto* controller=stored->instance(downwell::StartupRuntime::controllerInstanceId);
        assert(requests==1&&controller&&controller->controllerSlow==30&&controller->alarms.get(0)==1&&
            state.changes.size()==1&&changeId&&stored->instance(changeId));
        assert(stored->stepWithTimeStopEffects(shards,changes,state,{})&&state.roomSpeed==60&&
            controller->controllerSlow==0&&state.changes.size()==1);
        assert(stored->stepWithTimeStopEffects(shards,changes,state,{})&&state.roomSpeed==60&&
            state.changes.size()==0&&!stored->instance(changeId));
        assert(state.reset());
    }
    for(int speed:{30,50,60}){
        auto stored=std::make_unique<downwell::StartupRuntime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects);
        stored->skipToGameForTesting();
        const auto* controller=stored->instance(downwell::StartupRuntime::controllerInstanceId);
        assert(controller&&controller->objectId==37);
        const int original=speed;
        assert(stored->withMomentDelayRoomSpeed(speed,[&]{
            struct SilentAudio {
                bool isPlaying(int,bool&){assert(false);return false;}
                bool stop(int){assert(false);return false;}
            } audio;
            assert(stored->noKillAchievement());unsigned cues=0;
            assert(stored->recordEnemyDeath(false,audio,
                [&]{return stored->requestMomentDelay();},[&]{
                    assert(!stored->noKillAchievement()&&controller->controllerSlow==(original==60?30:0));++cues;return true;
                })&&cues==1&&stored->mainEnemiesDefeated()==1);
            assert(speed==(original==60?30:original)&&controller->controllerSlow==(original==60?30:0));
            if(original==60){
                assert(controller->alarms.get(0)==1);
                assert(stored->requestMomentDelay()&&controller->alarms.get(0)==1);
                stored->step({});
                assert(speed==60&&controller->controllerSlow==0&&controller->alarms.get(0)==0);
                speed=35;stored->step({});
                assert(speed==35&&controller->alarms.get(0)==-1);
            }
            return stored->effectsSucceeded();
        }));
    }
    for(unsigned failure=0;failure<=4;++failure){
        auto native=std::make_unique<GameplayWorld>(300,true,0,false);
        checkOwnedAchievement(*native,[&]{return native->enemiesDefeated();},failure);
        auto stored=std::make_unique<downwell::StartupRuntime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects);
        stored->skipToGameForTesting();
        checkOwnedAchievement(*stored,[&]{return stored->mainEnemiesDefeated();},failure);
        if(!failure){
            assert(native->restartFromResults(downwell::DeathMenuSequence::Action::Retry,2,300)==downwell::ResultsDestination::gameplay&&
                native->noKillAchievement());
            stored->reset(downwell::PlatformProfile::nonDesktop);assert(!stored->noKillAchievement());
            stored->skipToGameForTesting();assert(stored->noKillAchievement());
        }
    }
    for(unsigned failure=0;failure<=5;++failure){
        auto native=std::make_unique<GameplayWorld>(300,true,0,false);
        check(*native,[&]{return native->enemiesDefeated();},failure);
        auto stored=std::make_unique<downwell::StartupRuntime>();
        check(*stored,[&]{return stored->mainEnemiesDefeated();},failure);
    }
    std::puts("Enemy death: both-world counters and level-begin cue; five bookkeeping, thirteen normal cue and six ending cue failure prefixes passed");
}
