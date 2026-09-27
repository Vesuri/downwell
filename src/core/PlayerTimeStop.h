#ifndef DOWNWELL_CORE_PLAYER_TIME_STOP_H
#define DOWNWELL_CORE_PLAYER_TIME_STOP_H
namespace downwell {
// objPlayer_n Begin Step blocks 46..79. Synchronous callbacks preserve source
// Create/RNG/audio order; they must not mutate this owner. This is the boolean
// state domain written by the source, not arbitrary VM numeric values.
struct PlayerTimeStop {
    bool checkStop=false,stopped=false,bgmOn=true;
    enum class Effect {
        shards,             // scrTimeShard: published x/y, own field lookup/RNG
        shake,              // scrSShake(2,2)
        change,             // instance_create(xx,yy,211), synchronous Create
        pauseMusic,resumeMusic,stopWind,
        enterSound,exitSound,// soundPlay(184/185,80,0,1)
        duckMusic,          // bgmDuck(200,0.7)
        oxygenFloor         // if oxygen<10, oxygen=10 (every stopped Begin Step)
    };
    enum class AudioQuery {musicPlaying,musicPaused,windPlaying};
    // field(bool&) queries place_meeting(xx,yy,82), including its repeated
    // call on entry. outsideSides means precise xx<160 || xx>room_width-160.
    // mainRoom includes rmTutorialMain. Other rooms have no side exemption.
    template<class Field,class Audio,class Emit>
    bool step(bool mainRoom,bool outsideSides,Field field,Audio audio,Emit emit){
        return stepChecked(mainRoom,[&](bool& outside){outside=outsideSides;return true;},field,audio,emit);
    }
    // Side predicates are short-circuited after the first field query, and
    // never evaluated outside rmMain/rmTutorialMain.
    template<class Side,class Field,class Audio,class Emit>
    bool stepChecked(bool mainRoom,Side side,Field field,Audio audio,Emit emit){
        bool inField=false;
        if(!field(inField))return false;
        if(mainRoom){
            bool outsideSides=false;
            if(!inField&&!side(outsideSides))return false;
            if(inField || outsideSides){
                if(!checkStop){
                    checkStop=true;
                    if(!field(inField))return false;
                    if(inField && (!emit(Effect::shards) || !emit(Effect::shake)))return false;
                }
            }else if(checkStop){
                checkStop=false;
                if(!emit(Effect::shards))return false;
            }
        }else checkStop=inField;
        if(checkStop && !stopped){
            stopped=true;
            if(!emit(Effect::change))return false;
            if(audio(AudioQuery::musicPlaying) && !emit(Effect::pauseMusic))return false;
            bgmOn=false;
            if(!emit(Effect::shards) || !emit(Effect::enterSound))return false;
        }else if(!checkStop && stopped){
            stopped=false;
            if(!emit(Effect::change))return false;
            if(audio(AudioQuery::musicPaused) && !emit(Effect::resumeMusic))return false;
            if(audio(AudioQuery::windPlaying) && !emit(Effect::stopWind))return false;
            bgmOn=true;
            if(!emit(Effect::shards) || !emit(Effect::exitSound) || !emit(Effect::duckMusic))return false;
        }
        return !stopped || emit(Effect::oxygenFloor);
    }
};
}
#endif
