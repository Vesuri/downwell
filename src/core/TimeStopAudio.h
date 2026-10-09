#ifndef DOWNWELL_CORE_TIME_STOP_AUDIO_H
#define DOWNWELL_CORE_TIME_STOP_AUDIO_H
#include "PlayerTimeStop.h"
#include "SoundPlayScript.h"
#include "MusicDuck.h"
namespace downwell {
// bgmDuck writes these fields on its caller. Voices remain owned by the
// session/backend, not by the room-local transition or an individual field.
struct TimeStopAudio : MusicDuck {
    template<class Audio>
    static bool query(PlayerTimeStop::AudioQuery query,int music,int wind,Audio& audio,bool& result){
        switch(query){
        case PlayerTimeStop::AudioQuery::musicPlaying:return audio.isPlaying(music,result);
        case PlayerTimeStop::AudioQuery::musicPaused:return audio.isPaused(music,result);
        case PlayerTimeStop::AudioQuery::windPlaying:return audio.isPlaying(wind,result);
        }
        return false;
    }
    static bool handles(PlayerTimeStop::Effect effect){
        using E=PlayerTimeStop::Effect;
        return effect==E::pauseMusic||effect==E::resumeMusic||effect==E::stopWind||
            effect==E::enterSound||effect==E::exitSound||effect==E::duckMusic;
    }
    template<class Audio>
    bool emit(PlayerTimeStop::Effect effect,SoundPlayState& caller,SoundPlayEnvironment& env,
              int area,int music,int wind,Audio& audio){
        using E=PlayerTimeStop::Effect;
        switch(effect){
        case E::pauseMusic:return audio.pause(music);
        case E::resumeMusic:return audio.resume(music);
        case E::stopWind:return audio.stop(wind);
        case E::enterSound:case E::exitSound:{
            int result=-1;bool good=true;
            return SoundPlayScript::plain(caller,env,audio,effect==E::enterSound?184:185,
                80,false,true,result,good);
        }
        case E::duckMusic:{
            return execute(200,{0x3fe66666u,0x66666666u},[&]{return area;},[&]{return music;},audio);
        }
        default:return false;
        }
    }
};
}
#endif
