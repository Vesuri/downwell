#ifndef DOWNWELL_CORE_REGISTERED_SCATTERED_CONTACT_H
#define DOWNWELL_CORE_REGISTERED_SCATTERED_CONTACT_H
#include "ScatteredEvents.h"
#include "SoundPlayScript.h"
namespace downwell {
// Scenery contact is not scrEnemyStomped: no death guard, blood, stomp sound,
// second recharge or explosion smoke. Preserve scrFjump's caller fields and
// its synchronous recharge-before-velocity order. Player supplies exact live
// selector writes/recharge, not PlayerMotion::stompBounce's compact shortcut.
template<class World,class Player,class Blast,class Audio>
auto registeredScatteredContact(World& world,ScatteredScriptState& state,
    Player& player,Blast blast,SoundPlayState& sound,SoundPlayEnvironment& environment,Audio& audio,bool& good){
    struct Contact {
        World& world;ScatteredScriptState& state;Player& player;Blast blast;
        SoundPlayState& soundState;SoundPlayEnvironment& environment;Audio& audio;bool& good;
        bool playerYSpeed(MotionBinary64& speed){return player.playerYSpeed(speed);}
        bool playStyle(int& value){value=player.playStyle();return true;}
        bool jump(int horizontal,MotionBinary64 vertical){
            return state.jump.execute(MotionBinary64::fromInteger(horizontal),vertical,player);
        }
        bool shake(int amount,int duration){return state.shake.execute(amount,duration,
            [&](int a,int d){return world.requestCameraShake(a,d);});}
        bool explodingKnees(bool& value){value=world.upgrades().levels[0]!=0;return true;}
        bool createBlast(){return blast();}
        bool sound(int id,int priority,int loop,int cut){int voice;
            return SoundPlayScript::plain(soundState,environment,audio,id,priority,loop!=0,cut!=0,voice,good);
        }
    };
    return Contact{world,state,player,blast,sound,environment,audio,good};
}
}
#endif
