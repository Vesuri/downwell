#include "core/GameplayWorld.h"
#include "core/StartupRuntime.h"
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
int main(){
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
            bool noKill=true;unsigned cues=0;
            assert(stored->recordEnemyDeath(false,audio,[&]{noKill=false;return true;},
                [&]{return stored->requestMomentDelay();},[&]{
                    assert(!noKill&&controller->controllerSlow==(original==60?30:0));++cues;return true;
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
    for(unsigned failure=0;failure<=5;++failure){
        auto native=std::make_unique<GameplayWorld>(300,true,0,false);
        check(*native,[&]{return native->enemiesDefeated();},failure);
        auto stored=std::make_unique<downwell::StartupRuntime>();
        check(*stored,[&]{return stored->mainEnemiesDefeated();},failure);
    }
    std::puts("Enemy death: both worlds publish real counters in source order and retain five failure prefixes");
}
