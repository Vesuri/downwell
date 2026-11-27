#ifndef DOWNWELL_CORE_MUSIC_DUCK_H
#define DOWNWELL_CORE_MUSIC_DUCK_H
#include "AudioLevel.h"
namespace downwell {
// bgmDuck caller fields. Read global.bgm again for every audio operation;
// synchronous callbacks can change both globals and this caller's fields.
struct MusicDuck {
    int duckDuration=0;
    AudioLevel duckAmt{};
    template<class Area,class Music,class Audio>
    bool execute(int duration,AudioLevel amount,Area area,Music music,Audio& audio){
        duckDuration=duration;duckAmt=amount;
        if(area()==5)return true;
        bool playing=false;if(!audio.isPlaying(music(),playing))return false;
        if(!playing)return true;
        AudioLevel gain;if(!audio.getGain(music(),gain))return false;
        if(!(gain==audio_level::one))return true;
        const auto duck=duckAmt;
        const auto firstMusic=music();
        if(!audio.setGain(firstMusic,duck,0))return false;
        const auto restoreDuration=duckDuration;
        const auto restoreMusic=music();
        return audio.setGain(restoreMusic,audio_level::one,restoreDuration);
    }
};
}
#endif
