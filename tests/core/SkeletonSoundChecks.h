#ifndef DOWNWELL_TEST_SKELETON_SOUND_CHECKS_H
#define DOWNWELL_TEST_SKELETON_SOUND_CHECKS_H
#include "core/RunSession.h"
#include <cstdlib>
#include <initializer_list>
struct SkeletonSoundAudio {
    unsigned calls=0,failAt=0;
    int ids[8]{},events[8]{};
    bool wet=false;
    static void check(bool ok){if(!ok)std::abort();}
    bool call(int event,int id){check(calls<8);events[calls]=event;ids[calls++]=id;return calls!=failAt;}
    bool isPlaying(int id,bool& playing){playing=true;return call(1,id);}
    bool stop(int id){return call(2,id);}
    bool play(int id,int priority,bool loop,int& voice){check(priority==50&&!loop);voice=1001;return call(3,id);}
    bool getGain(int voice,downwell::MotionBinary64& gain){gain=downwell::MotionBinary64::fromInteger(1);return call(4,voice);}
    bool setGain(int voice,downwell::MotionBinary64 gain,int time){
        check(gain.high==0x3fd00000u&&!gain.low&&!time);return call(5,voice);
    }
    bool setPitch(int voice,downwell::MotionBinary64 pitch){
        check(pitch.high==(wet?0x3fe19999u:0x3ff00000u)&&pitch.low==(wet?0x9999999au:0));return call(6,voice);
    }
};
inline void checkSkeletonSoundContract(){
    using namespace downwell;
    for(int sound=132;sound<=134;++sound)for(int y:{-1,0,100,101})for(bool water:{false,true})
    for(unsigned failure=0;failure<=7;++failure){
        RunSession run;SoundPlayState caller;SoundPlayEnvironment env;
        env.y=MotionBinary64::fromInteger(y);env.viewHeight=MotionBinary64::fromInteger(100);
        env.gameVolume={0x3fd00000u,0};env.inWater=water;run.overlapSounds[4]=777;
        SkeletonSoundAudio audio;audio.failAt=failure;audio.wet=water;
        const bool visible=y>=0&&y<=100;const unsigned maximum=visible?7:2;
        const bool accepted=!failure||failure>maximum;
        SkeletonSoundAudio::check(run.playSkeletonSound(caller,sound,env,audio)==accepted&&run.effectsSucceeded==accepted&&
            audio.calls==(accepted?maximum:failure)&&caller.sfxIndex==sound&&caller.soundPriority==50&&!caller.loop&&
            !caller.cutPrv&&caller.chanNum==4&&caller.doNotPlay==!visible&&audio.ids[0]==sound);
        if(audio.calls>=2)SkeletonSoundAudio::check(audio.events[1]==2&&audio.ids[1]==sound);
        SkeletonSoundAudio::check(run.overlapSounds[4]==(visible&&(accepted||failure>=3)?sound:777));
        if(visible&&accepted)SkeletonSoundAudio::check(caller.sndsnd==1001&&caller.sndgain.high==0x3fd00000u&&
            !caller.sndgain.low&&audio.events[6]==1&&audio.ids[6]==1001);
        if(!accepted){const auto before=audio.calls;
            SkeletonSoundAudio::check(!run.playSkeletonSound(caller,sound,env,audio)&&audio.calls==before);}
    }
}
#endif
