#ifndef DOWNWELL_GAMEPLAY_FIELD_REENTRY_VERIFICATION_H
#define DOWNWELL_GAMEPLAY_FIELD_REENTRY_VERIFICATION_H
#include "GameplayCollisionOwner.h"
#include "SoundPlayVerification.h"
#include "BoxSoundVerification.h"
#include "CurrencyCreateVerification.h"
#include "CurrencySpawnVerification.h"
#include "DroneBulletVerification.h"
namespace downwell {
// Test-only lifecycle tape, shared by host tests and the 68000 debugger gate.
// The exit is deliberately placed at the player; this is not evidence for a
// naturally generated level, drawing parity, or complete collision selection.
// Run before terrain/scene binding. Keep this workspace off the native stack.
struct GameplayFieldReentryVerification {
    struct Builder {
        AreaOneBuilder state;
        void resetRoom(int x,int y,GameMakerRandom& random,const AreaOneBuilder::Setup& setup){state.reset(x,y,random,setup);}
    } builder;
    unsigned checks=0,stage=0,gameSteps=0,clearSteps=0;
    unsigned firstGeneration=0,laterGeneration=0,retryGeneration=0;
    TimeChangeEffects<8> timeChanges;
    int sourceRoomSpeed=60;
    bool run(GameplayWorld& world,GameplayCollisionOwner& owner,const ObjectPack& objects,unsigned origin){
        checks=0;gameSteps=clearSteps=0;
        unsigned population=0;
        stage=1;
        if(!SoundPlayVerification::run())return false;
        if(!CurrencyCreateVerification::run())return false;
        if(!CurrencySpawnVerification::run())return false;
        if(!DroneBulletVerification::run())return false;
        // Controlled explicit source IDs exercise registration; this does not
        // supply the unrecovered global birth stream of a natural run.
        if(!owner.createTimeChangeWithSourceId(timeChanges,105187,
            RunnerImageValue{0x43704000u}.promoted(),RunnerImageValue{0x43308000u}.promoted(),
            sourceRoomSpeed,[&](unsigned ms){return ms==30&&sourceRoomSpeed==30;}))return false;
        const auto timeKey=owner.timeChanges().handle(timeChanges.beginPhase().newest);
        InstanceHandle sharedTime;
        if(!GameplayCollisionView::encode(GameplayCollisionView::Kind::timeChange,timeKey,sharedTime)||
            !owner.state(sharedTime)||owner.state(sharedTime)->x.bits!=0x43704000u||
            !owner.population(211,population)||population!=1)return false;
        if(!owner.timeChanges().activation(timeKey,false)||!(owner.state(sharedTime)->flags&2u)||
            !owner.reconcileActivation())return false;
        for(unsigned tick=0;tick<7;++tick)for(unsigned index=0;index<2;++index)
            if(!world.advanceTimeChangeRoomAlarm(timeKey,index))return false;
        const auto* suspended=owner.timeChanges().find(timeKey);
        if(!suspended||suspended->alarms.get(0)!=2||suspended->alarms.get(1)!=2||sourceRoomSpeed!=30||
            !owner.timeChanges().activation(timeKey,true)||(owner.state(sharedTime)->flags&2u)||
            !owner.reconcileActivation())return false;
        for(unsigned tick=1;tick<=14;++tick){
            for(unsigned index=0;index<2;++index)
                if(!world.advanceTimeChangeRoomAlarm(timeKey,index))return false;
            if(sourceRoomSpeed!=int(tick<12?30+5*(tick/2):60))return false;
        }
        if(timeChanges.size()||owner.timeChanges().size()!=1||!owner.prepareDraw()||
            owner.timeChanges().size()||owner.state(sharedTime))return false;
        if(!world.effectsSucceeded()||!owner.valid()||world.runStage().area!=1||world.runStage().level!=1||
            !owner.population(255,population)||population)return false;
        firstGeneration=world.goalFieldInstances().birthSequence();
        const auto draws=world.randomDrawCount();
        if(!world.createRoomStartField()||world.startTimeField()||world.randomDrawCount()!=draws+1||
            world.goalFieldInstances().find({0,firstGeneration})||!owner.population(371,population)||population)return false;
        checks|=1;

        stage=2;
        if(!world.registerLevelExit(176+world.player().offsetY())||!owner.population(255,population)||population!=1)return false;
        const auto trigger=world.exitControllers().handle(0),setter=world.exitControllers().handle(1);
        InstanceHandle triggerKey,setterKey;
        if(!GameplayCollisionView::encode(GameplayCollisionView::Kind::exitController,trigger,triggerKey)||
            !GameplayCollisionView::encode(GameplayCollisionView::Kind::exitController,setter,setterKey)||
            !owner.state(triggerKey)||!owner.state(setterKey)||
            !owner.population(306,population)||population!=1||!owner.population(74,population)||population!=1)return false;
        for(unsigned i=0;i<100;++i){world.step(0);++gameSteps;if(!world.effectsSucceeded())return false;}
        if(!world.levelResultRequested()||!world.goalStopped())return false;
        checks|=2;

        stage=3;
        if(!world.enterLevelClear(0,false)||!owner.population(255,population)||population)return false;
        if(owner.state(triggerKey)||owner.state(setterKey)||world.exitControllers().find(trigger)||
            world.exitControllers().find(setter)||!owner.population(306,population)||population||
            !owner.population(74,population)||population)return false;
        bool heart=false;
        while(clearSteps<400&&!world.nextLevelRequested()){
            bool confirm=true;++clearSteps;
            if(!world.stepLevelClear(confirm,false,false,heart,[](auto,unsigned){}))return false;
        }
        if(!world.nextLevelRequested())return false;
        // Exercise nonpersistent effect teardown on the target too, including
        // a destroy-marked particle whose ordinary Draw cleanup has not run.
        InstanceHandle liveDebris,markedDebris,liveKey,markedKey;
        if(!world.spawnDebris({},{},1,liveDebris)||!world.spawnDebris({},{},1,markedDebris)||
            !GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,liveDebris,liveKey)||
            !GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,markedDebris,markedKey))return false;
        for(unsigned i=0;i<30;++i)
            if(!world.advanceOwnedDebrisRoomAlarm(markedDebris,world.debrisEffects().beginPhase(),0))return false;
        const auto* marked=world.debrisEffects().retained(markedDebris.generation);
        if(!marked||!marked->destroyRequested||!owner.state(liveKey)||!owner.state(markedKey))return false;
        const auto debrisNext=world.debrisEffects().birthSequence().next;
        const auto transitionDraws=world.randomDrawCount();
        if(!owner.createTimeChangeWithSourceId(timeChanges,105188,{},{},sourceRoomSpeed,
            [](unsigned ms){return ms==30;}))return false;
        const auto transitionTime=owner.timeChanges().handle(timeChanges.beginPhase().newest);
        if(!world.prepareNextAreaOneLevel({},origin)||world.runStage().level!=2||
            timeChanges.size()||owner.timeChanges().size()||owner.timeChanges().find(transitionTime)||
            world.debrisInstances()->size()||world.debrisEffects().retained(liveDebris.generation)||
            world.debrisEffects().retained(markedDebris.generation)||owner.state(liveKey)||owner.state(markedKey)||
            world.debrisEffects().birthSequence().next!=debrisNext||world.randomDrawCount()!=transitionDraws||
            !owner.population(370,population)||population)return false;
        checks|=4;

        stage=4;
        if(!world.initializeAreaOneBuilder(builder,-16,704)||!world.startTimeField()||
            !owner.population(371,population)||population!=1||!owner.population(255,population)||population!=1)return false;
        const auto local=world.goalFieldInstances().handle(0);
        laterGeneration=local.generation;
        InstanceHandle key;
        if(laterGeneration<=firstGeneration||!GameplayCollisionView::encode(GameplayCollisionView::Kind::goalField,local,key))return false;
        const auto* state=owner.state(key);const auto* properties=owner.properties(key);
        PackedObject definition;
        if(!objects.findObject(371,definition)||!state||!properties||(state->flags&0x100003u)||
            state->x.bits!=0x43700000u||state->y.bits!=0x43400000u||
            properties->sprite.spriteId!=definition.spriteId||properties->sprite.maskId!=definition.textureMaskId||
            properties->sprite.scaleX.bits!=0x40000000u||properties->sprite.scaleY.bits!=0x40000000u)return false;
        checks|=8;

        stage=5;
        const int boxX=64,boxY=1200;
        const InstanceHandle box{0,1};
        if(!world.appendBreakables(&boxX,&boxY,1)||!world.publishBoxAlarm0(box,2))return false;
        const auto* liveBox=world.findBreakable(box);
        if(!liveBox||!BoxSoundVerification::run(world,box,liveBox->state.breakSound,
            [&](){return &world.findBreakable(box)->sound;}))return false;
        for(unsigned i=0;i<8;++i){
            world.step(0);++gameSteps;if(!world.effectsSucceeded())return false;
            // Controlled frozen-instance input, not a substitute for the live
            // player's still-unconnected time-stop transition/effect owner.
            if(i<2&&(!world.stopBoxAlarm0(box,[](bool& allowed){allowed=false;return true;})||
                world.findBreakable(box)->alarm0!=2))return false;
        }
        liveBox=world.findBreakable(box);
        // Alarm expires on the fourth tick (the first two were refunded).
        // Its ordinary Step marks destruction; subsequent ticks exclude it.
        if(!liveBox||liveBox->alarm0!=0||!liveBox->state.wallHp.zero()||!liveBox->active||!liveBox->destroyRequested)return false;
        InstanceHandle sharedBox;BoxDestructionScript::Caller caller;
        if(!GameplayCollisionView::encode(GameplayCollisionView::Kind::breakable,box,sharedBox)||
            !owner.readBoxDestruction(sharedBox,caller)||!caller.state.wallHp.zero()||
            caller.x.high!=MotionBinary64::fromInteger(boxX+240).high||
            caller.y.high!=MotionBinary64::fromInteger(boxY+176).high)return false;
        if(world.breakableActiveAt(boxX,boxY)||!world.markBreakableDestroyed(box)||
            world.breakableActiveAt(boxX,boxY)||!world.findBreakable(box)||
            !owner.prepareDraw()||world.findBreakable(box)||owner.readBoxDestruction(sharedBox,caller))return false;
        if(!world.goalFieldInstances().find(local)||!owner.state(key)||!world.startTimeField()||
            !owner.population(371,population)||population!=1)return false;
        checks|=16;

        stage=6;
        if(world.restartFromResults(DeathMenuSequence::Action::Retry,2,origin)!=ResultsDestination::gameplay||
            !world.effectsSucceeded()||!owner.valid()||world.goalFieldInstances().find(local)||owner.state(key)||
            !owner.population(255,population)||population)return false;
        if(world.overlapSound(7)!=0)return false;
        checks|=32;

        stage=7;
        retryGeneration=world.goalFieldInstances().birthSequence();
        if(retryGeneration<=laterGeneration||!world.createRoomStartField()||world.startTimeField()||
            world.goalFieldInstances().find({0,retryGeneration})||owner.state(key)||
            !owner.population(371,population)||population)return false;
        checks|=64;

        stage=8;
        if(world.restartFromResults(DeathMenuSequence::Action::Retry,2,origin)!=ResultsDestination::gameplay||
            !world.effectsSucceeded()||!owner.valid()||world.tick()!=0||world.health()!=4||
            world.runStage().area!=1||world.runStage().level!=1||!owner.population(255,population)||population)return false;
        checks|=128;return true;
    }
};
}
#endif
