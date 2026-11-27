#ifndef DOWNWELL_CORE_SOUND_VOICES_H
#define DOWNWELL_CORE_SOUND_VOICES_H
#include "AudioLevel.h"
#include "SourceSoundDurations.h"
#include <cstdint>
namespace downwell {
// W2.3: GameMaker's audio voices as the game logic sees them. It answers
// audio_is_playing from each source sound's length in 60 Hz steps, never from
// a device, so gameplay that reads audio state (soundLand's sfxGunRecharge
// test, global.voidWind) replays identically on every backend and whatever
// the Amiga cut. Ids follow the device backends: assets are resource numbers,
// voices count up from 1000000; -1 is no voice.
//
// Owners call advance() once at the start of every source step that runs
// (gameplay, the clear screen, ...). A sound started during step S then
// reads as playing at S + k while k < SourceSoundDurations::ticks. Loops play
// until stopped; paused voices keep their remaining steps
// (audio_pause_all / audio_resume_all around the pause menu).
//
// The channel limit is GameMaker's audio_channel_num(32): a full set drops
// its lowest-priority, oldest voice for a new sound of at least its priority,
// and otherwise the new sound does not start (play still returns its id,
// which never reads as playing).
//
// Live voices are kept packed in [0, count_), so an idle world scans nothing
// (the quiet-fall budget): advance() and every query touch live voices only.
class SoundVoices {
public:
    static constexpr unsigned capacity=32;
    static constexpr int firstVoice=1000000;
    // Source gain of a fresh voice: the asset volume (SourceSoundDurations).
    using AssetGain=AudioLevel (*)(int asset);

    void configureAssetGain(AssetGain gain){assetGain_=gain;}
    void advance(){
        for(unsigned i=0;i<count_;){
            Voice& v=voices_[i];
            if(!v.paused&&!v.loop&&--v.remaining==0){remove(i);continue;}
            ++i;
        }
    }
    bool play(int asset,int priority,bool loop,int& voice){
        voice=next_++;
        if(next_<firstVoice)next_=firstVoice; // Wrapped after 2^31 plays: keep ids apart from assets.
        const std::uint16_t remaining=SourceSoundDurations::ticks(asset);
        if(!loop&&!remaining)return true; // Nothing to hear: never reads as playing.
        Voice* slot=nullptr;
        if(count_<capacity)slot=&voices_[count_++];
        else{
            for(unsigned i=0;i<count_;++i){Voice& v=voices_[i];
                if(!slot||v.priority<slot->priority||(v.priority==slot->priority&&v.serial<slot->serial))slot=&v;}
            if(slot->priority>priority)return true; // Dropped; the id reads as stopped.
        }
        *slot={};
        slot->id=voice;slot->asset=asset;slot->priority=priority;slot->loop=loop;
        slot->serial=serial_++;
        slot->remaining=remaining;
        slot->gain=assetGain_?assetGain_(asset):audio_level::one;
        slot->pitch=audio_level::one;
        return true;
    }
    bool isPlaying(int id,bool& playing)const{playing=find(id)!=nullptr;return true;}
    bool stop(int id){
        for(unsigned i=0;i<count_;)if(matches(voices_[i],id))remove(i);else ++i;
        return true;
    }
    bool stopAll(){count_=0;return true;}
    bool pauseAll(){for(unsigned i=0;i<count_;++i)voices_[i].paused=true;return true;}
    bool resumeAll(){for(unsigned i=0;i<count_;++i)voices_[i].paused=false;return true;}
    bool pause(int id){for(unsigned i=0;i<count_;++i)if(matches(voices_[i],id))voices_[i].paused=true;return true;}
    bool resume(int id){for(unsigned i=0;i<count_;++i)if(matches(voices_[i],id))voices_[i].paused=false;return true;}
    bool isPaused(int id,bool& paused)const{
        paused=false;
        for(unsigned i=0;i<count_;++i)if(matches(voices_[i],id)&&voices_[i].paused){paused=true;break;}
        return true;
    }
    // audio_sound_get_gain/audio_sound_gain/audio_sound_pitch on a voice or an
    // asset. The model keeps the values for the request log; they never
    // change a voice's length (pitch is 1 for every gameplay-visible sound).
    bool getGain(int id,AudioLevel& gain)const{
        gain=assetGain_&&id>=0&&id<firstVoice?assetGain_(id):audio_level::one;
        for(unsigned i=0;i<count_;++i)if(voices_[i].id==id){gain=voices_[i].gain;break;}
        return true;
    }
    bool setGain(int id,AudioLevel gain,int){
        for(unsigned i=0;i<count_;++i)if(matches(voices_[i],id))voices_[i].gain=gain;
        return true;
    }
    bool setPitch(int id,AudioLevel pitch){
        for(unsigned i=0;i<count_;++i)if(matches(voices_[i],id))voices_[i].pitch=pitch;
        return true;
    }
    bool playing(int id)const{return find(id)!=nullptr;}
    unsigned liveCount()const{return count_;}
    // f(asset) for each live voice (route logs: peak concurrency per sound).
    template<class F> void forEachLive(F f)const{for(unsigned i=0;i<count_;++i)f(voices_[i].asset);}
private:
    struct Voice {
        int id=-1,asset=-1,priority=0;
        std::uint32_t serial=0;
        std::uint16_t remaining=0;
        bool loop=false,paused=false;
        AudioLevel gain{},pitch{};
    };
    static bool matches(const Voice& v,int id){return id>=0&&(v.id==id||(id<firstVoice&&v.asset==id));}
    const Voice* find(int id)const{
        if(id<0)return nullptr;
        for(unsigned i=0;i<count_;++i)if(matches(voices_[i],id))return &voices_[i];
        return nullptr;
    }
    // Order inside the live range is not observable: stealing compares
    // serials, and every query is a membership test.
    void remove(unsigned i){voices_[i]=voices_[--count_];}
    Voice voices_[capacity]{};
    unsigned count_=0;
    int next_=firstVoice;
    std::uint32_t serial_=0;
    AssetGain assetGain_=&SourceSoundDurations::gain;
};
}
#endif
