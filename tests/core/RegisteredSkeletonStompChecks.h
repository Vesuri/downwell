#ifndef DOWNWELL_TEST_REGISTERED_SKELETON_STOMP_CHECKS_H
#define DOWNWELL_TEST_REGISTERED_SKELETON_STOMP_CHECKS_H
#include "SkeletonSoundChecks.h"
#include "core/SmokeEffect.h"
#include "core/EmitSmokeScript.h"
// Exact player setters are explicit test dependencies, not a production
// replacement for player motion. Reject unimplemented child services loudly.
struct RegisteredStompPlayer {
    unsigned recharges=0,spins=0,blasts=0;int style=0,expectedX=0,expectedY=0;
    downwell::MotionBinary64 vertical{};bool grounded=true;
    int playStyle(){return style;}
    void setSpinJumping(){++spins;}
    bool recharge(){++recharges;return true;}
    bool setGrounded(bool value){grounded=value;return true;}
    bool setHorizontal(downwell::MotionBinary64){std::abort();}
    bool setVertical(downwell::MotionBinary64 value){vertical=value;return true;}
    bool createBlast(int id,int x,int y){
        SkeletonSoundAudio::check(id==276&&x==expectedX&&y==expectedY);++blasts;return false;
    }
    unsigned choose(unsigned){std::abort();}
    downwell::MotionBinary64 playerX(){std::abort();}
    downwell::MotionBinary64 playerY(){std::abort();}
    bool emitSmoke(downwell::MotionBinary64,downwell::MotionBinary64,int,int){std::abort();}
};
template<class World,class Invoke>
void checkRegisteredSkeletonStomp(World& world,downwell::SkeletonActor& actor,bool knees,int style,Invoke invoke){
    using namespace downwell;
    RegisteredStompPlayer player;player.style=style;player.expectedX=actor.x;player.expectedY=actor.y;
    SkeletonSoundAudio audio;SoundPlayEnvironment env;env.gameVolume={0x3fd00000u,0};
    bool good=true;const auto draws=world.randomDrawCount();
    SkeletonSoundAudio::check(invoke(player,env,audio,good)==!knees&&good==!knees&&
        world.effectsSucceeded()==!knees&&actor.myFx&&actor.health==-40&&actor.stomped&&actor.hitPending&&
        world.randomDrawCount()==draws+2&&world.cameraShakeAmount()==2&&
        player.recharges==2&&player.spins==1&&!player.grounded&&player.blasts==unsigned(knees)&&
        player.vertical.high==(style==2?0xbff80000u:0xc0040000u)&&!player.vertical.low&&
        actor.damageScripts.shake.shakeAmt==2&&actor.damageScripts.shake.shakeDur==10&&
        audio.calls==7&&audio.ids[0]==0&&audio.ids[2]==122&&world.overlapSound(0)==122&&
        actor.sound.sfxIndex==122&&actor.sound.chanNum==0&&actor.sound.cutPrv);
    if(knees){const auto after=world.randomDrawCount();
        SkeletonSoundAudio::check(!invoke(player,env,audio,good)&&world.randomDrawCount()==after&&audio.calls==7);}
}
template<class World,class Emit,class Find>
void checkRegisteredSmokeScript(World& world,Emit emit,Find find){
    using namespace downwell;
    auto random=world.randomState();EmitSmokeScript caller;caller.mySmoke=777;
    SkeletonSoundAudio::check(world.chooseSkeletonExplosion(3)==random.chooseIndexSmall(3));
    const int directions[]={0,180,15,165,42};
    const MotionBinary64 x{0x40248000u,0},y{0x40388000u,0}; // 10.25,24.5
    for(unsigned i=0;i<5;++i){
        SmokeEffect reference;SkeletonSoundAudio::check(reference.create(x,y,random));
        const auto direction=MotionBinary64::fromInteger(directions[i]);
        const auto speed=i==4?MotionBinary64{0x7ff80000u,0}:MotionBinary64::fromInteger(i<2?4:3);
        const auto before=caller.mySmoke;
        SkeletonSoundAudio::check(emit(caller,x,y,direction,speed)==(i<4)&&caller.mySmoke!=before);
        const auto* actual=find(caller.mySmoke);
        SkeletonSoundAudio::check(actual&&actual->direction.high==direction.high&&actual->direction.low==direction.low&&
            actual->speed.high==(i<4?speed:reference.speed).high&&actual->speed.low==(i<4?speed:reference.speed).low&&
            actual->imageSpeed.bits==reference.imageSpeed.bits&&!actual->allSet&&actual->x.bits==reference.x.bits&&
            actual->y.bits==reference.y.bits&&world.randomDrawCount()==random.drawCount());
        for(unsigned n=0;n<16;++n)SkeletonSoundAudio::check(world.randomState().stateWord(n)==random.stateWord(n));
    }
    const auto id=caller.mySmoke;const auto draws=world.randomDrawCount();
    SkeletonSoundAudio::check(!emit(caller,x,y,MotionBinary64{},MotionBinary64{})&&caller.mySmoke==id&&world.randomDrawCount()==draws);
}
#endif
