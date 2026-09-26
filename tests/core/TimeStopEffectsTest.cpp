#include "core/TimeStopEffects.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
int main(){
    using Effect=PlayerTimeStop::Effect;
    for(unsigned seed=0;seed<256;++seed){
        TimeStopEffects<16> state;GameMakerRandom rng(seed);unsigned lookups=0;
        TimeStopEffects<16>::PlayerPose pose{{0x43700000},{0x43300000},MotionBinary64::fromInteger(1),MotionBinary64::fromInteger(-1)};
        bool inField=true;
        auto field=[&](bool& hit){hit=inField;return true;};
        auto lookup=[&](auto x,auto y,auto& id,auto& fx,auto& fy){
            ++lookups;assert(x.bits==pose.x.bits && y.bits==pose.y.bits);
            id=inField?123:-4;fx=MotionBinary64::fromInteger(220);fy=MotionBinary64::fromInteger(200);return true;
        };
        std::vector<Effect> events;
        auto emit=[&](Effect effect){
            events.push_back(effect);
            if(effect==Effect::shake){assert(state.shards.size()==3 && rng.drawCount()==24);}
            if(effect==Effect::change){assert(state.shards.size()==3 || state.shards.size()==9);assert(state.roomSpeed==30 && state.changes.size()>0);}
            if(effect==Effect::enterSound){assert(state.shards.size()==6 && rng.drawCount()==48);}
            if(effect==Effect::exitSound){assert(state.shards.size()==12 && rng.drawCount()==96);}
            return true;
        };
        auto audio=[](auto){return true;};
        assert(state.beginStep(true,false,pose,false,rng,field,lookup,audio,emit));
        assert(state.player.stopped && !state.player.bgmOn && lookups==4);
        assert((events==std::vector<Effect>{Effect::shake,Effect::change,Effect::pauseMusic,Effect::enterSound,Effect::oxygenFloor}));
        events.clear();
        assert(state.beginStep(true,false,pose,false,rng,field,lookup,audio,emit));
        assert(rng.drawCount()==48 && lookups==4 && (events==std::vector<Effect>{Effect::oxygenFloor}));
        inField=false;events.clear();
        assert(state.beginStep(true,false,pose,false,rng,field,lookup,audio,emit));
        assert(!state.player.stopped && state.player.bgmOn && lookups==6);
        assert((events==std::vector<Effect>{Effect::change,Effect::resumeMusic,Effect::stopWind,Effect::exitSound,Effect::duckMusic}));
        assert(state.changes.size()==2);
        for(unsigned tick=0;tick<8;++tick){const auto phase=state.changes.beginPhase();
            state.changes.advanceAlarm(phase,0,state.roomSpeed);state.changes.advanceAlarm(phase,1,state.roomSpeed);}
        assert(state.changes.size()==0 && state.roomSpeed==60);
        const auto stale=state.shards.beginPhase();state.reset();assert(state.shards.size()==0 && !state.player.stopped);
        assert(state.shards.create({}, {},false,rng)>stale.newest);
    }
    TimeStopEffects<2> small;GameMakerRandom rng(9);TimeStopEffects<2>::PlayerPose pose{};
    auto field=[](bool& hit){hit=true;return true;};
    auto lookup=[](auto,auto,auto& id,auto& x,auto& y){id=1;x={};y={};return true;};
    unsigned events=0;
    auto emit=[&](auto){++events;return true;};
    assert(!small.beginStep(true,false,pose,false,rng,field,lookup,[](auto){return false;},emit));
    assert(!small.succeeded && small.shards.size()==2 && rng.drawCount()==18 && events==0);
    assert(!small.beginStep(true,false,pose,false,rng,field,lookup,[](auto){return false;},emit));
    assert(rng.drawCount()==18);small.reset();assert(small.succeeded);
    std::puts("Time-stop coordination: 256 enter/hold/leave sequences, synchronous shard/RNG/audio order, reset and latched failure passed");
}
