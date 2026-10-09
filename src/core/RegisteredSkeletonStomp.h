#ifndef DOWNWELL_CORE_REGISTERED_SKELETON_STOMP_H
#define DOWNWELL_CORE_REGISTERED_SKELETON_STOMP_H
#include "SkeletonActor.h"
namespace downwell {
// Borrow live state; never copy the attacker's script variables or supply
// successful stand-ins for precise player writes / synchronous child births.
template<class World,class Events,class Blood,class Smoke,class Audio>
bool registeredSkeletonStomp(World& world,SkeletonActor& actor,Events& external,
    Blood blood,Smoke smoke,SoundPlayEnvironment& environment,Audio& audio,bool& good){
    struct Services {
        World& world;SkeletonActor& actor;Events& external;Blood& burst;Smoke& smoke;
        SoundPlayEnvironment& environment;Audio& audio;
        bool playerDead(){return world.playerDeathActive();}
        bool explodingKnees(){return world.upgrades().levels[0]!=0;}
        int playStyle(){return external.playStyle();}
        bool blood(int count,int spread){return count==3&&spread==0&&burst();}
        void setSpinJumping(){external.setSpinJumping();}
        bool recharge(){return external.recharge();}
        bool setGrounded(bool value){return external.setGrounded(value);}
        bool setHorizontal(MotionBinary64 value){return external.setHorizontal(value);}
        bool setVertical(MotionBinary64 value){return external.setVertical(value);}
        bool shake(int amount,int duration){return world.requestCameraShake(amount,duration);}
        bool stompSound(int id,int priority,bool loop,bool cut,const char* channel){
            return id==122&&priority==50&&!loop&&cut&&SoundPlayScript::channel(channel)==0&&
                world.playSkeletonStompSound(actor,environment,audio);
        }
        bool explosionSound(int id,int priority,bool loop,bool cut){
            bool succeeded=true;int voice=-1;
            return SoundPlayScript::plain(actor.sound,environment,audio,id,priority,loop,cut,voice,succeeded);
        }
        bool createBlast(int id,int x,int y){return external.createBlast(id,x,y);}
        unsigned choose(unsigned count){return world.chooseSkeletonExplosion(count);}
        MotionBinary64 playerX(){return external.playerX();}
        MotionBinary64 playerY(){return external.playerY();}
        bool emitSmoke(MotionBinary64 x,MotionBinary64 y,int direction,int speed){
            return smoke(x,y,MotionBinary64::fromInteger(direction),MotionBinary64::fromInteger(speed));
        }
    } services{world,actor,external,blood,smoke,environment,audio};
    return actor.stompPlayerChecked(services,good);
}
}
#endif
