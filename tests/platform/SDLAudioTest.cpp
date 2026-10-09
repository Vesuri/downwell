#include "platform/sdl/AudioSDL.h"
#include "core/TimeFieldWind.h"
#include <sndfile.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
using namespace downwell;
int main(){
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
        assert(audio.setGain(voice,half,0));assert(audio.getGain(voice,gain)&&gain.high==half.high);
        assert(!audio.setGain(voice,one,200));assert(!audio.setPitch(voice,{}));
        assert(audio.setPitch(voice,half));SDL_Delay(250);
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
