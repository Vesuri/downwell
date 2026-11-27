#ifndef DOWNWELL_CORE_BOX_SOUND_VERIFICATION_H
#define DOWNWELL_CORE_BOX_SOUND_VERIFICATION_H
#include "SoundPlayScript.h"
#include "InstanceHandle.h"
namespace downwell {
// World ownership tape. Script branch semantics have separate VM-pinned tests.
// The injected audio backend records commands; it does not play audible sound.
struct BoxSoundVerification {
    template<class World> struct Audio {
        World& world;
        int sound;
        unsigned fail=0,calls=0;
        bool valid=true;
        bool chain=false,water=false;
        bool call(int op){
            const int expected[]={sound,1000+sound,2000+sound,3000,4000,5000,900,
                342,1342,2342,3000,4000,5000};
            if(calls>=(chain?13u:7u)||op!=expected[calls])valid=false;
            ++calls;return valid&&calls!=fail;
        }
        bool isPlaying(int id,bool& yes){yes=true;return call(id);}
        bool stop(int id){return call(1000+id);}
        bool play(int id,int priority,bool loop,int& result){
            valid=valid&&world.overlapSound(7)==sound&&priority==50&&!loop;
            result=900;return call(2000+id);
        }
        bool getGain(int id,AudioLevel& gain){valid=valid&&id==900;gain=audio_level::half;return call(3000);}
        bool setGain(int id,AudioLevel gain,int duration){
            valid=valid&&id==900&&duration==0&&gain==AudioLevel::raw(8192);return call(4000);
        }
        bool setPitch(int id,AudioLevel pitch){
            const auto expected=calls>=7&&water?AudioLevel::raw(36045):audio_level::one;
            valid=valid&&id==900&&pitch==expected;return call(5000);
        }
    };
    template<class World,class Read> static bool run(World& world,InstanceHandle box,int sound,Read read,unsigned failure=0){
        Audio<World> audio{world,sound,failure};const auto draws=world.randomDrawCount();
        const int before=world.overlapSound(7);
        const bool ok=world.playBoxBreakSound(box,AudioLevel::raw(16384),audio);
        const auto* caller=read();
        if(!caller||!audio.valid||audio.calls!=(failure?failure:7)||ok!=(failure==0)||world.effectsSucceeded()!=ok||
            world.randomDrawCount()!=draws||caller->sfxIndex!=sound||caller->soundPriority!=50||
            caller->loop||caller->cutPrv||caller->chanNum!=7||caller->doNotPlay||
            world.overlapSound(7)!=((!failure||failure>=3)?sound:before))return false;
        if(!failure)return caller->sndsnd==900&&caller->sndgain==AudioLevel::raw(8192);
        return !world.playBoxBreakSound(box,AudioLevel::raw(16384),audio)&&audio.calls==failure&&world.randomDrawCount()==draws;
    }
};
}
#endif
