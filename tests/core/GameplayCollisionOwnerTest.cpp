#include "core/GameplayCollisionOwner.h"
#include "TimeoutDamageEvents.h"
#include "core/Replay.h"
#include "core/CurrencySmokeComposition.h"
#include "QuietSkeletonEffects.h"
#ifndef DOWNWELL_NATIVE_QUERY_PROBE
#include "SkeletonSoundChecks.h"
#include "RegisteredSkeletonStompChecks.h"
#include "BlastSkeletonChecks.h"
#include "BulletWallSoundChecks.h"
#include "SourceBulletSchedulingChecks.h"
#include "RegisteredSkeletonThrowChecks.h"
#include "ScheduledBoxEffectsCheck.h"
#include "GeneratedSourceBirthChecks.h"
#endif
#include "core/BoxWorldEffects.h"
#include "core/SpawnerStep.h"
#include "core/StoredSpawnerStates.h"
#include "core/WhiteJellyBirth.h"
#include "core/WhiteJellyStep.h"
#include "core/StoredWhiteJellyStates.h"
#ifdef DOWNWELL_LOCAL_DRAW_CAPTURE
namespace downwell { volatile unsigned localDrawCaptureMode=DOWNWELL_LOCAL_DRAW_CAPTURE; }
#endif
#include "core/RunnerBindingRevisionVerification.h"
#include "core/ActorDrawOrderVerification.h"
#include "core/MachineGunSourceStep.h"
#include "core/NativeSourceBulletStep.h"
#include "core/CurrencyHomingMath.h"
#include "core/TerrainWallContacts.h"
#ifdef DOWNWELL_NATIVE_QUERY_PROBE
struct NativeWhiteJellyBirthOps {
    bool position(bool,downwell::RunnerImageValue,downwell::RunnerImageValue&);
    bool playerY(downwell::MotionBinary64&);bool playerX(downwell::MotionBinary64&);
    bool pointDirection(downwell::MotionBinary64,downwell::MotionBinary64,downwell::MotionBinary64,
        downwell::MotionBinary64,downwell::MotionBinary64&);
    bool direction(downwell::MotionBinary64,downwell::MotionBinary64&);
    bool imageSpeed(downwell::RunnerImageValue,downwell::RunnerImageValue&);
};
bool nativeWhiteJellyBirth(downwell::WhiteJellyBirth& state,downwell::RunnerImageValue x,
    downwell::RunnerImageValue y,downwell::GameMakerRandom& random,NativeWhiteJellyBirthOps& ops){
    return state.create(x,y,random,ops);
}
struct NativeWhiteJellyStepOps : NativeWhiteJellyBirthOps {
    using N=downwell::MotionBinary64;
    bool imageIndex(int);bool sound(int,int,int,int,const char*);bool alarm(int,int);
    bool enemyPlayerY(N&);bool enemyPlayerX(N&);bool lengthdir(bool,N,N,N&);
    bool meeting(N,N,int,bool&);bool enemyDeath();bool smoke(N,N,int,int);
    bool movingFx(N,N,int,N,N,N);bool flash(N,N,int,int,int);bool currency(int);
    bool destroy();bool sprite(int);
    bool otherX(N&);bool otherY(N&);bool viewY(N&);bool room(int&);
    bool emitTimer(N&);bool effectAlarmReset(N);
};
bool nativeWhiteJellyStep(downwell::WhiteJellyStep& state,bool stop,
    downwell::GameMakerRandom& random,NativeWhiteJellyStepOps& ops){
    return state.step(stop,random,ops);
}
bool nativeWhiteJellyEvents(downwell::WhiteJellyStep& state,bool stop,
    downwell::GameMakerRandom& random,NativeWhiteJellyStepOps& ops){
    return state.enemyContact(random,ops)&&state.effectAlarm(stop,random,ops)&&state.endStep(ops);
}
bool nativeStoredWhiteJellyCreate(downwell::StoredWhiteJellyStates<8,2>& owner,
    downwell::InstanceHandle key,downwell::GameMakerRandom& random,NativeWhiteJellyStepOps& ops){
    return owner.create(key,random,ops);
}
bool nativeStoredWhiteJellyStep(downwell::StoredWhiteJellyStates<8,2>& owner,
    downwell::InstanceHandle key,bool stop,downwell::GameMakerRandom& random,NativeWhiteJellyStepOps& ops){
    return owner.step(key,stop,random,ops);
}
struct NativeSpawnerOps {
    bool playerY(downwell::MotionBinary64&);bool area(int&);bool fallDepth(downwell::MotionBinary64&);
    bool death(bool&);bool endReached(bool&);bool enemyCount(int&);bool objectMask(int,int&);bool mask(int);
    bool viewY(downwell::MotionBinary64&);bool viewHeight(downwell::MotionBinary64&);
    bool meeting(downwell::MotionBinary64,downwell::MotionBinary64,int,bool&);
    bool tile(int,downwell::MotionBinary64,downwell::MotionBinary64,bool&);
    bool create(int,downwell::MotionBinary64,downwell::MotionBinary64,unsigned&);bool deactivate(unsigned);
};
bool nativeSpawnerStep(downwell::SpawnerStep& state,downwell::MotionBinary64 y,
    downwell::GameMakerRandom& random,NativeSpawnerOps& ops){return state.step(y,random,ops);}
bool nativeSpawnerCreate(downwell::SpawnerStep& state,int level,bool hard){return state.createCaverns(level,hard);}
bool nativeStoredSpawnerCreate(downwell::StoredSpawnerStates<2,1>& states,downwell::InstanceHandle key,
    int level,bool hard){return states.createCaverns(key,level,hard);}
bool nativeStoredSpawnerStep(downwell::StoredSpawnerStates<2,1>& states,downwell::InstanceHandle key,
    downwell::GameMakerRandom& random,NativeSpawnerOps& ops){return states.step(key,random,ops);}
struct NativeScatteredEffects {
    bool outOfView();bool currency(int);bool attractGem();bool createDebris();
    bool debrisYSpeed(downwell::MotionBinary64);bool sound(int,int,int,int);bool sprite(int);
    bool playerYSpeed(downwell::MotionBinary64&);bool playStyle(int&);
    bool jump(int,downwell::MotionBinary64);bool shake(int,int);bool explodingKnees(bool&);bool createBlast();
};
bool nativeScatteredStep(downwell::GameplayScattered& store,downwell::InstanceHandle key,
    downwell::GameMakerRandom& random,NativeScatteredEffects& effects){return store.step(key,random,effects);}
bool nativeScatteredContact(downwell::GameplayScattered& store,downwell::InstanceHandle key,
    downwell::GameMakerRandom& random,NativeScatteredEffects& effects){return store.playerContact(key,random,effects);}
struct NativeShopBirthOwner {
    bool begin(downwell::ShopWorld::Instance&);
    bool play(int,int,bool,int&);bool setGain(int,downwell::MotionBinary64,int);
    unsigned create(int,int,int);
    bool readX(unsigned,int&);bool position(unsigned,int&,int&);
    bool upgrade(unsigned,unsigned);bool stored(unsigned,unsigned);
};
downwell::ShopWorld::Instance* nativeRegisteredShop(downwell::ShopWorld& shops,NativeShopBirthOwner& owner,
    unsigned (*range)(unsigned,unsigned),unsigned (*choice)(),bool& good){
    return shops.createRegistered(100,700,range,choice,4,1,false,0,false,owner,good);
}
bool nativeExitSourceBirth(GameplayWorld& world,int y,bool (*allocate)(int,unsigned&)){
    return world.registerLevelExitWithSourceIds(y,allocate);
}
bool nativeGeneratedFieldBirth(GameplayWorld& world,unsigned source,const downwell::DeferredCreateInstance& command){
    return world.adoptGeneratedTimeFieldWithSourceId(source,command);
}
bool nativeGeneratedEnemyBirth(GameplayWorld& world,unsigned source,const downwell::DeferredCreateInstance& command,
    const downwell::ObjectPack& objects){return world.adoptGeneratedEnemyWithSourceId(source,command,objects);}
bool nativeGeneratedSourceBirth(GameplayWorld& world,const downwell::DeferredCreateInstance& command,
    const downwell::ObjectPack& objects,bool (*allocate)(int,unsigned&),
    bool (*terrain)(const downwell::DeferredCreateInstance&,unsigned)){
    return world.createGeneratedInstanceWithSourceIds(command,objects,allocate,terrain,
        [](const auto&,auto&){return false;});
}
struct NativeSkeletonAudio {
    bool isPlaying(int,bool&);bool stop(int);bool play(int,int,bool,int&);
    bool getGain(int,downwell::MotionBinary64&);bool setGain(int,downwell::MotionBinary64,int);
    bool setPitch(int,downwell::MotionBinary64);
};
bool nativeSkeletonSound(GameplayWorld& world,downwell::SkeletonActor& actor,
    downwell::SoundPlayEnvironment& environment,NativeSkeletonAudio& audio){
    return world.playSkeletonSound(actor,133,environment,audio);
}
struct NativeSkeletonStompPlayer {
    bool playerYSpeed(downwell::MotionBinary64&);
    int playStyle();void setSpinJumping();bool recharge();bool setGrounded(bool);
    bool setHorizontal(downwell::MotionBinary64);bool setVertical(downwell::MotionBinary64);
    bool createBlast(int,int,int);unsigned choose(unsigned);
    downwell::MotionBinary64 playerX();downwell::MotionBinary64 playerY();
    bool emitSmoke(downwell::MotionBinary64,downwell::MotionBinary64,int,int);
};
bool nativeScatteredPlayerContact(downwell::GameplayCollisionOwner& owner,
    const downwell::RunnerCollisionEventContext::Frame& event,bool (*allocate)(unsigned&),bool (*delay)(),
    NativeSkeletonStompPlayer& player,downwell::SoundPlayEnvironment& environment,NativeSkeletonAudio& audio){
    return owner.dispatchScatteredPlayerCollision(event,allocate,delay,player,environment,audio);
}
bool nativeRegisteredStomp(GameplayWorld& world,downwell::SkeletonActor& actor,
    bool (*allocate)(unsigned&),NativeSkeletonStompPlayer& player,
    downwell::SoundPlayEnvironment& environment,NativeSkeletonAudio& audio,bool& good){
    return world.stompSkeleton(actor,allocate,player,environment,audio,good);
}
bool nativeBoundStomp(downwell::GameplayCollisionOwner& owner,downwell::SkeletonActor& actor,
    bool (*allocate)(unsigned&),NativeSkeletonStompPlayer& player,
    downwell::SoundPlayEnvironment& environment,NativeSkeletonAudio& audio,bool& good){
    return owner.stompSkeleton(actor,allocate,player,environment,audio,good);
}
bool nativeOwnedLevelCue(GameplayWorld& world,const downwell::TimeStopEffects<64>& stop,
    bool (*allocate)(unsigned&),const downwell::LevelNumberTextContext& text,
    downwell::SoundPlayState& caller,downwell::SoundPlayEnvironment& environment,NativeSkeletonAudio& audio){
    return world.beginLevelCueWithTitle(stop,{}, {},allocate,text,caller,environment,audio);
}
bool nativeSkeletonDeathBookkeeping(downwell::GameplayCollisionOwner& owner,downwell::SkeletonActor& actor,
    const downwell::TimeStopEffects<64>& stop,bool (*allocate)(unsigned&),
    const downwell::LevelNumberTextContext& text,downwell::SoundPlayEnvironment& environment,NativeSkeletonAudio& audio){
    return owner.recordSkeletonDeath(actor,true,stop,allocate,text,environment,audio);
}
bool nativeSkeletonBloodSmoke(GameplayWorld& world,downwell::SkeletonActor& actor,bool (*allocate)(unsigned&)){
    return world.emitSkeletonBlood(actor,allocate)&&world.emitSkeletonSmoke(actor,allocate)&&world.emitSkeletonFlash(actor,allocate);
}
struct NativeSkeletonRewardQueries {
    bool wall(downwell::InstanceHandle,downwell::MotionBinary64,downwell::MotionBinary64,bool&);
    bool tile(downwell::InstanceHandle,int,downwell::MotionBinary64,downwell::MotionBinary64,bool&);
};
bool nativeRegisteredScatteredStep(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle key,
    bool (*allocate)(std::uint32_t&),NativeSkeletonRewardQueries& queries,
    downwell::SoundPlayEnvironment& environment,NativeSkeletonAudio& audio){
    return owner.stepRegisteredScattered(key,11,downwell::MotionBinary64::fromInteger(600),allocate,queries,environment,audio);
}
bool nativeSkeletonReward(GameplayWorld& world,const downwell::ObjectPack& objects,
    downwell::SkeletonActor& actor,bool (*allocate)(std::uint32_t&),NativeSkeletonRewardQueries& queries){
    return world.emitSkeletonCurrency(objects,actor,allocate,queries);
}
struct NativeSkeletonCorpseTiles {
    bool operator()(downwell::InstanceHandle,int,downwell::MotionBinary64,downwell::MotionBinary64,bool&);
};
bool nativeCompleteSkeletonStep(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle key,
    const downwell::TimeStopEffects<64>& stop,bool (*allocate)(std::uint32_t&),NativeSkeletonCorpseTiles tile,
    NativeSkeletonRewardQueries& queries,const downwell::LevelNumberTextContext& text,
    downwell::SoundPlayEnvironment& environment,NativeSkeletonAudio& audio){
    return owner.stepRegisteredSkeleton(key,stop,true,allocate,tile,queries,text,environment,audio);
}
bool nativeScheduledSkeletonStep(GameplayWorld& world,downwell::GameplayCollisionOwner& owner,
    downwell::GameplayCollisionOwner::SkeletonBoneStepOrder& phase,
    const downwell::TimeStopEffects<64>& stop,bool (*allocate)(std::uint32_t&),NativeSkeletonCorpseTiles tile,
    NativeSkeletonRewardQueries& queries,const downwell::LevelNumberTextContext& text,
    downwell::SoundPlayEnvironment& environment,NativeSkeletonAudio& audio,bool (*frames)(int,unsigned&)){
    return owner.withSkeletonBoneSteps(phase,[&](downwell::InstanceHandle key){
        return owner.stepRegisteredSkeleton(key,stop,true,allocate,tile,queries,text,environment,audio);
    },[&](downwell::InstanceHandle key){return owner.stepBone(key,stop.player.stopped,frames);},
    [&]{world.step(0);return world.effectsSucceeded();});
}
bool nativeScheduledEncounterStep(GameplayWorld& world,downwell::GameplayCollisionOwner& owner,
    downwell::GameplayCollisionOwner::EncounterStepOrder& phase,
    bool (*execute)(downwell::InstanceHandle,int)){
    return owner.withEncounterSteps(phase,execute,[&]{world.step(0);return world.effectsSucceeded();});
}
bool nativeSkeletonCorpse(GameplayWorld& world,GameplayWorld::Skeletons::Publisher& publish,
    downwell::MotionBinary64 impact,bool (*allocate)(std::uint32_t&),NativeSkeletonCorpseTiles tile){
    return world.emitSkeletonCorpse(publish,impact,allocate,tile);
}
bool nativeSkeletonThrow(GameplayWorld& world,downwell::SkeletonActor& actor,downwell::RegisteredSkeletonThrow& caller,
    downwell::SoundPlayEnvironment& environment,NativeSkeletonAudio& audio){
    return world.throwSkeletonBoneWithSourceId(505201,actor,caller,environment,audio);
}
bool nativeSkeletonTimeState(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle key,
    const downwell::TimeStopEffects<64>& state,unsigned& hurts){
    return owner.stepSkeleton(key,state,[&](auto&,auto&){return QuietSkeletonEffects{hurts};});
}
bool nativeSkeletonEncounterContact(downwell::GameplayCollisionOwner& owner,
    const downwell::RunnerCollisionEventContext::Frame& event,const downwell::TimeStopEffects<64>& stop,
    downwell::SoundPlayEnvironment& env,NativeSkeletonAudio& audio,
    bool (*frames)(int,unsigned&),bool (*atan)(downwell::MotionBinary64,downwell::MotionBinary64,downwell::MotionBinary64&),
    bool (*damage)(downwell::BoneProjectile&,int,int,int),bool (*other)(const downwell::RunnerCollisionEventContext::Frame&),unsigned& stomps,unsigned& damages){
    return owner.dispatchSkeletonEncounterCollision(event,stop,frames,env,audio,atan,
        [&](auto& actor){return CheckedSkeletonContact{actor,0,stomps,damages};},damage,other);
}
bool nativeSkeletonEncounterBlastContact(downwell::GameplayCollisionOwner& owner,
    const downwell::RunnerCollisionEventContext::Frame& event,const downwell::TimeStopEffects<64>& stop,
    downwell::SoundPlayEnvironment& env,NativeSkeletonAudio& audio,bool (*allocate)(unsigned&),
    bool (*frames)(int,unsigned&),bool (*atan)(downwell::MotionBinary64,downwell::MotionBinary64,downwell::MotionBinary64&),
    bool (*damage)(downwell::BoneProjectile&,int,int,int),bool (*other)(const downwell::RunnerCollisionEventContext::Frame&),unsigned& stomps,unsigned& damages){
    return owner.dispatchSkeletonEncounterCollisionWithBlasts(event,stop,allocate,frames,env,audio,atan,
        [&](auto& actor){return CheckedSkeletonContact{actor,0,stomps,damages};},damage,other);
}
struct NativeLimboViews {
    bool outside(downwell::RunnerImageValue,downwell::RunnerImageValue,bool&);
    bool viewTop(downwell::MotionBinary64&);
};
bool nativeLimboProbe(downwell::GameplayCollisionOwner& owner,GameplayWorld& world,
    const downwell::SpriteMaskPack& masks,NativeLimboViews& views){
    downwell::InstanceHandle child;
    return world.createLimboShardWithSourceId(105193,{}, {},false,child)&&
        owner.stepLimboShard(child,true,480,{},masks,views);
}
struct NativeBulletVelocity {
    bool operator()(downwell::InstanceHandle,downwell::MotionBinary64&,downwell::MotionBinary64&);
};
struct NativeBulletSoundAudio {
    bool isPlaying(int,bool&);bool stop(int);bool play(int,int,bool,int&);
    bool getGain(int,downwell::MotionBinary64&);bool setGain(int,downwell::MotionBinary64,int);
    bool setPitch(int,downwell::MotionBinary64);
};
bool nativeBulletSound(downwell::GameplayCollisionOwner& owner,GameplayWorld& world,
    downwell::InstanceHandle shot,downwell::InstanceHandle wall,downwell::SoundPlayEnvironment& env,NativeBulletSoundAudio& audio){
    return world.playBulletWallSound(shot,env,[&](const char*& material){return owner.projectileWallMaterial(wall,material);},audio);
}
struct NativeBulletFrames {bool operator()(int,unsigned&);};
struct NativeBulletAtan {bool operator()(downwell::MotionBinary64,downwell::MotionBinary64,downwell::MotionBinary64&);};
bool nativeSkeletonBullet(downwell::GameplayCollisionOwner& owner,
    const downwell::RunnerCollisionEventContext::Frame& event,NativeBulletFrames frames,
    downwell::SoundPlayEnvironment& environment,NativeBulletSoundAudio& audio,NativeBulletAtan atan){
    return owner.dispatchSkeletonBulletCollision(event,frames,environment,audio,atan);
}
bool nativeBoundSourceBullet(GameplayWorld& world,downwell::GameplayCollisionOwner& owner,
    downwell::InstanceHandle shot,downwell::MachineGunSourceMotion& motion,const downwell::SpriteMaskPack& masks,
    NativeBulletFrames frames,downwell::SoundPlayEnvironment& environment,NativeBulletSoundAudio& audio){
    return downwell::NativeSourceBulletStep::run(world,owner,shot,motion,masks,frames,environment,audio);
}
struct NativeBulletSourceOps {
    bool publishAngle(downwell::RunnerImageValue);bool publishPosition(downwell::RunnerImageValue,downwell::RunnerImageValue);
    bool line(int,bool&);bool selectWall();bool approachSolid();bool wallParent(int&);
    bool wallHealth(downwell::MotionBinary64&);bool wallHealthWrite(downwell::MotionBinary64);bool wallHit();
    bool destroySelf();bool soundWall();bool explosion(int);bool placeShootable(int,bool&);bool selectShootable(int);
    bool thingHealth(downwell::MotionBinary64&);bool thingHealthWrite(downwell::MotionBinary64);bool thingHit();bool thingHitEffect(bool&);
};
bool nativeWorldSourceBullet(GameplayWorld& world,downwell::InstanceHandle shot,downwell::MachineGunSourceMotion& motion,
    NativeBulletSourceOps& ops,NativeBulletFrames frames){return world.stepSourceBullet(shot,motion,ops,frames);}
bool nativeScheduledSourceBullet(GameplayWorld& world,NativeBulletSourceOps& ops,NativeBulletFrames frames){
    return world.withSourceBulletSteps([&](downwell::InstanceHandle shot,downwell::MachineGunSourceMotion& motion){
        return world.stepSourceBullet(shot,motion,ops,frames);
    },[&]{world.step(0);return world.effectsSucceeded();});
}
bool nativeBulletEndStep(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle shot){
    return owner.endStepBullet(shot,{});
}
bool nativeBulletEffects(GameplayWorld& world,downwell::GameplayScriptSpawnInstances& registry,
    downwell::ScriptSpawnEffects<4>& effects,downwell::InstanceHandle shot,NativeBulletFrames frames){
    auto ops=world.bulletEffectOperations(shot,effects,registry,frames);
    return downwell::EffectSpawnScript::run(ops,{}, {},101,{0x3fe00000u,0},{},{});
}
bool nativeOwnedBulletEffects(GameplayWorld& world,downwell::InstanceHandle shot,NativeBulletFrames frames){
    return world.withBulletEffects(shot,frames,[&](auto& ops){
        return downwell::EffectSpawnScript::run(ops,{}, {},101,{0x3fe00000u,0},{},{});
    });
}
bool nativeLimboContact(downwell::GameplayCollisionOwner& owner,
    const downwell::RunnerCollisionEventContext::Frame& event,NativeBulletVelocity read){
    return owner.dispatchLimboShardBulletCollision(event,read);
}
bool nativeOwnedLimboContact(downwell::GameplayCollisionOwner& owner,
    const downwell::RunnerCollisionEventContext::Frame& event){
    return owner.dispatchLimboShardBulletCollision(event);
}
bool nativeBoneTarget(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle target,
    downwell::MotionBinary64 health){
    downwell::MotionBinary64 read;bool effect;
    return owner.projectileShootableFields(target,read,effect)&&owner.projectileShootableHealthWrite(target,health)&&
        owner.projectileShootableHit(target);
}
bool nativeShootableQuery(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle shot,
    const downwell::SpriteMaskPack& masks,downwell::GameplayCollisionOwner::Owner::Result& selected){
    bool hit;
    return owner.projectileShootableMeeting(shot,{}, {},masks,hit)&&
        owner.projectileShootableSelection(shot,{}, {},masks,selected);
}
struct NativeEnemyDeathAudioProbe { bool isPlaying(int,bool&);bool stop(int); };
bool nativeRunControllerBirth(downwell::GameplayCollisionOwner& owner,unsigned source,
    bool (*initialize)(downwell::InstanceHandle),downwell::InstanceHandle& key){
    return owner.runControllers().create(source,{}, {},initialize,key);
}
bool nativeMomentDelay(downwell::GameplayCollisionOwner& owner){return owner.requestMomentDelay();}
bool nativeBlastBirth(downwell::GameplayCollisionOwner& owner,unsigned source,downwell::InstanceHandle& key){
    return owner.createBlast(source,{}, {},key);
}
struct NativeBlastImpact {
    bool operator()(downwell::BlastActor<64>&,downwell::MotionBinary64,downwell::MotionBinary64,
        int,downwell::MotionBinary64,downwell::MotionBinary64,int);
};
bool nativeBlastContactWithEffects(downwell::GameplayCollisionOwner& owner,const downwell::RunnerCollisionEventContext::Frame& event,
    bool (*allocate)(unsigned&),bool (*frames)(int,unsigned&),downwell::SoundPlayEnvironment& environment,NativeSkeletonAudio& audio){
    return owner.dispatchBlastSkeletonCollisionWithEffects(event,allocate,frames,environment,audio);
}
bool nativeBlastContact(downwell::GameplayCollisionOwner& owner,const downwell::RunnerCollisionEventContext::Frame& event,
    NativeBlastImpact impact,downwell::SoundPlayEnvironment& environment,NativeSkeletonAudio& audio){
    return owner.dispatchBlastSkeletonCollision(event,impact,environment,audio);
}
struct NativeEnemyDeathScriptProbe { bool operator()(); };
bool nativeEnemyDeath(GameplayWorld& world,NativeEnemyDeathAudioProbe& audio,
    NativeEnemyDeathScriptProbe flag,NativeEnemyDeathScriptProbe delay,NativeEnemyDeathScriptProbe cue){
    return world.recordEnemyDeath(true,audio,flag,delay,cue);
}
bool nativeOwnedEnemyDeath(GameplayWorld& world,NativeEnemyDeathAudioProbe& audio,
    NativeEnemyDeathScriptProbe delay,NativeEnemyDeathScriptProbe cue){
    return world.recordEnemyDeath(true,audio,delay,cue);
}
struct NativeConsumptionGlobalsProbe {
    bool gravity(downwell::MotionBinary64);
    downwell::MotionBinary64 gravity();downwell::MotionBinary64 fallSpeed();
};
bool nativeConsumptionStep(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle shared,
    NativeConsumptionGlobalsProbe& globals){return owner.stepConsumptionBlood(shared,globals);}
bool nativeConsumptionContact(downwell::GameplayCollisionOwner& owner,const downwell::RunnerCollisionEventContext::Frame& event){
    return owner.dispatchConsumptionBloodCollision(event);
}
struct NativeCorpseStepProbe {
    int area();int restInPieces();
    bool create(int,downwell::MotionBinary64,downwell::MotionBinary64);
};
struct NativeCorpseContactProbe {
    bool incrementBodiesEaten();void eatSound(int,int,int,int);
    downwell::MotionBinary64 playerY();downwell::MotionBinary64 playerX();
    bool create(int,downwell::MotionBinary64,downwell::MotionBinary64,std::uint32_t&);
};
bool nativeCorpseContact(downwell::GameplayCollisionOwner& owner,const downwell::RunnerCollisionEventContext::Frame& event,
    NativeCorpseContactProbe& effects){return owner.dispatchCorpsePlayerCollision(event,true,false,effects);}
bool nativeCorpseStep(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle shared,
    bool stopped,int roomWidth,NativeCorpseStepProbe& events){
    return owner.stepCorpse(shared,stopped,roomWidth,events);
}
struct NativeCurrencyAudioProbe {
    bool isPlaying(int,bool&);bool stop(int);bool play(int,int,bool,int&);
    bool getGain(int,downwell::MotionBinary64&);bool setGain(int,downwell::MotionBinary64,int);
    bool setPitch(int,downwell::MotionBinary64);
};
struct NativeCurrencyGainProbe {
    using Handle=downwell::InstanceHandle;
    bool add(downwell::CurrencyGainScript::AccountField,int);bool currencyAtLeast(unsigned,bool&);
    bool achievement(int);bool resetTimer();bool extendTimer(unsigned);bool lessResist();bool resetControllers();
    bool gemPop();bool gemPowered();bool playerPosition(downwell::MotionBinary64&,downwell::MotionBinary64&);
    bool effect(downwell::MotionBinary64,downwell::MotionBinary64,int,downwell::MotionBinary64,int,int);
    bool create(int,downwell::MotionBinary64,downwell::MotionBinary64,Handle&);bool myBullet(Handle);
    bool bulletDirection(downwell::MotionBinary64);bool bulletSprite(int);bool bulletDamage(int);bool bulletSpeed(int);
    bool sound(int,int,bool,bool);bool soundOverlap(int,int,bool,bool,const char*);
    bool fxNoLoop(int,downwell::MotionBinary64);bool recharge();bool destroy();
};
bool nativeCurrencyGain(downwell::GameplayCollisionOwner& owner,const downwell::RunnerCollisionEventContext::Frame& event,
    NativeCurrencyGainProbe& ops){return owner.dispatchCurrencyCollision(event,ops);}
bool nativeCurrencyPickupAudio(GameplayWorld& world,downwell::InstanceHandle key,NativeCurrencyAudioProbe& audio){
    return world.playCurrencyPickupSound(key,downwell::MotionBinary64::fromInteger(1),audio);
}
bool nativeCurrencyStep(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle key,
    const downwell::SpriteMaskPack& masks,NativeCurrencyAudioProbe& audio){
    downwell::CurrencyHomingMath math{downwell::portableAtan};
    return owner.stepCurrency(key,true,480,downwell::MotionBinary64::fromInteger(240),
        downwell::MotionBinary64::fromInteger(512),masks,math,[](auto& top){top={};return true;},
        audio,downwell::MotionBinary64::fromInteger(1),false);
}
bool nativeCurrencyFrame(GameplayWorld& world,downwell::GameplayCollisionOwner& owner,
    const downwell::SpriteMaskPack& masks,NativeCurrencyAudioProbe& audio){
    return world.withCurrencyEvents([&](auto key,auto phase,unsigned alarm){
        if(phase==GameplayWorld::CurrencyPhase::alarm)
            return alarm==2&&owner.currency().expire(key,downwell::MotionBinary64::fromInteger(1),false,audio);
        downwell::InstanceHandle shared;
        return downwell::GameplayCollisionView::encode(downwell::GameplayCollisionView::Kind::currency,key,shared)&&
            nativeCurrencyStep(owner,shared,masks,audio);
    },[&](){world.step(0);return world.effectsSucceeded();});
}
bool nativeRegisteredTimeChange(downwell::GameplayCollisionOwner& owner,
    downwell::TimeChangeEffects<8>& effects,int& speed,std::uint32_t sourceId){
    return owner.createTimeChangeWithSourceId(effects,sourceId,
        downwell::RunnerImageValue{0x43704000u}.promoted(),
        downwell::RunnerImageValue{0x43308000u}.promoted(),speed,[](unsigned ms){return ms==30;});
}
bool nativeAllocatedTimeChange(GameplayWorld& world,downwell::GameplayCollisionOwner& owner,
    downwell::TimeChangeEffects<8>& effects,int& speed,std::uint32_t& next){
    return owner.withTimeEffectSourceIds([&](std::uint32_t& id){id=next++;return true;},[&]{
        return world.createTimeChange(effects,{}, {},speed,[](unsigned ms){return ms==30;});
    });
}
struct NativeTimeStopAudioProbe { bool operator()(downwell::PlayerTimeStop::AudioQuery,bool&); };
struct NativeTimeStopEmitProbe { bool operator()(downwell::PlayerTimeStop::Effect); };
bool nativePlayerTimeStop(GameplayWorld& world,downwell::GameplayCollisionOwner& owner,
    downwell::TimeStopEffects<64>& state,const downwell::PackedRoom& room,
    const downwell::SpriteMaskPack& masks,std::uint32_t& next,
    NativeTimeStopAudioProbe audio,NativeTimeStopEmitProbe emit){
    return owner.withPlayerTimeStop(state,room,masks,false,[&](std::uint32_t& id){id=next++;return true;},
        audio,emit,[&]{world.step(0);return world.effectsSucceeded();});
}
bool nativeFieldWind(downwell::GameplayCollisionOwner& owner,const downwell::RunnerCollisionEventContext::Frame& event,
    downwell::TimeFieldWind& wind,int& channel,NativeCurrencyAudioProbe& audio){
    return owner.dispatchTimeFieldWind(event,wind,channel,downwell::MotionBinary64::fromInteger(1),audio);
}
bool nativeSessionFieldWind(downwell::GameplayCollisionOwner& owner,const downwell::RunnerCollisionEventContext::Frame& event,
    NativeCurrencyAudioProbe& audio){
    return owner.dispatchTimeFieldWind(event,downwell::MotionBinary64::fromInteger(1),audio);
}
bool nativePlayerControllerBirth(downwell::GameplayCollisionOwner& owner,unsigned source,
    downwell::RunnerImageValue x,downwell::RunnerImageValue y,downwell::InstanceHandle& key){
    return owner.playerControllers().create(64,source,x,y,key);
}
bool nativeCameraCreate(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle key,
    downwell::RunnerImageValue x,downwell::RunnerImageValue y){
    return owner.playerControllers().initializeCamera(key,480,false,x,y);
}
bool nativeCompanionCreate(downwell::GameplayCollisionOwner& owner,unsigned source,downwell::InstanceHandle& key){
    return owner.playerControllers().createCompanion(249,source,{0x43700000u},{0x43300000u},key);
}
struct NativeChildAllocatorProbe {bool operator()(int,unsigned&);};
bool nativePlayerChildSequence(GameplayWorld& world,NativeChildAllocatorProbe allocate){
    return world.createPlayerChildren(allocate);
}
bool nativeAuthoredPlayerChildren(GameplayWorld& world,NativeChildAllocatorProbe allocate,
    const downwell::RoomPack& rooms,const downwell::PackedRoom& room){
    return world.withPlayerChildCreation(allocate,[&]{
        return world.loadRoomTerrainWithStartField(rooms,room,[](const auto&){return true;});
    });
}
bool nativeRegisteredTimeShardStep(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle shared,
    const downwell::SpriteMaskPack& masks,bool stopped){
    return owner.stepTimeShard(shared,stopped,masks);
}
bool nativePlayerTimeField(downwell::GameplayCollisionOwner& owner,downwell::MotionBinary64 x,
    downwell::MotionBinary64 y,const downwell::SpriteMaskPack& masks,bool& hit){
    return owner.playerTimeFieldMeeting(x,y,masks,hit);
}
bool nativeBoxAlarmStop(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle box,
    const downwell::SpriteMaskPack& masks,bool stopped,int width){
    return owner.stopBoxAlarm0(box,stopped,width,masks);
}
bool nativePlayerTimeFieldSelection(downwell::GameplayCollisionOwner& owner,downwell::MotionBinary64 x,
    downwell::MotionBinary64 y,const downwell::SpriteMaskPack& masks,
    downwell::GameplayCollisionOwner::RegisteredFieldContact& result){
    return owner.playerTimeFieldSelection(x,y,masks,result);
}
bool nativeOwnedDebrisTick(GameplayWorld& world){
    return world.stepWithOwnedDebris(0,[](auto,const auto&,bool& frozen){frozen=false;return true;},
        [](auto,const auto&,auto,auto,bool& hit){hit=false;return true;});
}
bool nativeAlarmDispatch(downwell::GameplayCollisionOwner& owner,downwell::RunnerStepSnapshot<16>& phase,
    bool (*dispatch)(downwell::InstanceHandle,unsigned,const downwell::PackedObjectEvent&)){
    return owner.dispatchAlarmPhase(phase,dispatch);
}
bool nativeDebrisOwnership(GameplayWorld& world,downwell::DebrisEffects<2>& effects,
    downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle& key){
    return world.spawnDebris(effects,{},{},1,key)&&owner.prepareDraw();
}
bool nativePublishedDebrisOwnership(GameplayWorld& world,downwell::GameplayCollisionOwner& owner,
    downwell::InstanceHandle& key,std::uint32_t sourceId){
    return world.spawnDebrisWithSourceId(sourceId,{},{},1,key)&&owner.prepareDraw();
}
bool nativeDebrisQuery(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle local,
    downwell::MotionBinary64 x,downwell::MotionBinary64 y,const downwell::SpriteMaskPack& masks,bool& hit){
    return owner.debrisPlaceMeeting(local,x,y,masks,hit);
}
bool nativeDebrisDispatch(GameplayWorld& world,downwell::GameplayCollisionOwner& owner,
    downwell::DebrisEffects<2>& effects,downwell::InstanceHandle local,downwell::InstanceHandle shared,
    const downwell::SpriteMaskPack& masks){
    const auto phase=effects.beginPhase();
    return world.advanceDebrisRoomAlarm(effects,local,phase,0)&&owner.stepDebris(effects,shared,phase,masks,
        [](auto,const auto&,bool& frozen){frozen=false;return true;});
}
bool nativeDebrisFreeze(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle local,
    const downwell::SpriteMaskPack& masks,bool stopped,bool& frozen){
    return owner.debrisFrozen(local,stopped,480,masks,frozen);
}
bool nativeScriptSpawn(GameplayWorld& world,downwell::ScriptSpawnEffects<2>& effects,
    const downwell::SpriteCollisionPack& sprites,downwell::InstanceHandle& caller){
    return world.spawnScriptEffect(effects,{},{},603,{0x3fe00000u,0},{},{},
        [&](int id,unsigned& frames){downwell::PackedSpriteCollision sprite;
            if(!sprites.sprite(id,sprite))return false;frames=sprite.frameCount;return true;},caller)&&
        world.advanceScriptEffectAnimation(effects,effects.beginPhase());
}
bool nativeScriptCleanup(downwell::GameplayCollisionOwner& owner,downwell::ScriptSpawnEffects<2>& effects){
    return owner.prepareDraw(effects);
}
bool nativeScriptTick(GameplayWorld& world,downwell::ScriptSpawnEffects<2>& effects){
    return world.stepWithScriptEffects(effects,0);
}
bool nativeOwnedScript(GameplayWorld& world,const downwell::SpriteCollisionPack& sprites,downwell::InstanceHandle& caller){
    return world.spawnScriptEffect({},{},603,{0x3fe00000u,0},{},{},
        [&](int id,unsigned& frames){downwell::PackedSpriteCollision r;if(!sprites.sprite(id,r))return false;
            frames=r.frameCount;return true;},caller);
}
bool nativeBoxScript(GameplayWorld& world,const downwell::SpriteCollisionPack& sprites,downwell::InstanceHandle box){
    return world.spawnBoxScriptEffect(box,{},{},603,{0x3fe00000u,0},
        downwell::MotionBinary64::fromInteger(270),downwell::MotionBinary64::fromInteger(-60000),
        [&](int id,unsigned& frames){downwell::PackedSpriteCollision r;if(!sprites.sprite(id,r))return false;
            frames=r.frameCount;return true;});
}
bool nativeActorDraw(const GameplayWorld& world){
    return world.visitRoomActors([](auto,downwell::InstanceHandle key,auto){return key.generation!=0;});
}
bool nativeBoxWrites(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle h,
    downwell::MotionBinary64 health,int& parent){
    return owner.projectileWallParent(h,parent)&&owner.projectileBoxHealth(h,health)&&
        owner.projectileBoxHealthWrite(h,health)&&owner.projectileBoxHit(h);
}
downwell::ResultsDestination nativeScriptRetry(GameplayWorld& world,downwell::ScriptSpawnEffects<2>& effects){
    return world.restartFromResultsWithScriptEffects(downwell::DeathMenuSequence::Action::Retry,43,300,effects);
}
// Compile-only A500 gate: instantiate the public selectors without the host
// fixture loader, filesystem, heap-backed assertions or C++ runtime headers.
bool nativeCollisionQueries(downwell::GameplayCollisionOwner& owner,
    const downwell::SpriteMaskPack& masks,downwell::InstanceHandle self,
    downwell::MotionBinary64 x,downwell::MotionBinary64 y,
    downwell::GameplayCollisionOwner::Owner::Result& result){
    return owner.terrainCollisionPoint(self,60,x,y,true,true,masks,result)&&
        owner.terrainNonPreciseLine(self,60,x,y,x,y,true,result)&&
        owner.projectileNonPreciseLine(self,59,x,y,x,y,false,result)&&
        owner.projectileNonPreciseLine(self,83,x,y,x,y,false,result)&&
        owner.projectileSolidPoint(self,x,y,false,masks,result);
}
#else
#include "BoxBurstWorldCheck.h"
#include "core/GameplayFieldReentryVerification.h"
#include "core/ProjectilePublication.h"
#include "core/WallPresentation.h"
#include "platform/amiga/AmigaWallDamage.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
using namespace downwell;
static void checkReadOnlyDraw(const GameplayWorld& world){
    std::vector<std::array<std::uint32_t,4>> expected;
    assert(world.visitRoomActors([&](auto kind,InstanceHandle h,auto depth){
        expected.push_back({unsigned(kind),h.slot,h.generation,depth.bits});return true;
    }));
    // Fail before completing a possible cold fill; no later callback may run.
    if(!expected.empty()){
        unsigned calls=0;
        assert(!world.visitRoomActorsReadOnly([&](auto,auto,auto){++calls;return false;}));
        assert(calls==1);
    }
    for(unsigned pass=0;pass<2;++pass){
        std::vector<std::array<std::uint32_t,4>> actual;
        assert(world.visitRoomActorsReadOnly([&](auto kind,InstanceHandle h,auto depth){
            actual.push_back({unsigned(kind),h.slot,h.generation,depth.bits});return true;
        }));
        assert(actual==expected);
    }
}
static std::vector<std::uint8_t> read(const char* path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
struct TerrainAudit {
    GameplayTerrainInstances* store=nullptr;
    unsigned births=0,retires=0,failBirth=0,failRetire=0;
    bool birth(InstanceHandle h,const GameplayTerrainInstances::Record& r){
        assert(store->ownsIdentity(h,r));return ++births!=failBirth;
    }
    bool retire(InstanceHandle h,const GameplayTerrainInstances::Record& r){
        assert(store->ownsIdentity(h,r));return ++retires!=failRetire;
    }
    bool activation(InstanceHandle,const GameplayTerrainInstances::Record&,bool){return true;}
    bool membership(InstanceHandle,const GameplayTerrainInstances::Record&,bool){return true;}
};
struct TerrainDestructionAudit:TerrainAudit {
    unsigned destroys=0,activations=0;bool rejectDestroy=false,rejectActivation=false;
    bool move(InstanceHandle,const GameplayTerrainInstances::Record&,RunnerImageValue,RunnerImageValue){return true;}
    bool sprite(InstanceHandle,const GameplayTerrainInstances::Record&,int){return true;}
    bool imageIndex(InstanceHandle,const GameplayTerrainInstances::Record&,RunnerImageValue){return true;}
    bool markDestroyed(InstanceHandle h,const GameplayTerrainInstances::Record& r){
        assert(store->ownsIdentity(h,r)&&!r.destroyRequested);++destroys;return !rejectDestroy;
    }
    bool activation(InstanceHandle h,const GameplayTerrainInstances::Record& r,bool){
        assert(store->ownsIdentity(h,r)&&r.destroyRequested&&r.active);++activations;return !rejectActivation;
    }
};
struct GoalAudit {
    GameplayWorld* world=nullptr;
    unsigned reject=0;
    std::uint64_t draws=0;
    unsigned stage=0;
    bool roomBoundary=false;
    unsigned expectedSourceId=0;
    const unsigned* terrainCalls=nullptr;
    using Record=GameplayGoalFields::Record;
    bool birth(InstanceHandle h,const Record& r){
        assert(world->goalFieldInstances().find(h)==&r&&r.scaleX==1&&r.scaleY==1&&world->randomDrawCount()==draws);
        assert(r.sourceId==expectedSourceId);
        if(terrainCalls)assert(*terrainCalls==144);
        if(r.objectId==255){
            const auto& exits=world->exitControllers();
            assert(exits.find(exits.handle(0))->objectId==306&&exits.find(exits.handle(1))->objectId==74);
        }
        stage=1;return reject!=stage;
    }
    bool transform(InstanceHandle h,const Record& r,RunnerTransformField field,RunnerImageValue value,RunnerImageValue,unsigned){
        assert(world->goalFieldInstances().find(h)==&r&&world->randomDrawCount()==draws&&value.bits==0x40000000u);
        assert(r.scaleY==1&&((stage==1&&field==RunnerTransformField::scaleX&&r.scaleX==1)||
            (stage==2&&field==RunnerTransformField::scaleY&&r.scaleX==2)));
        return ++stage!=reject;
    }
    bool retire(InstanceHandle h,const Record& r){
        assert(world->goalFieldInstances().find(h)==&r&&r.scaleX==2&&r.scaleY==2);
        if(r.objectId==371)assert(roomBoundary&&r.destroyRequested);
        stage=4;return reject!=stage;
    }
    bool markDestroyed(InstanceHandle h,const Record& r){
        assert(world->goalFieldInstances().find(h)==&r&&r.objectId==371&&r.scaleX==2&&r.scaleY==2&&
            !r.destroyRequested&&world->randomDrawCount()==draws+1);
        stage=5;return reject!=stage;
    }
    bool mask(InstanceHandle,const Record&,int){return true;}
    bool activation(InstanceHandle,const Record&,bool){return true;}
    bool membership(InstanceHandle,const Record&,bool){return true;}
    bool move(InstanceHandle,const Record&,RunnerImageValue,RunnerImageValue){return true;}
    bool sprite(InstanceHandle,const Record&,int){return true;}
    bool imageIndex(InstanceHandle,const Record&,RunnerImageValue){return true;}
};
struct ExitAudit {
    GameplayWorld* world;
    unsigned reject=0,births=0,retires=0;
    std::uint64_t draws;
    bool birth(InstanceHandle h,const GameplayExitControllers::Record& r){
        assert(world->exitControllers().find(h)==&r&&r.active&&r.x==240&&r.y==700);
        assert(world->randomDrawCount()==draws&&!world->goalFieldInstances().find({0,1}));
        assert(r.objectId==(births==0?306:74));
        return ++births!=reject;
    }
    bool retire(InstanceHandle h,const GameplayExitControllers::Record& r){
        assert(world->exitControllers().find(h)==&r);++retires;return reject!=3;
    }
    bool activation(InstanceHandle,const GameplayExitControllers::Record&,bool){return true;}
    bool membership(InstanceHandle,const GameplayExitControllers::Record&,bool){return true;}
};
struct BulletAudit {
    GameplayWorld* world;unsigned reject=0,last=0;
    using Record=GameplayWorld::BulletState;
    bool check(InstanceHandle h,const Record& r,unsigned event){
        assert(world->resolveBulletLifetime(h)==&r);last=event;return event!=reject;
    }
    bool birth(InstanceHandle h,const Record& r){assert(!r.trajectory.initialized);return check(h,r,1);}
    bool sprite(InstanceHandle h,const Record& r,int sprite){assert(sprite==459);return check(h,r,2);}
    bool transform(InstanceHandle h,const Record& r,RunnerTransformField,RunnerImageValue,RunnerImageValue,unsigned){
        assert(r.trajectory.initialized);return check(h,r,3);
    }
    bool move(InstanceHandle h,const Record& r,RunnerImageValue,RunnerImageValue){return check(h,r,4);}
    bool imageIndex(InstanceHandle h,const Record& r,RunnerImageValue){return check(h,r,5);}
    bool markDestroyed(InstanceHandle h,const Record& r){assert(!r.destroyRequested);return check(h,r,6);}
    bool retire(InstanceHandle h,const Record& r){return check(h,r,7);}
    bool activation(InstanceHandle,const Record&,bool){return true;}
    bool membership(InstanceHandle,const Record&,bool){return true;}
    bool mask(InstanceHandle,const Record&,int){return true;}
};
static void checkTypedCollisionView(const GameplayCollisionView& view,GameplayCollisionView::Kind kind,InstanceHandle local){
    using K=GameplayCollisionView::Kind;
    InstanceHandle shared;GameplayCollisionView::Record reference;
    if(GameplayCollisionView::encode(kind,local,shared))view.resolve(shared,reference);
    switch(kind){
        case K::scattered:assert(view.resolveLocal<K::scattered>(local)==reference.scattered);break;
        case K::spawner:assert(view.resolveLocal<K::spawner>(local)==reference.spawner);break;
        case K::sideCameraLocker:assert(view.resolveLocal<K::sideCameraLocker>(local)==reference.sideCameraLocker);break;
        case K::player:assert(view.resolveLocal<K::player>(local)==reference.player);break;
        case K::enemy:assert(view.resolveLocal<K::enemy>(local)==reference.enemy);break;
        case K::breakable:assert(view.resolveLocal<K::breakable>(local)==reference.breakable);break;
        case K::terrain:assert(view.resolveLocal<K::terrain>(local)==reference.terrain);break;
        case K::timeField:assert(view.resolveLocal<K::timeField>(local)==reference.timeField);break;
        case K::goalField:assert(view.resolveLocal<K::goalField>(local)==reference.goalField);break;
        case K::exitController:assert(view.resolveLocal<K::exitController>(local)==reference.exitController);break;
        case K::bullet:assert(view.resolveLocal<K::bullet>(local)==reference.bullet);break;
        case K::scriptEffect:assert(view.resolveLocal<K::scriptEffect>(local)==reference.scriptEffect);break;
        case K::corpseExpiry:assert(view.resolveLocal<K::corpseExpiry>(local)==reference.corpseExpiry);break;
        case K::smoke:assert(view.resolveLocal<K::smoke>(local)==reference.smoke);break;
        case K::corpse:assert(view.resolveLocal<K::corpse>(local)==reference.corpse);break;
        case K::consumptionBlood:assert(view.resolveLocal<K::consumptionBlood>(local)==reference.consumptionBlood);break;
        case K::runController:assert(view.resolveLocal<K::runController>(local)==reference.runController);break;
        case K::result:assert(view.resolveLocal<K::result>(local)==reference.result);break;
        case K::levelNumber:assert(view.resolveLocal<K::levelNumber>(local)==reference.levelNumber);break;
        case K::limboShard:assert(view.resolveLocal<K::limboShard>(local)==reference.limboShard);break;
        case K::flashBall:assert(view.resolveLocal<K::flashBall>(local)==reference.flashBall);break;
        case K::blast:assert(view.resolveLocal<K::blast>(local)==reference.blast);break;
        case K::debris:assert(view.resolveLocal<K::debris>(local)==reference.debris);break;
        case K::timeChange:assert(view.resolveLocal<K::timeChange>(local)==reference.timeChange);break;
        case K::timeShard:assert(view.resolveLocal<K::timeShard>(local)==reference.timeShard);break;
        case K::playerController:assert(view.resolveLocal<K::playerController>(local)==reference.playerController);break;
        case K::entryDither:assert(view.resolveLocal<K::entryDither>(local)==reference.entryDither);break;
        case K::drone:assert(view.resolveLocal<K::drone>(local)==reference.drone);break;
        case K::bone:assert(view.resolveLocal<K::bone>(local)==reference.bone);break;
        case K::skeleton:assert(view.resolveLocal<K::skeleton>(local)==reference.skeleton);break;
        case K::blood:assert(view.resolveLocal<K::blood>(local)==reference.blood);break;
        case K::currency:assert(view.resolveLocal<K::currency>(local)==reference.currency);break;
        case K::ditherAbove:assert(view.resolveLocal<K::ditherAbove>(local)==reference.ditherAbove);break;
        case K::builder:assert(view.resolveLocal<K::builder>(local)==reference.builder);break;
    }
}
static void checkCollisionViewMembership(const GameplayWorld& world){
    const GameplayCollisionView view(world);
    for(unsigned family=0;family<=unsigned(GameplayCollisionView::Kind::scattered);++family)
        for(auto local:{InstanceHandle{0,0},InstanceHandle{0xffffffffu,1},InstanceHandle{GameplayCollisionView::capacity,1}})
            checkTypedCollisionView(view,GameplayCollisionView::Kind(family),local);
    for(unsigned slot=0;slot<=GameplayCollisionView::capacity;++slot){
        for(unsigned generation:{0u,1u,2u,3u,0xffffffffu}){
            GameplayCollisionView::Record record;
            const InstanceHandle h{slot,generation};
            assert(view.contains(h)==view.resolve(h,record));
            auto kind=GameplayCollisionView::Kind::debris;
            InstanceHandle local{0xffffffffu,0xffffffffu};
            const bool decoded=GameplayCollisionView::decode(h,kind,local);
            assert(decoded==(slot<GameplayCollisionView::capacity&&generation!=0));
            if(decoded){
                InstanceHandle encoded;
                assert(GameplayCollisionView::encode(kind,local,encoded));
                assert(encoded.slot==h.slot&&encoded.generation==h.generation);
                checkTypedCollisionView(view,kind,local);
            }else assert(kind==GameplayCollisionView::Kind::debris&&local.slot==0xffffffffu&&local.generation==0xffffffffu);
        }
    }
}
static void checkEnemySlotDirectory(const ObjectPack& objects,const SpriteCollisionPack& sprites){
    auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
    auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
        owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
    assert(owner->attach(*world));
    InstanceHandle handles[24]{};bool retired[24]{};
    for(unsigned i=0;i<24;++i){
        assert(world->createGeneratedInstance({112,240+int(i)*16,512},objects,[](const auto&){return true;}));
        const auto& e=world->enemyAt(i);handles[i]={e.identitySlot,e.lifetimeId};
    }
    const auto check=[&](){
        checkCollisionViewMembership(*world);
        for(unsigned i=0;i<24;++i){
            const GameplayWorld::EnemyState* expected=nullptr;
            for(unsigned j=0;j<world->enemyCount();++j){
                const auto& e=world->enemyAt(j);
                if(e.identitySlot==handles[i].slot&&e.lifetimeId==handles[i].generation)expected=&e;
            }
            assert((expected==nullptr)==retired[i]);
            assert(world->resolveEnemyLifetime(handles[i])==expected);
            assert(world->findEnemy(handles[i])==expected);
            InstanceHandle shared;
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::enemy,handles[i],shared));
            assert(GameplayCollisionView(*world).contains(shared)==(expected!=nullptr));
        }
    };
    check();
    // Middle, head and tail removals keep source traversal stable while the
    // borrowed record addresses change. Compare every indexed lookup to scan.
    for(unsigned parity=0;parity<2;++parity)for(unsigned i=parity;i<24;i+=2){
        assert(world->markEnemyDestroyed(handles[i])&&owner->prepareDraw());
        retired[i]=true;check();
    }
    assert(world->enemyCount()==0);
    assert(world->createGeneratedInstance({112,240,512},objects,[](const auto&){return true;}));
    const auto& next=world->enemyAt(0);const InstanceHandle replacement{next.identitySlot,next.lifetimeId};
    assert(replacement.slot==handles[0].slot&&replacement.generation!=handles[0].generation);
    assert(!world->resolveEnemyLifetime(handles[0])&&world->resolveEnemyLifetime(replacement)==&next);
    assert(world->configureEnemy(0,112,163,0,336,0,0,5,14,0,0,-1,-1,0,0,false));
    const auto& overwritten=world->enemyAt(0);const InstanceHandle dormant{overwritten.identitySlot,overwritten.lifetimeId};
    assert(!world->resolveEnemyLifetime(replacement)&&world->resolveEnemyLifetime(dormant)==&overwritten);
    auto& flags=const_cast<GameplayWorld::EnemyState&>(overwritten);
    flags.activationPending=false;
    assert(!world->findEnemy(dormant)&&world->resolveEnemyLifetime(dormant)==&overwritten);
}
static void checkNativeAnimationBatches(const ObjectPack& objects,const SpriteCollisionPack& sprites){
    struct Cursor {
        std::vector<unsigned> slots;unsigned at=0;
        unsigned take(){return slots.at(at++);}
        bool within(unsigned first,unsigned last)const{return at<slots.size()&&slots[at]>=first&&slots[at]<last;}
    };
    using View=GameplayCollisionView;
    for(unsigned scenario=0;scenario<3;++scenario){
        unsigned phases[2][3]{},remainders[2][3]{};bool results[2]{},healthy[2]{};
        for(unsigned side=0;side<2;++side){
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},nullptr,GameplayCollisionOwner::Ownership::native);
            auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),owner->breakableBinding(),
                owner->playerBinding(),&owner->terrain());
            assert(owner->attach(*world));
            InstanceHandle keys[3];Cursor cursor;
            for(unsigned i=0;i<3;++i){
                assert(world->createGeneratedInstance({112,240+int(i)*16,512},objects,[](const auto&){return true;}));
                auto& enemy=const_cast<GameplayWorld::EnemyState&>(world->enemyAt(i));
                keys[i]={enemy.identitySlot,enemy.lifetimeId};cursor.slots.push_back(View::enemyBase+keys[i].slot);
                enemy.imagePhase=scenario==1&&i==1?0x7fffffffu:17*i;enemy.imageRemainder=i;enemy.imageSpeed=7+i;
            }
            if(scenario==2)++keys[1].generation;
            auto identity=[&](unsigned slot){for(const auto key:keys)if(key.slot+View::enemyBase==slot)return key.generation;return 0u;};
            if(side)results[side]=world->advanceNativeEnemyAnimations(cursor,identity,View::enemyBase,View::breakableBase);
            else {
                results[side]=true;
                for(const auto key:keys){
                    // The retained adapter checks the lifetime before entering
                    // the world method; a stale handle must not alter run state.
                    if(!world->resolveEnemyLifetime(key)||!world->advanceEnemyRoomAnimation(key)){results[side]=false;break;}
                }
            }
            healthy[side]=world->effectsSucceeded();
            for(unsigned i=0;i<3;++i){phases[side][i]=world->enemyAt(i).imagePhase;remainders[side][i]=world->enemyAt(i).imageRemainder;}
        }
        assert(results[0]==results[1]&&healthy[0]==healthy[1]);
        for(unsigned i=0;i<3;++i)assert(phases[0][i]==phases[1][i]&&remainders[0][i]==remainders[1][i]);
    }
}
int main(){
    assert(downwell::RunnerBindingRevisionVerification::run());
    {
        bool owner=true,otherOwner=true;
        FailureSignal<> signal;signal.observe(owner);
        assert(signal&&owner);signal=false;assert(!signal&&!owner);
        signal=true;assert(signal&&!owner); // Child recovery cannot recover its owner.
        FailureSignal<> copy(signal);copy=false;assert(signal&&!copy);
        FailureSignal<> destination;destination.observe(otherOwner);
        destination=copy;assert(!destination&&!otherOwner);
        owner=true;FailureSignal<false> inverted(false);inverted.observe(owner);
        inverted=true;assert(inverted&&!owner);
        owner=true;FailureSignal<> failed(false);failed.observe(owner);assert(!owner);
        owner=otherOwner=true;FailureSignal<> bound;bound.observe(owner);bound.observe(otherOwner);
        assert(!owner&&!otherOwner);
        owner=true;RunnerDepthChanges<1> depths;depths.observeFailure(owner);
        RunnerImageValue a{},b{};
        assert(depths.request(0,a,MotionBinary64::fromInteger(1))&&owner);
        assert(!depths.request(1,b,MotionBinary64::fromInteger(2))&&!owner&&!depths.succeeded());
    }
    for(bool failSmoke:{false,true}){
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false);
        checkBoxBurstWorld(*world,failSmoke);
    }
    {
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false);
        BulletAudit audit{world.get()};
        assert(world->bindBulletLifecycle(GameplayWorld::BulletLifecycleBinding::bindMaskDestruction(audit)));
        world->step(4);assert(world->effectsSucceeded()&&world->bulletCount()==1);
        const auto& shot=world->bulletAt(0);const InstanceHandle key{shot.identitySlot,shot.lifetimeId};
        assert(world->markBulletDestroyed(key)&&audit.last==6);
        const auto* retained=world->resolveBulletLifetime(key);
        assert(retained&&retained->destroyRequested&&!retained->active);
        audit.last=0;assert(world->markBulletDestroyed(key)&&audit.last==0);
        assert(world->retireMarkedBullet(key)&&audit.last==7&&!world->resolveBulletLifetime(key));
        assert(!world->markBulletDestroyed(key)&&!world->effectsSucceeded()&&audit.last==7);
    }
    {
        // Observe real publications, not a synthetic list of event names:
        // bat 116 must move before the usual bullet 261 publishes its Step.
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false);
        struct OrderedBullet:BulletAudit {
            long enemyX=0,enemyY=0;bool observed=false;
            bool move(InstanceHandle h,const Record& r,RunnerImageValue x,RunnerImageValue y){
                enemyX=world->enemyAt(0).preciseX;enemyY=world->enemyAt(0).preciseY;observed=true;
                return BulletAudit::move(h,r,x,y);
            }
        } audit;
        audit.world=world.get();
        assert(world->bindBulletLifecycle(GameplayWorld::BulletLifecycleBinding::bindMaskDestruction(audit)));
        world->configureSolidResolver([](void*,int,int){return false;},nullptr);
        assert(world->configureEnemy(0,116,153,64,12,0,0,15,12,0,0,-1,-1,270,256,true,false,true));
        const auto beforeX=world->enemyAt(0).preciseX,beforeY=world->enemyAt(0).preciseY;
        world->step(4);
        const auto& bat=world->enemyAt(0);
        assert(world->effectsSucceeded()&&audit.observed&&world->bulletCount()==1&&
            audit.enemyX==bat.preciseX&&audit.enemyY==bat.preciseY&&
            (bat.preciseX!=beforeX||bat.preciseY!=beforeY));
    }
    for(bool rejectStep:{false,true}){
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false);
        assert(world->configureEnemy(0,116,153,0,0,0,0,15,12,0,0,-1,-1,270,256,true,false,true));
        struct Boundary {GameplayWorld* world;bool reject;int before,atStep=-1,atCollision=-1;}
            boundary{world.get(),rejectStep,world->health()};
        world->configurePhaseBoundary([](void* p,GameplayWorld::PhaseBoundary phase){
            auto& b=*static_cast<Boundary*>(p);
            if(phase==GameplayWorld::PhaseBoundary::step){b.atStep=b.world->health();return !b.reject;}
            if(phase==GameplayWorld::PhaseBoundary::collision)b.atCollision=b.world->health();
            return true;
        },&boundary);
        world->step(0);
        assert(world->effectsSucceeded()==!rejectStep&&boundary.atStep==boundary.before);
        if(rejectStep)assert(world->health()==boundary.before&&boundary.atCollision==-1&&world->damageNumbers().size()==0);
        else assert(boundary.atCollision==boundary.before-1&&world->damageNumbers().size()==1);
    }
    for(unsigned failure=1;failure<=7;++failure){
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false);
        BulletAudit audit{world.get(),failure};
        assert(world->bindBulletLifecycle(GameplayWorld::BulletLifecycleBinding::bindMaskDestruction(audit)));
        if(failure>=6)world->configureSolidResolver([](void*,int,int y){return y==8;},nullptr);
        world->step(4);
        if(failure==5)world->step(0);
        if(failure==7){
            const auto& bullet=world->bulletAt(0);
            assert(bullet.destroyRequested&&!world->retireMarkedBullet({bullet.identitySlot,bullet.lifetimeId}));
        }
        assert(!world->effectsSucceeded()&&audit.last==failure);
        const auto draws=world->randomDrawCount();world->step(4);
        assert(audit.last==failure&&world->randomDrawCount()==draws);
    }
    for(unsigned i=0;i<10000;++i){
        const std::uint32_t turns=i*2654435761u;
        RunnerImageValue actual;
        assert(ProjectilePublication::angle(turns,actual));
        float expected=static_cast<float>(double(turns)*360.0/4294967296.0);
        std::uint32_t bits;std::memcpy(&bits,&expected,4);assert(actual.bits==bits);
        const MachineGunTrajectory::Coordinate position{int(i)-5000,turns&1048575u};
        assert(ProjectilePublication::coordinate(position,176,actual));
        expected=static_cast<float>(position.pixel+176+double(position.fraction)/1048576.0);
        std::memcpy(&bits,&expected,4);assert(actual.bits==bits);
    }
    {
        std::uint32_t seed=0x97463251u;
        auto next=[&](){seed=seed*1664525u+1013904223u;return seed;};
        for(unsigned i=0;i<100000;++i){
            const auto raw=next();
            const std::int32_t pixel=raw<=0x7fffffffu?std::int32_t(raw):-1-std::int32_t(~raw);
            const unsigned fraction=next()&1048575u;
            const int origin=int(next()&511u);
            RunnerImageValue actual{0xdeadbeefu},reference=actual;
            const MachineGunTrajectory::Coordinate position{pixel,fraction};
            const bool valid=ProjectilePublication::coordinateBinary64(position,origin,reference);
            assert(ProjectilePublication::coordinate(position,origin,actual)==valid&&actual.bits==reference.bits);
            if(valid){
                const float native=float(double(pixel)+origin+double(fraction)/1048576.0);
                std::uint32_t bits;std::memcpy(&bits,&native,4);assert(actual.bits==bits);
            }
        }
        assert(ProjectilePublicationVerification::run());
    }
    for(unsigned failure=0;failure<4;++failure){
        auto world=std::make_unique<GameplayWorld>(300,true,0,false);
        const auto draws=world->randomDrawCount();
        unsigned calls=0,next=810000;
        const int expected[]={306,74,255};
        const bool created=world->registerLevelExitWithSourceIds(700,[&](int object,unsigned& id){
            assert(calls<3&&object==expected[calls]);
            if(calls){
                const auto* parent=world->exitControllers().find(world->exitControllers().handle(0));
                assert(parent&&parent->sourceId==810000);
            }
            ++calls;if(calls==failure)return false;
            id=next;next+=7;return true;
        });
        assert(created==(failure==0)&&calls==(failure?failure:3));
        assert(world->randomDrawCount()==draws+(created?1:0));
        if(!created){
            assert(!world->effectsSucceeded());
            assert(!world->registerLevelExitWithSourceIds(800,[&](int,unsigned&){assert(false);return true;}));
            assert(bool(world->exitControllers().find(world->exitControllers().handle(0)))==(failure>1));
            assert(bool(world->exitControllers().find(world->exitControllers().handle(1)))==(failure>2));
        }else{
            assert(world->exitControllers().find(world->exitControllers().handle(1))->sourceId==810007);
            assert(world->goalFieldInstances().find(world->goalFieldInstances().handle(0))->sourceId==810014);
        }
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
        GameplayCollisionOwner::Ownership::nativeScenery}){
        const auto ob=read("generated/downwell-objects.dwob");
        const auto sb=read("generated/downwell-sprites.dwsc");
        ObjectPack objects(ob.data(),ob.size());SpriteCollisionPack sprites(sb.data(),sb.size());
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),nullptr);
        assert(owner->attach(*world));
        DeferredCreateInstance command{82,240,512};command.setTimeFieldRadiusWord(0xabc12345u);
        const auto draws=world->randomDrawCount();
        assert(world->adoptGeneratedTimeFieldWithSourceId(820007,command));
        const auto* field=world->timeFields().at(0);
        assert(field&&field->sourceId==820007&&field->radius.equal(OrdinaryTimeField::radiusFromWord(0xabc12345u)));
        unsigned population=0;
        assert(owner->visitRegistrationOrder([&](InstanceHandle,int object,std::uint32_t){
            if(object==82)++population;return true;
        })&&population==1&&world->randomDrawCount()==draws);
        assert(!world->adoptGeneratedTimeFieldWithSourceId(820007,command)&&!world->effectsSucceeded());
        assert(world->timeFields().size()==1&&world->randomDrawCount()==draws);
    }
    for(unsigned failure=0;failure<4;++failure){
        auto world=std::make_unique<GameplayWorld>(300,true,0,false);
        ExitAudit audit{world.get(),failure,0,0,world->randomDrawCount()};
        assert(world->bindExitControllerLifecycle(GameplayExitControllers::Binding::bind(audit)));
        const bool created=world->registerLevelExit(700);
        assert(created==(failure==0||failure==3));
        if(!created){
            assert(audit.births==failure&&world->randomDrawCount()==audit.draws&&!world->effectsSucceeded());
            assert(!world->registerLevelExit(700)&&audit.births==failure);
            continue;
        }
        assert(audit.births==2&&world->randomDrawCount()==audit.draws+1);
        const auto old=world->exitControllers().handle(0);
        const auto result=world->restartFromResults(DeathMenuSequence::Action::Retry,2,300);
        if(failure==3){assert(result==ResultsDestination::none&&!world->effectsSucceeded()&&audit.retires==1);continue;}
        assert(result==ResultsDestination::gameplay&&audit.retires==2&&!world->exitControllers().find(old));
        assert(world->exitControllers().birthSequence()>old.generation);
    }
    for(bool authored:{false,true})for(unsigned failure=0;failure<=6;++failure){
        const auto roomBytes=read("generated/downwell-rooms.dwrm");
        RoomPack rooms(roomBytes.data(),roomBytes.size());
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,
            GameplayWorld::EnemyLifecycleBinding{},GameplayWorld::BreakableLifecycleBinding{},
            GameplayWorld::PlayerLifecycleBinding{},nullptr,authored?&rooms:nullptr);
        GoalAudit audit{world.get(),failure,world->randomDrawCount(),0};
        audit.expectedSourceId=authored?101097:0;
        assert(world->bindGoalFieldLifecycle(GameplayGoalFields::Binding::bindMaskDestruction(audit)));
        world->configurePhaseBoundary([](void* p,GameplayWorld::PhaseBoundary phase){
            auto& a=*static_cast<GoalAudit*>(p);
            const auto h=a.world->goalFieldInstances().handle(0);
            assert(phase==GameplayWorld::PhaseBoundary::roomEntry&&a.world->goalFieldInstances().find(h)->destroyRequested);
            a.roomBoundary=true;return a.reject!=6;
        },&audit);
        assert(world->createRoomStartField()==(failure==0));
        assert(world->randomDrawCount()==audit.draws+(failure==0||failure>=4?1:0));
        const auto* record=world->goalFieldInstances().find({0,1});
        if(!failure)assert(!record&&audit.roomBoundary);
        else{
            assert(record&&!world->effectsSucceeded()&&record->destroyRequested==(failure==4||failure==6));
            assert(!world->createRoomStartField());
            struct BuilderProbe {
                bool called=false;
                void resetRoom(int,int,GameMakerRandom&,const AreaOneBuilder::Setup&){called=true;}
            } builder;
            const auto draws=world->randomDrawCount();
            assert(!world->initializeAreaOneBuilder(builder,0,0)&&!builder.called&&world->randomDrawCount()==draws);
        }
    }
    for(bool prepared:{false,true})for(unsigned failure:{0u,144u,145u,240u,241u}){
        const auto bytes=read("generated/downwell-rooms.dwrm");RoomPack rooms(bytes.data(),bytes.size());
        PackedRoom room;assert(rooms.findRoom(11,room));
        PreparedRoomTerrain plan;assert(plan.prepare(rooms,room));
        auto world=std::make_unique<GameplayWorld>(300,true,0,false);
        unsigned calls=0;
        GoalAudit audit{world.get(),0,world->randomDrawCount(),0};
        audit.expectedSourceId=101097;audit.terrainCalls=&calls;
        assert(world->bindGoalFieldLifecycle(GameplayGoalFields::Binding::bindMaskDestruction(audit)));
        world->configurePhaseBoundary([](void* p,GameplayWorld::PhaseBoundary phase){
            auto& a=*static_cast<GoalAudit*>(p);
            assert(phase==GameplayWorld::PhaseBoundary::roomEntry&&*a.terrainCalls==240&&
                a.world->goalFieldInstances().find({0,1})->destroyRequested);
            a.roomBoundary=true;return a.reject!=241;
        },&audit);
        audit.reject=failure==241?241:0;
        auto terrain=[&](const auto&){
            if(calls>=144)assert(world->goalFieldInstances().find({0,1})->destroyRequested);
            return ++calls!=failure;
        };
        assert(world->loadRoomTerrainWithStartField(rooms,room,terrain,prepared?&plan:nullptr)==(failure==0));
        assert(calls==(failure&&failure<241?failure:240));
        assert(world->randomDrawCount()==audit.draws+(failure==144?0:1));
        assert(audit.roomBoundary==(failure==0||failure==241));
        const auto* retained=world->goalFieldInstances().find({0,1});
        assert(bool(retained)==(failure==145||failure==240||failure==241));
        if(retained)assert(retained->sourceId==101097&&retained->destroyRequested);
        const auto oldCalls=calls;
        assert(!world->loadRoomTerrainWithStartField(rooms,room,terrain,prepared?&plan:nullptr)&&calls==oldCalls);
    }
    for(unsigned failure=1;failure<=4;++failure){
        auto world=std::make_unique<GameplayWorld>(300,true,0,false);
        GoalAudit audit{world.get(),failure,world->randomDrawCount(),0};
        assert(world->bindGoalFieldLifecycle(GameplayGoalFields::Binding::bindTransforms(audit)));
        const bool created=world->registerLevelExit(700);
        assert(created==(failure==4)&&audit.stage==(failure==4?3:failure));
        const auto key=world->goalFieldInstances().handle(0);
        const auto* record=world->goalFieldInstances().find(key);
        assert(record&&record->scaleX==(failure<=2?1:2)&&record->scaleY==(failure==4?2:1));
        assert(world->randomDrawCount()==audit.draws+(failure==4?1:0));
        if(failure==4)assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::none&&audit.stage==4);
        const auto draws=world->randomDrawCount();
        assert(!world->effectsSucceeded()&&!world->registerLevelExit(800)&&world->randomDrawCount()==draws&&
            world->goalFieldInstances().find(key)==record);
    }
    {
        TerrainDestructionAudit lifecycle;
        struct RejectDamage {unsigned calls=0;bool changed(int x,int y,int before,int after){assert(x==32&&y==48&&before==51&&after==-1);++calls;return false;}} damage;
        auto store=std::make_unique<GameplayTerrainInstances>(GameplayTerrainInstances::Binding::bindDestruction(lifecycle),
            GameplayTerrainInstances::Presentation::bind(damage));lifecycle.store=store.get();
        assert(store->append(60,32,48,true));const auto h=store->handle(0);
        assert(!store->requestDestroy(h)&&store->find(h)->destroyRequested&&!store->valid()&&damage.calls==1&&lifecycle.destroys==1);
        assert(!store->activation(h,false)&&!store->retireMarked(h)&&lifecycle.activations==0&&lifecycle.retires==0);
    }
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                       GameplayCollisionOwner::Ownership::nativeScenery}){
        const auto ob=read("generated/downwell-objects.dwob"),sb=read("generated/downwell-sprites.dwsc"),
            rb=read("generated/downwell-rooms.dwrm");
        ObjectPack objects(ob.data(),ob.size());SpriteCollisionPack sprites(sb.data(),sb.size());
        RoomPack rooms(rb.data(),rb.size());
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms,ownership);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));auto& entries=owner->entryDithers();InstanceHandle first;
        auto zero=[](auto& value){value={};return true;};
        assert(entries.create(411001,zero,zero,first));
        struct Builder {
            GameplayEntryDithers* entries;InstanceHandle first,child{};unsigned calls=0;
        } builder{&entries,first};
        world->configureGenerationStep([](void* context,GameplayWorld&,GameMakerRandom&,int){
            auto& b=*static_cast<Builder*>(context);int y=-1;
            // Builder 65 sees the old position: entry wipe 73 has not stepped.
            assert(b.entries->find(b.first)->y.promoted().roundPixel(y)&&y==int(16*b.calls));
            if(b.calls++==0){
                auto zero=[](auto& value){value={};return true;};
                return b.entries->create(411002,zero,zero,b.child);
            }
            return true;
        },&builder);
        for(int tick=1;tick<=2;++tick){
            world->step(0);int existing=-1,newborn=-1;
            assert(world->effectsSucceeded()&&entries.find(first)->y.promoted().roundPixel(existing)&&
                entries.find(builder.child)->y.promoted().roundPixel(newborn)&&existing==16*tick&&newborn==16*(tick-1));
        }
    }
    for(unsigned failure=0;failure<3;++failure){
        TerrainDestructionAudit audit;audit.rejectDestroy=failure==1;audit.rejectActivation=failure==2;
        auto terrain=std::make_unique<GameplayTerrainInstances>(GameplayTerrainInstances::Binding::bindDestruction(audit));
        audit.store=terrain.get();assert(terrain->append(60,32,48,true));
        const auto h=terrain->handle(0);const auto record=terrain->find(h);
        assert(terrain->requestDestroy(h)==(failure!=1)&&audit.destroys==1);
        if(failure!=1){
            assert(terrain->requestDestroy(h)&&audit.destroys==1);
            assert(terrain->activation(h,false)==(failure==0)&&audit.activations==1);
        }
        assert(terrain->find(h)==record&&terrain->size()==1&&terrain->find(h)->destroyRequested==(failure!=1));
        assert(terrain->find(h)->active==(failure!=0)&&terrain->valid()==(failure==0));
        if(failure){
            assert(!terrain->requestDestroy(h)&&!terrain->activation(h,false)&&audit.destroys==1);
        }else{
            assert(terrain->clear()&&audit.retires==1&&!terrain->find(h));
        }
    }
    {
        auto terrain=std::make_unique<GameplayTerrainInstances>();assert(terrain->append(60,0,0,true));
        const auto h=terrain->handle(0);
        GameplayTerrainInstances::Record snapshot;
        assert(terrain->read(h,snapshot)&&terrain->ownsIdentity(h,snapshot));
        ++snapshot.x;assert(!terrain->ownsIdentity(h,snapshot));--snapshot.x;
        ++snapshot.lifetimeId;assert(!terrain->ownsIdentity(h,snapshot));--snapshot.lifetimeId;
        const auto stale=InstanceHandle{h.slot,h.generation+1};
        snapshot.imageIndex=123;
        assert(!terrain->read(stale,snapshot)&&snapshot.imageIndex==123&&
            !terrain->ownsIdentity(stale,snapshot));
        assert(!terrain->requestDestroy(h)&&!terrain->find(h)->destroyRequested&&!terrain->valid());
        assert(terrain->read(h,snapshot)&&terrain->ownsIdentity(h,snapshot));
    }
    {
        TerrainDestructionAudit audit;
        auto store=std::make_unique<GameplayTerrainInstances>(GameplayTerrainInstances::Binding::bindDestruction(audit));
        audit.store=store.get();
        for(unsigned i=0;i<32;++i)assert(store->append(60,int(i),0,true));
        for(unsigned i=0;i<4096;++i){
            const unsigned index=(i*13u)%32u;const auto old=store->handle(index);
            InstanceHandle survivors[31];decltype(store->find(InstanceHandle{})) pointers[31];unsigned n=0;
            for(unsigned j=0;j<32;++j)if(j!=index){survivors[n]=store->handle(j);pointers[n]=store->find(survivors[n]);++n;}
            assert(store->requestDestroy(old)&&store->retireMarked(old)&&!store->find(old)&&store->size()==31);
            assert(store->append(60,int(i),0,true));const auto fresh=store->handle(31);
            assert(fresh.slot==old.slot&&fresh.generation>old.generation&&!store->find(old));
            for(unsigned j=0;j<31;++j)assert(store->handle(j).slot==survivors[j].slot&&store->find(survivors[j])==pointers[j]);
        }
        const auto rejected=store->handle(7);assert(store->requestDestroy(rejected));audit.failRetire=audit.retires+1;
        assert(!store->retireMarked(rejected)&&!store->valid()&&store->size()==32&&store->find(rejected));
        assert(!store->retireMarked(rejected)&&audit.retires==audit.failRetire);
    }
    auto ob=read("generated/downwell-objects.dwob"),sb=read("generated/downwell-sprites.dwsc");
    ObjectPack objects(ob.data(),ob.size());SpriteCollisionPack sprites(sb.data(),sb.size());
    checkGeneratedSourceBirths(objects,sprites);
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
        GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),nullptr);
        assert(owner->attach(*world)&&world->registerSideCameraLocker(0,700,830001));
        const auto local=world->sideCameraLockers().handle(0);InstanceHandle shared;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::sideCameraLocker,local,shared));
        checkTypedCollisionView(GameplayCollisionView(*world),GameplayCollisionView::Kind::sideCameraLocker,local);
        assert(GameplayCollisionView(*world).contains(shared)&&world->sideCameraLockers().find(local)->sourceId==830001);
        RunnerImageValue x,y;
        if(mode==GameplayCollisionOwner::Ownership::reference){
            const auto* state=owner->state(shared);assert(state);x=state->x;y=state->y;
        }else assert(owner->nativePublishedPosition(shared,x,y));
        assert(x.bits==0x42a00000u&&y.bits==0x442f0000u);
        unsigned found=0;
        assert(owner->visitRegistrationOrder([&](InstanceHandle h,int object,std::uint32_t){
            if(object==62){assert(h.slot==shared.slot&&h.generation==shared.generation);++found;}return true;
        })&&found==1);
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,2,300)==ResultsDestination::gameplay);
        assert(!GameplayCollisionView(*world).contains(shared)&&!world->sideCameraLockers().find(local));
        assert(world->registerSideCameraLocker(400,900,830002));
        const auto fresh=world->sideCameraLockers().handle(0);
        assert(fresh.generation>local.generation&&world->sideCameraLockers().find(fresh)->x==400);
        assert(owner->valid()&&world->effectsSucceeded());
    }
    const auto resultRoomBytes=read("generated/downwell-rooms.dwrm");
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
        GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),nullptr);
        assert(owner->attach(*world));const auto draws=world->randomDrawCount();
        DeferredCreateInstance command{116,240,700};
        assert(world->adoptGeneratedEnemyWithSourceId(840001,command,objects));
        const auto& first=world->enemyAt(0);const InstanceHandle old{first.identitySlot,first.lifetimeId};
        assert(first.sourceId==840001);
        assert(world->adoptGeneratedEnemyWithSourceId(840009,command,objects));
        const auto& second=world->enemyAt(1);const InstanceHandle retained{second.identitySlot,second.lifetimeId};
        assert(world->markEnemyDestroyed(old)&&world->retireMarkedEnemy(old)&&!world->resolveEnemyLifetime(old));
        assert(world->resolveEnemyLifetime(retained)&&world->resolveEnemyLifetime(retained)->sourceId==840009);
        assert(world->adoptGeneratedEnemyWithSourceId(840017,command,objects));
        const auto& newest=world->enemyAt(world->enemyCount()-1);
        assert(newest.sourceId==840017&&newest.lifetimeId!=old.generation&&world->randomDrawCount()==draws);
        const auto count=world->enemyCount();
        assert(!world->adoptGeneratedEnemyWithSourceId(840009,command,objects)&&!world->effectsSucceeded());
        assert(world->enemyCount()==count&&world->resolveEnemyLifetime(retained)->sourceId==840009);
    }
    RoomPack resultRooms(resultRoomBytes.data(),resultRoomBytes.size());
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::nativeTerrain,
                  GameplayCollisionOwner::Ownership::native}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&resultRooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));auto& numbers=owner->levelNumbers();InstanceHandle local,shared;
        auto init=[](auto,auto& state){return state.create(3,MotionBinary64::fromInteger(24),
            [](unsigned i,unsigned& token){token=i;return true;});};
        assert(numbers.create(105187,MotionBinary64::fromInteger(241),MotionBinary64::fromInteger(603),init,local)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::levelNumber,local,shared));
        GameplayCollisionView view(*world);assert(view.contains(shared)&&numbers.activate(local,false));
        world->step(0);assert(world->effectsSucceeded()&&numbers.find(local)->state.alarms.get(0)==4);
        assert(numbers.activate(local,true)&&owner->reconcileActivation());
        for(unsigned pass=0;pass<2;++pass){
            unsigned seen=0;
            const bool visited=world->visitRoomActorsReadOnly([&](auto kind,auto key,auto depth){
                if(kind!=GameplayWorld::DrawActor::levelNumber)return true;
                RunnerImageValue expectedDepth;assert(expectedDepth.assignInteger(-100000));
                ++seen;return key.slot==local.slot&&key.generation==local.generation&&depth.bits==expectedDepth.bits;
            });
            assert(visited&&seen==1);
        }
        struct NoTitleSurface {
            bool exists(bool& value){value=false;return true;}
            bool begin(){std::abort();}bool end(){std::abort();}bool color(unsigned){std::abort();}
            bool view(unsigned,MotionBinary64&){std::abort();}
            bool character(unsigned,MotionBinary64,MotionBinary64){std::abort();}
        } noSurface;
        EffectMaskAssets emptyAssets;EffectSurfacePlacement::View emptyView{};
        auto emit=[](const EffectMaskAssets::Asset&,MotionBinary64,MotionBinary64){return true;};
        assert(world->visitRegisteredEffectSurfaces(emptyAssets,emptyView,emit)); // Titles are not conditional drawFx.
        unsigned titleDraws=0;
        assert(world->visitUnconditionalSurfaceDraw([&](auto kind,auto key,auto){
            assert(kind==GameplayWorld::DrawActor::levelNumber);
            ++titleDraws;return world->drawLevelNumber(key,noSurface);
        })&&titleDraws==1&&numbers.find(local)->state.characters[1].offset==-2);
        struct TitleSurface {
            unsigned glyphs=0;
            bool exists(bool& value){value=true;return true;}
            bool begin(){return true;}bool end(){return true;}bool color(unsigned){return true;}
            bool view(unsigned,MotionBinary64& value){value={};return true;}
            bool character(unsigned,MotionBinary64,MotionBinary64){++glyphs;return true;}
        } titleSurface;
        auto composition=std::make_unique<CurrencySmokeComposition>();
        for(unsigned setting=0;setting<4;++setting){
            CurrencySmokeComposition::Settings settings;
            settings.death=setting==1;settings.lowSpec=setting==2;settings.effectsExist=setting!=3;
            unsigned conditionalCalls=0,unconditionalCalls=0;
            assert(composition->prepareWithSurfaceProducers([](auto){},world->smokeEffects(),emptyAssets,
                emptyView,settings,[&](auto){++conditionalCalls;return true;},[&](auto){
                    ++unconditionalCalls;
                    return world->visitUnconditionalSurfaceDraw([&](auto kind,auto key,auto){
                        return kind==GameplayWorld::DrawActor::levelNumber&&world->drawLevelNumber(key,titleSurface);
                    });
                },[](auto){},[](MotionBinary64 value,std::int32_t& out){return value.roundPixel(out);}));
            assert(conditionalCalls==(setting==0?1u:0u)&&unconditionalCalls==(setting!=3?1u:0u));
            assert(titleSurface.glyphs==5*(setting<3?setting+1:3));
            assert(numbers.find(local)->state.characters[1].offset==(setting<3?int(setting)-1:1));
        }
        for(unsigned tick=1;tick<=142;++tick){
            world->step(0);assert(world->effectsSucceeded()&&owner->valid()&&owner->prepareDraw());
            if(tick<142){
                assert(view.contains(shared));const auto& state=numbers.find(local)->state;
                if(tick==4)assert(state.shown==2&&state.alarms.get(0)==4);
                if(tick==12)assert(state.alarms.get(1)==119);
                if(tick==131)assert(state.exit==1&&state.alarms.get(2)==3);
                assert(state.characters[1].offset==1); // Alarms/Step do not change the three Draw updates above.
            }
        }
        assert(!view.contains(shared)&&!numbers.find(local)&&!numbers.size());
        assert(numbers.create(105188,{}, {},init,local));
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::gameplay&&
            world->levelNumbers()==&numbers&&!numbers.size()&&!numbers.find(local)&&owner->valid());
    }
    for(unsigned failure:{1u,2u}){
        struct ResultAudit {
            GameplayWorld* world=nullptr;InstanceHandle oldPlayer{};
            unsigned reject=0,births=0,writes=0;
            bool birth(InstanceHandle,const GameplayResultInstance::Record& r){
                ++births;assert(!world->findPlayer(oldPlayer)&&r.sourceId==102522&&
                    r.x.bits==0x42a00000u&&r.y.bits==0x43100000u);
                return reject!=1;
            }
            bool move(InstanceHandle,const GameplayResultInstance::Record& r,RunnerImageValue x,RunnerImageValue y){
                ++writes;assert(r.y.bits==0x43100000u&&x.bits==r.x.bits&&y.bits==0x430e0000u);
                return reject!=2;
            }
            bool retire(InstanceHandle,const GameplayResultInstance::Record&){return true;}
            bool activation(InstanceHandle,const GameplayResultInstance::Record&,bool){return true;}
            bool membership(InstanceHandle,const GameplayResultInstance::Record&,bool){return true;}
        } audit;
        GameplayResultInstance result(GameplayResultInstance::Binding::bindMovement(audit));
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,GameplayWorld::EnemyLifecycleBinding{},
            GameplayWorld::BreakableLifecycleBinding{},GameplayWorld::PlayerLifecycleBinding{},nullptr,&resultRooms);
        audit.world=world.get();audit.oldPlayer=world->playerHandle();audit.reject=failure;
        assert(world->bindResultInstance(result)&&world->registerLevelExit(176+world->player().offsetY()));
        for(unsigned tick=0;tick<100;++tick)world->step(0);
        const auto draws=world->randomDrawCount();
        assert(world->levelResultRequested()&&!world->enterLevelClear(0,false)&&!world->effectsSucceeded()&&
            !world->levelClearActive()&&world->randomDrawCount()==draws&&audit.births==1&&audit.writes==failure-1);
        assert(result.find(result.handle())->y.bits==0x43100000u);
        assert(!world->enterLevelClear(0,false)&&audit.births==1&&world->randomDrawCount()==draws);
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                   GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        const auto maskBytes=read("generated/downwell-sprite-masks.dwsm");
        SpriteMaskPack masks(maskBytes.data(),maskBytes.size());assert(masks.valid());
        const auto x=MotionBinary64::fromInteger(240),y=MotionBinary64::fromInteger(200);
        InstanceHandle skeleton,bone,smoke,blood;GameMakerRandom random(2);
        // Deliberately create in a different order from the Step object table.
        assert(world->createBoneWithSourceId(706000,240,200,bone)&&
            owner->registeredSmoke().create(706001,x,y,random,smoke)&&
            world->createBloodWithSourceId(706002,x,y,{},[](InstanceHandle){return true;},blood)&&
            world->createSkeletonWithSourceId(706003,240,200,skeleton));
        auto phase=std::make_unique<GameplayCollisionOwner::EncounterStepOrder>();
        std::vector<int> order;unsigned hurts=0;bool born=false;
        auto tick=[&]{return owner->withEncounterSteps(*phase,[&](InstanceHandle h,int object){
            order.push_back(object);
            if(object==120){
                if(!born){born=true;InstanceHandle child;
                    assert(owner->registeredSmoke().create(706004,x,y,random,child));}
                return owner->stepSkeleton(h,true,{}, {},[&](auto&,auto&){return QuietSkeletonEffects{hurts};});
            }
            if(object==220)return owner->stepBlood(h,false,480,masks);
            if(object==225)return owner->stepSmoke(h);
            if(object==269)return owner->stepBone(h,true,[&](int sprite,unsigned& n){return sprites.frameCount(sprite,n);});
            return false;
        },[&]{world->step(0);return world->effectsSucceeded();});};
        assert(tick()&&(order==std::vector<int>{120,220,225,269}));
        order.clear();
        assert(tick()&&(order==std::vector<int>{120,220,225,225,269}));
        assert(hurts==0&&owner->valid());
    }
    // Exercise the real world tick, not dispatchStepPhase after the frame.
    // Synthetic births test the shared ordinary-Step creation cutoff.
    for(bool fail:{false,true})for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                   GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        InstanceHandle skeleton,bone;
        assert(world->createSkeletonWithSourceId(705000,240,200,skeleton)&&
            world->createBoneWithSourceId(705001,240,200,bone));
        struct Generation {
            bool born=false;unsigned calls=0;
            static bool run(void* context,GameplayWorld& world,GameMakerRandom&,int){
                auto& self=*static_cast<Generation*>(context);++self.calls;
                if(self.born)return true;
                self.born=true;InstanceHandle child;
                return world.createSkeletonWithSourceId(705002,240,200,child);
            }
        } generation;
        world->configureGenerationStep(Generation::run,&generation);
        auto phase=std::make_unique<GameplayCollisionOwner::SkeletonBoneStepOrder>();
        unsigned skeletonSteps=0,boneSteps=0,hurts=0;bool childBorn=false;
        auto tick=[&]{
            return owner->withSkeletonBoneSteps(*phase,[&](InstanceHandle h){
                assert(generation.calls>0);++skeletonSteps;
                if(fail)return false;
                if(!childBorn){
                    childBorn=true;InstanceHandle child;
                    assert(world->createBoneWithSourceId(705003,240,200,child));
                }
                return owner->stepSkeleton(h,true,{}, {},[&](auto&,auto&){return QuietSkeletonEffects{hurts};});
            },[&](InstanceHandle h){
                assert(skeletonSteps>boneSteps);++boneSteps;
                return owner->stepBone(h,true,[&](int sprite,unsigned& frames){return sprites.frameCount(sprite,frames);});
            },[&]{world->step(0);return world->effectsSucceeded();});
        };
        assert(tick()==!fail&&skeletonSteps==1&&boneSteps==(fail?0u:1u));
        if(fail){assert(!world->effectsSucceeded()&&!owner->valid());continue;}
        assert(tick()&&skeletonSteps==3&&boneSteps==3&&hurts==0);
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                   GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&resultRooms);
        assert(owner->attach(*world));int speed=60;assert(world->bindControllerRoomSpeed(speed));
        auto& controllers=owner->runControllers();InstanceHandle controller,shared;unsigned creates=0;
        assert(controllers.create(100065,{}, {},[&](InstanceHandle){++creates;return true;},controller)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::runController,controller,shared));
        GameplayCollisionView view(*world);checkTypedCollisionView(view,GameplayCollisionView::Kind::runController,controller);
        assert(owner->requestMomentDelay()&&speed==30&&controllers.find(controller)->alarms.get(0)==1);
        TimeChangeEffects<8> changes;
        assert(owner->createTimeChangeWithSourceId(changes,105187,{}, {},speed,[](unsigned ms){return ms==30;}));
        world->step(0);
        assert(world->effectsSucceeded()&&speed==60&&controllers.find(controller)->controllerSlow==0&&
            controllers.find(controller)->alarms.get(0)==0&&changes.size()==1);
        world->step(0);
        assert(world->effectsSucceeded()&&speed==60&&changes.size()==0&&creates==1);
        assert(controllers.activate(controller,false)&&owner->reconcileActivation());
        assert(owner->requestMomentDelay()&&speed==60);
        assert(controllers.activate(controller,true)&&owner->reconcileActivation());
        assert(owner->requestMomentDelay()&&speed==30);
        world->step(0);assert(world->effectsSucceeded()&&speed==60&&creates==1);
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::gameplay&&
            world->runControllers()==&controllers&&controllers.find(controller)&&creates==1);
        assert(owner->requestMomentDelay()&&speed==30);
        world->step(0);assert(world->effectsSucceeded()&&speed==60&&creates==1);
        assert(world->registerLevelExit(176+world->player().offsetY()));
        for(unsigned tick=0;tick<100;++tick)world->step(0);
        assert(world->levelResultRequested());
        const auto beforeInvalid=world->playerHandle();
        const auto beforeInvalidRandom=world->randomDrawCount();
        assert(!world->enterLevelClear(9,false)&&world->effectsSucceeded()&&
            world->findPlayer(beforeInvalid)&&world->randomDrawCount()==beforeInvalidRandom);
        // Result entry, not the later rmMain load, retires old-room ownership.
        assert(world->configureEnemy(0,112,163,240,300,0,0,5,14,0,1,-1,1));
        assert(world->configureEnemy(1,112,163,240,320,0,0,5,14,0,1,-1,1));
        const auto activeEnemy=world->enemyHandle(world->enemyAt(0).lifetimeId);
        const auto inactiveEnemy=world->enemyHandle(world->enemyAt(1).lifetimeId);
        auto& sleeping=const_cast<GameplayWorld::EnemyState&>(*world->findEnemy(inactiveEnemy));
        assert(owner->enemyBinding().activation(inactiveEnemy,sleeping,false));sleeping.active=false;
        assert(owner->reconcileActivation());
        InstanceHandle debris;
        assert(world->spawnDebris({}, {},1,debris));
        InstanceHandle oldPlayer,oldActive,oldInactive,oldDebris;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::player,world->playerHandle(),oldPlayer)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::enemy,activeEnemy,oldActive)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::enemy,inactiveEnemy,oldInactive)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,debris,oldDebris));
        assert(world->levelResultRequested()&&world->enterLevelClear(0,false));
        const auto resultLocal=world->resultInstance()->handle();
        const auto* result=world->resultInstance()->find(resultLocal);InstanceHandle resultKey;
        assert(result&&result->sourceId==102522&&result->x.bits==0x42a00000u&&result->y.bits==0x430e0000u&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::result,resultLocal,resultKey)&&view.contains(resultKey));
        checkTypedCollisionView(view,GameplayCollisionView::Kind::result,resultLocal);
        GameplayCollisionView::Record departed;
        for(auto old:{oldPlayer,oldActive,oldInactive,oldDebris})
            assert(!GameplayCollisionView(*world).resolve(old,departed));
        unsigned population=99;
        assert(owner->valid());
        if(mode==GameplayCollisionOwner::Ownership::reference)
            assert(owner->population(112,population)&&population==0);
        assert(controllers.find(controller)&&creates==1);
        assert(owner->requestMomentDelay()&&speed==30&&
            controllers.activate(controller,false)&&owner->reconcileActivation());
        bool heart=false;
        for(unsigned tick=0;tick<400&&!world->nextLevelRequested();++tick){
            if(tick==1)assert(controllers.activate(controller,true)&&owner->reconcileActivation());
            bool confirm=true;assert(world->stepLevelClear(confirm,false,false,heart,[&](auto effect,unsigned){
                if(effect==LevelClearSelection::Effect::dither)
                    assert(speed==60&&owner->requestMomentDelay()&&speed==30);
            }));
            if(tick==0||tick==120)assert(speed==30&&controllers.find(controller)->alarms.get(0)==1);
            if(tick==1||tick==121)assert(speed==60&&controllers.find(controller)->alarms.get(0)==0);
            std::int32_t resultY;RunnerImageValue expectedY;
            assert(world->levelClear().motion.instanceY(resultY)&&expectedY.assignInteger(resultY)&&
                world->resultInstance()->find(resultLocal)->y.bits==expectedY.bits);
        }
        assert(world->nextLevelRequested()&&owner->requestMomentDelay()&&speed==30);
        assert(controllers.activate(controller,false)&&owner->reconcileActivation());
        assert(world->prepareNextAreaOneLevel({},300)&&world->runStage().level==2&&
            controllers.find(controller)&&controllers.find(controller)->alarms.get(0)==1&&creates==1);
        assert(!world->resultInstance()->find(resultLocal)&&!view.contains(resultKey));
        world->step(0);assert(world->effectsSucceeded()&&speed==30&&controllers.find(controller)->alarms.get(0)==1);
        assert(controllers.activate(controller,true)&&owner->reconcileActivation());
        world->step(0);assert(world->effectsSucceeded()&&speed==60&&controllers.find(controller)->alarms.get(0)==0&&creates==1);
        assert(controllers.clear()&&!view.contains(shared));
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                   GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&resultRooms);
        assert(owner->attach(*world));int speed=60;assert(world->bindControllerRoomSpeed(speed));
        InstanceHandle controller;auto& controllers=owner->runControllers();
        assert(controllers.create(100065,{},{},[](InstanceHandle){return true;},controller));
        assert(world->registerLevelExit(176+world->player().offsetY()));
        for(unsigned tick=0;tick<100;++tick)world->step(0);
        assert(world->enterLevelClear(0,false));bool heart=false;
        for(unsigned tick=0;tick<120;++tick){bool confirm=true;
            assert(world->stepLevelClear(confirm,false,false,heart,[](auto,unsigned){}));}
        const auto camera=world->levelClear().motion.cameraY;
        assert(controllers.setAlarm(controller,1,1));unsigned events=0;bool confirm=true;
        assert(!world->stepLevelClear(confirm,false,false,heart,[&](auto,unsigned){++events;})&&
            !world->effectsSucceeded()&&events==2&&world->exitDither().active&&
            world->exitDither().alarm0==15&&world->levelClear().selection.alarm1==90&&
            controllers.find(controller)->alarms.get(1)==0&&world->levelClear().motion.cameraY.equal(camera));
        const auto tick=world->tick();
        assert(!world->stepLevelClear(confirm,false,false,heart,[](auto,unsigned){std::abort();})&&world->tick()==tick);
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                   GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));int speed=60;assert(world->bindControllerRoomSpeed(speed));
        InstanceHandle controller;
        assert(owner->runControllers().create(100065,{}, {},[](InstanceHandle){return true;},controller));
        InstanceHandle local,shared;const auto before=world->randomDrawCount();
        assert(owner->createBlast(700000,MotionBinary64{0x406e0800u,0},MotionBinary64::fromInteger(800),local));
        assert(speed==30&&world->randomDrawCount()==before+1&&world->cameraShakeAmount()==6);
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::blast,local,shared));
        GameplayCollisionView view(*world);GameplayCollisionView::Record record;
        assert(view.resolve(shared,record)&&record.blast==owner->registeredBlasts().find(local)&&record.blast->sourceId==700000);
        checkTypedCollisionView(view,GameplayCollisionView::Kind::blast,local);
        auto phase=std::make_unique<RunnerStepSnapshot<GameplayCollisionView::capacity>>();
        for(unsigned tick=1;tick<=28;++tick){
            world->step(0);assert(world->effectsSucceeded()&&owner->valid());
            unsigned count=0;
            assert(owner->dispatchStepPhase(0,*phase,[&](InstanceHandle selected,const PackedObjectEvent&){
                GameplayCollisionView::Kind kind;InstanceHandle key;
                assert(GameplayCollisionView::decode(selected,kind,key));
                if(kind!=GameplayCollisionView::Kind::blast)return true;
                ++count;return owner->stepBlast(selected);
            }));
            const auto* r=owner->registeredBlasts().find(local);
            assert(r&&r->state.damageEnd==(tick>=7)&&r->destroyRequested==(tick==28)&&count==(tick<28?1u:0u));
            assert(owner->prepareDraw());
            assert(view.contains(shared)==(tick<28));
        }
        InstanceHandle reused;assert(owner->createBlast(700001,{}, {},reused));
        assert(reused.slot==local.slot&&reused.generation!=local.generation&&!view.contains(shared));
        assert(owner->registeredBlasts().clear()&&owner->valid());
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                   GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));auto& smoke=owner->registeredSmoke();GameMakerRandom random(2);
        InstanceHandle local,shared;const auto x=MotionBinary64::fromInteger(240),y=MotionBinary64::fromInteger(192);
        assert(smoke.create(105187,x,y,random,local)&&GameplayCollisionView::encode(GameplayCollisionView::Kind::smoke,local,shared));
        GameplayCollisionView view(*world);GameplayCollisionView::Record record;
        assert(view.resolve(shared,record)&&record.smoke==smoke.find(local)&&record.smoke->sourceId==105187);
        checkTypedCollisionView(view,GameplayCollisionView::Kind::smoke,local);
        SmokeEffect oracle;GameMakerRandom oracleRandom(2);assert(oracle.create(x,y,oracleRandom));
        assert(smoke.setAlarm0(local,2));
        unsigned ticks=0;
        auto smokePhase=std::make_unique<RunnerStepSnapshot<GameplayCollisionView::capacity>>();
        while(!oracle.destroyRequested){
            assert(oracle.advanceImage());world->step(0);
            assert(world->effectsSucceeded()&&owner->valid());
            if(ticks==1)oracle.alarm0();
            if(!oracle.destroyRequested){
                auto expectedRandom=world->randomState();
                unsigned selectedSmoke=0;
                assert(owner->dispatchStepPhase(0,*smokePhase,[&](InstanceHandle selected,const PackedObjectEvent& event){
                    GameplayCollisionView::Kind kind;InstanceHandle local;
                    assert(GameplayCollisionView::decode(selected,kind,local));
                    if(kind!=GameplayCollisionView::Kind::smoke)return true;
                    assert(event.codeId==2015&&selected.slot==shared.slot&&selected.generation==shared.generation);
                    ++selectedSmoke;return owner->stepSmoke(selected);
                })&&selectedSmoke==1&&oracle.step(expectedRandom,[](auto speed,auto direction,auto& vx,auto& vy){
                    return SmokeDirection::vector(speed,direction,vx,vy,smokeTrig);
                }));
                assert(world->randomState().drawCount()==expectedRandom.drawCount());
                for(unsigned word=0;word<16;++word)assert(world->randomState().stateWord(word)==expectedRandom.stateWord(word));
            }
            const auto* current=smoke.find(local);
            assert(current&&current->state.imageIndex.bits==oracle.imageIndex.bits&&current->destroyRequested==oracle.destroyRequested&&
                current->x.bits==oracle.x.bits&&current->y.bits==oracle.y.bits&&current->state.noCollision==oracle.noCollision);
            assert(owner->prepareDraw());
            if(oracle.destroyRequested)assert(!smoke.find(local)&&!view.contains(shared));
            assert(++ticks<200);
        }
        InstanceHandle before,after;assert(smoke.create(105188,x,y,random,before));
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::gameplay&&
            world->registeredSmoke()==&smoke&&!smoke.find(before)&&owner->valid());
        assert(smoke.create(105189,x,y,random,after)&&after.generation>before.generation&&smoke.clear());
        InstanceHandle first,child;assert(smoke.create(105190,x,y,random,first));
        unsigned calls=0;
        assert(owner->dispatchStepPhase(0,*smokePhase,[&](InstanceHandle selected,const PackedObjectEvent&){
            GameplayCollisionView::Kind kind;InstanceHandle local;assert(GameplayCollisionView::decode(selected,kind,local));
            if(kind!=GameplayCollisionView::Kind::smoke)return true;
            ++calls;assert(smoke.create(105191,x,y,random,child));return owner->stepSmoke(selected);
        })&&calls==1&&smoke.find(first)->state.allSet&&!smoke.find(child)->state.allSet);
        calls=0;
        assert(owner->dispatchStepPhase(0,*smokePhase,[&](InstanceHandle selected,const PackedObjectEvent&){
            GameplayCollisionView::Kind kind;InstanceHandle local;assert(GameplayCollisionView::decode(selected,kind,local));
            if(kind!=GameplayCollisionView::Kind::smoke)return true;
            ++calls;return owner->stepSmoke(selected);
        })&&calls==2&&smoke.find(child)->state.allSet&&smoke.clear());
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native})
    for(unsigned reject=0;reject<3;++reject){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));auto& smoke=owner->registeredSmoke();GameMakerRandom random(2);
        InstanceHandle local,self,walls[2];assert(smoke.create(105187,{}, {},random,local)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::smoke,local,self));
        for(unsigned i=0;i<2;++i){assert(owner->terrain().append(60,int(i)*16,200,true)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,
                owner->terrain().handle(owner->terrain().size()-1),walls[i]));}
        auto oracle=smoke.find(local)->state;const auto draws=world->randomDrawCount();
        for(unsigned i=0;i<3;++i){
            if(i==1){assert(smoke.alarmEvent(local,0));oracle.alarm0();}
            RunnerCollisionEventContext context;
            const bool okay=context.dispatch(self,walls[i%2],{true,false},[&](auto,auto,int& subtype,bool& found,PackedObjectEvent& event){
                subtype=60;return objects.collisionEvent(225,60,found,event);
            },[&](auto event){
                if(reject==1)event.code=2016;
                if(reject==2)++event.other.generation;
                return owner->dispatchSmokeCollision(event);
            });
            assert(okay==(reject==0));if(!okay)break;
            assert(oracle.collideWall());const auto* r=smoke.find(local);
            assert(r&&r->state.speed.high==oracle.speed.high&&r->state.speed.low==oracle.speed.low&&
                r->state.imageSpeed.bits==oracle.imageSpeed.bits);
        }
        assert(world->randomDrawCount()==draws&&owner->valid()==(reject==0));
        if(reject){world->step(0);assert(!world->effectsSucceeded());}
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
        GameplayCollisionOwner::Ownership::nativeScenery})
    for(unsigned reject=0;reject<3;++reject){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));auto& particles=owner->consumptionParticles();GameMakerRandom random(2);
        struct Globals {
            MotionBinary64 grav{};
            bool gravity(MotionBinary64 v){grav=v;return true;}
            MotionBinary64 gravity(){return grav;}MotionBinary64 fallSpeed(){return {};}
        } globals;
        InstanceHandle local,self,wall;
        assert(particles.create(105187,{}, {},random,globals,local)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::consumptionBlood,local,self));
        assert(owner->terrain().append(60,0,200,true)&&GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,
            owner->terrain().handle(owner->terrain().size()-1),wall));
        const auto draws=world->randomDrawCount();RunnerCollisionEventContext context;
        const bool ok=context.dispatch(self,wall,{true,false},[&](auto,auto,int& subtype,bool& found,PackedObjectEvent& event){
            subtype=60;return objects.collisionEvent(223,60,found,event);
        },[&](auto event){
            if(reject==1)event.code=2007; // A water event cannot be substituted for a wall event.
            if(reject==2)++event.other.generation;
            return owner->dispatchConsumptionBloodCollision(event);
        });
        assert(ok==(reject==0)&&owner->valid()==ok&&world->randomDrawCount()==draws);
        const auto* record=particles.find(local);
        assert(record&&record->destroyRequested==ok&&record->state.destroyRequested==ok);
        if(ok){assert(owner->prepareDraw()&&!particles.find(local));}
        else {world->step(0);assert(!world->effectsSucceeded());}
    }
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native})
    for(unsigned failure=0;failure<4;++failure){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,ownership);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        CurrencySpawnSource<std::uint32_t> caller;
        caller.objectIndex=92;caller.x=RunnerImageValue{0x43704000u}.promoted();
        caller.y=RunnerImageValue{0x44482000u}.promoted();
        struct Queries {
            unsigned failure,tiles=0;
            bool wall(InstanceHandle,MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
            bool tile(InstanceHandle,int layer,MotionBinary64,MotionBinary64,bool& hit){
                assert(layer==100);hit=false;return !(++tiles==2&&failure==2);
            }
        } queries{failure};
        unsigned allocations=0,publications=0;
        auto expectedRandom=world->randomState();
        auto spawn=[&]{return world->spawnRegisteredCurrency(objects,
            [&](auto& out){out=caller;return true;},
            [&](auto amount){caller.moniAmount=amount;return true;},
            [&](bool attracted){caller.atrState=attracted;return true;},
            [&](std::uint32_t id){
                if(++publications==2&&failure==3)return false;
                caller.myGem=id;
                return caller.x.add(MotionBinary64::fromInteger(16));
            },MotionBinary64::fromInteger(14),
            [&](std::uint32_t& id){id=805000+allocations++;return !(allocations==2&&failure==1);},queries);};
        assert(spawn()==(failure==0));
        const unsigned births=failure==1?1:failure?2:3;
        assert(owner->currency().size()==births&&world->effectsSucceeded()==(failure==0));
        struct OracleQueries {
            bool wall(MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
            bool tile(MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
            bool publish(RunnerImageValue,RunnerImageValue){return true;}
        } oracleQueries;
        for(unsigned i=0;i<births;++i){
            const auto key=owner->currency().handleForSourceId(805000+i);
            const auto* child=owner->currency().find(key);assert(child&&child->sourceId==805000+i);
            CurrencyCreate oracle;
            const auto x=RunnerImageValue{0x43704000u}.promoted();auto birthX=x;
            assert(birthX.add(MotionBinary64::fromInteger(16*int(i)))&&
                oracle.create(birthX,caller.y,1,false,expectedRandom,oracleQueries));
            assert(child->x.promoted().high==birthX.high&&child->x.promoted().low==birthX.low&&
                child->state.xsp.high==oracle.xsp.high&&child->state.xsp.low==oracle.xsp.low);
            assert(child->state.getAmount==(i==0?10:2)&&child->state.size==(i==0?1:0));
        }
        assert(world->randomState().drawCount()==expectedRandom.drawCount()&&
            world->randomState().stateIndex()==expectedRandom.stateIndex());
        for(unsigned word=0;word<16;++word)assert(world->randomState().stateWord(word)==expectedRandom.stateWord(word));
        assert(caller.myGem==(failure?805000u:805002u)&&!caller.atrState&&
            caller.moniAmount.high==MotionBinary64::fromInteger(failure?4:0).high&&!caller.moniAmount.low);
        if(failure){
            const auto draws=world->randomState().drawCount();
            assert(!spawn()&&world->randomState().drawCount()==draws&&owner->currency().size()==births);
        }else{
            const auto old=owner->currency().handleForSourceId(805000);
            assert(owner->currency().clear()&&!owner->currency().find(old)&&
                !owner->currency().handleForSourceId(805000).generation);
        }
    }
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
        GameplayCollisionOwner::Ownership::nativeScenery})
    for(unsigned failure=0;failure<4;++failure){
        const auto roomBytes=read("generated/downwell-rooms.dwrm");RoomPack rooms(roomBytes.data(),roomBytes.size());
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,ownership);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        const int x=240,y=800;assert(world->appendBreakables(&x,&y,1));
        const InstanceHandle box{0,world->breakableSlot(0).lifetimeId};
        RunnerImageValue px{0x43704000u},py{0x44482000u};
        if(ownership!=GameplayCollisionOwner::Ownership::reference){
            assert(px.assignInteger(x+240)&&py.assignInteger(y+176));
        }
        assert(owner->breakableBinding().publishedPosition(box,*world->findBreakable(box),px,py)&&
            world->markBreakableDestroyed(box));
        struct Queries {
            unsigned failure;
            bool wall(InstanceHandle,MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
            bool tile(InstanceHandle,int,MotionBinary64,MotionBinary64,bool& hit){hit=false;return failure!=2;}
        } queries{failure};
        unsigned births=0;InstanceHandle replacement;
        const bool completed=world->spawnBoxCurrency(objects,box,MotionBinary64::fromInteger(10),
            [&](InstanceHandle h,auto& outX,auto& outY){
                if(failure==1)return false;
                if(failure==3){
                    world->configureBreakables(&x,&y,1);
                    replacement={0,world->breakableSlot(0).lifetimeId};
                    outX=px.promoted();outY=py.promoted();return true;
                }
                InstanceHandle shared;BoxDestructionScript::Caller caller;
                if(!GameplayCollisionView::encode(GameplayCollisionView::Kind::breakable,h,shared)||
                    !owner->readBoxDestruction(shared,caller))return false;
                outX=caller.x;outY=caller.y;return true;
            },[&](std::uint32_t& id){id=806000+births++;return true;},queries);
        if(completed!=(failure==0))fprintf(stderr,"box reward failure %d %u %u %d\n",int(ownership),failure,births,int(owner->valid()));
        assert(completed==(failure==0));
        if(failure==3){
            const auto* fresh=world->boxReward(replacement);
            assert(replacement.generation!=box.generation&&!world->boxReward(box)&&fresh&&
                fresh->moniAmount.zero()&&!fresh->myGem&&!births);
            continue;
        }
        const auto* reward=world->boxReward(box);
        assert(reward&&!reward->atrState&&world->findBreakable(box)->destroyRequested);
        assert(reward->myGem==(failure?0u:806000u)&&
            reward->moniAmount.high==MotionBinary64::fromInteger(failure?10:0).high&&!reward->moniAmount.low);
        assert(births==(failure==1?0u:1u));
        if(births){
            const auto* gem=owner->currency().find(owner->currency().handleForSourceId(806000));
            assert(gem&&gem->x.bits==px.bits&&gem->y.bits==py.bits&&
                gem->state.getAmount==(failure?2:10)&&gem->state.size==(failure?0:1));
        }
        if(!failure){
            assert(world->retireMarkedBreakable(box)&&!world->boxReward(box));
            world->configureBreakables(&x,&y,1);
            replacement={0,world->breakableSlot(0).lifetimeId};
            const auto* fresh=world->boxReward(replacement);
            assert(fresh&&fresh->moniAmount.zero()&&!fresh->myGem&&!fresh->atrState);
            if(ownership!=GameplayCollisionOwner::Ownership::reference)
                assert(!owner->breakableBinding().publishedPosition(replacement,*world->findBreakable(replacement),
                    RunnerImageValue{0x43704000u},py)&&!owner->valid());
        }
    }
    std::vector<bool> boxFieldReference;
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
        GameplayCollisionOwner::Ownership::nativeScenery}){
        const auto rb=read("generated/downwell-rooms.dwrm"),mb=read("generated/downwell-sprite-masks.dwsm");
        RoomPack rooms(rb.data(),rb.size());SpriteMaskPack masks(mb.data(),mb.size());
        PackedObject boxObject;PackedSpriteCollision boxShape;SpriteMaskPack::Mask boxMask;
        assert(objects.findObject(92,boxObject)&&boxObject.textureMaskId==585&&sprites.sprite(585,boxShape)&&
            boxShape.originX==8&&boxShape.originY==8&&boxShape.marginLeft==0&&boxShape.marginTop==0&&
            boxShape.marginRight==15&&boxShape.marginBottom==15&&boxShape.collisionMaskCount==1&&
            masks.mask(585,0,boxMask)&&boxMask.width==16&&boxMask.height==16);
        for(int x=0;x<16;++x)for(int y=0;y<16;++y)assert(boxMask.contains(x,y));
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,ownership);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world)&&world->createRoomStartField()&&
            world->createGeneratedInstance({82,240,512},objects,[](const auto&){return true;})&&world->registerLevelExit(1024));
        const int offsets[]={-129,-128,-127,-25,-24,-23,0,23,24,25,127,128,129};
        std::vector<bool> results;
        for(int centre:{176,512,1024})for(int dx:offsets)for(int dy:offsets){
            const int x=dx,y=centre+dy-176;assert(world->appendBreakables(&x,&y,1));
            const unsigned slot=world->breakableSlots()-1;const InstanceHandle box{slot,world->breakableSlot(slot).lifetimeId};
            bool allowed=false;
            assert(owner->boxTimeAllowed(box,false,480,masks,allowed)&&allowed);
            assert(owner->boxTimeAllowed(box,true,480,masks,allowed));results.push_back(allowed);
            assert(world->publishBoxAlarm0(box,4)&&owner->stopBoxAlarm0(box,true,480,masks)&&
                world->findBreakable(box)->alarm0==(allowed?4:5));
            assert(world->markBreakableDestroyed(box));bool retained=!allowed;
            const bool queried=owner->boxTimeAllowed(box,true,480,masks,retained);
            const bool outside=dx < -80||dx > 80;
            assert(queried&&retained==outside);results.push_back(retained);
            assert(world->publishBoxAlarm0(box,4)&&owner->stopBoxAlarm0(box,true,480,masks)&&
                world->findBreakable(box)->alarm0==(outside?4:5)&&world->retireMarkedBreakable(box));
        }
        if(ownership==GameplayCollisionOwner::Ownership::reference)boxFieldReference=results;
        else assert(results==boxFieldReference);
        assert(std::find(results.begin(),results.end(),true)!=results.end()&&
            std::find(results.begin(),results.end(),false)!=results.end());
        struct Queries {
            bool wall(InstanceHandle,MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
            bool tile(InstanceHandle,int,MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
        } queries;
        InstanceHandle gem;const auto gx=MotionBinary64::fromInteger(240),gy=MotionBinary64::fromInteger(512);
        assert(world->createCurrencyWithSourceId(806900,gx,gy,queries,gem));bool hit=false;
        assert(owner->currencyFieldMeeting(gem,gx,gy,masks,hit)&&hit&&owner->currency().markDestroyed(gem)&&
            owner->currencyFieldMeeting(gem,gx,gy,masks,hit)&&!hit);
    }
    for(unsigned mode=0;mode<16;++mode){
        const bool fullEffects=mode!=0;
        const bool rewardSuccess=mode==12||mode==14;
        const auto roomBytes=read("generated/downwell-rooms.dwrm");RoomPack roomPack(roomBytes.data(),roomBytes.size());
        const auto maskBytes=read("generated/downwell-sprite-masks.dwsm");SpriteMaskPack boxMasks(maskBytes.data(),maskBytes.size());
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&roomPack,
            mode>=14?GameplayCollisionOwner::Ownership::nativeScenery:GameplayCollisionOwner::Ownership::reference);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        const int x=0,y=336;BreakableState creation;
        if(mode>=12)creation.haveGem=2;
        assert(world->appendBreakables(&x,&y,1,&creation));
        const InstanceHandle box{0,1};
        ScheduledBoxCounts counts;
        auto tick=[&]{return world->withBoxDestruction(
            [&](InstanceHandle key,BoxDestructionScript::Caller& caller){
                InstanceHandle shared;
                return GameplayCollisionView::encode(GameplayCollisionView::Kind::breakable,key,shared)&&
                    owner->readBoxDestruction(shared,caller);
            },[&](InstanceHandle key){return scheduledBoxEffects(*world,key,counts,
                [&](InstanceHandle h){return world->markBreakableDestroyed(h);});},
            [&]{world->step(0);return world->effectsSucceeded();});};
        assert(tick()&&counts.stop==1&&!counts.sound&&world->findBreakable(box));
        counts={};assert(world->publishBoxHealth(box,MotionBinary64::fromInteger(mode==10||mode==11?1:0)));
        if(mode==10||mode==11)assert(world->publishBoxAlarm0(box,2));
        if(fullEffects){
            const unsigned soundFailure=mode>=3&&mode<12?mode-2:0;
            BoxSoundVerification::Audio<GameplayWorld> audio{*world,world->findBreakable(box)->state.breakSound,soundFailure};
            GameMakerRandom soundRandom{0};
            bool boxAllowed=false;unsigned queries=0;
            if(mode==2)for(unsigned i=0;i<debrisEffectCapacity;++i){
                InstanceHandle child;
                assert(world->spawnDebris(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),1,child)&&
                    owner->debris().find(child));
            }
            auto stopped=[](auto,const auto&,bool& frozen){frozen=false;return true;};
            auto meeting=[](auto,const auto&,auto,auto,bool& hit){hit=false;return true;};
            struct RewardQueries {
                unsigned mode;
                bool wall(InstanceHandle,MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
                bool tile(InstanceHandle,int,MotionBinary64,MotionBinary64,bool& hit){hit=false;return mode!=13&&mode!=15;}
            } rewardQueries{mode};
            unsigned rewardBirths=0;
            const bool completed=world->withBoxDestruction([&](InstanceHandle key,BoxDestructionScript::Caller& caller){
                InstanceHandle shared;
                return GameplayCollisionView::encode(GameplayCollisionView::Kind::breakable,key,shared)&&
                    owner->readBoxDestruction(shared,caller);
            },[&](InstanceHandle key){
                auto callbacks=scheduledBoxEffects(*world,key,counts,[&](InstanceHandle h){return world->markBreakableDestroyed(h);});
                callbacks.verifyDebris=mode<12;
                callbacks.timeQuery=[&](InstanceHandle h,bool& allowed){
                    ++queries;if(mode==11)return false;
                    const bool ok=owner->boxTimeAllowed(h,mode==10,480,boxMasks,allowed);
                    if(ok)boxAllowed=allowed;return ok;};
                callbacks.sawSound=mode<10&&key.slot==box.slot&&key.generation==box.generation;callbacks.before=world->randomState();
                if(callbacks.sawSound||mode>=12)soundRandom=world->randomState();
                auto rewards=world->boxCurrencyOperations(objects,key,callbacks,
                    [&](InstanceHandle h,auto& px,auto& py){
                        InstanceHandle shared;BoxDestructionScript::Caller caller;
                        if(!GameplayCollisionView::encode(GameplayCollisionView::Kind::breakable,h,shared)||
                            !owner->readBoxDestruction(shared,caller))return false;
                        px=caller.x;py=caller.y;return true;
                    },[&](std::uint32_t& id){id=807000+rewardBirths++;return true;},rewardQueries);
                return boxWorldEffects(*world,key,rewards,
                [&](int id,unsigned& frames){PackedSpriteCollision art;if(!sprites.sprite(id,art))return false;
                    frames=art.frameCount;return true;},audio,MotionBinary64{0x3fd00000u,0},false);},
                [&]{return world->stepWithOwnedDebris(0,stopped,meeting);});
            assert(completed==(mode==1||mode==10||rewardSuccess));
            if(mode>=12){
                const auto* reward=world->boxReward(box);
                const auto* gem=owner->currency().find(owner->currency().handleForSourceId(807000));
                assert(rewardBirths==1&&reward&&gem&&counts.mark==1&&audio.calls==7&&
                    world->findBreakable(box)->destroyRequested&&world->debrisEffects().size()==2&&
                    world->smokeEffects().size()==1&&world->scriptEffects().size()==1);
                assert(reward->myGem==(rewardSuccess?807000u:0u)&&
                    reward->moniAmount.high==MotionBinary64::fromInteger(rewardSuccess?0:10).high&&
                    gem->state.getAmount==(rewardSuccess?10:2)&&gem->state.size==(rewardSuccess?1:0)&&
                    !gem->state.allSet&&gem->state.alarms.get(0)==10&&gem->state.alarms.get(3)==15&&
                    counts.stop==(rewardSuccess?1u:0u));
                SmokeEffects<1> smoke;FlashBallEffects<1> flash;DebrisEffects<2> debris;
                const auto px=MotionBinary64::fromInteger(240),py=MotionBinary64::fromInteger(512);
                assert(smoke.boxBurst(px,MotionBinary64::fromInteger(516),soundRandom)&&flash.boxBurst(px,py,soundRandom)&&
                    debris.create(px,py,1,soundRandom)&&debris.create(px,py,1,soundRandom));
                struct CreateQueries {
                    bool wall(MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
                    bool tile(MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
                    bool publish(RunnerImageValue,RunnerImageValue){return true;}
                } createQueries;
                CurrencyCreate expectedGem;assert(expectedGem.create(px,py,1,false,soundRandom,createQueries));
                assert(soundRandom.drawCount()==world->randomState().drawCount()&&
                    soundRandom.stateIndex()==world->randomState().stateIndex());
                for(unsigned i=0;i<16;++i)assert(soundRandom.stateWord(i)==world->randomState().stateWord(i));
                if(rewardSuccess)assert(owner->prepareDraw()&&!world->findBreakable(box)&&owner->currency().size()==1);
                continue;
            }
            if(mode>=10){
                if(mode==10)assert(!boxAllowed);
                const auto* retained=world->findBreakable(box);
                assert(queries==1&&retained&&!retained->destroyRequested&&!counts.mark&&audio.calls==0&&
                    !world->debrisEffects().size());
                assert(retained->alarm0==(mode==10&&!boxAllowed?2:1)&&retained->state.scriptIndex==(mode==10?1:0));
                continue;
            }
            if(soundFailure){
                assert(!world->effectsSucceeded()&&audio.valid&&audio.calls==soundFailure&&
                    world->findBreakable(box)&&!world->findBreakable(box)->destroyRequested&&!counts.mark&&counts.stop==0&&
                    !world->smokeEffects().size()&&!world->flashEffects().slot(0).handle&&!world->scriptEffects().size()&&
                    !world->debrisEffects().size());
                assert(soundRandom.drawCount()==world->randomState().drawCount()&&
                    soundRandom.stateIndex()==world->randomState().stateIndex());
                for(unsigned i=0;i<16;++i)assert(soundRandom.stateWord(i)==world->randomState().stateWord(i));
                continue;
            }
            if(mode==2){
                assert(!world->effectsSucceeded()&&world->debrisEffects().size()==debrisEffectCapacity&&
                    world->findBreakable(box)&&world->findBreakable(box)->destroyRequested&&
                    audio.valid&&audio.calls==7&&counts.mark==1&&counts.stop==0&&
                    world->smokeEffects().size()==1&&world->scriptEffects().size()==1);
                continue;
            }
            assert(world->effectsSucceeded()&&world->debrisEffects().size()==2&&
                world->smokeEffects().size()==1&&world->flashEffects().slot(0).handle&&world->scriptEffects().size()==1&&
                counts.mark==1&&audio.valid&&audio.calls==7&&counts.stop==1&&world->findBreakable(box)->destroyRequested);
            assert(owner->prepareDraw()&&!world->findBreakable(box));
            assert(world->stepWithOwnedDebris(0,stopped,meeting)&&world->debrisEffects().size()==2);
            continue;
        }
        assert(!tick());
        assert(!world->effectsSucceeded()&&counts.sound==1&&counts.smoke==1&&counts.flash==1&&
            counts.impact==1&&counts.mark==1&&counts.debris==1&&counts.stop==0&&
            world->smokeEffects().size()==1&&world->flashEffects().slot(0).handle&&world->scriptEffects().size()==1&&
            world->findBreakable(box)&&world->findBreakable(box)->destroyRequested);
    }
    for(bool native:{false,true})for(unsigned family=0;family<6;++family){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},nullptr,
            native?GameplayCollisionOwner::Ownership::native:GameplayCollisionOwner::Ownership::reference);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world)&&owner->valid());
        // Fail before any family binding callback: enclosing validation must
        // observe this immediately, not only at the next simulation boundary.
        switch(family){
            case 0:assert(!owner->terrain().append(-1,0,0,true));break;
            case 1:{ScriptSpawnEffect effect;assert(!owner->scriptEffects().birth(0,effect));break;}
            case 2:{DebrisAnimation effect;assert(!owner->debris().birth(0,effect));break;}
            case 3:{std::uint32_t id=0;assert(!owner->timeChanges().birth(0,{},{},id));break;}
            case 4:{InstanceHandle key;auto camera=[](auto&){assert(false);return false;};
                assert(!owner->entryDithers().create(0,camera,camera,key));break;}
            case 5:assert(!owner->droneBullets().advanceAnimation({}));break;
        }
        assert(!owner->valid());
        unsigned callbacks=0;
        assert(!owner->visitRegistrationOrder([&](auto,auto,auto){++callbacks;return true;})&&callbacks==0);
        world->step(0);assert(!world->effectsSucceeded());
    }
    {
        ObjectAlarmMetadata<386> metadata;unsigned mask=0;
        assert(!metadata.mask(0,mask)&&metadata.bind(objects));
        ObjectAlarmMetadata<386,388> tooSmall;
        assert(!tooSmall.bind(objects)&&!tooSmall.mask(0,mask));
        ObjectAlarmMetadata<386,389> exact;
        assert(exact.bind(objects));
        unsigned inherited=0;
        for(unsigned pass=0;pass<2;++pass)for(unsigned id=0;id<objects.objectCount();++id){
            assert(metadata.mask(int(id),mask));
            for(unsigned alarm=0;alarm<12;++alarm){
                PackedObjectEvent event;
                const bool found=objects.findEvent(int(id),2,int(alarm),event);
                assert(bool(mask&(1u<<alarm))==found);
                for(unsigned repeat=0;repeat<2;++repeat){
                    PackedObjectEvent cached{};
                    assert(metadata.findEvent(int(id),alarm,cached)==found);
                    if(found)assert(cached.ownerObjectId==event.ownerObjectId&&
                        cached.eventType==event.eventType&&cached.eventSubtype==event.eventSubtype&&cached.codeId==event.codeId);
                    PackedObjectEvent prepared{};
                    assert(exact.findEvent(int(id),alarm,prepared)==found);
                    if(found)assert(prepared.ownerObjectId==event.ownerObjectId&&prepared.codeId==event.codeId);
                }
                if(found&&event.ownerObjectId!=int(id))++inherited;
            }
        }
        assert(inherited&&!metadata.mask(-1,mask)&&!metadata.mask(int(objects.objectCount()),mask));
        ObjectPack invalid(nullptr,0);
        assert(!metadata.bind(invalid)&&!metadata.mask(0,mask));
        assert(metadata.bind(objects)&&metadata.mask(266,mask));
        assert(mask&1u);
        PackedObjectEvent cached{};
        assert(metadata.findEvent(266,0,cached)&&!metadata.findEvent(-1,0,cached)&&
            !metadata.findEvent(386,0,cached)&&!metadata.findEvent(266,12,cached));
        auto noAlarms=ob;
        for(unsigned i=0;i<objects.bindingCount();++i){
            PackedObjectEvent event;assert(objects.binding(i,event));
            if(event.eventType==2)noAlarms[ObjectPack::headerSize+
                objects.objectCount()*ObjectPack::objectRecordSize+i*ObjectPack::bindingRecordSize]=3;
        }
        ObjectPack replacement(noAlarms.data(),noAlarms.size());
        assert(metadata.bind(replacement)&&metadata.mask(266,mask)&&mask==0&&!metadata.findEvent(266,0,cached));
        assert(!metadata.bind(invalid)&&!metadata.findEvent(266,0,cached));
        assert(metadata.bind(objects)&&metadata.findEvent(266,0,cached));
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                   GameplayCollisionOwner::Ownership::nativeScenery}){
        const auto roomBytes=read("generated/downwell-rooms.dwrm");RoomPack rooms(roomBytes.data(),roomBytes.size());
        TimeStopEffects<64> stop;PackedRoom room;assert(rooms.findRoom(11,room));
        const auto maskBytes=read("generated/downwell-sprite-masks.dwsm");SpriteMaskPack masks(maskBytes.data(),maskBytes.size());
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),
            mode==GameplayCollisionOwner::Ownership::reference?&owner->terrain():nullptr,&rooms);
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&
            (mode==GameplayCollisionOwner::Ownership::reference||world->bindNativeTerrain(*terrain))&&
            world->createTimeFieldWithSourceId(820000,240,176));
        ContactDamageScripts damageCaller;unsigned selections=0;
        auto damageBinding=bindContactDamagePlayer(damageCaller,*world,[&](InstanceHandle& selected){
            assert(world->playerDamaged());++selections;selected=world->playerHandle();return true;
        });
        DamageLiveStateEvents damageEvents;damageEvents.timeout=false;damageEvents.immune=true;
        const auto one=MotionBinary64::fromInteger(1);
        auto noBirth=[](std::uint32_t&){return false;};
        assert(world->typicalPlayerDamage(damageBinding,one,one,one,damageEvents,noBirth)&&world->playerDamaged());
        assert(damageEvents.alarm==-1&&!damageEvents.clears&&!world->noDamageAchievement()&&!world->areaNoDamageAchievement());
        assert(world->typicalPlayerDamage(damageBinding,one,one,one,damageEvents,noBirth)&&!damageEvents.clears&&selections==1);
        assert(world->setPlayerDamageAlarm(world->playerHandle(),2));
        world->setPlayerHit();assert(world->playerHit()==1);
        assert(world->setOxygen({0x40230000u,0}));
        unsigned next=830000;std::uint8_t input=0;std::vector<PlayerTimeStop::Effect> events;
        auto encounterPhase=std::make_unique<GameplayCollisionOwner::EncounterStepOrder>();
        unsigned shardSteps=0;
        const auto tick=[&]{return owner->withPlayerTimeStop(stop,room,masks,false,
            [&](std::uint32_t& id){id=next++;return true;},
            [&](PlayerTimeStop::AudioQuery,bool& answer){answer=true;return true;},
            [&](PlayerTimeStop::Effect effect){events.push_back(effect);return true;},
            [&]{return owner->withEncounterSteps(*encounterPhase,[&](InstanceHandle h,int object){
                if(object!=238)return false;
                ++shardSteps;return owner->stepTimeShard(h,stop.player.stopped,masks);
            },[&]{world->step(input);return world->effectsSucceeded();});});};
        assert(tick()&&stop.player.stopped&&stop.player.checkStop&&!stop.player.bgmOn);
        // Begin Step births join this ordinary Step and read its freshly
        // published stop state; they do not wait for the following frame.
        assert(shardSteps==6);
        assert(owner->timeShards().find(owner->timeShards().handle(1))->imageSpeed.bits==0);
        assert(world->playerDamaged()&&world->playerHit()==2);
        assert(world->oxygen().high==0x40240000u&&!world->oxygen().low);
        assert(next==830007&&owner->timeShards().size()==6&&owner->timeChanges().size()==1);
        assert((events==std::vector<PlayerTimeStop::Effect>{PlayerTimeStop::Effect::shake,
            PlayerTimeStop::Effect::change,PlayerTimeStop::Effect::pauseMusic,
            PlayerTimeStop::Effect::enterSound,PlayerTimeStop::Effect::oxygenFloor}));
        const auto* change=owner->timeChanges().find(owner->timeChanges().handle(1));
        assert(change&&change->sourceId==830003&&change->alarms.get(1)==1&&stop.roomSpeed==30);
        assert(stop.burst.field==820000);
        events.clear();assert(tick()&&next==830007&&stop.roomSpeed==35);
        assert(!world->playerDamaged()&&!world->playerHit());
        assert((events==std::vector<PlayerTimeStop::Effect>{PlayerTimeStop::Effect::oxygenFloor}));
        Input right;right.right=true;input=packReplayInput(right);
        for(unsigned i=0;i<60&&stop.player.stopped;++i){events.clear();assert(tick());}
        assert(!stop.player.stopped&&!stop.player.checkStop&&stop.player.bgmOn);
        assert(next==830014&&owner->timeShards().size()==12);
        assert((events==std::vector<PlayerTimeStop::Effect>{PlayerTimeStop::Effect::change,
            PlayerTimeStop::Effect::resumeMusic,PlayerTimeStop::Effect::stopWind,
            PlayerTimeStop::Effect::exitSound,PlayerTimeStop::Effect::duckMusic}));
        if(mode==GameplayCollisionOwner::Ownership::nativeScenery){
            InstanceHandle controller;
            assert(owner->runControllers().create(900001,{}, {},[](InstanceHandle){return true;},controller));
            DamageControllerFailureEvents failure;const int health=world->health();
            assert(!world->typicalPlayerDamage(damageBinding,one,one,one,failure,noBirth)&&!world->effectsSucceeded());
            assert(world->health()==health-1&&world->playerHit()==1&&!world->playerDamaged());
        }else{
        DamageSoundFailureEvents failedSound;const int health=world->health();
        const auto numbers=world->damageNumbers().size();
        assert(!world->typicalPlayerDamage(damageBinding,one,one,one,failedSound,noBirth)&&!world->effectsSucceeded());
        assert(world->health()==health-1&&world->damageNumbers().size()==numbers+1&&
            failedSound.soundCalls==1&&!world->playerDamaged()&&!world->playerHit());
        }
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                   GameplayCollisionOwner::Ownership::nativeScenery})for(bool active:{false,true}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));int speed=60;assert(world->bindControllerRoomSpeed(speed));
        InstanceHandle controller;
        assert(owner->runControllers().create(100065,{}, {},[](InstanceHandle){return true;},controller));
        assert(owner->runControllers().activate(controller,active)&&owner->reconcileActivation());
        ContactDamageScripts caller;DamageControllerFailureEvents events;
        auto bound=bindContactDamagePlayer(caller,*world,[&](InstanceHandle& h){h=world->playerHandle();return true;});
        // Its external controller callback fails. Registered damage must use
        // actual ownership instead, including empty/inactive WITH populations.
        assert(caller.player.executeWithState(MotionBinary64::fromInteger(1),events,bound.state));
        assert(world->effectsSucceeded()&&world->playerHit()==1&&world->playerDamaged()&&world->health()==3);
        const auto* record=owner->runControllers().find(controller);
        assert(speed==(active?40:60)&&record->controllerSlow==(active?23:0)&&record->alarms.get(0)==(active?40:-1));
        if(active){
            for(unsigned tick=0;tick<40;++tick)assert(world->advanceControllerRoomAlarm(controller,0));
            assert(speed==60&&record->controllerSlow==0);
        }
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                   GameplayCollisionOwner::Ownership::nativeScenery})for(bool upgraded:{false,true})for(bool failJump:{false,true}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        auto& inventory=const_cast<UpgradeInventory&>(world->upgrades());
        inventory.levels[7]=inventory.levels[17]=upgraded?1:0;
        ContactDamageScripts caller;
        auto bound=bindContactDamagePlayer(caller,*world,[&](InstanceHandle& h){h=world->playerHandle();return true;});
        struct Events:DamageLiveStateEvents {
            bool candle=false,failJump=false;bool candleOwned(){return candle;}
            bool setVertical(Number value){vy=value;return !failJump;}
            bool lengthdirX(Number,Number,Number&){return false;}
        } events;
        events.candle=events.timeout=!upgraded;
        events.failJump=failJump;
        unsigned births=0;
        const auto one=MotionBinary64::fromInteger(1);
        const auto fields=world->timeFields().size();const auto draws=world->randomDrawCount();
        assert(world->noDamageAchievement()&&world->areaNoDamageAchievement());
        assert(world->typicalPlayerDamage(bound,one,one,one,events,[&](std::uint32_t& id){id=910000+births++;return true;})==!failJump);
        assert(events.vx.high==one.high&&events.vx.low==one.low);
        assert(world->noDamageAchievement()==failJump&&world->areaNoDamageAchievement()==failJump&&!events.clears);
        if(failJump){assert(world->timeFields().size()==fields&&world->randomDrawCount()==draws&&
            !births&&world->playerDamaged());continue;}
        assert(caller.player.hitInvulnerability==(upgraded?240:90)&&events.players==(upgraded?2u:0u)&&births==unsigned(upgraded));
        bool pending=false;assert(world->playerDamageAlarmPending(world->playerHandle(),1,pending)&&pending);
        assert(world->findPlayer(world->playerHandle())->damageAlarm==(upgraded?240:90));
        assert(world->timeFields().size()==fields+unsigned(upgraded)&&world->randomDrawCount()==draws+unsigned(upgraded));
        if(upgraded){const auto* field=world->timeFields().at(fields);
            assert(field&&field->sourceId==910000&&field->x==240&&field->y==40100);}
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                   GameplayCollisionOwner::Ownership::nativeScenery})for(unsigned scenario=0;scenario<3;++scenario){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        if(scenario==2){assert(world->subtractPlayerHealth(MotionBinary64::fromInteger(4)));world->step(0);}
        assert(world->playerDeathActive()==(scenario==2));
        ContactDamageScripts caller;DamageLiveStateEvents events;events.timeout=false;events.deadAnswer=scenario!=2;
        auto bound=bindContactDamagePlayer(caller,*world,[&](InstanceHandle& h){h=world->playerHandle();return true;});
        const auto one=MotionBinary64::fromInteger(1);
        assert(world->typicalPlayerDamage(bound,MotionBinary64::fromInteger(scenario==1?4:1),one,one,events,
            [](std::uint32_t&){return false;}));
        assert(world->health()==(scenario==0?3:0)&&world->playerDeathActive()==(scenario==2)&&
            events.sounds==unsigned(scenario!=2)&&events.ducks==unsigned(scenario!=2)&&
            !events.shakes&&events.hits==unsigned(scenario==0));
        assert(world->cameraShakeAmount()==(scenario==2?0:5)&&caller.shake.shakeAmt==(scenario==2?0:5)&&
            caller.shake.shakeDur==(scenario==2?0:8));
        assert(world->playerDamaged()&&!world->noDamageAchievement()&&!world->areaNoDamageAchievement());
        if(scenario!=2){
            assert(world->requestTimeStopCameraShake()&&world->cameraShakeAmount()==5);
            for(unsigned tick=1;tick<=8;++tick){world->step(0);
                assert(world->effectsSucceeded()&&world->cameraShakeAmount()==(tick<8?5:0));}
        }
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                   GameplayCollisionOwner::Ownership::nativeScenery})for(unsigned reject:{0u,1u,2u,3u}){
        TimeShardEffects<64> shards;TimeChangeEffects<8> changes;int speed=60;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world)&&shards.bindPhases(owner->timeShards())&&changes.bind(owner->timeChanges()));
        unsigned next=830000,calls=0,pauses=0;
        auto allocate=[&](std::uint32_t& id){
            ++calls;if(calls==reject)return false;
            id=next++;return true;
        };
        const auto shard=[&]{return world->createTimeShard(shards,MotionBinary64::fromInteger(240),
            MotionBinary64::fromInteger(512),false);};
        const bool ok=owner->withTimeEffectSourceIds(allocate,[&]{
            return shard()&&world->createTimeChange(changes,{}, {},speed,[&](unsigned ms){
                assert(ms==30);++pauses;
                // Another synchronous source caller consumes the SAME stream.
                ++next;return true;
            })&&shard();
        });
        assert(ok==!reject&&calls==(reject?reject:3));
        assert(next==(reject==1?830000u:reject==2?830001u:reject==3?830003u:830004u));
        assert(pauses==(reject==1||reject==2?0u:1u));
        if(reject!=1){const auto* r=owner->timeShards().find(owner->timeShards().handle(1));
            assert(r&&r->sourceId==830000);}
        if(!reject||reject==3){const auto* r=owner->timeChanges().find(owner->timeChanges().handle(1));
            assert(r&&r->sourceId==830001);}
        if(!reject){
            const auto* r=owner->timeShards().find(owner->timeShards().handle(2));
            assert(r&&r->sourceId==830003);
            world->step(0);assert(world->effectsSucceeded()&&calls==3);
            // The borrowed allocator must not survive the synchronous scope.
            assert(!shard()&&calls==3);
        }else assert(!owner->valid()&&!world->effectsSucceeded());
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                   GameplayCollisionOwner::Ownership::nativeScenery}){
        TimeShardEffects<64> effects;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        for(unsigned i=0;i<3;++i){world->step(0);assert(world->effectsSucceeded());}
        const auto draws=world->randomDrawCount();
        auto create=[&](std::uint32_t id){return owner->createTimeShardWithSourceId(effects,id,
            RunnerImageValue{0x43704000u}.promoted(),RunnerImageValue{0x43308000u}.promoted(),false);};
        assert(create(105187)&&world->randomDrawCount()==draws+6);
        const auto key=owner->timeShards().handle(1);InstanceHandle shared;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::timeShard,key,shared));
        auto* record=owner->timeShards().schedulerRecord(key);
        assert(record&&record->sourceId==105187&&record->x.bits==0x43704000u&&record->y.bits==0x43308000u);
        assert(GameplayCollisionView(*world).resolveLocal<GameplayCollisionView::Kind::timeShard>(key)==record);
        unsigned population=0;assert(owner->population(population)&&population==1);
        assert(owner->timeShards().activation(key,false)&&owner->population(population)&&population==0);
        const int alarm=record->alarms.get(0);
        world->step(0);assert(world->effectsSucceeded()&&record->alarms.get(0)==alarm);
        assert(owner->timeShards().activation(key,true));
        InstanceHandle player;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::player,world->playerHandle(),player));
        RunnerCollisionEventContext context;TimeStopEffects<4> stop;
        SkeletonSoundAudio audio;SoundPlayEnvironment env;unsigned unexpected=0,stomps=0,damage=0;
        auto contact=[&]{return context.dispatch(shared,player,{true,false},
            [&](auto,auto,int& subtype,bool& found,PackedObjectEvent& event){
                subtype=35;return objects.collisionEvent(238,35,found,event);
            },[&](const auto& event){return owner->dispatchSkeletonEncounterCollision(event,stop,
                [&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);},env,audio,
                [](auto dy,auto dx,auto& angle){return portableAtan(dy,dx,angle);},
                [&](auto& actor){++unexpected;return CheckedSkeletonContact{actor,0,stomps,damage};},
                [&](auto&,int,int,int){++unexpected;return false;},
                [&](const auto&){++unexpected;return false;});});};
        auto beforeContact=world->randomDrawCount();
        assert(contact()&&world->randomDrawCount()==beforeContact); // Create cooldown.
        record->alarms.set(2,1);
        world->step(0);assert(world->effectsSucceeded()&&!effects.get(1)->noTouch);
        const auto life=record->alarms.get(0),fade=record->alarms.get(1);
        beforeContact=world->randomDrawCount();
        assert(contact()&&world->randomDrawCount()==beforeContact+2&&record->alarms.get(2)==5);
        assert(record->alarms.get(0)==life&&record->alarms.get(1)==fade);
        assert(contact()&&world->randomDrawCount()==beforeContact+2);
        assert(!unexpected&&!audio.calls);
        // Exercise the real room animation/alarm traversal and selected cleanup.
        record->alarms.set(0,2);
        world->step(0);assert(world->effectsSucceeded()&&record->alarms.get(0)==1);
        world->step(0);assert(world->effectsSucceeded()&&record->destroyRequested&&effects.size()==0);
        assert(owner->timeShards().size()==1&&owner->population(population)&&population==0);
        assert(owner->prepareDraw()&&owner->timeShards().size()==0&&!owner->timeShards().find(key));
        assert(create(105188));
        const auto replacement=owner->timeShards().handle(2);
        assert(replacement.slot==key.slot&&replacement.generation!=key.generation);
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,43,300)==ResultsDestination::gameplay);
        assert(world->effectsSucceeded()&&owner->valid()&&effects.size()==0&&owner->timeShards().size()==0);
        assert(create(105189)&&effects.clear()&&owner->valid());
    }
    {
        struct Audio {
            unsigned queries=0,plays=0;bool playing=false,fail=false;
            bool isPlaying(int id,bool& value){++queries;value=id==900?playing:false;return !fail;}
            bool stop(int){return true;}
            bool play(int sound,int priority,bool loop,int& voice){
                assert(sound>=349&&sound<=355&&priority==90&&!loop);++plays;voice=900;playing=true;return true;
            }
            bool getGain(int,MotionBinary64& gain){gain=MotionBinary64::fromInteger(1);return true;}
            bool setGain(int,MotionBinary64,int){return true;}
            bool setPitch(int,MotionBinary64){return true;}
        } audio;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        // Admit a retained entry-field lifetime directly: ordinary first-level
        // Create destroys this object, so it cannot exercise a later contact.
        auto& fields=const_cast<GameplayGoalFields&>(world->goalFieldInstances());
        assert(fields.create(240,192,371));
        InstanceHandle player;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::player,world->playerHandle(),player));
        SoundPlayState entrySound;
        for(unsigned i=0;i<2;++i){
            if(i==1)assert(world->registerLevelExit(512));
            const auto key=world->goalFieldInstances().handle(i);
            const auto* record=world->goalFieldInstances().find(key);assert(record);
            InstanceHandle field;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::goalField,key,field));
            PackedObjectEvent binding;bool found=false;
            assert(objects.collisionEvent(record->objectId,35,found,binding)&&found&&binding.ownerObjectId==255);
            RunnerCollisionEventContext::Frame event{field,player,4,35,binding.ownerObjectId,binding.codeId};
            auto oracle=world->randomState();const auto draws=world->randomDrawCount();
            audio.playing=false;const int sound=349+int(oracle.chooseIndexSmall(7));
            assert(owner->dispatchTimeFieldWind(event,MotionBinary64::fromInteger(1),audio));
            assert(record->windSound==sound&&world->overlapSound(8)==sound&&world->timeFieldWindVoice()==900&&world->randomDrawCount()==draws+1&&
                record->sound.sfxIndex==sound&&record->sound.sndsnd==900&&record->sound.chanNum==8);
            if(!i)entrySound=record->sound;
            else {const auto& retained=world->goalFieldInstances().find(world->goalFieldInstances().handle(0))->sound;
                assert(retained.sfxIndex==entrySound.sfxIndex&&retained.sndsnd==entrySound.sndsnd&&
                    retained.sndgain.high==entrySound.sndgain.high&&retained.sndgain.low==entrySound.sndgain.low);}
            const auto queries=audio.queries,plays=audio.plays;
            assert(owner->dispatchTimeFieldWind(event,MotionBinary64::fromInteger(1),audio)&&
                world->randomDrawCount()==draws+1&&audio.queries==queries+1&&audio.plays==plays);
            if(i==1){
                audio.fail=true;
                assert(!owner->dispatchTimeFieldWind(event,MotionBinary64::fromInteger(1),audio)&&
                    !world->effectsSucceeded()&&!owner->valid()&&world->randomDrawCount()==draws+1);
                const auto stoppedQueries=audio.queries;
                assert(!owner->dispatchTimeFieldWind(event,MotionBinary64::fromInteger(1),audio)&&audio.queries==stoppedQueries);
            }
        }
    }
    {
        TimeChangeEffects<8> effects;int speed=60;unsigned pauses=0;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        const auto draws=world->randomDrawCount();
        auto create=[&](std::uint32_t id){return owner->createTimeChangeWithSourceId(effects,id,
            RunnerImageValue{0x43704000u}.promoted(),RunnerImageValue{0x43308000u}.promoted(),speed,
            [&](unsigned ms){
                ++pauses;const auto key=owner->timeChanges().handle(pauses);
                const auto* record=owner->timeChanges().find(key);InstanceHandle shared;
                if(!record||!GameplayCollisionView::encode(GameplayCollisionView::Kind::timeChange,key,shared))return false;
                const auto published=owner->state(shared);
                return ms==30&&speed==30&&record->sourceId==id&&record->alarms.get(0)==2&&
                    record->alarms.get(1)==2&&published&&published->x.bits==0x43704000u&&published->y.bits==0x43308000u;
            });};
        // Warm an empty combat capture/mask before admitting new alarm
        // columns. Later inactive/active transitions and Retry must rebuild
        // that plan, while unchanged captures retain immediate countdowns.
        for(unsigned tick=0;tick<3;++tick){world->step(0);assert(world->effectsSucceeded()&&owner->valid());}
        assert(create(105187));
        const auto old=owner->timeChanges().handle(1);InstanceHandle shared;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::timeChange,old,shared));
        assert(owner->timeChanges().activation(old,false)&&!owner->timeChanges().find(old)->active&&
            (owner->state(shared)->flags&2u));
        for(unsigned tick=0;tick<7;++tick){
            world->step(0);
            const auto* record=owner->timeChanges().find(old);
            assert(world->effectsSucceeded()&&owner->valid()&&record&&record->alarms.get(0)==2&&
                record->alarms.get(1)==2&&speed==30);
        }
        assert(owner->timeChanges().activation(old,true)&&owner->timeChanges().find(old)->active&&
            !(owner->state(shared)->flags&2u));
        for(unsigned tick=1;tick<=14;++tick){
            world->step(0);
            assert(world->effectsSucceeded()&&owner->valid());
            assert(speed==int(tick<12?30+5*(tick/2):60));
        }
        assert(effects.size()==0&&owner->timeChanges().size()==1&&owner->state(shared)&&
            (owner->state(shared)->flags&1u)&&world->randomDrawCount()==draws);
        assert(owner->prepareDraw()&&!owner->state(shared)&&!owner->timeChanges().find(old));
        assert(create(105188));const auto beforeRetry=owner->timeChanges().handle(2);
        assert(beforeRetry.slot==old.slot&&beforeRetry.generation!=old.generation);
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,43,300)==ResultsDestination::gameplay);
        assert(world->effectsSucceeded()&&owner->valid()&&effects.size()==0&&owner->timeChanges().size()==0);
        assert(!owner->timeChanges().find(beforeRetry)&&create(105189));
        for(unsigned tick=0;tick<14;++tick)world->step(0);
        assert(world->effectsSucceeded()&&owner->prepareDraw()&&effects.size()==0&&owner->timeChanges().size()==0);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        InstanceHandle shared[3],local[3];
        const int kinds[]={60,93,99};
        for(unsigned i=0;i<3;++i){
            assert(owner->terrain().append(kinds[i],32+int(i)*16,240,true));
            local[i]=owner->terrain().handle(i);
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local[i],shared[i]));
            const auto state=owner->state(shared[i]);
            assert(state&&state->previous.x.bits==state->x.bits);
            // Seed only the snapshot with a sentinel to make the animation
            // boundary's write observable without moving indexed geometry.
            const_cast<GameplayCollisionOwner::Owner::State*>(state)->previous={};
        }
        assert(owner->terrain().activation(local[1],false)&&owner->reconcileActivation());
        world->step(0);assert(world->effectsSucceeded()&&owner->valid());
        for(unsigned i:{0u,2u}){
            const auto state=owner->state(shared[i]);
            assert(state&&state->previous.x.bits==state->x.bits&&state->previous.y.bits==state->y.bits);
        }
        assert(owner->state(shared[1])->previous.x.bits==0&&owner->state(shared[1])->previous.y.bits==0);
        assert(owner->terrain().activation(local[1],true)&&owner->reconcileActivation());
        world->step(0);assert(world->effectsSucceeded()&&owner->valid());
        const auto reactivated=owner->state(shared[1]);
        assert(reactivated->previous.x.bits==reactivated->x.bits&&reactivated->previous.y.bits==reactivated->y.bits);
    }
    checkEnemySlotDirectory(objects,sprites);
    checkNativeAnimationBatches(objects,sprites);
    const auto mb=read("generated/downwell-sprite-masks.dwsm");SpriteMaskPack masks(mb.data(),mb.size());
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery})for(unsigned contact=0;contact<6;++contact){
        const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),owner->breakableBinding(),
            owner->playerBinding(),native?nullptr:&owner->terrain());
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain)));
        int x=240,y=172;GameMakerRandom random(2);InstanceHandle smoke;
        if(!contact){
            x=240;y=40008;
            if(native){assert(terrain->append(60,232,40000)&&terrain->createProperties(terrain->handle(0),51));}
            else assert(owner->terrain().append(60,232,40000,true)&&
                owner->terrain().createProperties(owner->terrain().handle(0),51,false,random));
            assert(owner->registeredSmoke().create(506000,MotionBinary64::fromInteger(x),MotionBinary64::fromInteger(y),random,smoke)&&
                owner->registeredSmoke().alarmEvent(smoke,0));
        }
        InstanceHandle local,second;assert(world->createBoneWithSourceId(506001,x,y,local));
        if(contact==5)assert(world->createBoneWithSourceId(506002,x,y,second));
        if(contact==3)assert(owner->bones().dispatch(local,[](auto& state,auto&,bool&){state.active=false;return true;}));
        auto pairs=std::make_unique<RunnerCollisionPairs<GameplayCollisionView::capacity,64>>();
        assert(owner->collectCollisionPairs(*pairs)&&pairs->size());
        struct NonSolid {
            bool rollback(InstanceHandle){std::abort();}
            bool prepareMotion(InstanceHandle){std::abort();}
            bool advanceMotion(InstanceHandle){std::abort();}
        } motion;
        RunnerCollisionEventContext context;TimeStopEffects<4> stop;stop.player.stopped=contact==2;
        SkeletonSoundAudio audio;SoundPlayEnvironment env;unsigned damage=0,stomps=0,unexpected=0,smokeContacts=0;
        const bool okay=owner->executeCollisionPairs(*pairs,masks,context,motion,
            []{return RunnerCollisionEventContext::RoomState{true,false};},[&](const auto& event){
                if(event.owner==225)++smokeContacts;
                return owner->dispatchSkeletonEncounterCollision(event,stop,
                    [&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);},env,audio,
                    [](auto dy,auto dx,auto& angle){return portableAtan(dy,dx,angle);},
                    [&](auto& actor){++unexpected;return CheckedSkeletonContact{actor,0,stomps,damage};},
                    [&](auto& caller,int amount,int horizontal,int vertical){assert(amount==1&&horizontal==3&&vertical==2);++damage;
                        caller.damageScripts.typical.damage=MotionBinary64::fromInteger(amount);
                        if(contact==5)stop.player.stopped=true;return contact!=4;},
                    [&](const auto&){++unexpected;return false;});
            });
        assert(okay==(contact!=4)&&damage==unsigned(contact==1||contact==4||contact==5)&&!unexpected&&!audio.calls);
        const auto* r=owner->bones().find(local);
        assert(r&&r->destroyRequested==(contact<3||contact==5)&&bool(r->myFxLocal)==(contact<3||contact==5));
        if(contact==5)assert(owner->bones().find(second)->destroyRequested&&owner->bones().find(second)->myFxLocal);
        if(!contact)assert(smokeContacts==1&&owner->registeredSmoke().find(smoke));
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));world->configureEnemyRandom(GameMakerRandom(91));
        InstanceHandle local,shared;
        assert(world->spawnDebrisWithSourceId(105999,MotionBinary64::fromInteger(240),
            MotionBinary64::fromInteger(40000),2,local));
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,local,shared));
        GameplayCollisionView view(*world);GameplayCollisionView::Record record;
        assert(view.resolve(shared,record)&&record.debris==owner->debris().find(local));
        assert(record.debris->sourceId==105999&&local.generation!=105999);
        GameMakerRandom random(91);DebrisBirth expected;assert(expected.create(2,random));
        const auto& effect=*record.debris->effect;
        assert(effect.spriteId==116&&effect.imageIndex.bits==expected.imageIndex.bits);
        assert(effect.imageSpeed.bits==expected.imageSpeed.bits&&world->randomDrawCount()==4);
        assert(owner->prepareDraw());
        for(unsigned i=0;i<30;++i){
            world->step(0);assert(world->effectsSucceeded()&&owner->prepareDraw());
        }
        assert(!owner->debris().find(local)&&owner->valid());
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery}){
        const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),
            owner->playerBinding(),native?nullptr:&owner->terrain());
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain))&&owner->configureCorpseMasks(masks));
        GameMakerRandom wallRandom(1);
        if(native){
            assert(terrain->append(60,240,192)&&terrain->createProperties(terrain->handle(0),51));
        }else{
            assert(owner->terrain().append(60,240,192,true)&&
                owner->terrain().createProperties(owner->terrain().handle(0),51,false,wallRandom));
        }
        auto& corpses=owner->registeredCorpses();GameMakerRandom random(2);
        DeadBodySource source{MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(192),{},MotionBinary64::fromInteger(-1)};
        assert(corpses.createFromDeath(205187,source,206,MotionBinary64::fromInteger(2),102,false,random,
            [](auto,int layer,auto,auto,bool& hit){assert(layer==100);hit=false;return true;}));
        const auto key=corpses.sourceHandle(205187);InstanceHandle shared;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::corpse,key,shared));
        GameplayCollisionView view(*world);checkTypedCollisionView(view,GameplayCollisionView::Kind::corpse,key);
        assert(view.contains(shared)&&corpses.find(key)->state.alarms.get(0)==180);
        assert(random.drawCount()>3); // The real wall-57 shape query caused relocation.
        std::uint32_t nextId=205188;
        owner->configureCorpseChildIds([](void* context,std::uint32_t& out){
            out=(*static_cast<std::uint32_t*>(context))++;return true;
        },&nextId);
        for(unsigned tick=0;tick<8;++tick){world->step(0);assert(world->effectsSucceeded()&&owner->valid());}
        assert(corpses.find(key)&&corpses.find(key)->state.active&&corpses.find(key)->state.alarms.get(0)==172);
        struct CorpseEnvironment {
            int area(){return 1;}int restInPieces(){return 0;}
            bool create(int,MotionBinary64,MotionBinary64){assert(false);return false;}
        } environment;
        auto phase=std::make_unique<RunnerStepSnapshot<GameplayCollisionView::capacity>>();
        const auto before=corpses.find(key)->state;
        unsigned selected=0;
        assert(owner->dispatchStepPhase(0,*phase,[&](InstanceHandle actor,const PackedObjectEvent& event){
            GameplayCollisionView::Kind kind;InstanceHandle local;assert(GameplayCollisionView::decode(actor,kind,local));
            if(kind!=GameplayCollisionView::Kind::corpse)return true;
            PackedObjectEvent original;assert(objects.findEvent(263,3,0,original)&&event.codeId==original.codeId);
            ++selected;return owner->stepCorpse(actor,true,480,environment);
        })&&selected==1);
        const auto& frozen=corpses.find(key)->state;
        assert(frozen.x.bits==before.x.bits&&frozen.y.bits==before.y.bits&&
            frozen.xx.high==before.xx.high&&frozen.xx.low==before.xx.low&&
            frozen.alarms.get(0)==173&&frozen.alarms.get(1)==before.alarms.get(1)+1);
        // Resume uses the same registered solid queries and publishes Y then X.
        assert(owner->stepCorpse(shared,false,480,environment));
        const auto* moving=corpses.find(key);
        assert(moving&&moving->x.bits==moving->state.x.bits&&moving->y.bits==moving->state.y.bits&&
            (moving->state.xx.high!=before.xx.high||moving->state.xx.low!=before.xx.low));
        std::int32_t fieldX,fieldY;
        assert(moving->x.promoted().roundPixel(fieldX)&&moving->y.promoted().roundPixel(fieldY)&&
            world->createTimeFieldWithSourceId(205190,fieldX,fieldY));
        const auto inside=moving->state;
        assert(owner->stepCorpse(shared,true,480,environment));
        const auto& allowed=corpses.find(key)->state;
        assert(allowed.alarms.get(0)==inside.alarms.get(0)&&
            (allowed.xx.high!=inside.xx.high||allowed.xx.low!=inside.xx.low));
        assert(owner->advanceCorpseAlarm(key,0)); // Account for the frozen Step's compensation.
        for(unsigned tick=8;tick<180;++tick)for(unsigned alarm=0;alarm<4;++alarm)assert(owner->advanceCorpseAlarm(key,alarm));
        const auto* corpse=corpses.find(key);
        assert(corpse&&corpse->destroyRequested&&corpse->state.myExplosion==205188&&nextId==205189);
        const auto expiryKey=owner->registeredExpiry().sourceHandle(205188);
        assert(owner->registeredExpiry().find(expiryKey)&&owner->registeredExpiry().find(expiryKey)->state.spriteId==102);
        assert(owner->prepareDraw()&&!corpses.find(key)&&!view.contains(shared));
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::gameplay&&
            world->registeredCorpses()==&corpses&&!corpses.size()&&owner->valid());
        assert(corpses.createFromDeath(205191,source,206,{},102,false,random,
            [](auto,int,auto,auto,bool& hit){hit=false;return true;}));
        const auto eating=corpses.sourceHandle(205191);InstanceHandle eatingShared,player;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::corpse,eating,eatingShared)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::player,world->playerHandle(),player));
        struct ParticleGlobals {
            MotionBinary64 grav{};
            bool gravity(MotionBinary64 v){grav=v;return true;}
            MotionBinary64 gravity(){return grav;}MotionBinary64 fallSpeed(){return {};}
        } particleGlobals;
        struct Consumption {
            GameplayCollisionOwner& owner;GameMakerRandom& random;ParticleGlobals& globals;
            unsigned eaten=0,sounds=0,children=0,reads=0,reject=0;
            bool incrementBodiesEaten(){++eaten;return true;}
            void eatSound(int s,int p,int l,int g){assert(eaten==1&&s==83&&p==80&&!l&&g==1);++sounds;}
            MotionBinary64 playerY(){assert(reads++==children*2);return MotionBinary64::fromInteger(192+children);}
            MotionBinary64 playerX(){assert(reads++==children*2+1);return MotionBinary64::fromInteger(240+children);}
            bool create(int object,MotionBinary64 x,MotionBinary64 y,std::uint32_t& id){
                assert(object==223&&eaten==1&&sounds==1&&x.high==MotionBinary64::fromInteger(240+children).high&&
                    y.high==MotionBinary64::fromInteger(192+children).high);
                id=205192+children++;InstanceHandle child;
                return children!=reject&&owner.consumptionParticles().create(id,x,y,random,globals,child);
            }
        } effects{*owner,random,particleGlobals};
        RunnerCollisionEventContext context;
        auto contact=[&](bool upgrade,bool dead){return owner->dispatchCollisionEvent(context,eatingShared,player,{true,false},
            [&](const auto& event){return owner->dispatchCorpsePlayerCollision(event,upgrade,dead,effects);});};
        assert(contact(true,false)&&!effects.eaten); // Source active flag is still false.
        for(unsigned tick=0;tick<8;++tick)assert(owner->advanceCorpseAlarm(eating,3));
        assert(contact(false,false)&&contact(true,true)&&!effects.eaten);
        assert(contact(true,false)&&effects.eaten==1&&effects.sounds==1&&effects.children==5&&effects.reads==10&&
            corpses.find(eating)&&corpses.find(eating)->destroyRequested);
        assert(owner->prepareDraw()&&!corpses.find(eating)&&!view.contains(eatingShared));
        assert(owner->consumptionParticles().size()==5);
        for(unsigned child=0;child<5;++child){
            const auto local=owner->consumptionParticles().sourceHandle(205192+child);
            checkTypedCollisionView(view,GameplayCollisionView::Kind::consumptionBlood,local);
        }
        for(unsigned tick=0;tick<120;++tick){
            world->step(0);assert(world->effectsSucceeded()&&owner->valid());unsigned selectedParticles=0;
            assert(owner->dispatchStepPhase(0,*phase,[&](InstanceHandle actor,const PackedObjectEvent& event){
                GameplayCollisionView::Kind kind;InstanceHandle local;assert(GameplayCollisionView::decode(actor,kind,local));
                if(kind!=GameplayCollisionView::Kind::consumptionBlood)return true;
                assert(event.codeId==2006);++selectedParticles;return owner->stepConsumptionBlood(actor,particleGlobals);
            })&&selectedParticles==(tick==119?0u:5u));
            assert(owner->prepareDraw()&&owner->consumptionParticles().size()==(tick==119?0u:5u));
        }
        assert(corpses.createFromDeath(205200,source,206,{},102,false,random,
            [](auto,int,auto,auto,bool& hit){hit=false;return true;}));
        const auto rejected=corpses.sourceHandle(205200);
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::corpse,rejected,eatingShared));
        for(unsigned tick=0;tick<8;++tick)assert(owner->advanceCorpseAlarm(rejected,3));
        effects.eaten=effects.sounds=effects.children=effects.reads=0;effects.reject=3;
        assert(!contact(true,false)&&!owner->valid()&&effects.eaten==1&&effects.sounds==1&&effects.children==3&&
            corpses.find(rejected)&&!corpses.find(rejected)->destroyRequested);
        assert(!contact(true,false)&&effects.children==3); // No replay of the committed prefix.
    }
    for(unsigned layout=0;layout<3;++layout){
        std::vector<std::uint32_t> expected;
        for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                      GameplayCollisionOwner::Ownership::nativeScenery}){
            const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
            const auto bytes=read("generated/downwell-rooms.dwrm");RoomPack roomPack(bytes.data(),bytes.size());
            TimeShardEffects<64> effects;
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&roomPack,mode);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),
                owner->playerBinding(),native?nullptr:&owner->terrain(),&roomPack);
            auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();GameMakerRandom createRandom;
            assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain)));
            if(layout)for(unsigned i=0;i<9;++i){
                const int x=224+int(i%3)*16,y=176+int(i/3)*16;
                if(i==4){assert(world->createGeneratedInstance({92,x,y},objects,[](const auto&){return true;}));continue;}
                const int object=i==8?93:60;
                if(native){assert(terrain->append(object,x,y));assert(terrain->createProperties(terrain->handle(terrain->size()-1),51));}
                else{assert(owner->terrain().append(object,x,y,true));
                    assert(owner->terrain().createProperties(owner->terrain().handle(owner->terrain().size()-1),51,false,createRandom));}
            }
            assert(owner->createTimeShardWithSourceId(effects,205187,MotionBinary64::fromInteger(240),
                MotionBinary64::fromInteger(layout==2?192:160),false));
            const auto key=owner->timeShards().handle(1);InstanceHandle shared;
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::timeShard,key,shared));
            auto* particle=effects.get(1);assert(particle);
            particle->xsp=MotionBinary64::fromInteger(2);particle->ysp={};
            std::vector<std::uint32_t> actual;
            for(unsigned tick=0;tick<8&&effects.get(1);++tick){
                world->step(0);assert(world->effectsSucceeded());
                unsigned outsideCalls=0;
                const auto outside=[&](const auto& effect,bool& retire){
                    ++outsideCalls;const auto* published=owner->timeShards().find(key);
                    assert(published&&published->y.bits==effect.y.bits);retire=false;return true;
                };
                assert(layout==0?owner->stepTimeShard(shared,tick<4,masks):
                    (owner->stepTimeShard(shared,tick<4,masks,outside)&&outsideCalls==1));
                const auto* r=owner->timeShards().find(key);assert(r);
                if(layout==0&&tick==0)assert(r->x.bits!=0x43700000u&&r->imageSpeed.bits==0);
                if(layout==2&&tick==0)assert(r->destroyRequested);
                actual.insert(actual.end(),{r->x.bits,r->y.bits,r->imageIndex.bits,r->imageSpeed.bits,
                    std::uint32_t(r->alarms.get(0)),std::uint32_t(r->alarms.get(1)),
                    std::uint32_t(world->randomDrawCount()),std::uint32_t(r->destroyRequested)});
                if(r->destroyRequested)assert(owner->prepareDraw()&&!owner->timeShards().find(key));
            }
            assert(!actual.empty());
            if(!native)expected=actual;else assert(actual==expected);
            assert(effects.clear()&&owner->valid());
        }
    }
    {
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));
        assert(scene->createGeneratedInstance({82,240,512},objects,[](const auto&){return true;}));
        DebrisEffects<1> effects;InstanceHandle local;
        assert(scene->spawnDebris(effects,{},{},1,local));auto& particle=*effects.get(local.generation);
        SpriteMaskPack::Mask shardMask,fieldMask;
        assert(masks.mask(598,0,shardMask)&&masks.mask(597,0,fieldMask));
        auto move=[&](MotionBinary64 x,MotionBinary64 y){RunnerImageValue px,py;
            assert(px.assign(x)&&py.assign(y)&&collision->debris().move(local.generation,particle,px,py));
            particle.x=px;particle.y=py;
        };
        unsigned edgeHits=0;
        for(int y=480;y<=544;++y)for(int x=208;x<=272;++x){
            move(MotionBinary64::fromInteger(x),MotionBinary64::fromInteger(y));
            bool overlap=false;
            for(int py=y-5;py<=y+4&&!overlap;++py)for(int px=x-5;px<=x+4;++px)
                if(shardMask.contains(px-x+8,py-y+8)&&fieldMask.contains(px-240+24,py-512+24)){overlap=true;break;}
            bool frozen=overlap;
            assert(collision->debrisFrozen(local,true,480,masks,frozen)&&frozen==!overlap);
            if(overlap&&!fieldMask.contains(x-240+24,y-512+24))++edgeHits;
        }
        assert(edgeHits); // A centre-only circle/point query would miss these.
        const SpriteMaskPack unavailable(nullptr,0);
        for(int qx:{639,640,960,1280,1281}){
            auto x=MotionBinary64::fromInteger(qx);x.high-=2u<<20;
            move(x,MotionBinary64::fromInteger(20000));bool frozen=true;
            assert(collision->debrisFrozen(local,false,480,unavailable,frozen)&&!frozen);
            assert(collision->debrisFrozen(local,true,480,masks,frozen)&&frozen==(qx>=640&&qx<=1280));
        }
        move(MotionBinary64::fromInteger(159),MotionBinary64::fromInteger(20000));bool unchanged=true;
        assert(!collision->debrisFrozen(local,true,480,unavailable,unchanged)&&unchanged);
        // Even side-exempt particles query the field first; missing mask data
        // is a terminal query failure, not a successful outside-area result.
        assert(!collision->valid());
    }
    for(unsigned seed=0;seed<16;++seed){
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));scene->configureEnemyRandom(GameMakerRandom(seed));
        for(int x:{224,240,256})assert(scene->createGeneratedInstance({60,x,512},objects,[](const auto&){return true;}));
        DebrisEffects<3> effects;InstanceHandle local[3],shared[3];DebrisAnimation expected[3];
        for(unsigned i=0;i<3;++i){
            assert(scene->spawnDebris(effects,MotionBinary64::fromInteger(232+int(i)*8),
                MotionBinary64::fromInteger(500+int(i)*4),1,local[i]));
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,local[i],shared[i]));
            expected[i]=*effects.get(local[i].generation);
        }
        auto random=scene->randomState();PackedSpriteCollision shape;
        assert(sprites.sprite(598,shape)&&!shape.collisionMode);
        auto real=[](MotionBinary64 v){const std::uint64_t bits=(std::uint64_t(v.high)<<32)|v.low;
            double d;std::memcpy(&d,&bits,sizeof d);return float(d);};
        auto contact=[&](MotionBinary64 px,MotionBinary64 py,bool& hit){
            const float x=real(px),y=real(py);
            const int left=int(std::nearbyint(x+float(shape.marginLeft-shape.originX)));
            const int right=int(std::nearbyint(x+float(shape.marginRight-shape.originX+1)))-1;
            const int top=int(std::nearbyint(y+float(shape.marginTop-shape.originY)));
            const int bottom=int(std::nearbyint(y+float(shape.marginBottom-shape.originY+1)))-1;
            hit=left<=263&&right>=216&&top<=519&&bottom>=504;return true;
        };
        for(unsigned tick=0;tick<40;++tick){
            const auto phase=effects.beginPhase();
            for(unsigned alarm=0;alarm<3;++alarm)for(unsigned i=0;i<3;++i){
                if(!effects.retained(local[i].generation))continue;
                expected[i].advanceAlarm(alarm);
                assert(scene->advanceDebrisRoomAlarm(effects,local[i],phase,alarm));
            }
            RunnerStepSnapshot<8> snapshot;unsigned next=0,stops=0;
            assert(collision->dispatchStepPhase(0,snapshot,[&](InstanceHandle h,const PackedObjectEvent&){
                GameplayCollisionView::Kind kind;InstanceHandle selected;
                assert(GameplayCollisionView::decode(h,kind,selected));
                if(kind!=GameplayCollisionView::Kind::debris)return true;
                while(next<3&&expected[next].destroyRequested)++next;
                assert(next<3&&selected.generation==local[next].generation);
                const auto i=next++;const bool frozen=i==1&&(tick%3)!=0;
                assert(expected[i].step(frozen,random,contact));
                assert(collision->stepDebris(effects,h,phase,masks,
                    [&](InstanceHandle key,const DebrisAnimation&,bool& stopped){
                        assert(key.generation==local[i].generation);++stops;stopped=frozen;return true;
                    }));
                return true;
            }));
            assert(stops<=3);
            for(unsigned i=0;i<3;++i){
                const auto* actual=effects.retained(local[i].generation);if(!actual)continue;
                const auto& e=expected[i];
                assert(actual->x.bits==e.x.bits&&actual->y.bits==e.y.bits&&
                    actual->xsp.high==e.xsp.high&&actual->xsp.low==e.xsp.low&&
                    actual->ysp.high==e.ysp.high&&actual->ysp.low==e.ysp.low&&
                    actual->imageSpeed.bits==e.imageSpeed.bits&&actual->flash==e.flash&&
                    actual->noTouch==e.noTouch&&actual->disappearing==e.disappearing&&actual->destroyRequested==e.destroyRequested);
                for(unsigned a=0;a<3;++a)assert(actual->alarms.get(a)==e.alarms.get(a));
                assert(collision->state(shared[i])->x.bits==e.x.bits&&collision->state(shared[i])->y.bits==e.y.bits);
            }
            assert(scene->randomState().drawCount()==random.drawCount()&&scene->randomState().stateIndex()==random.stateIndex());
            for(unsigned w=0;w<16;++w)assert(scene->randomState().stateWord(w)==random.stateWord(w));
            assert(collision->prepareDraw());
            for(unsigned i=0;i<3;++i)if(expected[i].destroyRequested)
                assert(!effects.retained(local[i].generation)&&!collision->state(shared[i]));
        }
        assert(effects.clear(collision->debris()));
    }
    {
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));DebrisEffects<1> effects;
        const auto oldPhase=effects.beginPhase();InstanceHandle local,shared;
        assert(scene->spawnDebris(effects,{},{},1,local)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,local,shared));
        unsigned stops=0;const auto draws=scene->randomDrawCount();
        auto reject=[&](auto,const auto&,bool&){++stops;return false;};
        assert(scene->advanceDebrisRoomAlarm(effects,local,oldPhase,0)&&effects.get(local.generation)->alarms.get(0)==30);
        assert(collision->stepDebris(effects,shared,oldPhase,masks,reject)&&!stops);
        assert(!collision->stepDebris(effects,shared,effects.beginPhase(),masks,reject)&&stops==1&&
            !scene->effectsSucceeded()&&!effects.valid()&&!collision->valid());
        assert(!collision->stepDebris(effects,shared,effects.beginPhase(),masks,reject)&&stops==1&&scene->randomDrawCount()==draws);
    }
    // Family 56 is broader than either wall selector: thin platforms and
    // shootable boxes must participate. These resource masks are rectangular;
    // independently round their quarter-pixel edges, not the particle anchor.
    for(int area:{1,2,4})for(int object:{60,93,92}){
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));
        assert(scene->createGeneratedInstance({object,240,512},objects,[](const auto&){return true;}));
        DebrisEffects<1> effects;InstanceHandle local,shared;
        assert(scene->spawnDebris(effects,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(504),area,local));
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,local,shared));
        PackedSpriteCollision particle,target;PackedObject definition;
        const auto p=collision->properties(shared);
        assert(p&&sprites.collisionSprite(p->sprite.spriteId,p->sprite.maskId,particle)&&!particle.collisionMode);
        assert(objects.findObject(object,definition)&&
            sprites.collisionSprite(object==92?definition.spriteId:terrainTileForArea(1),definition.textureMaskId,target)&&!target.collisionMode);
        auto roundQuarter=[](int n){const int whole=n/4,tail=n%4;
            return whole+int(tail>2||(tail==2&&(whole&1)));};
        auto coordinate=[](int n){auto value=MotionBinary64::fromInteger(n);value.high-=2u<<20;return value;};
        const auto before=*collision->state(shared);
        unsigned hits=0,misses=0;
        for(int y=492;y<=532;++y)for(int x=220;x<=260;++x)for(int fraction=0;fraction<4;++fraction){
            const int qx=x*4+fraction,qy=y*4+fraction;
            const int left=roundQuarter(qx+4*(particle.marginLeft-particle.originX));
            const int right=roundQuarter(qx+4*(particle.marginRight-particle.originX+1))-1;
            const int top=roundQuarter(qy+4*(particle.marginTop-particle.originY));
            const int bottom=roundQuarter(qy+4*(particle.marginBottom-particle.originY+1))-1;
            const bool expected=left<=240+target.marginRight-target.originX&&
                right>=240+target.marginLeft-target.originX&&top<=512+target.marginBottom-target.originY&&
                bottom>=512+target.marginTop-target.originY;
            bool hit=!expected;
            assert(collision->debrisPlaceMeeting(local,coordinate(qx),coordinate(qy),masks,hit)&&hit==expected);
            hit?++hits:++misses;
            assert(collision->state(shared)->x.bits==before.x.bits&&collision->state(shared)->y.bits==before.y.bits);
        }
        assert(hits&&misses);
        if(object!=92){
            const auto targetKey=collision->terrain().handle(0);bool hit=true;
            const auto hitX=MotionBinary64::fromInteger(240+target.marginLeft-target.originX-particle.marginLeft+particle.originX);
            const auto hitY=MotionBinary64::fromInteger(512+target.marginTop-target.originY-particle.marginTop+particle.originY);
            assert(collision->terrain().activation(targetKey,false)&&collision->reconcileActivation());
            assert(collision->debrisPlaceMeeting(local,hitX,hitY,masks,hit)&&!hit);
            assert(collision->terrain().activation(targetKey,true)&&collision->reconcileActivation());
            assert(collision->debrisPlaceMeeting(local,hitX,hitY,masks,hit)&&hit);
            assert(collision->terrain().requestDestroy(targetKey));
            assert(collision->debrisPlaceMeeting(local,hitX,hitY,masks,hit)&&!hit);
        }
        assert(effects.clear(collision->debris()));
        bool unchanged=true;
        assert(!collision->debrisPlaceMeeting(local,{},{},masks,unchanged)&&unchanged&&!collision->valid());
    }
    {
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));
        InstanceHandle first,second;
        assert(scene->spawnDebris(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(300),1,first));
        assert(scene->debrisEffects().size()==1&&collision->debris().find(first));
        assert(scene->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        assert(!scene->debrisEffects().size()&&!collision->debris().find(first));
        assert(scene->spawnDebris(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(300),1,second));
        assert(second.generation>first.generation&&scene->debrisEffects().retained(second.generation));
        auto expected=*scene->debrisEffects().retained(second.generation);auto random=scene->randomState();
        for(unsigned tick=0;tick<35;++tick){
            const auto phase=scene->debrisEffects().beginPhase();
            for(unsigned a=0;a<3;++a){
                expected.advanceAlarm(a);
                assert(scene->advanceOwnedDebrisRoomAlarm(second,phase,a));
            }
            if(expected.destroyRequested){
                assert(collision->prepareDraw()&&!scene->debrisEffects().retained(second.generation));
                break;
            }
            const bool frozen=tick<5;
            assert(expected.step(frozen,random,[](auto,auto,bool& hit){hit=false;return true;}));
            assert(scene->stepOwnedDebrisRoomEffect(second,phase,
                [&](InstanceHandle,const auto&,bool& stop){stop=frozen;return true;},
                [](InstanceHandle,const auto&,auto,auto,bool& hit){hit=false;return true;}));
            const auto* actual=scene->debrisEffects().retained(second.generation);assert(actual);
            assert(actual->x.bits==expected.x.bits&&actual->y.bits==expected.y.bits&&
                actual->imageSpeed.bits==expected.imageSpeed.bits&&actual->flash==expected.flash);
            for(unsigned a=0;a<3;++a)assert(actual->alarms.get(a)==expected.alarms.get(a));
        }
        assert(expected.destroyRequested&&!scene->debrisEffects().size());
        assert(scene->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
    }
    {
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));
        // Deliberately create higher object IDs first; registration order is
        // retained within object 370, not used to order different objects.
        DebrisEffects<3> effects;InstanceHandle first,second,newborn;
        assert(scene->spawnDebris(effects,{},{},1,first));
        assert(scene->createGeneratedInstance({112,240,512},objects,[](const auto&){return true;}));
        assert(scene->spawnDebris(effects,{},{},1,second));
        assert(scene->createGeneratedInstance({92,240,512},objects,[](const auto&){return true;}));
        effects.get(first.generation)->alarms.set(1,1);
        effects.get(second.generation)->alarms.set(1,1);
        const auto cutoff=effects.beginPhase();RunnerStepSnapshot<16> snapshot;
        std::vector<unsigned> order,debrisOrder;unsigned bornVisits=0;
        assert(collision->dispatchAlarmPhase(snapshot,[&](InstanceHandle h,unsigned alarm,const PackedObjectEvent& event){
            GameplayCollisionView::Kind kind;InstanceHandle local;
            assert(GameplayCollisionView::decode(h,kind,local));
            if(kind==GameplayCollisionView::Kind::breakable||kind==GameplayCollisionView::Kind::enemy||
                kind==GameplayCollisionView::Kind::debris){
                const int object=kind==GameplayCollisionView::Kind::breakable?92:
                    kind==GameplayCollisionView::Kind::enemy?112:370;
                PackedObjectEvent expected;
                assert(objects.findEvent(object,2,int(alarm),expected));
                assert(event.ownerObjectId==expected.ownerObjectId&&event.codeId==expected.codeId&&
                    event.eventType==expected.eventType&&event.eventSubtype==expected.eventSubtype);
            }
            if(kind==GameplayCollisionView::Kind::breakable){
                order.push_back(alarm*1000+92);
                assert(scene->spawnDebris(effects,{},{},1,newborn));
            }else if(kind==GameplayCollisionView::Kind::enemy){
                order.push_back(alarm*1000+112);
                // This earlier object's Alarm 1 changes a later object's
                // countdown before it is touched in the same alarm index.
                effects.get(second.generation)->alarms.set(1,2);
            }else if(kind==GameplayCollisionView::Kind::debris){
                order.push_back(alarm*1000+370);
                bornVisits+=local.generation==newborn.generation;
                debrisOrder.push_back(local.generation);
                return scene->advanceDebrisRoomAlarm(effects,local,cutoff,alarm);
            }
            return true;
        }));
        assert((order==std::vector<unsigned>{92,370,370,1112,1370,1370,2370,2370}));
        assert((debrisOrder==std::vector<unsigned>{first.generation,second.generation,
            first.generation,second.generation,first.generation,second.generation}));
        assert(!bornVisits&&effects.get(first.generation)->alarms.get(1)==0&&
            effects.get(second.generation)->alarms.get(1)==1&&effects.get(newborn.generation)->alarms.get(0)==30);
        // A previous callback may destroy a later captured lifetime. Recheck
        // flags before every countdown, including later alarm indices.
        unsigned secondVisits=0;const auto nextCutoff=effects.beginPhase();
        assert(collision->dispatchAlarmPhase(snapshot,[&](InstanceHandle h,unsigned alarm,const PackedObjectEvent&){
            GameplayCollisionView::Kind kind;InstanceHandle local;
            assert(GameplayCollisionView::decode(h,kind,local));
            if(kind!=GameplayCollisionView::Kind::debris)return true;
            if(local.generation==first.generation&&alarm==0){
                effects.get(second.generation)->alarms.set(0,1);
                assert(scene->advanceDebrisRoomAlarm(effects,second,nextCutoff,0));
            }
            secondVisits+=local.generation==second.generation;
            return scene->advanceDebrisRoomAlarm(effects,local,nextCutoff,alarm);
        }));
        assert(!secondVisits&&effects.retained(second.generation)->destroyRequested);
        assert(collision->prepareDraw()&&!effects.retained(second.generation));
        assert(effects.clear(collision->debris()));
        unsigned calls=0;
        assert(!collision->dispatchAlarmPhase(snapshot,[&](InstanceHandle,unsigned,const PackedObjectEvent&){++calls;return false;}));
        assert(calls==1&&!collision->valid());
        assert(!collision->dispatchAlarmPhase(snapshot,[&](InstanceHandle,unsigned,const PackedObjectEvent&){++calls;return true;}));
        assert(calls==1);
    }
    {
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));InstanceHandle key;
        for(unsigned retry=0;retry<2;++retry){
            assert(scene->spawnDebris(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(300),1,key));
            for(unsigned tick=1;tick<=30;++tick){
                scene->step(0);assert(scene->effectsSucceeded());
                const auto* actual=scene->debrisEffects().retained(key.generation);assert(actual);
                assert(actual->alarms.get(0)==int(30-tick));
                assert(actual->destroyRequested==(tick==30));
                assert(collision->prepareDraw());
            }
            assert(!scene->debrisEffects().retained(key.generation)&&!collision->debris().size());
            assert(scene->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        }
    }
    {
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));InstanceHandle key;
        assert(scene->spawnDebris(MotionBinary64::fromInteger(20000),MotionBinary64::fromInteger(20000),1,key));
        auto expected=*scene->debrisEffects().retained(key.generation);auto random=scene->randomState();unsigned stops=0;
        for(unsigned tick=0;tick<40;++tick){
            const bool frozen=tick<5;
            assert(expected.advanceImage());for(unsigned a=0;a<3;++a)expected.advanceAlarm(a);
            assert(expected.step(frozen,random,[](auto,auto,bool& hit){hit=false;return true;}));
            assert(scene->stepWithOwnedDebris(0,[&](auto,const auto&,bool& stop){++stops;stop=frozen;return true;},
                [&](InstanceHandle local,const auto&,auto x,auto y,bool& hit){return collision->debrisPlaceMeeting(local,x,y,masks,hit);}));
            assert(collision->prepareDraw());
            const auto* actual=scene->debrisEffects().retained(key.generation);
            if(expected.destroyRequested){assert(!actual);continue;}
            assert(actual&&actual->x.bits==expected.x.bits&&actual->y.bits==expected.y.bits&&
                actual->imageIndex.bits==expected.imageIndex.bits&&actual->imageSpeed.bits==expected.imageSpeed.bits&&
                actual->noTouch==expected.noTouch&&actual->disappearing==expected.disappearing&&actual->flash==expected.flash);
            for(unsigned a=0;a<3;++a)assert(actual->alarms.get(a)==expected.alarms.get(a));
        }
        assert(stops==34);
        assert(scene->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        assert(scene->spawnDebris(MotionBinary64::fromInteger(20000),MotionBinary64::fromInteger(20000),1,key));
        // A failed source time-stop query is terminal, not interpreted as an
        // unfrozen effect; no collision callbacks may follow it.
        unsigned queries=0;
        assert(!scene->stepWithOwnedDebris(0,[](auto,const auto&,bool&){return false;},
            [&](auto,const auto&,auto,auto,bool&){++queries;return true;}));
        assert(!queries&&!scene->effectsSucceeded());
    }
    {
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));
        assert(scene->createGeneratedInstance({82,240,512},objects,[](const auto&){return false;}));
        InstanceHandle player;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::player,scene->playerHandle(),player));
        const auto before=*collision->state(player);
        unsigned hits=0,misses=0;
        for(int facing:{-1,1}){
            auto binding=collision->playerBinding();
            const auto* record=scene->findPlayer(scene->playerHandle());assert(record);
            assert(binding.integerTransform(scene->playerHandle(),*record,RunnerTransformField::scaleX,facing,{0x3f800000u},0xffffffu));
            for(int qx=640;qx<=1280;qx+=7){
                auto x=MotionBinary64::fromInteger(qx);x.high-=2u<<20; // quarter-pixel inputs
                bool hit=false;
                assert(collision->playerTimeFieldMeeting(x,MotionBinary64::fromInteger(512),masks,hit));
                hit?++hits:++misses;
                const auto state=collision->state(player);assert(state);
                assert(state->x.bits==before.x.bits&&state->y.bits==before.y.bits&&
                    state->previous.x.bits==before.previous.x.bits&&state->previous.y.bits==before.previous.y.bits);
            }
            bool hit=false;
            assert(collision->playerTimeFieldMeeting(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),masks,hit)&&hit);
            assert(collision->playerTimeFieldMeeting(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(10000),masks,hit)&&!hit);
        }
        assert(hits&&misses);
        GameplayCollisionOwner::RegisteredFieldContact selected;
        assert(collision->playerTimeFieldSelection(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),masks,selected));
        assert(selected.found&&selected.x.bits==0x43700000u&&selected.y.bits==0x44000000u);
        const auto first=selected.handle;
        assert(scene->createGeneratedInstance({82,241,512},objects,[](const auto&){return false;}));
        assert(collision->playerTimeFieldSelection(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),masks,selected));
        assert(selected.found&&selected.handle.slot==first.slot&&selected.handle.generation==first.generation);
        const auto fieldBinding=scene->timeFields().lifecycle();
        const auto* firstField=scene->timeFields().at(0);assert(firstField);
        assert(fieldBinding.activation({0,scene->timeFields().identityAt(0)},*firstField,false)&&collision->reconcileActivation());
        assert(collision->playerTimeFieldSelection(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),masks,selected));
        assert(selected.found&&selected.handle.slot!=first.slot&&selected.x.bits==0x43710000u);
        assert(collision->playerTimeFieldSelection(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(10000),masks,selected));
        assert(!selected.found&&!selected.handle.generation&&selected.x.bits==0&&selected.y.bits==0);
        SpriteMaskPack absent(nullptr,0);bool sentinel=true;
        assert(!collision->playerTimeFieldMeeting({},{},absent,sentinel)&&sentinel&&!collision->valid());
    }
    for(bool rejectMark:{false,true}){
        using R=GameplayPlayerControllers::Record;
        struct Reject {
            bool rejectMark=false;unsigned marks=0,retires=0;
            bool birth(InstanceHandle,const R&){return true;}
            bool retire(InstanceHandle,const R& r){assert(r.destroyRequested);++retires;return false;}
            bool activation(InstanceHandle,const R&,bool){return true;}
            bool membership(InstanceHandle,const R&,bool){return true;}
            bool move(InstanceHandle,const R&,RunnerImageValue,RunnerImageValue){return true;}
            bool sprite(InstanceHandle,const R&,int){return true;}
            bool imageIndex(InstanceHandle,const R&,RunnerImageValue){return true;}
            bool markDestroyed(InstanceHandle,const R& r){assert(!r.destroyRequested);++marks;return !rejectMark;}
        } binding;
        binding.rejectMark=rejectMark;
        GameplayPlayerControllers children(GameplayPlayerControllers::Binding::bindDestruction(binding));
        InstanceHandle balloon;assert(children.createCompanion(256,105187,{},{},balloon));
        assert(children.balloonEndStep(balloon,true)&&!binding.marks);
        assert(children.balloonEndStep(balloon,false)==!rejectMark&&binding.marks==1);
        assert(children.find(balloon)->destroyRequested==!rejectMark);
        if(!rejectMark)assert(!children.retire(balloon)&&binding.retires==1);
        assert(!children.valid()&&children.find(balloon)&&children.size()==1);
        assert(!children.balloonEndStep(balloon,false)&&!children.retire(balloon)&&binding.marks==1&&
            binding.retires==unsigned(!rejectMark));
    }
    for(unsigned failure=0;failure<=2;++failure){
        CameraCreateState state;RunnerImageValue x{0x80000000u},y{0x43300000u};
        unsigned calls=0;
        const bool ok=state.create(160,true,x,y,{0},{0x43400000u},
            [&](bool vertical,RunnerImageValue value,RunnerImageValue& accepted){
                ++calls;assert(vertical==(calls==2));
                if(calls==failure)return false;
                // Model the setter's retained negative-zero representation.
                if(vertical)y=value;else if(value.bits&0x7fffffffu)x=value;
                accepted=vertical?y:x;return true;
            });
        assert(ok==!failure&&calls==(failure?failure:2)&&state.daop==-32&&
            state.camSlowdown==5&&state.chasePlayer==1&&state.roomEnd==-1);
        assert(y.bits==(failure?0x43300000u:0x43400000u)&&x.bits==0x80000000u);
        assert(state.initialized==!failure);
        if(!failure)assert(state.xx.high==0x80000000u&&state.xx.low==0&&state.yy.high==0x40680000u);
        else assert(state.xx.zero()&&state.yy.zero());
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery})for(unsigned scenario=0;scenario<8;++scenario){
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),nullptr);
        assert(collision->attach(*scene));
        TimeoutDamageEvents events;events.immune=scenario==1;events.timeout=scenario!=2;
        events.stopped=scenario==3;events.stopSecond=scenario!=5&&scenario!=7;
        BoneProjectile caller;unsigned allocations=0;const auto draws=scene->randomDrawCount();
        const auto allocate=[&](std::uint32_t& id){
            assert(scene->randomDrawCount()==draws+allocations);
            id=210001+allocations++;
            if(scenario==6)id=0;
            return scenario!=4&&!(scenario==5&&allocations==2);
        };
        const bool okay=scene->typicalPlayerDamage(caller.damageScripts,MotionBinary64::fromInteger(1),
            MotionBinary64::fromInteger(3),MotionBinary64::fromInteger(2),events,allocate);
        const bool failure=scenario>=4&&scenario<=6;
        const unsigned count=scenario==0||scenario==5?1:scenario==7?2:0;
        assert(okay==!failure&&scene->effectsSucceeded()==!failure&&scene->timeFields().size()==count&&
            scene->randomDrawCount()==draws+count&&events.clears==unsigned(!failure&&scenario!=1)*2);
        assert(allocations==(scenario==4||scenario==6?1:scenario==5||scenario==7?2:count));
        assert(events.spin==(scenario!=1)&&events.grounded==(scenario==1)&&events.alarm==(scenario==1?-1:90));
        if(count){
            const auto* field=scene->timeFields().at(0);
            assert(field->sourceId==210001&&field->x==240&&field->y==40100);
            if(okay){RunnerImageValue x,y;x.assignInteger(240);y.assignInteger(40100);InstancePlaceResult result;
                assert(collision->playerTimeFieldInstance(x,y,masks,result)&&result.id>=210001);}
        }
        if(failure){const auto saved=allocations;
            assert(!scene->typicalPlayerDamage(caller.damageScripts,{}, {}, {},events,allocate)&&allocations==saved);}
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery}){
        const auto rb=read("generated/downwell-rooms.dwrm");RoomPack rooms(rb.data(),rb.size());
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,mode);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,true,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain(),&rooms);
        assert(collision->attach(*scene));const auto draws=scene->randomDrawCount();
        assert(scene->createTimeFieldWithSourceId(210001,240,176)&&scene->randomDrawCount()==draws+1);
        InstancePlaceResult result;
        assert(collision->playerTimeFieldInstance({0x43700000u},{0x43300000u},masks,result)&&result.id==210001&&
            result.x.high==MotionBinary64::fromInteger(240).high&&result.y.high==MotionBinary64::fromInteger(176).high);
        unsigned transitions=0;
        assert(scene->withPlayerTimeTransition([&](const auto& player,const auto&,int x,int y){
            ++transitions;RunnerImageValue px,py;bool hit=false;
            return px.assignInteger(player.x)&&py.assignInteger(player.y)&&
                collision->playerTimeFieldMeeting(MotionBinary64::fromInteger(x),MotionBinary64::fromInteger(y),masks,hit)&&hit&&
                collision->playerTimeFieldInstance(px,py,masks,result)&&result.id==210001;
        },[&]{scene->step(0);return scene->effectsSucceeded();})&&transitions==1);
        assert(collision->playerTimeFieldInstance({0x43700000u},{0x461c4000u},masks,result)&&result.id==-4);
        assert(scene->createGeneratedInstance({82,240,1000},objects,[](const auto&){return true;}));
        result.id=123;
        assert(!collision->playerTimeFieldInstance({0x43700000u},{0x447a0000u},masks,result)&&result.id==123&&!collision->valid());
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        InstanceHandle skeleton;
        assert(world->createSkeletonWithSourceId(607000,240,192,skeleton));
        assert(world->createGeneratedInstance({92,240,-10000},objects,[](const auto&){return true;}));
        const InstanceHandle first{0,world->breakableSlot(0).lifetimeId};
        InstanceHandle newborn,enemy;
        std::vector<int> seen;
        assert(owner->dispatchActorEndStep([&](InstanceHandle shared,int object){
            seen.push_back(object);
            if(object==35)return true;
            if(object==92){
                assert(shared.slot==GameplayCollisionView::breakableBase+first.slot&&shared.generation==first.generation);
                assert(world->markBreakableDestroyed(first));
                assert(world->createGeneratedInstance({92,256,-10000},objects,[](const auto&){return true;}));
                newborn={world->breakableSlots()-1,world->breakableSlot(world->breakableSlots()-1).lifetimeId};
                assert(world->createGeneratedInstance({111,240,-10000},objects,[](const auto&){return true;}));
                enemy=world->enemyHandle(world->enemyAt(0).lifetimeId);
            }else assert(object==120&&world->findBreakable(first)&&world->findBreakable(first)->destroyRequested);
            return true;
        }));
        assert((seen==std::vector<int>{35,92,120})); // Neither newborn family enters the captured phase.
        assert(owner->prepareDraw()&&!world->findBreakable(first)&&world->findBreakable(newborn));
        world->step(0);
        assert(world->effectsSucceeded()&&world->findBreakable(newborn)&&world->findBreakable(newborn)->destroyRequested&&
            world->resolveEnemyLifetime(enemy)&&world->resolveEnemyLifetime(enemy)->destroyRequested);
        assert(owner->prepareDraw()&&!world->findBreakable(newborn)&&!world->resolveEnemyLifetime(enemy));
        assert(owner->skeletons().find(skeleton));
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        InstanceHandle skeleton;
        assert(world->createSkeletonWithSourceId(607001,240,-10000,skeleton));
        assert(world->createGeneratedInstance({92,240,-10000},objects,[](const auto&){return true;}));
        const InstanceHandle box{0,world->breakableSlot(0).lifetimeId};
        const_cast<GameplayWorld::Breakable*>(world->findBreakable(box))->state.autoTile=true;
        world->step(0);
        assert(!world->effectsSucceeded()&&world->findBreakable(box)&&world->findBreakable(box)->destroyRequested&&
            !world->findBreakable(box)->state.checked&&owner->skeletons().find(skeleton)&&
            !owner->skeletons().find(skeleton)->destroyRequested);
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery}){
        const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),native?nullptr:&owner->terrain());
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain)));
        const int top=world->cameraOffsetY()+34;
        const int ys[]={top-161,top-160,top+3000,top+3001,top-200};
        InstanceHandle keys[5];
        for(unsigned i=0;i<5;++i){
            DeferredCreateInstance birth(93,240,ys[i]);birth.active=i!=4;
            assert(world->createGeneratedInstance(birth,objects,[&](const auto& c){
                return !native||(terrain->append(c.objectId,c.x,c.y,c.active)&&
                    world->finishNativeTerrainCreate(terrain->handle(terrain->size()-1)));
            }));
            const auto local=native?terrain->handle(terrain->size()-1):owner->terrain().handle(owner->terrain().size()-1);
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,keys[i]));
        }
        unsigned collisions=0;
        assert(world->withCollisionPass([&]{
            ++collisions;
            for(const auto key:keys){GameplayWorld::NativeTerrain::Identity id;bool active,destroyed;
                assert(GameplayCollisionView(*world).readTerrain(key,id,active,destroyed)&&!destroyed);}
            return true;
        },[&]{world->step(0);return world->effectsSucceeded();})&&collisions==1);
        for(unsigned i=0;i<5;++i){GameplayWorld::NativeTerrain::Identity id;bool active,destroyed;
            assert(GameplayCollisionView(*world).readTerrain(keys[i],id,active,destroyed)&&
                destroyed==(i==0||i==3)&&active==(i!=4));}
        assert(owner->prepareDraw());
        for(unsigned i=0;i<5;++i)
            assert(GameplayCollisionView(*world).contains(keys[i])==(i!=0&&i!=3));
    }
    {
        std::vector<std::uint32_t> expected;
        const auto rb=read("generated/downwell-rooms.dwrm");RoomPack rooms(rb.data(),rb.size());
        for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                      GameplayCollisionOwner::Ownership::nativeScenery}){
            auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,mode);
            auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
                collision->breakableBinding(),collision->playerBinding(),&collision->terrain(),&rooms);
            assert(collision->attach(*scene));std::vector<std::uint32_t> actual;
            const auto query=[&](int qx,int qy){
                auto x=MotionBinary64::fromInteger(qx);x.high-=2u<<20;
                GameplayCollisionOwner::RegisteredFieldContact result;
                assert(collision->playerTimeFieldSelection(x,MotionBinary64::fromInteger(qy),masks,result));
                actual.insert(actual.end(),{unsigned(result.found),result.handle.slot,result.handle.generation,result.x.bits,result.y.bits});
                bool hit=false;assert(collision->playerTimeFieldMeeting(x,MotionBinary64::fromInteger(qy),masks,hit)&&hit==result.found);
            };
            query(960,512); // Empty family must not move the caller.
            assert(scene->createGeneratedInstance({82,240,512},objects,[](const auto&){return true;}));
            for(int x=600;x<=1320;x+=7)query(x,512);
            assert(scene->createRoomStartField()&&scene->registerLevelExit(512));
            for(unsigned i=0;i<40;++i)
                assert(scene->createGeneratedInstance({82,224+int(i%4)*16,480+int(i/4)*16},objects,[](const auto&){return true;}));
            for(int facing:{-1,1}){
                auto binding=collision->playerBinding();const auto* record=scene->findPlayer(scene->playerHandle());assert(record);
                assert(binding.integerTransform(scene->playerHandle(),*record,RunnerTransformField::scaleX,facing,{0x3f800000u},0xffffffu));
                for(int y:{160,480,512,600,10000})for(int x=600;x<=1320;x+=17)query(x,y);
            }
            if(mode==GameplayCollisionOwner::Ownership::reference)expected=actual;else assert(actual==expected);
        }
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery}){
        const auto rb=read("generated/downwell-rooms.dwrm");RoomPack rooms(rb.data(),rb.size());
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms);
        assert(owner->attach(*world));
        auto& controllers=owner->playerControllers();InstanceHandle meter,camera,shared;
        RunnerImageValue x,y,negative;x.assignInteger(240);y.assignInteger(176);negative.assignInteger(-100);
        const auto draws=world->randomDrawCount();
        assert(controllers.create(66,105187,negative,y,meter)&&controllers.create(64,105188,x,y,camera));
        assert(controllers.size()==2&&controllers.find(meter)->sourceId==105187&&controllers.find(camera)->sourceId==105188);
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::playerController,camera,shared));
        checkTypedCollisionView(GameplayCollisionView(*world),GameplayCollisionView::Kind::playerController,camera);
        PackedObject cameraObject;assert(objects.findObject(64,cameraObject));
        if(mode==GameplayCollisionOwner::Ownership::reference)
            assert(owner->state(shared)&&owner->properties(shared)->sprite.spriteId==cameraObject.spriteId);
        auto flags=[&]{std::uint32_t result=0;unsigned found=0;
            assert(owner->visitRegistrationOrder([&](auto key,int object,std::uint32_t value){
                if(key.slot==shared.slot&&key.generation==shared.generation){assert(object==64);result=value;++found;}return true;
            })&&found==1);return result;};
        assert(controllers.activation(camera,false)&&!controllers.find(camera)->active&&(flags()&2u));
        assert(controllers.activation(camera,true)&&!(flags()&2u));
        x.bits=0x43704000u; // 240.25: no integer position substitute.
        assert(controllers.position(camera,x,y)&&controllers.find(camera)->x.bits==x.bits);
        if(mode==GameplayCollisionOwner::Ownership::reference)assert(owner->state(shared)->x.bits==x.bits);
        RunnerImageValue playerX{0x43718000u},playerY{0x43344000u}; // 241.5, 180.25
        assert(controllers.initializeCamera(camera,480,false,playerX,playerY));
        const auto& initialized=controllers.find(camera)->camera;
        assert(initialized.initialized&&initialized.roomw==480&&initialized.daop==24&&
            initialized.camSlowdown==5&&initialized.camSlowdownX==1&&initialized.chasePlayer==1&&
            initialized.centered==1&&initialized.xAhead==16&&initialized.roomEnd==-1&&
            initialized.freeCam==0&&initialized.autoScroll==0);
        auto sameDouble=[](MotionBinary64 value,double expected){
            std::uint64_t bits;std::memcpy(&bits,&expected,8);
            return value.high==std::uint32_t(bits>>32)&&value.low==std::uint32_t(bits);
        };
        assert(sameDouble(initialized.camAcclx,(80.0-240.25)/10.0)&&
            sameDouble(initialized.camAccly,(180.25+24.0-176.0)/5.0)&&
            sameDouble(initialized.xx,241.5)&&sameDouble(initialized.yy,180.25)&&
            controllers.find(camera)->x.bits==playerX.bits&&controllers.find(camera)->y.bits==playerY.bits);
        unsigned found=0;assert(owner->visitRegistrationOrder([&](auto key,int object,auto){
            if(key.slot==shared.slot&&key.generation==shared.generation){assert(object==64);++found;}return true;
        })&&found==1&&world->randomDrawCount()==draws);
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,43,300)==ResultsDestination::gameplay);
        assert(owner->valid()&&world->effectsSucceeded()&&controllers.size()==0&&!controllers.find(camera)&&!owner->state(shared));
        InstanceHandle next;
        assert(controllers.createCamera(105200,x,y,160,true,playerX,playerY,next)&&
            next.generation!=camera.generation&&controllers.find(next)->camera.initialized&&
            controllers.find(next)->camera.daop==-32&&controllers.clear());
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));auto& children=owner->playerControllers();
        auto& companionUpgrades=const_cast<UpgradeInventory&>(world->upgrades());
        companionUpgrades.levels[15]=1;
        InstanceHandle balloon,first,second;const auto draws=world->randomDrawCount();
        assert(children.createCompanion(256,106001,{0x43704000u},{0x43308000u},balloon)&&
            children.createCompanion(249,106002,{0x43700000u},{0x43300000u},first)&&
            children.createCompanion(249,106003,{0x43700000u},{0x43300000u},second)&&children.reverseDrone(second));
        const auto* b=children.find(balloon);const auto* d=children.find(first);const auto* d2=children.find(second);
        assert(b&&d&&d2&&b->imageSpeed.bits==0x3da3d70au&&d->imageSpeed.bits==0x3f000000u&&
            b->companion.balloon.xx.high==b->x.promoted().high&&b->companion.balloon.xx.low==b->x.promoted().low&&
            d2->companion.drone.xdir.high==MotionBinary64::fromInteger(180).high&&world->randomDrawCount()==draws);
        // Test initial sequence advancement through the actual mixed room walk.
        PlayerCompanionCreateState oracleBalloon=b->companion,oracleDrone=d->companion;
        RunnerImageValue bi=b->imageIndex,di=d->imageIndex;
        auto& balloonState=const_cast<PlayerCompanionCreateState&>(b->companion);
        auto& droneState=const_cast<PlayerCompanionCreateState&>(d->companion);
        auto& secondState=const_cast<PlayerCompanionCreateState&>(d2->companion);
        balloonState.balloon.noHold=droneState.drone.shotDelay=secondState.drone.droneShooting=MotionBinary64::fromInteger(3);
        assert(children.setAlarm(balloon,0,1)&&children.setAlarm(first,2,1)&&children.setAlarm(second,3,1));
        for(unsigned tick=0;tick<16;++tick){
            assert(oracleBalloon.advanceImage(256,526,bi,b->imageSpeed,[&](auto v){bi=v;return true;})&&
                oracleDrone.advanceImage(249,131,di,d->imageSpeed,[&](auto v){di=v;return true;}));
            world->step(0);assert(world->effectsSucceeded()&&owner->valid());
            assert(b->imageIndex.bits==bi.bits&&d->imageIndex.bits==di.bits&&d2->imageIndex.bits==di.bits);
            assert(b->alarms.get(0)==(tick?-1:0)&&d->alarms.get(2)==(tick?-1:0)&&d2->alarms.get(3)==(tick?-1:0));
            assert(b->companion.balloon.noHold.zero()&&d->companion.drone.shotDelay.zero()&&
                d2->companion.drone.droneShooting.high==MotionBinary64::fromInteger(3).high);
        }
        assert(!b->destroyRequested);
        companionUpgrades.levels[15]=0;
        assert(children.activation(balloon,false));
        world->step(0);assert(world->effectsSucceeded()&&!b->destroyRequested);
        assert(children.activation(balloon,true));
        // Pending activation joins at the normal frame boundary.
        world->step(0);world->step(0);
        assert(world->effectsSucceeded()&&children.find(balloon)&&b->destroyRequested);
        unsigned endCalls=0;
        assert(owner->dispatchActorEndStep([&](auto,int object){assert(object==35);++endCalls;return true;})&&endCalls==1);
        assert(owner->prepareDraw()&&!children.find(balloon)&&children.size()==2);
        InstanceHandle replacement;
        assert(children.createCompanion(256,106004,{},{},replacement)&&replacement.generation>balloon.generation);
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,43,300)==ResultsDestination::gameplay&&
            children.size()==0&&!children.find(balloon)&&!children.find(first)&&!children.find(second));
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery})for(unsigned failure=0;failure<=6;++failure){
        const auto rb=read("generated/downwell-rooms.dwrm");RoomPack rooms(rb.data(),rb.size());
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms);
        assert(owner->attach(*world));
        // Source can branch on value two even though normal offers cap Drone.
        auto& upgrades=const_cast<UpgradeInventory&>(world->upgrades());upgrades.levels[9]=2;upgrades.levels[15]=1;
        unsigned calls=0;const int expected[]={256,249,249,66,64,73};
        const auto draws=world->randomDrawCount();
        auto allocate=[&](int object,unsigned& id){
            assert(calls<6&&object==expected[calls]);++calls;id=150000+calls*3;return calls!=failure;
        };
        const bool ok=world->createPlayerChildren(allocate);
        assert(ok==!failure&&calls==(failure?failure:6)&&world->effectsSucceeded()==!failure&&world->randomDrawCount()==draws);
        const auto& children=owner->playerControllers();
        const unsigned born=failure?failure-1:6;
        assert(children.size()==(born<5?born:5)&&owner->entryDithers().size()==unsigned(born==6));
        for(unsigned i=0;i<children.size();++i){const auto* child=children.find(children.handle(i));
            assert(child&&child->objectId==expected[i]&&child->sourceId==150003+i*3);
        }
        const auto* player=world->findPlayer(world->playerHandle());assert(player);
        assert(player->cameraSourceId==(born>=5?150015u:0u));
        assert(world->droneNumber()==unsigned(born>=2));
        if(born>=3){const auto* second=children.find(children.handle(2));
            assert(second->companion.drone.xdir.high==MotionBinary64::fromInteger(180).high);}
        if(ok){
            const auto* wipe=owner->entryDithers().find(owner->entryDithers().handle(0));
            const auto* camera=children.find(children.sourceHandle(player->cameraSourceId));
            assert(wipe&&camera&&camera->camera.initialized&&wipe->sourceId==150018&&
                wipe->x.bits==camera->x.bits&&wipe->y.bits==world->roomEntryView()->y.bits&&wipe->effect.alarm0==120);
            assert(world->restartFromResults(DeathMenuSequence::Action::Retry,43,300)==ResultsDestination::gameplay&&
                children.size()==0&&owner->entryDithers().size()==0);
            unsigned retryCalls=0;const int retryOrder[]={66,64,73};
            assert(world->createPlayerChildren([&](int object,unsigned& id){
                assert(retryCalls<3&&object==retryOrder[retryCalls]);id=170000+ ++retryCalls;return true;
            })&&retryCalls==3&&children.size()==2&&owner->entryDithers().size()==1&&world->droneNumber()==0);
            assert(world->findPlayer(world->playerHandle())->cameraSourceId==170002);
        }else{
            assert(!world->createPlayerChildren(allocate)&&calls==failure);
        }
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery})for(unsigned failure=0;failure<=3;++failure){
        const auto rb=read("generated/downwell-rooms.dwrm");RoomPack rooms(rb.data(),rb.size());
        PackedRoom room;assert(rooms.findRoom(11,room));
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms,
            GameplayWorld::PlayerBirthTiming::authoredRoom);
        assert(owner->attach(*world));
        unsigned allocations=0,walls=0;const int expected[]={66,64,73};
        auto allocate=[&](int object,unsigned& id){
            assert(allocations<3&&object==expected[allocations]&&!walls);
            assert(world->playerHandle().generation&&!world->awaitingPlayerBirth());
            assert(world->builderController()->handle().generation&&!world->ditherAbove()->handle().generation);
            id=180000+ ++allocations;return allocations!=failure;
        };
        const bool ok=world->withPlayerChildCreation(allocate,[&]{
            return world->loadRoomTerrainWithStartField(rooms,room,[&](const auto&){
                assert(allocations==3&&owner->entryDithers().size()==1);
                ++walls;return true;
            });
        });
        assert(ok==!failure&&allocations==(failure?failure:3)&&walls==(failure?0u:240u));
        unsigned position=0;const int prefix[]={65,35,66,64,73,61};
        assert(owner->visitRegistrationOrder([&](auto,int object,auto){
            if(position<6){assert(object==prefix[position]);++position;}return true;
        }));
        assert(position==(failure?failure+1:6));
        if(ok){
            const auto* wipe=owner->entryDithers().find(owner->entryDithers().handle(0));
            assert(wipe&&wipe->y.bits==world->roomEntryView()->y.bits);
        }
        bool invoked=false;
        assert(!world->withPlayerChildCreation(allocate,[&]{invoked=true;return true;})&&!invoked);
    }
    for(int area=1;area<=5;++area){
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));
        DebrisEffects<2> effects;auto& registry=collision->debris();InstanceHandle local,key;
        const MotionBinary64 x{0x406e0800u,0},y{0x40660100u,0};
        auto expectedRandom=scene->randomState();DebrisAnimation expected;
        assert(expected.create(x,y,area,expectedRandom));
        assert(scene->spawnDebris(effects,x,y,area,local));
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,local,key));
        GameplayCollisionView::Kind kind;InstanceHandle decoded;
        assert(GameplayCollisionView::decode(key,kind,decoded)&&kind==GameplayCollisionView::Kind::debris&&
            decoded.slot==local.slot&&decoded.generation==local.generation);
        const auto* record=registry.find(local);assert(record);
        const auto state=collision->state(key);const auto properties=collision->properties(key);
        RunnerImageValue depth;assert(depth.assign(MotionBinary64::fromInteger(-1000)));
        assert(state&&state->x.bits==expected.x.bits&&state->y.bits==expected.y.bits&&
            properties&&properties->depth.bits==depth.bits&&properties->sprite.spriteId==expected.spriteId&&
            properties->sprite.imageIndex.bits==expected.imageIndex.bits);
        assert(scene->randomState().drawCount()==expectedRandom.drawCount());
        assert(scene->randomState().stateIndex()==expectedRandom.stateIndex());
        for(unsigned word=0;word<16;++word)assert(scene->randomState().stateWord(word)==expectedRandom.stateWord(word));
        unsigned count=0;assert(collision->population(370,count)&&count==1);
        assert(expected.advanceImage());scene->step(0);
        assert(scene->effectsSucceeded()&&record->effect->imageIndex.bits==expected.imageIndex.bits&&
            collision->properties(key)->sprite.imageIndex.bits==expected.imageIndex.bits);
        assert(collision->state(key)->previous.x.bits==expected.x.bits);
        unsigned writes=0;
        assert(effects.stepRegisteredOne(local.generation,effects.beginPhase(),expectedRandom,
            [](auto,const auto&,bool& stopped){stopped=false;return true;},
            [&](auto,const auto& effect,auto,auto,bool& hit){
                assert(collision->state(key)->x.bits==effect.x.bits&&collision->state(key)->y.bits==effect.y.bits);
                hit=false;return true;
            },[&](auto h,const auto& effect,auto px,auto py){++writes;return registry.move(h,effect,px,py);},registry));
        assert(writes==2&&collision->state(key)->x.bits==record->effect->x.bits&&
            collision->state(key)->y.bits==record->effect->y.bits);
        effects.get(local.generation)->alarms.set(0,1);
        assert(effects.advanceRegisteredAlarmOne(local.generation,effects.beginPhase(),0,registry));
        assert(collision->state(key)->flags&1u);
        assert(registry.find(local)==record&&effects.retained(local.generation)==record->effect);
        assert(collision->prepareDraw()&&!collision->state(key)&&!registry.find(local)&&registry.size()==0);
        assert(collision->population(370,count)&&count==0);
        InstanceHandle next;assert(scene->spawnDebris(effects,x,y,area,next)&&next.generation>local.generation);
        assert(scene->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none&&
            !registry.size()&&!effects.size()&&scene->debrisInstances()==&registry&&collision->valid());
        InstanceHandle renewed;assert(scene->spawnDebris(effects,x,y,area,renewed)&&renewed.generation>next.generation);
        scene->step(0);assert(scene->effectsSucceeded()&&effects.clear(registry));
    }
    {
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));
        ScriptSpawnEffects<2> effects;auto& registration=collision->scriptEffects();
        const auto phase=effects.beginPhase();
        InstanceHandle caller;
        auto frames=[&](int id,unsigned& count){PackedSpriteCollision resource;
            if(!sprites.sprite(id,resource))return false;count=resource.frameCount;return true;};
        assert(scene->spawnScriptEffect(effects,{0x406e0800u,0},{0x40afff00u,0},603,
            {0x3fe00000u,0},MotionBinary64::fromInteger(270),{},frames,caller));
        const auto first=caller.generation;
        assert(first&&registration.size()==1);
        InstanceHandle key;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::scriptEffect,registration.handle(first),key));
        const auto* effect=effects.get(first);const auto state=collision->state(key);
        assert(effect&&state&&state->x.bits==effect->x.bits&&state->y.bits==effect->y.bits);
        assert(collision->properties(key)->sprite.spriteId==603&&collision->properties(key)->sprite.angle.bits==0x43870000u);
        unsigned count=0;assert(collision->population(219,count)&&count==1);
        assert(scene->advanceScriptEffectAnimation(effects,phase)&&effects.get(first)->imageIndex.bits==0);
        assert(scene->stepWithScriptEffects(effects,0));
        assert(collision->properties(key)->sprite.imageIndex.bits==0x3f000000u);
        assert(collision->state(key)->previous.x.bits==effect->x.bits&&collision->state(key)->previous.y.bits==effect->y.bits);
        const auto second=effects.create({}, {},registration);assert(second>first);
        assert(effects.setSprite(second,602,1,registration));
        assert(effects.mutate(second,[](auto& e){e.imageSpeed={0x3f000000u};return true;}));
        assert(scene->stepWithScriptEffects(effects,0)&&collision->state(key)&&
            (collision->state(key)->flags&1u)&&registration.size()==2&&collision->properties(key)->sprite.imageIndex.bits==0);
        assert(effects.get(second)&&effects.get(second)->imageIndex.bits==0x3f000000u);
        assert(scene->stepWithScriptEffects(effects,0)&&!effects.get(second)&&registration.size()==2);
        assert(collision->prepareDraw(effects));
        assert(!collision->state(key)&&registration.size()==0);
        assert(collision->population(219,count)&&count==0);
        assert(scene->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none&&
            scene->scriptSpawnInstances()==&registration);
        const auto next=effects.create({}, {},registration);assert(next>first&&!collision->state(key));
        assert(effects.setSprite(next,602,1,registration));
        const auto oldHandle=registration.handle(next);
        assert(scene->restartFromResultsWithScriptEffects(DeathMenuSequence::Action::None,43,300,effects)==ResultsDestination::none&&
            registration.find(oldHandle)&&effects.size()==1&&scene->effectsSucceeded());
        assert(scene->restartFromResultsWithScriptEffects(DeathMenuSequence::Action::Retry,43,300,effects)!=ResultsDestination::none&&
            effects.size()==0&&registration.size()==0&&!registration.find(oldHandle)&&collision->valid());
        const auto renewed=effects.create({}, {},registration);
        assert(renewed>next&&!registration.find(oldHandle)&&effects.clear(registration));
        assert(scene->spawnScriptEffect(effects,{},{},603,{0x3fe00000u,0},{},{},frames,caller));
        const auto automatic=caller;
        scene->step(0);
        assert(scene->effectsSucceeded()&&effects.get(automatic.generation)->imageIndex.bits==0x3f000000u);
        assert(collision->prepareDraw());
        scene->step(0);
        assert(scene->effectsSucceeded()&&collision->prepareDraw()&&effects.size()==0&&!registration.find(automatic));
        assert(scene->spawnScriptEffect(effects,{},{},603,{0x3fe00000u,0},{},{},frames,caller));
        const auto beforeRetry=caller;
        assert(scene->restartFromResults(DeathMenuSequence::Action::Retry,44,300)!=ResultsDestination::none&&
            effects.size()==0&&!registration.find(beforeRetry)&&collision->valid());
        // Failure after Create must poison the actual runtime, retain its
        // completed birth/caller publication and never repeat the event.
        const auto draws=scene->randomDrawCount();unsigned lookups=0;
        auto rejectFrames=[&](int,unsigned&){++lookups;return false;};
        assert(!scene->spawnScriptEffect(effects,{},{},603,{0x3fe00000u,0},{},{},rejectFrames,caller));
        assert(!scene->effectsSucceeded()&&registration.find(caller)&&effects.size()==1&&lookups==1);
        assert(!scene->spawnScriptEffect(effects,{},{},603,{},{},{},rejectFrames,caller)&&lookups==1&&
            scene->randomDrawCount()==draws);
        assert(!scene->advanceScriptEffectAnimation(effects,effects.beginPhase())&&effects.size()==1);
        assert(scene->restartFromResultsWithScriptEffects(DeathMenuSequence::Action::Retry,43,300,effects)==ResultsDestination::none&&
            effects.size()==1&&registration.find(caller)&&scene->randomDrawCount()==draws);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        InstanceHandle previous{};
        auto frames=[&](int id,unsigned& count){PackedSpriteCollision r;if(!sprites.sprite(id,r))return false;
            count=r.frameCount;return true;};
        for(unsigned cycle=0;cycle<3;++cycle){
            InstanceHandle caller;
            assert(world->spawnScriptEffect({},{},603,{0x3fe00000u,0},{},{},frames,caller)&&
                caller.generation>previous.generation&&!owner->scriptEffects().find(previous));
            world->step(0);
            assert(world->effectsSucceeded()&&world->scriptEffects().size()==1&&
                world->scriptEffects().atNewest(0)->imageIndex.bits==0x3f000000u&&owner->prepareDraw());
            world->step(0);
            assert(world->effectsSucceeded()&&owner->prepareDraw()&&world->scriptEffects().size()==0);
            assert(world->spawnScriptEffect({},{},602,{0x3fe00000u,0},{},{},frames,caller));previous=caller;
            assert(world->restartFromResults(DeathMenuSequence::Action::Retry,43+cycle,300)!=ResultsDestination::none&&
                world->scriptEffects().size()==0&&!owner->scriptEffects().find(previous)&&world->hasRoomActorDraw());
            assert(world->visitRoomActors([](auto kind,auto,auto){return kind!=GameplayWorld::DrawActor::scriptEffect;}));
        }
    }
    assert(masks.valid());
    std::vector<unsigned> shootableReference;
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
        GameplayCollisionOwner::Ownership::nativeScenery}){
        const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),owner->breakableBinding(),
            owner->playerBinding(),native?nullptr:&owner->terrain());
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain))&&owner->configureCorpseMasks(masks));
        world->step(4);assert(world->effectsSucceeded()&&world->bulletCount()==1);
        const auto& bullet=world->bulletAt(0);InstanceHandle shot;
        const InstanceHandle shotLocal{bullet.identitySlot,bullet.lifetimeId};
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::bullet,shotLocal,shot));
        RunnerImageValue px,py;assert(px.assignInteger(240)&&py.assignInteger(512)&&world->publishBulletCollisionPosition(shotLocal,px,py));
        std::vector<unsigned> selections;std::vector<InstanceHandle> bones;
        auto query=[&]{GameplayCollisionOwner::Owner::Result result;
            assert(owner->projectileShootableSelection(shot,px.promoted(),py.promoted(),masks,result));
            unsigned id=0;
            if(result.found){GameplayCollisionView::Record record;assert(GameplayCollisionView(*world).resolve(result.handle,record));
                assert(record.bone||record.corpse);id=record.bone?record.bone->sourceId:record.corpse->sourceId;}
            selections.push_back(id);return result;
        };
        assert(!query().found);
        for(unsigned i=0;i<12;++i){
            InstanceHandle bone;assert(world->createBoneWithSourceId(605200+i,240,512,bone));bones.push_back(bone);
            if(i==0){
                assert(query().found);
                DeadBodySource source{px.promoted(),py.promoted(),{},MotionBinary64::fromInteger(-1)};GameMakerRandom random(2);
                assert(owner->registeredCorpses().createFromDeath(605300,source,206,{},102,false,random,
                    [](auto,int,auto,auto,bool& hit){hit=false;return true;}));
            }
            assert(query().found);
        }
        bool hit=false;assert(owner->projectileShootableMeeting(shot,px.promoted(),py.promoted(),masks,hit)&&hit);
        // Mutation after the boolean query must be observed by the separate
        // identity query, including when it switches to another family owner.
        for(auto bone:bones)assert(owner->bones().activate(bone,false));
        auto selected=query();GameplayCollisionView::Record record;
        assert(selected.found&&GameplayCollisionView(*world).resolve(selected.handle,record)&&record.corpse&&record.corpse->sourceId==605300);
        MotionBinary64 health;bool effect=true;const auto beforeWrite=world->randomDrawCount();
        assert(owner->projectileShootableFields(selected.handle,health,effect)&&health.zero()&&!effect);
        assert(owner->projectileShootableHealthWrite(selected.handle,{0xc0040000u,0})&&owner->projectileShootableHit(selected.handle)&&
            owner->projectileShootableFields(selected.handle,health,effect)&&health.high==0xc0040000u&&!health.low&&
            record.corpse->state.hit&&!record.corpse->destroyRequested&&world->randomDrawCount()==beforeWrite);
        const auto corpse=owner->registeredCorpses().sourceHandle(605300);
        assert(owner->registeredCorpses().activate(corpse,false)&&!query().found);
        assert(owner->bones().activate(bones.front(),true)&&query().found);
        assert(owner->bones().markDestroyed(bones.front())&&!query().found);
        assert(owner->prepareDraw()&&!owner->bones().find(bones.front()));
        InstanceHandle replacement;assert(world->createBoneWithSourceId(605400,240,512,replacement)&&query().found);
        assert(selections.back()==605400);
        assert(px.assignInteger(1000)&&!query().found); // Query restores the shooter's published position.
        RunnerImageValue actualX,actualY;
        if(native)assert(owner->nativePublishedPosition(shot,actualX,actualY));
        else {const auto state=owner->state(shot);assert(state);actualX=state->x;actualY=state->y;}
        assert(actualX.bits==0x43700000u&&actualY.bits==0x44000000u);
        if(!native)shootableReference=selections;else assert(selections==shootableReference);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));world->step(4);
        assert(world->effectsSucceeded()&&world->bulletCount()==1);
        const auto& shot=world->bulletAt(0);const InstanceHandle key{shot.identitySlot,shot.lifetimeId};
        InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::bullet,key,shared));
        const auto original=*owner->state(shared);const auto trajectory=shot.trajectory;
        MachineGunSourceMotion motion;GameMakerRandom random(2);
        assert(motion.reset(original.x.promoted(),original.y.promoted())&&motion.initialize(random));
        assert(world->markBulletDestroyed(key));unsigned writes=0;
        assert(motion.commitPosition([&](RunnerImageValue x,RunnerImageValue y){
            assert(writes==0?y.bits==original.y.bits:x.bits==motion.x.bits);
            assert(world->publishBulletCollisionPosition(key,x,y));
            const auto state=owner->state(shared);
            assert(state&&state->x.bits==x.bits&&state->y.bits==y.bits&&(state->flags&1u));
            assert(state->previous.x.bits==original.previous.x.bits&&state->previous.y.bits==original.previous.y.bits);
            ++writes;return true;
        }));
        assert(writes==2&&shot.destroyRequested&&shot.trajectory.x.pixel==trajectory.x.pixel&&
            shot.trajectory.x.fraction==trajectory.x.fraction&&shot.trajectory.y.pixel==trajectory.y.pixel&&
            shot.trajectory.y.fraction==trajectory.y.fraction);
        const auto last=*owner->state(shared);
        assert(!world->publishBulletCollisionPosition(key,{0x7f800000u},{})&&!world->effectsSucceeded());
        assert(owner->state(shared)->x.bits==last.x.bits&&owner->state(shared)->y.bits==last.y.bits);
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
        GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));InstanceHandle shot;
        assert(world->withCollisionPass([&]{
            assert(world->bulletCount()==1);const auto& r=world->bulletAt(0);shot={r.identitySlot,r.lifetimeId};
            RunnerImageValue x,y;assert(x.assignInteger(240)&&y.assignInteger(10000));
            return world->publishBulletCollisionPosition(shot,x,y)&&!world->resolveBulletLifetime(shot)->destroyRequested;
        },[&]{world->step(4);return world->effectsSucceeded();}));
        assert(world->resolveBulletLifetime(shot)&&world->resolveBulletLifetime(shot)->destroyRequested);
        assert(owner->prepareDraw()&&!world->resolveBulletLifetime(shot));
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));unsigned calls=0,initializations=0;InstanceHandle shot;
        auto frames=[&](int id,unsigned& count){return sprites.frameCount(id,count);};
        for(unsigned tick=0;tick<3;++tick){
            assert(world->withSourceBulletSteps([&](InstanceHandle key,MachineGunSourceMotion& motion){
                shot=key;++calls;const bool first=!motion.initialized;initializations+=first;
                const auto before=world->randomDrawCount();ScheduledEmptyBulletOps<GameplayWorld> ops{*world,key};
                return world->stepSourceBullet(key,motion,ops,frames)&&ops.writes==2&&world->randomDrawCount()==before+unsigned(first);
            },[&]{world->step(tick?0:4);return world->effectsSucceeded();}));
            assert(calls==tick+1&&initializations==1&&world->resolveBulletLifetime(shot)->sourceOwned&&
                !world->resolveBulletLifetime(shot)->trajectory.initialized);
        }
        const auto& motion=world->resolveBulletLifetime(shot)->sourceMotion;
        InstanceHandle shard,sharedShard,sharedShot;MotionBinary64 vx,vy;
        assert(world->sourceBulletVelocity(shot,vx,vy)&&vx.high==motion.xsp.high&&vx.low==motion.xsp.low&&
            vy.high==motion.ysp.high&&vy.low==motion.ysp.low);
        assert(world->createLimboShardWithSourceId(605501,motion.x.promoted(),motion.y.promoted(),false,shard)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::limboShard,shard,sharedShard)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::bullet,shot,sharedShot));
        RunnerCollisionEventContext contact;const auto contactDraws=world->randomDrawCount();
        assert(owner->dispatchCollisionEvent(contact,sharedShard,sharedShot,{true,false},[&](const auto& event){
            return owner->dispatchLimboShardBulletCollision(event);
        }));
        const auto* touched=owner->limboShards().find(shard);
        assert(touched&&touched->state.noTouch&&touched->state.ysp.high==vy.high&&touched->state.ysp.low==vy.low&&
            world->randomDrawCount()==contactDraws+1);
        auto stale=shot;++stale.generation;
        assert(!world->sourceBulletVelocity(stale,vx,vy)&&vy.high==motion.ysp.high&&vy.low==motion.ysp.low);
        const auto before=world->randomDrawCount();world->step(0);
        assert(!world->effectsSucceeded()&&world->randomDrawCount()==before); // Never silently resume the legacy path.
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},nullptr,mode);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        struct BeforeFirstStep {
            GameplayWorld& world;GameplayCollisionOwner& owner;unsigned writes=0;
            RunnerImageValue x{0x43704000u},y{0x43304000u}; // Absolute 240.25, 176.25.
            bool boundary(GameplayWorld::PhaseBoundary phase){
                if(phase==GameplayWorld::PhaseBoundary::beginStep){
                    assert(world.bulletCount()==1);const auto& bullet=world.bulletAt(0);
                    assert(!bullet.sourceOwned&&!bullet.sourceMotion.initialized&&!bullet.trajectory.initialized);
                    assert(world.publishBulletCollisionPosition({bullet.identitySlot,bullet.lifetimeId},x,y));++writes;
                }
                return owner.reconcileActivation();
            }
        } before{*world,*owner};
        // Isolated first-admission probe: stop inside the first Step, before
        // any effect/depth mutation, rather than install a fake game backend.
        world->configurePhaseBoundary([](void* p,auto phase){return static_cast<BeforeFirstStep*>(p)->boundary(phase);},&before);
        unsigned steps=0;
        assert(!world->withSourceBulletSteps([&](InstanceHandle shot,MachineGunSourceMotion& motion){
            ++steps;assert(before.writes==1&&motion.x.bits==before.x.bits&&motion.y.bits==before.y.bits&&!motion.initialized);
            InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::bullet,shot,shared));
            RunnerImageValue x,y;
            if(mode==GameplayCollisionOwner::Ownership::reference){const auto* state=owner->state(shared);assert(state);x=state->x;y=state->y;}
            else assert(owner->nativePublishedPosition(shared,x,y));
            assert(x.bits==motion.x.bits&&y.bits==motion.y.bits);
            return false; // Intentional terminal stop after observing admission.
        },[&]{world->step(4);return world->effectsSucceeded();}));
        assert(steps==1&&!world->effectsSucceeded());
    }
    std::vector<std::uint32_t> scheduledContacts[2][6];
    const auto sourceRoomBytes=read("generated/downwell-rooms.dwrm");
    RoomPack sourceRooms(sourceRoomBytes.data(),sourceRoomBytes.size());assert(sourceRooms.valid());
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery})for(unsigned crowd:{0u,12u})for(unsigned target:{0u,1u,2u,3u,4u,5u}){
        const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&sourceRooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),native?nullptr:&owner->terrain(),&sourceRooms);
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain)));
        InstanceHandle bone,shot,skeleton;unsigned calls=0;
        if(target==2)assert(world->createBoneWithSourceId(605500,240,200,bone));
        else if(target==4){
            assert(owner->configureCorpseMasks(masks));
            DeadBodySource source{MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(500),{},MotionBinary64::fromInteger(-1)};
            GameMakerRandom corpseRandom(2);
            assert(owner->registeredCorpses().createFromDeath(605502,source,206,{},102,false,corpseRandom,
                [](auto,int,auto,auto,bool& hit){hit=false;return true;}));
        }else assert(world->createGeneratedInstance({target==0?92:60,240,512},objects,[&](const auto& command){
            return !native||command.objectId!=60||(terrain->append(command.objectId,command.x,command.y)&&
                world->finishNativeTerrainCreate(terrain->handle(terrain->size()-1)));
        }));
        // Source Create snaps to a 16px grid. At 496 the bullet's endpoint
        // has already passed this mask; 512 exercises actual post-Step overlap.
        if(target==3||target==5)assert(world->createSkeletonWithSourceId(605503,240,target==3?512:500,skeleton));
        for(unsigned i=0;i<crowd;++i)
            assert(world->createGeneratedInstance({92,1000,600+int(i)*16},objects,[](const auto&){return true;}));
        auto frames=[&](int id,unsigned& count){return sprites.frameCount(id,count);};
        int controllerSpeed=60;InstanceHandle controller;
        if(target==3||target==5)assert(world->bindControllerRoomSpeed(controllerSpeed)&&
            owner->runControllers().create(605504,{}, {},[](InstanceHandle){return true;},controller));
        const auto sourceStep=[&](InstanceHandle key,MachineGunSourceMotion& motion){
            shot=key;++calls;RunnerImageValue x,y;
            assert(x.assignInteger(240)&&y.assignInteger(target==2?200:500)&&
                world->publishBulletCollisionPosition(key,x,y));
            SoundPlayEnvironment environment;environment.gameVolume={0x3fd00000u,0};environment.inWater=true;
            BulletWallAudio audio{world->resolveBulletLifetime(key)->sound,world->overlapSound(2),29,true};
            const auto before=world->randomDrawCount();
            const bool ok=NativeSourceBulletStep::run(*world,*owner,key,motion,masks,frames,environment,audio);
            if(!ok||world->randomDrawCount()!=before+(target==1?3u:1u)||!audio.valid||audio.calls!=(target==1?7u:0u))
                std::fprintf(stderr,"scheduled source contact mode=%u target=%u ok=%u rng=%u audio=%u/%u owner=%u world=%u initialized=%u effects=%u\n",
                    unsigned(mode),target,unsigned(ok),unsigned(world->randomDrawCount()-before),audio.calls,unsigned(audio.valid),
                    unsigned(owner->valid()),unsigned(world->effectsSucceeded()),unsigned(motion.initialized),unsigned(world->scriptEffects().size()));
            assert(ok&&world->randomDrawCount()==before+(target==1?3u:1u)&&audio.valid&&audio.calls==(target==1?7u:0u));
            const auto* retained=world->resolveBulletLifetime(key);
            assert(retained&&retained->sourceOwned&&retained->destroyRequested==(target<3)&&motion.initialized&&!retained->trajectory.initialized);
            assert(world->scriptEffects().size()==(target==1?2u:1u));
            std::vector<std::uint32_t> snapshot{motion.x.bits,motion.y.bits,motion.imageAngle.bits,
                motion.direction.high,motion.direction.low,motion.xsp.high,motion.xsp.low,motion.ysp.high,motion.ysp.low,
                std::uint32_t(world->randomState().stateIndex())};
            for(unsigned i=0;i<16;++i)snapshot.push_back(world->randomState().stateWord(i));
            if(!native)scheduledContacts[crowd!=0][target]=snapshot;
            else assert(scheduledContacts[crowd!=0][target]==snapshot);
            return ok;
        };
        unsigned collisionPasses=0;
        const auto collisionPass=[&]{
            ++collisionPasses;
            if(target==3||target==5){
                const auto key=shot;
                // A real fired lifetime and inherited event lookup, not a
                // freestanding damage helper with synthetic destruction.
                struct Audio {
                    unsigned calls=0;
                    bool isPlaying(int id,bool& playing){assert(++calls==1&&id==29);playing=false;return true;}
                    bool stop(int){assert(false);return false;}
                    bool play(int id,int priority,bool loop,int& voice){assert(++calls==2&&id==29&&priority==90&&!loop);voice=123;return true;}
                    bool getGain(int id,MotionBinary64& gain){assert(++calls==3&&id==123);gain=MotionBinary64::fromInteger(1);return true;}
                    bool setGain(int id,MotionBinary64 gain,int duration){assert(++calls==4&&id==123&&gain.high==0x3ff00000u&&!gain.low&&!duration);return true;}
                    bool setPitch(int id,MotionBinary64 pitch){assert(++calls==5&&id==123&&pitch.high==0x3ff00000u&&!pitch.low);return true;}
                } hitAudio;
                SoundPlayEnvironment hitEnvironment;
                InstanceHandle self,other;
                assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::bullet,key,self)&&
                    GameplayCollisionView::encode(GameplayCollisionView::Kind::skeleton,skeleton,other));
                RunnerCollisionEventContext context;
                const auto rng=world->randomDrawCount();
                auto pairs=std::make_unique<RunnerCollisionPairs<GameplayCollisionView::capacity,16384>>();
                struct NonSolidMotion {
                    bool rollback(InstanceHandle){assert(false);return false;}
                    bool prepareMotion(InstanceHandle){assert(false);return false;}
                    bool advanceMotion(InstanceHandle){assert(false);return false;}
                } nonSolid;
                assert(owner->collectCollisionPairs(*pairs));
                const bool hitOkay=owner->executeCollisionPairs(*pairs,masks,context,nonSolid,
                    []{return RunnerCollisionEventContext::RoomState{true,false};},[&](const auto& event){
                        assert(event.self.slot==self.slot&&event.other.slot==other.slot&&event.code==2224);
                        return owner->dispatchSkeletonBulletCollision(event,frames,hitEnvironment,hitAudio,
                            [](auto y,auto x,auto& result){return portableAtan(y,x,result);});
                    });
                if(!hitOkay)std::fprintf(stderr,"skeleton bullet mode=%u owner=%u world=%u skeleton=%u audio=%u\n",
                    unsigned(mode),unsigned(owner->valid()),unsigned(world->effectsSucceeded()),unsigned(owner->skeletons().valid()),hitAudio.calls);
                assert(hitOkay);
                const auto* victim=owner->skeletons().find(skeleton);
                const auto* caller=world->resolveBulletLifetime(key);
                if(target==5){
                    assert(victim&&victim->state.health==60&&!victim->state.hitPending&&caller&&!caller->destroyRequested&&
                        !hitAudio.calls&&controllerSpeed==60&&world->scriptEffects().size()==1&&world->randomDrawCount()==rng);
                    return true; // Line suppression is not itself an enemy Collision event.
                }
                assert(victim&&victim->state.health==51&&victim->state.hitDamage==9&&victim->state.damageStun&&
                    victim->state.hitPending&&hitAudio.calls==5&&caller&&caller->destroyRequested&&!caller->hitIgnore);
                assert(controllerSpeed==30&&owner->runControllers().find(controller)->alarms.get(0)==1&&
                    owner->runControllers().find(controller)->controllerSlow==30);
                const auto* child=owner->scriptEffects().find(caller->myFx);
                assert(child&&child->effect->spriteId==605&&world->scriptEffects().size()==2&&world->randomDrawCount()==rng);
            }
            return true;
        };
        assert(world->withSourceBulletSteps(sourceStep,[&]{return world->withCollisionPass(collisionPass,[&]{
            world->step(4);return world->effectsSucceeded();
        });}));
        assert(calls==1&&collisionPasses==1);
        if(target==0){const auto* box=world->findBreakable({0,1});assert(box&&box->state.hit&&box->state.wallHp.high==0xc0200000u);}
        if(target==2){const auto* victim=owner->bones().find(bone);assert(victim&&victim->state.hit&&victim->state.health.high==0xc0200000u);}
        assert(owner->prepareDraw()&&!world->resolveBulletLifetime(shot));
    }
    for(bool failSprite:{false,true}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));world->step(4);
        const auto& shot=world->bulletAt(0);const InstanceHandle key{shot.identitySlot,shot.lifetimeId};
        ScriptSpawnEffects<4> effects;
        auto ops=world->bulletEffectOperations(key,effects,owner->scriptEffects(),[&](int id,unsigned& count){
            const auto* caller=world->resolveBulletLifetime(key);
            assert(caller&&owner->scriptEffects().find(caller->myFx));
            PackedSpriteCollision art;if(failSprite||!sprites.sprite(id,art))return false;
            count=art.frameCount;return true;
        });
        assert(EffectSpawnScript::run(ops,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(500),
            101,{0x3fe00000u,0},{},{})==!failSprite);
        const auto* child=owner->scriptEffects().find(world->resolveBulletLifetime(key)->myFx);
        assert(child&&child->effect->spriteId==(failSprite?-1:101));
        if(failSprite){assert(!world->effectsSucceeded());continue;}
        world->step(0);
        assert(world->effectsSucceeded()&&child->effect->imageIndex.bits==0x3f000000u);
        assert(effects.clear(owner->scriptEffects()));
    }
    for(bool high:{false,true})for(bool laser:{false,true})for(unsigned target:{0u,1u,2u})for(bool worldRandom:{false,true}){
        if(worldRandom&&(high||laser))continue; // Actual birth has the ordinary profile and no Laser Sight.
        const bool wall=target==1,boneTarget=target==2;
        // Exercise the recovered Step against real resource-backed selectors
        // and box writes, using a real fired lifetime for setter/destruction
        // callbacks. The supplied profile and isolated position are fixture
        // inputs, not a claim about natural gem-high activation/scheduling.
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));world->step(4);
        assert(world->effectsSucceeded()&&world->bulletCount()==1);
        const auto& shot=world->bulletAt(0);const InstanceHandle shotKey{shot.identitySlot,shot.lifetimeId};
        InstanceHandle shotShared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::bullet,shotKey,shotShared));
        InstanceHandle bone,boneShared;
        if(boneTarget){
            assert(world->createBoneWithSourceId(605500,240,200,bone)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::bone,bone,boneShared));
        }else assert(world->createGeneratedInstance({wall?60:92,240,512},objects,[](const auto&){return true;}));
        MachineGunSourceMotion motion;GameMakerRandom random(2);
        assert(motion.reset(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(boneTarget?200:500),laser));
        assert(world->publishBulletCollisionPosition(shotKey,motion.x,motion.y));
        bool succeeded=true;
        auto frames=[&](int id,unsigned& count){PackedSpriteCollision art;
            const auto* caller=world->resolveBulletLifetime(shotKey);
            const auto* child=caller?owner->scriptEffects().find(caller->myFx):nullptr;
            assert(child&&child->effect->spriteId==-1); // myFx precedes the first child setter.
            if(!sprites.sprite(id,art))return false;count=art.frameCount;return true;};
        struct Ops {
            GameplayCollisionOwner& owner;MachineGunSourceMotion& motion;const SpriteMaskPack& masks;
            GameplayWorld& world;InstanceHandle shotKey,shotShared;
            InstanceHandle selected{};unsigned lines=0,points=0,writes=0,approachWrites=0;
            bool destroyed=false,approaching=false;
            bool publishAngle(RunnerImageValue angle){
                return angle.bits==motion.imageAngle.bits&&world.publishBulletAngle(shotKey,angle)&&
                    owner.properties(shotShared)->sprite.angle.bits==angle.bits;
            }
            bool publishPosition(RunnerImageValue x,RunnerImageValue y){
                if(approaching){
                    assert((approachWrites%2==0?x.bits==motion.x.bits:y.bits==motion.y.bits));
                    ++approachWrites;
                }else{
                    assert(destroyed&&(writes%2==0?y.bits==motion.y.bits:x.bits==motion.x.bits));
                    ++writes;
                }
                if(!world.publishBulletCollisionPosition(shotKey,x,y))return false;
                const auto state=owner.state(shotShared);
                return state&&state->x.bits==x.bits&&state->y.bits==y.bits;
            }
            bool query(int object,GameplayCollisionOwner::Owner::Result& result){
                MotionBinary64 x,y;if(!motion.endpoint(x,y))return false;++lines;
                return owner.projectileNonPreciseLine(shotShared,object,motion.x.promoted(),motion.y.promoted(),x,y,false,result);
            }
            bool line(int object,bool& hit){GameplayCollisionOwner::Owner::Result result;
                if(!query(object,result))return false;hit=result.found;return true;}
            bool selectWall(){GameplayCollisionOwner::Owner::Result result;
                if(!query(59,result)||!result.found)return false;selected=result.handle;return true;}
            bool approachSolid(){approaching=true;unsigned iterations=0;
                const bool ok=motion.approachSolid([&](auto x,auto y,bool& hit){
                    ++points;GameplayCollisionOwner::Owner::Result result;
                    if(!owner.projectileSolidPoint(shotShared,x,y,false,masks,result))return false;
                    hit=result.found;return true;
                },[&](auto x,auto y){return publishPosition(x,y);},iterations);
                approaching=false;return ok;
            }
            bool wallParent(int& parent){return owner.projectileWallParent(selected,parent);}
            bool wallHealth(MotionBinary64& hp){return owner.projectileBoxHealth(selected,hp);}
            bool wallHealthWrite(MotionBinary64 hp){return owner.projectileBoxHealthWrite(selected,hp);}
            bool wallHit(){return owner.projectileBoxHit(selected);}
            bool destroySelf(){destroyed=world.markBulletDestroyed(shotKey);return destroyed;}
            bool effect(int,MotionBinary64,MotionBinary64){return false;}
            bool soundWall(){
                const auto before=world.randomDrawCount();
                SoundPlayEnvironment env;env.gameVolume={0x3fd00000u,0};env.inWater=true;
                BulletWallAudio audio{world.resolveBulletLifetime(shotKey)->sound,world.overlapSound(2),29,true};
                return world.playBulletWallSound(shotKey,env,[&](const char*& material){
                    return owner.projectileWallMaterial(selected,material);
                },audio)&&audio.valid&&audio.calls==7&&world.randomDrawCount()==before+2;
            }
            bool explosion(int){return false;}
            bool placeShootable(int object,bool& hit){return object==90&&
                owner.projectileShootableMeeting(shotShared,motion.x.promoted(),motion.y.promoted(),masks,hit);}
            bool selectShootable(int object){GameplayCollisionOwner::Owner::Result result;
                if(object!=90||!owner.projectileShootableSelection(shotShared,motion.x.promoted(),motion.y.promoted(),masks,result)||!result.found)return false;
                selected=result.handle;return true;}
            bool thingHealth(MotionBinary64& hp){bool effect;return owner.projectileShootableFields(selected,hp,effect);}
            bool thingHealthWrite(MotionBinary64 hp){return owner.projectileShootableHealthWrite(selected,hp);}
            bool thingHit(){return owner.projectileShootableHit(selected);}
            bool thingHitEffect(bool& effect){MotionBinary64 hp;return owner.projectileShootableFields(selected,hp,effect);}
        } ops{*owner,motion,masks,*world,shotKey,shotShared};
        const auto drawsBefore=world->randomDrawCount();auto expectedRandom=world->randomState();
        auto expectedMotion=motion;
        if(worldRandom){
            assert(!shot.gemHigh&&expectedMotion.initialize(expectedRandom));
            motion.zeroSpread=true; // Runtime must use the retained birth override, not this supplied value.
            if(wall){expectedRandom.nextU32();expectedRandom.nextU32();}
            assert(world->stepSourceBullet(shotKey,motion,ops,frames));
            assert(!motion.zeroSpread&&world->randomDrawCount()==drawsBefore+1+(wall?2:0)&&
                motion.direction.high==expectedMotion.direction.high&&motion.direction.low==expectedMotion.direction.low&&
                world->randomState().stateIndex()==expectedRandom.stateIndex());
            for(unsigned i=0;i<16;++i)assert(world->randomState().stateWord(i)==expectedRandom.stateWord(i));
        }else assert(world->withBulletEffects(shotKey,frames,[&](auto& muzzle){
            return MachineGunSourceStep::run(motion,random,high,muzzle,ops,succeeded);
        }));
        assert(succeeded&&ops.destroyed&&ops.lines==(boneTarget?1u:3u)&&ops.writes==2);
        if(!boneTarget)assert(ops.points>1&&ops.approachWrites==2*(ops.points-1));
        const auto myFx=world->resolveBulletLifetime(shotKey)->myFx;
        const auto* child=owner->scriptEffects().find(myFx);
        assert(random.drawCount()==(worldRandom?0u:1u)&&child&&owner->scriptEffects().size()==(wall?2u:1u));
        if(boneTarget){
            const auto* victim=owner->bones().find(bone);const auto hp=MotionBinary64::fromInteger(1-machineGunProfile(high).damage);
            assert(victim&&victim->state.hit&&!victim->destroyRequested&&victim->state.health.high==hp.high&&victim->state.health.low==hp.low);
            const auto before=world->randomDrawCount();
            auto effectFrames=[&](int id,unsigned& count){return sprites.frameCount(id,count);};
            assert(owner->stepBone(boneShared,true,effectFrames)&&!owner->bones().find(bone)->destroyRequested&&world->scriptEffects().size()==1);
            assert(owner->stepBone(boneShared,false,effectFrames)&&owner->bones().find(bone)->destroyRequested&&world->scriptEffects().size()==2&&world->randomDrawCount()==before);
            assert(world->scriptEffects().atNewest(0)->spriteId==85&&owner->prepareDraw()&&!owner->bones().find(bone)&&!world->resolveBulletLifetime(shotKey));
            continue;
        }
        if(wall){
            const auto* impact=child->effect;
            assert(impact&&impact->spriteId==101&&impact->imageSpeed.bits==0x3f000000u&&
                impact->angle.bits==0&&impact->depth.bits==0);
            assert(world->resolveBulletLifetime(shotKey)->destroyRequested&&world->overlapSound(2)==29);
            assert(owner->prepareDraw()&&!world->resolveBulletLifetime(shotKey));
            continue;
        }
        const auto* box=world->findBreakable({0,1});assert(box);
        const auto expected=MotionBinary64::fromInteger(1-machineGunProfile(high).damage);
        assert(box->state.wallHp.high==expected.high&&box->state.wallHp.low==expected.low&&box->state.hit);
        assert(box->active&&!box->destroyRequested); // Its Step, not bullet damage, owns destruction effects.
        GameplayCollisionOwner::Owner::Result selected;
        const auto x=MotionBinary64::fromInteger(240),y=MotionBinary64::fromInteger(512);
        assert(owner->projectileSolidPoint({},x,y,false,masks,selected)&&selected.found&&
            selected.handle.slot==ops.selected.slot&&selected.handle.generation==ops.selected.generation);
        assert(world->resolveBulletLifetime(shotKey)&&world->resolveBulletLifetime(shotKey)->destroyRequested);
        {
            const auto before=world->randomDrawCount();const int previous=world->overlapSound(2);
            SoundPlayEnvironment env;env.gameVolume={0x3fd00000u,0};env.inWater=true;
            BulletWallAudio audio{world->resolveBulletLifetime(shotKey)->sound,previous,29,true};
            assert(world->playBulletWallSound(shotKey,env,[&](const char*& material){
                return owner->projectileWallMaterial(ops.selected,material);
            },audio)&&audio.valid&&audio.calls==7&&world->randomDrawCount()==before+2&&world->overlapSound(2)==29);
        }
        if(!worldRandom)std::printf("SOURCE_SHOT %u %u %08x %08x %08x %08x %08x %u %u %u %u\n",
            unsigned(high),unsigned(laser),unsigned(motion.x.bits),unsigned(motion.y.bits),unsigned(motion.imageAngle.bits),
            unsigned(box->state.wallHp.high),unsigned(box->state.wallHp.low),ops.lines,ops.points,ops.approachWrites,ops.writes);
        assert(owner->prepareDraw()&&!world->resolveBulletLifetime(shotKey)&&!owner->state(shotShared));
        if(worldRandom){const auto before=world->randomDrawCount();
            assert(!world->stepSourceBullet(shotKey,motion,ops,frames)&&!world->effectsSucceeded()&&world->randomDrawCount()==before);}
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        DeferredCreateInstance box{92,240,512};box.setBoxCreate(174,2);
        box.imageSpeed=0;box.imageIndex=22;box.integerImageIndex=22;
        const auto draws=world->randomDrawCount();
        assert(world->createGeneratedInstance(box,objects,[](const auto&){return true;})&&world->randomDrawCount()==draws);
        InstanceStore<1> stored;DeferredInstanceQueue<1> queue;std::uint32_t next=100000;
        assert(queue.enqueueCreate(box)&&queue.applyChecked(stored,objects,11,next,[](auto&){return true;})==ApplyInstanceCommandsResult::success);
        const auto* native=world->findBreakable({0,1});assert(native);
        const auto& a=native->state;const auto& b=stored.at(0).box;
        assert(a.wallHp.high==0x3ff00000u&&!a.wallHp.low&&a.wallHp.high==b.wallHp.high&&
            a.breakSound==174&&a.breakSound==b.breakSound&&a.haveGem==2&&a.haveGem==b.haveGem&&
            !a.hit&&!b.hit&&!a.drilled&&!b.drilled&&!a.autoTile&&!b.autoTile&&!a.checked&&!b.checked);
        InstanceHandle selected;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::breakable,{0,1},selected));
        GameplayCollisionOwner::Owner::Result line;
        const auto xroom=MotionBinary64::fromInteger(240),yroom=MotionBinary64::fromInteger(512);
        assert(owner->projectileNonPreciseLine({},59,xroom,yroom,xroom,yroom,false,line)&&line.found&&
            line.handle.slot==selected.slot&&line.handle.generation==selected.generation);
        selected=line.handle;
        int parent=-123;MotionBinary64 hp;
        assert(owner->projectileWallParent(selected,parent)&&parent==84&&owner->projectileBoxHealth(selected,hp));
        const auto flags=owner->state(selected)->flags;
        const MotionBinary64 fractional{0xbfe00000u,0}; // -0.5, never clamped to zero.
        assert(owner->projectileBoxHealthWrite(selected,fractional)&&owner->projectileBoxHit(selected)&&
            owner->projectileBoxHealth(selected,hp)&&hp.high==fractional.high&&!hp.low&&native->state.hit&&
            native->active&&!native->destroyRequested&&owner->state(selected)->flags==flags&&
            world->randomDrawCount()==draws);
        BoxDestructionScript::Caller caller;
        RunnerImageValue preciseX{0x43704000u},preciseY{0x44000800u};
        assert(owner->breakableBinding().publishedPosition({0,1},*native,preciseX,preciseY)&&
            owner->readBoxDestruction(selected,caller)&&caller.x.high==preciseX.promoted().high&&
            caller.x.low==preciseX.promoted().low&&caller.y.high==preciseY.promoted().high&&
            caller.y.low==preciseY.promoted().low&&caller.state.wallHp.high==fractional.high&&caller.state.hit);
        assert(world->markBreakableDestroyed({0,1})&&owner->projectileBoxHealthWrite(selected,{})&&
            owner->projectileBoxHit(selected)&&owner->readBoxDestruction(selected,caller)&&caller.state.wallHp.zero());
        assert(owner->prepareDraw()&&!owner->readBoxDestruction(selected,caller)&&caller.state.wallHp.zero()&&
            caller.x.high==preciseX.promoted().high&&caller.x.low==preciseX.promoted().low);
        hp={0x3ff00000u,0};parent=-123;
        assert(!owner->projectileBoxHealth(selected,hp)&&hp.high==0x3ff00000u&&
            !owner->projectileWallParent(selected,parent)&&parent==-123&&
            !owner->projectileBoxHealthWrite(selected,fractional)&&!owner->projectileBoxHit(selected));
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,43,300)!=ResultsDestination::none);
        const int x=0,y=336;assert(world->appendBreakables(&x,&y,1));
        const auto* fresh=world->findBreakable({0,2});
        assert(fresh&&fresh->state.breakSound==171&&!fresh->state.haveGem&&!fresh->state.hit&&
            fresh->state.wallHp.high==0x3ff00000u);
        assert(!owner->projectileBoxHealthWrite(selected,fractional)&&!fresh->state.hit);
        InstanceHandle replacement;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::breakable,{0,2},replacement));
        assert(!owner->projectileBoxHealthWrite(replacement,{0x7ff00000u,0})&&
            !owner->projectileBoxHit(replacement)&&!fresh->state.hit&&fresh->state.wallHp.high==0x3ff00000u);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*scene));
        assert(scene->createGeneratedInstance({60,240,512},objects,[](const auto&){return true;}));
        assert(scene->createGeneratedInstance({92,240,512},objects,[](const auto&){return true;}));
        assert(scene->configureEnemy(0,112,163,0,336,0,0,5,14,0,0,-1,-1,0,0,true,true,true));
        const auto enemy=scene->enemyHandle(scene->enemyAt(0).lifetimeId);
        InstanceHandle enemyShared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::enemy,enemy,enemyShared));
        auto n=[](int v){return MotionBinary64::fromInteger(v);};
        auto query=[&](int object,InstanceHandle self={},bool notMe=false){
            GameplayCollisionOwner::Owner::Result selected;
            assert(owner->projectileNonPreciseLine(self,object,n(240),n(496),n(240),n(528),notMe,selected));
            return selected;
        };
        const auto wall=query(59),actor=query(83);
        GameplayCollisionOwner::Owner::Result point;
        assert(owner->projectileSolidPoint({},n(240),n(512),false,masks,point)&&point.found);
        const auto firstPoint=point.handle;
        assert(owner->projectileSolidPoint(firstPoint,n(240),n(512),true,masks,point)&&
            point.found&&point.handle.slot!=firstPoint.slot);
        assert(wall.found&&actor.found&&actor.handle.slot==enemyShared.slot&&actor.handle.generation==enemyShared.generation);
        const auto excluded=query(83,enemyShared,true);assert(!excluded.found);
        assert(query(83,enemyShared,false).found); // VM uses notme=false.
        const auto* liveEnemy=scene->resolveEnemyLifetime(enemy);assert(liveEnemy);
        assert(owner->enemyBinding().activation(enemy,*liveEnemy,false)&&!query(83).found);
        assert(owner->reconcileActivation()&&!query(83).found);
        assert(owner->enemyBinding().activation(enemy,*liveEnemy,true)&&owner->reconcileActivation()&&query(83).found);
        const auto otherSolid=query(59,wall.handle,true);
        assert(otherSolid.found&&otherSolid.handle.slot!=wall.handle.slot);
        GameplayCollisionView::Kind firstKind,otherKind;InstanceHandle firstLocal,otherLocal;
        assert(GameplayCollisionView::decode(wall.handle,firstKind,firstLocal)&&
            GameplayCollisionView::decode(otherSolid.handle,otherKind,otherLocal)&&
            ((firstKind==GameplayCollisionView::Kind::terrain&&otherKind==GameplayCollisionView::Kind::breakable)||
             (otherKind==GameplayCollisionView::Kind::terrain&&firstKind==GameplayCollisionView::Kind::breakable)));
        // The enemy guard must not accidentally return the overlapping solid.
        assert(scene->markEnemyDestroyed(enemy)&&!query(83).found&&query(59).found);
        assert(owner->prepareDraw()&&!scene->resolveEnemyLifetime(enemy));
        assert(scene->configureEnemy(0,112,163,0,336,0,0,5,14));
        const auto replacement=query(83);
        assert(replacement.found&&replacement.handle.generation!=enemyShared.generation);
        assert(scene->restartFromResults(DeathMenuSequence::Action::Retry,43,300)!=ResultsDestination::none);
        assert(!query(59).found&&!query(83).found&&owner->valid());
        GameplayCollisionOwner::Owner::Result sentinel;sentinel.found=true;sentinel.handle=enemyShared;
        assert(!owner->projectileNonPreciseLine({},90,n(240),n(496),n(240),n(528),false,sentinel)&&
            !owner->valid()&&sentinel.found&&sentinel.handle.generation==enemyShared.generation);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        for(int object:{112,92,60,112})
            assert(world->createGeneratedInstance({object,240,512},objects,[](const auto&){return true;}));
        const auto first=world->enemyHandle(world->enemyAt(0).lifetimeId);
        const auto survivor=world->enemyHandle(world->enemyAt(1).lifetimeId);
        assert(!world->resolveEnemyLifetime({GameplayWorld::enemyCapacity,1})&&
            !world->resolveEnemyLifetime({first.slot,0})&&
            !world->resolveEnemyLifetime({first.slot,first.generation+100}));
        const InstanceHandle box{0,1};assert(world->findBreakable(box));
        const auto wall=owner->terrain().handle(0);
        assert(world->markEnemyDestroyed(first)&&world->markEnemyDestroyed(first));
        assert(world->markBreakableDestroyed(box)&&world->markBreakableDestroyed(box));
        assert(owner->terrain().requestDestroy(wall));
        assert(world->resolveEnemyLifetime(first)->active&&world->resolveEnemyLifetime(first)->destroyRequested);
        assert(world->findBreakable(box)->active&&world->findBreakable(box)->destroyRequested);
        unsigned count=99;
        assert(owner->population(112,count)&&count==1);
        assert(owner->population(92,count)&&count==0);
        assert(owner->reconcileActivation()&&world->resolveEnemyLifetime(first)&&world->findBreakable(box));
        RunnerStepSnapshot<16> snapshot;unsigned enemies=0,boxes=0;
        assert(owner->dispatchStepPhase(2,snapshot,[&](InstanceHandle,const PackedObjectEvent& event){
            enemies+=event.codeId==1200;boxes+=event.codeId==1087;return true;
        }));
        assert(enemies==1&&boxes==0);
        assert(owner->prepareDraw()&&owner->prepareDraw());
        assert(!world->resolveEnemyLifetime(first)&&!world->findBreakable(box)&&!owner->terrain().find(wall));
        assert(world->enemyCount()==1&&world->resolveEnemyLifetime(survivor)==&world->enemyAt(0));
        assert(world->createGeneratedInstance({112,272,512},objects,[](const auto&){return true;}));
        const auto replacement=world->enemyHandle(world->enemyAt(1).lifetimeId);
        assert(replacement.slot==first.slot&&replacement.generation!=first.generation);
        assert(!world->resolveEnemyLifetime(replacement)->destroyRequested);
        assert(world->markEnemyDestroyed(replacement)&&owner->prepareDraw());
        assert(world->resolveEnemyLifetime(survivor)&&!world->resolveEnemyLifetime(replacement));
        assert(!world->markEnemyDestroyed(first)&&!world->effectsSucceeded());
    }
    {
        auto world=std::make_unique<GameplayWorld>(300,true,0,false);
        assert(world->createGeneratedInstance({112,240,512},objects,[](const auto&){return true;}));
        const auto h=world->enemyHandle(world->enemyAt(0).lifetimeId);
        assert(!world->markEnemyDestroyed(h)&&!world->effectsSucceeded());
        assert(world->resolveEnemyLifetime(h)&&!world->resolveEnemyLifetime(h)->destroyRequested);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        // Deliberately interleave registration/storage order across families.
        for(int object:{99,60,112,93,60})
            assert(world->createGeneratedInstance({object,240,512},objects,[](const auto&){return true;}));
        const auto first=owner->terrain().handle(1),second=owner->terrain().handle(3);
        assert(owner->terrain().activation(first,false)&&owner->reconcileActivation());
        assert(owner->terrain().activation(first,true)&&owner->reconcileActivation());
        RunnerStepSnapshot<16> snapshot;
        std::vector<int> codes;std::vector<InstanceHandle> walls;
        assert(owner->dispatchStepPhase(2,snapshot,[&](InstanceHandle h,const PackedObjectEvent& event){
            assert(event.eventType==3&&event.eventSubtype==2);
            codes.push_back(event.codeId);
            if(event.codeId==986){
                GameplayCollisionView::Kind kind;InstanceHandle local;
                assert(GameplayCollisionView::decode(h,kind,local)&&kind==GameplayCollisionView::Kind::terrain);
                walls.push_back(local);
            }
            return true;
        }));
        assert((codes==std::vector<int>{848,986,986,1091,1112,1200}));
        assert(walls.size()==2&&walls[0].slot==second.slot&&walls[1].slot==first.slot);
        // Object 99's empty own End Step (1112) must NOT inherit wall 986.
        codes.clear();bool changed=false;InstanceHandle newborn;
        assert(owner->dispatchStepPhase(2,snapshot,[&](InstanceHandle,const PackedObjectEvent& event){
            codes.push_back(event.codeId);
            if(event.codeId==986&&!changed){
                changed=true;
                assert(owner->terrain().requestDestroy(first));
                assert(owner->prepareDraw()); // Exercise physical slot reuse too.
                assert(world->createGeneratedInstance({60,272,512},objects,[](const auto&){return true;}));
                newborn=owner->terrain().handle(owner->terrain().size()-1);
                assert(newborn.slot==first.slot&&newborn.generation!=first.generation);
            }
            return true;
        }));
        assert(changed&&(codes==std::vector<int>{848,986,1091,1112,1200}));
        codes.clear();
        assert(owner->dispatchStepPhase(2,snapshot,[&](InstanceHandle,const PackedObjectEvent& event){
            codes.push_back(event.codeId);return true;
        }));
        assert((codes==std::vector<int>{848,986,986,1091,1112,1200}));
        unsigned calls=0;
        assert(!owner->dispatchStepPhase(2,snapshot,[&](InstanceHandle,const PackedObjectEvent&){return ++calls<2;}));
        assert(calls==2&&!owner->valid());
        assert(!owner->dispatchStepPhase(2,snapshot,[&](InstanceHandle,const PackedObjectEvent&){++calls;return true;})&&calls==2);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        assert(world->createGeneratedInstance({60,240,512},objects,[](const auto&){return true;}));
        RunnerStepSnapshot<1> tooSmall;unsigned calls=0;
        assert(!owner->dispatchStepPhase(2,tooSmall,[&](InstanceHandle,const PackedObjectEvent&){++calls;return true;}));
        assert(calls==0&&!owner->valid());
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery}){
        const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
        const auto rb=read("generated/downwell-rooms.dwrm");RoomPack rooms(rb.data(),rb.size());
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),
            owner->playerBinding(),native?nullptr:&owner->terrain(),&rooms);
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();GameMakerRandom random(123);
        assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain)));
        InstanceHandle local;
        if(native){assert(terrain->append(99,240,160));local=terrain->handle(0);assert(terrain->createProperties(local,51));}
        else {assert(owner->terrain().append(99,240,160,true));local=owner->terrain().handle(0);
            assert(owner->terrain().createProperties(local,51,false,random));}
        struct NoEffects {
            bool tile(MotionBinary64,MotionBinary64){assert(false);return false;}
            bool create(int,MotionBinary64,MotionBinary64){assert(false);return false;}
            bool outOfView(InstanceHandle){assert(false);return false;}
        } effects;
        const auto draws=random.drawCount();RunnerStepSnapshot<8> phase;unsigned calls=0;
        assert(owner->dispatchStepPhase(2,phase,[&](InstanceHandle,const PackedObjectEvent& event){
            if(event.codeId==848)return true;
            assert(event.codeId==1112);++calls;
            return owner->wallEndStepEvent(local,4,random,masks,effects);
        }));
        assert(calls==1&&owner->wallEndStepEvent(local,5,random,masks,effects)&&random.drawCount()==draws);
        assert(native?world->activateNativeTerrain(local,false):owner->terrain().activation(local,false));
        assert(owner->wallEndStepEvent(local,5,random,masks,effects)&&random.drawCount()==draws);
        assert(native?world->destroyNativeTerrain(local):owner->terrain().requestDestroy(local));
        assert(owner->wallEndStepEvent(local,5,random,masks,effects)&&owner->prepareDraw());
        assert((native?terrain->size():owner->terrain().size())==0);
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
        GameplayCollisionOwner::Ownership::nativeScenery}){
        const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
        const auto rb=read("generated/downwell-rooms.dwrm");RoomPack rooms(rb.data(),rb.size());
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),
            owner->playerBinding(),native?nullptr:&owner->terrain(),&rooms);
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain)));
        InstanceHandle local,shared;
        assert(world->createLimboShardWithSourceId(105190,MotionBinary64::fromInteger(240),
            MotionBinary64::fromInteger(10000),false,local));
        GameplayCollisionView::Record resolved;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::limboShard,local,shared)&&GameplayCollisionView(*world).resolve(shared,resolved));
        checkTypedCollisionView(GameplayCollisionView(*world),GameplayCollisionView::Kind::limboShard,local);
        unsigned seen=0;
        assert(world->visitRoomActorsReadOnly([&](auto kind,auto key,auto){
            if(kind==GameplayWorld::DrawActor::limboShard){assert(key.slot==local.slot&&key.generation==local.generation);++seen;}
            return true;
        })&&seen==1);
        struct Views {
            int top=9000;
            bool outside(RunnerImageValue,RunnerImageValue,bool&){assert(false);return false;}
            bool viewTop(MotionBinary64& y){y=MotionBinary64::fromInteger(top);return true;}
        } views;
        assert(owner->limboShards().advanceAnimation(local));
        assert(owner->stepLimboShard(local,true,480,{},masks,views));
        const auto* r=owner->limboShards().find(local);
        assert(r&&r->state.imageSpeed.bits==0&&!r->destroyRequested);
        assert(owner->limboShards().activate(local,false)&&owner->stepLimboShard(local,false,480,{},masks,views));
        assert(owner->limboShards().activate(local,true));views.top=11000;
        assert(owner->stepLimboShard(local,false,480,{},masks,views)&&owner->limboShards().find(local)->destroyRequested);
        assert(owner->prepareDraw()&&!owner->limboShards().find(local)&&!GameplayCollisionView(*world).resolve(shared,resolved));
        assert(world->createLimboShardWithSourceId(105191,{}, {},false,local)&&local.generation!=shared.generation);
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,2,0)==ResultsDestination::gameplay&&
            owner->limboShards().size()==0&&owner->valid());
    }
    for(unsigned reject=0;reject<5;++reject){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));world->step(4);
        assert(world->effectsSucceeded()&&world->bulletCount()==1);
        const auto& bullet=world->bulletAt(0);InstanceHandle shot,shard,local;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::bullet,{bullet.identitySlot,bullet.lifetimeId},shot));
        MachineGunSourceMotion motion;GameMakerRandom spread(2);
        // Explicit checked source state for this real bullet lifetime, not a
        // reconstruction from its old trajectory or a natural Step fixture.
        const auto* position=owner->state(shot);assert(position);
        assert(motion.reset(position->x.promoted(),position->y.promoted())&&motion.initialize(spread));
        assert(world->createLimboShardWithSourceId(105194,motion.x.promoted(),motion.y.promoted(),false,local)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::limboShard,local,shard));
        unsigned reads=0;
        auto velocity=[&](InstanceHandle selected,MotionBinary64& x,MotionBinary64& y){
            ++reads;assert(selected.slot==shot.slot&&selected.generation==shot.generation);
            if(reject==1||!motion.initialized)return false;
            x=motion.xsp;y=reject==2?MotionBinary64{0x7ff00000u,0}:motion.ysp;return true;
        };
        const auto draws=world->randomDrawCount();auto expectedRandom=world->randomState();MotionBinary64 expectedX;
        assert(randomBinary64(expectedRandom.nextU32(),MotionBinary64::fromInteger(2),expectedX));
        if(motion.xsp.zero())expectedX={};else expectedX.high|=motion.xsp.high&0x80000000u;
        RunnerCollisionEventContext context;
        auto dispatch=[&]{return owner->dispatchCollisionEvent(context,shard,shot,{true,false},[&](const auto& event){
            assert(event.owner==379&&event.code==2771&&event.subtype==261);
            auto selected=event;
            if(reject==3)selected.owner=89;
            if(reject==4)++selected.other.generation;
            return owner->dispatchLimboShardBulletCollision(selected,velocity);
        });};
        if(reject){
            assert(!dispatch()&&!owner->valid()&&!context.current()&&world->randomDrawCount()==draws&&
                !owner->limboShards().find(local)->state.noTouch&&reads==(reject<3?1u:0u));
            continue;
        }
        assert(owner->limboShards().activate(local,false)&&dispatch()&&reads==0&&world->randomDrawCount()==draws);
        assert(owner->limboShards().activate(local,true)&&dispatch()&&reads==1&&world->randomDrawCount()==draws+1);
        const auto* record=owner->limboShards().find(local);assert(record);
        const auto& state=record->state;
        assert(state.xsp.high==expectedX.high&&state.xsp.low==expectedX.low&&
            state.ysp.high==motion.ysp.high&&state.ysp.low==motion.ysp.low&&state.noTouch&&state.alarm2==5&&record->alarms.get(2)==5);
        motion.initialized=false;
        assert(dispatch()&&reads==1&&world->randomDrawCount()==draws+1&&!context.current());
        assert(owner->limboShards().clear());
    }
    // Real shape/point queries through the retained wall script, not mocked
    // neighbour occupancy. Compare callback order and every retained field.
    for(int area:{0,1,4,5})for(bool rejectTile:{false,true}){
        const auto roomBytes=read("generated/downwell-rooms.dwrm");RoomPack rooms(roomBytes.data(),roomBytes.size());
        std::vector<int> expected;
        for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
            GameplayCollisionOwner::Ownership::nativeScenery}){
            const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
                GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms,mode);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),
                owner->playerBinding(),native?nullptr:&owner->terrain(),&rooms);
            auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
            assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain)));
            std::vector<int> actual;
            struct Effects {
                GameplayWorld& world;std::vector<int>& values;bool reject;
                GameplayCollisionOwner& owner;GameplayWorld::NativeTerrain& terrain;
                const SpriteMaskPack& masks;bool native;InstanceHandle current{};
                unsigned creates=0;
                bool position(int kind,MotionBinary64 x,MotionBinary64 y){
                    std::int32_t px,py;assert(x.roundPixel(px)&&y.roundPixel(py));
                    values.insert(values.end(),{kind,px,py});return true;
                }
                bool tile(MotionBinary64 x,MotionBinary64 y){return position(739,x,y)&&!reject&&world.addWallBackground(x,y);}
                bool create(int object,MotionBinary64 x,MotionBinary64 y){
                    assert(object==379);
                    ++creates;
                    // instance_create runs synchronously after image_index is
                    // assigned, while native wall state is still borrowed.
                    // Exercise actual collision consumers here, not just the
                    // packed state visible after End Step returns.
                    WallEndStepState state;
                    if(native)assert(terrain.wallState(current,state));
                    else state=owner.terrain().find(current)->wall;
                    assert(state.imageIndex==state.neighbors);
                    InstanceHandle shared;
                    assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,current,shared));
                    RunnerImageValue image;assert(image.assignInteger(state.imageIndex));
                    if(native){
                        int sprite=-1;unsigned index=999;bool created=false;
                        assert(terrain.appearance(current,sprite,index,created)&&created&&
                            sprite==51&&index==unsigned(state.imageIndex));
                    }else{
                        const auto appearance=owner.properties(shared);
                        assert(appearance&&appearance->sprite.imageIndex.bits==image.bits);
                    }
                    std::int32_t cx,cy;assert(x.roundPixel(cx)&&y.roundPixel(cy));
                    for(int dy=-9;dy<=9;dy+=3)for(int dx=-9;dx<=9;dx+=3){
                        RunnerImageValue px,py;
                        assert(px.assignInteger(cx+dx)&&py.assignInteger(cy+16+dy));
                        bool hit=false;
                        assert(owner.collisionPointCandidate(shared,px,py,true,true,masks,hit));
                        values.push_back(int(hit));
                    }
                    return position(object,x,y);
                }
                bool outOfView(InstanceHandle h){values.insert(values.end(),{5,int(h.slot)});return true;}
            } effects{*world,actual,rejectTile,*owner,*terrain,masks,native};
            GameMakerRandom random(123);
            for(int y=0;y<3;++y)for(int x=0;x<3;++x){
                const unsigned i=unsigned(y*3+x);
                if(native){assert(terrain->append(60,240+x*16,512+y*16));assert(terrain->createProperties(terrain->handle(i),51));}
                else {assert(owner->terrain().append(60,240+x*16,512+y*16,true));
                    assert(owner->terrain().createProperties(owner->terrain().handle(i),51,false,random));}
            }
            for(unsigned pass=0;pass<2;++pass)for(unsigned i=0;i<9;++i){
                const auto h=native?terrain->handle(i):owner->terrain().handle(i);
                effects.current=h;
                const bool ok=owner->wallEndStepEvent(h,area,random,masks,effects);
                WallEndStepState s;
                if(native)assert(terrain->wallState(h,s));else s=owner->terrain().find(h)->wall;
                actual.insert(actual.end(),{int(ok),s.checked,s.pillar,s.neighbors,s.imageIndex,s.checkThing,
                    s.checkDistance,int(s.surrounded),int(s.deactivate),int(s.lights),int(random.drawCount())});
                if(!ok){assert(rejectTile&&pass==1&&i==4);break;}
                // Consume the event's newly published appearance immediately,
                // before any animation pass or Draw preparation. Native wall
                // state is borrowed during the event and packed afterwards;
                // queries must not revert to constructor/previous images.
                InstanceHandle shared;
                assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,h,shared));
                if(native){
                    int sprite=-1;unsigned image=999;bool created=false;
                    assert(terrain->appearance(h,sprite,image,created)&&created&&
                        sprite==51&&image==unsigned(s.imageIndex));
                }else{
                    const auto appearance=owner->properties(shared);RunnerImageValue image;
                    assert(image.assignInteger(s.imageIndex)&&appearance&&
                        appearance->sprite.spriteId==51&&appearance->sprite.imageIndex.bits==image.bits);
                }
                for(int dy=-9;dy<=9;dy+=3)for(int dx=-9;dx<=9;dx+=3){
                    RunnerImageValue px,py;
                    assert(px.assignInteger(240+int(i%3)*16+dx)&&
                        py.assignInteger(512+int(i/3)*16+dy));
                    for(bool precise:{false,true}){
                        bool hit=false;
                        assert(owner->collisionPointCandidate(shared,px,py,true,precise,masks,hit));
                        actual.push_back(int(hit));
                    }
                }
            }
            assert((area==4)==(effects.creates!=0));
            assert(world->wallBackgroundTiles().size()==(rejectTile?0u:1u));
            if(!rejectTile){
                std::int32_t x,y;const auto* tile=world->wallBackgroundTiles().at(0);
                assert(tile->x.roundPixel(x)&&tile->y.roundPixel(y)&&x==248&&y==520);
                // The tile outlives its source wall. Draw-entry cleanup must
                // consume the native destruction mark, not erase the patch.
                assert(owner->prepareDraw());
                assert((native?terrain->size():owner->terrain().size())==8);
                assert(world->wallBackgroundTiles().size()==1);
            }
            if(!native)expected=actual;else assert(actual==expected);
        }
    }
    for(bool rejectTile:{false,true}){
        struct Damage:AmigaWallDamage<64> {std::vector<std::pair<int,int>> cells;
            bool changed(int x,int y,int before,int after){assert(before==51&&(after==51||after==-1));cells.push_back({x,y});return AmigaWallDamage<64>::changed(x,y,before,after);}
        } damage;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation::bind(damage));
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*scene));
        for(int y=0;y<3;++y)for(int x=0;x<3;++x)
            assert(scene->createGeneratedInstance({60,240+x*16,512+y*16},objects,[](const auto&){return true;}));
        struct Effects {
            GameplayWorld& world;bool reject;unsigned tiles=0;
            bool tile(MotionBinary64 x,MotionBinary64 y){++tiles;return !reject&&world.addWallBackground(x,y);}
            bool outOfView(InstanceHandle){assert(false);return false;}
            bool create(int,MotionBinary64,MotionBinary64){assert(false);return false;}
        } effects{*scene,rejectTile};
        GameMakerRandom random(123);const auto draws=random.drawCount();
        RunnerStepSnapshot<16> phase;unsigned wallEvents=0;
        assert(owner->dispatchStepPhase(2,phase,[&](InstanceHandle shared,const PackedObjectEvent& event){
            if(event.codeId!=986){assert(event.codeId==848);return true;}
            GameplayCollisionView::Kind kind;InstanceHandle local;
            assert(GameplayCollisionView::decode(shared,kind,local)&&kind==GameplayCollisionView::Kind::terrain);
            ++wallEvents;return owner->wallEndStepEvent(local,1,random,masks,effects);
        }));
        assert(wallEvents==9);
        assert(damage.cells.size()==9&&damage.size()==1);damage.cells.clear();damage.clear();
        const auto middle=owner->terrain().handle(4);const auto record=owner->terrain().find(middle);
        assert(record->wall.surrounded&&record->wall.checked==1&&record->wall.lights==0&&record->imageIndex==15&&
            !record->destroyRequested&&effects.tiles==0&&scene->wallBackgroundTiles().size()==0);
        unsigned drawn=0;
        assert(drawWallPresentation(*record,[]{assert(false);return 0u;},[&](int sprite,unsigned frame){
            assert(sprite==record->spriteId&&frame==15);++drawn;
        })&&drawn==1);
        assert(owner->wallEndStepEvent(middle,1,random,masks,effects)==!rejectTile&&effects.tiles==1);
        assert(damage.cells.size()==(rejectTile?0u:1u));
        if(!rejectTile){
            assert(damage.cells[0].first==256&&damage.cells[0].second==528&&damage.size()==1);
            assert(damage.visit([](int first,int end){return first==520&&end==536;}));
        }
        assert(random.drawCount()==draws&&owner->terrain().find(middle)->destroyRequested==!rejectTile&&owner->terrain().find(middle)==record);
        if(!rejectTile)assert(drawWallPresentation(*owner->terrain().find(middle),[]{assert(false);return 0u;},
            [](int,unsigned){assert(false);}));
        if(rejectTile){
            assert(!owner->valid()&&scene->wallBackgroundTiles().size()==0);
            assert(!owner->wallEndStepEvent(middle,1,random,masks,effects)&&effects.tiles==1);
        }else{
            assert(scene->wallBackgroundTiles().size()==1);
            std::int32_t x,y;const auto* tile=scene->wallBackgroundTiles().at(0);
            assert(tile->x.roundPixel(x)&&tile->y.roundPixel(y)&&x==248&&y==520);
            assert(owner->wallEndStepEvent(middle,1,random,masks,effects)&&effects.tiles==1);
            assert(owner->terrain().activation(middle,false)&&damage.cells.size()==1);
            assert(owner->cleanupDestroyed([&](InstanceHandle shared,const auto&){
                GameplayCollisionView::Kind kind;InstanceHandle local;
                return GameplayCollisionView::decode(shared,kind,local)&&kind==GameplayCollisionView::Kind::terrain&&
                    owner->terrain().retireMarked(local);
            }));
            assert(!owner->terrain().find(middle)&&owner->terrain().size()==8&&scene->wallBackgroundTiles().size()==1);
            assert(damage.cells.size()==1); // Hiding, deactivating, cleanup do not repaint the same wall three times.
        }
    }
    {
        auto queries=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,queries->enemyBinding(),
            queries->breakableBinding(),queries->playerBinding(),&queries->terrain());
        assert(queries->attach(*scene));
        for(auto p:{DeferredCreateInstance{60,240,512},DeferredCreateInstance{60,256,512},DeferredCreateInstance{60,240,528}})
            assert(scene->createGeneratedInstance(p,objects,[](const auto&){return true;}));
        const auto self=queries->terrain().handle(0),right=queries->terrain().handle(1);
        InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,self,shared));
        const auto before=*queries->state(shared);
        for(int y=492;y<=548;++y)for(int x=220;x<=276;++x){
            bool expected=false,boxHit=false;
            for(unsigned i=0;i<3;++i){
                const auto local=queries->terrain().handle(i);const auto r=queries->terrain().find(local);
                InstanceHandle key;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,key));
                const auto appearance=queries->properties(key);
                const auto& p=appearance->sprite;PackedSpriteCollision resource;
                assert(sprites.collisionSprite(p.spriteId,p.maskId,resource));
                const int px=x-r->x+resource.originX,py=y-r->y+resource.originY;
                if(px<resource.marginLeft||px>resource.marginRight||py<resource.marginTop||py>resource.marginBottom)continue;
                boxHit=true;
                if(!resource.collisionMode||!resource.collisionMaskCount){expected=true;break;}
                SpriteMaskPack::Mask mask;assert(masks.mask(unsigned(p.maskId<0?p.spriteId:p.maskId),0,mask));
                if(mask.contains(px,py)){expected=true;break;}
            }
            bool hit=!expected;
            const bool ok=queries->terrainPositionMeeting(60,MotionBinary64::fromInteger(x),MotionBinary64::fromInteger(y),masks,hit);
            if(!ok||hit!=expected)std::fprintf(stderr,"native point %d,%d: ok=%d hit=%d expected=%d\n",x,y,ok,hit,expected);
            assert(ok&&hit==expected);
            const auto px=MotionBinary64::fromInteger(x),py=MotionBinary64::fromInteger(y);
            GameplayCollisionOwner::Owner::Result selected;
            assert(queries->terrainCollisionPoint({},60,px,py,true,false,masks,selected)&&selected.found==expected);
            assert(queries->terrainCollisionPoint({},60,px,py,false,false,masks,selected)&&selected.found==boxHit);
            assert(queries->terrainNonPreciseLine({},60,px,py,px,py,false,selected)&&selected.found==boxHit);
            if(selected.found){
                GameplayCollisionView::Kind kind;InstanceHandle local;
                assert(GameplayCollisionView::decode(selected.handle,kind,local)&&
                    kind==GameplayCollisionView::Kind::terrain&&queries->terrain().find(local));
                const auto excluded=selected.handle;
                assert(queries->terrainNonPreciseLine(excluded,60,px,py,px,py,true,selected));
                assert(!selected.found||selected.handle.slot!=excluded.slot||selected.handle.generation!=excluded.generation);
            }
        }
        for(auto p:{DeferredCreateInstance{0,256,512},DeferredCreateInstance{0,240,528},DeferredCreateInstance{1,240,512},DeferredCreateInstance{1,224,512}}){
            bool hit=false;
            assert(queries->terrainPlaceMeeting(self,60,MotionBinary64::fromInteger(p.x),MotionBinary64::fromInteger(p.y),masks,hit)&&hit==(p.objectId==0));
            const auto restored=queries->state(shared);
            assert(restored&&restored->x.bits==before.x.bits&&restored->y.bits==before.y.bits);
            assert(restored->previous.x.bits==before.previous.x.bits&&restored->previous.y.bits==before.previous.y.bits);
        }
        const auto after=*queries->state(shared);
        assert(before.x.bits==after.x.bits&&before.y.bits==after.y.bits&&scene->effectsSucceeded());
        bool hit=true;assert(queries->terrain().activation(right,false));
        assert(queries->terrainPositionMeeting(60,MotionBinary64::fromInteger(256),MotionBinary64::fromInteger(512),masks,hit)&&!hit);
        assert(queries->reconcileActivation()&&queries->terrain().activation(right,true)&&queries->reconcileActivation());
        assert(queries->terrainPositionMeeting(60,MotionBinary64::fromInteger(256),MotionBinary64::fromInteger(512),masks,hit)&&hit);
        assert(queries->terrain().requestDestroy(right));
        assert(queries->terrainPositionMeeting(60,MotionBinary64::fromInteger(256),MotionBinary64::fromInteger(512),masks,hit)&&!hit);
        assert(scene->createGeneratedInstance({99,400,800},objects,[](const auto&){return true;}));
        const auto marker=queries->terrain().handle(3);
        assert(queries->terrain().find(marker)->spriteId==46&&queries->terrain().find(marker)->imageSpeed==1);
        assert(queries->terrainPositionMeeting(60,MotionBinary64::fromInteger(400),MotionBinary64::fromInteger(800),masks,hit)&&hit);
        assert(scene->createGeneratedInstance({93,432,800},objects,[](const auto&){return true;}));
        // All supported terrain identities can be the temporary query caller.
        // A fractional query must neither move its authoritative record nor
        // overwrite the pre-animation snapshot, on hits or misses. Repeated
        // queries also exercise flushing the preceding restoration's queues.
        for(auto caller:{self,marker,queries->terrain().handle(4)}){
            InstanceHandle callerKey;
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,caller,callerKey));
            const auto original=*queries->state(callerKey);
            const auto record=*queries->terrain().find(caller);
            for(int repeat=0;repeat<3;++repeat)for(int targetX:{240,256,400,432,1000}){
                auto x=MotionBinary64::fromInteger(targetX);
                auto y=MotionBinary64::fromInteger(targetX<400?512:800);
                assert(x.add({0x3fd00000u,0})&&y.add({0xbfd00000u,0})); // +1/4, -1/4
                bool found=false;
                assert(queries->terrainPlaceMeeting(caller,60,x,y,masks,found));
                const auto restored=queries->state(callerKey);
                const auto terrain=queries->terrain().find(caller);
                assert(restored&&terrain&&restored->x.bits==original.x.bits&&restored->y.bits==original.y.bits);
                assert(restored->previous.x.bits==original.previous.x.bits&&restored->previous.y.bits==original.previous.y.bits);
                assert(terrain->x==record.x&&terrain->y==record.y&&terrain->lifetimeId==record.lifetimeId);
            }
        }
        hit=true;assert(!queries->terrainPositionMeeting(82,{},{},masks,hit)&&hit&&!queries->valid());
    }
    {
        auto phaseOwner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto phaseWorld=std::make_unique<GameplayWorld>(300,true,0,false,phaseOwner->enemyBinding(),
            phaseOwner->breakableBinding(),phaseOwner->playerBinding(),&phaseOwner->terrain());
        assert(phaseOwner->attach(*phaseWorld));
        assert(phaseWorld->createGeneratedInstance({60,240,512},objects,[](const auto&){return true;}));
        auto local=phaseOwner->terrain().handle(0);InstanceHandle shared;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,shared));
        assert(phaseOwner->terrain().activation(local,false));
        assert(!(phaseOwner->state(shared)->flags&0x80000u)); // Deferred until phase boundary.
        phaseWorld->step(0);assert(phaseWorld->effectsSucceeded());
        assert(phaseOwner->state(shared)->flags&0x80000u);
        assert(phaseOwner->terrain().activation(local,true));
        assert(phaseOwner->state(shared)->flags&0x80000u);
        phaseWorld->step(0);assert(phaseWorld->effectsSucceeded());
        assert(!(phaseOwner->state(shared)->flags&0x80000u));
        phaseWorld->configureGenerationStep([](void* p,GameplayWorld&,GameMakerRandom&,int){
            auto& terrain=static_cast<GameplayCollisionOwner*>(p)->terrain();
            return terrain.activation(terrain.handle(0),false);
        },phaseOwner.get());
        phaseWorld->step(0);assert(phaseWorld->effectsSucceeded());
        assert(phaseOwner->state(shared)->flags&0x80000u); // Step request is not delayed to next frame.
        assert(phaseWorld->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        assert(phaseWorld->createGeneratedInstance({60,240,512},objects,[](const auto&){return true;}));
        local=phaseOwner->terrain().handle(0);
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,shared));
        assert(phaseOwner->terrain().activation(local,false));
        phaseWorld->step(0);assert(phaseWorld->effectsSucceeded());
        assert(phaseOwner->state(shared)->flags&0x80000u); // Callback survives Retry.
    }
    {
        auto phaseWorld=std::make_unique<GameplayWorld>(300);
        struct Audit{unsigned boundaries=0,generations=0;} audit;
        phaseWorld->configurePhaseBoundary([](void* p,GameplayWorld::PhaseBoundary phase){
            assert(phase==GameplayWorld::PhaseBoundary::beginStep);
            ++static_cast<Audit*>(p)->boundaries;return false;
        },&audit);
        phaseWorld->configureGenerationStep([](void* p,GameplayWorld&,GameMakerRandom&,int){
            ++static_cast<Audit*>(p)->generations;return true;
        },&audit);
        phaseWorld->step(0);
        assert(!phaseWorld->effectsSucceeded()&&audit.boundaries==1&&!audit.generations);
        phaseWorld->step(0);assert(audit.boundaries==1&&!audit.generations);
    }
    for(unsigned reject=0;reject<=5;++reject){
        auto phaseWorld=std::make_unique<GameplayWorld>(300);
        struct Audit{unsigned reject,calls=0,generations=0;} audit{reject};
        phaseWorld->configurePhaseBoundary([](void* p,GameplayWorld::PhaseBoundary phase){
            auto& a=*static_cast<Audit*>(p);
            assert(unsigned(phase)==a.calls);
            assert(a.generations==(a.calls<2?0u:1u));
            return a.calls++!=a.reject;
        },&audit);
        phaseWorld->configureGenerationStep([](void* p,GameplayWorld&,GameMakerRandom&,int){
            auto& a=*static_cast<Audit*>(p);assert(a.calls==2);++a.generations;return true;
        },&audit);
        phaseWorld->step(0);
        assert(phaseWorld->effectsSucceeded()==(reject==5));
        assert(audit.calls==(reject<5?reject+1:5));
        if(reject<5){phaseWorld->step(0);assert(audit.calls==reject+1);}
    }
    for(unsigned mode=0;mode<4;++mode){
        auto world=std::make_unique<GameplayWorld>(300);
        std::vector<unsigned> order;
        world->configurePhaseBoundary([](void* p,GameplayWorld::PhaseBoundary phase){
            static_cast<std::vector<unsigned>*>(p)->push_back(unsigned(phase));return true;
        },&order);
        world->configureRegisteredActorEndStep([](void* p,long){
            static_cast<std::vector<unsigned>*>(p)->push_back(11);return true;
        },&order);
        unsigned passes=0;
        const bool ok=world->withCollisionPass([&]{
            ++passes;order.push_back(10);
            assert((order==std::vector<unsigned>{0,1,2,10}));
            if(mode==2){
                assert(!world->withCollisionPass([]{assert(false);return true;},[]{assert(false);return true;}));
            }else if(mode==3)world->step(0); // Direct recursion must also fail before advancing.
            return mode!=1; // Deliberately ignore the inner failure in modes 2/3.
        },[&]{world->step(0);return world->effectsSucceeded();});
        assert(ok==(mode==0)&&passes==1&&world->effectsSucceeded()==ok);
        if(ok){
            assert((order==std::vector<unsigned>{0,1,2,10,3,11,4}));
            order.clear();world->step(0);
            assert(world->effectsSucceeded()&&passes==1&&(order==std::vector<unsigned>{0,1,2,3,11,4}));
        }else{
            assert((order==std::vector<unsigned>{0,1,2,10}));world->step(0);
            assert(passes==1&&order.size()==4);
        }
    }
    for(bool reject:{false,true}){
        auto ordered=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,ordered->enemyBinding(),
            ordered->breakableBinding(),ordered->playerBinding(),&ordered->terrain());
        assert(ordered->attach(*scene));
        InstanceHandle h[3];
        for(unsigned i=0;i<3;++i){
            assert(scene->createGeneratedInstance({60,240,int(512+i*16)},objects,[](const auto&){return true;}));
            h[i]=ordered->terrain().handle(i);
        }
        assert(ordered->terrain().activation(h[1],false)&&ordered->reconcileActivation());
        unsigned population=0;assert(ordered->population(60,population)&&population==2);
        for(auto key:h)assert(ordered->terrain().requestDestroy(key));
        std::vector<unsigned> order;
        const bool cleaned=ordered->cleanupDestroyed([&](InstanceHandle shared,const GameplayCollisionView::Record& record){
            GameplayCollisionView::Kind kind;InstanceHandle local;
            assert(GameplayCollisionView::decode(shared,kind,local)&&kind==GameplayCollisionView::Kind::terrain&&
                record.terrain==ordered->terrain().find(local));
            order.push_back(local.slot);
            if(reject&&order.size()==2)return true; // A lying success must not acknowledge cleanup.
            return ordered->terrain().retireMarked(local);
        });
        assert(cleaned==!reject&&order[0]==h[0].slot&&order[1]==h[2].slot);
        if(reject){
            assert(!ordered->valid()&&order.size()==2&&ordered->terrain().size()==2&&
                !ordered->terrain().find(h[0])&&ordered->terrain().find(h[2]));
            assert(!ordered->cleanupDestroyed([](auto,const auto&){assert(false);return true;}));
        }else{
            assert(order.size()==3&&order[2]==h[1].slot&&ordered->terrain().size()==0&&ordered->valid());
            assert(ordered->population(60,population)&&population==0);
        }
    }
    auto rb=read("generated/downwell-rooms.dwrm");RoomPack rooms(rb.data(),rb.size());
    for(auto mode:{GameplayCollisionOwner::Ownership::native,GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),
            owner->playerBinding(),nullptr,&rooms);
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&world->bindNativeTerrain(*terrain)&&terrain->append(60,240,160));
        const auto local=terrain->handle(0);InstanceHandle shared;
        assert(terrain->createProperties(local,51)&&owner->flushNativeSpatial()&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,shared));
        PackedObject object;PackedSpriteCollision art;
        assert(objects.findObject(60,object)&&sprites.collisionSprite(51,object.textureMaskId,art));
        for(const auto delta:std::vector<std::pair<int,int>>{{0,0},{0,-16},{16,0},{0,16},{-16,0}}){
            assert(terrain->publishCachedOffset(local,delta.first,delta.second));
            RunnerImageValue x,y;assert(x.assignInteger(240+delta.first)&&y.assignInteger(160+delta.second));
            RunnerSpatialSplit::Box bounds;
            assert(RunnerSpriteBounds::fromSprite(art,x,y,{0x3f800000u},{0x3f800000u},false,{}, {},true,
                [](auto value,std::int32_t& pixel){return value.roundPixel(pixel);},bounds));
            bool hit=false;assert(owner->collisionPointCandidate(shared,bounds.left,bounds.top,true,false,masks,hit)&&hit);
            RunnerImageValue outside;assert(RunnerSpatialSplit::sub(bounds.right,{0xc0000000u},outside));
            assert(owner->collisionPointCandidate(shared,outside,bounds.top,true,false,masks,hit)&&!hit);
            GameplayWorld::NativeTerrain::Identity id;bool active,destroyed;
            assert(terrain->read(local,id,active,destroyed)&&id.x==240&&id.y==160);
        }
    }
    std::vector<std::vector<unsigned>> layerCutoffOracle;
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
        GameplayCollisionOwner::Ownership::nativeScenery}){
        const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),
            owner->playerBinding(),native?nullptr:&owner->terrain(),&rooms);
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain)));
        // Live actors at terrain depth remain intentionally unadmitted until
        // mixed raster composition is connected. Exercise independent layers
        // here; GameplayRoomLayers fixtures below cover same-layer cutoffs.
        const auto depth=MotionBinary64::fromInteger(0);
        auto frames=[&](int id,unsigned& count){PackedSpriteCollision art;
            if(!sprites.sprite(id,art))return false;count=art.frameCount;return true;};
        auto effect=[&]{InstanceHandle h;
            assert(world->spawnScriptEffect(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(160),
                603,{},{},depth,frames,h));
            world->step(0);assert(world->effectsSucceeded()); // Service the deferred source depth write.
            assert(owner->prepareDraw());return h;
        };
        const auto first=effect();
        InstanceHandle wall;
        if(native){assert(terrain->append(60,240,160));wall=terrain->handle(0);}
        else {assert(owner->terrain().append(60,240,160,true));wall=owner->terrain().handle(0);}
        const auto second=effect();assert(first.slot!=second.slot);
        unsigned checkpoint=0;
        auto draw=[&]{
            checkReadOnlyDraw(*world);std::vector<unsigned> result;
            assert(world->visitRoomActorsReadOnly([&](auto kind,InstanceHandle h,auto){
                if(kind==GameplayWorld::DrawActor::scriptEffect)result.push_back(h.slot);return true;
            }));
            if(!native)layerCutoffOracle.push_back(result);
            else assert(result==layerCutoffOracle.at(checkpoint));
            ++checkpoint;return result;
        };
        assert(draw().size()==2);
        auto activate=[&](bool active){return native?world->activateNativeTerrain(wall,active):owner->terrain().activation(wall,active);};
        assert(activate(false));assert(draw().size()==2);
        assert(owner->reconcileActivation());draw();
        assert(activate(true));draw();
        assert(owner->reconcileActivation());assert(draw().size()==2);
        assert(native?world->destroyNativeTerrain(wall):owner->terrain().requestDestroy(wall));
        assert(owner->prepareDraw());assert(draw().size()==2);
    }
    std::vector<unsigned> roomCleanupOracle;
    std::vector<std::vector<std::uint64_t>> registrationOracle;
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
        GameplayCollisionOwner::Ownership::nativeScenery}){
        const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),
            owner->playerBinding(),native?nullptr:&owner->terrain(),&rooms);
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain)));
        InstanceHandle local[3],shared[3];
        for(unsigned i=0;i<3;++i){
            if(native){assert(terrain->append(60,240+int(i)*16,160));local[i]=terrain->handle(i);}
            else {assert(owner->terrain().append(60,240+int(i)*16,160,true));local[i]=owner->terrain().handle(i);}
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local[i],shared[i]));
        }
        auto activate=[&](unsigned i,bool active){return native?world->activateNativeTerrain(local[i],active):owner->terrain().activation(local[i],active);};
        const int bx=0,by=160;assert(world->appendBreakables(&bx,&by,1));
        const InstanceHandle box{0,world->breakableSlot(0).lifetimeId};
        unsigned checkpoint=0;
        auto registrations=[&]{
            std::vector<std::uint64_t> actual;
            assert(owner->visitRegistrationOrder([&](InstanceHandle h,int object,std::uint32_t flags){
                actual.push_back((std::uint64_t(h.slot)<<32)|(std::uint64_t(object)<<20)|(flags&0x80003u));
                return true;
            }));
            if(!native)registrationOracle.push_back(actual);
            else assert(actual==registrationOracle.at(checkpoint));
            ++checkpoint;
        };
        auto member=[&](unsigned i){bool active=false;assert(owner->roomMembershipActive(shared[i],active));return active;};
        registrations();
        assert(activate(0,false)&&activate(2,false)&&member(0)&&member(2));
        registrations(); // Immediate inactive flags; membership unchanged.
        assert(owner->reconcileActivation()&&!member(0)&&member(1)&&!member(2));
        registrations(); // Deferred unlink, preserving the mixed-family order.
        assert(activate(0,true)&&!member(0)&&owner->reconcileActivation()&&member(0));
        registrations(); // Promotion appends after the box, not its old slot.
        for(unsigned i=0;i<3;++i)assert(native?world->destroyNativeTerrain(local[i]):owner->terrain().requestDestroy(local[i]));
        assert(world->markBreakableDestroyed(box));
        registrations(); // Destruction marks do not eagerly unlink.
        std::vector<unsigned> retired;
        assert(owner->cleanupDestroyed([&](InstanceHandle h,const GameplayCollisionView::Record& record){
            if(record.breakable){retired.push_back(99);return world->retireMarkedBreakable(box);}
            retired.push_back(h.slot-GameplayCollisionView::terrainBase);
            const InstanceHandle key{h.slot-GameplayCollisionView::terrainBase,h.generation};
            return record.nativeTerrain?world->retireNativeTerrain(key):owner->terrain().retireMarked(key);
        }));
        if(!native)roomCleanupOracle=retired;else assert(retired==roomCleanupOracle);
        std::vector<unsigned> walls;for(auto slot:retired)if(slot!=99)walls.push_back(slot);
        assert(walls==std::vector<unsigned>({1,0,2})&&retired.size()==4);
        for(auto h:shared){bool active;assert(!owner->roomMembershipActive(h,active));}
        registrations();
        assert(owner->prepareDraw());
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::native,GameplayCollisionOwner::Ownership::nativeScenery}){
        struct Notice {
            std::vector<bool> collision;std::vector<int> sprites;
            bool changed(int x,int y,bool enabled){assert(x==240&&y==160);collision.push_back(enabled);return true;}
            bool changed(int x,int y,int before,int after){assert(x==240&&y==160);sprites.push_back(before);sprites.push_back(after);return true;}
        } notice;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation::bind(notice),GameplayTerrainInstances::WallCollision::bind(notice),&rooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),
            owner->playerBinding(),nullptr,&rooms);
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&world->bindNativeTerrain(*terrain)&&terrain->append(60,240,160));
        const auto h=terrain->handle(0);assert(terrain->createProperties(h,51));
        auto pairs=std::make_unique<RunnerCollisionPairs<GameplayCollisionView::capacity,64>>();
        assert(owner->collectCollisionPairs(*pairs));
        std::uint32_t flags;assert(terrain->spatialFlags(h,flags)&&!(flags&8u)&&(flags&0x8000u));
        auto queued=[&](bool collision){unsigned count=0;
            assert(owner->visitNativeDirty(collision,[&](InstanceHandle item){
                assert(item.slot==GameplayCollisionView::terrainBase+h.slot&&item.generation==h.generation);++count;return true;}));
            return count;
        };
        assert(world->activateNativeTerrain(h,false)&&world->activateNativeTerrain(h,false));
        assert(terrain->spatialFlags(h,flags)&&(flags&2u)&&!(flags&8u)&&(flags&0x8000u));
        assert(!queued(false)&&!queued(true));
        assert(world->activateNativeTerrain(h,true)&&world->activateNativeTerrain(h,true));
        assert(terrain->spatialFlags(h,flags)&&!(flags&10u)&&queued(false)==1&&queued(true)==1);
        assert(owner->flushNativeSpatial()&&!queued(false)&&queued(true)==1);
        assert(world->destroyNativeTerrain(h)&&world->activateNativeTerrain(h,false)&&world->activateNativeTerrain(h,true));
        assert(notice.collision==std::vector<bool>({false,true,false}));
        assert(notice.sprites==std::vector<int>({51,-1,-1,51,51,-1}));
        assert(terrain->spatialFlags(h,flags)&&(flags&1u)&&!(flags&2u));
        assert(terrain->retire(h)&&!world->activateNativeTerrain(h,true));
    }
    for(bool scheduled:{false,true})for(unsigned transition:{0u,1u,2u,3u}){
        std::vector<unsigned> expected;
        for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
            GameplayCollisionOwner::Ownership::nativeScenery}){
            const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
                GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms,mode);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),
                owner->playerBinding(),native?nullptr:&owner->terrain(),&rooms);
            auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
            assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain))&&world->createAuthoredDitherAbove());
            GameMakerRandom random;
            for(unsigned i=0;i<8;++i){
                if(native){assert(terrain->append(60,496+int(i)*16,-128));assert(terrain->createProperties(terrain->handle(i),51));}
                else {assert(owner->terrain().append(60,496+int(i)*16,-128,true));
                    assert(owner->terrain().createProperties(owner->terrain().handle(i),51,false,random));}
            }
            auto pairs=std::make_unique<RunnerCollisionPairs<GameplayCollisionView::capacity,64>>();
            struct NonSolidMotion {
                bool rollback(InstanceHandle){assert(false);return false;}
                bool prepareMotion(InstanceHandle){assert(false);return false;}
                bool advanceMotion(InstanceHandle){assert(false);return false;}
            } motion;
            RunnerCollisionEventContext context;bool pending=transition==2;std::vector<unsigned> actual;
            const auto draws=world->randomDrawCount();
            unsigned passes=0;
            const auto pass=[&]{
                ++passes;
                assert(owner->collectCollisionPairs(*pairs)&&pairs->size()>0);
                return owner->executeCollisionPairs(*pairs,masks,context,motion,
                [&]{return RunnerCollisionEventContext::RoomState{true,pending};},
                [&](const auto& event){
                    assert(context.depth()==1&&context.current()->code==991&&event.code==991);
                    actual.push_back(event.other.slot);
                    if(!owner->dispatchDitherAboveCollision(event))return false;
                    if(transition==1)pending=true;
                    return transition!=3;
                });
            };
            const bool executed=scheduled?world->withCollisionPass(pass,[&]{
                world->step(0);return world->effectsSucceeded();
            }):pass();
            assert(passes==1&&(!scheduled||world->effectsSucceeded()==executed));
            assert(executed==(transition!=3)&&owner->valid()==(transition!=3));
            assert(!context.depth()&&!context.current()&&world->randomDrawCount()==draws);
            assert(transition==2?actual.empty():(!actual.empty()&&(!transition||actual.size()==1)));
            if(!native)expected=actual;else assert(actual==expected);
            for(unsigned slot:actual){
                const unsigned index=slot-GameplayCollisionView::terrainBase;
                if(native){GameplayWorld::NativeTerrain::Identity id;bool active,destroyed;
                    assert(terrain->read(terrain->handle(index),id,active,destroyed)&&destroyed);}
                else assert(owner->terrain().find(owner->terrain().handle(index))->destroyRequested);
            }
            if(executed){
                assert(owner->prepareDraw());
                assert((native?terrain->size():owner->terrain().size())==8-actual.size());
            }
        }
    }
    {
        std::vector<bool> expected;unsigned hits=0,misses=0;
        for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
            GameplayCollisionOwner::Ownership::nativeScenery}){
            const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
                GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms,mode);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),
                owner->playerBinding(),native?nullptr:&owner->terrain(),&rooms);
            auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
            assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain)));
            InstanceHandle wall,player;GameMakerRandom random;
            if(native){assert(terrain->append(60,240,160));wall=terrain->handle(0);assert(terrain->createProperties(wall,51));}
            else {assert(owner->terrain().append(60,240,160,true));wall=owner->terrain().handle(0);
                assert(owner->terrain().createProperties(wall,51,false,random));}
            const auto local=world->playerHandle();const auto* record=world->findPlayer(local);
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::player,local,player));
            InstanceHandle target;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,wall,target));
            unsigned ordinal=0;
            auto compare=[&](bool publish){
                for(bool reverse:{false,true}){
                    bool hit=false;assert(owner->collisionOverlap(reverse?target:player,reverse?player:target,masks,publish,hit));
                    if(!native){expected.push_back(hit);hit?++hits:++misses;}
                    else assert(ordinal<expected.size()&&hit==expected[ordinal]);
                    ++ordinal;
                }
            };
            compare(false); // Cached constructor bounds; no implicit flush.
            auto point=[&](RunnerImageValue x,RunnerImageValue y){
                for(auto h:{player,target})for(bool compatibility:{false,true})for(bool precise:{false,true}){
                    bool hit=false;assert(owner->collisionPointCandidate(h,x,y,compatibility,precise,masks,hit));
                    if(!native){expected.push_back(hit);hit?++hits:++misses;}
                    else assert(ordinal<expected.size()&&hit==expected[ordinal]);
                    ++ordinal;
                }
                {
                    bool hit=false;
                    assert(owner->actorSolidPlaceMeeting(player,x.promoted(),y.promoted(),masks,hit));
                    if(!native){expected.push_back(hit);hit?++hits:++misses;}
                    else assert(ordinal<expected.size()&&hit==expected[ordinal]);
                    ++ordinal;compare(false);compare(true);
                }
                for(int object:{60,93}){
                    bool hit=false;assert(owner->terrainPositionMeeting(object,x.promoted(),y.promoted(),masks,hit));
                    if(!native){expected.push_back(hit);hit?++hits:++misses;}
                    else assert(ordinal<expected.size()&&hit==expected[ordinal]);
                    ++ordinal;
                    auto shifted=x.promoted();assert(shifted.add(MotionBinary64::fromInteger(16)));
                    assert(owner->actorTerrainPlaceMeeting(player,object,shifted,y.promoted(),masks,hit));
                    if(!native){expected.push_back(hit);hit?++hits:++misses;}
                    else assert(ordinal<expected.size()&&hit==expected[ordinal]);
                    ++ordinal;
                    compare(false); // Includes the list path's retained temporary bounds.
                    compare(true); // Restored position must refresh to its original shape.
                    for(const auto delta:std::vector<std::pair<int,int>>{{0,-16},{16,0},{0,16},{-16,0}}){
                        assert(owner->terrainPlaceMeeting(wall,object,MotionBinary64::fromInteger(240+delta.first),
                            MotionBinary64::fromInteger(160+delta.second),masks,hit));
                        if(!native){expected.push_back(hit);hit?++hits:++misses;}
                        else assert(ordinal<expected.size()&&hit==expected[ordinal]);
                        ++ordinal;compare(false);compare(true);
                    }
                }
            };
            RunnerImageValue queryX,queryY;assert(queryX.assignInteger(240)&&queryY.assignInteger(160));
            point(queryX,queryY); // Dirty post-Create bounds refresh on the candidate path.
            // A split tree with many walls and one platform exercises spatial
            // traversal for 60 and the population<height list path for 93.
            for(unsigned n=0;n<41;++n){
                const int object=n==40?93:60,x=n==40?240:n==0?256:2000+int(n)*32;
                if(native){assert(terrain->append(object,x,160));assert(terrain->createProperties(terrain->handle(n+1),51));}
                else {assert(owner->terrain().append(object,x,160,true));
                    assert(owner->terrain().createProperties(owner->terrain().handle(n+1),51,false,random));}
            }
            point(queryX,queryY);
            assert(native?world->activateNativeTerrain(wall,false):owner->terrain().activation(wall,false));
            point(queryX,queryY); // Candidate geometry itself must not filter inactive state.
            assert(native?world->activateNativeTerrain(wall,true):owner->terrain().activation(wall,true));
            for(int mask:{-1,51,99999})for(unsigned variant=0;variant<3;++variant){
                const auto binding=owner->playerBinding();
                assert(binding.mask(local,*record,mask));
                assert(binding.transform(local,*record,RunnerTransformField::scaleX,
                    {variant==1?0xbf800000u:0x3f800000u},{0x3f800000u},0xffffffu));
                assert(binding.transform(local,*record,RunnerTransformField::angle,
                    {variant==2?0x41f00000u:0u},{0x3f800000u},0xffffffu));
                for(int y=144;y<=176;y+=8)for(int x=220;x<=260;x+=5)for(bool fractional:{false,true}){
                    RunnerImageValue px,py;assert(px.assignInteger(x)&&py.assignInteger(y));
                    if(fractional){
                        auto xx=MotionBinary64::fromInteger(x),yy=MotionBinary64::fromInteger(y);
                        assert(xx.add(MotionBinary64{0x3fd00000u,0})&&yy.add(MotionBinary64{0x3fe00000u,0})&&px.assign(xx)&&py.assign(yy));
                    }
                    assert(binding.publishedPosition(local,*record,px,py));compare(false);compare(true);point(px,py);
                }
            }
            if(native)assert(terrain->markDestroyed(wall));else assert(owner->terrain().requestDestroy(wall));
            compare(true);
            RunnerImageValue px,py;assert(px.assignInteger(240)&&py.assignInteger(160));point(px,py);
            bool selfHit=true;assert(owner->collisionOverlap(player,player,masks,true,selfHit)&&!selfHit);
            if(native)assert(ordinal==expected.size());
        }
        assert(hits&&misses);
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::native,GameplayCollisionOwner::Ownership::nativeScenery})
    for(unsigned failure=0;failure<5;++failure){
        struct Notice {
            std::vector<int> order;
            bool changed(int x,int y,bool enabled){assert(x==240&&y==160&&!enabled);order.push_back(1);return true;}
            bool changed(int x,int y,int before,int after){assert(x==240&&y==160&&before==51&&after==-1);order.push_back(2);return true;}
        } notice;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation::bind(notice),GameplayTerrainInstances::WallCollision::bind(notice),&rooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),nullptr,&rooms);
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&world->bindNativeTerrain(*terrain)&&world->createAuthoredDitherAbove());
        assert(terrain->append(failure==2?93:60,240,160,failure!=4));
        const auto wall=terrain->handle(0);InstanceHandle self,other;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::ditherAbove,world->ditherAbove()->handle(),self)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,wall,other));
        RunnerCollisionEventContext::Frame event{self,other,4,60,61,991};
        if(failure==1)event.code=990;
        if(failure==3)++event.other.generation;
        const auto draws=world->randomDrawCount();
        const bool accepted=failure==0||failure==4;
        assert(owner->dispatchDitherAboveCollision(event)==accepted&&world->randomDrawCount()==draws);
        if(!accepted){assert(notice.order.empty());continue;}
        GameplayWorld::NativeTerrain::Identity id;bool active,destroyed;
        assert(terrain->read(wall,id,active,destroyed)&&destroyed&&GameplayCollisionView(*world).contains(other));
        assert(notice.order==(failure==4?std::vector<int>{}:std::vector<int>{1,2}));
        assert(owner->dispatchDitherAboveCollision(event)); // Once-only publication.
        assert(notice.order.size()==(failure==4?0u:2u));
        std::uint32_t flags;assert(terrain->spatialFlags(wall,flags)&&(flags&1u));
        if(!failure)assert(flags&0x8000u); // Keep spatial identity until physical cleanup.
        assert(terrain->retire(wall)&&!GameplayCollisionView(*world).contains(other)&&terrain->append(60,240,160));
        assert(terrain->handle(0).slot==wall.slot&&terrain->handle(0).generation!=wall.generation);
        assert(!owner->dispatchDitherAboveCollision(event));
    }
    {
        using Pairs=RunnerCollisionPairs<GameplayCollisionView::capacity,512>;
        std::vector<std::vector<Pairs::Pair>> expected;
        for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
            GameplayCollisionOwner::Ownership::nativeScenery}){
            const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
                GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms,mode);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),
                owner->playerBinding(),native?nullptr:&owner->terrain(),&rooms);
            auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
            assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain))&&world->createAuthoredDitherAbove());
            GameMakerRandom random;
            // The authored dither's mask ends at y=-80. Put the walls
            // inside it, including an unindexed inactive wall and a marked
            // but still indexed wall (candidate exclusion is dispatch-time).
            for(unsigned i=0;i<12;++i){
                InstanceHandle h;
                if(native){
                    assert(terrain->append(60,240+int(i)*16,-96,i!=4));h=terrain->handle(i);
                    assert(terrain->createProperties(h,51));
                    if(i==5)assert(terrain->markDestroyed(h));
                }else{
                    assert(owner->terrain().append(60,240+int(i)*16,-96,i!=4));h=owner->terrain().handle(i);
                    assert(owner->terrain().createProperties(h,51,false,random));
                    if(i==5)assert(owner->terrain().requestDestroy(h));
                }
            }
            assert(!random.drawCount());
            auto pairs=std::make_unique<Pairs>();
            const auto player=world->playerHandle();const auto* record=world->findPlayer(player);
            for(unsigned pass=0;pass<4;++pass){
                if(pass==2)assert(owner->playerBinding().publishedPosition(player,*record,{0x43708000u},{0x43200000u}));
                assert(owner->collectCollisionPairs(*pairs,pass!=2));
                std::vector<Pairs::Pair> actual;
                for(unsigned i=0;i<pairs->size();++i)actual.push_back(*pairs->at(i));
                if(!pass){
                    assert(actual.size()==11);
                    bool marked=false;
                    for(const auto& pair:actual){
                        assert(pair.self.slot!=GameplayCollisionView::terrainBase+4&&
                            pair.other.slot!=GameplayCollisionView::terrainBase+4);
                        marked|=pair.self.slot==GameplayCollisionView::terrainBase+5||
                            pair.other.slot==GameplayCollisionView::terrainBase+5;
                    }
                    assert(marked);
                }
                if(!native)expected.push_back(actual);
                else {
                    assert(actual.size()==expected[pass].size());
                    for(unsigned i=0;i<actual.size();++i){const auto& a=actual[i];const auto& e=expected[pass][i];
                        assert(a.self.slot==e.self.slot&&a.self.generation==e.self.generation&&
                            a.other.slot==e.other.slot&&a.other.generation==e.other.generation);
                    }
                }
            }
        }
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::native,GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),owner->playerBinding(),nullptr,&rooms);
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&world->bindNativeTerrain(*terrain));
        InstanceHandle walls[4];
        for(unsigned i=0;i<4;++i){
            assert(terrain->append(i==1?93:60,240+int(i)*16,160,i!=2));walls[i]=terrain->handle(i);
            assert(terrain->createProperties(walls[i],51));int cached;
            assert(terrain->cachedSprite(walls[i],cached)&&cached==-2);
        }
        assert(terrain->markDestroyed(walls[3]));
        const auto player=world->playerHandle();const auto* record=world->findPlayer(player);
        assert(owner->playerBinding().publishedPosition(player,*record,{0x43708000u},{0x442f9000u}));
        std::vector<InstanceHandle> collision;
        assert(owner->visitNativeDirty(true,[&](InstanceHandle h){collision.push_back(h);return true;}));
        assert(owner->flushNativeSpatial());
        assert(owner->visitNativeDirty(false,[](InstanceHandle){assert(false);return false;}));
        unsigned ordinal=0;
        assert(owner->visitNativeDirty(true,[&](InstanceHandle h){
            assert(ordinal<collision.size()&&h.slot==collision[ordinal].slot&&h.generation==collision[ordinal].generation);++ordinal;return true;
        })&&ordinal==collision.size());
        for(unsigned i=0;i<4;++i){
            int cached;std::uint32_t flags;
            assert(terrain->cachedSprite(walls[i],cached)&&cached==(i<2?51:-2)&&terrain->spatialFlags(walls[i],flags));
            if(i<2)assert(!(flags&8u)&&(flags&0x8000u));
        }
        const auto pending=owner->pendingSpatialIndex();
        assert(owner->flushNativeSpatial()&&owner->pendingSpatialIndex()==pending); // Empty pass emits no mutations.
        GameplayCollisionOwner::Owner::Box all;assert(all.left.assignInteger(-10000)&&all.top.assignInteger(-10000)&&
            all.right.assignInteger(10000)&&all.bottom.assignInteger(10000));
        unsigned hits=0;assert(owner->inspectNativeIndex(all,[&](InstanceHandle){++hits;return true;})&&hits==4);
        assert(terrain->markDestroyed(walls[0])&&terrain->retire(walls[0]));
        assert(terrain->markDestroyed(walls[1])&&terrain->retire(walls[1]));
        assert(terrain->retire(walls[3]));
        hits=0;assert(owner->inspectNativeIndex(all,[&](InstanceHandle){++hits;return true;})&&hits==1);
    }
    {
        using Sprite=GameplayCollisionOwner::Owner::Sprite;
        std::vector<Sprite> expected;
        std::vector<std::uint32_t> expectedFlags;
        std::vector<GameplayCollisionOwner::Owner::Box> expectedBounds;
        for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
            GameplayCollisionOwner::Ownership::nativeScenery}){
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
                GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms,mode);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
                owner->breakableBinding(),owner->playerBinding(),nullptr,&rooms);
            assert(owner->attach(*world));
            const auto local=world->playerHandle();const auto* record=world->findPlayer(local);
            InstanceHandle h;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::player,local,h));
            const auto binding=owner->playerBinding();unsigned step=0;
            auto compare=[&]{
                Sprite value;std::uint32_t flags;
                if(mode==GameplayCollisionOwner::Ownership::reference){
                    value=owner->properties(h)->sprite;flags=owner->state(h)->flags;
                    expected.push_back(value);expectedFlags.push_back(flags);
                    expectedBounds.push_back(owner->state(h)->bounds);
                }else{
                    assert(owner->nativePublishedSprite(h,value,flags));const auto& e=expected[step];
                    assert(value.spriteId==e.spriteId&&value.maskId==e.maskId&&value.scaleX.bits==e.scaleX.bits&&
                        value.scaleY.bits==e.scaleY.bits&&value.angle.bits==e.angle.bits&&value.imageIndex.bits==e.imageIndex.bits&&
                        (flags&0xc208u)==(expectedFlags[step]&0xc208u));
                    GameplayCollisionOwner::Owner::Box bounds;assert(owner->nativeActorBounds(h,bounds));
                    const auto& b=expectedBounds[step];
                    assert(bounds.left.bits==b.left.bits&&bounds.right.bits==b.right.bits&&
                        bounds.top.bits==b.top.bits&&bounds.bottom.bits==b.bottom.bits);
                }
                ++step;
            };
            compare();
            assert(binding.mask(local,*record,99999));compare(); // Missing positive resources remain stored; negative IDs fall back to the sprite.
            if(mode==GameplayCollisionOwner::Ownership::reference)assert(expected.back().maskId==99999);
            assert(binding.transform(local,*record,RunnerTransformField::scaleX,{0x3f000000u},{0x3f800000u},0xffffffu));compare();
            assert(binding.transform(local,*record,RunnerTransformField::angle,{0x42340000u},{0x3f800000u},0xffffffu));compare();
            assert(binding.transform(local,*record,RunnerTransformField::angle,{0},{0x3f800000u},0xffffffu));compare();
            // Numeric no-op bypasses appearance validation and keeps +0.
            assert(binding.transform(local,*record,RunnerTransformField::angle,{0x80000000u},{0x7fc00000u},0));compare();
            assert(binding.transform(local,*record,RunnerTransformField::scaleX,{0x3f800000u},{0x3f800000u},0xffffffu));compare();
            assert(binding.imageIndex(local,*record,{0x447a0000u}));compare();
            assert(binding.sprite(local,*record,-1));compare();
            assert(binding.imageIndex(local,*record,{0xc0200000u}));compare();
            assert(binding.sprite(local,*record,35));compare();
            if(mode!=GameplayCollisionOwner::Ownership::reference){
                GameplayCollisionOwner::Owner::Box bounds;
                assert(owner->nativeActorBounds(h,bounds,true)); // Missing mask refresh caches the position.
                RunnerImageValue x,y;assert(owner->nativePublishedPosition(h,x,y));
                assert(bounds.left.bits==x.bits&&bounds.right.bits==x.bits&&bounds.top.bits==y.bits&&bounds.bottom.bits==y.bits);
                std::uint32_t flags;Sprite value;assert(owner->nativePublishedSprite(h,value,flags)&&!(flags&0x208u)&&(flags&0x8000u));
                GameplayCollisionOwner::Owner::Box all;assert(all.left.assignInteger(-10000)&&all.top.assignInteger(-10000)&&
                    all.right.assignInteger(10000)&&all.bottom.assignInteger(10000));
                unsigned hits=0;assert(owner->inspectNativeIndex(all,[&](InstanceHandle found){
                    if(found.slot==h.slot&&found.generation==h.generation)++hits;return true;
                })&&hits==1);
                // Retirement must retry the tree search: missing-mask cached
                // bounds no longer describe the old indexed rectangle.
                assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::gameplay);
                assert(owner->inspectNativeIndex(all,[&](InstanceHandle found){
                    assert(found.slot!=h.slot||found.generation!=h.generation);return true;
                }));
            }
        }
    }
    for(auto ownership:{GameplayCollisionOwner::Ownership::native,GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms,ownership);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),nullptr,&rooms);
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&world->bindNativeTerrain(*terrain)&&world->bindNativeTerrain(*terrain));
        {
            const auto local=world->playerHandle();const auto* record=world->findPlayer(local);
            InstanceHandle shared;assert(record&&GameplayCollisionView::encode(GameplayCollisionView::Kind::player,local,shared));
            auto binding=owner->playerBinding();RunnerImageValue x,y;
            assert(binding.publishedPosition(local,*record,{0},{0}));
            assert(binding.publishedPosition(local,*record,{0x80000000u},{0x80000000u}));
            assert(owner->nativePublishedPosition(shared,x,y)&&!x.bits&&!y.bits);
            assert(binding.publishedPosition(local,*record,{0x43708000u},{0xc1fa0000u}));
            assert(owner->nativePublishedPosition(shared,x,y)&&x.bits==0x43708000u&&y.bits==0xc1fa0000u);
            assert(!owner->nativePublishedPosition({shared.slot,shared.generation+1},x,y));
        }
        assert(terrain->append(60,240,160,true,100857));
        const unsigned firstOrder=terrain->registrationOrder(terrain->handle(0));
        assert(firstOrder&&world->createAuthoredDitherAbove());
        assert(terrain->append(60,240,160,false,100858));
        assert(terrain->registrationOrder(terrain->handle(1))==firstOrder+2);
        const auto local=terrain->handle(0),coincident=terrain->handle(1);
        InstanceHandle shared,other;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,shared)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,coincident,other));
        InstanceHandle dither;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::ditherAbove,
            world->ditherAbove()->handle(),dither));
        auto queued=[&](bool collision){
            std::vector<InstanceHandle> result;
            assert(owner->visitNativeDirty(collision,[&](InstanceHandle h){result.push_back(h);return true;}));
            return result;
        };
        auto same=[](InstanceHandle a,InstanceHandle b){return a.slot==b.slot&&a.generation==b.generation;};
        auto indexed=[&](InstanceHandle target){
            GameplayCollisionOwner::Owner::Box all;assert(all.left.assignInteger(-10000)&&all.top.assignInteger(-10000)&&
                all.right.assignInteger(10000)&&all.bottom.assignInteger(10000));
            unsigned count=0;assert(owner->inspectNativeIndex(all,[&](InstanceHandle h){if(same(h,target))++count;return true;}));
            return count;
        };
        std::uint32_t terrainFlags;
        assert(terrain->spatialFlags(local,terrainFlags)&&(terrainFlags&0x8000u)&&!(terrainFlags&8u));
        assert(terrain->spatialFlags(coincident,terrainFlags)&&!(terrainFlags&0x8000u)&&(terrainFlags&2u));
        assert(indexed(shared)==1&&indexed(other)==0); // Inactive birth refreshes bounds but does not insert.
        {
            int sprite;unsigned frame;bool created;
            assert(terrain->appearance(coincident,sprite,frame,created)&&!created&&sprite==-2);
            const auto random=world->randomDrawCount();
            assert(world->finishNativeTerrainCreate(coincident)&&world->randomDrawCount()==random);
            assert(terrain->appearance(coincident,sprite,frame,created)&&created&&frame==0&&
                sprite==terrainTileForArea(world->runStage().area));
            assert(indexed(shared)==1&&indexed(other)==0); // Create must not immediately republish bounds.
        }
        for(bool collision:{false,true}){
            const auto q=queued(collision);assert(q.size()>=3);
            assert(same(q[0],other)&&same(q[1],dither)&&same(q[2],shared));
        }
        GameplayCollisionView view(*world);GameplayCollisionView::Record record;
        GameplayWorld::NativeTerrain::Identity id;bool active,destroyed;
        assert(view.contains(shared)&&view.resolve(shared,record)&&record.nativeTerrain&&!record.terrain);
        assert(view.readTerrain(shared,id,active,destroyed)&&id.source==100857&&id.object==60&&
            id.x==240&&id.y==160&&active&&!destroyed);
        int parent;assert(owner->projectileWallParent(shared,parent)&&parent==59);
        assert(terrain->markDestroyed(local)&&view.contains(shared)&&view.readTerrain(shared,id,active,destroyed)&&destroyed);
        assert(indexed(shared)==1); // Exclusion is immediate, spatial retirement is deferred.
        for(bool collision:{false,true})assert(same(queued(collision)[2],shared)); // Mark is not retirement or a setter.
        assert(terrain->retire(local)&&!view.contains(shared)&&!owner->projectileWallParent(shared,parent));
        assert(indexed(shared)==0);
        for(bool collision:{false,true})for(auto h:queued(collision))assert(!same(h,shared));
        assert(view.readTerrain(other,id,active,destroyed)&&id.source==100858&&!active&&!destroyed);
        assert(terrain->append(93,240,160));const auto replacement=terrain->handle(1);
        assert(replacement.slot==local.slot&&replacement.generation!=local.generation&&!view.contains(shared));
        assert(world->finishNativeTerrainCreate(replacement));
        int platformSprite;unsigned platformIndex;bool platformCreated;
        assert(terrain->appearance(replacement,platformSprite,platformIndex,platformCreated)&&platformCreated&&
            platformIndex==23&&platformSprite==terrainTileForArea(world->runStage().area));
        const auto playerLocal=world->playerHandle();const auto* playerRecord=world->findPlayer(playerLocal);
        InstanceHandle playerShared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::player,playerLocal,playerShared));
        auto playerBinding=owner->playerBinding();
        // An equal setter does not move the player ahead of intervening births.
        assert(playerBinding.publishedPosition(playerLocal,*playerRecord,{0x43708000u},{0xc1fa0000u}));
        for(bool collision:{false,true})assert(queued(collision)[0].slot==shared.slot);
        assert(playerBinding.publishedPosition(playerLocal,*playerRecord,{0x43710000u},{0xc1fa0000u}));
        for(bool collision:{false,true})assert(same(queued(collision)[0],playerShared));
        const int boxX=0,boxY=160;assert(world->appendBreakables(&boxX,&boxY,1));
        InstanceHandle box;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::breakable,
            {world->breakableSlots()-1,world->breakableSlot(world->breakableSlots()-1).lifetimeId},box));
        for(bool collision:{false,true})assert(same(queued(collision)[0],box));
        GameplayCollisionOwner::Owner::Sprite published;std::uint32_t publishedFlags;
        assert(owner->nativePublishedSprite(playerShared,published,publishedFlags));
        assert(playerBinding.mask(playerLocal,*playerRecord,published.maskId));
        assert(playerBinding.sprite(playerLocal,*playerRecord,published.spriteId));
        assert(playerBinding.transform(playerLocal,*playerRecord,RunnerTransformField::angle,published.angle,{0x3f800000u},0xffffffu));
        assert(playerBinding.imageIndex(playerLocal,*playerRecord,{0x3fc00000u}));
        for(bool collision:{false,true})assert(same(queued(collision)[0],box));
        assert(playerBinding.activation(playerLocal,*playerRecord,false));
        for(bool collision:{false,true})assert(same(queued(collision)[0],box));
        assert(playerBinding.activation(playerLocal,*playerRecord,true));
        for(bool collision:{false,true})assert(same(queued(collision)[0],playerShared));
        assert(world->appendBreakables(&boxX,&boxY,1));
        assert(playerBinding.mask(playerLocal,*playerRecord,-123));
        for(bool collision:{false,true})assert(same(queued(collision)[0],playerShared));
        assert(world->appendBreakables(&boxX,&boxY,1));
        assert(playerBinding.transform(playerLocal,*playerRecord,RunnerTransformField::angle,{0x42340000u},{0x3f800000u},0xffffffu));
        for(bool collision:{false,true})assert(same(queued(collision)[0],playerShared));
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::gameplay);
        assert(world->nativeTerrainInstances()==terrain.get()&&!terrain->size()&&!view.contains(other));
        assert(indexed(shared)==0&&indexed(other)==0&&indexed(dither)==0);
        for(bool collision:{false,true})for(auto h:queued(collision))assert(!same(h,other)&&!same(h,dither)&&!same(h,box));
        assert(terrain->append(60,240,160)&&terrain->handle(0).generation>replacement.generation&&
            terrain->registrationOrder(terrain->handle(0))>firstOrder+2);
    }
    for(unsigned failure=0;failure<4;++failure){
        struct CollisionNotice {
            unsigned disabled=0;
            bool changed(int x,int y,bool enabled){assert(x==240&&y==160&&!enabled);++disabled;return true;}
        } collision;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision::bind(collision),&rooms);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms);
        assert(owner->attach(*world)&&world->createAuthoredDitherAbove());
        auto& terrain=owner->terrain();
        assert(terrain.append(60,240,160,true)&&terrain.append(failure==2?93:60,240,160,true));
        const auto first=terrain.handle(0),second=terrain.handle(1),above=world->ditherAbove()->handle();
        InstanceHandle self,other;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::ditherAbove,above,self)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,second,other));
        RunnerCollisionEventContext::Frame event{self,other,4,60,61,991};
        if(failure==1)event.code=990;
        if(failure==3)++event.other.generation;
        const auto draws=world->randomDrawCount();
        assert(owner->dispatchDitherAboveCollision(event)==(failure==0));
        assert(world->randomDrawCount()==draws&&terrain.find(first)&&!terrain.find(first)->destroyRequested);
        if(failure){assert(!collision.disabled&&!terrain.find(second)->destroyRequested);continue;}
        assert(collision.disabled==1&&terrain.find(second)->destroyRequested&&
            world->ditherAbove()->find(above)&&!world->ditherAbove()->find(above)->destroyRequested);
        assert(owner->state(other)->flags&1u);
        assert(owner->dispatchDitherAboveCollision(event)&&collision.disabled==1);
        assert(owner->prepareDraw()&&!terrain.find(second)&&terrain.find(first)&&world->ditherAbove()->find(above));
        assert(terrain.append(60,240,160,true));
        const auto replacement=terrain.handle(1);
        assert(replacement.slot==second.slot&&replacement.generation!=second.generation);
        assert(!owner->dispatchDitherAboveCollision(event)&&!terrain.find(replacement)->destroyRequested&&collision.disabled==1);
    }
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
        GameplayCollisionOwner::Ownership::nativeScenery})for(bool low:{false,true}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms,ownership);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms);
        assert(owner->attach(*world)&&world->createAuthoredDitherAbove());
        world->configureDitherAboveEnvironment(low,false);
        const auto key=world->ditherAbove()->handle();InstanceHandle shared;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::ditherAbove,key,shared));
        assert(world->ditherAbove()->find(key)->sourceId==100854);
        for(unsigned tick=0;tick<40;++tick){
            const auto* r=world->ditherAbove()->find(key);assert(r);
            int y;assert(r->y.roundPixel(y));
            if(world->cameraOffsetY()+176-y>=(low?208:272))y+=16;
            world->step(0);assert(world->effectsSucceeded()&&owner->valid());
            r=world->ditherAbove()->find(key);assert(r);
            int actual;assert(r->y.roundPixel(actual)&&actual==y&&r->x.bits==0x43700000u);
            GameplayCollisionView::Record record;
            assert(GameplayCollisionView(*world).resolve(shared,record)&&record.ditherAbove==r);
            checkReadOnlyDraw(*world);
            if(ownership==GameplayCollisionOwner::Ownership::reference)
                assert(owner->state(shared)->x.bits==r->x.bits&&owner->state(shared)->y.bits==r->y.bits);
            else {
                RunnerImageValue x,y;
                assert(owner->nativePublishedPosition(shared,x,y)&&x.bits==r->x.bits&&y.bits==r->y.bits);
            }
        }
        world->configureDitherAboveEnvironment(low,true);
        world->step(0);assert(world->effectsSucceeded());
        assert(owner->prepareDraw()&&!world->ditherAbove()->find(key));
        RunnerImageValue retiredX,retiredY;
        assert(!owner->nativePublishedPosition(shared,retiredX,retiredY));
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::gameplay);
        assert(world->createAuthoredDitherAbove()&&world->ditherAbove()->handle().generation>key.generation&&
            !world->ditherAbove()->find(key));
        const auto replacement=world->ditherAbove()->handle();
        assert(!world->createAuthoredDitherAbove()&&!world->effectsSucceeded()&&
            world->ditherAbove()->find(replacement)->sourceId==100854);
    }
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
        GameplayCollisionOwner::Ownership::nativeScenery})for(unsigned mode=0;mode<4;++mode){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms,ownership);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms);
        assert(owner->attach(*world));
        GameplayDitherAbove::Snapshot source;
        source.sourceId=mode==3?0:100854;source.x.bits=0x43708000u;source.y.bits=0xc1fa0000u;
        source.active=mode!=1;source.destroyRequested=mode==2;source.lowSpec=true;
        const auto random=world->randomDrawCount();
        assert(world->adoptDitherAbove(source)&&world->randomDrawCount()==random);
        const auto key=world->ditherAbove()->handle();
        const auto* r=world->ditherAbove()->find(key);
        if(mode==3){assert(!r);continue;}
        assert(r&&r->sourceId==source.sourceId&&r->x.bits==source.x.bits&&r->y.bits==source.y.bits&&
            r->active==source.active&&r->destroyRequested==source.destroyRequested);
        InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::ditherAbove,key,shared));
        if(ownership==GameplayCollisionOwner::Ownership::reference)
            assert(owner->state(shared)->x.bits==source.x.bits&&owner->state(shared)->y.bits==source.y.bits);
        else {
            RunnerImageValue x,y;
            assert(owner->nativePublishedPosition(shared,x,y)&&x.bits==source.x.bits&&y.bits==source.y.bits);
        }
        assert(owner->prepareDraw());
        if(mode==2){assert(!world->ditherAbove()->find(key));continue;}
        checkReadOnlyDraw(*world);
        world->step(0);assert(world->effectsSucceeded());
        r=world->ditherAbove()->find(key);assert(r);
        // Adoption keeps fractional coordinates until the next eligible
        // source event. Inactive controllers do not run that event.
        assert(mode==1?(r->x.bits==source.x.bits&&r->y.bits==source.y.bits):r->y.bits!=source.y.bits);
    }
    {
        auto layers=std::make_unique<GameplayRoomLayers<8>>(&rooms);bool healthy=true;
        layers->observeFailure(healthy);assert(healthy&&layers->prepare());
        // The nested lifetime owner, not GameplayRoomLayers' own flag, fails.
        assert(!layers->writeDepth({0,1},MotionBinary64::fromInteger(2)));
        assert(!healthy&&!layers->valid());
    }
    if(const char* path=std::getenv("DOWNWELL_OWNERSHIP_WITNESS")){
        // Compare the whole owner, not just a membership container. Emit full
        // field values (not a hash) so a divergent publication is inspectable.
        std::ofstream trace(path);assert(trace);
        // Feed the real builder's borrowed spans through the core transaction.
        // The backend can fail at any cell, but may not mutate core ownership
        // inside this trusted run. Queries follow the run, never prewarm it.
        auto pb=read("generated/downwell-builder-patterns.dwbp");
        BuilderPatternPack patterns(pb.data(),pb.size());assert(patterns.valid());
        for(unsigned seed:{1u,5u,9u})for(unsigned reject:{0u,1u,3u,9u,41u}){
            auto runOwner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
                GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
            auto runWorld=std::make_unique<GameplayWorld>(300,true,0,false,runOwner->enemyBinding(),
                runOwner->breakableBinding(),runOwner->playerBinding(),&runOwner->terrain());
            assert(runOwner->attach(*runWorld));
            GameMakerRandom random(seed);AreaOneBuilder builder;
            builder.reset(240,704,random,{1,0,true,false});builder.enableTerrainRuns(true);
            struct RunContext{GameplayWorld& world;const ObjectPack& objects;unsigned calls=0,reject=0,runs=0;};
            RunContext context{*runWorld,objects,0,reject};
            const auto emit=[](void* opaque,const DeferredCreateInstance& command){
                auto& c=*static_cast<RunContext*>(opaque);
                if(command.objectId!=60&&command.objectId!=93&&command.objectId!=99)return true;
                return c.world.createGeneratedTerrain(command,[&](const auto&){return ++c.calls!=c.reject;});
            };
            builder.setTerrainRunEmitter([](void* opaque,const AreaOneBuilder::TerrainRun& run,AreaOneBuilder::EmitFunction){
                auto& c=*static_cast<RunContext*>(opaque);++c.runs;
                return c.world.createGeneratedTerrainRun(run,[&](const auto&){return ++c.calls!=c.reject;});
            });
            AreaOneBuilderStep step;
            const bool okay=builder.stepVisible(patterns,random,100000,emit,&context,step);
            trace<<"builder-run "<<seed<<' '<<reject<<' '<<okay<<' '<<context.calls<<' '<<context.runs
                <<' '<<runOwner->valid()<<' '<<runWorld->effectsSucceeded()<<' '<<random.drawCount()<<' '<<random.stateIndex()<<'\n';
            for(unsigned i=0;i<16;++i)trace<<random.stateWord(i)<<' ';
            trace<<'\n';
            for(unsigned i=0;i<runOwner->terrain().size();++i){
                const auto h=runOwner->terrain().handle(i);const auto r=runOwner->terrain().find(h);
                InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,h,shared));
                const auto s=runOwner->state(shared);const auto p=runOwner->properties(shared);assert(s&&p);
                trace<<"run-cell "<<h.slot<<' '<<h.generation<<' '<<r->objectId<<' '<<r->x<<' '<<r->y
                    <<' '<<r->spriteId<<' '<<r->imageIndex<<' '<<r->imageSpeed<<' '<<s->x.bits<<' '<<s->y.bits
                    <<' '<<s->previous.x.bits<<' '<<s->previous.y.bits<<' '<<s->flags<<' '<<s->kind<<' '<<s->special
                    <<' '<<s->bounds.left.bits<<' '<<s->bounds.top.bits<<' '<<s->bounds.right.bits<<' '<<s->bounds.bottom.bits
                    <<' '<<p->sprite.spriteId<<' '<<p->sprite.maskId<<' '<<p->sprite.scaleX.bits<<' '<<p->sprite.scaleY.bits
                    <<' '<<p->sprite.angle.bits<<' '<<p->sprite.imageIndex.bits<<' '<<p->depth.bits<<' '<<p->timeline.bits
                    <<' '<<p->cachedFrame.bits<<'\n';
            }
            for(int object:{60,93}){unsigned count;assert(runOwner->population(object,count));
                trace<<"run-count "<<object<<' '<<count<<'\n';}
            for(unsigned i=0;i<runOwner->terrain().size();i+=7){
                const auto r=runOwner->terrain().find(runOwner->terrain().handle(i));
                const auto x=MotionBinary64::fromInteger(r->x),y=MotionBinary64::fromInteger(r->y);
                GameplayCollisionOwner::Owner::Result result;
                assert(runOwner->terrainNonPreciseLine({},60,x,y,x,y,false,result));
                trace<<"run-query "<<result.found<<' '<<result.handle.slot<<' '<<result.handle.generation<<'\n';
            }
            assert(runOwner->prepareDraw());
            assert(runWorld->visitRoomActors([&](auto kind,InstanceHandle h,auto depth){
                trace<<"run-draw "<<unsigned(kind)<<' '<<h.slot<<' '<<h.generation<<' '<<depth.bits<<'\n';return true;
            }));
        }
        // Do not warm the representation by resolving every birth. A compact
        // terrain owner must survive a cold burst before its first observer,
        // including interleaved actors, coincident geometry and removed cells.
        for(unsigned mode=0;mode<4;++mode){
            auto cold=std::make_unique<GameplayCollisionOwner>(objects,sprites,
                GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
            auto scene=std::make_unique<GameplayWorld>(300,true,0,false,cold->enemyBinding(),
                cold->breakableBinding(),cold->playerBinding(),&cold->terrain());
            assert(cold->attach(*scene));
            for(unsigned i=0;i<128;++i){
                // Each group contains a coincident wall pair. Preserve first
                // identity, not merely the set of occupied cells.
                const int objectsByCell[]={60,60,93,99};
                const int x=240+int((i/4)%8)*16,y=512+int(i/32)*16;
                assert(scene->createGeneratedInstance({objectsByCell[i%4],x,y},objects,
                    [](const auto&){return true;}));
                if(i%17==0)assert(scene->createGeneratedInstance({112,x,y-32},objects,
                    [](const auto&){return true;}));
                auto random=scene->randomState();random.nextU32();scene->configureEnemyRandom(random);
            }
            for(unsigned i=0;i<128;i+=8){
                const auto h=cold->terrain().handle(i);
                if(mode==1||mode==2)assert(cold->terrain().activation(h,false));
                if(mode==3)assert(cold->terrain().requestDestroy(h));
            }
            if(mode==2)assert(cold->reconcileActivation());
            const auto random=scene->randomState();
            trace<<"cold "<<mode<<' '<<random.drawCount()<<' '<<random.stateIndex()<<'\n';
            for(unsigned probe=0;probe<32;++probe){
                const auto x=MotionBinary64::fromInteger(240+int(probe%8)*16);
                const auto y=MotionBinary64::fromInteger(512+int(probe/8)*16);
                GameplayCollisionOwner::Owner::Result hit;
                auto record=[&](const char* kind){
                    trace<<kind<<' '<<probe<<' '<<hit.found<<' '<<hit.handle.slot<<' '<<hit.handle.generation<<'\n';
                    if(hit.found){
                        GameplayCollisionView::Kind family;InstanceHandle local;
                        assert(GameplayCollisionView::decode(hit.handle,family,local)&&
                            family==GameplayCollisionView::Kind::terrain&&cold->terrain().find(local));
                    }
                };
                assert(cold->terrainCollisionPoint({},60,x,y,false,false,masks,hit));record("cold-point");
                const auto first=hit.handle;
                assert(cold->terrainCollisionPoint(first,60,x,y,false,true,masks,hit));record("cold-excluded");
                assert(!hit.found||hit.handle.slot!=first.slot||hit.handle.generation!=first.generation);
                assert(cold->terrainNonPreciseLine({},60,x,y,x,y,false,hit));record("cold-line");
            }
            for(unsigned i=0;i<128;++i){
                const auto h=cold->terrain().handle(i);const auto r=cold->terrain().find(h);assert(r);
                InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,h,shared));
                const auto before=*cold->state(shared);
                bool hit=false;
                assert(cold->terrainPlaceMeeting(h,60,MotionBinary64::fromInteger(r->x+16),
                    MotionBinary64::fromInteger(r->y),masks,hit));
                const auto after=cold->state(shared);assert(after);
                assert(before.x.bits==after->x.bits&&before.y.bits==after->y.bits&&
                    before.previous.x.bits==after->previous.x.bits&&before.previous.y.bits==after->previous.y.bits);
                trace<<"cold-restore "<<i<<' '<<hit<<' '<<after->x.bits<<' '<<after->y.bits
                    <<' '<<after->flags<<' '<<after->bounds.left.bits<<' '<<after->bounds.top.bits
                    <<' '<<after->bounds.right.bits<<' '<<after->bounds.bottom.bits<<'\n';
            }
            assert(scene->randomState().drawCount()==random.drawCount()&&
                scene->randomState().stateIndex()==random.stateIndex());
            for(unsigned i=0;i<16;++i){
                assert(scene->randomState().stateWord(i)==random.stateWord(i));
                trace<<"cold-rng "<<i<<' '<<random.stateWord(i)<<'\n';
            }
        }
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        unsigned checkpoint=0;
        auto capture=[&](){
            checkReadOnlyDraw(*world);
            trace<<"checkpoint "<<checkpoint++<<' '<<owner->valid()<<' '<<world->effectsSucceeded()<<'\n';
            const auto& random=world->randomState();
            trace<<"rng "<<random.drawCount()<<' '<<random.stateIndex();
            for(unsigned i=0;i<16;++i)trace<<' '<<random.stateWord(i);
            trace<<'\n';
            for(unsigned i=0;i<owner->terrain().size();++i){
                const auto h=owner->terrain().handle(i);const auto r=owner->terrain().find(h);assert(r);
                InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,h,shared));
                const auto s=owner->state(shared);const auto p=owner->properties(shared);assert(s&&p);
                trace<<"terrain "<<h.slot<<' '<<h.generation<<' '<<r->objectId<<' '<<r->x<<' '<<r->y
                    <<' '<<r->active<<' '<<r->sourceInstanceId<<' '<<r->creationCodeId<<' '<<r->spriteId
                    <<' '<<r->imageIndex<<' '<<r->imageSpeed<<' '<<r->destroyRequested<<'\n';
                trace<<"state "<<s->x.bits<<' '<<s->y.bits<<' '<<s->previous.x.bits<<' '<<s->previous.y.bits
                    <<' '<<s->bounds.left.bits<<' '<<s->bounds.top.bits<<' '<<s->bounds.right.bits
                    <<' '<<s->bounds.bottom.bits<<' '<<s->flags<<' '<<s->kind<<' '<<s->special<<'\n';
                trace<<"properties "<<p->sprite.spriteId<<' '<<p->sprite.maskId<<' '<<p->sprite.scaleX.bits
                    <<' '<<p->sprite.scaleY.bits<<' '<<p->sprite.angle.bits<<' '<<p->sprite.imageIndex.bits
                    <<' '<<p->depth.bits<<' '<<p->timeline.bits<<' '<<p->cachedFrame.bits<<'\n';
            }
            for(int object:{35,60,93,99,112}){unsigned count;assert(owner->population(object,count));
                trace<<"population "<<object<<' '<<count<<'\n';}
            RunnerStepSnapshot<256> phase;
            assert(owner->dispatchStepPhase(2,phase,[&](InstanceHandle h,const PackedObjectEvent& event){
                trace<<"event "<<h.slot<<' '<<h.generation<<' '<<event.ownerObjectId<<' '<<event.codeId<<'\n';
                return true;
            }));
            assert(world->visitRoomActors([&](auto kind,InstanceHandle h,auto depth){
                trace<<"draw "<<unsigned(kind)<<' '<<h.slot<<' '<<h.generation<<' '<<depth.bits<<'\n';return true;
            }));
        };
        capture();
        for(unsigned i=0;i<96;++i){
            const int kinds[]={60,60,93,99,112,60};
            DeferredCreateInstance next(kinds[i%6],240+int(i%8)*16,512+int(i/8)*16);
            next.active=i%7!=0;
            // The terrain sink precedes native ownership and Create. Capture
            // there as well as after the builder-facing call returns.
            assert(world->createGeneratedInstance(next,objects,[&](const auto&){capture();return true;}));
            capture();
            if(i%5==0&&owner->terrain().size()>2){
                const auto h=owner->terrain().handle(1);
                assert(owner->terrain().activation(h,false));capture();
                assert(owner->reconcileActivation());capture();
                assert(owner->terrain().activation(h,true));capture();
                assert(owner->reconcileActivation());capture();
            }
            if(i%3==0&&owner->terrain().size()>4){
                const auto h=owner->terrain().handle(2);
                assert(owner->terrain().requestDestroy(h));capture();
                assert(owner->prepareDraw());capture();
                assert(!owner->terrain().find(h));
            }
        }
        // Observe the birth/Create boundary separately. The complete builder
        // above cannot expose it through its terrain sink (which runs first).
        for(unsigned i=0;i<16;++i){
            assert(owner->terrain().append(60,240+int(i%8)*16,768+int(i/8)*16,true));
            capture();
            auto random=world->randomState();
            assert(owner->terrain().createProperties(owner->terrain().handle(owner->terrain().size()-1),
                terrainTileForArea(1),false,random));
            world->configureEnemyRandom(random);capture();
            // An unrelated gameplay draw must remain between these Creates.
            random.nextU32();world->configureEnemyRandom(random);capture();
        }
        capture();
        assert(!world->createGeneratedInstance({60,256,832},objects,[](const auto&){return false;}));
        capture(); // Sink rejection publishes neither lifetime nor RNG draw.
        assert(trace.good());
    }
    {
        // Ownership-replacement gate: typed terrain appearance, runner
        // appearance and published bounds are three different observations.
        // A compact owner must not derive all three from the current tile.
        auto boundary=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,boundary->enemyBinding(),
            boundary->breakableBinding(),boundary->playerBinding(),&boundary->terrain());
        assert(boundary->attach(*scene));
        GameMakerRandom random;const auto draws=random.drawCount();
        for(int object:{60,93,99}){
            assert(boundary->terrain().append(object,240,512,true));
            const auto local=boundary->terrain().handle(boundary->terrain().size()-1);
            InstanceHandle shared;
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,shared));
            const auto initial=*boundary->properties(shared);
            const auto birth=*boundary->state(shared);
            auto unchangedDefaults=[&]{
                const auto p=*boundary->properties(shared);
                assert(p.sprite.maskId==initial.sprite.maskId&&p.depth.bits==initial.depth.bits&&
                    p.sprite.scaleX.bits==0x3f800000u&&p.sprite.scaleY.bits==0x3f800000u&&
                    p.sprite.angle.bits==0&&p.timeline.bits==0&&p.cachedFrame.bits==0xbf800000u);
                const auto stateRead=boundary->state(shared);const auto& s=*stateRead;
                assert(s.x.bits==birth.x.bits&&s.y.bits==birth.y.bits&&
                    s.previous.x.bits==birth.previous.x.bits&&s.previous.y.bits==birth.previous.y.bits&&
                    s.kind==birth.kind&&s.special==birth.special);
            };
            auto unchangedBounds=[&]{
                const auto stateRead=boundary->state(shared);const auto& b=stateRead->bounds;
                assert(b.left.bits==birth.bounds.left.bits&&b.top.bits==birth.bounds.top.bits&&
                    b.right.bits==birth.bounds.right.bits&&b.bottom.bits==birth.bounds.bottom.bits);
            };
            unchangedDefaults();
            // Marker Create is a no-op, even with an invalid requested tile.
            if(object==99){
                assert(boundary->terrain().createProperties(local,-1,false,random));
                assert(boundary->properties(shared)->sprite.spriteId==initial.sprite.spriteId);
                unchangedDefaults();unchangedBounds();continue;
            }
            for(int area=0;area<6;++area){
                assert(boundary->terrain().createProperties(local,terrainTileForArea(area),false,random));
                assert(boundary->terrain().imageIndex(local,15));
                assert(boundary->properties(shared)->sprite.imageIndex.bits==0x41700000u);
                // Missing sprite: the original ordinary sprite setter resets
                // the nonnegative index, but wall Create does not subsequently
                // write the typed presentation index. Platform Create does.
                assert(boundary->terrain().createProperties(local,-1,false,random));
                const auto p=*boundary->properties(shared);
                const auto typed=boundary->terrain().find(local);
                assert(p.sprite.spriteId==-1&&typed->spriteId==-1);
                assert(p.sprite.imageIndex.bits==(object==60?0u:0x41b80000u));
                assert(typed->imageIndex==(object==60?15:23));
                unchangedDefaults();unchangedBounds();
                // A same-sprite write is a no-op, including its index. It
                // must not perform another inferred constructor/reset step.
                const auto savedIndex=p.sprite.imageIndex.bits;
                assert(boundary->terrain().createProperties(local,-1,false,random));
                assert(boundary->properties(shared)->sprite.imageIndex.bits==savedIndex);
                unchangedDefaults();unchangedBounds();
            }
        }
        assert(random.drawCount()==draws&&boundary->valid());
    }
    PackedRoom room;assert(rooms.valid()&&rooms.findRoom(11,room));
    {
        // Prepared Draw changes only at ownership mutations. Compare the
        // layer adapter against its live oracle after every intermediate
        // state, including inactive cutoffs before activation reconciliation.
        auto layers=std::make_unique<GameplayRoomLayers<16>>(&rooms);
        layers->enablePreparedDraw();assert(layers->prepare());
        RunnerLayerDraw::Element states[16]{};
        unsigned generations[16]{};
        auto publish=[&](unsigned slot){
            assert(layers->publishPreparedDraw({slot,generations[slot]},slot,
                RunnerLayerDraw::action(states[slot])));
        };
        auto compare=[&](){
            std::vector<std::pair<unsigned,std::uint32_t>> expected,actual;
            assert(layers->visit([&](InstanceHandle h,auto& out){
                assert(h.generation==generations[h.slot]);out=states[h.slot];return true;
            },[&](InstanceHandle h,int depth){
                RunnerImageValue value;assert(value.assignInteger(depth));
                expected.push_back({h.slot,value.bits});return true;
            }));
            assert(layers->visitPreparedDraw([&](unsigned slot,RunnerImageValue depth){
                actual.push_back({slot,depth.bits});return true;
            }));
            assert(actual==expected);
            if(!expected.empty()){
                unsigned calls=0;
                assert(!layers->visitPreparedDraw([&](unsigned,auto){++calls;return false;}));
                assert(calls==1);
            }
        };
        auto birth=[&](unsigned slot,int depth,bool projected){
            ++generations[slot];auto& state=states[slot];state={};
            state.type=2;state.instance=true;state.flags=0x10;
            state.object=projected;state.sprite=true;
            RunnerImageValue value;assert(value.assignInteger(depth));
            assert(layers->birth({slot,generations[slot]},0,value,false,projected));
            publish(slot);compare();
        };
        for(unsigned i=0;i<12;++i)birth(i,i<8?-321:-500,i%3!=0);
        for(unsigned i=0;i<12;++i){
            const InstanceHandle h{i,generations[i]};
            states[i].flags|=2;assert(layers->activation(h,false));publish(i);compare();
            assert(layers->reconcile(h));compare();
            states[i].flags&=~2u;assert(layers->activation(h,true));publish(i);compare();
            assert(layers->reconcile(h));compare();
            states[i].sprite=false;publish(i);compare();
            states[i].drawEvent=states[i].object;publish(i);compare();
            states[i].flags|=1;publish(i);compare();
            states[i].flags&=~1u;publish(i);compare();
            assert(layers->writeDepth(h,MotionBinary64::fromInteger(i%2?-600:-200)));
            compare(); // Pending depth requests have not moved any elements.
            assert(layers->serviceDepths());compare();
        }
        for(unsigned i=0;i<12;++i){
            assert(layers->retire({i,generations[i]}));compare();
            birth(i,-321,i%2==0); // Element/identity reuse, different eligibility.
        }
        for(unsigned i=0;i<12;++i)assert(layers->retire({i,generations[i]}));
        compare();assert(layers->prepare());compare();
    }
    {
        // Cached retained scenery must have the same live termination behavior
        // as the uncached instance lookup, before and after relinking/reuse.
        auto cached=std::make_unique<GameplayRoomLayers<8>>(&rooms);
        auto oracle=std::make_unique<GameplayRoomLayers<8>>(&rooms);
        assert(cached->prepare()&&oracle->prepare());
        RunnerImageValue depth;assert(depth.assign(MotionBinary64::fromInteger(-321)));
        bool inactive=false;unsigned terrainGeneration=1;
        auto birth=[&](InstanceHandle h){
            assert(cached->birth(h,0,depth,false,h.slot!=0));
            assert(oracle->birth(h,0,depth,false,true));
        };
        auto compare=[&](){
            std::vector<unsigned> actual,expected;unsigned cachedTerrain=0,oracleTerrain=0;
            auto walk=[&](auto& layers,auto& order,unsigned& terrainCalls){
                return layers.visit([&](InstanceHandle h,RunnerLayerDraw::Element& out){
                    out.instance=true;out.flags=0x10;out.object=h.slot!=0;out.drawEvent=true;
                    if(h.slot==0){++terrainCalls;assert(h.generation==terrainGeneration);if(inactive)out.flags|=2;}
                    return true;
                },[&](InstanceHandle h,int){order.push_back(h.slot);return true;});
            };
            assert(walk(*cached,actual,cachedTerrain)&&walk(*oracle,expected,oracleTerrain));
            assert(actual==expected&&cachedTerrain==0);
            return actual;
        };
        birth({1,1});birth({0,1});birth({2,1});
        assert((compare()==std::vector<unsigned>{2,1}));
        inactive=true;assert(cached->activation({0,1},false)&&oracle->activation({0,1},false));
        assert((compare()==std::vector<unsigned>{2}));
        assert(cached->reconcile({0,1})&&oracle->reconcile({0,1}));
        assert((compare()==std::vector<unsigned>{2,1}));
        inactive=false;assert(cached->activation({0,1},true)&&oracle->activation({0,1},true));compare();
        assert(cached->reconcile({0,1})&&oracle->reconcile({0,1}));compare();
        assert(cached->writeDepth({0,1},MotionBinary64::fromInteger(-400))&&
            oracle->writeDepth({0,1},MotionBinary64::fromInteger(-400)));
        assert(cached->serviceDepths()&&oracle->serviceDepths());compare();
        assert(cached->retire({0,1})&&oracle->retire({0,1}));compare();
        terrainGeneration=2;birth({0,2});compare();
        inactive=true;assert(cached->activation({0,2},false)&&oracle->activation({0,2},false));compare();
        for(auto h:{InstanceHandle{0,2},InstanceHandle{1,1},InstanceHandle{2,1}})
            assert(cached->retire(h)&&oracle->retire(h));
        assert(cached->prepare()&&oracle->prepare());compare();
    }
    for(unsigned removed=0;removed<8;++removed)for(unsigned mode=0;mode<4;++mode){
        // Interior retirement repairs only the preceding skipped run. Cover
        // actor/scenery/head/tail removal, inactive termination, cold/warm
        // projections and reuse of the freed element by a different kind.
        auto cached=std::make_unique<GameplayRoomLayers<8>>(&rooms);
        auto oracle=std::make_unique<GameplayRoomLayers<8>>(&rooms);
        assert(cached->prepare()&&oracle->prepare());
        RunnerImageValue depth;assert(depth.assignInteger(-321));
        bool scenery[8]={false,false,true,true,false,true,true,false};
        bool live[8]={true,true,true,true,true,true,true,true};
        bool inactive[8]={false,false,bool(mode&2),false,false,false,false,false};
        unsigned generation[8]={1,1,1,1,1,1,1,1};
        auto birth=[&](unsigned slot){
            assert(cached->birth({slot,generation[slot]},0,depth,inactive[slot],!scenery[slot]));
            assert(oracle->birth({slot,generation[slot]},0,depth,inactive[slot],true));
        };
        std::vector<unsigned> rawOrder;
        for(unsigned slot=0;slot<8;++slot){birth(slot);rawOrder.insert(rawOrder.begin(),slot);}
        auto compare=[&](){
            std::vector<unsigned> expected;
            for(auto slot:rawOrder)if(live[slot]){
                if(inactive[slot])break;
                if(!scenery[slot])expected.push_back(slot);
            }
            for(unsigned side=0;side<2;++side){
                std::vector<unsigned> actual;
                assert((side?oracle:cached)->visit([&](InstanceHandle h,RunnerLayerDraw::Element& out){
                    assert(live[h.slot]&&h.generation==generation[h.slot]);
                    assert(side||!scenery[h.slot]);
                    out.instance=true;out.flags=0x10|(inactive[h.slot]?2:0);
                    out.object=!scenery[h.slot];out.drawEvent=true;return true;
                },[&](InstanceHandle h,int){actual.push_back(h.slot);return true;}));
                assert(actual==expected);
            }
        };
        if(mode&1)compare();
        assert(cached->retire({removed,1})&&oracle->retire({removed,1}));live[removed]=false;
        compare();
        rawOrder.erase(std::find(rawOrder.begin(),rawOrder.end(),removed));
        scenery[removed]=!scenery[removed];inactive[removed]=false;
        generation[removed]=2;live[removed]=true;birth(removed);
        rawOrder.insert(rawOrder.begin(),removed);compare();
        for(auto slot:rawOrder){
            assert(cached->retire({slot,generation[slot]})&&oracle->retire({slot,generation[slot]}));
            live[slot]=false;compare();
        }
        assert(cached->prepare()&&oracle->prepare());compare();
    }
    for(unsigned mode=0;mode<5;++mode){
        // Warm skip links, then change a later scenery element or insert at
        // the head during Draw. The uncached/all-projected owner walks every
        // raw element and is the ordering oracle for this optimization.
        auto cached=std::make_unique<GameplayRoomLayers<8>>(&rooms);
        auto oracle=std::make_unique<GameplayRoomLayers<8>>(&rooms);
        assert(cached->prepare()&&oracle->prepare());
        RunnerImageValue depth;assert(depth.assign(MotionBinary64::fromInteger(-321)));
        bool inactive[2]={mode==1,mode==1};
        for(unsigned slot:{1u,3u,0u,2u}){
            const bool scenery=slot==0||slot==3;
            assert(cached->birth({slot,1},0,depth,false,!scenery));
            assert(oracle->birth({slot,1},0,depth,false,true));
        }
        for(unsigned side=0;side<2;++side)if(inactive[side])
            assert((side?oracle:cached)->activation({3,1},false));
        auto draw=[&](unsigned side,bool mutate){
            auto& layers=*(side?oracle:cached);std::vector<unsigned> trace;
            assert(layers.visit([&](InstanceHandle h,RunnerLayerDraw::Element& out){
                out.instance=true;out.flags=0x10;out.object=h.slot!=0&&h.slot!=3;out.drawEvent=true;
                if(h.slot==4&&mode>=3){out.object=false;if(mode==4)out.flags|=2;}
                if(h.slot==3&&inactive[side])out.flags|=2;
                return true;
            },[&](InstanceHandle h,int){
                trace.push_back(h.slot);
                if(mutate&&h.slot==2){
                    if(mode>=2)assert(layers.birth({4,1},0,depth,mode==4,side||mode==2));
                    else{inactive[side]=mode==0;assert(layers.activation({3,1},!inactive[side]));}
                }
                return true;
            }));return trace;
        };
        assert(draw(0,false)==draw(1,false));
        const auto actual=draw(0,true),expected=draw(1,true);
        assert(actual==expected&&actual==(mode==0?std::vector<unsigned>{2}:std::vector<unsigned>{2,1}));
        assert(draw(0,false)==draw(1,false));
        if(mode>=2){
            assert(cached->retire({4,1})&&oracle->retire({4,1}));
            assert(draw(0,false)==draw(1,false));
        }
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms);
        assert(owner->attach(*world));auto& entries=owner->entryDithers();
        InstanceHandle key,shared;unsigned reads=0;
        auto camera=[&](MotionBinary64& value){++reads;value=RunnerImageValue{0x43708000u}.promoted();return true;};
        // Explicit fixture ID, not an assertion about the unfinished allocator.
        assert(entries.create(400001,camera,[&](MotionBinary64& value){
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::entryDither,key,shared));
            assert(owner->state(shared)&&owner->state(shared)->x.bits==0x43708000u&&owner->state(shared)->y.bits==0);
            value=world->roomEntryView()->y.promoted();return true;
        },key));
        assert(entries.find(key)->sourceId==400001&&entries.find(key)->effect.alarm0==120&&reads==1);
        assert(owner->properties(shared)->sprite.spriteId==129);
        auto phase=std::make_unique<RunnerStepSnapshot<GameplayCollisionView::capacity>>();
        auto tick=[&](){
            assert(owner->dispatchAlarmPhase(*phase,[&](InstanceHandle h,unsigned alarm,const auto& event){
                if(h.slot!=shared.slot)return true;
                assert(event.codeId==1032&&alarm==0);return entries.advanceAlarm(key,alarm);
            }));
            assert(owner->dispatchStepPhase(0,*phase,[&](InstanceHandle h,const auto& event){
                if(h.slot!=shared.slot)return true;
                assert(event.codeId==1033);return entries.step(key,camera);
            }));
        };
        assert(entries.activate(key,false)&&owner->reconcileActivation());
        tick();assert(reads==1&&entries.find(key)->effect.alarm0==120);
        assert(entries.activate(key,true)&&owner->reconcileActivation());
        for(unsigned frame=1;frame<=120;++frame){
            tick();const auto* entry=entries.find(key);assert(entry);
            assert(entry->effect.alarm0==120-int(frame)&&entry->destroyRequested==(frame==120));
            const unsigned steps=frame<120?frame:119;
            RunnerImageValue expectedY;assert(expectedY.assign(MotionBinary64::fromInteger(16*int(steps))));
            assert(reads==1+steps&&entry->y.bits==expectedY.bits);
            unsigned drawn=0;
            checkReadOnlyDraw(*world);
            assert(world->visitRoomActors([&](auto kind,auto local,auto depth){
                if(kind!=GameplayWorld::DrawActor::entryDither)return true;
                ++drawn;std::int32_t d;assert(depth.promoted().roundPixel(d)&&d==-200000);
                return local.slot==key.slot&&local.generation==key.generation;
            }));
            assert(drawn==(frame==120?0u:1u)); // No offscreen-origin culling.
        }
        assert(entries.size()==1&&owner->prepareDraw()&&entries.size()==0&&!owner->state(shared));
        const auto old=key;
        assert(entries.create(400002,camera,[](auto& v){v={};return true;},key));
        assert(key.slot==old.slot&&key.generation>old.generation&&!entries.find(old));
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::gameplay);
        assert(entries.size()==0&&owner->valid()&&world->entryDithers()==&entries);
        const auto previous=key;
        assert(entries.create(400003,camera,[](auto& v){v={};return true;},key));
        assert(key.generation>previous.generation&&entries.clear()&&owner->valid());
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms);
        assert(owner->attach(*world));auto& entries=owner->entryDithers();InstanceHandle key,shared;
        // Birth allocation is still explicit; all subsequent scheduling is the
        // ordinary gameplay path, with no manual alarm or Step dispatch.
        assert(entries.create(410001,[](auto& x){x=MotionBinary64::fromInteger(240);return true;},
            [&](auto& y){y=world->roomEntryView()->y.promoted();return true;},key));
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::entryDither,key,shared));
        assert(entries.activate(key,false)&&owner->reconcileActivation());
        world->step(0);
        assert(world->effectsSucceeded()&&entries.find(key)->effect.alarm0==120&&entries.find(key)->y.bits==0);
        assert(entries.activate(key,true)&&owner->reconcileActivation());
        for(unsigned tick=1;tick<=120;++tick){
            world->step(0);assert(world->effectsSucceeded()&&owner->valid());
            const auto* r=entries.find(key);assert(r);
            std::int32_t y;assert(r->y.promoted().roundPixel(y));
            assert(y==16*int(tick<120?tick:119)&&r->effect.alarm0==120-int(tick)&&
                r->destroyRequested==(tick==120)&&r->x.bits==0x43700000u);
            assert(owner->state(shared)->x.bits==r->x.bits&&owner->state(shared)->y.bits==r->y.bits);
        }
        assert(owner->prepareDraw()&&!entries.find(key)&&!owner->state(shared));
    }
    for(unsigned failure=0;failure<3;++failure){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms);
        assert(owner->attach(*world));auto& entries=owner->entryDithers();InstanceHandle key;
        unsigned cameras=0,views=0;
        auto camera=[&](MotionBinary64& v){++cameras;v=MotionBinary64::fromInteger(240);return failure!=1;};
        auto view=[&](MotionBinary64& v){++views;v={};return false;};
        assert(!entries.create(failure==0?0:400001,camera,view,key)&&!entries.valid());
        assert(cameras==(failure==0?0u:1u)&&views==(failure==2?1u:0u));
        assert(entries.size()==(failure==0?0u:1u));
        if(failure){
            const auto* r=entries.find(key);assert(r&&r->effect.alarm0==-1&&r->y.bits==0);
            assert(r->x.bits==(failure==2?0x43700000u:0)); // Accepted setter prefix survives.
        }
        assert(!entries.create(400002,camera,view,key));
        assert(cameras==(failure==0?0u:1u)&&views==(failure==2?1u:0u));
    }
    {
        struct PlayerViewProbe {
            GameplayWorld* world=nullptr;unsigned births=0;bool reject=false;
            bool birth(InstanceHandle,const GameplayWorld::PlayerInstance& player){
                const auto* view=world->roomEntryView();
                assert(player.sourceInstanceId==100855);
                assert(view&&view->enabled&&view->x.bits==0&&view->y.bits==0&&
                    view->width.bits==0x43200000u&&view->height.bits==0x438e0000u);
                assert(world->cameraOffsetX()==0&&world->cameraOffsetY()==0&&world->ringOrigin()==300);
                ++births;return !reject;
            }
            bool retire(InstanceHandle,const GameplayWorld::PlayerInstance&){return true;}
            bool activation(InstanceHandle,const GameplayWorld::PlayerInstance&,bool){return true;}
            bool membership(InstanceHandle,const GameplayWorld::PlayerInstance&,bool){return true;}
        } probe;
        // Supply the address before construction so the actual birth callback
        // reads the already-constructed cache, not a post-construction snapshot.
        void* storage=::operator new(sizeof(GameplayWorld));
        probe.world=static_cast<GameplayWorld*>(storage);
        std::unique_ptr<GameplayWorld> world(new(storage) GameplayWorld(300,false,1025,true,
            {},{},GameplayWorld::PlayerLifecycleBinding::bind(probe),nullptr,&rooms));
        assert(world->effectsSucceeded()&&probe.births==1&&world->cameraOffsetY()==5&&world->ringOrigin()==305);
        RoomEntryViews expectedViews;assert(expectedViews.load(rooms,11));
        auto checkViews=[&]{
            for(unsigned slot=0;slot<RoomPack::viewCount;++slot){
                const auto* expected=expectedViews.get(slot);const auto* actual=world->roomEntryView(slot);
                assert(actual&&expected&&actual->x.bits==expected->x.bits&&actual->y.bits==expected->y.bits&&
                    actual->width.bits==expected->width.bits&&actual->height.bits==expected->height.bits&&
                    actual->enabled==expected->enabled);
            }
        };
        unsigned terrainCalls=0;
        assert(world->loadRoomTerrain(rooms,room,[&](const auto&){checkViews();++terrainCalls;return true;}));
        assert(terrainCalls==240);checkViews();
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::gameplay);
        assert(world->effectsSucceeded()&&probe.births==2&&world->roomEntryView()->y.bits==0&&
            world->cameraOffsetY()==5&&world->ringOrigin()==305);
        terrainCalls=0;
        assert(!world->loadRoomTerrain(rooms,room,[&](const auto&){checkViews();return ++terrainCalls<3;}));
        assert(terrainCalls==3&&!world->effectsSucceeded());checkViews();
        assert(!world->loadRoomTerrain(rooms,room,[&](const auto&){++terrainCalls;return true;}));
        assert(terrainCalls==3); // Failed source prefix still cannot be retried.
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::none);
        // Use a fresh healthy world for the remaining failure-path fixture.
        world.reset();probe.world=static_cast<GameplayWorld*>(::operator new(sizeof(GameplayWorld)));
        world.reset(new(probe.world) GameplayWorld(300,false,1025,true,
            {},{},GameplayWorld::PlayerLifecycleBinding::bind(probe),nullptr,&rooms));
        assert(probe.births==3);
        RoomPack invalid(nullptr,0);
        auto rejected=std::make_unique<GameplayWorld>(300,false,1025,true,
            GameplayWorld::EnemyLifecycleBinding{},GameplayWorld::BreakableLifecycleBinding{},
            GameplayWorld::PlayerLifecycleBinding::bind(probe),nullptr,&invalid);
        assert(!rejected->effectsSucceeded()&&!rejected->roomEntryView()&&
            !rejected->playerHandle().generation&&probe.births==3);
        probe.reject=true;
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::none);
        assert(!world->effectsSucceeded()&&probe.births==4&&world->cameraOffsetY()==0&&world->ringOrigin()==300);
        for(bool prematureTick:{false,true}){
            world.reset();probe.births=0;probe.reject=false;
            probe.world=static_cast<GameplayWorld*>(::operator new(sizeof(GameplayWorld)));
            world.reset(new(probe.world) GameplayWorld(300,false,1025,true,
                {},{},GameplayWorld::PlayerLifecycleBinding::bind(probe),nullptr,&rooms,
                GameplayWorld::PlayerBirthTiming::authoredRoom));
            assert(world->effectsSucceeded()&&world->awaitingPlayerBirth()&&!probe.births&&
                !world->playerHandle().generation&&world->cameraOffsetY()==0);
            terrainCalls=0;
            if(prematureTick){
                world->step(0);
                assert(!world->effectsSucceeded()&&!world->tick()&&!probe.births);
            }else{
                assert(world->loadRoomTerrain(rooms,room,[&](const auto&){
                    assert(probe.births==1&&!world->awaitingPlayerBirth());++terrainCalls;return true;
                })&&terrainCalls==240&&world->cameraOffsetY()==5&&world->ringOrigin()==305);
                assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::gameplay);
                assert(world->awaitingPlayerBirth()&&probe.births==1&&!world->playerHandle().generation);
                probe.reject=true;terrainCalls=0;
            }
            assert(!world->loadRoomTerrain(rooms,room,[&](const auto&){++terrainCalls;return true;}));
            assert(!terrainCalls&&probe.births==(prematureTick?0u:2u));
            assert(!world->loadRoomTerrain(rooms,room,[&](const auto&){++terrainCalls;return true;}));
            assert(!terrainCalls&&probe.births==(prematureTick?0u:2u));
        }
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));InstanceHandle effect;
        auto frames=[&](int id,unsigned& count){PackedSpriteCollision art;
            if(!sprites.sprite(id,art))return false;count=art.frameCount;return true;};
        assert(world->spawnScriptEffect(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(300),
            603,{},{},MotionBinary64::fromInteger(-60000),frames,effect));
        InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::scriptEffect,effect,shared));
        RunnerImageValue requested;assert(requested.assign(MotionBinary64::fromInteger(-60000)));
        assert(owner->properties(shared)->depth.bits==requested.bits);
        auto drawDepth=[&](){int depth=123;unsigned seen=0;
            checkReadOnlyDraw(*world);
            assert(world->visitRoomActors([&](auto kind,auto key,auto value){
                if(kind==GameplayWorld::DrawActor::scriptEffect&&key.slot==effect.slot&&key.generation==effect.generation){
                    ++seen;std::int32_t integer;assert(RunnerDepthService::integerDepth(value,integer));depth=integer;
                }return true;
            }));assert(seen==1);return depth;};
        assert(drawDepth()==0); // Setter publication is not immediate relinking.
        world->step(0);
        assert(world->effectsSucceeded()&&drawDepth()==-60000);
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,43,300)!=ResultsDestination::none&&
            !owner->state(shared)&&owner->valid());
    }
    {
        // Reuse is tied to the exact immutable constructor pack and rmMain.
        // Another resource object must publish its own view values instead.
        auto changed=rb;
        unsigned mainIndex=0;
        for(;mainIndex<rooms.roomCount();++mainIndex){
            PackedRoom candidate;assert(rooms.room(mainIndex,candidate));
            if(candidate.roomId==11)break;
        }
        assert(mainIndex<rooms.roomCount());
        const auto xOffset=RoomPack::headerSize+mainIndex*RoomPack::roomRecordSize+44;
        for(unsigned byte=0;byte<4;++byte)changed[xOffset+byte]=std::uint8_t(16777217u>>(byte*8));
        RoomPack replacement(changed.data(),changed.size());
        assert(replacement.valid());
        for(bool missing:{false,true}){
            auto world=std::make_unique<GameplayWorld>(300,true,0,false,
                GameplayWorld::EnemyLifecycleBinding{},GameplayWorld::BreakableLifecycleBinding{},
                GameplayWorld::PlayerLifecycleBinding{},nullptr,&rooms);
            assert(world->effectsSucceeded()&&world->roomEntryView()->x.bits==0);
            auto requested=room;if(missing)requested.roomId=-123;
            unsigned calls=0;
            const auto& source=missing?rooms:replacement;
            const bool loaded=world->loadRoomTerrain(source,requested,[&](const auto&){
                ++calls;assert(world->roomEntryView()->x.bits==0x4b800000u);return true;
            });
            assert(loaded!=missing&&calls==(missing?0u:240u));
            if(missing)assert(!world->roomEntryView()&&!world->effectsSucceeded());
            else assert(world->roomEntryView()->x.bits==0x4b800000u);
        }
    }
    {
        auto roomOwner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
        auto roomWorld=std::make_unique<GameplayWorld>(300,true,0,false,roomOwner->enemyBinding(),
            roomOwner->breakableBinding(),roomOwner->playerBinding(),&roomOwner->terrain());
        assert(roomOwner->attach(*roomWorld));
        const auto oldPlayer=roomWorld->playerHandle();
        assert(roomWorld->findPlayer(oldPlayer)&&roomWorld->findPlayer(oldPlayer)->sourceInstanceId==0);
        unsigned calls=0,codes=0;
        assert(!roomWorld->roomEntryView());
        assert(roomWorld->loadRoomTerrain(rooms,room,[&](const PackedRoomInstance& r){
            const auto* view=roomWorld->roomEntryView();
            assert(view&&view->enabled&&view->x.bits==0&&view->y.bits==0&&
                view->width.bits==0x43200000u&&view->height.bits==0x438e0000u);
            ++calls;codes+=r.creationCodeId!=-1;return true;}));
        assert(calls==240&&codes==3&&roomOwner->terrain().size()==240);
        checkCollisionViewMembership(*roomWorld);
        unsigned authoredPlayers=0;
        for(unsigned i=0;i<room.instanceCount;++i){
            PackedRoomInstance r;assert(rooms.roomInstance(room,i,r));
            if(r.objectId!=35)continue;
            ++authoredPlayers;
            assert(r.instanceId==100855&&roomWorld->findPlayer(oldPlayer)->sourceInstanceId==r.instanceId);
        }
        assert(authoredPlayers==1);
        unsigned ordinal=0;
        for(unsigned i=0;i<room.instanceCount;++i){
            PackedRoomInstance r;assert(rooms.roomInstance(room,i,r));
            if(r.objectId!=60&&r.objectId!=93)continue;
            const auto local=roomOwner->terrain().handle(ordinal++);
            const auto stored=roomOwner->terrain().find(local);InstanceHandle shared;
            assert(stored&&stored->sourceInstanceId==r.instanceId&&stored->creationCodeId==r.creationCodeId&&
                stored->x==r.x&&stored->y==r.y&&stored->objectId==r.objectId&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,shared));
            const auto state=roomOwner->state(shared);std::int32_t x,y;
            assert(state&&state->x.promoted().roundPixel(x)&&state->y.promoted().roundPixel(y)&&x==r.x&&y==r.y);
            assert(stored->spriteId==51&&stored->imageSpeed==0&&roomOwner->properties(shared)->sprite.spriteId==51);
            assert(stored->wall.pillar==(r.creationCodeId==677?1:r.creationCodeId==676||r.creationCodeId==678?-1:0));
            assert(!stored->wall.checked&&!stored->wall.surrounded&&!stored->wall.deactivate&&!stored->wall.lights);
        }
        const auto old=roomOwner->terrain().handle(0);
        assert(roomWorld->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        assert(roomOwner->terrain().size()==0&&!roomOwner->terrain().find(old));
        const auto newPlayer=roomWorld->playerHandle();
        assert(newPlayer.generation>oldPlayer.generation&&!roomWorld->findPlayer(oldPlayer)&&
            roomWorld->findPlayer(newPlayer)->sourceInstanceId==0);
        assert(roomWorld->loadRoomTerrain(rooms,room,[](const auto&){return true;}));
        assert(roomWorld->findPlayer(newPlayer)->sourceInstanceId==100855&&!roomWorld->findPlayer(oldPlayer));
        assert(roomOwner->terrain().size()==240&&roomOwner->terrain().handle(0).generation>old.generation);
        assert(!roomWorld->loadRoomTerrain(rooms,room,[&](const auto&){++calls;return true;})&&calls==240);
        assert(!roomWorld->effectsSucceeded()&&roomOwner->terrain().size()==240);
    }
    {
        auto terrain=std::make_unique<GameplayTerrainInstances>();
        auto partial=std::make_unique<GameplayWorld>(300,true,0,false,
            GameplayWorld::EnemyLifecycleBinding{},GameplayWorld::BreakableLifecycleBinding{},
            GameplayWorld::PlayerLifecycleBinding{},terrain.get());
        unsigned calls=0;
        assert(!partial->loadRoomTerrain(rooms,room,[&](const auto&){return ++calls<3;}));
        assert(calls==3&&terrain->size()==2&&!partial->effectsSucceeded());
        assert(!partial->loadRoomTerrain(rooms,room,[&](const auto&){++calls;return true;})&&calls==3);
    }
    {
        auto terrain=std::make_unique<GameplayTerrainInstances>();PackedRoomInstance r;
        assert(rooms.roomInstance(room,0,r));r.scaleX=2;
        assert(!terrain->appendRoom(r)&&!terrain->valid()&&terrain->size()==0);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        struct ViewProbe {InstanceHandle edge,inside;int previous;unsigned calls=0;} probe;
        probe.previous=world->cameraOffsetY()+34;
        assert(world->createBoneWithSourceId(405194,240,probe.previous,probe.edge)&&
            world->createBoneWithSourceId(405195,240,probe.previous+1,probe.inside));
        world->configureGenerationStep([](void* p,GameplayWorld& w,GameMakerRandom&,int){
            auto& probe=*static_cast<ViewProbe*>(p);bool edge=true,inside=false;++probe.calls;
            for(int offset:{-1,0,1,319,320,321}){
                RunnerImageValue y;bool retire=false;
                if(!y.assignInteger(probe.previous+offset)||!w.timeShardOutsideView(y,retire)||
                   retire!=(offset<0||offset>320))return false;
            }
            for(int offset:{-1,1}){
                auto y=MotionBinary64::fromInteger(probe.previous+(offset>0?320:0));RunnerImageValue published;bool retire=false;
                if(!y.add(MotionBinary64{offset<0?0xbfd00000u:0x3fd00000u,0})||
                   !published.assign(y)||!w.timeShardOutsideView(published,retire)||!retire)return false;
            }
            return w.cameraOffsetY()+34>probe.previous+1&&w.boneInView(probe.edge,edge)&&
                w.boneInView(probe.inside,inside)&&!edge&&inside;
        },&probe);
        world->step(0);
        assert(world->effectsSucceeded()&&probe.calls==1&&owner->bones().clear());
    }
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                       GameplayCollisionOwner::Ownership::nativeScenery})
    for(int initial:{(-2147483647-1),-2,-1,0,1,2}){
        // Compare the bound pending-counter path with the original unfiltered
        // captured traversal. Negative values must remain untouched; zero
        // still retires, and positive counters retain countdown/RNG order.
        const auto* alarmRooms=ownership==GameplayCollisionOwner::Ownership::nativeScenery?&rooms:nullptr;
        auto optimized=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},alarmRooms,ownership);
        auto reference=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},alarmRooms);
        auto a=std::make_unique<GameplayWorld>(300,true,0,false,optimized->enemyBinding(),
            optimized->breakableBinding(),optimized->playerBinding(),&optimized->terrain());
        auto b=std::make_unique<GameplayWorld>(300,true,0,false,reference->enemyBinding(),
            reference->breakableBinding(),reference->playerBinding(),&reference->terrain());
        assert(optimized->attach(*a)&&reference->attach(*b));
        for(auto* world:{a.get(),b.get()}){
            unsigned slot=0;
            for(int object:{112,113,116,117,119}){
                PackedObject metadata;assert(objects.findObject(object,metadata));
                assert(world->configureEnemy(slot,object,metadata.spriteId,int(slot)*16-32,128,
                    0,0,5,14,0,1,initial,object==113&&initial==1?-1:initial,0,0,true,true,true));
                ++slot;
            }
            assert(world->createGeneratedInstance({92,240,512},objects,[](const auto&){return true;}));
            auto* box=const_cast<GameplayWorld::Breakable*>(world->findBreakable({0,1}));
            assert(box);box->alarm0=initial;
        }
        RunnerStepSnapshot<32> snapshot;
        auto unfiltered=[&](){
            return reference->dispatchAlarmPhase(snapshot,[&](InstanceHandle h,unsigned alarm,const auto&){
                GameplayCollisionView::Kind kind;InstanceHandle local;
                if(!GameplayCollisionView::decode(h,kind,local))return false;
                if(kind==GameplayCollisionView::Kind::enemy)return b->advanceEnemyRoomAlarm(local,alarm);
                if(kind==GameplayCollisionView::Kind::breakable)return b->advanceBoxRoomAlarm(local,alarm);
                return true; // Other captured families are not combat alarms.
            });
        };
        b->configureRoomAlarms([](void* p){return (*static_cast<decltype(unfiltered)*>(p))();},&unfiltered);
        for(unsigned tick=0;tick<24;++tick){
            if(tick==8)for(auto* world:{a.get(),b.get()}){
                // A reused capture must observe counters rearmed between ticks.
                auto& enemy=const_cast<GameplayWorld::EnemyState&>(world->enemyAt(0));
                enemy.alarm1=1;
                const_cast<GameplayWorld::Breakable*>(world->findBreakable({0,1}))->alarm0=0;
            }
            a->step(0);b->step(0);
            assert(a->effectsSucceeded()&&b->effectsSucceeded()&&optimized->valid()&&reference->valid());
            assert(a->enemyCount()==b->enemyCount());
            for(unsigned i=0;i<a->enemyCount();++i){
                const auto& x=a->enemyAt(i);const auto& y=b->enemyAt(i);
                assert(x.objectId==y.objectId&&x.lifetimeId==y.lifetimeId&&x.active==y.active&&
                    x.alarm0==y.alarm0&&x.alarm1==y.alarm1&&x.health==y.health&&
                    x.preciseX==y.preciseX&&x.preciseY==y.preciseY&&x.speedX==y.speedX&&x.speedY==y.speedY&&
                    x.motionState==y.motionState&&x.auxiliaryState==y.auxiliaryState&&
                    x.spriteId==y.spriteId&&x.imageFrame==y.imageFrame);
                // Frog Alarm 0 arms the previously negative Alarm 1; that
                // later index must count down immediately, not next frame.
                if(!tick&&initial==1&&x.objectId==113)assert(x.alarm1==59);
            }
            const auto* x=a->findBreakable({0,1});const auto* y=b->findBreakable({0,1});
            assert(x&&y&&x->alarm0==y->alarm0&&x->active==y->active);
            assert(a->randomState().drawCount()==b->randomState().drawCount()&&
                a->randomState().stateIndex()==b->randomState().stateIndex());
            for(unsigned w=0;w<16;++w)assert(a->randomState().stateWord(w)==b->randomState().stateWord(w));
        }
    }
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                       GameplayCollisionOwner::Ownership::nativeScenery}){
        auto ordered=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},
            ownership==GameplayCollisionOwner::Ownership::nativeScenery?&rooms:nullptr,ownership);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,ordered->enemyBinding(),
            ordered->breakableBinding(),ordered->playerBinding(),&ordered->terrain());
        assert(ordered->attach(*scene));
        for(unsigned i=0;i<3;++i)
            assert(scene->configureEnemy(i,112,163,int(i)*32,128,0,0,5,14,0,1,-1,1));
        const auto first=scene->enemyHandle(scene->enemyAt(0).lifetimeId);
        auto& record=const_cast<GameplayWorld::EnemyState&>(*scene->findEnemy(first));
        auto binding=ordered->enemyBinding();
        assert(binding.activation(first,record,false));record.active=false;
        assert(ordered->reconcileActivation());
        assert(binding.activation(first,record,true));record.active=true;
        assert(ordered->reconcileActivation());
        GameMakerRandom expected=scene->randomState();
        const int second=120+int(expected.irandomSmall(180));
        const int third=120+int(expected.irandomSmall(180));
        const int reactivated=120+int(expected.irandomSmall(180));
        scene->step(0);
        assert(scene->effectsSucceeded()&&scene->enemyAt(1).alarm1==second&&
            scene->enemyAt(2).alarm1==third&&scene->findEnemy(first)->alarm1==reactivated);
        // A failed external combat alarm phase is terminal before countdown.
        scene->configureRoomAlarms([](void*){return false;},nullptr);
        const int before=scene->enemyAt(1).alarm1;
        scene->step(0);
        assert(!scene->effectsSucceeded()&&scene->enemyAt(1).alarm1==before);
    }
    {
        auto fieldsOwner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms);
        auto fieldWorld=std::make_unique<GameplayWorld>(300,true,0,false,fieldsOwner->enemyBinding(),
            fieldsOwner->breakableBinding(),fieldsOwner->playerBinding(),&fieldsOwner->terrain(),&rooms);
        assert(fieldsOwner->attach(*fieldWorld));
        const auto draws=fieldWorld->randomDrawCount();
        assert(fieldWorld->createGeneratedInstance({82,240,512},objects,[](const auto&){return false;}));
        const auto identity=fieldWorld->timeFields().identityAt(0);
        InstanceHandle fieldKey;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::timeField,{0,identity},fieldKey));
        GameplayCollisionView::Record record;
        assert(GameplayCollisionView(*fieldWorld).resolve(fieldKey,record)&&record.timeField==fieldWorld->timeFields().at(0));
        checkCollisionViewMembership(*fieldWorld);
        const auto state=fieldsOwner->state(fieldKey);std::int32_t x,y;
        assert(state&&state->x.promoted().roundPixel(x)&&x==240&&state->y.promoted().roundPixel(y)&&y==512);
        unsigned population=0;
        assert(fieldsOwner->population(82,population)&&population==1&&
            fieldsOwner->properties(fieldKey)->sprite.spriteId==597&&fieldWorld->randomDrawCount()==draws);
        assert(fieldWorld->registerLevelExit(900));
        const auto exitLocal=fieldWorld->exitControllers().handle(0);
        InstanceHandle exitKey,setterKey;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::exitController,exitLocal,exitKey)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::exitController,fieldWorld->exitControllers().handle(1),setterKey));
        for(auto key:{exitKey,setterKey}){
            const auto p=fieldsOwner->properties(key);const auto s=fieldsOwner->state(key);
            PackedObject definition;
            assert(objects.findObject(key.slot==exitKey.slot?306:74,definition)&&p&&s&&!(s->flags&0x100003u)&&
                p->sprite.spriteId==definition.spriteId&&p->sprite.maskId==definition.textureMaskId&&
                p->sprite.scaleX.bits==0x3f800000u&&p->sprite.scaleY.bits==0x3f800000u);
        }
        const auto goalLocal=fieldWorld->goalFieldInstances().handle(0);
        InstanceHandle goalKey;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::goalField,goalLocal,goalKey));
        const auto goal=fieldsOwner->state(goalKey);
        const auto goalProperties=fieldsOwner->properties(goalKey);
        assert(goal&&goalProperties&&goal->x.promoted().roundPixel(x)&&x==240&&
            goal->y.promoted().roundPixel(y)&&y==932&&goalProperties->sprite.scaleX.bits==0x40000000u&&
            goalProperties->sprite.scaleY.bits==0x40000000u&&fieldWorld->randomDrawCount()==draws+1);
        assert(fieldWorld->registerLevelExit(910)&&fieldsOwner->population(255,population)&&population==2&&
            fieldsOwner->state(goalKey)&&fieldWorld->randomDrawCount()==draws+2);
        assert(fieldWorld->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        assert(fieldsOwner->valid()&&!fieldsOwner->state(fieldKey)&&
            !GameplayCollisionView(*fieldWorld).resolve(fieldKey,record)&&fieldsOwner->population(82,population)&&population==0);
        assert(!fieldsOwner->state(goalKey)&&fieldsOwner->population(255,population)&&population==0);
        assert(!fieldsOwner->state(exitKey)&&!fieldsOwner->state(setterKey)&&
            fieldsOwner->population(306,population)&&population==0&&fieldsOwner->population(74,population)&&population==0);
        assert(fieldWorld->registerLevelExit(920));
        const auto nextGoal=fieldWorld->goalFieldInstances().handle(0);
        assert(nextGoal.generation>goalLocal.generation&&fieldWorld->goalFieldInstances().find(nextGoal));
        assert(fieldWorld->createGeneratedInstance({82,256,600},objects,[](const auto&){return false;}));
        const auto next=fieldWorld->timeFields().identityAt(0);
        InstanceHandle nextKey;
        assert(next>identity&&GameplayCollisionView::encode(GameplayCollisionView::Kind::timeField,{0,next},nextKey)&&
            fieldsOwner->state(nextKey)&&!fieldsOwner->state(fieldKey));
        assert(!fieldWorld->enterLevelClear(0,false)&&fieldsOwner->population(255,population)&&population==1);
        assert(fieldWorld->registerLevelExit(176+fieldWorld->player().offsetY()));
        for(unsigned i=0;i<100;++i)fieldWorld->step(0);
        assert(fieldWorld->levelResultRequested()&&fieldWorld->enterLevelClear(0,false)&&
            fieldsOwner->population(255,population)&&population==0);
        bool heart=false;
        for(unsigned i=0;i<400&&!fieldWorld->nextLevelRequested();++i){
            bool confirm=true;
            assert(fieldWorld->stepLevelClear(confirm,false,false,heart,[](auto,unsigned){}));
        }
        // Nonpersistent debris must leave with the old room, whether still
        // active or already marked for deferred destruction before Draw.
        InstanceHandle roomDebris,markedDebris,roomDebrisKey,markedDebrisKey;
        assert(fieldWorld->spawnDebris({},{},1,roomDebris));
        assert(fieldWorld->spawnDebris({},{},1,markedDebris));
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,roomDebris,roomDebrisKey));
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,markedDebris,markedDebrisKey));
        for(unsigned tick=0;tick<30;++tick)
            assert(fieldWorld->advanceOwnedDebrisRoomAlarm(markedDebris,fieldWorld->debrisEffects().beginPhase(),0));
        assert(fieldWorld->debrisEffects().retained(markedDebris.generation)->destroyRequested);
        assert(fieldsOwner->state(roomDebrisKey)&&fieldsOwner->state(markedDebrisKey));
        const auto debrisSequence=fieldWorld->debrisEffects().birthSequence();
        const auto transitionDraws=fieldWorld->randomDrawCount();
        int controllerSpeed=60;assert(fieldWorld->bindControllerRoomSpeed(controllerSpeed));
        auto& controllers=fieldsOwner->runControllers();InstanceHandle activeController,inactiveController;
        unsigned controllerCreates=0;
        assert(controllers.create(100065,{}, {},[&](InstanceHandle){++controllerCreates;return true;},activeController)&&
            controllers.create(105999,{}, {},[&](InstanceHandle){++controllerCreates;return true;},inactiveController));
        assert(fieldsOwner->requestMomentDelay()&&controllerSpeed==30);
        assert(controllers.activate(inactiveController,false)&&fieldsOwner->reconcileActivation());
        assert(fieldWorld->prepareNextAreaOneLevel({},300)&&fieldWorld->runStage().level==2);
        assert(fieldWorld->runControllers()==&controllers&&controllerCreates==2&&controllerSpeed==30&&
            controllers.find(activeController)->alarms.get(0)==1&&controllers.find(inactiveController)->alarms.get(0)==1&&
            !controllers.find(inactiveController)->active);
        assert(!fieldWorld->debrisInstances()->size()&&!fieldWorld->debrisEffects().retained(roomDebris.generation)&&
            !fieldWorld->debrisEffects().retained(markedDebris.generation));
        assert(!fieldsOwner->state(roomDebrisKey)&&!fieldsOwner->state(markedDebrisKey)&&
            fieldsOwner->population(370,population)&&population==0);
        assert(fieldWorld->debrisEffects().birthSequence().next==debrisSequence.next&&
            fieldWorld->randomDrawCount()==transitionDraws);
        fieldWorld->step(0);
        assert(fieldWorld->effectsSucceeded()&&controllerSpeed==60&&controllerCreates==2&&
            controllers.find(activeController)->alarms.get(0)==0&&controllers.find(inactiveController)->alarms.get(0)==1);
        assert(controllers.activate(inactiveController,true)&&fieldsOwner->reconcileActivation());
        controllerSpeed=35;fieldWorld->step(0);
        assert(fieldWorld->effectsSucceeded()&&controllerSpeed==60&&controllers.find(inactiveController)->alarms.get(0)==0);
        assert(controllers.clear());
        InstanceHandle nextDebris,nextDebrisKey;
        assert(fieldWorld->spawnDebris({},{},1,nextDebris)&&nextDebris.generation>markedDebris.generation);
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,nextDebris,nextDebrisKey));
        assert(fieldsOwner->state(nextDebrisKey)&&!fieldsOwner->state(roomDebrisKey)&&!fieldsOwner->state(markedDebrisKey));
        const auto startDraws=fieldWorld->randomDrawCount();
        assert(fieldWorld->createRoomStartField()&&fieldWorld->startTimeField()&&
            fieldsOwner->population(371,population)&&population==1&&fieldWorld->randomDrawCount()==startDraws+1);
        const auto startLocal=fieldWorld->goalFieldInstances().handle(0);
        assert(fieldWorld->goalFieldInstances().find(startLocal)->sourceId==101097);
        checkCollisionViewMembership(*fieldWorld);
        InstanceHandle startKey;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::goalField,startLocal,startKey));
        assert(fieldsOwner->state(startKey)&&fieldsOwner->properties(startKey)->sprite.scaleX.bits==0x40000000u&&
            fieldsOwner->properties(startKey)->sprite.scaleY.bits==0x40000000u);
        assert(fieldWorld->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        fieldWorld->confirmFirstStage();
        const auto firstIdentity=fieldWorld->goalFieldInstances().birthSequence();
        assert(fieldWorld->createRoomStartField()&&!fieldWorld->startTimeField()&&!fieldsOwner->state(startKey)&&
            fieldsOwner->population(371,population)&&population==0&&
            !fieldWorld->goalFieldInstances().find({0,firstIdentity})&&fieldsOwner->valid());
    }
    for(bool rejectCreate:{false,true}){
        struct RejectedBuilder {
            unsigned births=0;
            bool rejectBirth=true;
            bool birth(InstanceHandle,const GameplayBuilderController::Record& r){
                ++births;assert(r.sourceId==100856);return !rejectBirth;
            }
            bool retire(InstanceHandle,const GameplayBuilderController::Record&){return true;}
            bool activation(InstanceHandle,const GameplayBuilderController::Record&,bool){return true;}
            bool membership(InstanceHandle,const GameplayBuilderController::Record&,bool){return true;}
        } rejected;
        rejected.rejectBirth=!rejectCreate;
        GameplayBuilderController builder(GameplayBuilderController::Binding::bind(rejected));
        auto world=std::make_unique<GameplayWorld>(300,false,1025,true,
            GameplayWorld::EnemyLifecycleBinding{},GameplayWorld::BreakableLifecycleBinding{},
            GameplayWorld::PlayerLifecycleBinding{},nullptr,&rooms,GameplayWorld::PlayerBirthTiming::authoredRoom);
        assert(world->bindBuilderController(builder));
        unsigned walls=0,creates=0;const auto draws=world->randomDrawCount();
        assert(!world->loadRoomTerrainWithStartField(rooms,room,[&](const auto&){++walls;return true;},nullptr,
            [&](const auto&){++creates;assert(builder.handle().generation);return false;}));
        assert(creates==unsigned(rejectCreate));
        assert(rejected.births==1&&!walls&&!world->effectsSucceeded()&&world->awaitingPlayerBirth()&&
            !world->playerHandle().generation&&world->randomDrawCount()==draws);
        assert(!world->loadRoomTerrainWithStartField(rooms,room,[&](const auto&){++walls;return true;}));
        assert(rejected.births==1&&!walls);
    }
    for(auto timing:{GameplayWorld::PlayerBirthTiming::construction,GameplayWorld::PlayerBirthTiming::authoredRoom})
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                       GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,ownership);
        const bool native=ownership!=GameplayCollisionOwner::Ownership::reference;
        auto nativeTerrain=std::make_unique<GameplayWorld::NativeTerrain>();
        auto world=std::make_unique<GameplayWorld>(300,false,1201-176,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),native?nullptr:&owner->terrain(),&rooms,timing);
        assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*nativeTerrain)));
        unsigned priorGeneration=0,priorDitherGeneration=0,priorPlayerGeneration=0,priorBuilderGeneration=0;
        for(unsigned retry=0;retry<2;++retry){
            const bool deferred=timing==GameplayWorld::PlayerBirthTiming::authoredRoom;
            assert(world->builderController()&&!world->builderController()->handle().generation);
            assert(world->awaitingPlayerBirth()==deferred);
            if(deferred)assert(!world->playerHandle().generation&&world->cameraOffsetY()==0);
            world->confirmFirstStage();
            const auto generation=world->goalFieldInstances().birthSequence();
            const auto draws=world->randomDrawCount();
            assert(generation>priorGeneration);
            PackedRoom room;assert(rooms.findRoom(11,room));
            PreparedRoomTerrain plan;assert(plan.prepare(rooms,room));
            struct Builder : AreaOneBuilder {
                void resetRoom(int x,int y,GameMakerRandom& random,const Setup& setup){reset(x,y,random,setup);}
            } liveBuilder;
            AreaOneBuilder expectedBuilder;
            auto expectedRandom=world->randomState();
            expectedBuilder.reset(-16,704,expectedRandom);
            unsigned walls=0,creates=0;
            assert(world->loadRoomTerrainWithStartField(rooms,room,[&](const auto& instance){
                assert(creates==1);
                assert(!world->awaitingPlayerBirth());
                const auto player=world->playerHandle();
                assert(player.generation>priorPlayerGeneration&&world->findPlayer(player)->sourceInstanceId==100855);
                const auto above=world->ditherAbove()->handle();
                const auto* dither=world->ditherAbove()->find(above);
                assert(dither&&dither->sourceId==100854&&above.generation>priorDitherGeneration);
                const auto* field=world->goalFieldInstances().find({0,generation});
                assert(bool(field)==(walls>=144));
                if(field)assert(field->sourceId==101097&&field->destroyRequested);
                if(walls==144){
                    assert((native?nativeTerrain->size():world->terrainInstances()->size())==144);
                }
                // The native registration projection intentionally excludes
                // retained terrain; use the complete reference walk here.
                if(walls==144&&ownership==GameplayCollisionOwner::Ownership::reference){
                    unsigned precedingWalls=0;bool found=false,aboveFound=false;
                    assert(owner->visitRegistrationOrder([&](auto,int object,auto){
                        if(object==61){assert(precedingWalls==0&&!aboveFound);aboveFound=true;}
                        if(object==371){assert(precedingWalls==144);found=true;}
                        if(object==60){assert(!found&&aboveFound);++precedingWalls;}
                        return true;
                    })&&found&&aboveFound);
                }
                ++walls;return !native||nativeTerrain->append(instance.objectId,instance.x,instance.y,true,
                    instance.instanceId,instance.creationCodeId);
            },retry?&plan:nullptr,[&](const auto& instance){
                ++creates;
                assert(instance.objectId==65&&instance.instanceId==100856&&!walls&&
                    world->builderController()->handle().generation&&world->awaitingPlayerBirth()==deferred&&
                    world->randomDrawCount()==draws);
                return world->resetAreaOneBuilder(liveBuilder,instance.x,instance.y);
            })&&walls==240&&creates==1);
            expectedRandom.nextU32(); // Entry field Create, after builder and player.
            assert(world->randomDrawCount()==expectedRandom.drawCount()&&
                world->randomState().stateIndex()==expectedRandom.stateIndex());
            for(unsigned i=0;i<16;++i)assert(world->randomState().stateWord(i)==expectedRandom.stateWord(i));
            const auto builderLocal=world->builderController()->handle();
            assert(builderLocal.generation>priorBuilderGeneration);
            const auto* builder=world->builderController()->find(builderLocal);
            assert(builder&&builder->sourceId==100856&&builder->x.bits==0xc1800000u&&builder->y.bits==0x44300000u);
            InstanceHandle builderKey;
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::builder,builderLocal,builderKey));
            checkTypedCollisionView(GameplayCollisionView(*world),GameplayCollisionView::Kind::builder,builderLocal);
            unsigned firstTwo=0;
            assert(owner->visitRegistrationOrder([&](auto,int object,auto){
                if(object==65||object==35){
                    assert(firstTwo<2&&object==((firstTwo==0)==deferred?65:35));++firstTwo;
                }
                return true;
            })&&firstTwo==2);
            RunnerImageValue bx,by;
            assert(world->publishBuilderPosition(-16,720)&&
                (native?owner->nativePublishedPosition(builderKey,bx,by)&&by.bits==0x44340000u:
                    owner->state(builderKey)&&owner->state(builderKey)->y.bits==0x44340000u)&&
                world->builderController()->find(builderLocal)->y.bits==0x44340000u);
            assert(!world->goalFieldInstances().find({0,generation}));
            InstanceHandle shared;
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::goalField,{0,generation},shared));
            assert(!GameplayCollisionView(*world).contains(shared)&&owner->valid());
            priorGeneration=generation;
            priorPlayerGeneration=world->playerHandle().generation;
            priorDitherGeneration=world->ditherAbove()->handle().generation;
            priorBuilderGeneration=builderLocal.generation;
            assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
            assert(!world->builderController()->find(builderLocal)&&!owner->state(builderKey));
            if(native)assert(!owner->nativePublishedPosition(builderKey,bx,by));
        }
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms);
        auto world=std::make_unique<GameplayWorld>(300,false,1201-176,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms);
        assert(owner->attach(*world));
        auto tape=std::make_unique<GameplayFieldReentryVerification>();
        assert(tape->run(*world,*owner,objects,sprites,300)&&tape->checks==255&&tape->gameSteps==108&&
            tape->clearSteps>0&&tape->clearSteps<400&&tape->firstGeneration<tape->laterGeneration&&
            tape->laterGeneration<tape->retryGeneration);
        assert(tape->preparedResources.objectsChecked==386&&
            tape->preparedResources.alarmsChecked==4632&&tape->preparedResources.spritesChecked==741);
        std::printf("Prepared resources: objects=%u alarm slots=%u sprites=%u\n",
            tape->preparedResources.objectsChecked,tape->preparedResources.alarmsChecked,
            tape->preparedResources.spritesChecked);
        std::printf("Field reentry tape: checks=%u gameplay=%u upgrade=%u generations=%u/%u/%u\n",
            tape->checks,tape->gameSteps,tape->clearSteps,tape->firstGeneration,tape->laterGeneration,tape->retryGeneration);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        auto& drones=owner->droneBullets();InstanceHandle local,shared;
        assert(drones.create(105190,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(176),local)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::drone,local,shared));
        for(unsigned tick=1;tick<=16;++tick){
            RunnerImageValue centre;assert(centre.assign(MotionBinary64::fromInteger(world->cameraOffsetY()+176)));
            assert(drones.dispatch(local,[&](auto&,auto& publish,bool&){return publish.publishPosition({0x43700000u},centre);}));
            world->step(0);
            const auto* record=drones.find(local);
            assert(world->effectsSucceeded()&&record&&record->state.decelerate==(tick==16));
        }
        // Exercise the actual pre-Step room animation dispatch and deferred
        // cleanup, not a direct call to the projectile animation helper.
        assert(drones.dispatch(local,[](auto& state,auto&,bool&){
            state.animation=true;state.imageSpeed={0x3f000000u};
            state.timeline={0x40b00000u};state.imageIndex=state.timeline;return true;
        }));
        world->step(0);
        assert(world->effectsSucceeded()&&owner->prepareDraw()&&!drones.find(local)&&!owner->state(shared));
        InstanceHandle edge;
        assert(drones.create(105193,MotionBinary64::fromInteger(240),
            MotionBinary64::fromInteger(world->cameraOffsetY()+34+316),edge));
        world->step(0);
        assert(world->effectsSucceeded()&&drones.find(edge)&&!drones.find(edge)->destroyRequested);
        RunnerImageValue below;assert(below.assign(MotionBinary64::fromInteger(world->cameraOffsetY()+34+317)));
        assert(drones.dispatch(edge,[&](auto&,auto& publish,bool&){return publish.publishPosition({0x43700000u},below);}));
        world->step(0);
        assert(world->effectsSucceeded()&&owner->prepareDraw()&&!drones.find(edge));
        InstanceHandle retained;
        assert(drones.create(105191,{},{},retained));
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        assert(world->droneBullets()==&drones&&!drones.find(retained)&&drones.size()==0&&owner->valid());
        InstanceHandle replacement;
        assert(drones.create(105192,{},{},replacement)&&replacement.generation>retained.generation&&drones.clear());
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        InstanceHandle drone,first,second,newDrone,balloon;
        assert(owner->droneBullets().create(605000,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(192),drone));
        assert(world->createSkeletonWithSourceId(605001,240,192,first));
        assert(world->createSkeletonWithSourceId(605002,256,192,second));
        assert(owner->playerControllers().createCompanion(256,605004,{},{},balloon));
        std::vector<int> objectsSeen;
        assert(owner->dispatchActorEndStep([&](InstanceHandle,int object){objectsSeen.push_back(object);return true;}));
        assert((objectsSeen==std::vector<int>{35,120,120,256,266}));
        assert(owner->playerControllers().balloonEndStep(balloon,false));
        unsigned calls=0;
        assert(owner->dispatchActorEndStep([&](InstanceHandle shared,int object){
            if(object==35)return true;
            assert(object==120);GameplayCollisionView::Kind kind;InstanceHandle local;
            assert(GameplayCollisionView::decode(shared,kind,local)&&kind==GameplayCollisionView::Kind::skeleton);
            assert(local.slot==(calls?second:first).slot);
            if(!calls++){
                assert(owner->droneBullets().markDestroyed(drone));
                assert(owner->droneBullets().create(605003,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(192),newDrone));
            }
            return true;
        }));
        assert(calls==2); // Marked lifetime suppressed; new birth waits for next capture.
        objectsSeen.clear();
        assert(owner->dispatchActorEndStep([&](InstanceHandle shared,int object){
            objectsSeen.push_back(object);
            if(object==266){GameplayCollisionView::Kind kind;InstanceHandle local;
                assert(GameplayCollisionView::decode(shared,kind,local)&&local.slot==newDrone.slot&&local.generation==newDrone.generation);}
            return true;
        }));
        assert((objectsSeen==std::vector<int>{35,120,120,266}));
    }
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                  GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,mode);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        InstanceHandle skeleton;
        assert(world->createSkeletonWithSourceId(606000,240,192,skeleton));
        assert(world->createGeneratedInstance({111,240,-10000},objects,[](const auto&){return true;}));
        const auto first=world->enemyHandle(world->enemyAt(0).lifetimeId);
        InstanceHandle newborn;
        std::vector<int> seen;
        assert(owner->dispatchActorEndStep([&](InstanceHandle shared,int object){
            seen.push_back(object);
            if(object==35)return true;
            if(object==111){
                GameplayCollisionView::Kind kind;InstanceHandle local;
                assert(GameplayCollisionView::decode(shared,kind,local)&&kind==GameplayCollisionView::Kind::enemy&&
                    local.slot==first.slot&&local.generation==first.generation);
                assert(world->markEnemyDestroyed(local));
                assert(world->createGeneratedInstance({111,256,-10000},objects,[](const auto&){return true;}));
                newborn=world->enemyHandle(world->enemyAt(world->enemyCount()-1).lifetimeId);
            }else{
                assert(object==120&&world->resolveEnemyLifetime(first)&&world->resolveEnemyLifetime(first)->destroyRequested&&
                    world->resolveEnemyLifetime(newborn));
            }
            return true;
        }));
        assert((seen==std::vector<int>{35,111,120})); // New enemy does not inherit a turn in this phase.
        assert(owner->prepareDraw()&&!world->resolveEnemyLifetime(first)&&world->resolveEnemyLifetime(newborn));
        // Production dispatch, rather than the fixture callback above, now
        // owns the offscreen enemy's mark. Removal must still wait for Draw.
        world->step(0);
        assert(world->effectsSucceeded()&&owner->valid()&&world->resolveEnemyLifetime(newborn)&&
            world->resolveEnemyLifetime(newborn)->destroyRequested&&owner->skeletons().find(skeleton));
        assert(owner->prepareDraw()&&!world->resolveEnemyLifetime(newborn)&&owner->skeletons().find(skeleton));
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,43,300)==ResultsDestination::gameplay);
        assert(world->createGeneratedInstance({111,240,-10000},objects,[](const auto&){return true;}));
        const auto afterRetry=world->enemyHandle(world->enemyAt(0).lifetimeId);
        world->step(0);
        assert(world->effectsSucceeded()&&world->resolveEnemyLifetime(afterRetry)&&
            world->resolveEnemyLifetime(afterRetry)->destroyRequested);
        assert(owner->prepareDraw()&&!world->resolveEnemyLifetime(afterRetry));
    }
    {
        std::vector<std::uint32_t> expected;
        for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                      GameplayCollisionOwner::Ownership::nativeScenery}){
            const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,mode);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),
                owner->playerBinding(),native?nullptr:&owner->terrain(),&rooms);
            auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();GameMakerRandom random;
            assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain)));
            std::vector<std::uint32_t> actual;
            const auto probe=[&](InstanceHandle self){
                for(int object:{60,93})for(int dx:{-33,-8,0,8,33})for(int dy:{-17,0,17})
                    for(bool reverse:{false,true})for(bool notMe:{false,true}){
                        if(notMe&&!self.generation)continue;
                        auto x1=MotionBinary64::fromInteger(240+dx),y1=MotionBinary64::fromInteger(160+dy);
                        auto x2=MotionBinary64::fromInteger(240-dx),y2=MotionBinary64::fromInteger(160-dy);
                        assert(x1.add({0x3fd00000u,0})&&y1.add({0x3fe00000u,0}));
                        GameplayCollisionOwner::Owner::Result result;
                        assert(owner->terrainNonPreciseLine(self,object,reverse?x2:x1,reverse?y2:y1,
                            reverse?x1:x2,reverse?y1:y2,notMe,result));
                        actual.insert(actual.end(),{unsigned(result.found),result.handle.slot,result.handle.generation});
                        assert(owner->terrainNonPreciseLine(self,object,x1,y1,x1,y1,notMe,result));
                        actual.insert(actual.end(),{unsigned(result.found),result.handle.slot,result.handle.generation});
                        for(bool precise:{false,true}){
                            assert(owner->terrainCollisionPoint(self,object,x1,y1,precise,notMe,masks,result));
                            actual.insert(actual.end(),{unsigned(result.found),result.handle.slot,result.handle.generation});
                        }
                    }
            };
            probe({});InstanceHandle first;
            for(unsigned i=0;i<41;++i){
                const int object=i==40?93:60,x=240+int(i%8)*16,y=160+int(i/8)*16;
                InstanceHandle local;
                if(native){assert(terrain->append(object,x,y));local=terrain->handle(i);assert(terrain->createProperties(local,51));}
                else {assert(owner->terrain().append(object,x,y,true));local=owner->terrain().handle(i);
                    assert(owner->terrain().createProperties(local,51,false,random));}
                if(!i){assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,first));probe(first);}
                if(i==1){
                    bool movedHit=false;
                    assert(owner->terrainPlaceMeeting(local,60,MotionBinary64::fromInteger(272),
                        MotionBinary64::fromInteger(160),masks,movedHit));
                    GameplayCollisionOwner::Owner::Result point;
                    assert(owner->terrainCollisionPoint({},60,MotionBinary64::fromInteger(240),
                        MotionBinary64::fromInteger(160),true,false,masks,point)&&point.found&&point.handle.slot==first.slot);
                    // First-hit point selection must not flush this later,
                    // unvisited wall's restored-position dirty bounds.
                    std::uint32_t flags;
                    if(native)assert(terrain->spatialFlags(local,flags));
                    else {InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,shared));
                        const auto state=owner->state(shared);assert(state);flags=state->flags;}
                    assert(flags&8u);
                }
            }
            probe(first);const InstanceHandle local{first.slot-GameplayCollisionView::terrainBase,first.generation};
            assert(native?world->activateNativeTerrain(local,false):owner->terrain().activation(local,false));
            assert(owner->reconcileActivation());probe(first);
            assert(native?world->activateNativeTerrain(local,true):owner->terrain().activation(local,true));
            assert(owner->reconcileActivation());probe(first);
            if(!native)expected=actual;else assert(actual==expected);
        }
    }
    std::vector<bool> bloodContactReference;
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                       GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,ownership);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world)&&world->createRoomStartField()&&world->createGeneratedInstance({82,240,512},objects,[](const auto&){return true;})&&
            world->registerLevelExit(1024));
        std::vector<bool> contacts;unsigned id=705000;
        for(int y:{192,512,1056})for(int angle:{0,14,180})
            for(int dx:{-130,-128,-126,-26,-24,-22,0,22,24,26,126,128,130}){
                MotionBinary64 x=MotionBinary64::fromInteger(240+dx),py=MotionBinary64::fromInteger(y);
                assert(x.add({0x3fd00000u,0})&&py.add({0x3fe00000u,0}));
                InstanceHandle key,shared;
                assert(world->createBloodWithSourceId(id++,x,py,MotionBinary64::fromInteger(angle),[](auto){return true;},key)&&
                    GameplayCollisionView::encode(GameplayCollisionView::Kind::blood,key,shared));
                // Suppress right-side escape; the source's left-side rule still applies.
                bool allowed=false;
                assert(owner->bloodTimeAllowed(key,true,10000,masks,allowed));contacts.push_back(allowed);
                const auto* blood=owner->blood().find(key);assert(blood);
                if(ownership!=GameplayCollisionOwner::Ownership::reference){
                    RunnerImageValue restoredX,restoredY;
                    assert(owner->nativePublishedPosition(shared,restoredX,restoredY)&&
                        restoredX.bits==blood->x.bits&&restoredY.bits==blood->y.bits);
                }else{
                    const auto restored=owner->state(shared);
                    assert(restored&&restored->x.bits==blood->x.bits&&restored->y.bits==blood->y.bits);
                }
                assert(owner->stepBlood(shared,true,10000,masks)&&
                    owner->blood().find(key)->state.imageSpeed.bits==(allowed?0x3f000000u:0u));
                assert(owner->blood().clear());
            }
        assert(std::count(contacts.begin(),contacts.end(),true)>0&&std::count(contacts.begin(),contacts.end(),false)>0);
        if(ownership==GameplayCollisionOwner::Ownership::reference)bloodContactReference=contacts;
        else assert(contacts==bloodContactReference);
    }
    std::vector<bool> currencyContacts;
    std::vector<bool> currencyWallContacts;
    std::vector<bool> currencyPlatformContacts;
    std::vector<std::uint32_t> currencyMotionReference;
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                       GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,ownership);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world)&&world->createRoomStartField()&&
            world->createGeneratedInstance({82,240,512},objects,[](const auto&){return true;})&&world->registerLevelExit(1024));
        struct Queries {
            bool wall(InstanceHandle,MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
            bool tile(InstanceHandle,int,MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
        } queries;
        TerrainWallContacts<16> walls;
        for(int x:{224,240,256}){
            assert(world->createGeneratedInstance({60,x,640},objects,[](const auto&){return true;})&&walls.append(60,x,640));
            assert(world->createGeneratedInstance({93,x,640},objects,[](const auto&){return true;}));
        }
        world->configureSolidResolver(nullptr,&walls,nullptr,nullptr,[](void* context,const ContactBounds& bounds,unsigned& count){
            count=0;return static_cast<TerrainWallContacts<16>*>(context)->visit(bounds,
                [](int,int,int){return true;},[&](){++count;return true;});
        },[](void*,const ContactBounds& bounds,bool& hit){
            hit=false;for(int x:{224,240,256})hit|=contactBoundsOverlap(bounds,{x-8,x+8,632,634});return true;
        });
        std::vector<bool> wallHits;
        std::vector<bool> platformHits;
        for(int size:{0,1}){
            InstanceHandle key;assert(world->createCurrencyWithSourceId(804900+size,
                MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(600),queries,key));
            assert(owner->currency().dispatch(key,[&](auto& state,bool&){state.size=size;return true;})&&
                owner->currency().draw(key,[](auto,const auto&){return true;}));
            for(int dx=-32;dx<=32;++dx)for(int dy=-16;dy<=16;++dy)for(int quarter:{0,1,2,3}){
                auto x=MotionBinary64::fromInteger(240+dx),y=MotionBinary64::fromInteger(640+dy);
                const MotionBinary64 fractions[]={{},{0x3fd00000u,0},{0x3fe00000u,0},{0x3fe80000u,0}};
                assert(x.add(fractions[quarter])&&y.add(fractions[quarter]));bool hit=false;
                assert(owner->currencyWallMeeting(key,x,y,masks,hit));wallHits.push_back(hit);
                assert(owner->currencyPlatformMeeting(key,x,y,masks,hit));platformHits.push_back(hit);
            }
            assert(owner->currency().clear());
        }
        if(ownership==GameplayCollisionOwner::Ownership::reference)currencyWallContacts=wallHits;
        else assert(wallHits==currencyWallContacts);
        if(ownership==GameplayCollisionOwner::Ownership::reference)currencyPlatformContacts=platformHits;
        else assert(platformHits==currencyPlatformContacts);
        std::vector<bool> contacts;unsigned id=805000;
        for(int size:{0,1})for(int cy:{192,512,1056})for(int dx=-136;dx<=136;dx+=4)for(int dy:{-25,-5,0,5,25}){
            auto x=MotionBinary64::fromInteger(240+dx),y=MotionBinary64::fromInteger(cy+dy);
            assert(x.add({0x3fd00000u,0})&&y.add({0x3fe00000u,0}));InstanceHandle key;
            assert(world->createCurrencyWithSourceId(id++,x,y,queries,key));bool allowed=false;
            assert(owner->currency().dispatch(key,[&](auto& state,bool&){state.size=size;state.flash=1;return true;}));
            unsigned draws=0;
            assert(owner->currency().draw(key,[&](auto,const auto&){++draws;return true;})&&!draws&&
                owner->currency().find(key)->spriteId==133+size&&owner->currency().find(key)->maskId==133+size);
            assert(owner->currencyTimeAllowed(key,true,10000,masks,allowed));contacts.push_back(allowed);
            assert(owner->currency().dispatch(key,[](auto& state,bool&){state.flash=-1;return true;}));
            assert(owner->currency().draw(key,[&](auto drawKey,const auto& record){
                ++draws;return drawKey.generation==key.generation&&record.spriteId==133+size;
            })&&draws==1);
            assert(owner->currency().clear());
        }
        assert(std::count(contacts.begin(),contacts.end(),true)>0&&std::count(contacts.begin(),contacts.end(),false)>0);
        if(ownership==GameplayCollisionOwner::Ownership::reference)currencyContacts=contacts;
        else assert(contacts==currencyContacts);
    }
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                       GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},
            &rooms,ownership);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        {
            auto& currency=owner->currency();InstanceHandle local,shared;
            struct Queries {
                GameplayWorld& world;unsigned calls=0;
                bool wall(InstanceHandle key,MotionBinary64,MotionBinary64,bool& hit){
                    const auto* r=world.currencyInstances()->find(key);assert(r&&r->sourceId==605079&&r->spriteId==133);
                    InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::currency,key,shared)&&
                        GameplayCollisionView(world).contains(shared));++calls;hit=false;return true;
                }
                bool tile(InstanceHandle,int layer,MotionBinary64,MotionBinary64,bool& hit){assert(layer==100);++calls;hit=false;return true;}
            } queries{*world};
            auto expectedRandom=world->randomState();CurrencyCreate expected;
            struct ReferenceQueries {
                bool wall(MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
                bool tile(MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
                bool publish(RunnerImageValue,RunnerImageValue){return true;}
            } reference;
            const auto x=MotionBinary64::fromInteger(240),y=MotionBinary64::fromInteger(512);
            assert(expected.create(x,y,1,false,expectedRandom,reference));
            assert(world->createCurrencyWithSourceId(605079,x,y,queries,local)&&queries.calls==2&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::currency,local,shared));
            const auto* r=currency.find(local);assert(r&&r->state.xsp.high==expected.xsp.high&&r->state.xsp.low==expected.xsp.low&&
                r->state.ysp.high==expected.ysp.high&&r->state.ysp.low==expected.ysp.low&&r->state.alarms.get(0)==10&&r->state.alarms.get(3)==15);
            assert(world->randomDrawCount()==expectedRandom.drawCount());
            for(unsigned i=0;i<16;++i)assert(world->randomState().stateWord(i)==expectedRandom.stateWord(i));
            checkTypedCollisionView(GameplayCollisionView(*world),GameplayCollisionView::Kind::currency,local);
            // Use the resource-backed global alarm pass: Alarm 0 arms Alarm 1,
            // whose first decrement must happen in this same pass.
            RunnerStepSnapshot<32> alarmPhase;
            auto alarms=[&](){return owner->dispatchAlarmPhase(alarmPhase,[&](InstanceHandle h,unsigned alarm,const auto&){
                GameplayCollisionView::Kind kind;InstanceHandle key;
                assert(GameplayCollisionView::decode(h,kind,key));
                if(kind!=GameplayCollisionView::Kind::currency)return true;
                return currency.advanceAlarm(key,alarm,[](auto&,unsigned,bool&){return false;});
            });};
            for(unsigned tick=1;tick<=15;++tick){
                assert(alarms());const auto& state=currency.find(local)->state;
                assert(state.obtainable==(tick>=10)&&state.suckable==(tick>=15));
                if(tick>=10)assert(state.alarms.get(1)==180-int(tick-9));
            }
            assert(currency.dispatch(local,[](auto& state,bool&){state.alarms.set(1,1);return true;}));
            assert(alarms()&&currency.find(local)->state.disappearing&&currency.find(local)->state.alarms.get(2)==119);
            RunnerImageValue index{};
            for(unsigned tick=0;tick<24;++tick){
                const RunnerImageValue speed=tick<8?RunnerImageValue{0x3f000000u}:tick<16?RunnerImageValue{}:RunnerImageValue{0xbf000000u};
                assert(currency.dispatch(local,[&](auto& state,bool&){state.imageSpeed=speed;return true;}));
                bool crossed=false;assert(SequenceTimeline::advance(index,speed,4,crossed));index.bits&=0x7fffffffu;
                assert(currency.advanceAnimation(local)&&currency.find(local)->imageIndex.bits==index.bits&&!currency.find(local)->destroyRequested);
                if(ownership==GameplayCollisionOwner::Ownership::reference)
                    assert(owner->properties(shared)->sprite.imageIndex.bits==index.bits);
            }
            assert(currency.markDestroyed(local)&&owner->prepareDraw()&&!currency.find(local)&&!GameplayCollisionView(*world).contains(shared));
            InstanceHandle old,replacement;
            assert(world->createCurrencyWithSourceId(605079,x,y,queries,old));
            assert(world->restartFromResults(DeathMenuSequence::Action::Retry,43,300)==ResultsDestination::gameplay&&
                world->currencyInstances()==&currency&&!currency.find(old));
            assert(world->createCurrencyWithSourceId(605079,x,y,queries,replacement)&&
                replacement.slot==old.slot&&replacement.generation!=old.generation);
            assert(currency.dispatch(replacement,[](auto& state,bool&){state.alarms.set(2,1);return true;}));
            unsigned expiryCalls=0;
            assert(!currency.advanceAlarm(replacement,2,[&](auto&,unsigned index,bool&){
                assert(index==2);++expiryCalls;return false;
            })&&expiryCalls==1&&!currency.valid()&&!currency.find(replacement)->destroyRequested);
            assert(currency.clear()&&currency.valid());
        }
        {
            auto animationOwner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto animationWorld=std::make_unique<GameplayWorld>(300,false,1000,false,animationOwner->enemyBinding(),
                animationOwner->breakableBinding(),animationOwner->playerBinding(),&animationOwner->terrain());
            assert(animationOwner->attach(*animationWorld));
            struct Queries {
                bool wall(InstanceHandle,MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
                bool tile(InstanceHandle,int,MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
            } queries;
            InstanceHandle key;
            assert(animationWorld->createCurrencyWithSourceId(605090,MotionBinary64::fromInteger(240),
                MotionBinary64::fromInteger(512),queries,key));
            assert(animationOwner->currency().dispatch(key,[](auto& state,bool&){state.imageSpeed={0x3f000000u};return true;}));
            animationWorld->step(0);
            assert(animationOwner->currency().find(key)->imageIndex.bits==0x3f000000u&&
                !animationWorld->effectsSucceeded()); // Animation runs; still-unbound alarms stop the frame.
        }
        for(unsigned failure=0;failure<3;++failure){
            auto collecting=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto scene=std::make_unique<GameplayWorld>(300,false,1000,false,collecting->enemyBinding(),
                collecting->breakableBinding(),collecting->playerBinding(),&collecting->terrain());
            assert(collecting->attach(*scene));
            struct Queries {
                bool wall(InstanceHandle,MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
                bool tile(InstanceHandle,int,MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
            } queries;
            InstanceHandle key,self,player;
            assert(scene->createCurrencyWithSourceId(605093,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),queries,key)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::currency,key,self)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::player,scene->playerHandle(),player));
            assert(collecting->currency().dispatch(key,[&](auto& state,bool&){state.obtainable=failure!=0;return true;}));
            struct Audio {
                GameplayWorld& world;unsigned failure,plays=0,pitches=0;
                bool isPlaying(int,bool& playing){playing=false;return true;}bool stop(int){return true;}
                bool play(int id,int priority,bool loop,int& voice){
                    assert(id==167&&priority==60&&!loop&&world.overlapSound(8)==167);++plays;voice=900;return failure!=1;
                }
                bool getGain(int,MotionBinary64& gain){gain=MotionBinary64::fromInteger(1);return true;}
                bool setGain(int,MotionBinary64,int){return true;}
                bool setPitch(int voice,MotionBinary64 pitch){assert(voice==900&&pitch.high==0x3ff00000u&&!pitch.low);++pitches;return true;}
            } audio{*scene,failure};
            struct Controls {
                using Handle=InstanceHandle;GameplayWorld& world;InstanceHandle key;Audio& audio;unsigned& resets;unsigned& effects;
                bool achievement(int){return false;}bool timerStart(std::uint32_t& timer){timer=300;return true;}
                bool lessResist(){return false;}bool resetControllers(){++resets;return true;}
                bool gemPop(){return false;}bool gemPowered(){return false;}
                bool playerPosition(MotionBinary64&,MotionBinary64&){return false;}
                bool effect(MotionBinary64,MotionBinary64,int,MotionBinary64,int,int){return false;}
                bool create(int,MotionBinary64,MotionBinary64,Handle&){return false;}bool myBullet(Handle){return false;}
                bool bulletDirection(MotionBinary64){return false;}bool bulletSprite(int){return false;}
                bool bulletDamage(int){return false;}bool bulletSpeed(int){return false;}bool sound(int,int,bool,bool){return false;}
                bool soundOverlap(int id,int priority,bool loop,bool cut,const char* channel){
                    assert(id==167&&priority==60&&!loop&&cut&&std::strcmp(channel,"UI")==0);
                    return world.playCurrencyPickupSound(key,MotionBinary64::fromInteger(1),audio);
                }
                bool fxNoLoop(int sprite,MotionBinary64 speed){
                    assert(sprite==105&&speed.high==0x3fe99999u&&speed.low==0x9999999au);++effects;return false;
                }
                bool recharge(){return false;}bool destroy(){return false;}
            };
            unsigned resets=0,effects=0;auto ops=scene->currencyGainOperations(Controls{*scene,key,audio,resets,effects});
            const auto draws=scene->randomDrawCount();const auto timer=scene->gemStreakTimer();RunnerCollisionEventContext context;
            const bool ok=context.dispatch(self,player,{true,false},[&](auto,auto,int& subtype,bool& found,PackedObjectEvent& event){
                subtype=35;return objects.collisionEvent(248,35,found,event);
            },[&](const auto& event){return collecting->dispatchCurrencyCollision(event,ops);});
            assert(ok==(failure==0)&&scene->effectsSucceeded()==ok&&scene->randomDrawCount()==draws);
            assert(scene->currency()==(failure?2u:0u)&&scene->gameGemTotal()==(failure?2u:0u)&&
                scene->gemStreak()==(failure?2u:0u)&&scene->gemStreakTimer()==(failure?300u:timer));
            const auto* caller=collecting->currency().find(key);
            assert(caller&&!caller->destroyRequested&&resets==(failure?1u:0u)&&audio.plays==(failure?1u:0u)&&
                effects==(failure==2?1u:0u)&&audio.pitches==(failure==2?1u:0u));
        }
        {
            auto moving=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto scene=std::make_unique<GameplayWorld>(300,false,1000,false,moving->enemyBinding(),
                moving->breakableBinding(),moving->playerBinding(),&moving->terrain());
            assert(moving->attach(*scene)&&scene->createGeneratedInstance({92,240,544},objects,[](const auto&){return true;}));
            scene->configureSolidResolver(nullptr,nullptr,nullptr,nullptr,[](void*,const ContactBounds& bounds,unsigned& count){
                count=contactBoundsOverlap(bounds,{232,248,536,552})?1:0;return true;
            },[](void*,const ContactBounds&,bool& hit){hit=false;return true;});
            struct Queries {
                bool wall(InstanceHandle,MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
                bool tile(InstanceHandle,int,MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
            } queries;
            struct Audio {
                unsigned sounds=0,expiry=0;
                bool isPlaying(int,bool& playing){playing=false;return true;}
                bool stop(int){return true;}
                bool play(int id,int priority,bool loop,int& voice){
                    if(priority!=30||loop)return false;
                    if(id==166){++expiry;voice=800;return true;}
                    if(id<168||id>170)return false;voice=700+int(++sounds);return true;
                }
                bool getGain(int,MotionBinary64& gain){gain=MotionBinary64::fromInteger(1);return true;}
                bool setGain(int,MotionBinary64,int){return true;}
                bool setPitch(int,MotionBinary64){return true;}
            } audio;
            InstanceHandle key,shared;
            assert(scene->createCurrencyWithSourceId(605092,MotionBinary64::fromInteger(248),
                MotionBinary64::fromInteger(512),queries,key)&&GameplayCollisionView::encode(GameplayCollisionView::Kind::currency,key,shared));
            assert(moving->currency().dispatch(key,[](auto& s,bool&){s.xsp={};s.ysp=MotionBinary64::fromInteger(2);return true;}));
            CurrencyHomingMath math{portableAtan};bool bounced=false;
            std::vector<std::uint32_t> motion;
            for(unsigned tick=0;tick<32;++tick){
                assert(moving->stepCurrency(shared,false,480,MotionBinary64::fromInteger(999),MotionBinary64::fromInteger(999),
                    masks,math,[](auto& top){top={};return true;},audio,MotionBinary64::fromInteger(1),false));
                const auto* r=moving->currency().find(key);std::int32_t x,y;
                assert(r&&r->state.xx.roundPixel(x)&&r->state.yy.roundPixel(y));
                RunnerImageValue px,py;assert(px.assignInteger(x)&&py.assignInteger(y)&&r->x.bits==px.bits&&r->y.bits==py.bits);
                if(ownership==GameplayCollisionOwner::Ownership::reference)
                    assert(moving->state(shared)&&moving->state(shared)->x.bits==px.bits&&moving->state(shared)->y.bits==py.bits);
                if(r->state.yCollision){assert(r->state.ysp.zero()||(r->state.ysp.high==0xbff00000u&&!r->state.ysp.low));bounced=true;}
                for(auto value:{r->state.xx,r->state.yy,r->state.xsp,r->state.ysp,r->state.direction}){
                    motion.push_back(value.high);motion.push_back(value.low);
                }
                for(unsigned word=0;word<16;++word)motion.push_back(scene->randomState().stateWord(word));
            }
            if(ownership==GameplayCollisionOwner::Ownership::reference)currencyMotionReference=motion;
            else assert(motion==currencyMotionReference);
            assert(bounced&&audio.sounds);
            const auto before=moving->currency().find(key)->state;
            const auto draws=scene->randomDrawCount();
            assert(moving->stepCurrency(shared,true,480,MotionBinary64::fromInteger(999),MotionBinary64::fromInteger(999),
                masks,math,[](auto& top){top={};return true;},audio,MotionBinary64::fromInteger(1),false));
            const auto& frozen=moving->currency().find(key)->state;
            assert(!frozen.imageSpeed.bits&&frozen.obtainable&&frozen.xx.high==before.xx.high&&frozen.xx.low==before.xx.low&&
                frozen.yy.high==before.yy.high&&frozen.yy.low==before.yy.low&&scene->randomDrawCount()==draws);
            for(unsigned i=0;i<4;++i)assert(frozen.alarms.get(i)==before.alarms.get(i)+(before.alarms.get(i)>0));
            assert(moving->currency().dispatch(key,[](auto& s,bool&){s.attracted=true;s.gemSpeed=MotionBinary64::fromInteger(4);
                s.rotationSpeed=MotionBinary64::fromInteger(32);return true;}));
            auto expected=moving->currency().find(key)->state;bool good=true;
            const auto targetX=MotionBinary64::fromInteger(260),targetY=MotionBinary64::fromInteger(500);
            assert(CurrencyHomingSource::run(expected,targetX,targetY,math,good)&&
                moving->stepCurrency(shared,true,480,targetX,targetY,masks,math,[](auto& top){top={};return true;},
                    audio,MotionBinary64::fromInteger(1),false));
            const auto& attracted=moving->currency().find(key)->state;
            assert(attracted.xx.high==expected.xx.high&&attracted.xx.low==expected.xx.low&&
                attracted.yy.high==expected.yy.high&&attracted.yy.low==expected.yy.low);
            unsigned scheduledSteps=0;
            auto events=[&](InstanceHandle current,GameplayWorld::CurrencyPhase phase,unsigned alarm){
                if(phase==GameplayWorld::CurrencyPhase::alarm)
                    return alarm==2&&moving->currency().expire(current,MotionBinary64::fromInteger(1),false,audio);
                ++scheduledSteps;InstanceHandle h;
                return GameplayCollisionView::encode(GameplayCollisionView::Kind::currency,current,h)&&
                    moving->stepCurrency(h,false,480,targetX,targetY,masks,math,[](auto& top){top={};return true;},
                        audio,MotionBinary64::fromInteger(1),false);
            };
            assert(moving->currency().dispatch(key,[](auto& state,bool&){
                state.attracted=false;state.alarms.set(0,1);state.alarms.set(1,-1);return true;
            }));
            assert(scene->withCurrencyEvents(events,[&](){scene->step(0);return scene->effectsSucceeded();})&&
                scheduledSteps==1&&moving->currency().find(key)->state.obtainable&&
                moving->currency().find(key)->state.alarms.get(1)==179);
            assert(moving->currency().dispatch(key,[](auto& state,bool&){state.alarms.set(2,1);return true;}));
            assert(scene->withCurrencyEvents(events,[&](){scene->step(0);return scene->effectsSucceeded();})&&
                audio.expiry==1&&scheduledSteps==1&&moving->currency().find(key)->destroyRequested);
            if(ownership!=GameplayCollisionOwner::Ownership::reference){
                const int unsupported=ownership==GameplayCollisionOwner::Ownership::native?94:100;
                assert(scene->createGeneratedInstance({unsupported,240,512},objects,[](const auto&){return true;})&&
                    !scene->nativeCurrencyQueriesReady());
                const auto draws=scene->randomDrawCount();
                assert(!moving->stepCurrency(shared,false,480,targetX,targetY,masks,math,[](auto& top){top={};return true;},
                    audio,MotionBinary64::fromInteger(1),false)&&scene->randomDrawCount()==draws);
            }else assert(moving->currency().clear());
        }
        {
            auto& blood=owner->blood();InstanceHandle local,shared;unsigned calls=0;
            const auto rng=world->randomDrawCount();
            assert(world->createBloodWithSourceId(605080,MotionBinary64::fromInteger(240),
                MotionBinary64::fromInteger(990),MotionBinary64::fromInteger(14),[&](InstanceHandle key){
                    const auto* child=blood.find(key);++calls;
                    return child&&child->spriteId==-1&&child->state.imageSpeed.bits==0x3f800000u;
                },local)&&calls==1&&world->randomDrawCount()==rng);
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::blood,local,shared)&&
                GameplayCollisionView(*world).contains(shared)&&blood.find(local)->sourceId==605080);
            checkTypedCollisionView(GameplayCollisionView(*world),GameplayCollisionView::Kind::blood,local);
            assert(world->stepBloodRoomEffect(local,[](const auto& child,bool& allowed){
                assert(child.state.savedSpeed.bits==0x3f000000u);allowed=false;return true;
            }));
            const auto* child=blood.find(local);assert(child&&!child->state.imageSpeed.bits);
            world->step(0);assert(world->effectsSucceeded()&&owner->valid()&&!child->state.timeline.bits);
            assert(world->stepBloodRoomEffect(local,[](const auto&,bool& allowed){allowed=true;return true;}));
            if(ownership==GameplayCollisionOwner::Ownership::reference){
                assert(owner->properties(shared)->sprite.spriteId==84&&
                    owner->state(shared)->x.bits==child->x.bits&&owner->state(shared)->y.bits==child->y.bits);
            }
            for(unsigned tick=0;tick<18;++tick){world->step(0);assert(world->effectsSucceeded()&&owner->valid());}
            assert(blood.find(local)&&blood.find(local)->destroyRequested&&owner->prepareDraw());
            assert(!blood.find(local)&&!GameplayCollisionView(*world).contains(shared));
            InstanceHandle replacement;
            assert(world->createBloodWithSourceId(605081,{}, {},{},[](auto){return true;},replacement)&&
                replacement.slot==local.slot&&replacement.generation!=local.generation&&blood.clear());
        }
        for(int offset:{-161,-160,3000,3001}){
            auto retired=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto scene=std::make_unique<GameplayWorld>(300,false,0,false,retired->enemyBinding(),
                retired->breakableBinding(),retired->playerBinding(),&retired->terrain());
            assert(retired->attach(*scene));InstanceHandle key;
            assert(scene->createSkeletonWithSourceId(505080,240,192,key));
            RunnerImageValue x,y;assert(x.assignInteger(240)&&y.assignInteger(34+scene->cameraOffsetY()+offset));
            assert(retired->skeletons().dispatchPublished(key,[&](auto&,auto&,auto& publish,bool&){return publish.position(x,y);}));
            scene->step(0);const auto* actor=retired->skeletons().find(key);
            const bool destroy=offset==-161||offset==3001;
            assert(scene->effectsSucceeded()&&actor&&actor->destroyRequested==destroy&&actor->state.health==60);
            assert(retired->prepareDraw()&&bool(retired->skeletons().find(key))==!destroy);
        }
        for(unsigned mode=0;mode<8;++mode){
            auto contacts=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto scene=std::make_unique<GameplayWorld>(300,false,1000,false,contacts->enemyBinding(),
                contacts->breakableBinding(),contacts->playerBinding(),&contacts->terrain());
            assert(contacts->attach(*scene));InstanceHandle local,self,player;
            assert(scene->createSkeletonWithSourceId(505089,240,192,local)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::skeleton,local,self)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::player,scene->playerHandle(),player));
            int otherObject=35;
            if(mode>=6){
                assert(scene->registerLevelExit(192));player={};
                assert(contacts->visitRegistrationOrder([&](InstanceHandle key,int object,auto){
                    bool found=false;PackedObjectEvent binding{};
                    if(!objects.collisionEvent(120,object,found,binding))return false;
                    if(found&&binding.codeId==1066&&!player.generation){player=key;otherObject=object;}return true;
                })&&player.generation);
            }
            if(mode==7)assert(contacts->skeletons().dispatch(local,[](auto& actor,auto&,bool&){actor.timefieldImmune=true;return true;}));
            RunnerCollisionEventContext context;unsigned stomps=0,damages=0;
            const bool ok=context.dispatch(self,player,{true,false},[&](auto,auto,int& subtype,bool& found,PackedObjectEvent& event){
                subtype=otherObject;return objects.collisionEvent(120,otherObject,found,event);
            },[&](const auto& event){return contacts->dispatchSkeletonCollision(event,
                [&](auto& actor){return CheckedSkeletonContact{actor,mode,stomps,damages};});});
            assert(ok==(mode!=3&&mode!=5)&&scene->effectsSucceeded()==ok&&contacts->valid()==ok&&
                stomps==unsigned(mode>=1&&mode<=3)&&damages==unsigned(mode==2||mode==4||mode==5));
            if(ok)assert(contacts->skeletons().find(local)->state.health==(mode==6?0:60));
        }
        for(unsigned contact=0;contact<5;++contact){
            auto contacts=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto scene=std::make_unique<GameplayWorld>(300,false,1000,false,contacts->enemyBinding(),
                contacts->breakableBinding(),contacts->playerBinding(),&contacts->terrain());
            assert(contacts->attach(*scene));InstanceHandle local,self,other;int otherObject=-1;
            assert(contacts->terrain().append(60,0,200,true));
            if(contact==0){
                assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,
                    contacts->terrain().handle(contacts->terrain().size()-1),other));otherObject=60;
            }
            assert(scene->createBoneWithSourceId(505090,240,200,local)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::bone,local,self));
            assert(contacts->visitRegistrationOrder([&](InstanceHandle key,int object,auto){
                bool found=false;PackedObjectEvent binding{};
                if(!objects.collisionEvent(269,object,found,binding))return false;
                if(found&&binding.codeId==(contact?2292:2291)&&!other.generation){other=key;otherObject=object;}return true;
            })&&other.generation);
            if(contact==3)assert(contacts->bones().dispatch(local,[](auto& state,auto&,bool&){state.active=false;return true;}));
            RunnerCollisionEventContext context;unsigned damages=0;const auto rng=scene->randomDrawCount();
            const bool ok=context.dispatch(self,other,{true,false},[&](auto,auto,int& subtype,bool& found,PackedObjectEvent& event){
                subtype=otherObject;return objects.collisionEvent(269,otherObject,found,event);
            },[&](const auto& event){return contacts->dispatchBoneCollision(event,contact==2,
                [&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);},
                [&](auto& caller,int amount,int horizontal,int vertical){
                    const auto* actor=contacts->bones().find(local);
                    assert(amount==1&&horizontal==3&&vertical==2&&actor&&!actor->destroyRequested&&!actor->myFxLocal);
                    assert(&caller==&actor->state);
                    caller.damageScripts.typical.damage=MotionBinary64::fromInteger(amount);
                    ++damages;return contact!=4;
                });});
            if(ok!=(contact!=4)||damages!=unsigned(contact==1||contact==4)||scene->randomDrawCount()!=rng)
                std::fprintf(stderr,"bone contact mismatch ownership=%u contact=%u ok=%d damage=%u rng=%u/%u valid=%d world=%d\n",
                    unsigned(ownership),contact,int(ok),damages,unsigned(scene->randomDrawCount()),unsigned(rng),int(contacts->valid()),int(scene->effectsSucceeded()));
            assert(ok==(contact!=4)&&damages==unsigned(contact==1||contact==4)&&scene->randomDrawCount()==rng);
            assert(scene->effectsSucceeded()==ok&&contacts->valid()==ok);
            assert(contacts->bones().find(local)->state.damageScripts.typical.damage.zero()==(damages==0));
            if(ok){const auto* actor=contacts->bones().find(local);
                assert(actor&&actor->destroyRequested==(contact<3)&&bool(actor->myFxLocal)==(contact<3));}
        }
        {
            auto skeletonOwner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto skeletonWorld=std::make_unique<GameplayWorld>(300,false,0,false,skeletonOwner->enemyBinding(),
                skeletonOwner->breakableBinding(),skeletonOwner->playerBinding(),&skeletonOwner->terrain());
            assert(skeletonOwner->attach(*skeletonWorld));InstanceHandle key,sharedKey;
            const auto draws=skeletonWorld->randomDrawCount();
            assert(skeletonWorld->createSkeletonWithSourceId(505100,100,200,key)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::skeleton,key,sharedKey));
            auto& skeletons=skeletonOwner->skeletons();const auto* actor=skeletons.find(key);
            assert(actor&&actor->state.x==96&&actor->state.y==192&&actor->state.health==60&&
                actor->state.money==10&&actor->state.grounded&&actor->state.alive&&
                !actor->state.imageSpeed.high&&actor->sourceId==505100&&skeletonWorld->randomDrawCount()==draws);
            checkTypedCollisionView(GameplayCollisionView(*skeletonWorld),GameplayCollisionView::Kind::skeleton,key);
            auto skeletonDraw=std::make_unique<ActorDrawOrderVerification>();
            const auto visibleSkeletons=[&](){
                unsigned count=0;
                assert(skeletonOwner->prepareDraw()&&skeletonDraw->verify(*skeletonWorld));
                assert(skeletonWorld->visitRoomActorsReadOnly([&](auto kind,InstanceHandle drawKey,auto){
                    if(kind==GameplayWorld::DrawActor::skeleton){
                        assert(drawKey.slot==key.slot&&drawKey.generation==key.generation);++count;
                    }return true;
                }));return count;
            };
            assert(visibleSkeletons()==1&&visibleSkeletons()==1);
            assert(skeletons.activate(key,false)&&skeletonOwner->reconcileActivation()&&visibleSkeletons()==0);
            assert(skeletons.activate(key,true)&&skeletonOwner->reconcileActivation()&&visibleSkeletons()==1);
            assert(skeletons.dispatch(key,[](auto& state,auto&,bool&){
                state.imageSpeed=MotionBinary64::fromInteger(1);state.shot=true;state.alarms[2]=2;return true;
            }));
            skeletonWorld->step(0);
            assert(skeletonWorld->effectsSucceeded()&&actor->state.alarms[2]==1&&actor->state.shot);
            skeletonWorld->step(0);
            assert(skeletonWorld->effectsSucceeded()&&actor->state.alarms[2]==0&&!actor->state.shot&&
                actor->state.alarms[3]==14&&!actor->state.imageIndex.high&&!actor->state.imageIndex.low);
            assert(skeletons.dispatch(key,[](auto& state,auto&,bool&){state.hitPending=true;return true;}));
            unsigned hurts=0;SkeletonSoundAudio audio;audio.wet=true;
            SoundPlayEnvironment soundEnvironment;soundEnvironment.gameVolume={0x3fd00000u,0};
            soundEnvironment.viewY=MotionBinary64::fromInteger(10000);soundEnvironment.inWater=true;
            auto stepPhase=std::make_unique<RunnerStepSnapshot<GameplayCollisionView::capacity>>();
            TimeStopEffects<4> skeletonStop;skeletonStop.player.stopped=true;
            auto stepSkeletons=[&]{return skeletonOwner->dispatchStepPhase(0,*stepPhase,[&](InstanceHandle selected,const PackedObjectEvent&){
                GameplayCollisionView::Kind kind;InstanceHandle local;
                assert(GameplayCollisionView::decode(selected,kind,local));
                return kind!=GameplayCollisionView::Kind::skeleton||skeletonOwner->stepSkeleton(selected,skeletonStop,
                    [&](auto& actor,auto&){return soundingSkeletonEffects(hurts,[&actor,&skeletonWorld,&soundEnvironment,&audio](int id){
                        return skeletonWorld->playSkeletonSound(actor,id,soundEnvironment,audio);
                    });});
            });};
            assert(stepSkeletons()&&!hurts&&actor->state.hitPending&&actor->state.imageSpeed.zero());
            skeletonStop.player.stopped=false;
            assert(stepSkeletons()&&hurts==1&&actor->state.sprite==205&&actor->state.hitStun&&!actor->state.hitPending&&
                actor->state.alarms[1]==4&&actor->state.alarms[3]==30);
            assert(audio.calls==7&&skeletonWorld->overlapSound(4)==133&&actor->state.sound.sndsnd==1001&&
                soundEnvironment.viewHeight.high==MotionBinary64::fromInteger(284).high&&
                soundEnvironment.viewY.high!=MotionBinary64::fromInteger(10000).high);
            assert(skeletons.dispatchPublished(key,[&](auto& state,auto&,auto& publish,bool&){
                assert(publish.sprite(207)&&publish.imageIndex({0x40800000u})&&publish.sprite(205)&&state.sprite==205&&!state.timeline.bits&&
                    !state.imageIndex.high&&!state.imageIndex.low&&publish.sprite(207)&&publish.facing(-1));
                assert(publish.position({0x42e00000u},{0x43400000u})&&publish.position({0x42e00000u},{0x43500000u})&&
                    state.x==112&&state.y==208&&state.facing==-1);
                if(ownership==GameplayCollisionOwner::Ownership::reference){
                    const auto* geometry=skeletonOwner->state(sharedKey);const auto* properties=skeletonOwner->properties(sharedKey);
                    assert(geometry&&properties&&geometry->x.bits==0x42e00000u&&geometry->y.bits==0x43500000u&&
                        properties->sprite.spriteId==207&&properties->sprite.maskId==208&&properties->sprite.scaleX.bits==0xbf800000u);
                }
                return true;
            }));
            assert(skeletons.markDestroyed(key)&&skeletonOwner->prepareDraw()&&!skeletons.find(key));
            InstanceHandle next;assert(skeletonWorld->createSkeletonWithSourceId(505101,100,200,next)&&
                next.slot==key.slot&&next.generation!=key.generation&&!GameplayCollisionView(*skeletonWorld).contains(sharedKey));
            assert(skeletonWorld->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none&&
                skeletonWorld->skeletons()==&skeletons&&skeletons.size()==0&&skeletonOwner->valid());
        }
        {
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
                owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
            assert(owner->attach(*world));unsigned id=605100;
            checkRegisteredSmokeScript(*world,[&](auto& caller,auto x,auto y,auto direction,auto speed){
                return world->emitRegisteredSmoke(caller,x,y,direction,speed,[&](unsigned& source){source=id++;return true;});
            },[&](unsigned source){const auto* smoke=world->registeredSmoke();
                const auto* r=smoke->find(smoke->sourceHandle(source));return r?&r->state:nullptr;});
            assert(id==605105);
        }
        for(bool knees:{false,true})for(int style:{0,2}){
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
                owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
            assert(owner->attach(*world));if(knees)assert(world->grantUpgrade(0));
            InstanceHandle skeleton;assert(world->createSkeletonWithSourceId(605200,240,200,skeleton));unsigned id=605201;
            const bool ok=owner->skeletons().dispatch(skeleton,[&](auto& actor,auto&,bool& good){
                checkRegisteredSkeletonStomp(*world,actor,knees,style,[&](auto& player,auto& env,auto& audio,bool& accepted){
                    return world->stompSkeleton(actor,[&](unsigned& source){source=id++;return true;},player,env,audio,accepted);
                });good=!knees;return good;
            });
            assert(ok==!knees&&owner->blood().size()==2&&id==605203);
        }
        {
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
            assert(owner->attach(*world));int speed=60;assert(world->bindControllerRoomSpeed(speed));
            InstanceHandle controller,first,second,blast;
            assert(owner->runControllers().create(100065,{}, {},[](InstanceHandle){return true;},controller)&&
                world->createSkeletonWithSourceId(605200,240,200,first)&&
                world->createSkeletonWithSourceId(605201,240,200,second)&&
                owner->createBlast(605202,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(200),blast));
            auto pairs=std::make_unique<RunnerCollisionPairs<GameplayCollisionView::capacity,16384>>();
            struct NonSolidMotion {
                bool rollback(InstanceHandle){assert(false);return false;}
                bool prepareMotion(InstanceHandle){assert(false);return false;}
                bool advanceMotion(InstanceHandle){assert(false);return false;}
            } motion;
            unsigned next=605203,stomps=0,damages=0;BlastHitAudio audio;SoundPlayEnvironment env;
            TimeStopEffects<4> stop;RunnerCollisionEventContext context;
            for(unsigned repeat=0;repeat<2;++repeat){
                assert(owner->collectCollisionPairs(*pairs));
                assert(owner->executeCollisionPairs(*pairs,masks,context,motion,
                    []{return RunnerCollisionEventContext::RoomState{true,false};},[&](const auto& event){
                        return owner->dispatchSkeletonEncounterCollisionWithBlasts(event,stop,
                            [&](unsigned& id){id=next++;return true;},
                            [&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);},env,audio,
                            [](auto dy,auto dx,auto& angle){return portableAtan(dy,dx,angle);},
                            [&](auto& actor){return CheckedSkeletonContact{actor,4,stomps,damages};},
                            [](auto&,int,int,int){return false;},[](const auto&){assert(false);return false;});
                    }));
                assert(next==605205&&audio.calls==12&&!stomps&&!damages&&
                    owner->skeletons().find(first)->state.health==10&&owner->skeletons().find(second)->state.health==10&&
                    owner->scriptEffects().size()==2);
            }
            InstanceHandle player,self;unsigned contacts=0;
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::player,world->playerHandle(),player)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::blast,blast,self));
            for(bool ended:{false,true}){
                assert(owner->registeredBlasts().dispatch(blast,[&](auto& actor,auto&,bool&){actor.damageEnd=ended;return true;}));
                assert(owner->dispatchCollisionEvent(context,self,player,{true,false},[&](const auto& event){
                    ++contacts;return owner->dispatchSkeletonEncounterCollisionWithBlasts(event,stop,
                        [&](unsigned& id){id=next++;return true;},
                        [&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);},env,audio,
                        [](auto dy,auto dx,auto& angle){return portableAtan(dy,dx,angle);},
                        [&](auto& actor){return CheckedSkeletonContact{actor,4,stomps,damages};},
                        [](auto&,int,int,int){return false;},[](const auto&){assert(false);return false;});
                }));
            }
            assert(contacts==2&&owner->registeredBlasts().find(blast)->state.hits.count==2&&
                next==605205&&audio.calls==12&&!stomps&&!damages);
        }
        for(bool rip:{false,true}){
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),owner->playerBinding(),
                ownership==GameplayCollisionOwner::Ownership::reference?&owner->terrain():nullptr);
            auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
            assert(owner->attach(*world)&&owner->configureCorpseMasks(masks)&&
                (ownership==GameplayCollisionOwner::Ownership::reference||world->bindNativeTerrain(*terrain)));
            int speed=60;assert(world->bindControllerRoomSpeed(speed));InstanceHandle controller,blast,self,other;
            GameMakerRandom corpseRandom(2);DeadBodySource source{MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(200),{},MotionBinary64::fromInteger(1)};
            assert(owner->registeredCorpses().createFromDeath(605230,source,206,{},102,rip,corpseRandom,
                [](auto,int,auto,auto,bool& hit){hit=false;return true;}));
            const auto corpse=owner->registeredCorpses().sourceHandle(605230);
            assert(owner->runControllers().create(100065,{}, {},[](InstanceHandle){return true;},controller)&&
                owner->createBlast(605231,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(200),blast)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::blast,blast,self)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::corpse,corpse,other));
            const auto draws=world->randomDrawCount();unsigned allocations=0;RunnerCollisionEventContext context;
            auto pairs=std::make_unique<RunnerCollisionPairs<GameplayCollisionView::capacity,16384>>();unsigned forward=0,reverse=0;
            struct CorpseMotionGuard {
                bool rollback(InstanceHandle){assert(false);return false;}
                bool prepareMotion(InstanceHandle){assert(false);return false;}
                bool advanceMotion(InstanceHandle){assert(false);return false;}
            } motion;
            for(unsigned repeat=0;repeat<2;++repeat)
                {assert(owner->collectCollisionPairs(*pairs));
                assert(owner->executeCollisionPairs(*pairs,masks,context,motion,
                    []{return RunnerCollisionEventContext::RoomState{true,false};},[&](const auto& event){
                        if(event.owner==263){++reverse;return owner->dispatchCorpseBlastCollision(event);}
                        ++forward;return owner->dispatchBlastShootableCollision(event,[&](unsigned& id){++allocations;id=605232;return true;});
                    }));}
            assert(forward==2&&reverse==2);
            const auto* victim=owner->registeredCorpses().find(corpse);const auto* caller=owner->registeredBlasts().find(blast);
            assert(victim&&victim->state.health.high==MotionBinary64::fromInteger(rip?-49:0).high&&!victim->state.health.low&&
                !victim->state.hit&&!victim->destroyRequested&&caller&&caller->state.hits.count==1&&
                allocations==unsigned(rip)&&owner->registeredFlash().size()==unsigned(rip)&&world->randomDrawCount()==draws+(rip?8:0));
        }
        for(bool rejectFlash:{false,true}){
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
            assert(owner->attach(*world));int speed=60;assert(world->bindControllerRoomSpeed(speed));
            InstanceHandle controller,bone,blast,self,other;
            assert(owner->runControllers().create(100065,{}, {},[](InstanceHandle){return true;},controller)&&
                world->createBoneWithSourceId(605220,240,200,bone)&&
                owner->createBlast(605221,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(200),blast)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::blast,blast,self)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::bone,bone,other));
            const auto draws=world->randomDrawCount();unsigned allocations=0;RunnerCollisionEventContext context;
            auto pairs=std::make_unique<RunnerCollisionPairs<GameplayCollisionView::capacity,16384>>();
            struct BoneMotionGuard {
                bool rollback(InstanceHandle){assert(false);return false;}
                bool prepareMotion(InstanceHandle){assert(false);return false;}
                bool advanceMotion(InstanceHandle){assert(false);return false;}
            } motion;
            for(unsigned repeat=0;repeat<(rejectFlash?1u:2u);++repeat){
                assert(owner->collectCollisionPairs(*pairs));
                const bool ok=owner->executeCollisionPairs(*pairs,masks,context,motion,
                    []{return RunnerCollisionEventContext::RoomState{true,false};},[&](const auto& event){
                        assert(event.self.slot==self.slot&&event.other.slot==other.slot&&event.code==2329);
                        return owner->dispatchBlastShootableCollision(event,[&](unsigned& id){++allocations;id=605222;return !rejectFlash;});
                    });
                assert(ok==!rejectFlash);
            }
            const auto* target=owner->bones().find(bone);const auto* caller=owner->registeredBlasts().find(blast);
            assert(target&&target->state.health.high==MotionBinary64::fromInteger(-49).high&&!target->state.health.low&&
                !target->state.hit&&!target->destroyRequested&&caller&&caller->state.hits.count==1&&allocations==1&&
                caller->state.flashCaller.smokenum==1&&!caller->state.flashCaller.pvary&&
                world->randomDrawCount()==draws+(rejectFlash?4:8));
            const auto* flash=owner->registeredFlash().find(owner->registeredFlash().sourceHandle(605222));
            assert(bool(flash)==!rejectFlash);
            if(flash)assert(flash->state.imageIndex.bits==0x3f800000u&&flash->state.imageSpeed.bits==0x3e99999au);
        }
        for(bool generated:{false,true})for(bool knownId:{false,true}){
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
            assert(owner->attach(*world));int speed=60;assert(world->bindControllerRoomSpeed(speed));
            int x=0,y=24;const std::uint32_t source=605210;
            if(generated)assert(world->appendGeneratedBreakable(x,y,{22,173,2},knownId?source:0));
            else assert(world->appendBreakables(&x,&y,1,nullptr,knownId?&source:nullptr));
            assert(world->breakableSlot(0).sourceId==(knownId?source:0));
            if(generated)assert(world->breakableSlot(0).state.breakSound==173&&world->breakableSlot(0).state.haveGem==2);
            InstanceHandle controller,blast,self,other;
            assert(owner->runControllers().create(100065,{}, {},[](InstanceHandle){return true;},controller)&&
                owner->createBlast(605211,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(200),blast)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::blast,blast,self)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::breakable,{0,world->breakableSlot(0).lifetimeId},other));
            RunnerCollisionEventContext context;
            auto pairs=std::make_unique<RunnerCollisionPairs<GameplayCollisionView::capacity,16384>>();
            struct WallMotion {
                bool rollback(InstanceHandle){assert(false);return false;}
                bool prepareMotion(InstanceHandle){assert(false);return false;}
                bool advanceMotion(InstanceHandle){assert(false);return false;}
            } motion;
            for(unsigned repeat=0;repeat<(knownId?2u:1u);++repeat){
                assert(owner->collectCollisionPairs(*pairs));
                const bool ok=owner->executeCollisionPairs(*pairs,masks,context,motion,
                    []{return RunnerCollisionEventContext::RoomState{true,false};},[&](const auto& event){
                        assert(event.self.slot==self.slot&&event.other.slot==other.slot&&event.code==2330);
                        return owner->dispatchBlastWallCollision(event);
                    });
                assert(ok==knownId);
            }
            const auto& state=world->breakableSlot(0).state;
            assert(state.wallHp.high==MotionBinary64::fromInteger(knownId?-49:1).high&&!state.wallHp.low&&!state.hit&&
                !world->breakableSlot(0).destroyRequested);
            const auto* actor=owner->registeredBlasts().find(blast);
            assert(actor&&actor->state.hits.count==unsigned(knownId));
            if(knownId){
                assert(unsigned(actor->state.hits.ids[0])==source);
                if(generated){
                    assert(!world->appendGeneratedBreakable(16,24,{16,171,0},source)&&
                        !world->effectsSucceeded()&&world->breakableSlots()==1&&world->breakableSlot(0).sourceId==source);
                }
            }
        }
        for(bool missingFrames:{false,true}){
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
            assert(owner->attach(*world));int speed=60;assert(world->bindControllerRoomSpeed(speed));
            InstanceHandle controller,skeleton,blast,self,other;
            assert(owner->runControllers().create(100065,{}, {},[](InstanceHandle){return true;},controller)&&
                world->createSkeletonWithSourceId(605200,240,200,skeleton)&&
                owner->createBlast(605201,{0x406e0800u,0},MotionBinary64::fromInteger(200),blast)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::blast,blast,self)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::skeleton,skeleton,other));
            unsigned allocations=0;const auto draws=world->randomDrawCount();BlastHitAudio audio;SoundPlayEnvironment env;
            RunnerCollisionEventContext context;
            const bool hit=owner->dispatchCollisionEvent(context,self,other,{true,false},[&](const auto& event){
                return owner->dispatchBlastSkeletonCollisionWithEffects(event,[&](unsigned& id){++allocations;id=605202;return true;},
                    [&](int sprite,std::uint32_t& count){return !missingFrames&&sprites.frameCount(sprite,count);},env,audio);
            });
            const auto* registered=owner->scriptEffects().find(owner->scriptEffects().handle(1));
            const auto* effect=registered?registered->effect:nullptr;
            assert(hit==!missingFrames&&allocations==1&&effect&&registered&&registered->sourceId==605202&&
                owner->registeredBlasts().find(blast)->state.myFx==605202&&effect->x.bits==0x43704000u&&
                owner->skeletons().find(skeleton)->state.health==10&&world->randomDrawCount()==draws);
            if(hit)assert(effect->spriteId==605&&effect->imageSpeed.bits==0x3ecccccdu&&effect->angle.bits==0x43870000u&&
                effect->depth.bits==0);
        }
        for(bool rejectImpact:{false,true}){
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
            assert(owner->attach(*world));int speed=60;assert(world->bindControllerRoomSpeed(speed));
            InstanceHandle controller,skeleton,blast,self,other;
            assert(owner->runControllers().create(100065,{}, {},[](InstanceHandle){return true;},controller)&&
                world->createSkeletonWithSourceId(605200,240,200,skeleton)&&
                owner->createBlast(605201,{0x406e0800u,0},MotionBinary64::fromInteger(200),blast)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::blast,blast,self)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::skeleton,skeleton,other));
            checkBlastSkeleton(rejectImpact,605200,[&](auto impact,auto& env,auto& audio){
                RunnerCollisionEventContext context;
                return owner->dispatchCollisionEvent(context,self,other,{true,false},
                    [&](const auto& event){return owner->dispatchBlastSkeletonCollision(event,impact,env,audio);});
            },[&]()->const auto&{return owner->registeredBlasts().find(blast)->state;},
              [&]()->const auto&{return owner->skeletons().find(skeleton)->state;});
            assert(owner->valid()==!rejectImpact);
        }
        for(bool rejectSound:{false,true})for(int style:{0,2}){
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
                owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
            assert(owner->attach(*world)&&world->grantUpgrade(0));int speed=60;
            assert(world->bindControllerRoomSpeed(speed));InstanceHandle controller;
            assert(owner->runControllers().create(100065,{}, {},[](InstanceHandle){return true;},controller));
            InstanceHandle skeleton;assert(world->createSkeletonWithSourceId(605200,240,200,skeleton));unsigned id=605201;
            const bool ok=owner->skeletons().dispatch(skeleton,[&](auto& actor,auto&,bool& good){
                checkBoundExplodingStomp(*world,actor,style,id,rejectSound,[&](auto& player,auto& env,auto& audio,bool& accepted){
                    return owner->stompSkeleton(actor,[&](unsigned& source){source=id++;return true;},player,env,audio,accepted);
                },[&](unsigned source){auto& blasts=owner->registeredBlasts();return blasts.find(blasts.sourceHandle(source));},
                [&](unsigned source){auto& smoke=owner->registeredSmoke();const auto* r=smoke.find(smoke.sourceHandle(source));return r?&r->state:nullptr;});
                good=!rejectSound;return good;
            });
            assert(ok==!rejectSound&&speed==30&&owner->blood().size()==2&&owner->registeredBlasts().size()==1&&
                id==605201+(rejectSound?3u:7u));
        }
        auto throwOwner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,ownership);
        auto throwWorld=std::make_unique<GameplayWorld>(300,false,1000,false,throwOwner->enemyBinding(),
            throwOwner->breakableBinding(),throwOwner->playerBinding(),&throwOwner->terrain());
        assert(throwOwner->attach(*throwWorld));unsigned throwId=405200;
        {
            auto encounterOwner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto encounter=std::make_unique<GameplayWorld>(300,false,0,false,encounterOwner->enemyBinding(),
                encounterOwner->breakableBinding(),encounterOwner->playerBinding(),&encounterOwner->terrain());
            assert(encounterOwner->attach(*encounter));InstanceHandle skeleton;
            assert(encounter->createSkeletonWithSourceId(505200,240,200,skeleton));
            auto phase=std::make_unique<RunnerStepSnapshot<GameplayCollisionView::capacity>>();unsigned throws=0,hurts=0;
            TimeStopEffects<4> stop;
            for(unsigned frame=0;frame<24;++frame){
                encounter->step(0);assert(encounter->effectsSucceeded());
                assert(encounterOwner->dispatchStepPhase(0,*phase,[&](InstanceHandle selected,const PackedObjectEvent&){
                    GameplayCollisionView::Kind kind;InstanceHandle local;assert(GameplayCollisionView::decode(selected,kind,local));
                    if(kind==GameplayCollisionView::Kind::bone)return encounterOwner->stepBone(selected,false,
                        [&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);});
                    if(kind!=GameplayCollisionView::Kind::skeleton)return true;
                    return encounterOwner->stepSkeleton(selected,stop,
                        [&](auto& actor,auto& throwing){
                            auto launch=[&](int){
                                const auto before=encounter->randomDrawCount();
                                SkeletonSoundAudio audio;SoundPlayEnvironment environment;environment.gameVolume={0x3fd00000u,0};
                                const bool ok=encounter->throwSkeletonBoneWithSourceId(505201+throws,actor,throwing,environment,audio);
                                assert(ok&&encounter->randomDrawCount()==before+4&&audio.calls==7&&encounter->overlapSound(4)==134&&
                                    throwing.xarc.high==0x3ff00000u&&!throwing.xarc.low);++throws;return ok;
                            };
                            return ThrowingSkeletonEffects<decltype(launch)>{hurts,launch};
                        });
                })&&encounterOwner->prepareDraw());
            }
            assert(throws==1&&!hurts&&encounter->bones()->size()==1);
        }
        checkRegisteredSkeletonThrows(*throwWorld,[&](auto& caller,bool separated,int facing,bool& shot,auto sound){
            return throwWorld->throwRegisteredBoneWithSourceId(throwId++,caller,240,200,separated,facing,shot,sound);
        },[&](InstanceHandle key){return throwOwner->bones().find(key);},[&]{return throwOwner->bones().clear();});
        {
            auto stoppedOwner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},&rooms,ownership);
            auto stoppedWorld=std::make_unique<GameplayWorld>(300,false,0,false,stoppedOwner->enemyBinding(),
                stoppedOwner->breakableBinding(),stoppedOwner->playerBinding(),&stoppedOwner->terrain());
            assert(stoppedOwner->attach(*stoppedWorld));InstanceHandle bone,selected;
            assert(stoppedWorld->createBoneWithSourceId(405300,240,100,bone)&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::bone,bone,selected));
            auto& stoppedBones=stoppedOwner->bones();
            MotionBinary64 hp;bool hitEffect=true;
            assert(stoppedOwner->projectileShootableFields(selected,hp,hitEffect)&&hp.high==0x3ff00000u&&!hp.low&&!hitEffect);
            assert(stoppedBones.activate(bone,false)&&stoppedOwner->projectileShootableHealthWrite(selected,{0xc0040000u,0})&&
                stoppedOwner->projectileShootableHit(selected)&&stoppedBones.find(bone)->state.hit&&!stoppedBones.find(bone)->active);
            assert(stoppedOwner->projectileShootableFields(selected,hp,hitEffect)&&hp.high==0xc0040000u&&!hp.low&&stoppedBones.activate(bone,true));
            const auto countFrames=[&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);};
            const auto draws=stoppedWorld->randomDrawCount();
            for(unsigned tick=0;tick<3;++tick){
                assert(stoppedOwner->stepBone(selected,true,countFrames));
                const auto* retained=stoppedBones.find(bone);
                assert(retained&&!retained->destroyRequested&&retained->state.motion.x==240&&
                    retained->state.motion.y==100&&!retained->state.imageSpeed.bits&&
                    stoppedWorld->scriptEffects().size()==0&&stoppedWorld->randomDrawCount()==draws);
            }
            assert(stoppedOwner->stepBone(selected,false,countFrames)&&stoppedBones.find(bone)->destroyRequested&&
                stoppedWorld->scriptEffects().size()==1&&stoppedWorld->randomDrawCount()==draws);
            assert(stoppedOwner->projectileShootableHealthWrite(selected,{0x3ff40000u,0})&&
                stoppedOwner->projectileShootableFields(selected,hp,hitEffect)&&hp.high==0x3ff40000u&&!hp.low&&
                stoppedBones.find(bone)->destroyRequested&&stoppedOwner->prepareDraw()&&!stoppedBones.find(bone));
            assert(!stoppedOwner->projectileShootableFields(selected,hp,hitEffect)&&!stoppedOwner->valid());
        }
        auto& bones=owner->bones();InstanceHandle local,shared;
        const auto before=world->randomDrawCount();auto random=world->randomState();BoneProjectile reference;
        assert(reference.create(240,990,random)&&world->createBoneWithSourceId(405190,240,990,local)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::bone,local,shared));
        assert(world->bones()==&bones&&bones.find(local)->sourceId==405190&&GameplayCollisionView(*world).contains(shared)&&
            world->randomDrawCount()==before+3);
        for(unsigned i=0;i<16;++i)assert(world->randomState().stateWord(i)==random.stateWord(i));
        checkTypedCollisionView(GameplayCollisionView(*world),GameplayCollisionView::Kind::bone,local);
        unsigned population=0;
        assert(owner->visitRegistrationOrder([&](InstanceHandle h,int object,std::uint32_t){
            if(object==269){assert(h.slot==shared.slot&&h.generation==shared.generation);++population;}return true;
        })&&population==1);
        if(ownership==GameplayCollisionOwner::Ownership::reference)assert(owner->population(269,population)&&population==1);
        auto drawOracle=std::make_unique<ActorDrawOrderVerification>();
        const auto checkBoneDraw=[&](std::initializer_list<InstanceHandle> expected){
            assert(owner->prepareDraw()&&drawOracle->verify(*world));
            for(unsigned repeat=0;repeat<2;++repeat){
                auto next=expected.begin();
                assert(world->visitRoomActorsReadOnly([&](auto kind,InstanceHandle key,auto depth){
                    if(kind!=GameplayWorld::DrawActor::bone)return true;
                    assert(next!=expected.end()&&key.slot==next->slot&&key.generation==next->generation&&
                        !(depth.bits&0x7fffffffu));++next;return true;
                })&&next==expected.end());
            }
        };
        checkBoneDraw({local});
        InstanceHandle newer;
        assert(world->createBoneWithSourceId(405196,248,994,newer));checkBoneDraw({newer,local});
        // Engine activation is not the bone's private 'active' damage flag.
        assert(bones.dispatch(newer,[](auto& state,auto&,bool&){state.active=false;return true;}));
        checkBoneDraw({newer,local});
        assert(bones.activate(newer,false)&&owner->reconcileActivation()&&drawOracle->verify(*world));
        assert(world->visitRoomActorsReadOnly([&](auto kind,InstanceHandle key,auto){
            return kind!=GameplayWorld::DrawActor::bone||key.slot!=newer.slot;
        }));
        assert(bones.activate(newer,true)&&owner->reconcileActivation());checkBoneDraw({newer,local});
        assert(bones.markDestroyed(newer));checkBoneDraw({local});
        InstanceHandle reused;
        assert(world->createBoneWithSourceId(405197,252,998,reused)&&reused.slot==newer.slot&&reused.generation!=newer.generation);
        checkBoneDraw({reused,local});
        assert(bones.markDestroyed(reused));checkBoneDraw({local});
        for(unsigned tick=0;tick<8;++tick){
            world->step(0);bool crossed;assert(reference.advanceImage(crossed));
            assert(world->effectsSucceeded()&&owner->valid()&&bones.find(local)->state.timeline.bits==reference.timeline.bits);
            if(ownership==GameplayCollisionOwner::Ownership::reference)
                assert(owner->properties(shared)->sprite.imageIndex.bits==reference.imageIndex.bits);
        }
        // Death body uses the registered impact owner and published view.
        unsigned effects=0,effectLocal=0;
        auto verifyImpact=[&](){
            const auto* caller=bones.find(local);assert(caller&&caller->myFxLocal&&!caller->myFxSourceId);
            effectLocal=caller->myFxLocal;
            const auto* effect=world->scriptEffects().get(effectLocal);
            const auto child=world->scriptSpawnInstances()->handle(effectLocal);
            assert(effect&&child.generation&&world->scriptSpawnInstances()->find(child)&&effect->spriteId==85&&
                effect->frames==6&&effect->imageSpeed.bits==0x3e99999au&&!effect->angle.bits&&!effect->depth.bits&&
                effect->x.bits==caller->x.bits&&effect->y.bits==caller->y.bits);
            return true;
        };
        assert(bones.dispatch(local,[](auto& state,auto&,bool&){state.health={};return true;}));
        auto bonePhase=std::make_unique<RunnerStepSnapshot<GameplayCollisionView::capacity>>();
        const auto beforeStepRng=world->randomDrawCount();
        assert(owner->dispatchStepPhase(0,*bonePhase,[&](InstanceHandle selected,const PackedObjectEvent&){
            GameplayCollisionView::Kind kind;InstanceHandle child;
            assert(GameplayCollisionView::decode(selected,kind,child));
            if(kind!=GameplayCollisionView::Kind::bone)return true;
            assert(child.slot==local.slot&&child.generation==local.generation);++effects;
            return owner->stepBone(selected,false,[&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);});
        })&&world->randomDrawCount()==beforeStepRng&&verifyImpact());
        assert(effects==1&&bones.find(local)->destroyRequested&&bones.find(local)->active&&
            !bones.find(local)->state.active&&GameplayCollisionView(*world).contains(shared));
        if(ownership==GameplayCollisionOwner::Ownership::reference)
            assert(owner->properties(shared)->sprite.maskId==582&&owner->state(shared)->x.bits==bones.find(local)->x.bits);
        assert(owner->prepareDraw()&&!bones.find(local)&&!GameplayCollisionView(*world).contains(shared));
        assert(effectLocal&&world->scriptEffects().get(effectLocal));
        InstanceHandle retained;
        assert(world->createBoneWithSourceId(405191,240,990,retained));
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        assert(world->bones()==&bones&&!bones.find(retained)&&bones.size()==0&&owner->valid());
        InstanceHandle replacement;assert(world->createBoneWithSourceId(405192,240,990,replacement)&&
            replacement.generation>retained.generation&&!bones.find(local));
        assert(bones.dispatch(replacement,[](auto& state,auto&,bool&){state.health={};return true;}));
        InstanceHandle replacementShared;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::bone,replacement,replacementShared));
        const auto beforeFailureRng=world->randomDrawCount();unsigned lookups=0;
        auto rejectSprite=[&](int sprite,unsigned&){
            ++lookups;const auto* caller=bones.find(replacement);
            assert(sprite==85&&caller&&caller->myFxLocal&&!caller->myFxSourceId&&
                world->scriptSpawnInstances()->find(world->scriptSpawnInstances()->handle(caller->myFxLocal)));
            return false;
        };
        assert(!owner->stepBone(replacementShared,false,rejectSprite)&&!world->effectsSucceeded()&&
            world->randomDrawCount()==beforeFailureRng&&world->scriptEffects().size()==1);
        assert(!owner->stepBone(replacementShared,false,rejectSprite)&&lookups==1&&world->scriptEffects().size()==1);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        for(unsigned i=0;i<8;++i){world->step(4);assert(world->effectsSucceeded()&&owner->prepareDraw());}
        assert(world->bulletCount()==2);
        const auto local=[](const auto& b){return InstanceHandle{b.identitySlot,b.lifetimeId};};
        const auto first=local(world->bulletAt(0)),second=local(world->bulletAt(1));
        InstanceHandle firstKey,secondKey;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::bullet,first,firstKey)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::bullet,second,secondKey));
        for(unsigned i=0;i<2;++i){
            const auto& bullet=world->bulletAt(i);InstanceHandle key;
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::bullet,local(bullet),key));
            const auto state=owner->state(key);const auto properties=owner->properties(key);
            RunnerImageValue x,y,angle;
            assert(ProjectilePublication::coordinate(bullet.trajectory.x,240,x)&&
                ProjectilePublication::coordinate(bullet.trajectory.y,176,y)&&
                ProjectilePublication::angle(bullet.trajectory.direction,angle));
            assert(state&&properties&&state->x.bits==x.bits&&state->y.bits==y.bits&&
                properties->sprite.imageIndex.bits==bullet.animation.index.bits&&
                properties->sprite.spriteId==machineGunProfile(bullet.gemHigh).sprite&&properties->sprite.angle.bits==angle.bits);
        }
        world->configureSolidResolver([](void*,int,int y){return y==70;},nullptr);
        world->step(4);
        unsigned population;
        assert(world->effectsSucceeded()&&world->resolveBulletLifetime(first)&&
            world->resolveBulletLifetime(first)->destroyRequested&&!world->findBullet(first.generation)&&
            owner->state(firstKey)&&(owner->state(firstKey)->flags&1u)&&
            owner->population(261,population)&&population==1);
        assert(owner->prepareDraw()&&!world->resolveBulletLifetime(first)&&!owner->state(firstKey));
        world->configureSolidResolver(nullptr,nullptr);
        for(unsigned i=9;i<=14;++i){world->step(4);assert(world->effectsSucceeded()&&owner->prepareDraw());}
        assert(world->bulletCount()==2&&world->resolveBulletLifetime(second)==&world->bulletAt(0)&&
            owner->state(secondKey)&&!owner->state(firstKey));
        const auto newest=local(world->bulletAt(1));
        assert(newest.generation>second.generation&&newest.slot==first.slot);
        // A mixed phase retains generation keys across compaction and reuse.
        // Selecting an old key must not step the new occupant or dirty Draw.
        bool changed=false;const auto beforeStepRng=world->randomDrawCount();
        const auto beforeY=world->resolveBulletLifetime(newest)->trajectory.y;
        assert(world->stepBulletRoomEffect(first,0,changed)&&!changed&&
            world->randomDrawCount()==beforeStepRng);
        const auto afterY=world->resolveBulletLifetime(newest)->trajectory.y;
        assert(afterY.pixel==beforeY.pixel&&afterY.fraction==beforeY.fraction);
        assert(world->stepBulletRoomEffect(newest,0,changed)&&changed&&world->effectsSucceeded());
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,2,300)==ResultsDestination::gameplay&&
            !owner->state(secondKey)&&!world->resolveBulletLifetime(newest)&&owner->population(261,population)&&population==0);
        world->step(4);
        assert(world->effectsSucceeded()&&world->bulletCount()==1&&world->bulletAt(0).lifetimeId>newest.generation);
    }
    auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
    {
        auto terrain=std::make_unique<GameplayTerrainInstances>();GameMakerRandom random;
        assert(terrain->append(60,128,0,true,123,999));
        const auto h=terrain->handle(0);
        assert(terrain->createProperties(h,51,false,random));
        assert(!terrain->roomCreation(h)&&!terrain->valid());
        assert(terrain->size()==1&&terrain->find(h)->spriteId==51&&terrain->find(h)->wall.pillar==0);
    }
    assert(owner->valid());
    auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
    auto unrelated=std::make_unique<GameplayWorld>(300,true,0,false);
    assert(!owner->attach(*unrelated)&&owner->valid());
    assert(owner->attach(*world)&&world->effectsSucceeded());
    world->configureSolidResolver([](void*,int,int){return false;},nullptr);
    int x=80,y=40;world->configureBreakables(&x,&y,1);
    assert(world->configureEnemy(0,111,163,100,-2000,0,0,5,6));
    assert(world->configureEnemy(1,116,153,40,500,0,0,15,12,0,0,-1,-1,180,320,false));
    const auto local=world->enemyHandle(world->enemyAt(1).lifetimeId);
    InstanceHandle key,player;
    assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::enemy,local,key)&&
        GameplayCollisionView::encode(GameplayCollisionView::Kind::player,world->playerHandle(),player));
    auto state=owner->state(key);std::int32_t px,py;
    assert(state&&(state->flags&2)&&state->x.promoted().roundPixel(px)&&px==280&&state->y.promoted().roundPixel(py)&&py==676);
    assert(owner->properties(key)->sprite.spriteId==153&&owner->properties(key)->sprite.maskId==-1);
    unsigned count=0;
    assert(owner->population(35,count)&&count==1&&owner->population(92,count)&&count==1);
    unsigned terrainCalls=0;
    auto terrain=[&](const auto&){++terrainCalls;return true;};
    assert(world->createGeneratedInstance({60,240,512},objects,terrain));
    assert(world->createGeneratedInstance({60,240,512},objects,terrain)); // Coincident, distinct births.
    DeferredCreateInstance platform(93,256,528);platform.active=false;
    assert(world->createGeneratedInstance(platform,objects,terrain));
    assert(terrainCalls==3&&owner->terrain().size()==3);
    InstanceHandle wall,otherWall,ledge;
    const auto wallLocal=owner->terrain().handle(0);
    assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,wallLocal,wall)&&
        GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,owner->terrain().handle(1),otherWall)&&
        GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,owner->terrain().handle(2),ledge));
    assert(wall.slot!=otherWall.slot&&owner->state(wall)&&owner->state(otherWall));
    assert(owner->properties(ledge)->sprite.spriteId==51&&
        owner->properties(ledge)->sprite.imageIndex.bits==0x41b80000u&&
        owner->terrain().find(owner->terrain().handle(2))->imageIndex==23&&
        owner->terrain().find(owner->terrain().handle(2))->imageSpeed==0);
    assert(owner->state(wall)->x.promoted().roundPixel(px)&&px==240&&
        owner->state(wall)->y.promoted().roundPixel(py)&&py==512&&(owner->state(ledge)->flags&2));
    assert(owner->population(60,count)&&count==2);
    GameplayCollisionView::Record terrainRecord;
    assert(GameplayCollisionView(*world).resolve(wall,terrainRecord)&&terrainRecord.terrain==owner->terrain().find(wallLocal));
    const auto dying=terrainRecord.terrain;
    assert(owner->terrain().requestDestroy(wallLocal)&&owner->terrain().find(wallLocal)->destroyRequested&&
        owner->terrain().size()==3&&owner->terrain().find(wallLocal)==dying&&
        (owner->state(wall)->flags&1u)&&!(owner->state(otherWall)->flags&1u));
    assert(owner->terrain().requestDestroy(wallLocal)); // repeated request retains the same lifetime
    assert(owner->terrain().activation(wallLocal,false)&&!owner->terrain().find(wallLocal)->active&&
        (owner->state(wall)->flags&3u)==3u&&owner->terrain().find(wallLocal)==dying);
    const auto survivor=owner->terrain().find(owner->terrain().handle(1));
    const auto survivorHandle=owner->terrain().handle(1);
    assert(owner->reconcileActivation()&&owner->terrain().find(wallLocal)==dying);
    assert(owner->prepareDraw()&&!owner->terrain().find(wallLocal)&&!owner->state(wall));
    assert(owner->prepareDraw()); // An unchanged frame does not sweep again.
    assert(owner->terrain().size()==2&&owner->terrain().handle(0).slot==survivorHandle.slot&&
        owner->terrain().find(survivorHandle)==survivor&&owner->state(otherWall));
    assert(world->createGeneratedInstance({60,240,512},objects,terrain));
    const auto replacement=owner->terrain().handle(2);
    assert(replacement.slot==wallLocal.slot&&replacement.generation!=wallLocal.generation&&
        !owner->terrain().find(wallLocal)&&owner->terrain().find(survivorHandle)==survivor);
    assert(owner->terrain().find(replacement)->spriteId==51&&!owner->terrain().find(replacement)->destroyRequested);
    // A backend rejection must not invent an identity for a cell it rejected.
    assert(!world->createGeneratedInstance({60,1,2},objects,[](const auto&){return false;})&&owner->terrain().size()==3);
    world->step(0);
    assert(world->effectsSucceeded()&&owner->valid()&&world->enemyCount()==2&&world->enemyAt(0).destroyRequested);
    assert(owner->prepareDraw()); // End Step marks; Draw-entry cleanup removes the offscreen snake.
    assert(world->effectsSucceeded()&&owner->valid()&&world->enemyCount()==1&&owner->state(key));
    const auto* enemy=world->findEnemy(local);assert(enemy);
    state=owner->state(key);
    assert(state->x.promoted().roundPixel(px)&&px==enemy->x+240&&state->y.promoted().roundPixel(py)&&py==enemy->y+176);
    assert(owner->properties(key)->sprite.spriteId==enemy->spriteId);
    assert(owner->population(111,count)&&count==0);
    assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
    assert(owner->valid()&&!owner->state(key)&&!owner->state(player)&&owner->population(35,count)&&count==1);
    assert(owner->population(92,count)&&count==0&&owner->population(116,count)&&count==0);
    assert(!owner->state(wall)&&!owner->state(otherWall)&&!owner->state(ledge)&&
        !owner->terrain().find(wallLocal)&&owner->terrain().size()==0&&owner->population(60,count)&&count==0);
    assert(world->createGeneratedInstance({60,240,512},objects,terrain));
    assert(owner->terrain().handle(0).generation>wallLocal.generation&&!GameplayCollisionView(*world).resolve(wall,terrainRecord));
    // A copied record must not mutate the installed lifetime's properties.
    auto copy=*world->findPlayer(world->playerHandle());
    assert(!owner->playerBinding().integerPosition(world->playerHandle(),copy,1,2)&&!owner->valid());
    world->step(0);assert(!world->effectsSucceeded());
    {
        auto masks=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto batWorld=std::make_unique<GameplayWorld>(300,false,1000,false,
            masks->enemyBinding(),masks->breakableBinding(),masks->playerBinding());
        assert(masks->attach(*batWorld));
        batWorld->configureSolidResolver([](void*,int,int){return false;},nullptr);
        assert(batWorld->configureEnemy(0,116,153,0,12,0,0,15,12,0,0,-1,-1,270,256,true,false,true));
        const auto bat=batWorld->enemyHandle(batWorld->enemyAt(0).lifetimeId);
        InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::enemy,bat,shared));
        assert(masks->properties(shared)->sprite.maskId==-1);
        batWorld->step(4); // Real surviving bullet hit, not a forced sprite write.
        assert(batWorld->effectsSucceeded()&&batWorld->enemyAt(0).health==6&&batWorld->enemyAt(0).batHit.pending);
        for(unsigned frame=0;frame<3;++frame){
            batWorld->step(0);
            const auto p=masks->properties(shared);
            assert(batWorld->effectsSucceeded()&&p&&p->sprite.spriteId==154&&p->sprite.maskId==154&&
                batWorld->enemyAt(0).maskId==154);
        }
        batWorld->step(0);
        assert(batWorld->effectsSucceeded()&&masks->properties(shared)->sprite.spriteId==153&&
            masks->properties(shared)->sprite.maskId==153&&batWorld->enemyAt(0).maskId==153);
        auto copied=batWorld->enemyAt(0);
        assert(!masks->enemyBinding().mask(bat,copied,999)&&!masks->valid()&&batWorld->enemyAt(0).maskId==153);
        batWorld->step(0);assert(!batWorld->effectsSucceeded());
    }
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                       GameplayCollisionOwner::Ownership::nativeScenery}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,ownership);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,
            owner->enemyBinding(),owner->breakableBinding(),owner->playerBinding());
        assert(owner->attach(*world));
        world->configureSolidResolver([](void*,int,int){return false;},nullptr);
        assert(world->configureEnemy(0,116,153,0,12,0,0,1,12,0,0,-1,-1,270,256,true,false,true));
        world->step(4);
        assert(world->effectsSucceeded()&&!world->enemyAt(0).active&&world->gemCount()>0);
        // Rewards born in bullet 261's Step cannot have run currency 248's
        // earlier Step. Their Create state must reach Draw unchanged.
        for(unsigned i=0;i<world->gemCount();++i){
            const auto& gem=world->gemAt(i);
            assert(gem.active&&gem.age==0&&gem.motion.lifetime.age==0&&
                gem.x==world->enemyAt(0).x&&gem.y==world->enemyAt(0).y);
        }
    }
    {
        TerrainAudit audit;
        auto store=std::make_unique<GameplayTerrainInstances>(GameplayTerrainInstances::Binding::bind(audit));
        audit.store=store.get();
        assert(store->append(60,1,2,true)&&store->append(93,3,4,true));
        const auto first=store->handle(0),second=store->handle(1);
        audit.failRetire=2;
        assert(!store->clear()&&!store->valid()&&audit.retires==2);
        assert(!store->find(first)&&store->find(second)); // Successful prefix is not rolled back.
        assert(store->size()==1&&store->handle(0).slot==second.slot);
        assert(!store->clear()&&audit.retires==2&&!store->append(60,5,6,true));
    }
    {
        TerrainAudit audit;
        auto store=std::make_unique<GameplayTerrainInstances>(GameplayTerrainInstances::Binding::bind(audit));
        audit.store=store.get();audit.failBirth=1;
        auto failure=std::make_unique<GameplayWorld>(300,true,0,false,
            GameplayWorld::EnemyLifecycleBinding{},GameplayWorld::BreakableLifecycleBinding{},
            GameplayWorld::PlayerLifecycleBinding{},store.get());
        assert(!failure->createGeneratedInstance({60,1,2},objects,[](const auto&){return true;}));
        assert(!failure->effectsSucceeded()&&!store->valid()&&store->find(store->handle(0))&&audit.births==1);
        failure->step(0);assert(!failure->effectsSucceeded());
    }
    {
        auto terrain=std::make_unique<GameplayTerrainInstances>();
        auto full=std::make_unique<GameplayWorld>(300,true,0,false,
            GameplayWorld::EnemyLifecycleBinding{},GameplayWorld::BreakableLifecycleBinding{},
            GameplayWorld::PlayerLifecycleBinding{},terrain.get());
        unsigned calls=0;
        const auto backend=[&](const auto&){++calls;return true;};
        for(unsigned i=0;i<GameplayTerrainInstances::capacity;++i)
            assert(full->createGeneratedInstance({60,240,512},objects,backend));
        assert(!full->createGeneratedInstance({93,240,528},objects,backend)&&!full->effectsSucceeded());
        assert(terrain->size()==GameplayTerrainInstances::capacity&&calls==GameplayTerrainInstances::capacity+1);
        assert(!full->createGeneratedInstance({60,0,0},objects,backend)&&calls==GameplayTerrainInstances::capacity+1);
        full->step(0);assert(!full->effectsSucceeded());
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));const int bx=0,by=336;
        assert(world->appendBreakables(&bx,&by,1));const InstanceHandle box{0,1};
        unsigned lookups=0;bool accept=true;InstanceHandle observed;
        auto frames=[&](int sprite,unsigned& count){
            ++lookups;assert(sprite==603);observed=world->findBreakable(box)->myFx;
            const auto* child=owner->scriptEffects().find(observed);
            assert(child&&child->effect&&child->effect->spriteId==-1);
            PackedSpriteCollision art;assert(sprites.sprite(sprite,art));count=art.frameCount;return accept;
        };
        auto x=MotionBinary64::fromInteger(240),y=MotionBinary64::fromInteger(524);
        assert(x.add({0x3fd00000u,0})&&y.add({0x3fc00000u,0}));
        const auto draws=world->randomDrawCount();
        assert(world->spawnBoxScriptEffect(box,x,y,603,{0x3fe00000u,0},MotionBinary64::fromInteger(270),
            MotionBinary64::fromInteger(-60000),frames));
        const auto first=observed;const auto* child=owner->scriptEffects().find(first)->effect;
        assert(child->spriteId==603&&child->x.bits==0x43704000u&&child->y.bits==0x44030800u&&
            child->imageSpeed.bits==0x3f000000u&&child->angle.bits==0x43870000u&&world->randomDrawCount()==draws);
        accept=false;
        assert(!world->spawnBoxScriptEffect(box,x,y,603,{0x3fe00000u,0},MotionBinary64::fromInteger(270),
            MotionBinary64::fromInteger(-60000),frames));
        const auto second=world->findBreakable(box)->myFx;
        assert(!world->effectsSucceeded()&&second.generation>first.generation&&owner->scriptEffects().find(first)&&
            owner->scriptEffects().find(second)&&world->scriptEffects().size()==2&&lookups==2);
        assert(!world->spawnBoxScriptEffect(box,x,y,603,{},{},{},frames)&&lookups==2&&world->randomDrawCount()==draws);
    }
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                       GameplayCollisionOwner::Ownership::nativeScenery})
    for(bool replacement:{false,true})for(bool sourceStep:{false,true}){
        // Birth during builder Step must not inherit a later box Step turn,
        // including a replacement that occupies an already-captured slot.
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},&rooms,ownership);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        const int x=64,y=512;assert(world->appendBreakables(&x,&y,1));
        const InstanceHandle old{0,world->breakableSlot(0).lifetimeId};
        if(!sourceStep)assert(world->publishBoxHealth(old,{}));
        struct Birth {bool replacement,sourceStep,done=false;InstanceHandle key{};} birth{replacement,sourceStep};
        world->configureGenerationStep([](void* context,GameplayWorld& w,GameMakerRandom&,int){
            auto& b=*static_cast<Birth*>(context);if(b.done)return true;b.done=true;
            const int x=96,y=512;
            if(b.replacement)w.configureBreakables(&x,&y,1);
            else if(!w.appendBreakables(&x,&y,1))return false;
            const unsigned slot=w.breakableSlots()-1;b.key={slot,w.breakableSlot(slot).lifetimeId};
            return w.effectsSucceeded()&&(b.sourceStep||w.publishBoxHealth(b.key,{}));
        },&birth);
        ScheduledBoxCounts counts;
        auto tick=[&]{
            if(!sourceStep){world->step(0);return world->effectsSucceeded();}
            return world->withBoxDestruction([&](InstanceHandle h,BoxDestructionScript::Caller& caller){
                InstanceHandle shared;
                return GameplayCollisionView::encode(GameplayCollisionView::Kind::breakable,h,shared)&&
                    owner->readBoxDestruction(shared,caller);
            },[&](InstanceHandle h){return scheduledBoxEffects(*world,h,counts,
                [&](InstanceHandle key){return world->markBreakableDestroyed(key);});},
                [&]{world->step(0);return world->effectsSucceeded();});
        };
        assert(tick()&&birth.done&&world->findBreakable(birth.key)&&!world->findBreakable(birth.key)->destroyRequested);
        assert(birth.key.generation>old.generation);
        if(replacement)assert(!world->findBreakable(old));
        else assert(world->findBreakable(old)->destroyRequested==!sourceStep);
        assert(counts.stop==(sourceStep&&!replacement?1u:0u)&&!counts.sound);
        counts={};assert(tick());
        assert(world->findBreakable(birth.key)->destroyRequested==!sourceStep);
        assert(counts.stop==(sourceStep?(replacement?1u:2u):0u)&&!counts.sound);
    }
    for(unsigned water=0;water<2;++water)for(unsigned failure=0;failure<=6;++failure){
        auto world=std::make_unique<GameplayWorld>(300,true);
        const int bx=0,by=336;assert(world->appendBreakables(&bx,&by,1));const InstanceHandle box{0,1};
        BoxSoundVerification::Audio<GameplayWorld> audio{*world,171};audio.chain=true;audio.water=water;
        const auto volume=MotionBinary64{0x3fd00000u,0};const auto draws=world->randomDrawCount();
        assert(world->playBoxBreakSound(box,volume,audio));
        audio.fail=failure?7+failure:0;
        assert(world->playBoxChainSound(box,volume,water,audio)==!failure);
        const auto& sound=world->findBreakable(box)->sound;
        assert(audio.valid&&audio.calls==(failure?7+failure:13)&&world->effectsSucceeded()==!failure&&
            sound.sfxIndex==342&&sound.soundPriority==50&&!sound.loop&&sound.cutPrv&&sound.chanNum==7&&
            world->overlapSound(7)==171&&world->randomDrawCount()==draws);
        if(failure)assert(!world->playBoxChainSound(box,volume,water,audio)&&audio.calls==7+failure);
    }
    for(unsigned failure=1;failure<=7;++failure){
        auto world=std::make_unique<GameplayWorld>(300,true);
        const int bx=0,by=336;assert(world->appendBreakables(&bx,&by,1));const InstanceHandle box{0,1};
        assert(BoxSoundVerification::run(*world,box,171,[&](){return &world->findBreakable(box)->sound;},failure));
    }
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                       GameplayCollisionOwner::Ownership::nativeScenery}){
        // Native ownership skips box groups while no live box has Alarm 0
        // set; an armed box must still count down beside idle ones.
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},
            ownership==GameplayCollisionOwner::Ownership::nativeScenery?&rooms:nullptr,ownership);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding());
        assert(owner->attach(*world));
        const int xs[]={64,96},ys[]={512,512};
        assert(world->appendBreakables(xs,ys,2));
        const InstanceHandle idle{0,world->breakableSlot(0).lifetimeId},armed{1,world->breakableSlot(1).lifetimeId};
        world->step(0);
        assert(world->effectsSucceeded()&&world->findBreakable(idle)->alarm0==-1&&world->findBreakable(armed)->alarm0==-1);
        assert(world->publishBoxAlarm0(armed,2));
        // Expiry zeroes health; the box's own Step then marks destruction,
        // which excludes it (still at zero) from later alarm passes.
        for(int expected:{1,0,0}){
            world->step(0);
            assert(world->effectsSucceeded()&&world->findBreakable(idle)->alarm0==-1&&
                world->findBreakable(armed)->alarm0==expected);
        }
        assert(world->findBreakable(armed)->state.wallHp.zero()&&world->findBreakable(armed)->destroyRequested&&
            !world->findBreakable(idle)->state.wallHp.zero()&&!world->findBreakable(idle)->destroyRequested);
    }
    {
        GameplayWorld world(300,true);
        const int xs[]={64,96,128},ys[]={512,512,512};
        assert(world.appendBreakables(xs,ys,3));
        const InstanceHandle a{0,world.breakableSlot(0).lifetimeId},
            b{1,world.breakableSlot(1).lifetimeId},c{2,world.breakableSlot(2).lifetimeId};
        assert(world.publishBoxAlarm0(a,1)&&world.publishBoxAlarm0(b,0)&&world.publishBoxAlarm0(c,1));
        const auto phase=world.captureBoxAlarms();
        assert(world.markBreakableDestroyed(c));
        assert(world.appendBreakables(xs,ys,1));
        const InstanceHandle newborn{3,world.breakableSlot(3).lifetimeId};
        assert(world.publishBoxAlarm0(newborn,1)&&world.advanceBoxAlarmPrefix(phase));
        assert(world.findBreakable(a)->alarm0==0&&world.findBreakable(a)->state.wallHp.zero());
        assert(world.findBreakable(b)->alarm0==-1&&!world.findBreakable(b)->state.wallHp.zero());
        assert(world.findBreakable(c)->alarm0==1&&world.findBreakable(newborn)->alarm0==1);
        world.configureBreakables(xs,ys,3);
        const InstanceHandle replacement{0,world.breakableSlot(0).lifetimeId};
        assert(replacement.generation>phase.lastLifetime&&world.publishBoxAlarm0(replacement,1));
        assert(world.advanceBoxAlarmPrefix(phase)&&world.findBreakable(replacement)->alarm0==1);
        assert(world.advanceBoxAlarmPrefix(world.captureBoxAlarms())&&world.findBreakable(replacement)->alarm0==0);
    }
    for(bool registered:{false,true}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,
            registered?owner->enemyBinding():GameplayWorld::EnemyLifecycleBinding{},
            registered?owner->breakableBinding():GameplayWorld::BreakableLifecycleBinding{},
            registered?owner->playerBinding():GameplayWorld::PlayerLifecycleBinding{});
        if(registered)assert(owner->attach(*world));
        const int x=64,y=512;
        assert(world->appendBreakables(&x,&y,1));
        const InstanceHandle key{0,1};
        if(registered){
            assert(world->publishBoxAlarm0(key,1));
            const auto draws=world->randomDrawCount();
            // Box is inside strict room sides, but well below any field.
            assert(owner->stopBoxAlarm0(key,true,480,masks)&&world->findBreakable(key)->alarm0==2&&
                world->findBreakable(key)->state.scriptIndex==1&&world->randomDrawCount()==draws);
            SpriteMaskPack absent(nullptr,0);
            assert(owner->stopBoxAlarm0(key,false,480,absent)&&world->findBreakable(key)->alarm0==2);
            for(int value:{-2,-1,0}){
                assert(world->publishBoxAlarm0(key,value)&&owner->stopBoxAlarm0(key,true,480,masks)&&
                    world->findBreakable(key)->alarm0==value);
            }
            RunnerImageValue py;assert(py.assign(MotionBinary64::fromInteger(20000)));
            for(int quarter:{639,640,1280,1281}){
                auto q=MotionBinary64::fromInteger(quarter);q.high-=2u<<20;RunnerImageValue px;assert(px.assign(q));
                assert(owner->breakableBinding().publishedPosition(key,*world->findBreakable(key),px,py)&&
                    world->publishBoxAlarm0(key,1)&&owner->stopBoxAlarm0(key,true,480,masks));
                assert(world->findBreakable(key)->alarm0==(quarter>=640&&quarter<=1280?2:1));
            }
            RunnerImageValue px;assert(px.assign(MotionBinary64::fromInteger(x+240))&&py.assign(MotionBinary64::fromInteger(y+176)));
            assert(owner->breakableBinding().publishedPosition(key,*world->findBreakable(key),px,py));
            assert(world->createGeneratedInstance({82,x+240,y+176},objects,[](const auto&){return true;}));
            assert(world->publishBoxAlarm0(key,1)&&owner->stopBoxAlarm0(key,true,480,masks)&&
                world->findBreakable(key)->alarm0==1);
        }
        assert(world->publishBoxAlarm0(key,2));
        world->step(0);
        assert(world->effectsSucceeded()&&world->findBreakable(key)->alarm0==1&&
            world->findBreakable(key)->state.wallHp.high==0x3ff00000u);
        // Zero retires to -1 without dispatching a second event.
        assert(world->publishBoxAlarm0(key,0)&&world->publishBoxHealth(key,MotionBinary64::fromInteger(3)));
        world->step(0);
        assert(world->effectsSucceeded()&&world->findBreakable(key)->alarm0==-1&&
            world->findBreakable(key)->state.wallHp.high==0x40080000u);
        assert(world->publishBoxAlarm0(key,-2));world->step(0);
        assert(world->effectsSucceeded()&&world->findBreakable(key)->alarm0==-2);
        if(registered){
            assert(world->breakableActiveAt(x,y));
            assert(world->publishBoxAlarm0(key,1)&&world->markBreakableDestroyed(key));
            assert(world->markBreakableDestroyed(key)&&world->findBreakable(key)&&!world->breakableActiveAt(x,y));
            // The event continues, but compatibility collision rejects the
            // destroyed self (flag 1), even while it overlaps a live field.
            assert(owner->stopBoxAlarm0(key,true,480,masks)&&world->findBreakable(key)->alarm0==2);
            world->step(0);
            assert(world->effectsSucceeded()&&world->findBreakable(key)->alarm0==2&&
                world->findBreakable(key)->state.wallHp.high==0x40080000u);
            assert(owner->prepareDraw()&&!world->findBreakable(key)&&!world->breakableActiveAt(x,y));
        }
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,2,300)!=ResultsDestination::none);
        assert(world->appendBreakables(&x,&y,1));
        const InstanceHandle fresh{0,2};unsigned queries=0;
        assert(world->publishBoxAlarm0(fresh,5));
        assert(!world->stopBoxAlarm0(fresh,[&](bool&){++queries;return false;})&&queries==1&&
            world->findBreakable(fresh)->alarm0==5&&world->findBreakable(fresh)->state.scriptIndex==0);
        assert(!world->stopBoxAlarm0(fresh,[&](bool&){++queries;return true;})&&queries==1);
        assert(!world->publishBoxAlarm0(key,1)&&!world->effectsSucceeded());
    }
    for(bool retire:{false,true}){
        auto world=std::make_unique<GameplayWorld>(300,true);
        const int x=64,y=512;assert(world->appendBreakables(&x,&y,1));
        const InstanceHandle box{0,1};unsigned queries=0;
        assert(world->publishBoxAlarm0(box,5));
        assert(!world->stopBoxAlarm0(box,[&](bool& allowed){
            ++queries;allowed=false;
            if(retire){
                assert(world->markBreakableDestroyed(box)&&world->retireMarkedBreakable(box));
            }else{
                // A callback cannot conceal a nested ownership failure by
                // returning true to its enclosing event.
                assert(!world->publishBoxAlarm0({box.slot,box.generation+1},17));
            }
            return true;
        })&&!world->effectsSucceeded()&&queries==1);
        if(!retire)assert(world->findBreakable(box)->alarm0==5);
        assert(!world->stopBoxAlarm0(box,[&](bool&){++queries;return true;})&&queries==1);
    }
    std::printf("Gameplay collision owner bytes (host): %zu\n",sizeof(GameplayCollisionOwner));
    std::puts("Native collision owner: actor/terrain births, dormant flags, movement, compaction, retirement, Retry and terminal failure passed");
}
#endif
