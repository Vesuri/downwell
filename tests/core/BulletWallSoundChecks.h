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
    bool getGain(int id,downwell::MotionBinary64& gain){
        valid=valid&&id==900;gain={0x3fe00000u,0};return event(4);
    }
    bool setGain(int id,downwell::MotionBinary64 gain,int duration){
        valid=valid&&id==900&&gain.high==0x3fc00000u&&!gain.low&&!duration;return event(5);
    }
    bool setPitch(int id,downwell::MotionBinary64 pitch){
        const downwell::MotionBinary64 expected=water?downwell::MotionBinary64{0x3fe19999u,0x9999999au}:
            downwell::MotionBinary64::fromInteger(1);
        valid=valid&&id==900&&pitch.high==expected.high&&pitch.low==expected.low;return event(6);
    }
};
#endif
