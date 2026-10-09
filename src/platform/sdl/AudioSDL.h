#ifndef DOWNWELL_AUDIO_SDL_H
#define DOWNWELL_AUDIO_SDL_H
#include "core/MotionBinary64.h"
#include "core/RunnerAudioGain.h"
#include <SDL.h>
#include <array>
#include <vector>

namespace downwell {
// Device-time voices, separate from the caller-visible GML sound-script state.
// Unsupported resources and exhausted capacity fail rather than fake playback.
class AudioSDL {
public:
    AudioSDL();
    ~AudioSDL();
    AudioSDL(const AudioSDL&)=delete;
    AudioSDL& operator=(const AudioSDL&)=delete;
    bool ready() const {return device_!=0;}
    bool load(int resource,const char* path,MotionBinary64 gain);
    bool play(int resource,int priority,bool loop,int& voice);
    bool isPlaying(int id,bool& playing);
    bool stop(int id);
    bool getGain(int id,MotionBinary64& gain);
    bool setGain(int id,MotionBinary64 gain,int milliseconds);
    bool setPitch(int id,MotionBinary64 pitch);
    void update();
private:
    friend struct AudioSDLTestAccess;
    struct Sample {std::vector<float> pcm;int rate=0;RunnerAudioGain gain{};};
    struct Voice {int id=-1,resource=-1;double cursor=0,pitch=1;RunnerAudioGain gain{};bool active=false,loop=false;};
    struct Lock {SDL_AudioDeviceID device;explicit Lock(SDL_AudioDeviceID d):device(d){if(d)SDL_LockAudioDevice(d);}~Lock(){if(device)SDL_UnlockAudioDevice(device);}};
    static void callback(void*,Uint8*,int);
    void mix(float*,unsigned);
    Voice* find(int);
    static double real(MotionBinary64);
    static std::int64_t microseconds();
    std::array<Sample,361> samples_;
    std::array<Voice,32> voices_{};
    SDL_AudioDeviceID device_=0;
    int nextVoice_=1000000;
    int rate_=48000;
};
}
#endif
