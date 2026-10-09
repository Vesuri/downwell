#ifndef DOWNWELL_CORE_MUSIC_DUCK_H
#define DOWNWELL_CORE_MUSIC_DUCK_H
#include "MotionBinary64.h"
namespace downwell {
// bgmDuck caller fields. Read global.bgm again for every audio operation;
// synchronous callbacks can change both globals and this caller's fields.
struct MusicDuck {
    int duckDuration=0;
    MotionBinary64 duckAmt{};
    template<class Area,class Music,class Audio>
    bool execute(int duration,MotionBinary64 amount,Area area,Music music,Audio& audio){
        duckDuration=duration;duckAmt=amount;
        if(area()==5)return true;
        bool playing=false;if(!audio.isPlaying(music(),playing))return false;
        if(!playing)return true;
        MotionBinary64 gain;if(!audio.getGain(music(),gain))return false;
        if(gain.high!=0x3ff00000u||gain.low)return true;
        const auto duck=duckAmt;
        const auto firstMusic=music();
        if(!audio.setGain(firstMusic,duck,0))return false;
        const auto restoreDuration=duckDuration;
        const auto restoreMusic=music();
        return audio.setGain(restoreMusic,MotionBinary64::fromInteger(1),restoreDuration);
    }
};
}
#endif
