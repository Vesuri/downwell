#include "core/TimeStopEffects.h"
#include "core/GameMakerRandom.h"
#include "core/RunSession.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
struct TransitionAudio {
    std::vector<int> calls;
    unsigned fail=0;
    bool playing=true,paused=true,wind=true,resource=true;
    AudioLevel gain=audio_level::one;
    bool call(int code){calls.push_back(code);return calls.size()!=fail;}
    bool isPlaying(int id,bool& value){value=id==700?playing:id==701?wind:resource;return call(id);}
    bool isPaused(int id,bool& value){assert(id==700);value=paused;return call(1700);}
    bool pause(int id){assert(id==700);return call(2700);}
    bool resume(int id){assert(id==700);return call(3700);}
    bool stop(int id){return call(4000+id);}
    bool play(int id,int priority,bool loop,int& voice){assert(priority==80&&!loop);voice=900;return call(5000+id);}
    bool getGain(int id,AudioLevel& value){value=id==700?gain:audio_level::one;return call(6000+id);}
    bool setGain(int id,AudioLevel value,int duration){
        if(id==900)assert(value==AudioLevel::raw(16384)&&!duration);
        else {assert(id==700);assert(duration==0||duration==200);
            assert(value==(duration?audio_level::one:AudioLevel::raw(45875)));}
        return call(7000+id+duration);
    }
    bool setPitch(int id,AudioLevel value){assert(id==900&&value==AudioLevel::raw(36045));return call(8900);}
};
static void checkedAudioTransitions(){
    using E=PlayerTimeStop::Effect;
    const std::vector<int> expected={700,2700,184,4184,5184,6900,7900,8900,
        1700,3700,701,4701,185,4185,5185,6900,7900,8900,700,6700,7700,7900};
    for(unsigned failure=0;failure<=expected.size();++failure){
        TimeStopEffects<16> state;GameMakerRandom random(2);TimeStopAudio audioState;
        SoundPlayState caller;SoundPlayEnvironment env;env.gameVolume=AudioLevel::raw(16384);env.inWater=true;
        TransitionAudio audio;audio.fail=failure;bool hit=true;
        TimeStopEffects<16>::PlayerPose pose{};
        auto field=[&](bool& result){result=hit;return true;};
        auto lookup=[&](auto,auto,auto& id,auto& x,auto& y){id=hit?123:-4;x={};y={};return true;};
        std::vector<E> effects;
        auto step=[&]{return state.beginStepWithAudio(true,[](bool& side){side=false;return true;},pose,false,
            random,field,lookup,audioState,caller,env,1,700,701,audio,[&](E e){effects.push_back(e);return true;});};
        bool ok=step();if(ok){hit=false;ok=step();}
        assert(ok==(failure==0));
        const unsigned count=failure?failure:expected.size();
        assert(audio.calls==std::vector<int>(expected.begin(),expected.begin()+count));
        if(failure){
            assert(!state.succeeded);const auto draws=random.drawCount();const auto emitted=effects.size();
            assert(!step()&&random.drawCount()==draws&&effects.size()==emitted&&audio.calls.size()==count);
            // Query failure cannot fall through to bgmOn or the second burst.
            if(failure==1)assert(state.player.bgmOn&&random.drawCount()==24&&state.shards.size()==3);
            if(failure==9||failure==11)assert(!state.player.bgmOn&&random.drawCount()==72&&state.shards.size()==9);
        }else {
            assert(state.player.bgmOn&&!state.player.stopped&&random.drawCount()==96);
            assert(caller.sfxIndex==185&&caller.soundPriority==80&&!caller.loop&&caller.cutPrv);
            assert(audioState.duckDuration==200&&audioState.duckAmt==AudioLevel::raw(45875));
            assert((effects==std::vector<E>{E::shake,E::change,E::oxygenFloor,E::change}));
        }
    }
    // Original bgmDuck always writes caller fields, even when guards suppress audio.
    for(unsigned guard=0;guard<3;++guard){
        TimeStopAudio state;SoundPlayState caller;SoundPlayEnvironment env;TransitionAudio audio;
        if(guard==1)audio.playing=false;
        if(guard==2)audio.gain=audio_level::half;
        assert(state.emit(E::duckMusic,caller,env,guard==0?5:1,700,701,audio));
        assert(state.duckDuration==200&&state.duckAmt==AudioLevel::raw(45875));
        assert(audio.calls.size()==guard);
    }
}
int main(){
    for(auto value:{Fixed16::fromInteger(-2),Fixed16{},
                   Fixed16::raw(10*65536-1),Fixed16::fromInteger(10),
                   Fixed16::raw(10*65536+1),Fixed16::fromInteger(100)}){
        RunSession run;assert(run.setOxygen(value)&&run.floorTimeStopOxygen());
        const auto expected=value.v<10*65536?Fixed16::fromInteger(10):value;
        assert(run.oxygen.v==expected.v);
        run.playerCreated();assert(run.oxygen.v==100*65536);
        assert(run.setOxygen(value));
        run.reset();assert(run.oxygen.v==100*65536);
    }
    checkedAudioTransitions();
    for(unsigned failure=0;failure<=4;++failure){
        RunSession run;MusicDuck caller;unsigned calls=0;run.backgroundMusicId=193;
        struct Audio {
            RunSession& run;MusicDuck& caller;unsigned& calls;unsigned failure;
            bool call(int id){assert(id==193+int(calls));++calls;++run.backgroundMusicId;return calls!=failure;}
            bool isPlaying(int id,bool& out){out=true;return call(id);}
            bool getGain(int id,AudioLevel& out){out=audio_level::one;return call(id);}
            bool setGain(int id,AudioLevel value,int milliseconds){
                if(calls==2){assert(milliseconds==0&&value==AudioLevel::raw(45875));caller.duckDuration=777;}
                else assert(calls==3&&milliseconds==777&&value==audio_level::one);
                return call(id);
            }
        } audio{run,caller,calls,failure};
        const auto amount=AudioLevel::raw(45875);
        assert(run.duckMusic(caller,500,amount,audio)==!failure&&run.effectsSucceeded==!failure);
        assert(calls==(failure?failure:4)&&caller.duckDuration==(calls>=3?777:500)&&caller.duckAmt==amount);
        if(failure){const auto previous=calls;assert(!run.duckMusic(caller,123,{},audio)&&calls==previous);}
    }
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
