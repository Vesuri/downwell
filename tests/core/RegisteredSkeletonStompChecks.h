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
    SkeletonSoundAudio audio;SoundPlayEnvironment env;env.gameVolume=downwell::AudioLevel::raw(16384);
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
struct BoundStompAudio {
    unsigned calls=0,failAt=0;int explosion=0;
    bool call(){return ++calls!=failAt;}
    bool isPlaying(int,bool& playing){playing=true;return call();}
    bool stop(int){return call();}
    bool play(int id,int priority,bool loop,int& voice){
        SkeletonSoundAudio::check(!loop&&((id==122&&priority==50)||(id==explosion&&priority==60)));
        voice=1001;return call();
    }
    bool getGain(int,downwell::AudioLevel& gain){gain=downwell::audio_level::one;return call();}
    bool setGain(int,downwell::AudioLevel value,int time){
        SkeletonSoundAudio::check(value==downwell::AudioLevel::raw(16384)&&!time);return call();
    }
    bool setPitch(int,downwell::AudioLevel value){
        SkeletonSoundAudio::check(value==downwell::audio_level::one);return call();
    }
};
template<class World,class Invoke,class FindBlast,class FindSmoke>
void checkBoundExplodingStomp(World& world,downwell::SkeletonActor& actor,int style,unsigned firstId,
    bool rejectSound,Invoke invoke,FindBlast findBlast,FindSmoke findSmoke){
    using namespace downwell;
    struct Player:RegisteredStompPlayer {
        MotionBinary64 playerX(){return {0x406e0800u,0};} // 240.25
        MotionBinary64 playerY(){return {0x40691000u,0};} // 200.5
    } player;player.style=style;
    auto random=world.randomState();random.nextU32();random.nextU32(); // one blood-angle draw per child
    const unsigned flip=random.chooseIndexSmall(2);
    BoundStompAudio audio;audio.explosion=91+random.chooseIndexSmall(3);audio.failAt=rejectSound?8:0;
    SmokeEffect expected[4];
    if(!rejectSound)for(auto& effect:expected)
        SkeletonSoundAudio::check(effect.create(player.playerX(),{0x40699000u,0},random)); // y+4
    SoundPlayEnvironment env;env.gameVolume=downwell::AudioLevel::raw(16384);bool good=true;
    SkeletonSoundAudio::check(invoke(player,env,audio,good)==!rejectSound&&good==!rejectSound&&
        world.effectsSucceeded()==!rejectSound&&actor.health==-40&&actor.stomped&&actor.hitPending&&
        player.recharges==2&&player.spins==1&&!player.grounded&&!player.blasts&&
        player.vertical.high==(style==2?0xbff80000u:0xc0040000u)&&!player.vertical.low&&
        world.cameraShakeAmount()==6&&audio.calls==(rejectSound?8u:13u)&&
        actor.sound.sfxIndex==audio.explosion&&world.overlapSound(0)==122);
    const auto* blast=findBlast(firstId+2);
    SkeletonSoundAudio::check(blast&&blast->state.xscale.bits==(flip?0xbf800000u:0x3f800000u)&&
        blast->state.imageSpeed.bits==0x3f000000u&&blast->state.shake.shakeAmt==6);
    const int directions[]={0,180,15,165};
    for(unsigned i=0;i<4;++i){const auto* smoke=findSmoke(firstId+3+i);
        if(rejectSound){SkeletonSoundAudio::check(!smoke);continue;}
        const auto direction=MotionBinary64::fromInteger(directions[i]);
        const auto speed=MotionBinary64::fromInteger(i<2?4:3);
        SkeletonSoundAudio::check(smoke&&smoke->x.bits==expected[i].x.bits&&smoke->y.bits==expected[i].y.bits&&
            smoke->imageSpeed.bits==expected[i].imageSpeed.bits&&smoke->direction.high==direction.high&&
            smoke->direction.low==direction.low&&smoke->speed.high==speed.high&&smoke->speed.low==speed.low);
    }
    SkeletonSoundAudio::check(world.randomDrawCount()==random.drawCount());
    for(unsigned i=0;i<16;++i)SkeletonSoundAudio::check(world.randomState().stateWord(i)==random.stateWord(i));
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
