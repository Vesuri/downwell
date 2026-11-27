#ifndef DOWNWELL_AMIGA_RUN_SOUND_AUDIO_H
#define DOWNWELL_AMIGA_RUN_SOUND_AUDIO_H
// W2.6: the encounter environment's Audio (AmigaEncounterResources, the
// scene collision candidate build) over the run's voice model and request
// ring. Its only player is the goal fields' TimeFieldWind, soundPlayOL on the
// UI channel with cut-previous, as RunSession::resultStepWind calls it, so
// the winds are ordinary run-bank sounds the Paula mixer plays and
// audio_is_playing reads the same voices as the rest of the run.
#include "core/GameplayWorld.h"

namespace downwell {
class AmigaRunSoundAudio {
public:
    explicit AmigaRunSoundAudio(GameplayWorld& world):world_(world){}
    bool ready()const{return true;}
    // The winds are in the run bank; a play the loaded banks lack counts as
    // missing in the mixer rather than failing the update.
    bool hasResource(int)const{return true;}
    // The volume option is the mixer's master gain (DownwellAmiga.cpp).
    bool setMasterGain(AudioLevel){return true;}
    void update(){}
    bool play(int asset,int priority,bool loop,int& voice){return audio().play(asset,priority,loop,voice);}
    bool isPlaying(int id,bool& playing){return audio().isPlaying(id,playing);}
    bool stop(int id){return audio().stop(id);}
    bool stopAll(){return audio().stopAll();}
    bool getGain(int id,AudioLevel& gain){return audio().getGain(id,gain);}
    bool setGain(int id,AudioLevel gain,int time){return audio().setGain(id,gain,time);}
    bool setPitch(int id,AudioLevel pitch){return audio().setPitch(id,pitch);}
private:
    SoundRequestAudio audio(){return world_.runSoundAudio(SoundPlayScript::channel("UI"),true);}
    GameplayWorld& world_;
};
}
#endif
