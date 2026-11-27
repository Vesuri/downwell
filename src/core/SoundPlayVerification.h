#ifndef DOWNWELL_CORE_SOUND_PLAY_VERIFICATION_H
#define DOWNWELL_CORE_SOUND_PLAY_VERIFICATION_H
#include "SoundPlayScript.h"
namespace downwell {
// Bounded, allocation-free debugger tape for the box's waterThings route.
// This verifies script execution on 68000, not audible backend output.
struct SoundPlayVerification {
    struct Audio {
        unsigned calls=0,fail=0;
        int* channel=nullptr;
        bool valid=true;
        bool call(int op){
            const int expected[]={42,1042,2042,3000,4000,5000,900};
            if(calls>=7||expected[calls]!=op)valid=false;
            ++calls;return valid&&calls!=fail;
        }
        bool isPlaying(int id,bool& yes){yes=true;return call(id);}
        bool stop(int id){return call(1000+id);}
        bool play(int id,int priority,bool loop,int& voice){
            valid=valid&&*channel==42&&priority==50&&!loop;voice=900;return call(2000+id);
        }
        bool getGain(int id,AudioLevel& gain){valid=valid&&id==900;gain=audio_level::half;return call(3000);}
        bool setGain(int id,AudioLevel gain,int duration){
            valid=valid&&id==900&&duration==0&&gain==AudioLevel::raw(8192);return call(4000);
        }
        bool setPitch(int id,AudioLevel pitch){
            valid=valid&&id==900&&pitch==audio_level::one;return call(5000);
        }
    };
    static bool run(){
        for(unsigned failure=0;failure<=7;++failure){
            SoundPlayState s;SoundPlayEnvironment env;env.inWater=true;env.gameVolume=AudioLevel::raw(16384);
            int channel=73,result=88;bool good=true,returned=false;Audio audio;
            audio.channel=&channel;audio.fail=failure;
            const bool ok=SoundPlayScript::overlap(s,env,audio,[]{return false;},channel,
                42,50,false,true,"waterThings",result,returned,good);
            if(!audio.valid||audio.calls!=(failure?failure:7)||ok!=(failure==0)||good!=ok||
                s.chanNum!=7||s.cutPrv||s.doNotPlay)return false;
            if(!failure){if(!returned||result!=900||channel!=42)return false;}
            else if(returned||result!=88||channel!=(failure>=3?42:73)||
                s.sndsnd!=(failure>3?900:-1)||
                SoundPlayScript::overlap(s,env,audio,[]{return false;},channel,
                    42,50,false,true,"waterThings",result,returned,good)||audio.calls!=failure)return false;
        }
        return true;
    }
};
}
#endif
