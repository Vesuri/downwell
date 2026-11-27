#ifndef DOWNWELL_CORE_TIME_FIELD_WIND_H
#define DOWNWELL_CORE_TIME_FIELD_WIND_H
#include "SoundPlayScript.h"
namespace downwell {
// goalTimeField Collision objPlayer_n and soundPlayOL's UI-channel path.
// voice is global.voidWind; sound and channel are self.windsnd and
// global.OLchannel[8]. Audio operations are synchronous and checked.
class TimeFieldWind {
public:
    static constexpr int firstSound=349,soundCount=7;
    int voice=-1;
    SoundPlayState soundState{}; // Standalone reference caller; live fields supply their own.
    bool succeeded()const{return good_;}
    template<class Random,class Audio>
    bool contact(Random& random,std::int32_t& sound,int& channel,AudioLevel gameVolume,Audio& audio){
        return contact(random,sound,soundState,channel,gameVolume,audio);
    }
    template<class Random,class Audio>
    bool contact(Random& random,std::int32_t& sound,SoundPlayState& caller,int& channel,AudioLevel gameVolume,Audio& audio){
        if(!good_)return false;
        bool playing=false;
        if(!check(audio.isPlaying(voice,playing)))return false;
        if(playing)return true;
        sound=firstSound+int(random.chooseIndexSmall(soundCount));
        SoundPlayEnvironment env;env.gameVolume=gameVolume;
        int result=-1;bool returned=false;
        if(!SoundPlayScript::overlap(caller,env,audio,[]{return true;},channel,
            sound,90,false,true,"UI",result,returned,good_))return false;
        if(!check(returned))return false;
        voice=result;
        return check(audio.setGain(voice,audio_level::half,0));
    }
private:
    bool good_=true;
    bool check(bool ok){if(!ok)good_=false;return ok;}
};
}
#endif
