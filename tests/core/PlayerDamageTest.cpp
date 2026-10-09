#include "core/RunSession.h"
#include "core/RunControllerInstances.h"
#include "core/PlayerLifeRules.h"
#include "core/BoneProjectile.h"
#include "core/SkeletonActor.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
static auto n(int v){return MotionBinary64::fromInteger(v);}
static bool eq(MotionBinary64 a,MotionBinary64 b){return a.high==b.high && a.low==b.low;}
struct Events {
    std::vector<int> log;
    bool candle=false,dead=false,damaged=false,hit=false,mutate=false;
    int health=4,initialHealth=4,roomSpeed=60,alarm=-1,controllers=2,fail=0;
    int slow[2]{},controllerAlarm[2]{},x=250,y=500;
    bool record(int v){log.push_back(v);return v!=fail;}
    bool setDamageSourceName(){return record(1);}
    bool candleOwned(){record(2);return candle;}
    bool playerDead(){record(3);return dead;}
    bool playerDamaged(){record(4);return damaged;}
    bool subtractHealth(MotionBinary64 amount){assert(eq(amount,n(1)));--health;return record(5);}
    auto playerY(){record(6);return n(y);}
    auto playerX(){record(7);return n(x);}
    auto callerY(){record(8);return n(490);}
    auto callerX(){record(9);return n(240);}
    bool pointDirection(MotionBinary64 cx,MotionBinary64 cy,MotionBinary64 px,MotionBinary64 py,MotionBinary64& out){
        assert(eq(cx,n(240))&&eq(cy,n(490))&&eq(px,n(250))&&eq(py,n(500)));
        out=n(315);return record(10);
    }
    bool damageNumber(MotionBinary64 amount,MotionBinary64 direction){
        assert(health==initialHealth-1 && eq(amount,n(1)) && eq(direction,n(315)));
        if(mutate){candle=!candle;dead=true;damaged=true;x=900;y=901;}
        return record(11);
    }
    bool sound(int id,int priority,int loop,int variation){
        assert(id==5 && priority==90 && loop==0 && variation==0);return record(12);
    }
    bool duckMusic(int ms,MotionBinary64 gain){assert(ms==500 && eq(gain,{0x3fe66666u,0x66666666u}));return record(13);}
    void setHit(){hit=true;record(14);}
    struct Controller {
        Events& events;int index;
        void setSlow(int value){assert(events.roomSpeed==40 && value==23);events.slow[index]=value;events.record(17);}
        bool setAlarm(int index,int value){assert(index==0 && value==40 && events.slow[this->index]==23);
            events.controllerAlarm[this->index]=value;return events.record(18);}
    };
    template<class Visit> bool forEachController(Visit visit){
        if(!record(15))return false;
        for(int i=0;i<controllers;++i){Controller controller{*this,i};if(!visit(controller))return false;}
        return true;
    }
    void setRoomSpeed(int value){assert(value==40);roomSpeed=value;record(16);}
    void setPlayerDamaged(){damaged=true;record(19);}
    bool setPlayerAlarm(int index,int value){assert(index==1 && damaged);alarm=value;return record(20);}
};
struct OwnedControllerEvents:Events {
    RunControllerInstances<2>* owner=nullptr;InstanceHandle first{},second{};
    template<class Visit> bool forEachController(Visit visit){
        return owner->visitDamageControllers([&](auto apply){return apply(second)&&apply(first);},visit);
    }
};
struct DamageControllerOwner {
    bool birth(InstanceHandle,const RunControllerInstances<2>::Record&){return true;}
    bool birthAt(InstanceHandle,const RunControllerInstances<2>::Record&,RunnerImageValue,RunnerImageValue){return true;}
    bool retire(InstanceHandle,const RunControllerInstances<2>::Record&){return true;}
    bool activation(InstanceHandle,const RunControllerInstances<2>::Record&,bool){return true;}
    bool membership(InstanceHandle,const RunControllerInstances<2>::Record&,bool){return true;}
};
int main(){
    {
        RunSession run;
        assert(!run.playerHit);run.playerDamageEndStep();assert(!run.playerHit);
        run.setPlayerHit();run.playerCreated();assert(run.playerHit==1);
        run.playerDamageEndStep();assert(run.playerHit==2);
        run.setPlayerHit();assert(run.playerHit==1); // A new hit restarts the marker.
        run.playerDamageEndStep();run.playerDamageEndStep();assert(!run.playerHit);
        assert(run.noDamage&&run.areaNoDamage);
        run.noDamage=run.areaNoDamage=false;
        run.reset();assert(run.noDamage&&!run.areaNoDamage);
        run.playerCreated();assert(run.noDamage&&run.areaNoDamage);
    }
    for(unsigned failure=0;failure<=8;++failure){
        const auto check=[&](auto& actor){
            RunSession world;world.stage.area=1;world.backgroundMusicId=193;
            Events events;SoundPlayEnvironment environment;environment.gameVolume={0x3fe00000u,0};environment.inWater=true;
            struct Audio {
                unsigned calls=0,failure;
                bool next(){return ++calls!=failure;}
                bool play(int id,int priority,bool loop,int& voice){assert(id==5&&priority==90&&!loop);voice=700;return next();}
                bool getGain(int id,MotionBinary64& gain){assert(id==(calls==1?700:193));gain=n(1);return next();}
                bool setGain(int id,MotionBinary64 gain,int duration){
                    if(calls==2)assert(id==700&&eq(gain,{0x3fe00000u,0})&&duration==0);
                    else if(calls==6)assert(id==193&&eq(gain,{0x3fe66666u,0x66666666u})&&duration==0);
                    else assert(calls==7&&id==193&&eq(gain,n(1))&&duration==500);
                    return next();
                }
                bool setPitch(int id,MotionBinary64 pitch){assert(id==700&&eq(pitch,{0x3fe19999u,0x9999999au}));return next();}
                bool isPlaying(int id,bool& result){assert(id==193);result=true;return next();}
                bool stop(int){assert(false);return false;} // Damage does not cut prior sounds.
            } audio{0,failure};
            actor.sound.OLnum="enemymove";
            ContactDamageAudio<RunSession,Audio> service{world,actor.damageScripts.music,actor.sound,environment,audio};
            const bool okay=actor.damageScripts.player.executeWithServices(n(1),events,events,service);
            assert(okay==(failure==0)&&audio.calls==(failure?failure:8));
            assert(actor.sound.sfxIndex==5&&actor.sound.soundPriority==90&&!actor.sound.cutPrv&&!actor.sound.loop);
            assert(actor.sound.OLnum&&actor.sound.OLnum[0]=='e'); // Plain playback does not reset overlap fields.
            assert(events.health==3&&events.hit==(failure==0)&&events.damaged==(failure==0));
            assert(actor.damageScripts.music.duckDuration==(failure&&failure<=4?0:500));
        };
        BoneProjectile bone;check(bone);SkeletonActor skeleton;check(skeleton);
    }
    int health=4;
    assert(subtractPlayerHealth(health,n(1))&&health==3);
    assert(subtractPlayerHealth(health,n(4))&&health==-1); // No clamp at zero.
    assert(!subtractPlayerHealth(health,{0x3fe00000u,0})&&health==-1); // No fractional rounding.
    assert(subtractPlayerHealth(health,n(-2))&&health==1);
    for(bool inactive:{false,true}){
        DamageControllerOwner binding;
        RunControllerInstances<2> owner(RunControllerInstances<2>::Binding::bindPositionedLifetime(binding));
        OwnedControllerEvents events;events.owner=&owner;
        assert(owner.create(100001,{}, {},[](auto){return true;},events.first));
        assert(owner.create(100002,{}, {},[](auto){return true;},events.second));
        assert(owner.activate(events.second,!inactive));
        PlayerDamage damage;
        assert(damage.execute(n(1),events)&&events.health==3&&events.roomSpeed==40&&events.alarm==90);
        assert(owner.find(events.first)->controllerSlow==23&&owner.find(events.first)->alarms.get(0)==40);
        assert(owner.find(events.second)->controllerSlow==(inactive?0:23)&&
            owner.find(events.second)->alarms.get(0)==(inactive?-1:40));
        for(unsigned i=0;i<40;++i)assert(owner.advanceAlarm(events.first,0,events.roomSpeed));
        assert(events.roomSpeed==60&&owner.find(events.first)->controllerSlow==0);
    }
    {
        DamageControllerOwner binding;
        RunControllerInstances<2> owner(RunControllerInstances<2>::Binding::bindPositionedLifetime(binding));InstanceHandle key;
        assert(owner.create(100001,{}, {},[](auto){return true;},key));
        assert(!owner.visitDamageControllers([&](auto visit){return visit(key);},[&](auto& controller){
            controller.setSlow(23);assert(owner.find(key)->controllerSlow==23);
            assert(owner.clear()); // Lifetime changes between the two source writes.
            return controller.setAlarm(0,40);
        })&&!owner.valid());
    }
    const std::vector<int> full{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,16,17,18,19,20};
    for(unsigned flags=0;flags<8;++flags){
        RunSession run;PlayerDamage caller;Events events;
        const bool candle=flags&1,dead=flags&2,damaged=flags&4;
        events.candle=candle;events.dead=dead;events.damaged=damaged;
        assert(run.damagePlayer(caller,n(1),events));
        assert(caller.hitInvulnerability==(candle?240:90) && events.alarm==caller.hitInvulnerability && events.damaged);
        if(dead || damaged){
            assert(events.health==4 && !events.hit && events.roomSpeed==60);
            assert(events.log==(dead?std::vector<int>{1,2,3,19,20}:std::vector<int>{1,2,3,4,19,20}));
        }else{
            assert(events.health==3 && events.hit && events.log==full);
            assert(events.slow[0]==23 && events.slow[1]==23 && events.controllerAlarm[0]==40 && events.controllerAlarm[1]==40);
        }
        events.log.clear();
        assert(run.damagePlayer(caller,n(1),events) && events.health==(dead||damaged?4:3));
        assert(events.log==(dead?std::vector<int>{1,2,3,19,20}:std::vector<int>{1,2,3,4,19,20}));
    }
    for(bool candle:{false,true}){
        RunSession run;PlayerDamage caller;Events events;events.mutate=true;events.candle=candle;
        assert(run.damagePlayer(caller,n(1),events) && events.log==full);
        assert(events.dead && events.damaged && events.hit && events.candle!=candle);
        assert(events.alarm==(candle?240:90)); // Chosen before synchronous effects.
    }
    for(int health:{1,0,-2}){
        RunSession run;PlayerDamage caller;Events events;events.health=events.initialHealth=health;
        assert(run.damagePlayer(caller,n(1),events));
        assert(events.health==health-1 && events.log==full && !events.dead);
        // Health is not an entry guard and this script does not publish death.
    }
    for(int fail:{1,5,10,11,12,13,15,18,20}){
        RunSession run;PlayerDamage caller;Events events;events.fail=fail;
        assert(!run.damagePlayer(caller,n(1),events) && !run.effectsSucceeded && events.log.back()==fail);
        auto log=events.log;
        assert(!run.damagePlayer(caller,n(1),events) && events.log==log);
        assert(events.health==(fail==1?4:3)); // Failure cannot roll back health.
        if(fail==1)assert(caller.hitInvulnerability==0);
    }
    {
        RunSession run;PlayerDamage caller;Events events;events.controllers=0;
        assert(run.damagePlayer(caller,n(1),events));
        assert(events.health==3 && events.hit && events.damaged && events.roomSpeed==60 && events.alarm==90);
    }
    std::puts("Player damage: health/immunity/death gates, caller state, controller ordering, synchronous mutation and failure passed");
}
