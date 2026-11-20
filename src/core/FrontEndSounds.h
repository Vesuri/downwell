#ifndef DOWNWELL_CORE_FRONT_END_SOUNDS_H
#define DOWNWELL_CORE_FRONT_END_SOUNDS_H
#include "SoundPlayScript.h"
#include "SoundRequests.h"
namespace downwell {
// W2.3: the title garden's and PlayMenu's sounds (objTitle, EnterSequence,
// PlayMenu) through the same scripts and request ring as the run. The front
// end has no gameplay-visible audio queries, so its own voice model only
// keeps soundPlayOL's same-sound stop and the backend's view consistent. The
// backend drains requests() each frame, before the run's ring.
class FrontEndSounds {
public:
    void step(std::uint32_t tick){voices_.advance();requests_.setTick(tick);}
    int play(int asset,int priority,bool loop,bool cut){
        SoundPlayEnvironment env;auto a=audio(-1,cut);int result=-1;
        SoundPlayScript::plain(state_,env,a,asset,priority,loop,cut,result,good_);
        return result;
    }
    int playOL(int asset,int priority,bool loop,bool cut,const char* channel){
        SoundPlayEnvironment env;const int category=SoundPlayScript::channel(channel);
        auto a=audio(category,cut);int result=-1;bool returned=false;
        SoundPlayScript::overlap(state_,env,a,[]{return true;},overlapSounds_[category],
            asset,priority,loop,cut,channel,result,returned,good_);
        return returned?result:-1;
    }
    bool isPlaying(int id)const{return voices_.playing(id);}
    void stop(int id){audio(-1,false).stop(id);}
    void stopAll(){audio(-1,false).stopAll();}
    SoundRequests& requests(){return requests_;}
    const SoundVoices& voices()const{return voices_;}
    bool good()const{return good_;}
private:
    SoundRequestAudio audio(int category,bool cut){return SoundRequestAudio(voices_,requests_).context(category,cut);}
    SoundVoices voices_;
    SoundRequests requests_;
    SoundPlayState state_;
    int overlapSounds_[11]{};
    bool good_=true;
};
}
#endif
