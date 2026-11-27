#ifndef DOWNWELL_TEST_BULLET_WALL_SOUND_CHECKS_H
#define DOWNWELL_TEST_BULLET_WALL_SOUND_CHECKS_H
#include "core/SoundPlayScript.h"
// Shared command recorder for scalar and real-world owner tests. No audible
// output is produced. Checks fields at the actual synchronous audio boundary.
struct BulletWallAudio {
    const downwell::SoundPlayState& caller;int previous,sound;bool water;
    unsigned fail=0,calls=0;bool valid=true;
    bool event(unsigned op){valid=valid&&op==++calls;return valid&&calls!=fail;}
    bool isPlaying(int id,bool& yes){
        const unsigned op=calls==0?1:7;yes=true;
        valid=valid&&id==(op==1?previous:900)&&caller.sfxIndex==sound&&caller.chanNum==2;
        return event(op);
    }
    bool stop(int id){valid=valid&&id==previous;return event(2);}
    bool play(int id,int priority,bool loop,int& voice){
        valid=valid&&id==sound&&priority==60&&!loop&&caller.cutPrv;voice=900;return event(3);
    }
    bool getGain(int id,downwell::AudioLevel& gain){
        valid=valid&&id==900;gain=downwell::audio_level::half;return event(4);
    }
    bool setGain(int id,downwell::AudioLevel gain,int duration){
        valid=valid&&id==900&&gain==downwell::AudioLevel::raw(8192)&&!duration;return event(5);
    }
    bool setPitch(int id,downwell::AudioLevel pitch){
        const auto expected=water?downwell::AudioLevel::raw(36045):downwell::audio_level::one;
        valid=valid&&id==900&&pitch==expected;return event(6);
    }
};
#endif
