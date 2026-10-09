#include "core/TimeFieldWind.h"
#include "core/GameMakerRandom.h"
#include "core/RunSession.h"
#include <cassert>
#include <vector>
#include <cstdio>
using namespace downwell;
struct Audio {
    std::vector<int> calls;
    bool wind=false,resource=true,started=true,scaled=false;
    unsigned fail=0;
    bool call(int n){calls.push_back(n);return calls.size()!=fail;}
    bool isPlaying(int id,bool& result){result=id==-1?false:id==777?wind:id==900?started:resource;return call(id);}
    bool stop(int id){assert(id>=349&&id<=355);return call(1000+id);}
    bool play(int id,int priority,bool loop,int& voice){assert(id>=349&&id<=355&&priority==90&&!loop);voice=900;return call(2000+id);}
    bool getGain(int voice,MotionBinary64& gain){assert(voice==900);gain=MotionBinary64::fromInteger(1);return call(3000);}
    bool setGain(int voice,MotionBinary64 gain,int duration){assert(duration==0);
        if(!scaled){scaled=true;assert(voice==900&&gain.high==0x3fd00000u&&gain.low==0);return call(4000);}
        assert((voice==900||voice==-1)&&gain.high==0x3fe00000u&&gain.low==0);return call(6000);}
    bool setPitch(int voice,MotionBinary64 pitch){assert(voice==900&&pitch.high==0x3ff00000u&&pitch.low==0);return call(5000);}
};
int main(){
    {RunSession run;run.fieldWind.voice=900;run.overlapSounds[8]=350;
        run.reset();assert(run.fieldWind.voice==-1&&run.fieldWind.succeeded()&&run.overlapSounds[8]==0);}
    for(unsigned seed=0;seed<128;++seed){
        GameMakerRandom random(seed),oracle(seed);const int selected=349+int(oracle.nextU32()%7);
        TimeFieldWind wind;SoundPlayState caller;Audio audio;int sound=-7,channel=22;
        assert(wind.contact(random,sound,caller,channel,{0x3fd00000u,0},audio));
        assert(sound==selected&&channel==selected&&wind.voice==900&&random.drawCount()==1);
        assert(caller.sfxIndex==selected&&caller.sndsnd==900&&caller.chanNum==8&&wind.soundState.sndsnd==-1);
        assert((audio.calls==std::vector<int>{-1,selected,1000+selected,2000+selected,3000,4000,5000,900,6000}));
        SoundPlayState other;int otherSound=-7;
        assert(wind.contact(random,otherSound,other,channel,{},audio)&&random.drawCount()==1&&audio.calls.size()==10&&audio.calls.back()==900&&
            otherSound==-7&&other.sndsnd==-1&&other.sfxIndex==0&&caller.sfxIndex==selected);
    }
    for(unsigned failure=1;failure<=9;++failure){
        TimeFieldWind wind;SoundPlayState caller;GameMakerRandom random(2);Audio audio;audio.fail=failure;
        int sound=-7,channel=22;
        assert(!wind.contact(random,sound,caller,channel,{0x3fd00000u,0},audio)&&!wind.succeeded());
        assert(audio.calls.size()==failure&&random.drawCount()==unsigned(failure>1));
        assert((sound==-7)==(failure==1));assert((channel==22)==(failure<=3));
        assert(wind.voice==(failure==9?900:-1));
        assert(caller.sndsnd==(failure>4?900:-1)&&wind.soundState.sndsnd==-1);
        assert(!wind.contact(random,sound,caller,channel,{0x3fd00000u,0},audio)&&audio.calls.size()==failure);
    }
    {TimeFieldWind wind;wind.voice=777;GameMakerRandom random(2);Audio audio;audio.wind=true;int sound=12,channel=13;
        assert(wind.contact(random,sound,channel,{},audio)&&sound==12&&channel==13&&random.drawCount()==0&&audio.calls.size()==1);}
    {TimeFieldWind wind;GameMakerRandom random(2);Audio audio;audio.started=false;int sound=0,channel=0;
        assert(wind.contact(random,sound,channel,{0x3fd00000u,0},audio)&&wind.voice==-1&&audio.calls.back()==6000);}
    {TimeFieldWind wind;GameMakerRandom random(2);Audio audio;audio.resource=false;int sound=0,channel=0;
        assert(wind.contact(random,sound,channel,{0x3fd00000u,0},audio)&&audio.calls.size()==8&&audio.calls[2]==2000+sound);}
    std::puts("Time-field wind: source UI audio order, 128 choices, guarded RNG and every failure prefix passed");
}
