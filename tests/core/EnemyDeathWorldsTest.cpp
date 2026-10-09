#include "core/GameplayWorld.h"
#include "core/StartupRuntime.h"
#include <cassert>
#include <memory>
#include <cstdio>
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
    for(unsigned failure=0;failure<=5;++failure){
        auto native=std::make_unique<GameplayWorld>(300,true,0,false);
        check(*native,[&]{return native->enemiesDefeated();},failure);
        auto stored=std::make_unique<downwell::StartupRuntime>();
        check(*stored,[&]{return stored->mainEnemiesDefeated();},failure);
    }
    std::puts("Enemy death: both worlds publish real counters in source order and retain five failure prefixes");
}
