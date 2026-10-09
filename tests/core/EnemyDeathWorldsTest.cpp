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
int main(){
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
