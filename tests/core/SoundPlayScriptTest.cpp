#include "core/SoundPlayScript.h"
#include "core/SoundPlayVerification.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
struct Audio {
    std::vector<int> calls;
    unsigned fail=0;
    bool resourcePlaying=true,voicePlaying=true;
    AudioLevel lastGain{},lastPitch{};
    bool call(int op){calls.push_back(op);return calls.size()!=fail;}
    bool isPlaying(int id,bool& yes){yes=id==900?voicePlaying:resourcePlaying;return call(id);}
    bool stop(int id){return call(1000+id);}
    bool play(int id,int priority,bool loop,int& voice){assert(id==42&&priority==50&&!loop);voice=900;return call(2000+id);}
    bool getGain(int id,AudioLevel& gain){assert(id==900);gain=audio_level::half;return call(3000);}
    bool setGain(int id,AudioLevel gain,int duration){assert(id==900&&duration==0);lastGain=gain;return call(4000);}
    bool setPitch(int id,AudioLevel value){assert(id==900);lastPitch=value;return call(5000);}
};
int main(){
    assert(SoundPlayVerification::run());
    const char* names[]={"footsteps","gunshot","gunwall","merchant","enemymove",
        "music","waterEnemy","waterThings","UI","boss","unknown"};
    unsigned cases=0,prefixes=0;
    for(unsigned c=0;c<11;++c)for(bool water:{false,true})for(bool muted:{false,true})
    for(bool cut:{false,true})for(bool playing:{false,true})for(int y:{9,10,30,31}){
        const int ch=c==10?0:c;assert(SoundPlayScript::channel(names[c])==ch);
        SoundPlayState s;SoundPlayEnvironment env;env.inWater=water;env.noBgm=muted;
        env.gameVolume=AudioLevel::raw(16384);env.y=MotionBinary64::fromInteger(y);
        env.viewY=MotionBinary64::fromInteger(10);env.viewHeight=MotionBinary64::fromInteger(20);
        Audio audio;audio.resourcePlaying=playing;bool good=true,returned=true;int channel=73,result=88;
        assert(SoundPlayScript::overlap(s,env,audio,[&]{return audio.call(6000);},channel,
            42,50,false,cut,names[c],result,returned,good));
        std::vector<int> expected;if(ch==1)expected.push_back(6000);
        const bool spatial=ch==4||ch==6,blocked=spatial&&(y<10||y>30);
        if(spatial||ch>=7){expected.push_back(42);if(playing)expected.push_back(1042);}
        if(!blocked){
            if(!spatial&&ch<7&&cut){expected.push_back(73);if(playing)expected.push_back(1073);}
            for(int op:{2042,3000,4000,5000})expected.push_back(op);
            if(ch==5&&muted)expected.push_back(1042);
            expected.push_back(900);
            assert(channel==42&&returned&&result==900);
            assert(s.sndgain==AudioLevel::raw(8192));
            assert(audio.lastGain==AudioLevel::raw(8192));
            const bool lowPitch=water&&(ch<5||ch>=9);
            assert(audio.lastPitch==(lowPitch?AudioLevel::raw(36045):audio_level::one));
        }else assert(channel==73&&!returned&&result==88&&s.sndsnd==-1);
        assert(s.doNotPlay==blocked&&s.chanNum==ch&&s.cutPrv==(cut&&!spatial&&ch<7));
        assert(s.sfxIndex==42&&s.soundPriority==50&&!s.loop&&s.OLnum==names[c]);
        assert(audio.calls==expected&&good);++cases;
        for(unsigned failure=1;failure<=expected.size();++failure){
            SoundPlayState failed;Audio a;a.fail=failure;a.resourcePlaying=playing;
            bool valid=true,has=false;int slot=73,out=88;
            assert(!SoundPlayScript::overlap(failed,env,a,[&]{return a.call(6000);},slot,
                42,50,false,cut,names[c],out,has,valid)&&!valid);
            assert(a.calls==std::vector<int>(expected.begin(),expected.begin()+failure));
            // Channel publication precedes play, but follows every stop/guard.
            unsigned playAt=0;for(unsigned i=0;i<expected.size();++i)if(expected[i]==2042)playAt=i+1;
            assert(slot==((playAt&&failure>=playAt)?42:73));
            assert(failed.sndsnd==((playAt&&failure>playAt)?900:-1));
            assert(!SoundPlayScript::overlap(failed,env,a,[]{return true;},slot,
                42,50,false,cut,names[c],out,has,valid)&&a.calls.size()==failure);++prefixes;
        }
    }
    for(bool water:{false,true})for(bool cut:{false,true})for(bool playing:{false,true}){
        SoundPlayState s;SoundPlayEnvironment env;env.inWater=water;Audio a;a.resourcePlaying=playing;
        bool good=true;int result=-1;
        assert(SoundPlayScript::plain(s,env,a,42,50,false,cut,result,good)&&result==900);
        std::vector<int> expected;if(cut){expected.push_back(42);if(playing)expected.push_back(1042);}
        for(int op:{2042,3000,4000,5000})expected.push_back(op);assert(a.calls==expected);
        assert(a.lastPitch==(water?AudioLevel::raw(36045):audio_level::one));
        for(unsigned failure=1;failure<=expected.size();++failure){
            SoundPlayState f;Audio b;b.resourcePlaying=playing;b.fail=failure;bool ok=true;int out=88;
            assert(!SoundPlayScript::plain(f,env,b,42,50,false,cut,out,ok)&&!ok&&out==88);
            assert(b.calls==std::vector<int>(expected.begin(),expected.begin()+failure));
        }
    }
    {SoundPlayState s;SoundPlayEnvironment env;Audio a;a.voicePlaying=false;
        bool good=true,has=false;int slot=73,result=88;
        assert(SoundPlayScript::overlap(s,env,a,[]{return true;},slot,42,50,false,true,
            "waterThings",result,has,good)&&has&&result==-1&&s.sndsnd==900);}
    std::printf("Sound scripts: %u channel/gate cases and %u overlap failure prefixes passed\n",cases,prefixes);
}
