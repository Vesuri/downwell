#ifndef DOWNWELL_CORE_SOUND_REQUESTS_H
#define DOWNWELL_CORE_SOUND_REQUESTS_H
#include "SoundVoices.h"
#include <cstdint>
namespace downwell {
// W2.3: what the game asks the audio backend to do, in source call order.
// Owners write through SoundRequestAudio (the Audio concept SoundPlayScript
// uses), which keeps SoundVoices, the gameplay-visible audio_is_playing
// model, in step. A backend (the Paula mixer, an SDL reference, a host route
// log) drains the ring once per frame; nothing in the game reads it back.
//
// A play request carries the soundPlay/soundPlayOL arguments: priority, loop,
// cut-previous and the OLchannel category (-1 for plain soundPlay), plus the
// gain and pitch the script settles on before it returns (SoundPlayScript
// sets them right after audio_play_sound, so they amend the request in
// place). Later gain or pitch changes to a live voice, stops of something
// the model still plays, and the pause menu's pause/resume-all follow as
// their own requests. Requests the model already knows are inaudible (a stop
// of a finished voice) are not written.
struct SoundRequest {
    enum Kind : std::uint8_t { play, stop, stopAll, pauseAll, resumeAll, gain, pitch, pause, resume };
    std::uint32_t tick=0;
    int voice=-1;         // play: the new voice; stop/gain/pitch: voice or asset id
    std::int16_t asset=-1; // play only
    Kind kind=play;
    std::uint8_t priority=0;
    std::int8_t category=-1;
    bool loop=false,cut=false;
    std::uint16_t fadeMs=0; // gain: audio_sound_gain's time argument
    AudioLevel gainValue{},pitchValue{};
};

class SoundRequests {
public:
    // A frame's worth with room to spare (the busiest natural-route step
    // writes about a dozen). A full ring drops its oldest request.
    static constexpr unsigned capacity=64;
    void setTick(std::uint32_t tick){tick_=tick;}
    std::uint32_t tick()const{return tick_;}
    void push(const SoundRequest& r){
        if(count_==capacity){first_=(first_+1)%capacity;--count_;++dropped_;}
        SoundRequest& slot=ring_[(first_+count_++)%capacity];
        slot=r;slot.tick=tick_;
    }
    unsigned size()const{return count_;}
    const SoundRequest& operator[](unsigned i)const{return ring_[(first_+i)%capacity];}
    SoundRequest& at(unsigned i){return ring_[(first_+i)%capacity];}
    void clear(){first_=count_=0;}
    std::uint32_t dropped()const{return dropped_;}
    // Drain into a backend: f(const SoundRequest&) for each, oldest first.
    template<class F> void drain(F f){for(unsigned i=0;i<count_;++i)f((*this)[i]);clear();}
private:
    SoundRequest ring_[capacity]{};
    unsigned first_=0,count_=0;
    std::uint32_t tick_=0,dropped_=0;
};

// The Audio concept over the voice model and the request ring. The caller
// names the call's category and cut flag (context) before running a sound
// script; SoundPlayScript itself only sees play/stop/gain/pitch.
class SoundRequestAudio {
public:
    SoundRequestAudio(SoundVoices& voices,SoundRequests& requests):voices_(voices),requests_(requests){}
    SoundRequestAudio& context(int category,bool cut){category_=std::int8_t(category);cut_=cut;return *this;}
    bool play(int asset,int priority,bool loop,int& voice){
        voices_.play(asset,priority,loop,voice);
        SoundRequest r;r.kind=SoundRequest::play;r.voice=voice;r.asset=std::int16_t(asset);
        r.priority=std::uint8_t(priority<0?0:priority>255?255:priority);
        r.category=category_;r.loop=loop;r.cut=cut_;
        voices_.getGain(voice,r.gainValue);r.pitchValue=audio_level::one;
        if(!voices_.playing(voice))voices_.getGain(asset,r.gainValue);
        requests_.push(r);
        return true;
    }
    bool isPlaying(int id,bool& playing)const{return voices_.isPlaying(id,playing);}
    bool stop(int id){
        if(!voices_.playing(id))return true;
        voices_.stop(id);
        SoundRequest r;r.kind=SoundRequest::stop;r.voice=id;requests_.push(r);
        return true;
    }
    bool stopAll(){voices_.stopAll();SoundRequest r;r.kind=SoundRequest::stopAll;requests_.push(r);return true;}
    bool pauseAll(){voices_.pauseAll();SoundRequest r;r.kind=SoundRequest::pauseAll;requests_.push(r);return true;}
    bool resumeAll(){voices_.resumeAll();SoundRequest r;r.kind=SoundRequest::resumeAll;requests_.push(r);return true;}
    // audio_pause_sound / audio_resume_sound / audio_is_paused (the song
    // around the void rooms).
    bool pause(int id){return single(id,SoundRequest::pause,[&]{voices_.pause(id);});}
    bool resume(int id){return single(id,SoundRequest::resume,[&]{voices_.resume(id);});}
    bool isPaused(int id,bool& paused)const{return voices_.isPaused(id,paused);}
    bool getGain(int id,AudioLevel& gain)const{return voices_.getGain(id,gain);}
    bool setGain(int id,AudioLevel gain,int time){
        voices_.setGain(id,gain,time);
        return amend(id,SoundRequest::gain,gain,time);
    }
    bool setPitch(int id,AudioLevel pitch){
        voices_.setPitch(id,pitch);
        return amend(id,SoundRequest::pitch,pitch);
    }
private:
    // An immediate change to a voice started this step (by voice id, or by
    // asset id as scrShotSound's audio_sound_pitch does) settles that play
    // request; a fade, or a change to an older live voice, is its own request.
    bool amend(int id,SoundRequest::Kind kind,AudioLevel value,int time=0){
        if(id<0)return true;
        for(unsigned i=time?0:requests_.size();i--;){
            SoundRequest& n=requests_.at(i);
            if(n.tick!=requests_.tick())break;
            if(n.kind==SoundRequest::play&&(n.voice==id||(id<SoundVoices::firstVoice&&n.asset==id))){
                (kind==SoundRequest::gain?n.gainValue:n.pitchValue)=value;return true;
            }
        }
        if(!voices_.playing(id))return true;
        SoundRequest r;r.kind=kind;r.voice=id;r.fadeMs=std::uint16_t(time<0?0:time>65535?65535:time);
        (kind==SoundRequest::gain?r.gainValue:r.pitchValue)=value;
        requests_.push(r);
        return true;
    }
    template<class Apply> bool single(int id,SoundRequest::Kind kind,Apply apply){
        if(!voices_.playing(id))return true;
        apply();
        SoundRequest r;r.kind=kind;r.voice=id;requests_.push(r);
        return true;
    }
    SoundVoices& voices_;
    SoundRequests& requests_;
    std::int8_t category_=-1;
    bool cut_=false;
};
}
#endif
