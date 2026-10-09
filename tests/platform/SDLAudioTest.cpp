#include "platform/sdl/AudioSDL.h"
#include "core/TimeFieldWind.h"
#include "core/MusicDuck.h"
#include "core/TimeStopAudio.h"
#include <sndfile.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
using namespace downwell;
namespace downwell {
struct AudioSDLTestAccess {
    static void manual(AudioSDL& audio){SDL_PauseAudioDevice(audio.device_,1);}
    static double cursor(AudioSDL& audio,int id){
        AudioSDL::Lock lock(audio.device_);auto* voice=audio.find(id);assert(voice);return voice->cursor;
    }
    static float sample(AudioSDL& audio){
        AudioSDL::Lock lock(audio.device_);float frame[2]{};audio.mix(frame,1);
        assert(frame[0]==frame[1]);return frame[0];
    }
};
}
int main(){
    {
        RunnerAudioGain ramp;
        const auto one=MotionBinary64::fromInteger(1);
        assert(ramp.set({},0,100)&&ramp.set(one,500,100));
        ramp.update(250100);assert(ramp.current.bits==0x3f000000u);
        // Retarget from the last published value, not an implicit clock update.
        assert(ramp.set({},200,300100));
        ramp.update(400100);assert(ramp.current.bits==0x3e800000u);
        assert(ramp.set(one,0,400100)&&ramp.endMicros==0);
        ramp.update(600100);assert(ramp.current.bits==0x3f800000u);
        // At exactly the endpoint, SSE interpolation can round away the target.
        const MotionBinary64 tiny{0x3e600000u,0}; // 2^-25
        assert(ramp.set(tiny,1,100));ramp.update(1100);
        assert(ramp.current.bits==0&&ramp.endMicros==1100);
        ramp.update(1101);assert(ramp.current.bits==0x33000000u&&ramp.endMicros==0);
        assert(!ramp.set(one,-1,0)&&!ramp.set(one,2147484,0));
    }
    assert(SDL_Init(SDL_INIT_AUDIO)==0);
    char path[]="/tmp/downwell-audio-test-XXXXXX";
    const int fd=mkstemp(path);assert(fd>=0);close(fd);
    SF_INFO info{};info.samplerate=48000;info.channels=1;info.format=SF_FORMAT_WAV|SF_FORMAT_PCM_16;
    SNDFILE* file=sf_open(path,SFM_WRITE,&info);assert(file);
    float samples[4800];for(auto& sample:samples)sample=0.25f;
    assert(sf_writef_float(file,samples,4800)==4800);assert(sf_close(file)==0);
    {
        AudioSDL audio;assert(audio.ready());
        const auto one=MotionBinary64::fromInteger(1);const MotionBinary64 half{0x3fe00000u,0};
        assert(audio.load(349,path,one));assert(!audio.load(349,path,one));
        bool playing=true;assert(audio.isPlaying(-1,playing)&&!playing);
        MotionBinary64 absent;assert(!audio.getGain(-1,absent));assert(!audio.setGain(-1,one,0));
        int voice=-1;assert(!audio.play(350,90,false,voice)&&voice==-1);
        assert(audio.play(349,90,true,voice));assert(voice>360);
        assert(audio.isPlaying(349,playing)&&playing);
        assert(audio.isPlaying(voice,playing)&&playing);
        MotionBinary64 gain;assert(audio.getGain(voice,gain)&&gain.high==one.high);
        assert(audio.getGain(349,gain)&&gain.high==one.high);
        assert(audio.setGain(349,half,0)&&audio.getGain(349,gain)&&gain.high==half.high);
        assert(audio.getGain(voice,gain)&&gain.high==one.high); // Resource gain never overwrites voice gain.
        assert(audio.setGain(voice,half,0));assert(audio.getGain(voice,gain)&&gain.high==half.high);
        assert(AudioSDLTestAccess::sample(audio)==0.0625f); // .25 PCM * .5 resource * .5 voice.
        const MotionBinary64 sevenTenths{0x3fe66666u,0x66666666u};
        assert(audio.setGain(349,sevenTenths,0)&&audio.getGain(349,gain)&&gain.high==0x3fe66666u&&gain.low==0x60000000u);
        assert(audio.getGain(voice,gain)&&gain.high==half.high);
        assert(audio.setGain(349,MotionBinary64::fromInteger(-1),0)&&audio.getGain(349,gain)&&gain.zero());
        assert(audio.setGain(349,one,0));
        assert(audio.load(348,path,half));int quiet=-1;
        assert(audio.play(348,90,true,quiet)&&audio.getGain(quiet,gain)&&gain.high==one.high);
        assert(audio.getGain(348,gain)&&gain.high==half.high&&audio.stop(348));
        assert(audio.setGain(voice,one,200));assert(!audio.setPitch(voice,{}));
        assert(audio.setPitch(voice,half));SDL_Delay(250);
        assert(audio.getGain(voice,gain)&&gain.high==half.high); // No callback-time ramp.
        audio.update();assert(audio.getGain(voice,gain)&&gain.high==one.high&&gain.low==0);
        MusicDuck duck;
        assert(duck.execute(500,sevenTenths,[]{return 1;},[]{return 349;},audio));
        assert(audio.getGain(349,gain)&&gain.high==0x3fe66666u&&gain.low==0x60000000u);
        // A second hit while ducked is guarded, not a restart of the ramp.
        assert(duck.execute(500,half,[]{return 1;},[]{return 349;},audio));
        assert(audio.getGain(349,gain)&&gain.high==0x3fe66666u&&gain.low==0x60000000u);
        SDL_Delay(510);audio.update();
        assert(audio.getGain(349,gain)&&gain.high==one.high&&gain.low==0);
        assert(audio.isPlaying(voice,playing)&&playing);
        assert(audio.stop(349));assert(audio.isPlaying(voice,playing)&&!playing);
        const int old=voice;assert(audio.play(349,90,false,voice)&&voice!=old);
        assert(audio.isPlaying(old,playing)&&!playing);
        SDL_Delay(250);assert(audio.isPlaying(voice,playing)&&!playing);
        int voices[32];for(auto& id:voices)assert(audio.play(349,90,true,id));
        assert(!audio.play(349,90,true,voice));assert(audio.stop(voices[0]));
        assert(audio.isPlaying(349,playing)&&playing);assert(audio.stop(349));
        assert(audio.isPlaying(349,playing)&&!playing);
    }
    {
        AudioSDL audio;assert(audio.ready());AudioSDLTestAccess::manual(audio);
        const auto one=MotionBinary64::fromInteger(1);
        assert(audio.load(193,path,one));int first,second;
        assert(audio.play(193,80,true,first)&&audio.play(193,80,true,second));
        assert(AudioSDLTestAccess::sample(audio)==0.5f);
        const auto cursor=AudioSDLTestAccess::cursor(audio,first);
        TimeStopAudio transition;SoundPlayState caller;SoundPlayEnvironment environment;
        using E=PlayerTimeStop::Effect;using Q=PlayerTimeStop::AudioQuery;
        assert(transition.emit(E::pauseMusic,caller,environment,1,193,-1,audio));
        bool state=false;
        assert(transition.query(Q::musicPaused,193,-1,audio,state)&&state);
        assert(transition.query(Q::musicPlaying,193,-1,audio,state)&&state);
        assert(AudioSDLTestAccess::sample(audio)==0&&AudioSDLTestAccess::cursor(audio,first)==cursor);
        // Repeated pause is harmless; future resource voices are not paused.
        assert(audio.pause(193));int third;assert(audio.play(193,80,true,third));
        assert(audio.isPaused(third,state)&&!state);
        assert(AudioSDLTestAccess::sample(audio)==0.25f);
        assert(audio.stop(third));
        assert(audio.setGain(first,{},0)&&audio.setGain(first,one,1));
        SDL_Delay(5);audio.update();MotionBinary64 gain;
        assert(audio.getGain(first,gain)&&gain.high==one.high);
        assert(AudioSDLTestAccess::cursor(audio,first)==cursor);
        assert(audio.resume(first)&&audio.isPaused(193,state)&&state); // Second remains paused.
        assert(AudioSDLTestAccess::sample(audio)==0.25f);
        assert(AudioSDLTestAccess::cursor(audio,first)==cursor+1);
        assert(transition.emit(E::resumeMusic,caller,environment,1,193,-1,audio));
        assert(audio.isPaused(193,state)&&!state&&AudioSDLTestAccess::sample(audio)==0.5f);
        assert(audio.pause(193)&&audio.stop(193)&&audio.resume(193));
        assert(audio.isPlaying(193,state)&&!state&&audio.isPaused(193,state)&&!state);
        assert(audio.pause(-1)&&audio.resume(-1)&&audio.isPaused(-1,state)&&!state);
    }
    {
        AudioSDL audio;assert(audio.ready());
        for(int id=349;id<=355;++id){
            char asset[128];std::snprintf(asset,sizeof(asset),
                "extracted/audio/sound-effects/%03d_sfxMagicWind%d.ogg",id,id-348);
            assert(audio.load(id,asset,MotionBinary64::fromInteger(1)));
        }
        struct Random {unsigned calls=0;unsigned chooseIndexSmall(unsigned n){return calls++%n;}} random;
        TimeFieldWind wind;SoundPlayState caller;int sound=-1,channel=0;
        for(int id=349;id<=355;++id){
            assert(wind.contact(random,sound,caller,channel,MotionBinary64::fromInteger(1),audio));
            assert(sound==id&&channel==id&&random.calls==unsigned(id-348));
            MotionBinary64 gain;assert(audio.getGain(wind.voice,gain)&&gain.high==0x3fe00000u&&gain.low==0);
            const int voice=wind.voice;
            assert(wind.contact(random,sound,caller,channel,MotionBinary64::fromInteger(1),audio));
            assert(wind.voice==voice&&random.calls==unsigned(id-348));
            assert(audio.stop(voice));
        }
    }
    assert(unlink(path)==0);SDL_Quit();std::puts("SDL audio: playback, expiry, loops, resource/voice guards and capacity passed");
}
