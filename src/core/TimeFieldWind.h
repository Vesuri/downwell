#ifndef DOWNWELL_CORE_TIME_FIELD_WIND_H
#define DOWNWELL_CORE_TIME_FIELD_WIND_H
#include "MultiplyBinary64.h"
namespace downwell {
// goalTimeField Collision objPlayer_n and soundPlayOL's UI-channel path.
// voice is global.voidWind; sound and channel are self.windsnd and
// global.OLchannel[8]. Audio operations are synchronous and checked.
class TimeFieldWind {
public:
    int voice=-1;
    bool succeeded()const{return good_;}
    template<class Random,class Audio>
    bool contact(Random& random,std::int32_t& sound,int& channel,MotionBinary64 gameVolume,Audio& audio){
        if(!good_)return false;
        bool playing=false;
        if(!check(audio.isPlaying(voice,playing)))return false;
        if(playing)return true;
        sound=349+int(random.chooseIndexSmall(7));
        // UI (channel 8) forces cutPrv=0 and stops the selected resource,
        // not the previous channel entry. No offscreen/water/music gates.
        if(!check(audio.isPlaying(sound,playing)))return false;
        if(playing&&!check(audio.stop(sound)))return false;
        channel=sound;
        int started=-1;
        if(!check(audio.play(sound,90,false,started)))return false;
        MotionBinary64 gain,scaled;
        if(!check(audio.getGain(started,gain))||!check(multiplyBinary64(gain,gameVolume,scaled))||
            !check(audio.setGain(started,scaled,0))||
            !check(audio.setPitch(started,MotionBinary64::fromInteger(1)))||
            !check(audio.isPlaying(started,playing)))return false;
        voice=playing?started:-1;
        return check(audio.setGain(voice,{0x3fe00000u,0},0));
    }
private:
    bool good_=true;
    bool check(bool ok){if(!ok)good_=false;return ok;}
};
}
#endif
