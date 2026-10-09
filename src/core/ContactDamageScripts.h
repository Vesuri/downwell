#ifndef DOWNWELL_CORE_CONTACT_DAMAGE_SCRIPTS_H
#define DOWNWELL_CORE_CONTACT_DAMAGE_SCRIPTS_H
#include "TypicalPlayerDamage.h"
#include "PlayerDamage.h"
#include "ForcedPlayerJump.h"
#include "MusicDuck.h"
#include "SoundPlayScript.h"
namespace downwell {
// VM self storage, owned by the attacking instance, not the player. Keep it
// through destruction marking and failed synchronous child calls. These are
// distinct source fields: pdmg/p?impact/pdir, hitInvi, and ?fjump/?fjumpdef.
// Object-name/string resolution remains the damage event adapter's job.
struct ContactDamageScripts {
    TypicalPlayerDamage typical;
    PlayerDamage player;
    ForcedPlayerJump jump;
    MusicDuck music;
    template<class Events,class CreateField>
    bool execute(MotionBinary64 amount,MotionBinary64 horizontal,MotionBinary64 vertical,
        Events& events,CreateField createField){
        return executeWithState(amount,horizontal,vertical,events,createField,events);
    }
    template<class Events,class CreateField,class State>
    bool executeWithState(MotionBinary64 amount,MotionBinary64 horizontal,MotionBinary64 vertical,
        Events& events,CreateField createField,State& state){
        return executeWithServices(amount,horizontal,vertical,events,createField,state,events);
    }
    template<class Events,class CreateField,class State,class Audio>
    bool executeWithServices(MotionBinary64 amount,MotionBinary64 horizontal,MotionBinary64 vertical,
        Events& events,CreateField createField,State& state,Audio& audio){
        return typical.executeCallsWithState(amount,horizontal,vertical,events,createField,
            [&](MotionBinary64 value){return player.executeWithServices(value,events,state,audio);},
            [&](MotionBinary64 x,MotionBinary64 y){return jump.execute(x,y,events);},state);
    }
};
// The sound state is the SAME caller storage used by its other sound scripts,
// e.g. a skeleton's throw/hurt/death sounds, not a separate damage-only copy.
template<class World,class Audio> struct ContactDamageAudio {
    World& world;MusicDuck& music;SoundPlayState& caller;
    SoundPlayEnvironment& environment;Audio& audio;bool succeeded=true;
    bool sound(int id,int priority,bool loop,bool cut){
        int voice=-1;
        return SoundPlayScript::plain(caller,environment,audio,id,priority,loop,cut,voice,succeeded);
    }
    bool duckMusic(int duration,MotionBinary64 amount){
        return succeeded&&(succeeded=world.duckMusic(music,duration,amount,audio));
    }
};
// Pass this borrowed view to either world's typicalPlayerDamage entry. Caller
// fields still belong to the attacking skeleton/bone; only player state is bound.
template<class World,class SelectPlayer> struct RegisteredContactDamage {
    ContactDamageScripts& caller;RegisteredPlayerDamageState<World,SelectPlayer> state;
    template<class Events,class CreateField>
    bool execute(MotionBinary64 amount,MotionBinary64 horizontal,MotionBinary64 vertical,
        Events& events,CreateField createField){
        return caller.executeWithState(amount,horizontal,vertical,events,createField,state);
    }
};
template<class World,class SelectPlayer> RegisteredContactDamage<World,SelectPlayer> bindContactDamagePlayer(
    ContactDamageScripts& caller,World& world,SelectPlayer selectPlayer){return {caller,{world,selectPlayer}};}
template<class World,class SelectPlayer,class Audio> struct AudibleRegisteredContactDamage {
    RegisteredContactDamage<World,SelectPlayer> binding;ContactDamageAudio<World,Audio> services;
    template<class Events,class CreateField>
    bool execute(MotionBinary64 amount,MotionBinary64 horizontal,MotionBinary64 vertical,
        Events& events,CreateField createField){
        return binding.caller.executeWithServices(amount,horizontal,vertical,events,createField,binding.state,services);
    }
};
template<class World,class SelectPlayer,class Audio>
AudibleRegisteredContactDamage<World,SelectPlayer,Audio> bindContactDamagePlayer(
    ContactDamageScripts& caller,World& world,SelectPlayer selectPlayer,
    SoundPlayState& sound,SoundPlayEnvironment& environment,Audio& audio){
    return {{caller,{world,selectPlayer}},{world,caller.music,sound,environment,audio}};
}
}
#endif
