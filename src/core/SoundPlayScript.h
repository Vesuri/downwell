#ifndef DOWNWELL_CORE_SOUND_PLAY_SCRIPT_H
#define DOWNWELL_CORE_SOUND_PLAY_SCRIPT_H
#include "AudioLevel.h"
#include "FixedPoint.h"
namespace downwell {
// Caller-visible script fields, not backend voice state. Preserve each write
// before the next synchronous audio operation; failures do not roll it back.
struct SoundPlayState {
    int sfxIndex=0,soundPriority=0,sndsnd=-1,chanNum=0;
    bool loop=false,cutPrv=false,doNotPlay=false;
    const char* OLnum=nullptr;
    AudioLevel sndgain{};
};
struct SoundPlayEnvironment {
    AudioLevel gameVolume=audio_level::one;
    // The caller's y and the room view, for the offscreen test (W3.20: Q16;
    // every caller supplies whole pixels or a Q8/Q12 view top).
    Fixed16 y{},viewY{},viewHeight{};
    bool inWater=false,noBgm=false;
};
struct SoundPlayScript {
    static int channel(const char* name){
        const char* names[]={"footsteps","gunshot","gunwall","merchant","enemymove",
            "music","waterEnemy","waterThings","UI","boss"};
        if(name)for(int i=0;i<10;++i){const char* a=name;const char* b=names[i];
            while(*a&&*a==*b){++a;++b;}if(*a==*b)return i;}
        return 0; // The source switch has no default override.
    }
    template<class Audio>
    static bool plain(SoundPlayState& s,SoundPlayEnvironment& env,Audio& audio,
        int sound,int priority,bool loop,bool cut,int& result,bool& good){
        if(!good)return false;
        s.sfxIndex=sound;s.soundPriority=priority;s.loop=loop;s.cutPrv=cut;
        if(s.cutPrv&&!stopPlaying(s.sfxIndex,audio,good))return false;
        if(!start(s,env,audio,good))return false;
        if(!check(audio.setPitch(s.sndsnd,pitch(env.inWater)),good))return false;
        result=s.sndsnd;return true;
    }
    // chan is the global OLchannel entry selected by channel(name). Duck is
    // bgmDuck(200,global.pBulSp3), needed only by the gunshot branch. A suppressed
    // offscreen call exits without returning a voice (hasResult=false).
    template<class Audio,class Duck>
    static bool overlap(SoundPlayState& s,SoundPlayEnvironment& env,Audio& audio,Duck duck,
        int& chan,int sound,int priority,bool loop,bool cut,const char* name,
        int& result,bool& hasResult,bool& good){
        if(!good)return false;
        s.sfxIndex=sound;s.soundPriority=priority;s.loop=loop;s.cutPrv=cut;s.OLnum=name;s.chanNum=0;
        const int selected=channel(name);
        if(selected==1&&!check(duck(),good))return false;
        s.chanNum=selected;s.doNotPlay=false;hasResult=false;
        if(selected==4||selected==6){
            constexpr std::int32_t range=std::int32_t(1)<<30;
            if(!check(env.viewY.v>-range&&env.viewY.v<range&&env.viewHeight.v>=0&&env.viewHeight.v<range,good))
                return false;
            const std::int32_t bottom=env.viewY.v+env.viewHeight.v;
            s.doNotPlay=bottom<env.y.v||env.y.v<env.viewY.v;
            s.cutPrv=false;
            if(!stopPlaying(s.sfxIndex,audio,good))return false;
        }
        if(selected>=7){s.cutPrv=false;if(!stopPlaying(s.sfxIndex,audio,good))return false;}
        if(s.doNotPlay)return true;
        if(s.cutPrv&&!stopPlaying(chan,audio,good))return false;
        chan=s.sfxIndex;
        if(!start(s,env,audio,good)||
            !check(audio.setPitch(s.sndsnd,pitch(env.inWater&&(s.chanNum<5||s.chanNum>=9))),good))return false;
        if(s.chanNum==5&&env.noBgm&&!check(audio.stop(s.sfxIndex),good))return false;
        bool playing=false;if(!check(audio.isPlaying(s.sndsnd,playing),good))return false;
        result=playing?s.sndsnd:-1;hasResult=true;return true;
    }
private:
    static bool check(bool ok,bool& good){if(!ok)good=false;return ok;}
    // 0.55 under water.
    static AudioLevel pitch(bool water){return water?AudioLevel::raw(36045):audio_level::one;}
    template<class Audio> static bool stopPlaying(int id,Audio& audio,bool& good){
        bool playing=false;
        return check(audio.isPlaying(id,playing),good)&&(!playing||check(audio.stop(id),good));
    }
    template<class Audio> static bool start(SoundPlayState& s,SoundPlayEnvironment& env,Audio& audio,bool& good){
        int voice=-1;AudioLevel gain;
        if(!check(audio.play(s.sfxIndex,s.soundPriority,s.loop,voice),good))return false;
        s.sndsnd=voice;
        if(!check(audio.getGain(s.sndsnd,gain),good))return false;
        s.sndgain=gain;
        s.sndgain=audio_level::product(s.sndgain,env.gameVolume);
        return check(audio.setGain(s.sndsnd,s.sndgain,0),good);
    }
};
}
#endif
