#include "AudioSDL.h"
#include "core/RunnerImageValue.h"
#include <sndfile.h>
#include <algorithm>
#include <climits>
#include <cstring>
#include <cmath>

namespace downwell {
double AudioSDL::real(MotionBinary64 value){
    const std::uint64_t bits=(std::uint64_t(value.high)<<32)|value.low;
    double result;static_assert(sizeof(result)==sizeof(bits));
    std::memcpy(&result,&bits,sizeof(result));return result;
}
AudioSDL::AudioSDL(){
    SDL_AudioSpec wanted{},actual{};
    wanted.freq=rate_;wanted.format=AUDIO_F32SYS;wanted.channels=2;
    wanted.samples=512;wanted.callback=callback;wanted.userdata=this;
    device_=SDL_OpenAudioDevice(nullptr,0,&wanted,&actual,0);
    if(device_)SDL_PauseAudioDevice(device_,0);
}
AudioSDL::~AudioSDL(){if(device_)SDL_CloseAudioDevice(device_);}
bool AudioSDL::load(int id,const char* path,MotionBinary64 gain){
    if(!device_||id<0||id>=int(samples_.size())||!gain.valid()||real(gain)<0)return false;
    SF_INFO info{};SNDFILE* file=sf_open(path,SFM_READ,&info);
    if(!file)return false;
    if(info.frames<=0||info.frames>48000*600||info.samplerate<=0||info.channels<1||info.channels>2){sf_close(file);return false;}
    std::vector<float> input(std::size_t(info.frames)*info.channels);
    const auto read=sf_readf_float(file,input.data(),info.frames);sf_close(file);
    if(read!=info.frames)return false;
    RunnerImageValue storedGain;if(!storedGain.assign(gain))return false;
    Sample sample;sample.rate=info.samplerate;sample.gain.current=storedGain;
    sample.pcm.resize(std::size_t(info.frames)*2);
    for(std::size_t frame=0;frame<std::size_t(info.frames);++frame)
        for(unsigned channel=0;channel<2;++channel){
            const float value=input[frame*info.channels+(info.channels==1?0:channel)];
            if(!std::isfinite(value))return false;
            sample.pcm[frame*2+channel]=value;
        }
    Lock lock(device_);
    if(!samples_[id].pcm.empty())return false;
    samples_[id]=std::move(sample);return true;
}
AudioSDL::Voice* AudioSDL::find(int id){if(id<1000000)return nullptr;for(auto& voice:voices_)if(voice.id==id)return &voice;return nullptr;}
bool AudioSDL::play(int resource,int /*priority*/,bool loop,int& id){
    Lock lock(device_);
    if(!device_||resource<0||resource>=int(samples_.size())||samples_[resource].pcm.empty()||nextVoice_==INT_MAX)return false;
    for(auto& voice:voices_)if(!voice.active){
        voice={};voice.id=nextVoice_++;voice.resource=resource;voice.gain.current.assignInteger(1);
        voice.active=true;voice.loop=loop;id=voice.id;return true;
    }
    return false; // No unverified priority stealing policy.
}
bool AudioSDL::isPlaying(int id,bool& playing){
    Lock lock(device_);playing=false;
    for(const auto& voice:voices_)if(voice.active&&(voice.id==id||voice.resource==id))playing=true;
    return device_!=0;
}
bool AudioSDL::stop(int id){Lock lock(device_);for(auto& v:voices_)if(v.id==id||v.resource==id)v.active=false;return device_!=0;}
bool AudioSDL::getGain(int id,MotionBinary64& gain){
    Lock lock(device_);
    if(id>=0&&id<int(samples_.size())&&!samples_[id].pcm.empty()){gain=samples_[id].gain.current.promoted();return true;}
    auto* v=find(id);if(!v)return false;gain=v->gain.current.promoted();return true;
}
bool AudioSDL::setGain(int id,MotionBinary64 gain,int milliseconds){
    Lock lock(device_);
    const auto now=microseconds();
    if(id>=0&&id<int(samples_.size())&&!samples_[id].pcm.empty())return samples_[id].gain.set(gain,milliseconds,now);
    auto* v=find(id);return v&&v->gain.set(gain,milliseconds,now);
}
std::int64_t AudioSDL::microseconds(){
    const auto ticks=SDL_GetPerformanceCounter(), frequency=SDL_GetPerformanceFrequency();
    return std::int64_t((ticks/frequency)*1000000+(ticks%frequency)*1000000/frequency);
}
void AudioSDL::update(){
    Lock lock(device_);const auto now=microseconds();
    for(auto& sample:samples_)sample.gain.update(now);
    for(auto& voice:voices_)if(voice.active)voice.gain.update(now);
}
bool AudioSDL::setPitch(int id,MotionBinary64 pitch){
    if(!pitch.valid()||real(pitch)<=0||real(pitch)>256)return false;
    Lock lock(device_);auto* v=find(id);if(!v)return false;v->pitch=real(pitch);return true;
}
void AudioSDL::callback(void* self,Uint8* bytes,int count){
    std::memset(bytes,0,count);
    static_cast<AudioSDL*>(self)->mix(reinterpret_cast<float*>(bytes),unsigned(count)/(2*sizeof(float)));
}
void AudioSDL::mix(float* output,unsigned frames){
    for(auto& v:voices_)if(v.active){
        const auto& sample=samples_[v.resource];const auto length=sample.pcm.size()/2;
        const double increment=v.pitch*sample.rate/rate_;
        // Resource and voice gains are independent binary32 controls.
        const float gain=RunnerAudioGain::value(v.gain.current)*RunnerAudioGain::value(sample.gain.current);
        for(unsigned frame=0;frame<frames&&v.active;++frame){
            const auto a=std::size_t(v.cursor);const auto b=a+1<length?a+1:(v.loop?0:a);
            const double fraction=v.cursor-a;
            for(unsigned channel=0;channel<2;++channel)
                output[frame*2+channel]+=float((sample.pcm[a*2+channel]*(1-fraction)+sample.pcm[b*2+channel]*fraction)*gain);
            v.cursor+=increment;
            if(v.cursor>=length){if(v.loop)v.cursor=std::fmod(v.cursor,double(length));else v.active=false;}
        }
    }
    for(unsigned i=0;i<frames*2;++i)output[i]=std::clamp(output[i],-1.0f,1.0f);
}
}
