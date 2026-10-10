#ifndef DOWNWELL_CORE_STARTUP_RUNTIME_H
#define DOWNWELL_CORE_STARTUP_RUNTIME_H

#include "AreaOneBuilder.h"
#include "BlastWallContact.h"
#include "BlastShootableContact.h"
#include "InstanceStore.h"
#include "StoredBuilderPublication.h"
#include "StoredShopCreationOwner.h"
#include "RegisteredScatteredContact.h"
#include "StoredSpawnerStates.h"
#include "StoredWhiteJellyStates.h"
#include "RunnerLayerIds.h"
#include "ScriptEffectRuntimeBinding.h"
#include "BoxStepDispatch.h"
#include "DeferredInstanceQueue.h"
#include "GameMakerRandom.h"
#include "GameplayLimits.h"
#include "GemMotion.h"
#include "ProjectileAnimation.h"
#include "ProjectileViewRetirement.h"
#include "ProjectileSegment.h"
#include "LaserSightRay.h"
#include "MachineGunEmitter.h"
#include "MachineGunSourceStep.h"
#include "CurrencyAccount.h"
#include "CurrencySpawnScript.h"
#include "CurrencyGainAccountOperations.h"
#include "MainCameraMotion.h"
#include "PlayerDeathSequence.h"
#include "RunSession.h"
#include "PlayerLifeRules.h"
#include "Input.h"
#include "PlayerMotion.h"
#include "StoredTimeShardRegistration.h"
#include "StoredLimboShardRegistration.h"
#include "StoredCorpseRegistration.h"
#include "StoredLevelNumberRegistration.h"
#include "LevelNumberDrawOps.h"
#include "LevelNumberText.h"
#include "StoredCorpseExpiryRegistration.h"
#include "StoredSmokeRegistration.h"
#include "StoredConsumptionBloodRegistration.h"
#include "StoredFlashBallRegistration.h"
#include "MomentDelay.h"
#include "StoredTimeChangeRegistration.h"
#include "StoredEntryDitherRegistration.h"
#include "GameplayDitherAbove.h"
#include "StoredDroneBulletRegistration.h"
#include "StoredBoxChainPublication.h"
#include "StoredCurrencyRegistration.h"
#include "StoredBoneRegistration.h"
#include "StoredBloodRegistration.h"
#include "StoredSkeletonRegistration.h"
#include "StoredBlastRegistration.h"
#include "BlastSkeletonContact.h"
#include "RegisteredSkeletonThrow.h"
#include "RegisteredSkeletonStomp.h"
#include "BoneViewRetirement.h"
#include "StoredScriptSpawnRegistration.h"
#include "StoredDebrisRegistration.h"
#include "StoredRoomLayers.h"
#include "RoomEntryViews.h"
#include "EventScheduler.h"
#include "ScriptSpawnOperations.h"
#include "EffectSpawnScript.h"
#include "TimeChangeEffects.h"
#include "TimeStopEffects.h"
#include "TimeShardBurst.h"
#include "RunnerCollisionEventContext.h"
#include "TimeFieldWind.h"
#include "PlayerCompanionCreate.h"
#include "SmokeDirection.h"
#include "SmokeTrig.h"
#include <array>
#include <cstdint>
#include <cstring>

namespace downwell {

class RoomPack;
class ObjectPack;
class BuilderPatternPack;
struct FrogAlarmEffect;
enum class RoomInstancePhase;

enum class PlatformProfile {
    desktop,
    nonDesktop
};

// Minimal, auditable model of the recovered rmInit startup path. This is kept
// separate from presentation so its tick behavior can be tested exactly.
class StartupRuntime {
public:
    void skipIntroForTesting();
    void skipToGameForTesting();
    struct MainBulletState {
        float x = 0.0F;
        float y = 0.0F;
        bool gemHigh = false;
        ProjectileAnimation animation;
        MachineGunTrajectory trajectory;
        InstanceHandle instance{};
        MachineGunSourceMotion sourceMotion{};
        bool sourceOwned=false;
    };
    struct MainCurrencyState {
        GemMotion motion;
        float x = 0.0F, y = 0.0F, speedX = 0.0F, speedY = 0.0F;
        std::int32_t amount = 0;
        unsigned age = 0;
        bool large = false;
        bool attracted = false;
    };

    static constexpr std::size_t instanceCapacity = 2048;
    static constexpr std::size_t mainBulletCapacity = gameplayBulletCapacity;
    static constexpr std::size_t mainCurrencyCapacity = gameplayGemCapacity;
    static constexpr int entryRoomId = 7;
    static constexpr int playMenuRoomId = 8;
    static constexpr int mainRoomId = 11;
    static constexpr int resultRoomId = 16;
    static constexpr int menuRoomId = 20;
    static constexpr std::uint32_t controllerInstanceId = 100065;
    static constexpr std::uint32_t initRoomInstanceId = 100066;
    static constexpr int surfaceWidth = 160;
    static constexpr int surfaceHeight = 284;
    static constexpr std::uint32_t defaultRandomSeed = 2;

    explicit StartupRuntime(PlatformProfile profile = PlatformProfile::desktop,
                            const RoomPack* roomPack = nullptr,
                            const ObjectPack* objectPack = nullptr,
                            std::uint32_t randomSeed = defaultRandomSeed,
                            const BuilderPatternPack* builderPatternPack = nullptr,
                            InstanceLifecycleBinding lifecycle = {});

    void reset(PlatformProfile profile);
    // Resource cache visible to room-entry events, not a current-frame follow query.
    const RoomEntryViews::View* roomEntryView(unsigned slot=0)const{
        return roomEntryViews_.get(slot);
    }
    // Call at a room-transition boundary, after consuming the results action.
    // Seed is explicit so randomize can be recorded/replayed by the platform.
    bool restartFromResults(DeathMenuSequence::Action action, std::uint32_t seed);
    template<unsigned Capacity,unsigned Registered> bool restartFromResultsWithScriptEffects(
        DeathMenuSequence::Action action,std::uint32_t seed,
        StoredScriptSpawnRegistration<instanceCapacity,Registered>& registration,ScriptSpawnEffects<Capacity>& effects){
        if(!run_.effectsSucceeded||resultsDestination(action)==ResultsDestination::none)return false;
        if(!registration.belongsTo(instances_)||!effects.boundTo(registration)||!effects.valid()||!effects.clear(registration))
            return run_.effectsSucceeded=false;
        return restartFromResults(action,seed);
    }
    void step(bool advancePressed);
    void step(const Input& input, bool advancePressed);

    std::uint64_t tick() const { return tick_; }
    int roomId() const { return roomId_; }
    int roomDestination() const { return roomDestination_; }
    int showSplash() const { return showSplash_; }
    unsigned splashDitherFrame() const {
        return static_cast<unsigned>(splashDitherTenths_ / 10);
    }
    int alarm0() const;
    int initTimes() const { return initTimes_; }
    int achievementLoginCalls() const { return achievementLoginCalls_; }
    bool controllerPersistent() const;
    std::size_t activeInstanceCount() const { return instances_.size(); }
    // Source-owned mixed layer traversal. Object flags below are the admitted
    // stored-runtime fields, not a replacement for unimplemented VM setters.
    template<class Visit,class Background> bool visitSourceDrawChecked(Visit visit,Background background)const{
        if(!roomPack_||!objectPack_)return false;
        return roomLayers_.visitDraw(
            [&](InstanceHandle key,std::uint32_t id,RunnerLayerDraw::Element& out){
                const auto* r=instances_.resolve(key);
                if(!r||r->instanceId!=id)return false;
                PackedObject object;
                out.object=objectPack_->findObject(r->objectId,object);
                out.flags=(!r->active?2u:0u)|(r->destroyRequested?1u:0u)|
                    (out.object&&object.visible()?0x10u:0u);
                PackedObjectEvent event;
                out.drawEvent=out.object&&objectPack_->findEvent(r->objectId,8,0,event);
                // A runtime sprite_index=-1 suppresses default drawing; the
                // object resource's original sprite is not a fallback.
                out.sprite=r->spriteId>=0;
                return true;
            },[&](InstanceHandle key,std::uint32_t id,int depth,RunnerLayerDraw::Action action){
                const auto* r=instances_.resolve(key);
                return r&&r->instanceId==id&&visit(key,*r,depth,action);
            },background);
    }
    // Instance-only projection for actor and offscreen debris passes. Their
    // surrounding renderer still owns background/terrain/surface composition.
    template<class Visit> bool visitDrawChecked(Visit visit)const{
        return visitDrawAtDepthChecked([&](InstanceHandle key,const InstanceRecord& r,int){return visit(key,r);});
    }
    template<class Visit> bool visitDrawAtDepthChecked(Visit visit)const{
        if(!roomPack_)return instances_.visitDrawChecked([&](InstanceHandle key,const InstanceRecord& r){
            return visit(key,r,r.depth); // Explicit resource-less fixtures.
        });
        return visitSourceDrawChecked([&](InstanceHandle key,const InstanceRecord& r,int depth,auto){
            return visit(key,r,depth);
        },[](const RunnerLayerBackground&,int){return true;});
    }
    bool hasInstance(std::uint32_t instanceId) const {
        return instances_.find(instanceId) != nullptr;
    }
    const InstanceRecord* instance(std::uint32_t instanceId) const {
        return instances_.find(instanceId);
    }
    // Stable slot/generation lookup follows compaction without an ID scan.
    const InstanceRecord* resolveInstance(InstanceHandle handle) const {
        return instances_.resolve(handle);
    }
    // Script field writes do not remove collision membership. objBox_n Step
    // owns destruction; a retained selection must resolve its lifetime again
    // after every synchronous Create, rather than retaining a record pointer.
    bool publishBoxHealth(InstanceHandle h,MotionBinary64 health){
        auto* r=instances_.resolve(h);
        if(!run_.effectsSucceeded||!r||r->objectId!=92||!health.valid())
            return run_.effectsSucceeded=false;
        r->box.wallHp=health;return true;
    }
    int overlapSound(unsigned channel)const{return channel<11?run_.overlapSounds[channel]:-1;}
    template<class Audio> bool playSkeletonStompSound(SkeletonActor& actor,
        SoundPlayEnvironment& environment,Audio& audio){
        return run_.playSkeletonStompSound(actor.sound,environment,audio);
    }
    unsigned chooseSkeletonExplosion(unsigned count){
        if(!run_.effectsSucceeded||count!=3){run_.effectsSucceeded=false;return 3;}
        return random_.chooseIndexSmall(3);
    }
    template<unsigned Capacity,unsigned SmokeCapacity,class Events,class Audio>
    bool stompSkeleton(StoredBloodRegistration<instanceCapacity,Capacity>& blood,
        StoredSmokeRegistration<instanceCapacity,SmokeCapacity>& smoke,
        SkeletonActor& actor,Events& events,SoundPlayEnvironment& environment,Audio& audio,bool& good){
        if(!run_.effectsSucceeded)return good=false;
        const bool ok=registeredSkeletonStomp(*this,actor,events,[&]{return emitSkeletonBlood(blood,actor);},
            [&](auto x,auto y,auto direction,auto speed){return emitRegisteredSmoke(smoke,actor.smokeEmitter,x,y,direction,speed);},
            environment,audio,good);
        return run_.effectsSucceeded=ok;
    }
    template<unsigned Capacity,unsigned SmokeCapacity,unsigned BlastCapacity,class Events,class Audio>
    bool stompSkeleton(StoredBloodRegistration<instanceCapacity,Capacity>& blood,
        StoredSmokeRegistration<instanceCapacity,SmokeCapacity>& smoke,
        StoredBlastRegistration<instanceCapacity,BlastCapacity>& blasts,
        SkeletonActor& actor,Events& events,SoundPlayEnvironment& environment,Audio& audio,bool& good){
        if(!run_.effectsSucceeded)return good=false;
        const bool ok=registeredSkeletonStomp(*this,actor,events,[&]{return emitSkeletonBlood(blood,actor);},
            [&](auto x,auto y,auto direction,auto speed){return emitRegisteredSmoke(smoke,actor.smokeEmitter,x,y,direction,speed);},
            [&](auto x,auto y){InstanceHandle child;return spawnBlast(blasts,x,y,child);},environment,audio,good);
        return run_.effectsSucceeded=ok;
    }
    template<class Audio> bool playSkeletonSound(SkeletonActor& actor,int sound,
        SoundPlayEnvironment& environment,Audio& audio){
        if(!run_.effectsSucceeded||roomId_!=mainRoomId||cameraViewHeight_!=surfaceHeight)
            return run_.effectsSucceeded=false;
        // Ordinary Step reads the previous Draw's published view, not the
        // followed camera instance after its own Step has moved it.
        if(!multiplyBinary64(MotionBinary64::fromInteger(ordinaryStepViewTopQ12_),{0x3f300000u,0},
            environment.viewY))return run_.effectsSucceeded=false;
        environment.viewHeight=MotionBinary64::fromInteger(cameraViewHeight_);
        environment.y=MotionBinary64::fromInteger(actor.y);
        return run_.playSkeletonSound(actor.sound,sound,environment,audio);
    }
    template<class Material,class Audio> bool playBulletWallSound(InstanceHandle shot,
        SoundPlayEnvironment& environment,Material material,Audio& audio){
        auto* caller=instances_.resolve(shot);
        if(!run_.effectsSucceeded||!caller||caller->objectId!=261)return run_.effectsSucceeded=false;
        return run_.playBulletWallSound(caller->sound,environment,random_,material,audio);
    }
    template<class Tick> bool withMomentDelayRoomSpeed(int& roomSpeed,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(momentDelayRoomSpeed_)return run_.effectsSucceeded=false;
        momentDelayRoomSpeed_=&roomSpeed;
        const bool okay=tick();momentDelayRoomSpeed_=nullptr;
        return run_.effectsSucceeded=okay&&run_.effectsSucceeded;
    }
    bool requestMomentDelay();
    bool applyPlayerDamageSlowdown(){
        if(!run_.effectsSucceeded)return false;
        return visitDamageControllers([&](auto& controller){
            if(!momentDelayRoomSpeed_)return false;
            *momentDelayRoomSpeed_=40;
            controller.setSlow(23);return controller.setAlarm(0,40);
        });
    }
    template<class Apply> bool visitDamageControllers(Apply apply){
        if(!run_.effectsSucceeded)return false;
        if(!objectPack_)return run_.effectsSucceeded=false;
        bool okay=true;
        instances_.visitObject(37,[&](int object,int parent){
            for(unsigned depth=0;depth<objectPack_->objectCount();++depth){
                if(object==parent)return true;
                PackedObject definition;
                if(!objectPack_->findObject(object,definition)){okay=false;return false;}
                if(definition.parentId<0)return false;
                object=definition.parentId;
            }
            okay=false;return false;
        },[&](InstanceHandle key,const InstanceRecord& record){
            if(!okay)return false;
            if(!record.active||record.destroyRequested)return true;
            return okay=withPlayerDamageController([&]{return instances_.resolve(key);},apply);
        });
        return run_.effectsSucceeded=okay;
    }
    bool noKillAchievement()const{return run_.noKill;}
    bool noDamageAchievement()const{return run_.noDamage;}
    bool areaNoDamageAchievement()const{return run_.areaNoDamage;}
    void clearNoDamageAchievement(){run_.noDamage=false;}
    void clearAreaNoDamageAchievement(){run_.areaNoDamage=false;}
    bool playerDamaged()const{return run_.playerDamaged;}
    bool playerDeathActive()const{return mainDeath_.active;}
    bool playerHealthPositive()const{return mainPlayerHealth_>0;}
    void setPlayerDamaged(){run_.playerDamaged=true;}
    unsigned playerHit()const{return run_.playerHit;}
    void setPlayerHit(){run_.setPlayerHit();}
    // The source object-selector resolver supplies the selected lifetime.
    bool setPlayerDamageAlarm(InstanceHandle key,int ticks){
        if(!run_.effectsSucceeded)return false;
        auto* player=instances_.resolve(key);
        if(!player||player->objectId!=35)return run_.effectsSucceeded=false;
        player->alarms.set(1,ticks);return true;
    }
    MotionBinary64 oxygen()const{return run_.oxygen;}
    bool setOxygen(MotionBinary64 value){return run_.setOxygen(value);}
    template<class Audio> bool duckMusic(MusicDuck& caller,int duration,MotionBinary64 amount,Audio& audio){
        return run_.duckMusic(caller,duration,amount,audio);
    }
    template<class Audio,class Delay,class Cue>
    bool recordEnemyDeath(bool cutGunshot,Audio& audio,Delay delay,Cue cue){
        return run_.recordEnemyDeath(cutGunshot,audio,delay,cue);
    }
    // Explicit publication provider retained for failure-prefix reference tests.
    template<class Audio,class NoKill,class Delay,class Cue>
    bool recordEnemyDeath(bool cutGunshot,Audio& audio,NoKill clearNoKill,Delay delay,Cue cue){
        return run_.recordEnemyDeath(cutGunshot,audio,clearNoKill,delay,cue);
    }
    template<class Create,class AreaMusic,class Audio>
    bool beginLevelCue(bool& firstLand,bool playerTimeStop,int& bgm,MotionBinary64 x,MotionBinary64 y,
        Create create,AreaMusic music,SoundPlayState& caller,SoundPlayEnvironment& env,Audio& audio){
        return run_.beginLevelCue(firstLand,playerTimeStop,bgm,x,y,create,music,caller,env,audio);
    }
    template<unsigned Capacity,class Audio> bool playCurrencyPickupSound(
        StoredCurrencyRegistration<instanceCapacity,Capacity>& currency,InstanceHandle gem,
        MotionBinary64 volume,Audio& audio){
        auto* caller=instances_.resolve(gem);
        if(!run_.effectsSucceeded||!currency.belongsTo(instances_)||!currency.find(gem)||!caller)
            return run_.effectsSucceeded=false;
        return run_.playCurrencyPickupSound(caller->sound,volume,audio);
    }
    template<class Audio> bool playBoxChainSound(InstanceHandle h,MotionBinary64 volume,bool inWater,Audio& audio){
        auto* box=instances_.resolve(h);
        if(!run_.effectsSucceeded||!box||box->objectId!=92)return run_.effectsSucceeded=false;
        return run_.playBoxChainSound(box->sound,volume,inWater,audio);
    }
    template<class Audio> bool playBoxBreakSound(InstanceHandle h,MotionBinary64 volume,Audio& audio){
        auto* box=instances_.resolve(h);
        if(!run_.effectsSucceeded||!box||box->objectId!=92)return run_.effectsSucceeded=false;
        return run_.playBoxBreakSound(box->sound,box->box.breakSound,volume,audio);
    }
    template<class Audio> bool playBoxBreakSound(InstanceHandle h,int sound,MotionBinary64 volume,Audio& audio){
        auto* box=instances_.resolve(h);
        if(!run_.effectsSucceeded||!box||box->objectId!=92)return run_.effectsSucceeded=false;
        return run_.playBoxBreakSound(box->sound,sound,volume,audio);
    }
    bool publishBoxAlarm0(InstanceHandle h,std::int32_t value){
        auto* record=instances_.resolve(h);
        if(!run_.effectsSucceeded||!record||record->objectId!=92)return run_.effectsSucceeded=false;
        record->alarms.set(0,value);return true;
    }
    template<class Query> bool stopBoxAlarm0(InstanceHandle h,Query timeBound){
        if(!run_.effectsSucceeded)return false;
        auto* record=instances_.resolve(h);
        if(!record||record->objectId!=92)return run_.effectsSucceeded=false;
        bool allowed=false;
        if(!timeBound(allowed))return run_.effectsSucceeded=false;
        // The query is an integration callback. Resolve the retained lifetime
        // again before publishing, and never clear a failure raised within it.
        record=instances_.resolve(h);
        if(!run_.effectsSucceeded||!record||record->objectId!=92)return run_.effectsSucceeded=false;
        auto alarm=record->alarms.get(0);
        run_.effectsSucceeded=record->box.stopAlarmZero(allowed,alarm);
        if(run_.effectsSucceeded)record->alarms.set(0,alarm);
        return run_.effectsSucceeded;
    }
    bool publishBoxHit(InstanceHandle h){
        auto* r=instances_.resolve(h);
        if(!run_.effectsSucceeded||!r||r->objectId!=92)return run_.effectsSucceeded=false;
        r->box.hit=true;return true;
    }
    template<class Read,class Operations,class Tick>
    bool withBoxDestruction(Read read,Operations operations,Tick tick){
        return boxStepDispatch_.run(run_.effectsSucceeded,read,operations,tick);
    }
    bool markBoxDestroyed(InstanceHandle h){
        auto* r=instances_.resolve(h);
        if(!run_.effectsSucceeded||!r||r->objectId!=92)return run_.effectsSucceeded=false;
        if(r->destroyRequested)return true;
        if(lifecycle_.hasOwner()&&!lifecycle_.markDestroyed(h,*r))return run_.effectsSucceeded=false;
        r->destroyRequested=true;return true;
    }
    bool markBulletDestroyed(InstanceHandle h){
        auto* r=instances_.resolve(h);
        if(!run_.effectsSucceeded||!r||r->objectId!=261)return run_.effectsSucceeded=false;
        if(r->destroyRequested)return true;
        if(lifecycle_.hasOwner()&&!lifecycle_.markDestroyed(h,*r))return run_.effectsSucceeded=false;
        r->destroyRequested=true;return true;
    }
    InstanceHandle instanceHandleAt(std::size_t index)const{return instances_.handleAt(index);}
    StoredBuilderPublication<instanceCapacity> builderPublication(){
        return {instances_,lifecycle_,run_.effectsSucceeded};
    }
    bool publishWallImage(InstanceHandle h,int frame){
        auto* r=instances_.resolve(h);
        if(!run_.effectsSucceeded||!r||r->objectId!=60||frame<0||frame>15)
            return run_.effectsSucceeded=false;
        if(!lifecycle_.phaseIndex(h,*r,unsigned(frame)*256u))return run_.effectsSucceeded=false;
        r->imageIndex=static_cast<float>(frame);return true;
    }
    bool requestWallDestroy(InstanceHandle h){
        auto* r=instances_.resolve(h);
        if(!run_.effectsSucceeded||!r||r->objectId!=60)return run_.effectsSucceeded=false;
        if(r->destroyRequested)return true;
        if(!lifecycle_.markDestroyed(h,*r))return run_.effectsSucceeded=false;
        r->destroyRequested=true;return true;
    }
    // Converted global.lowSpec / global.ending values. Default desktop startup
    // uses false for both; settings and ending owners publish changes here.
    void configureDitherAboveEnvironment(bool lowSpec,bool ending){
        ditherAboveLowSpec_=lowSpec;ditherAboveEnding_=ending;
    }
    bool captureDitherAbove(GameplayDitherAbove::Snapshot& snapshot)const;
    bool deactivateWall(InstanceHandle h){
        const auto* r=instances_.resolve(h);
        if(!run_.effectsSucceeded||!r||r->objectId!=60)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=instances_.requestActivation(h,false,
            [&](auto key,const auto& record,bool active){return lifecycle_.activation(key,record,active);});
    }
    bool retireMarkedWall(InstanceHandle h){
        const auto* r=instances_.resolve(h);
        if(!run_.effectsSucceeded||!r||r->objectId!=60||!r->destroyRequested)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=instances_.erase(r->instanceId,
            [&](auto key,const auto& record){return lifecycle_.retire(key,record);});
    }
    template<class Ops> bool wallEndStepEvent(InstanceHandle h,int area,Ops& ops){
        auto* r=instances_.resolve(h);
        if(!run_.effectsSucceeded||!r)return run_.effectsSucceeded=false;
        if(!r->active||r->destroyRequested)return true;
        if(r->objectId==99)return true;
        if(r->objectId!=60)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=wallEndStep(r->wall,area,random_,ops);
    }
    const InstanceRecord& instanceAt(std::size_t index) const {
        return instances_.at(index);
    }
    const SpawnerStep* spawnerState(unsigned id)const{return spawners_.state(id);}
    bool sourceFallDepth(MotionBinary64& value)const{
        if(!run_.effectsSucceeded||!mainPlayerInitialized_||roomId_!=mainRoomId)return false;
        value=run_.fallDepth;return true;
    }
    template<class MakeOps,class Tick> bool withSpawnerEvents(MakeOps makeOps,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(spawnerDispatch_)return run_.effectsSucceeded=false;
        auto dispatch=[&](InstanceHandle key){
            auto ops=makeOps(key);return spawners_.step(key,random_,ops);
        };
        spawnerContext_=&dispatch;
        spawnerDispatch_=[](void* context,InstanceHandle key){
            return (*static_cast<decltype(dispatch)*>(context))(key);
        };
        const bool ok=tick();spawnerDispatch_=nullptr;spawnerContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    // Use the stored world's actual player/run globals while borrowing the
    // remaining query and child-creation operations. The pre-movement player
    // global is deliberately distinct from ordinary Step's fall-depth value.
    template<class MakeOps,class Tick> bool withWorldSpawnerEvents(MakeOps makeOps,Tick tick){
        return withSpawnerEvents([&](InstanceHandle key){
            auto external=makeOps(key);
            struct Ops : decltype(external) {
                StartupRuntime& world;
                Ops(decltype(external) value,StartupRuntime& runtime):decltype(external)(value),world(runtime){}
                bool playerY(MotionBinary64& value){
                    MotionBinary64 x;
                    return world.mainPlayerInitialized_&&world.roomId_==mainRoomId&&
                        ContactCoordinates::playerGlobals(world.mainPlayerMotion_.frameStartXFixed(),
                            world.mainPlayerMotion_.frameStartYFixed(),world.mainPlayerOriginX_,world.mainPlayerOriginY_,x,value);
                }
                bool area(int& value){value=world.run_.stage.area;return true;}
                bool fallDepth(MotionBinary64& value){return world.sourceFallDepth(value);}
                bool death(bool& value){value=world.mainDeath_.active;return true;}
                bool endReached(bool& value){value=world.mainBuilderEndReached_;return true;}
            };
            return Ops{external,*this};
        },tick);
    }
    // Query ownership must have been attached before these room lifetimes
    // were born. Publish mask_index through the normal lifecycle before using
    // the spawner as the temporary-position caller of either family query.
    template<class Queries,class MakeEnvironment,class Tick>
    bool withRegisteredSpawnerEvents(Queries& queries,MakeEnvironment makeEnvironment,Tick tick){
        if(!queries.boundTo(*this))return run_.effectsSucceeded=false;
        return withWorldSpawnerEvents([&](InstanceHandle key){
            auto external=makeEnvironment(key);
            struct Ops : decltype(external) {
                StartupRuntime& world;Queries& queries;InstanceHandle key;
                Ops(decltype(external) value,StartupRuntime& runtime,Queries& owner,InstanceHandle h)
                    :decltype(external)(value),world(runtime),queries(owner),key(h){}
                bool enemyCount(int& value){
                    unsigned count;if(!queries.instanceNumber(83,count))return false;
                    value=int(count);return true;
                }
                bool objectMask(int object,int& value){
                    PackedObject resource;
                    if(!world.objectPack_||!world.objectPack_->findObject(object,resource))return false;
                    value=resource.textureMaskId;return true;
                }
                bool mask(int value){
                    auto* r=world.instances_.resolve(key);
                    if(!r||r->objectId!=78||!world.lifecycle_.mask(key,*r,value))return false;
                    r->maskId=value;return true;
                }
                bool meeting(MotionBinary64 x,MotionBinary64 y,int object,bool& hit){
                    const auto* r=world.instances_.resolve(key);
                    return r&&r->objectId==78&&queries.placeMeeting(key,*r,object,x,y,hit);
                }
                bool create(int object,MotionBinary64 x,MotionBinary64 y,unsigned& id){
                    return world.createSpawnerEnemy(object,x,y,id);
                }
                bool deactivate(unsigned id){return world.deactivateSpawnerEnemy(id);}
            };
            return Ops{external,*this,queries,key};
        },tick);
    }
    template<unsigned Capacity> StoredWhiteJellyStates<instanceCapacity,Capacity> whiteJellyStates(){
        return StoredWhiteJellyStates<instanceCapacity,Capacity>(instances_);
    }
    // Bind recovered events to their resource dispatch, not a second actor loop.
    // The caller supplies real builtin/effect operations and the current player
    // pTimeStop read. This scope does not admit unimplemented contact/Draw events.
    template<unsigned Capacity,class MakeOps,class ReadStop,class Tick>
    bool withWhiteJellyEvents(StoredWhiteJellyStates<instanceCapacity,Capacity>& owner,
        MakeOps makeOps,ReadStop readStop,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(whiteJellyDispatch_||!owner.belongsTo(instances_)||!owner.valid())return run_.effectsSucceeded=false;
        auto dispatch=[&](InstanceHandle key,int code){
            if(code==1222)return owner.hitStunAlarm(key);
            auto ops=makeOps(key);
            if(code==1221)return owner.create(key,random_,ops);
            if(code==1224)return owner.dispatch(key,[&](auto& s){return s.endStep(ops);});
            bool stop;
            if(!readStop(stop))return false;
            if(code==1223)return owner.dispatch(key,[&](auto& s){return s.effectAlarm(stop,random_,ops);});
            return code==1225&&owner.step(key,stop,random_,ops);
        };
        whiteJellyContext_=&dispatch;
        whiteJellyDispatch_=[](void* p,InstanceHandle key,int code){
            return (*static_cast<decltype(dispatch)*>(p))(key,code);
        };
        const bool ok=tick();whiteJellyDispatch_=nullptr;whiteJellyContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&owner.valid();
    }
    bool spawnWhiteJelly(int x,int y,InstanceHandle& key);
    template<unsigned Capacity,class Queries,class MakeEnvironment,class ReadStop,class Tick>
    bool withRegisteredWhiteJellyEvents(StoredWhiteJellyStates<instanceCapacity,Capacity>& owner,Queries& queries,
        MakeEnvironment makeEnvironment,ReadStop readStop,Tick tick){
        if(!queries.boundTo(*this))return run_.effectsSucceeded=false;
        return withWhiteJellyEvents(owner,[&](InstanceHandle key){
            auto external=makeEnvironment(key);
            struct Ops : decltype(external) {
                StartupRuntime& world;Queries& queries;InstanceHandle key;
                Ops(decltype(external) value,StartupRuntime& runtime,Queries& collision,InstanceHandle h)
                    :decltype(external)(value),world(runtime),queries(collision),key(h){}
                InstanceRecord* record(){
                    auto* r=world.instances_.resolve(key);return r&&r->objectId==115?r:nullptr;
                }
                bool position(bool vertical,RunnerImageValue value,RunnerImageValue& accepted){
                    auto* r=record();if(!r)return false;
                    RunnerImageValue x,y;std::memcpy(&x.bits,&r->preciseX,4);std::memcpy(&y.bits,&r->preciseY,4);
                    if(vertical)y=value;else x=value;
                    if(!world.publishCollisionPosition(key,x,y)||(r=record())==nullptr)return false;
                    std::memcpy(&r->preciseX,&x.bits,4);std::memcpy(&r->preciseY,&y.bits,4);
                    accepted=vertical?y:x;return true;
                }
                bool player(bool vertical,MotionBinary64& value){
                    MotionBinary64 x,y;
                    if(!world.mainPlayerInitialized_||world.roomId_!=mainRoomId||
                        !ContactCoordinates::playerGlobals(world.mainPlayerMotion_.frameStartXFixed(),
                            world.mainPlayerMotion_.frameStartYFixed(),world.mainPlayerOriginX_,world.mainPlayerOriginY_,x,y))return false;
                    value=vertical?y:x;return true;
                }
                bool playerX(MotionBinary64& value){return player(false,value);}
                bool playerY(MotionBinary64& value){return player(true,value);}
                bool imageSpeed(RunnerImageValue value,RunnerImageValue& accepted){
                    auto* r=record();if(!r||!value.promoted().valid())return false;
                    std::memcpy(&r->imageSpeed,&value.bits,4);accepted=value;return true;
                }
                bool imageIndex(int value){
                    auto* r=record();RunnerImageValue index;
                    if(!r||!index.assignInteger(value)||!world.lifecycle_.imageIndex(key,*r,index))return false;
                    std::memcpy(&r->imageIndex,&index.bits,4);return true;
                }
                bool sprite(int value){
                    auto* r=record();if(!r||!world.lifecycle_.sprite(key,*r,value))return false;
                    r->spriteId=value;return true;
                }
                bool alarm(int number,int value){
                    auto* r=record();if(!r||number<0||number>=int(AlarmBank::count))return false;
                    r->alarms.set(unsigned(number),value);return true;
                }
                bool meeting(MotionBinary64 x,MotionBinary64 y,int object,bool& hit){
                    const auto* r=record();return r&&queries.placeMeeting(key,*r,object,x,y,hit);
                }
                bool destroy(){
                    auto* r=record();if(!r)return false;
                    if(r->destroyRequested)return true;
                    if(!world.lifecycle_.markDestroyed(key,*r))return false;
                    r->destroyRequested=true;return true;
                }
            };
            return Ops{external,*this,queries,key};
        },readStop,tick);
    }
    // Spawner grids are integral world coordinates. Preserve synchronous
    // Create and the room-wide numeric allocator before returning the child
    // ID; unsupported channel families must not become generic room records.
    bool createSpawnerEnemy(int object,MotionBinary64 x,MotionBinary64 y,unsigned& id);
    bool deactivateSpawnerEnemy(unsigned id);
    bool roomDestinationSet() const { return roomDestinationSet_; }
    bool roomLoadSucceeded() const { return roomLoadSucceeded_; }
    bool recoveredEventBindingsActive() const { return objectPack_ != nullptr; }
    std::uint32_t implementedCodeCalls() const { return implementedCodeCalls_; }
    std::uint32_t unimplementedCodeCalls() const { return unimplementedCodeCalls_; }
    int cameraViewWidth() const { return cameraViewWidth_; }
    int cameraViewHeight() const { return cameraViewHeight_; }
    int backgroundMusicId() const { return run_.backgroundMusicId; }
    bool firstLand()const{return run_.firstLand;}
    bool controlsLocked() const { return controlsLocked_; }
    std::uint32_t soundGainRequests() const { return soundGainRequests_; }
    std::uint32_t soundPlayRequests() const { return soundPlayRequests_; }
    std::uint32_t ditherFadeSpawnRequests() const {
        return ditherFadeSpawnRequests_;
    }
    std::uint32_t nextRuntimeInstanceId() const { return nextRuntimeInstanceId_; }
    const RunnerLayerIds& sourceLayerIds()const{return layerIds_;}
    const auto& sourceRoomLayers()const{return roomLayers_;}
    bool deferredCommandsSucceeded() const { return deferredCommandsSucceeded_; }
    // Engine x/y setter for collision response. Keeps private actor motion and
    // preciseX/Y motion caches untouched; the collision owner stores exact x/y.
    // Objects 61 and 261 instead mirror published builtins in preciseX/Y.
    bool publishCollisionPosition(InstanceHandle h,RunnerImageValue x,RunnerImageValue y);
    bool publishBulletCollisionPosition(InstanceHandle h,RunnerImageValue x,RunnerImageValue y){
        const auto* record=instances_.resolve(h);
        if(!run_.effectsSucceeded||!record||record->objectId!=261)return run_.effectsSucceeded=false;
        return publishCollisionPosition(h,x,y);
    }
    bool publishBulletAngle(InstanceHandle h,RunnerImageValue angle);
    bool inMenu() const { return roomId_ == menuRoomId; }
    bool inPlayMenu() const { return roomId_ == playMenuRoomId; }
    bool inMainRoom() const { return roomId_ == mainRoomId; }
    // scrTimeShard's identity-bearing query at published x/y. Each call runs
    // the maintained search again; missing ownership is a terminal failure.
    bool playerTimeFieldInstance(RunnerImageValue x,RunnerImageValue y,InstancePlaceResult& result);
    bool playerTimeFieldMeeting(MotionBinary64 x,MotionBinary64 y,bool& hit);
    bool playerOutsideTimeStopSides(MotionBinary64 x,bool& outside);
    bool timeShardOutsideView(RunnerImageValue publishedY,bool& retire);
    // Install a collision pass for exactly one ordinary tick. The scheduler
    // invokes it after Step reconciliation and before End Step. No callback or
    // borrowed state survives this call, including on terminal failure.
    template<class Collisions> bool stepWithCollisionPass(const Input& input,bool advancePressed,Collisions collisions){
        if(!run_.effectsSucceeded)return false;
        if(collisionDispatch_){run_.effectsSucceeded=false;return false;}
        collisionContext_=&collisions;
        collisionDispatch_=[](void* context){return (*static_cast<Collisions*>(context))();};
        step(input,advancePressed);
        collisionDispatch_=nullptr;collisionContext_=nullptr;
        return run_.effectsSucceeded&&roomLoadSucceeded_&&deferredCommandsSucceeded_;
    }
    struct NoCollisionPass {bool operator()()const{return true;}};
    enum class DronePhase {animation,alarm,step,endStep};
    enum class CurrencyPhase {animation,alarm,step,cleanup};
    template<unsigned Capacity,class DrawFx>
    bool visitRegisteredCurrencyDraw(StoredCurrencyRegistration<instanceCapacity,Capacity>& registration,DrawFx drawFx){
        if(!run_.effectsSucceeded||!registration.belongsTo(instances_)||!registration.valid())
            return run_.effectsSucceeded=false;
        const bool ok=visitDrawAtDepthChecked([&](InstanceHandle key,const InstanceRecord& record,int depth){
            if(record.objectId!=248||!record.active||record.destroyRequested)return true;
            return registration.draw(key,[&](auto h,const auto& r,const auto& state){return drawFx(h,r,state,depth);});
        });
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    template<unsigned Capacity,class Execute,class Tick>
    bool withCurrencyEvents(StoredCurrencyRegistration<instanceCapacity,Capacity>& registration,Execute execute,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(currencyDispatch_||!registration.belongsTo(instances_)||!registration.valid())return run_.effectsSucceeded=false;
        auto dispatch=[&](const InstanceRecord& record,CurrencyPhase phase,unsigned index){
            const auto key=registration.handle(record);if(!key.generation)return false;
            if(phase==CurrencyPhase::animation)return registration.advanceAnimation(key);
            if(phase==CurrencyPhase::cleanup)return registration.retire(key);
            if(phase==CurrencyPhase::alarm)return registration.alarm(key,index,[&](auto& state,unsigned alarm,bool& good){
                return execute(key,state,phase,alarm,random_,good);});
            return registration.dispatch(key,[&](auto& state,bool& good){return execute(key,state,phase,index,random_,good);});
        };
        currencyContext_=&dispatch;
        currencyDispatch_=[](void* p,const InstanceRecord& r,CurrencyPhase phase,unsigned index){return (*static_cast<decltype(dispatch)*>(p))(r,phase,index);};
        const bool ok=tick();currencyDispatch_=nullptr;currencyContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&registration.valid();
    }
    enum class BonePhase {animation,step,cleanup};
    enum class BloodPhase {animation,step,cleanup};
    template<unsigned Capacity,class Query,class Tick>
    bool withBloodEvents(StoredBloodRegistration<instanceCapacity,Capacity>& registration,Query query,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(bloodDispatch_||!registration.belongsTo(instances_)||!registration.valid())return run_.effectsSucceeded=false;
        auto dispatch=[&](const InstanceRecord& record,BloodPhase phase){
            const auto key=registration.handle(record);if(!key.generation)return false;
            if(phase==BloodPhase::animation)return registration.advanceAnimation(key);
            if(phase==BloodPhase::cleanup)return registration.retire(key);
            return registration.step(key,[&](const auto& child,bool& allowed){return query(key,child,allowed);});
        };
        bloodContext_=&dispatch;
        bloodDispatch_=[](void* p,const InstanceRecord& r,BloodPhase phase){
            return (*static_cast<decltype(dispatch)*>(p))(r,phase);
        };
        const bool ok=tick();bloodDispatch_=nullptr;bloodContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&registration.valid();
    }
    enum class SkeletonPhase {animation,alarm,step,endStep,cleanup};
    enum class CorpsePhase {animation,alarm,step,endStep,cleanup};
    template<unsigned Capacity,unsigned TextCapacity=64>
    StoredLevelNumberRegistration<instanceCapacity,Capacity,TextCapacity> levelNumberRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity,unsigned TextCapacity,class AreaMusic,class Audio>
    bool beginLevelCueWithTitle(bool& firstLand,bool stopped,int& bgm,MotionBinary64 x,MotionBinary64 y,
        StoredLevelNumberRegistration<instanceCapacity,Capacity,TextCapacity>& registration,
        const LevelNumberTextContext& text,AreaMusic music,SoundPlayState& caller,SoundPlayEnvironment& env,Audio& audio){
        return run_.beginLevelCue(firstLand,stopped,bgm,x,y,[&](int object,auto px,auto py){
            InstanceHandle child;
            return object==372&&registration.belongsTo(instances_)&&registration.create(px,py,
                [&](auto,auto& state){return text.initialize(state,run_.stage);},child);
        },music,caller,env,audio);
    }
    template<unsigned Capacity,unsigned TextCapacity,unsigned TimeCapacity,class Audio>
    bool beginLevelCueWithTitle(const TimeStopEffects<TimeCapacity>& stop,MotionBinary64 x,MotionBinary64 y,
        StoredLevelNumberRegistration<instanceCapacity,Capacity,TextCapacity>& registration,
        const LevelNumberTextContext& text,SoundPlayState& caller,SoundPlayEnvironment& env,Audio& audio){
        return beginLevelCueWithTitle(run_.firstLand,stop.player.stopped,run_.backgroundMusicId,x,y,
            registration,text,RunSession::areaMusic,caller,env,audio);
    }
    template<unsigned Capacity,unsigned TextCapacity,unsigned TimeCapacity,class Audio>
    bool recordSkeletonDeath(SkeletonActor& actor,bool cutGunshot,const TimeStopEffects<TimeCapacity>& stop,
        StoredLevelNumberRegistration<instanceCapacity,Capacity,TextCapacity>& titles,
        const LevelNumberTextContext& text,SoundPlayEnvironment& environment,Audio& audio){
        return recordEnemyDeath(cutGunshot,audio,[&]{return requestMomentDelay();},[&]{
            return beginLevelCueWithTitle(stop,MotionBinary64::fromInteger(actor.x),MotionBinary64::fromInteger(actor.y),
                titles,text,actor.sound,environment,audio);
        });
    }
    template<unsigned Capacity,unsigned TextCapacity,class Tick>
    bool withLevelNumberEvents(StoredLevelNumberRegistration<instanceCapacity,Capacity,TextCapacity>& registration,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(levelNumberDispatch_||!registration.belongsTo(instances_)||!registration.valid())return run_.effectsSucceeded=false;
        auto dispatch=[&](const InstanceRecord& record,bool cleanup,unsigned alarm){
            const auto key=registration.handle(record);if(!key.generation)return false;
            return cleanup?registration.retire(key):registration.alarm(key,alarm);
        };
        levelNumberContext_=&dispatch;
        levelNumberDispatch_=[](void* context,const InstanceRecord& record,bool cleanup,unsigned alarm){
            return (*static_cast<decltype(dispatch)*>(context))(record,cleanup,alarm);
        };
        const bool okay=tick();levelNumberDispatch_=nullptr;levelNumberContext_=nullptr;
        return run_.effectsSucceeded=okay&&run_.effectsSucceeded&&registration.valid();
    }
    // Borrow private title Draw state only for the enclosing render. The
    // renderer still walks the shared layers; this is not a separate pass.
    template<unsigned Capacity,unsigned TextCapacity,class Render> bool withLevelNumberDraw(
        StoredLevelNumberRegistration<instanceCapacity,Capacity,TextCapacity>& registration,Render render){
        if(levelNumberDraw_||!registration.belongsTo(instances_)||!registration.valid())return false;
        levelNumberDrawContext_=&registration;
        levelNumberDraw_=[](void* context,const InstanceRecord& record,LevelNumberDrawOps& ops){
            auto& owner=*static_cast<StoredLevelNumberRegistration<instanceCapacity,Capacity,TextCapacity>*>(context);
            const auto key=owner.handle(record);
            return key.generation&&owner.draw(key,ops);
        };
        const bool okay=render();levelNumberDraw_=nullptr;levelNumberDrawContext_=nullptr;
        return okay&&registration.valid();
    }
    template<class Ops> bool drawLevelNumber(const InstanceRecord& record,Ops& ops)const{
        if(!levelNumberDraw_||record.objectId!=372)return false;
        LevelNumberDrawOps bridge(ops);
        return levelNumberDraw_(levelNumberDrawContext_,record,bridge);
    }
    // Corpse state is borrowed under the same enclosing-render lifetime.
    template<unsigned Capacity,class Render> bool withCorpseDraw(
        StoredCorpseRegistration<instanceCapacity,Capacity>& registration,Render render){
        if(corpseDrawState_||!registration.belongsTo(instances_)||!registration.valid())return false;
        corpseDrawContext_=&registration;
        corpseDrawState_=[](void* context,const InstanceRecord& record)->const CorpseAnimation*{
            auto& owner=*static_cast<StoredCorpseRegistration<instanceCapacity,Capacity>*>(context);
            return owner.state(owner.handle(record));
        };
        const bool ok=render();corpseDrawContext_=nullptr;corpseDrawState_=nullptr;
        return ok&&registration.valid();
    }
    const CorpseAnimation* corpseForDraw(const InstanceRecord& record)const{
        return corpseDrawState_?corpseDrawState_(corpseDrawContext_,record):nullptr;
    }
    template<unsigned Capacity,class Render> bool withLimboShardDraw(
        StoredLimboShardRegistration<instanceCapacity,Capacity>& registration,Render render){
        if(limboShardDrawState_||!registration.belongsTo(instances_)||!registration.valid())return false;
        limboShardDrawContext_=&registration;
        limboShardDrawState_=[](void* context,const InstanceRecord& record)->const LimboShard*{
            auto& owner=*static_cast<StoredLimboShardRegistration<instanceCapacity,Capacity>*>(context);
            return owner.state(owner.handle(record));
        };
        const bool ok=render();limboShardDrawContext_=nullptr;limboShardDrawState_=nullptr;
        return ok&&registration.valid();
    }
    const LimboShard* limboShardForDraw(const InstanceRecord& record)const{
        return limboShardDrawState_?limboShardDrawState_(limboShardDrawContext_,record):nullptr;
    }
    enum class SmokePhase {animation,alarm,step,cleanup};
    enum class BlastPhase {animation,alarm,step,cleanup};
    template<unsigned Capacity> StoredBlastRegistration<instanceCapacity,Capacity> blastRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity> bool spawnBlast(StoredBlastRegistration<instanceCapacity,Capacity>& registry,
        MotionBinary64 x,MotionBinary64 y,InstanceHandle& key){
        if(!run_.effectsSucceeded||!registry.belongsTo(instances_))return run_.effectsSucceeded=false;
        struct Services {
            StartupRuntime& world;
            bool shake(int a,int d){return world.requestCameraShake(a,d);}
            bool momentDelay(){return world.requestMomentDelay();}
            bool choose(unsigned count,unsigned& choice){if(count!=2)return false;choice=world.random_.chooseIndexSmall(2);return true;}
        } services{*this};
        return run_.effectsSucceeded=registry.create(x,y,services,key);
    }
    template<unsigned Capacity,class Tick>
    bool withBlastEvents(StoredBlastRegistration<instanceCapacity,Capacity>& registry,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(blastDispatch_||!registry.belongsTo(instances_)||!registry.valid())return run_.effectsSucceeded=false;
        auto dispatch=[&](const InstanceRecord& record,BlastPhase phase,unsigned index){
            const auto key=registry.handle(record);
            if(phase==BlastPhase::animation)return registry.advanceAnimation(key);
            if(phase==BlastPhase::cleanup)return registry.retire(key);
            if(phase==BlastPhase::alarm)return registry.alarm(key,index);
            return registry.step(key);
        };
        blastContext_=&dispatch;
        blastDispatch_=[](void* p,const InstanceRecord& r,BlastPhase phase,unsigned index){return (*static_cast<decltype(dispatch)*>(p))(r,phase,index);};
        const bool ok=tick();blastDispatch_=nullptr;blastContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&registry.valid();
    }
    enum class LimboShardPhase {animation,step,cleanup};
    template<unsigned Capacity,class Environment,class Tick> bool withLimboShardEvents(
        StoredLimboShardRegistration<instanceCapacity,Capacity>& registration,Environment& environment,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(limboShardDispatch_||!registration.belongsTo(instances_)||!registration.valid())return run_.effectsSucceeded=false;
        auto dispatch=[&](const InstanceRecord& record,LimboShardPhase phase){
            const auto key=registration.handle(record);
            if(!key.generation)return false;
            if(phase==LimboShardPhase::animation)return registration.advanceAnimation(key);
            if(phase==LimboShardPhase::cleanup)return registration.retire(key);
            bool stopped=false;int width=0;LimboShard::Player player;
            return environment.prepare(record,stopped,width,player)&&
                registration.step(key,stopped,width,player,random_,environment);
        };
        limboShardContext_=&dispatch;
        limboShardDispatch_=[](void* p,const InstanceRecord& r,LimboShardPhase phase){
            return (*static_cast<decltype(dispatch)*>(p))(r,phase);
        };
        const bool ok=tick();limboShardDispatch_=nullptr;limboShardContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&registration.valid();
    }
    template<unsigned Capacity,class Tick> bool withFlashBallEvents(
        StoredFlashBallRegistration<instanceCapacity,Capacity>& registration,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(flashBallDispatch_||!registration.belongsTo(instances_)||!registration.valid())return run_.effectsSucceeded=false;
        auto dispatch=[&](const InstanceRecord& record,bool cleanup){
            const auto key=registration.handle(record);
            return key.generation&&(cleanup?registration.retire(key):registration.advanceAnimation(key));
        };
        flashBallContext_=&dispatch;
        flashBallDispatch_=[](void* p,const InstanceRecord& r,bool cleanup){return (*static_cast<decltype(dispatch)*>(p))(r,cleanup);};
        const bool ok=tick();flashBallDispatch_=nullptr;flashBallContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&registration.valid();
    }
    enum class ConsumptionBloodPhase {animation,alarm,step,cleanup};
    template<unsigned Capacity,class Globals,class Tick> bool withConsumptionBloodEvents(
        StoredConsumptionBloodRegistration<instanceCapacity,Capacity>& registration,Globals& globals,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(consumptionBloodDispatch_||!registration.belongsTo(instances_)||!registration.valid())return run_.effectsSucceeded=false;
        auto dispatch=[&](const InstanceRecord& record,ConsumptionBloodPhase phase,unsigned index){
            if(phase==ConsumptionBloodPhase::animation)return registration.advanceAnimation(record);
            if(phase==ConsumptionBloodPhase::cleanup)return registration.retire(record);
            if(phase==ConsumptionBloodPhase::alarm)return index==0&&registration.kill(record);
            return registration.step(record,globals);
        };
        consumptionBloodContext_=&dispatch;
        consumptionBloodDispatch_=[](void* p,const InstanceRecord& r,ConsumptionBloodPhase phase,unsigned index){
            return (*static_cast<decltype(dispatch)*>(p))(r,phase,index);
        };
        const bool ok=tick();consumptionBloodDispatch_=nullptr;consumptionBloodContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&registration.valid();
    }
    template<unsigned Capacity,class Tick>
    bool withSmokeEvents(StoredSmokeRegistration<instanceCapacity,Capacity>& registration,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(smokeDispatch_||!registration.belongsTo(instances_)||!registration.valid())return run_.effectsSucceeded=false;
        auto dispatch=[&](const InstanceRecord& record,SmokePhase phase,unsigned index){
            if(phase==SmokePhase::animation)return registration.advanceAnimation(record);
            if(phase==SmokePhase::cleanup)return registration.retire(record);
            if(phase==SmokePhase::alarm)return registration.alarmEvent(record,index);
            return registration.step(record,random_);
        };
        smokeContext_=&dispatch;
        smokeDispatch_=[](void* p,const InstanceRecord& r,SmokePhase phase,unsigned index){
            return (*static_cast<decltype(dispatch)*>(p))(r,phase,index);
        };
        const bool ok=tick();smokeDispatch_=nullptr;smokeContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&registration.valid();
    }
    template<unsigned Capacity,class Tick>
    bool withCorpseExpiryEvents(StoredCorpseExpiryRegistration<instanceCapacity,Capacity>& registration,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(corpseExpiryDispatch_||!registration.belongsTo(instances_)||!registration.valid())return run_.effectsSucceeded=false;
        auto dispatch=[&](const InstanceRecord& record,bool cleanup){
            return cleanup?registration.retire(record):registration.advanceAnimation(record);
        };
        corpseExpiryContext_=&dispatch;
        corpseExpiryDispatch_=[](void* p,const InstanceRecord& r,bool cleanup){
            return (*static_cast<decltype(dispatch)*>(p))(r,cleanup);
        };
        const bool ok=tick();corpseExpiryDispatch_=nullptr;corpseExpiryContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&registration.valid();
    }
    template<unsigned Capacity,class Execute,class Tick>
    bool withCorpseEvents(StoredCorpseRegistration<instanceCapacity,Capacity>& registration,Execute execute,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(corpseDispatch_||!registration.belongsTo(instances_)||!registration.valid())return run_.effectsSucceeded=false;
        auto dispatch=[&](const InstanceRecord& record,CorpsePhase phase,unsigned index){
            const auto key=registration.handle(record);if(!key.generation)return false;
            if(phase==CorpsePhase::animation)return registration.advanceAnimation(key);
            if(phase==CorpsePhase::cleanup)return registration.retire(key);
            if(phase==CorpsePhase::endStep){
                MotionBinary64 top;
                if(!multiplyBinary64(MotionBinary64::fromInteger(ordinaryStepViewTopQ12_),{0x3f300000u,0},top))return false;
                return registration.endStep(key,top,roomId_==mainRoomId);
            }
            return registration.dispatchPublished(key,[&](auto& state,auto& publish,bool& good){
                return execute(key,phase,index,state,publish,good);
            });
        };
        corpseContext_=&dispatch;
        corpseDispatch_=[](void* p,const InstanceRecord& r,CorpsePhase phase,unsigned index){
            return (*static_cast<decltype(dispatch)*>(p))(r,phase,index);
        };
        const bool ok=tick();corpseDispatch_=nullptr;corpseContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&registration.valid();
    }
    template<unsigned Capacity,class Execute,class Tick>
    bool withSkeletonPublishedEvents(StoredSkeletonRegistration<instanceCapacity,Capacity>& registration,
        Execute execute,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(skeletonDispatch_||!registration.belongsTo(instances_)||!registration.valid())return run_.effectsSucceeded=false;
        auto dispatch=[&](const InstanceRecord& record,SkeletonPhase phase,unsigned index){
            const auto key=registration.handle(record);if(!key.generation)return false;
            if(phase==SkeletonPhase::animation)return registration.advanceAnimation(key);
            if(phase==SkeletonPhase::alarm)return registration.alarm(key,index);
            if(phase==SkeletonPhase::cleanup)return registration.retire(key);
            if(phase==SkeletonPhase::endStep){
                if(roomId_!=mainRoomId)return false;
                MotionBinary64 top;
                if(!multiplyBinary64(MotionBinary64::fromInteger(ordinaryStepViewTopQ12_),{0x3f300000u,0},top))return false;
                return registration.endStep(key,top,true);
            }
            return registration.dispatchPublished(key,[&](auto& actor,auto& throwing,auto& publish,bool& good){
                return execute(key,phase,actor,throwing,publish,good);
            });
        };
        skeletonContext_=&dispatch;
        skeletonDispatch_=[](void* p,const InstanceRecord& r,SkeletonPhase phase,unsigned index){
            return (*static_cast<decltype(dispatch)*>(p))(r,phase,index);
        };
        const bool ok=tick();skeletonDispatch_=nullptr;skeletonContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&registration.valid();
    }
    template<unsigned Capacity,unsigned StopCapacity,class MakeEffects,class Tick>
    bool withSkeletonTimeStopEvents(StoredSkeletonRegistration<instanceCapacity,Capacity>& registration,
        const TimeStopEffects<StopCapacity>& stop,MakeEffects makeEffects,Tick tick){
        return withSkeletonTimeStopPublishedEvents(registration,stop,
            [&](auto key,auto& actor,auto& throwing,auto&){return makeEffects(key,actor,throwing);},tick);
    }
    template<unsigned Capacity,unsigned StopCapacity,class MakeEffects,class Tick>
    bool withSkeletonTimeStopPublishedEvents(StoredSkeletonRegistration<instanceCapacity,Capacity>& registration,
        const TimeStopEffects<StopCapacity>& stop,MakeEffects makeEffects,Tick tick){
        if(!stop.succeeded)return run_.effectsSucceeded=false;
        return withSkeletonPublishedEvents(registration,[&](InstanceHandle key,SkeletonPhase phase,
            auto& actor,auto& throwing,auto& publish,bool& good){
            MotionBinary64 x,y;
            if(phase!=SkeletonPhase::step||!stop.succeeded||!mainPlayerInitialized_||roomId_!=mainRoomId||
               !ContactCoordinates::playerGlobals(mainPlayerMotion_.frameStartXFixed(),mainPlayerMotion_.frameStartYFixed(),
                   mainPlayerOriginX_,mainPlayerOriginY_,x,y))return good=false;
            auto effects=makeEffects(key,actor,throwing,publish);
            return actor.stepPublished(stop.player.stopped,x,y,publish,effects,good);
        },tick);
    }
    // Registries and publisher remain borrowed for this selected event. Child
    // registration uses the stored room's real allocator and synchronous Create.
    template<class Publish,class Bones,class Blood,class Smoke,class Flash,class Currency,class Corpses,class Titles,
             unsigned StopCapacity,class Tile,class Audio>
    auto registeredSkeletonEffects(SkeletonActor& actor,RegisteredSkeletonThrow& throwing,Publish& publish,
        Bones& bones,Blood& blood,Smoke& smoke,Flash& flash,Currency& currency,Corpses& corpses,Titles& titles,
        const TimeStopEffects<StopCapacity>& stop,bool cutGunshot,const LevelNumberTextContext& text,
        Tile& tile,SoundPlayEnvironment& environment,Audio& audio){
        struct Effects {
            StartupRuntime& world;SkeletonActor& actor;RegisteredSkeletonThrow& throwing;Publish& publish;
            Bones& bones;Blood& bloods;Smoke& smokes;Flash& flashes;Currency& gems;Corpses& corpses;Titles& titles;
            const TimeStopEffects<StopCapacity>& stop;bool cut;const LevelNumberTextContext& text;
            Tile& tile;SoundPlayEnvironment& environment;Audio& audio;
            bool hurtSound(int sound){return world.playSkeletonSound(actor,sound,environment,audio);}
            bool throwBone(int){return world.throwSkeletonBone(bones,actor,throwing,environment,audio);}
            bool enemyDeath(){return world.recordSkeletonDeath(actor,cut,stop,titles,text,environment,audio);}
            bool blood(int count,int spread){return count==10&&spread==3&&world.emitSkeletonBlood(bloods,actor);}
            bool smoke(MotionBinary64,MotionBinary64,int count,int spread){return count==5&&spread==1&&
                world.emitSkeletonSmoke(smokes,actor);}
            bool flash(MotionBinary64,MotionBinary64,int count,int spread,int radius){return count==6&&spread==2&&radius==10&&
                world.emitSkeletonFlash(flashes,actor);}
            bool currency(int amount){return amount==actor.money&&world.emitSkeletonCurrency(gems,actor,tile);}
            bool deathSound(int sound){return world.playSkeletonSound(actor,sound,environment,audio);}
            bool deadBody(int sprite,MotionBinary64 impact,int explosion){return sprite==206&&explosion==102&&
                world.emitSkeletonCorpse(corpses,publish,impact,tile);}
            bool debris(int x,int y){std::uint32_t id=0;return world.spawnDebris(MotionBinary64::fromInteger(x),
                MotionBinary64::fromInteger(y),world.runStage().area,id);}
        };
        return Effects{*this,actor,throwing,publish,bones,blood,smoke,flash,currency,corpses,titles,
            stop,cutGunshot,text,tile,environment,audio};
    }
    template<unsigned Capacity,class Bones,class Blood,class Smoke,class Flash,class Currency,class Corpses,class Titles,
             unsigned StopCapacity,class Tile,class Audio,class Tick>
    bool withRegisteredSkeletonEvents(StoredSkeletonRegistration<instanceCapacity,Capacity>& skeletons,
        Bones& bones,Blood& blood,Smoke& smoke,Flash& flash,Currency& currency,Corpses& corpses,Titles& titles,
        const TimeStopEffects<StopCapacity>& stop,bool cutGunshot,const LevelNumberTextContext& text,
        Tile tile,SoundPlayEnvironment& environment,Audio& audio,Tick tick){
        return withSkeletonTimeStopPublishedEvents(skeletons,stop,[&](auto,auto& actor,auto& throwing,auto& publish){
            return registeredSkeletonEffects(actor,throwing,publish,bones,blood,smoke,flash,currency,corpses,titles,
                stop,cutGunshot,text,tile,environment,audio);
        },tick);
    }
    template<unsigned Capacity,class Execute,class Tick>
    bool withSkeletonEvents(StoredSkeletonRegistration<instanceCapacity,Capacity>& registration,
        Execute execute,Tick tick){
        return withSkeletonPublishedEvents(registration,[&](InstanceHandle key,SkeletonPhase phase,
            auto& actor,auto& throwing,auto&,bool& good){return execute(key,phase,actor,throwing,good);},tick);
    }
    // Install one bone owner around a single caller-supplied tick. This allows
    // the time-stop and bone families to share the same animation/Begin Step/
    // ordinary Step/collision pass instead of advancing separate world ticks.
    template<unsigned Capacity,class Execute,class Tick>
    bool withBoneEvents(StoredBoneRegistration<instanceCapacity,Capacity>& registration,
        Execute execute,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(boneDispatch_||!registration.belongsTo(instances_)||!registration.valid())return run_.effectsSucceeded=false;
        auto dispatch=[&](const InstanceRecord& record,BonePhase phase){
            const auto key=registration.handle(record);
            if(!key.generation)return false;
            if(phase==BonePhase::animation)return registration.advanceAnimation(key);
            if(phase==BonePhase::cleanup)return registration.retire(key);
            return registration.dispatch(key,[&](auto& state,auto& publisher,bool& good){
                return execute(key,state,publisher,random_,good);
            });
        };
        boneContext_=&dispatch;
        boneDispatch_=[](void* context,const InstanceRecord& record,BonePhase phase){
            return (*static_cast<decltype(dispatch)*>(context))(record,phase);
        };
        const bool ok=tick();
        boneDispatch_=nullptr;boneContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&registration.valid();
    }
    template<unsigned Capacity,class Execute,class Collisions=NoCollisionPass>
    bool stepWithBones(StoredBoneRegistration<instanceCapacity,Capacity>& registration,
        const Input& input,Execute execute,bool advancePressed=false,Collisions collisions={}){
        return withBoneEvents(registration,[&](InstanceHandle,auto& state,auto& publisher,auto& random,bool& good){
            return execute(state,publisher,random,good);
        },[&]{return stepWithCollisionPass(input,advancePressed,collisions);});
    }
    // Scoped dispatch joins the actual room phases. The event adapter must
    // supply Step behavior; main-room End Step uses the phase-published view.
    template<unsigned Capacity,class Execute,class Tick>
    bool withDroneBulletEvents(StoredDroneBulletRegistration<instanceCapacity,Capacity>& registration,
        Execute execute,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(droneDispatch_||!registration.belongsTo(instances_)||!registration.valid())
            return run_.effectsSucceeded=false;
        auto dispatch=[&](const InstanceRecord& record,DronePhase phase){
            const auto key=registration.handle(record);if(!key.generation)return false;
            if(phase==DronePhase::animation)return registration.advanceAnimation(key);
            if(phase==DronePhase::alarm)return registration.alarm(record);
            if(phase==DronePhase::endStep){
                // Other rooms and dirty-camera setters need their own published
                // view adapter; do not substitute a main-room follow position.
                if(roomId_!=mainRoomId)return false;
                return registration.endStep(key,MotionBinary64::fromInteger(ordinaryStepViewTopQ12_/4096),
                    MotionBinary64::fromInteger(surfaceHeight),true);
            }
            return registration.dispatch(key,[&](auto& state,auto& publisher,bool& good){
                return execute(state,publisher,phase,random_,good);
            });
        };
        droneContext_=&dispatch;
        droneDispatch_=[](void* context,const InstanceRecord& record,DronePhase phase){
            return (*static_cast<decltype(dispatch)*>(context))(record,phase);
        };
        const bool ok=tick();
        droneDispatch_=nullptr;droneContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&registration.valid();
    }
    template<unsigned Capacity,class Execute>
    bool stepWithDroneBullets(StoredDroneBulletRegistration<instanceCapacity,Capacity>& registration,
        const Input& input,Execute execute,bool advancePressed=false){
        return withDroneBulletEvents(registration,execute,
            [&]{return stepWithCollisionPass(input,advancePressed,NoCollisionPass{});});
    }
    template<class Stopped,class Collisions=NoCollisionPass>
    bool stepWithOwnedDebris(const Input& input,Stopped stopped,bool advancePressed=false,Collisions collisions={}){
        return stepWithDebris(ownedDebrisRegistration_,ownedDebrisEffects_,input,stopped,advancePressed,collisions);
    }
    template<unsigned Capacity,unsigned Registered,class Stopped,class Collisions=NoCollisionPass>
    bool stepWithDebris(StoredDebrisRegistration<instanceCapacity,Registered>& registration,DebrisEffects<Capacity>& effects,
        const Input& input,Stopped stopped,bool advancePressed=false,Collisions collisions={}){
        if(!run_.effectsSucceeded)return false;
        if(debrisEventDispatch_||!registration.belongsTo(instances_)||!effects.valid())return run_.effectsSucceeded=false;
        if(debrisBinding_.effects!=&effects||debrisBinding_.registry!=&registration){
            if(hasDebrisRecords())return run_.effectsSucceeded=false;
            bindDebrisDraw(effects,registration);
        }
        auto dispatch=[&](const InstanceRecord& record,StepEvent event){
            if(event.phase==StepEventPhase::alarm)return registration.dispatchAlarmEvent(record,effects,event.subtype);
            if(event.phase==StepEventPhase::step)return registration.dispatchScheduledStep(record,effects,random_,stopped);
            return true; // Object 370 has no Begin/End Step body.
        };
        debrisEventContext_=&dispatch;
        debrisEventDispatch_=[](void* context,const InstanceRecord& record,StepEvent event){
            return (*static_cast<decltype(dispatch)*>(context))(record,event);
        };
        const bool ok=stepWithCollisionPass(input,advancePressed,collisions);
        debrisEventDispatch_=nullptr;debrisEventContext_=nullptr;return ok;
    }
    // Script effects join the real active-room animation traversal, not a
    // separate family-wide pass. The callback is scoped to this ordinary tick.
    template<unsigned Capacity,unsigned Registered,class Collisions=NoCollisionPass>
    bool stepWithScriptEffects(StoredScriptSpawnRegistration<instanceCapacity,Registered>& registration,
        ScriptSpawnEffects<Capacity>& effects,const Input& input,bool advancePressed=false,Collisions collisions={}){
        if(!run_.effectsSucceeded)return false;
        if(scriptAnimationDispatch_||!registration.belongsTo(instances_)||!effects.boundTo(registration)||!effects.valid()){
            run_.effectsSucceeded=false;return false;
        }
        const auto phase=effects.beginPhase();
        auto dispatch=[&](InstanceHandle key,const InstanceRecord& record){
            return registration.dispatchAnimation(key,record,effects,phase);
        };
        scriptAnimationContext_=&dispatch;
        scriptAnimationDispatch_=[](void* context,InstanceHandle key,const InstanceRecord& record){
            return (*static_cast<decltype(dispatch)*>(context))(key,record);
        };
        const bool ok=stepWithCollisionPass(input,advancePressed,collisions);
        scriptAnimationDispatch_=nullptr;scriptAnimationContext_=nullptr;
        return ok;
    }
    // Scoped animation/alarm/Step ownership for one ordinary world tick. No borrowed
    // callback survives the call. The optional collision pass shares this tick;
    // callers must route all admitted handlers, not silently discard unknowns.
    // The optional
    // transition callback runs at the shared player's source Begin Step boundary.
    struct NoTimeTransition {bool operator()(const InstanceRecord&,const PlayerMotion&,int,int)const{return true;}};
    enum class TimeEffectPhase {animation,alarm,step,playerBegin,cleanup};
    // Scoped admission while entering a room: the player's recovered Create
    // invokes this owner after scrSpawnCamera, not after the room has loaded.
    // The caller keeps the registry alive and retires it before room teardown.
    template<unsigned Capacity,class CameraX,class Operation>
    bool withEntryDitherRoomCreation(StoredEntryDitherRegistration<instanceCapacity,Capacity>& registration,
        CameraX cameraX,Operation operation){
        if(!run_.effectsSucceeded)return false;
        if(entryDitherCreate_||entryDitherDispatch_||!registration.belongsTo(instances_))
            return run_.effectsSucceeded=false;
        auto create=[&](int room){
            InstanceHandle key;
            return registration.createInRoom(room,cameraX,[&](MotionBinary64& out){
                const auto* view=roomEntryViews_.get(0);
                if(!view)return false;
                out=view->y.promoted();return true;
            },key);
        };
        auto dispatch=[&](InstanceRecord& record,bool alarm){
            const auto key=registration.handle(record);
            return key.generation&&(alarm?registration.alarm(key):registration.step(key,cameraX));
        };
        entryDitherCreateContext_=&create;
        entryDitherCreate_=[](void* p,int room){return (*static_cast<decltype(create)*>(p))(room);};
        entryDitherContext_=&dispatch;
        entryDitherDispatch_=[](void* p,InstanceRecord& r,bool alarm){return (*static_cast<decltype(dispatch)*>(p))(r,alarm);};
        const bool okay=operation();
        entryDitherCreate_=nullptr;entryDitherCreateContext_=nullptr;
        entryDitherDispatch_=nullptr;entryDitherContext_=nullptr;
        return run_.effectsSucceeded=okay&&run_.effectsSucceeded&&roomLoadSucceeded_&&deferredCommandsSucceeded_;
    }
    template<unsigned Capacity,class CameraX,class Collisions=NoCollisionPass>
    bool stepWithEntryDither(StoredEntryDitherRegistration<instanceCapacity,Capacity>& registration,
        CameraX cameraX,const Input& input,bool advancePressed=false,Collisions collisions={}){
        if(!run_.effectsSucceeded)return false;
        if(entryDitherDispatch_||!registration.belongsTo(instances_))return run_.effectsSucceeded=false;
        auto dispatch=[&](InstanceRecord& record,bool alarm){
            const auto key=registration.handle(record);
            return key.generation&&(alarm?registration.alarm(key):registration.step(key,cameraX));
        };
        entryDitherContext_=&dispatch;
        entryDitherDispatch_=[](void* p,InstanceRecord& r,bool alarm){return (*static_cast<decltype(dispatch)*>(p))(r,alarm);};
        const bool ok=stepWithCollisionPass(input,advancePressed,collisions);
        entryDitherDispatch_=nullptr;entryDitherContext_=nullptr;return ok;
    }
    template<unsigned Capacity,class Other>
    bool cleanupEntryDither(StoredEntryDitherRegistration<instanceCapacity,Capacity>& registration,Other other){
        if(!run_.effectsSucceeded)return false;
        return run_.effectsSucceeded=registration.belongsTo(instances_)&&instances_.cleanupDestroyed(
            [&](InstanceHandle key,const InstanceRecord& record){
                return record.objectId==73?registration.retire(key):other(key,record);
            });
    }
    template<unsigned Capacity,unsigned Registered,unsigned Changes,class Transition=NoTimeTransition,class Collisions=NoCollisionPass> bool stepWithTimeStopEffects(
        StoredTimeShardRegistration<instanceCapacity,Registered>& shards,
        StoredTimeChangeRegistration<instanceCapacity,Changes>& changes,TimeStopEffects<Capacity>& state,
        const Input& input,bool advancePressed=false,Transition transition={},Collisions collisions={}){
        if(!run_.effectsSucceeded)return false;
        if(timeEffectDispatch_||(momentDelayRoomSpeed_&&momentDelayRoomSpeed_!=&state.roomSpeed)||
            !state.succeeded||!state.shards.succeeded()||!state.changes.succeeded()||
            !shards.belongsTo(instances_)||!changes.belongsTo(instances_)||
            !state.shards.phasesBoundTo(shards)||!state.changes.boundTo(changes)){
            run_.effectsSucceeded=false;return false;
        }
        auto dispatch=[&](InstanceRecord& record,TimeEffectPhase phase,unsigned index){
            if(phase==TimeEffectPhase::cleanup){
                const auto slot=static_cast<unsigned>(&record-&instances_.at(0));
                if(slot>=instances_.size())return false;
                const auto key=instances_.handleAt(slot);
                return record.objectId==238?shards.retireMarked(key,record,state.shards):
                    record.objectId==211&&changes.retireMarked(key,record,state.changes);
            }
            if(phase==TimeEffectPhase::playerBegin)return transition(record,mainPlayerMotion_,mainPlayerOriginX_,mainPlayerOriginY_);
            if(phase==TimeEffectPhase::animation)return shards.dispatchAnimation(record,state.shards);
            if(phase==TimeEffectPhase::step)return shards.dispatchStep(record,state.shards,state.player.stopped,random_,
                [&](const auto& effect,bool& retire){return timeShardOutsideView(effect.y,retire);});
            return record.objectId==238?shards.dispatchAlarm(record,state.shards,index):
                changes.dispatchAlarm(record,state.changes,index,state.roomSpeed);
        };
        timeEffectContext_=&dispatch;
        timeEffectDispatch_=[](void* context,InstanceRecord& record,TimeEffectPhase phase,unsigned index){
            return (*static_cast<decltype(dispatch)*>(context))(record,phase,index);
        };
        // Controller Alarm 0 and object-211 alarms mutate the same source
        // global in the room's mixed alarm order. Never copy it into a second
        // moment-delay clock; retain an enclosing binding only if it aliases.
        int* previousRoomSpeed=momentDelayRoomSpeed_;
        momentDelayRoomSpeed_=&state.roomSpeed;
        const bool ok=stepWithCollisionPass(input,advancePressed,collisions);
        momentDelayRoomSpeed_=previousRoomSpeed;
        timeEffectDispatch_=nullptr;timeEffectContext_=nullptr;
        return ok;
    }
    // Compose the actual player Begin Step with the existing mixed room phases.
    // Audio queries and side effects must be supplied by the runtime service;
    // absence of audio is not equivalent to answering every query false.
    template<unsigned Capacity,unsigned Registered,unsigned Changes,class Audio,class Emit,class Collisions=NoCollisionPass>
    bool stepWithPlayerTimeStop(StoredTimeShardRegistration<instanceCapacity,Registered>& shards,
        StoredTimeChangeRegistration<instanceCapacity,Changes>& changes,TimeStopEffects<Capacity>& state,
        const Input& input,bool lowSpec,Audio audio,Emit emit,Collisions collisions={}){
        auto transition=[&](const InstanceRecord& record,const PlayerMotion& motion,int ox,int oy){
            RunnerImageValue x,y;
            std::memcpy(&x.bits,&record.preciseX,4);std::memcpy(&y.bits,&record.preciseY,4);
            typename TimeStopEffects<Capacity>::PlayerPose pose;
            if(!pose.fromPortableMotion(x,y,motion,ox,oy))return run_.effectsSucceeded=false;
            return beginTimeStopAudioChecked(shards,changes,state,pose,lowSpec,audio,emit);
        };
        return stepWithTimeStopEffects(shards,changes,state,input,false,transition,collisions);
    }
    // Source bone Step reads the same pTimeStop state just written by player
    // Begin Step. Transition remains an explicit source player/audio adapter;
    // this does not infer that global from goalStop or shop control locks.
    template<unsigned Bones,unsigned Capacity,unsigned Registered,unsigned Changes,class Frames,
        class Transition,class Collisions=NoCollisionPass>
    bool stepWithTimeStopAndBones(StoredBoneRegistration<instanceCapacity,Bones>& bones,
        StoredTimeShardRegistration<instanceCapacity,Registered>& shards,
        StoredTimeChangeRegistration<instanceCapacity,Changes>& changes,TimeStopEffects<Capacity>& stop,
        const Input& input,Frames frames,Transition transition,Collisions collisions={}){
        return withBoneEvents(bones,[&](InstanceHandle key,auto& state,auto& publisher,auto&,bool& good){
            auto impact=[&](){return spawnBoneImpact(bones,key,frames);};
            auto view=[&](bool& visible){return boneInView(bones,key,visible);};
            return state.stepInWorld(stop.player.stopped,publisher,impact,view,good);
        },[&]{return stepWithTimeStopEffects(shards,changes,stop,input,false,transition,collisions);});
    }
    // Compose blood into that same room tick. Its query reads the player state
    // at the particle's ordinary Step, after the player Begin Step transition.
    template<unsigned Blood,unsigned Bones,unsigned Capacity,unsigned Registered,unsigned Changes,
        class Frames,class Transition,class Collisions=NoCollisionPass>
    bool stepWithTimeStopBonesAndBlood(StoredBloodRegistration<instanceCapacity,Blood>& blood,
        StoredBoneRegistration<instanceCapacity,Bones>& bones,
        StoredTimeShardRegistration<instanceCapacity,Registered>& shards,
        StoredTimeChangeRegistration<instanceCapacity,Changes>& changes,TimeStopEffects<Capacity>& stop,
        const Input& input,Frames frames,Transition transition,Collisions collisions={}){
        return withBloodEvents(blood,[&](InstanceHandle key,const auto&,bool& allowed){
            return bloodTimeAllowed(blood,key,stop.player.stopped,allowed);
        },[&]{return stepWithTimeStopAndBones(bones,shards,changes,stop,input,frames,transition,collisions);});
    }
    // Explicit Begin Step boundary; the scheduler supplies synchronous audio,
    // pause and oxygen effects. Shake updates the registered main camera before
    // notifying the supplied effect callback. Registrations must
    // outlive the effect owner and be retired before room teardown.
    template<unsigned Capacity,unsigned Registered,unsigned Changes,class Audio,class Emit>
    bool beginTimeStop(StoredTimeShardRegistration<instanceCapacity,Registered>& shards,
        StoredTimeChangeRegistration<instanceCapacity,Changes>& changes,TimeStopEffects<Capacity>& state,
        const typename TimeStopEffects<Capacity>::PlayerPose& pose,
        bool lowSpec,Audio audio,Emit emit){
        return beginTimeStopAudioChecked(shards,changes,state,pose,lowSpec,
            [&](PlayerTimeStop::AudioQuery q,bool& value){value=audio(q);return true;},emit);
    }
    template<unsigned Capacity,unsigned Registered,unsigned Changes,class Audio,class Emit>
    bool beginTimeStopAudioChecked(StoredTimeShardRegistration<instanceCapacity,Registered>& shards,
        StoredTimeChangeRegistration<instanceCapacity,Changes>& changes,TimeStopEffects<Capacity>& state,
        const typename TimeStopEffects<Capacity>::PlayerPose& pose,
        bool lowSpec,Audio audio,Emit emit){
        if(!run_.effectsSucceeded)return false;
        run_.effectsSucceeded=shards.belongsTo(instances_)&&changes.belongsTo(instances_)&&
            state.shards.phasesBoundTo(shards)&&state.changes.boundTo(changes)&&
            state.beginStepWithAudioChecked(roomId_==11||roomId_==14,[&](bool& outside){
                    return playerOutsideTimeStopSides(pose.xx,outside);
                },pose,lowSpec,random_,
                [&](bool& hit){return playerTimeFieldMeeting(pose.xx,pose.yy,hit);},
                [&](auto x,auto y,auto& id,auto& fx,auto& fy){
                    InstancePlaceResult result;
                    if(!playerTimeFieldInstance(x,y,result))return false;
                    id=result.id;fx=result.x;fy=result.y;return true;
                },audio,[&](PlayerTimeStop::Effect effect){
                    if(effect==PlayerTimeStop::Effect::shake&&!requestTimeStopCameraShake())return false;
                    if(effect==PlayerTimeStop::Effect::oxygenFloor&&!run_.floorTimeStopOxygen())return false;
                    return emit(effect);
                });
        return run_.effectsSucceeded;
    }
    bool requestTimeStopCameraShake();
    bool requestCameraShake(int amount,int duration);
    int cameraShakeAmount()const{return mainCameraMotion_.shakeAmount;}
    // The caller must keep the returned bridge alive and retire its shards
    // before changing rooms. Phase dispatch is intentionally not supplied here.
    template<unsigned Capacity> StoredTimeShardRegistration<instanceCapacity,Capacity> timeShardRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity> StoredCorpseRegistration<instanceCapacity,Capacity> corpseRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity> StoredCorpseExpiryRegistration<instanceCapacity,Capacity> corpseExpiryRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity> StoredSmokeRegistration<instanceCapacity,Capacity> smokeRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity> StoredConsumptionBloodRegistration<instanceCapacity,Capacity> consumptionBloodRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity> StoredFlashBallRegistration<instanceCapacity,Capacity> flashBallRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity> bool emitSkeletonFlash(StoredFlashBallRegistration<instanceCapacity,Capacity>& registry,
        SkeletonActor& actor){
        if(!run_.effectsSucceeded)return false;
        if(!registry.belongsTo(instances_))return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=flashBurstPublished<6,2,10>(actor.smokeScript,actor.xx,actor.yy,random_,
            [&](int object,auto x,auto y){InstanceHandle child;return object==224&&registry.create(x,y,random_,child);});
    }
    template<unsigned Capacity> StoredLimboShardRegistration<instanceCapacity,Capacity> limboShardRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity> bool createRegisteredLimboShard(StoredLimboShardRegistration<instanceCapacity,Capacity>& registry,
        MotionBinary64 x,MotionBinary64 y,bool lowSpec,InstanceHandle& child){
        if(!registry.belongsTo(instances_))return run_.effectsSucceeded=false;
        return registry.create(x,y,lowSpec,random_,child);
    }
    template<unsigned Capacity,class Views> bool stepRegisteredLimboShard(
        StoredLimboShardRegistration<instanceCapacity,Capacity>& registry,InstanceHandle key,bool stopped,int roomWidth,
        const LimboShard::Player& player,Views& views){
        if(!registry.belongsTo(instances_))return run_.effectsSucceeded=false;
        return registry.step(key,stopped,roomWidth,player,random_,views);
    }
    template<unsigned Capacity,class Globals> bool createRegisteredConsumptionBlood(
        StoredConsumptionBloodRegistration<instanceCapacity,Capacity>& registry,MotionBinary64 x,MotionBinary64 y,
        Globals& globals,std::uint32_t& id){
        if(!registry.belongsTo(instances_))return run_.effectsSucceeded=false;
        return registry.create(x,y,random_,globals,id);
    }
    template<unsigned Capacity> bool createRegisteredSmoke(StoredSmokeRegistration<instanceCapacity,Capacity>& registry,
        MotionBinary64 x,MotionBinary64 y,std::uint32_t& id){
        if(!registry.belongsTo(instances_))return run_.effectsSucceeded=false;
        return registry.create(x,y,random_,id);
    }
    template<unsigned Capacity> bool emitRegisteredSmoke(StoredSmokeRegistration<instanceCapacity,Capacity>& registry,
        EmitSmokeScript& caller,MotionBinary64 x,MotionBinary64 y,MotionBinary64 direction,MotionBinary64 speed){
        if(!run_.effectsSucceeded||!registry.belongsTo(instances_))return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=caller.execute(x,y,direction,speed,
            [&](auto px,auto py,std::uint32_t& id){return registry.create(px,py,random_,id);},
            [&](unsigned id,auto value){return registry.setDirection(id,value);},
            [&](unsigned id,auto value){return registry.setSpeed(id,value);});
    }
    template<unsigned Capacity> bool emitRegisteredSkeletonSmoke(StoredSmokeRegistration<instanceCapacity,Capacity>& registry,
        SmokeBurstCaller& caller,MotionBinary64 x,MotionBinary64 y){
        if(!registry.belongsTo(instances_))return run_.effectsSucceeded=false;
        return registry.skeletonBurst(caller,x,y,random_);
    }
    template<unsigned Capacity,class Source,class Tile> bool createRegisteredCorpseFromDeath(
        StoredCorpseRegistration<instanceCapacity,Capacity>& registry,Source& source,
        int sprite,MotionBinary64 impact,int explosion,Tile tile){
        if(!registry.belongsTo(instances_))return run_.effectsSucceeded=false;
        return registry.createFromDeath(source,sprite,impact,explosion,run_.upgrades.levels[3]!=0,random_,tile);
    }
    template<unsigned Capacity,class Publish,class Tile> bool emitSkeletonCorpse(
        StoredCorpseRegistration<instanceCapacity,Capacity>& registry,Publish& publish,MotionBinary64 impact,Tile tile){
        if(!run_.effectsSucceeded)return false;
        if(!registry.belongsTo(instances_))return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=publish.deadBody([&](auto& source){
            return createRegisteredCorpseFromDeath(registry,source,206,impact,102,tile);
        });
    }
    template<unsigned Capacity> bool emitSkeletonSmoke(StoredSmokeRegistration<instanceCapacity,Capacity>& registry,
        SkeletonActor& actor){
        return emitRegisteredSkeletonSmoke(registry,actor.smokeScript,actor.xx,actor.yy);
    }
    template<unsigned Capacity> StoredTimeChangeRegistration<instanceCapacity,Capacity> timeChangeRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity> StoredEntryDitherRegistration<instanceCapacity,Capacity> entryDitherRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity> StoredDroneBulletRegistration<instanceCapacity,Capacity> droneBulletRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity> StoredCurrencyRegistration<instanceCapacity,Capacity> currencyRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity,class Tile> bool createRegisteredCurrency(
        StoredCurrencyRegistration<instanceCapacity,Capacity>& registry,MotionBinary64 x,MotionBinary64 y,
        Tile tile,InstanceHandle& output){
        if(!registry.belongsTo(instances_))return run_.effectsSucceeded=false;
        return registry.create(x,y,run_.stage.area,run_.upgrades.levels[10]!=0,random_,tile,output);
    }
    template<unsigned Capacity,class Tile> bool emitSkeletonCurrency(
        StoredCurrencyRegistration<instanceCapacity,Capacity>& registry,SkeletonActor& actor,Tile tile){
        return spawnRegisteredCurrency(registry,[&](auto& out){return actor.readCurrencyReward(out);},
            [&](MotionBinary64 value){actor.reward.moniAmount=value;return true;},
            [&](bool value){actor.reward.atrState=value;return true;},
            [&](std::uint32_t value){actor.reward.myGem=value;return true;},
            MotionBinary64::fromInteger(actor.money),tile);
    }
    template<unsigned Capacity,class Ops> bool collectRegisteredCurrency(
        StoredCurrencyRegistration<instanceCapacity,Capacity>& registry,InstanceHandle gem,Ops& ops){
        if(!registry.belongsTo(instances_))return run_.effectsSucceeded=false;
        return registry.collect(gem,random_,ops);
    }
    template<unsigned Capacity,class Position,class Tile>
    bool spawnBoxCurrency(StoredCurrencyRegistration<instanceCapacity,Capacity>& registry,
        InstanceHandle box,MotionBinary64 amount,Position position,Tile tile){
        auto retained=[&]()->InstanceRecord*{
            auto* r=instances_.resolve(box);
            return r&&r->objectId==92?r:nullptr;
        };
        return spawnRegisteredCurrency(registry,
            [&](CurrencySpawnSource<std::uint32_t>& out){
                const auto* r=retained();if(!r)return false;
                // preciseX/Y are private motion caches, not runner builtins.
                // Read the maintained owner, then resolve the caller again.
                if(!position(box,out.x,out.y))return false;
                r=retained();if(!r)return false;
                out.objectIndex=r->objectId;
                out.moniAmount=r->boxReward.moniAmount;out.myGem=r->boxReward.myGem;
                out.atrState=r->boxReward.atrState;out.stomped={};return true;
            },
            [&](MotionBinary64 value){auto* r=retained();if(!r)return false;r->boxReward.moniAmount=value;return true;},
            [&](bool value){auto* r=retained();if(!r)return false;r->boxReward.atrState=value;return true;},
            [&](std::uint32_t value){auto* r=retained();if(!r)return false;r->boxReward.myGem=value;return true;},amount,tile);
    }
    template<unsigned Capacity,class Controls,class Position,class Tile>
    auto boxCurrencyOperations(StoredCurrencyRegistration<instanceCapacity,Capacity>& registry,
        InstanceHandle box,Controls controls,Position position,Tile tile){
        struct Operations:Controls {
            StartupRuntime& world;StoredCurrencyRegistration<instanceCapacity,Capacity>& gems;
            InstanceHandle box;Position position;Tile tile;
            Operations(StartupRuntime& w,decltype(gems) g,InstanceHandle h,Controls c,Position p,Tile t)
                :Controls(c),world(w),gems(g),box(h),position(p),tile(t){}
            bool currency(int amount){return world.spawnBoxCurrency(gems,box,MotionBinary64::fromInteger(amount),position,tile);}
        };
        return Operations(*this,registry,box,controls,position,tile);
    }
    // Source numeric IDs cross this script boundary; the registry's local
    // generation handles stay private to child creation and setters.
    template<unsigned Capacity,class Read,class WriteAmount,class WriteAttracted,class WriteChild,class Tile>
    bool spawnRegisteredCurrency(StoredCurrencyRegistration<instanceCapacity,Capacity>& registry,
        Read read,WriteAmount writeAmount,WriteAttracted writeAttracted,WriteChild writeChild,
        MotionBinary64 amount,Tile tile){
        if(!run_.effectsSucceeded||!registry.belongsTo(instances_)||!registry.valid())
            return run_.effectsSucceeded=false;
        struct Ops {
            using Handle=std::uint32_t;
            StartupRuntime& world;StoredCurrencyRegistration<instanceCapacity,Capacity>& gems;Tile tile;
            bool parent(int object,int& value){
                PackedObject info;
                if(!world.objectPack_||!world.objectPack_->findObject(object,info))return false;
                value=info.parentId;return true;
            }
            bool create(int object,MotionBinary64 x,MotionBinary64 y,Handle& id){
                InstanceHandle key;
                if(object!=248||!world.createRegisteredCurrency(gems,x,y,tile,key))return false;
                const auto* r=world.instances_.resolve(key);
                if(!r)return false;id=r->instanceId;return true;
            }
            InstanceHandle child(Handle id){
                const auto* r=world.instances_.find(id);return r?gems.handle(*r):InstanceHandle{};
            }
            bool amount(Handle id,int value){return gems.amount(child(id),value);}
            bool size(Handle id,int value){return gems.size(child(id),value);}
            bool attracted(Handle id,bool value){return gems.attracted(child(id),value);}
        } ops{*this,registry,tile};
        return CurrencySpawnScript::runRetained(read,writeAmount,writeAttracted,writeChild,amount,ops,run_.effectsSucceeded);
    }
    template<unsigned Capacity,class Collision> auto boxChainPublication(
        StoredDroneBulletRegistration<instanceCapacity,Capacity>& bullets,Collision& collision,InstanceHandle box){
        return StoredBoxChainPublication<instanceCapacity,Capacity,Collision>(instances_,bullets,collision,box,run_.effectsSucceeded);
    }
    template<unsigned Capacity> StoredBloodRegistration<instanceCapacity,Capacity> bloodRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity,class Caller> bool spawnBlood(StoredBloodRegistration<instanceCapacity,Capacity>& registration,
        MotionBinary64 x,MotionBinary64 y,MotionBinary64 angle,Caller caller,InstanceHandle& key){
        if(!run_.effectsSucceeded||!registration.belongsTo(instances_))return run_.effectsSucceeded=false;
        return registration.create(x,y,angle,caller,key);
    }
    template<unsigned Capacity> bool bloodTimeAllowed(StoredBloodRegistration<instanceCapacity,Capacity>& registration,
        InstanceHandle key,bool pTimeStop,bool& allowed){
        if(!run_.effectsSucceeded)return false;
        if(!registration.belongsTo(instances_))return run_.effectsSucceeded=false;
        int width=0;
        if(pTimeStop&&!timeStopRoomWidth(width))return false;
        return run_.effectsSucceeded=registration.timeAllowed(key,pTimeStop,width,allowed);
    }
    // scrBloodfx ignores its count/vary arguments. Each random angle precedes
    // that child's synchronous Create and caller myFx assignment, not a batch.
    template<unsigned Capacity,class Caller> bool spawnBloodBurst(StoredBloodRegistration<instanceCapacity,Capacity>& registration,
        MotionBinary64 x,MotionBinary64 y,Caller caller){
        if(!run_.effectsSucceeded||!registration.belongsTo(instances_)||!registration.valid())return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=bloodBurst(x,y,random_,[&](auto px,auto py,auto angle){
            InstanceHandle key;
            return registration.create(px,py,angle,[&](InstanceHandle child){
                const auto* record=registration.instances().find(child);
                return record&&caller(child,record->sourceId);
            },key);
        });
    }
    template<unsigned Capacity> bool emitSkeletonBlood(StoredBloodRegistration<instanceCapacity,Capacity>& registration,
        SkeletonActor& actor){
        return spawnBloodBurst(registration,MotionBinary64::fromInteger(actor.x),MotionBinary64::fromInteger(actor.y),
            [&](InstanceHandle,unsigned source){actor.myFx=source;return true;});
    }
    template<unsigned Capacity> StoredBoneRegistration<instanceCapacity,Capacity> boneRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity> StoredSkeletonRegistration<instanceCapacity,Capacity> skeletonRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity> bool spawnSkeleton(StoredSkeletonRegistration<instanceCapacity,Capacity>& registration,
        int x,int y,InstanceHandle& key){
        if(!registration.belongsTo(instances_)||!registration.valid())return run_.effectsSucceeded=false;
        return registration.create(x,y,key);
    }
    // Builder-created primitives use the same global ID stream and synchronous
    // resource Create path as room children. Other families must have an owner.
    bool spawnBuilderPrimitive(int object,int x,int y,InstanceHandle& key);
    bool adoptBuilderScattered(const DeferredCreateInstance& command,InstanceHandle& key);
    template<unsigned Capacity> class BuilderOwner {
        StartupRuntime& world_;
        StoredSkeletonRegistration<instanceCapacity,Capacity>& skeletons_;
        StoredBuilderPublication<instanceCapacity> publication_;
    public:
        BuilderOwner(StartupRuntime& world,StoredSkeletonRegistration<instanceCapacity,Capacity>& skeletons)
            :world_(world),skeletons_(skeletons),publication_(world.builderPublication()){}
        template<class Post> bool create(int object,int x,int y,Post post){
            if(!world_.run_.effectsSucceeded)return false;
            if(!skeletons_.belongsTo(world_.instances_)||!skeletons_.valid())return world_.run_.effectsSucceeded=false;
            InstanceHandle key;
            if(object==120){
                InstanceHandle local;
                if(!world_.spawnSkeleton(skeletons_,x,y,local))return false;
                key=skeletons_.worldHandle(local);
            }else if(!world_.spawnBuilderPrimitive(object,x,y,key))return false;
            if(!world_.instances_.resolve(key))return world_.run_.effectsSucceeded=false;
            // The callback receives this child's handle, even if Create itself
            // allocated further children and advanced the numeric allocator.
            return world_.run_.effectsSucceeded=post(key)&&world_.run_.effectsSucceeded;
        }
        bool write(InstanceHandle key,BuilderChildField field,int value){return publication_.write(key,field,value);}
        bool publishBuilderY(int y){return world_.publishBuilderRow(y);}
        bool deactivate(InstanceHandle key){return publication_.deactivate(key);}
        bool rightOfMiddle(InstanceHandle key,int width,bool& right){return publication_.rightOfMiddle(key,width,right);}
    };
    template<unsigned Capacity> BuilderOwner<Capacity> builderOwner(
        StoredSkeletonRegistration<instanceCapacity,Capacity>& skeletons){return {*this,skeletons};}
    template<unsigned Capacity> bool spawnBone(StoredBoneRegistration<instanceCapacity,Capacity>& registration,
        int x,int y,InstanceHandle& key){
        if(!registration.belongsTo(instances_)||!registration.valid())return run_.effectsSucceeded=false;
        return registration.create(x,y,random_,key);
    }
    template<unsigned Capacity,class Sound> bool throwRegisteredBone(
        StoredBoneRegistration<instanceCapacity,Capacity>& registration,RegisteredSkeletonThrow& caller,
        int x,int y,bool separated,int facing,bool& shot,Sound sound){
        if(!registration.belongsTo(instances_)||!registration.valid())return run_.effectsSucceeded=false;
        return caller.run(x,y,separated,facing,shot,run_.effectsSucceeded,
            [&](int px,int py,InstanceHandle& key){
                if(!spawnBone(registration,px,py,key))return false;
                caller.myBoneSourceId=registration.instances().find(key)->sourceId;return true;
            },
            [&](InstanceHandle key,auto event){return registration.dispatch(key,event);},random_,sound);
    }
    template<unsigned Capacity,class Audio> bool throwSkeletonBone(
        StoredBoneRegistration<instanceCapacity,Capacity>& registration,SkeletonActor& actor,
        RegisteredSkeletonThrow& caller,SoundPlayEnvironment& environment,Audio& audio){
        if(!registration.belongsTo(instances_)||!registration.valid())return run_.effectsSucceeded=false;
        return caller.runAimed(actor.x,actor.y,actor.shot,run_.effectsSucceeded,
            [&](int x,int y,InstanceHandle& key){
                if(!spawnBone(registration,x,y,key))return false;
                caller.myBoneSourceId=registration.instances().find(key)->sourceId;return true;
            },[&](InstanceHandle key,auto event){return registration.dispatch(key,event);},random_,
            [&](int sound){return playSkeletonSound(actor,sound,environment,audio);},
            [&](bool& separated,int& facing){
                MotionBinary64 px,py;
                if(!mainPlayerInitialized_||roomId_!=mainRoomId||!ContactCoordinates::playerGlobals(
                    mainPlayerMotion_.frameStartXFixed(),mainPlayerMotion_.frameStartYFixed(),
                    mainPlayerOriginX_,mainPlayerOriginY_,px,py))return false;
                const auto x=MotionBinary64::fromInteger(actor.x);
                separated=x.high!=px.high||x.low!=px.low;facing=actor.facing;return true;
            });
    }
    template<unsigned Capacity> StoredScriptSpawnRegistration<instanceCapacity,Capacity> scriptSpawnRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity> StoredDebrisRegistration<instanceCapacity,Capacity> debrisRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity,unsigned Registered>
    bool spawnDebris(StoredDebrisRegistration<instanceCapacity,Registered>& registration,DebrisEffects<Capacity>& effects,
        MotionBinary64 x,MotionBinary64 y,int area,std::uint32_t& id){
        if(!run_.effectsSucceeded)return false;
        if(!registration.belongsTo(instances_))return run_.effectsSucceeded=false;
        if(debrisBinding_.effects!=&effects||debrisBinding_.registry!=&registration){
            if(hasDebrisRecords())return run_.effectsSucceeded=false;
            bindDebrisDraw(effects,registration);
        }
        const auto local=effects.createPublishedRegistered(x,y,area,random_,registration);
        if(!local)return run_.effectsSucceeded=false;
        id=registration.sourceId(local);return run_.effectsSucceeded=id!=0;
    }
    const DebrisEffects<debrisEffectCapacity>& debrisEffects()const{return ownedDebrisEffects_;}
    bool spawnDebris(MotionBinary64 x,MotionBinary64 y,int area,std::uint32_t& id){
        return spawnDebris(ownedDebrisRegistration_,ownedDebrisEffects_,x,y,area,id);
    }
    bool spawnDebris(MotionBinary64 x,MotionBinary64 y,int area,InstanceHandle& key){
        std::uint32_t id=0;
        if(!spawnDebris(x,y,area,id))return false;
        const auto created=ownedDebrisRegistration_.sourceHandle(id);
        if(!created.generation)return run_.effectsSucceeded=false;
        key=created;return true;
    }
    template<unsigned Capacity,unsigned Registered,class Stopped>
    bool stepDebris(StoredDebrisRegistration<instanceCapacity,Registered>& registration,DebrisEffects<Capacity>& effects,
        InstanceHandle key,typename DebrisEffects<Capacity>::Phase phase,Stopped stopped){
        if(!run_.effectsSucceeded)return false;
        const auto* record=instances_.resolve(key);
        return run_.effectsSucceeded=registration.belongsTo(instances_)&&record&&
            registration.dispatchStep(key,*record,effects,phase,random_,stopped);
    }
    const DebrisAnimation* debrisForDraw(InstanceHandle key,const InstanceRecord& record)const{
        return debrisBinding_.registry&&debrisLookup_?debrisLookup_(debrisBinding_.registry,key,record):nullptr;
    }
    template<class Visit> bool visitDebrisDraw(Visit visit)const{
        if(!debrisBinding_.registry)return true;
        return visitDrawChecked([&](InstanceHandle key,const InstanceRecord& record){
            if(record.objectId!=370||!record.active||record.destroyRequested)return true;
            const auto* effect=debrisForDraw(key,record);return effect&&visit(*effect);
        });
    }
    template<class Visit> bool visitTimeShardDraw(Visit visit)const{
        return visitDrawChecked([&](InstanceHandle,const InstanceRecord& record){
            if(record.objectId!=238||!record.active||record.destroyRequested)return true;
            return record.timeShardInitialized&&visit(record);
        });
    }
    // drawOnSurface does not use scrInView: the sprite can overlap the view
    // even with its origin outside it. Preserve the source layer traversal,
    // including depth-setter overrides; the renderer resolves transforms.
    template<class Visit> bool visitEntryDitherDraw(Visit visit)const{
        return visitDrawAtDepthChecked([&](InstanceHandle key,const InstanceRecord& record,int depth){
            if(record.objectId!=73||!record.active||record.destroyRequested)return true;
            return visit(key,record,depth);
        });
    }
    template<class Visit> bool visitUnconditionalSurfaceDraw(Visit visit)const{
        return visitDrawAtDepthChecked([&](InstanceHandle key,const InstanceRecord& record,int depth){
            if((record.objectId!=73&&record.objectId!=372)||!record.active||record.destroyRequested)return true;
            return visit(key,record,depth);
        });
    }
    // Read the source player's pTimeStop at the particle's Step boundary;
    // neither goalStopped nor a shop control lock is a substitute.
    template<unsigned Registered>
    bool debrisFrozen(StoredDebrisRegistration<instanceCapacity,Registered>& registration,InstanceHandle key,
        const DebrisAnimation& effect,bool pTimeStop,bool& frozen){
        if(!run_.effectsSucceeded)return false;
        if(!registration.belongsTo(instances_))return run_.effectsSucceeded=false;
        int width=0;
        if(pTimeStop&&!timeStopRoomWidth(width))return false;
        return run_.effectsSucceeded=registration.frozen(key,effect,pTimeStop,width,frozen);
    }
    const ScriptSpawnEffects<scriptSpawnEffectCapacity>& scriptEffects()const{return ownedScriptEffects_;}
    template<class Execute,class Tick> bool withSourceBulletSteps(Execute execute,Tick tick){
        if(!run_.effectsSucceeded||!objectPack_||sourceBulletDispatch_)return run_.effectsSucceeded=false;
        for(std::size_t i=0;i<mainBulletCount_;++i)
            if(!mainBullets_[i].sourceOwned&&mainBullets_[i].trajectory.initialized)return run_.effectsSucceeded=false;
        sourceBulletContext_=&execute;
        sourceBulletDispatch_=[](void* context,InstanceHandle shot,MachineGunSourceMotion& motion){
            return (*static_cast<Execute*>(context))(shot,motion);
        };
        const bool ok=tick();sourceBulletDispatch_=nullptr;sourceBulletContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    const MainBulletState* retainedSourceBullet(InstanceHandle shot)const{
        if(!run_.effectsSucceeded||!instances_.resolve(shot))return nullptr;
        for(std::size_t i=0;i<mainBulletCount_;++i){const auto& bullet=mainBullets_[i];
            if(bullet.instance.slot!=shot.slot||bullet.instance.generation!=shot.generation)continue;
            return bullet.sourceOwned&&bullet.sourceMotion.initialized?&bullet:nullptr;
        }
        return nullptr;
    }
    bool sourceBulletVelocity(InstanceHandle shot,MotionBinary64& x,MotionBinary64& y)const{
        const auto* bullet=retainedSourceBullet(shot);if(!bullet)return false;
        const auto& motion=bullet->sourceMotion;
        if(!motion.xsp.valid()||!motion.ysp.valid())return false;
        x=motion.xsp;y=motion.ysp;return true;
    }
    template<class Ops,class Frames>
    bool stepSourceBullet(InstanceHandle shot,MachineGunSourceMotion& motion,Ops& ops,Frames frames){
        if(!run_.effectsSucceeded||!instances_.resolve(shot))return run_.effectsSucceeded=false;
        const MainBulletState* caller=nullptr;
        for(std::size_t i=0;i<mainBulletCount_;++i)
            if(mainBullets_[i].instance.slot==shot.slot&&mainBullets_[i].instance.generation==shot.generation){
                caller=&mainBullets_[i];break;
            }
        if(!caller)return run_.effectsSucceeded=false;
        const bool high=caller->gemHigh;
        if(!motion.initialized)motion.zeroSpread=caller->trajectory.zeroSpread;
        return withBulletEffects(shot,frames,[&](auto& effects){
            return MachineGunSourceStep::run(motion,random_,high,effects,ops,run_.effectsSucceeded);
        });
    }
    template<class Frames,class Execute>
    bool withBulletEffects(InstanceHandle shot,Frames frames,Execute execute){
        if(!run_.effectsSucceeded)return false;
        auto ops=bulletEffectOperations(shot,ownedScriptEffects_,ownedScriptRegistration_,frames);
        if(!run_.effectsSucceeded)return false;
        const bool ok=execute(ops);return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    template<unsigned Capacity,class Frames> bool spawnCurrencyPickupEffect(
        StoredCurrencyRegistration<instanceCapacity,Capacity>& currency,InstanceHandle gem,Frames frames){
        if(!run_.effectsSucceeded||!currency.belongsTo(instances_)||!currency.valid())
            return run_.effectsSucceeded=false;
        const auto* caller=currency.find(gem);
        if(!caller)return run_.effectsSucceeded=false;
        const auto x=caller->x.promoted(),y=caller->y.promoted();
        if(scriptEffectsBinding_.effects!=&ownedScriptEffects_||scriptEffectsBinding_.registry!=&ownedScriptRegistration_){
            if(hasScriptEffectRecords())return run_.effectsSucceeded=false;
            scriptEffectsBinding_=ScriptEffectRuntimeBinding::bind<InstanceRecord>(ownedScriptEffects_,ownedScriptRegistration_);
        }
        auto ops=scriptSpawnOperations(ownedScriptEffects_,ownedScriptRegistration_,frames,[&](unsigned local){
            const auto id=ownedScriptRegistration_.sourceId(local);
            return id&&currency.dispatch(gem,[&](auto& state,bool&){state.myFx=id;return true;});
        },run_.effectsSucceeded);
        return EffectSpawnScript::runNol(ops,x,y,105,{0x3fe99999u,0x9999999au});
    }
    template<class Frames> bool spawnScriptEffect(MotionBinary64 x,MotionBinary64 y,int sprite,
        MotionBinary64 speed,MotionBinary64 angle,MotionBinary64 depth,Frames frames,std::uint32_t& myFx){
        return spawnScriptEffect(ownedScriptRegistration_,ownedScriptEffects_,x,y,sprite,speed,angle,depth,frames,myFx);
    }
    template<unsigned Capacity,unsigned Registered,class Frames>
    auto bulletEffectOperations(InstanceHandle shot,ScriptSpawnEffects<Capacity>& effects,
        StoredScriptSpawnRegistration<instanceCapacity,Registered>& registration,Frames frames){
        const auto* record=instances_.resolve(shot);
        if(!record||record->objectId!=261||!registration.belongsTo(instances_))run_.effectsSucceeded=false;
        if(run_.effectsSucceeded&&(scriptEffectsBinding_.effects!=&effects||scriptEffectsBinding_.registry!=&registration)){
            if(hasScriptEffectRecords())run_.effectsSucceeded=false;
            else scriptEffectsBinding_=ScriptEffectRuntimeBinding::bind<InstanceRecord>(effects,registration);
        }
        return scriptSpawnOperations(effects,registration,frames,[this,shot,&registration](unsigned local){
            auto* caller=instances_.resolve(shot);const auto id=registration.sourceId(local);
            if(!caller||caller->objectId!=261||!id)return false;
            caller->bulletMyFx=id;return true;
        },run_.effectsSucceeded);
    }
    template<unsigned Capacity,unsigned Registered,class Frames>
    bool spawnScriptEffect(StoredScriptSpawnRegistration<instanceCapacity,Registered>& registration,
        ScriptSpawnEffects<Capacity>& effects,MotionBinary64 x,MotionBinary64 y,int sprite,
        MotionBinary64 speed,MotionBinary64 angle,MotionBinary64 depth,Frames frames,std::uint32_t& myFx){
        return spawnScriptEffectWithCaller(registration,effects,x,y,sprite,speed,angle,depth,frames,
            [&](std::uint32_t child){myFx=child;return true;});
    }
    template<unsigned Capacity,unsigned Registered,class Frames,class Caller>
    bool spawnScriptEffectWithCaller(StoredScriptSpawnRegistration<instanceCapacity,Registered>& registration,
        ScriptSpawnEffects<Capacity>& effects,MotionBinary64 x,MotionBinary64 y,int sprite,
        MotionBinary64 speed,MotionBinary64 angle,MotionBinary64 depth,Frames frames,Caller caller){
        if(!run_.effectsSucceeded)return false;
        if(!registration.belongsTo(instances_)){run_.effectsSucceeded=false;return false;}
        if(scriptEffectsBinding_.effects!=&effects||scriptEffectsBinding_.registry!=&registration){
            if(hasScriptEffectRecords())return run_.effectsSucceeded=false;
            scriptEffectsBinding_=ScriptEffectRuntimeBinding::bind<InstanceRecord>(effects,registration);
        }
        auto ops=scriptSpawnOperations(effects,registration,frames,[&](unsigned local){
            const auto id=registration.sourceId(local);return id&&caller(id);
        },run_.effectsSucceeded);
        return EffectSpawnScript::run(ops,x,y,sprite,speed,angle,depth);
    }
    template<class Frames> bool spawnBoxScriptEffect(InstanceHandle box,MotionBinary64 x,MotionBinary64 y,int sprite,
        MotionBinary64 speed,MotionBinary64 angle,MotionBinary64 depth,Frames frames){
        const auto* r=instances_.resolve(box);
        if(!run_.effectsSucceeded||!r||r->objectId!=92)return run_.effectsSucceeded=false;
        return spawnScriptEffectWithCaller(ownedScriptRegistration_,ownedScriptEffects_,x,y,sprite,speed,angle,depth,frames,
            [&](std::uint32_t child){
                auto* caller=instances_.resolve(box);if(!caller||caller->objectId!=92)return false;
                caller->boxMyFx=child;return true;
            });
    }
    template<class Visit> bool visitBoneDraw(Visit visit)const{
        bool present=false;
        for(unsigned i=0;i<instances_.size();++i)if(instances_.at(i).objectId==269){present=true;break;}
        if(!present)return true;
        return visitDrawAtDepthChecked([&](InstanceHandle,const InstanceRecord& record,int){
            if(record.objectId!=269||!record.active||record.destroyRequested)return true;
            RunnerImageValue x,y;
            // These are the builtin fields as published by the owner, not
            // integer draw caches or the bone's private motion accumulators.
            std::memcpy(&x.bits,&record.preciseX,4);std::memcpy(&y.bits,&record.preciseY,4);
            return visit(x.promoted(),y.promoted());
        });
    }
    template<class Visit> bool visitRegisteredEnemyShades(Visit visit)const{
        bool present=false;
        for(unsigned i=0;i<instances_.size();++i){const int object=instances_.at(i).objectId;
            if(object==115||object==269||object==120){present=true;break;}}
        if(!present)return true;
        return visitDrawAtDepthChecked([&](InstanceHandle,const InstanceRecord& record,int){
            if((record.objectId!=115&&record.objectId!=269&&record.objectId!=120)||!record.active||record.destroyRequested)return true;
            RunnerImageValue x,y;
            std::memcpy(&x.bits,&record.preciseX,4);std::memcpy(&y.bits,&record.preciseY,4);
            return visit(x.promoted(),y.promoted());
        });
    }
    template<unsigned Capacity> bool boneInView(
        StoredBoneRegistration<instanceCapacity,Capacity>& bones,InstanceHandle bone,bool& visible){
        if(!run_.effectsSucceeded||roomId_!=mainRoomId||!bones.belongsTo(instances_)||!bones.valid())
            return run_.effectsSucceeded=false;
        const auto* caller=bones.instances().find(bone);
        if(!caller)return run_.effectsSucceeded=false;
        // Normal Step sees the previous Draw publication, even after camMain
        // moved. A destroyed caller remains readable until event cleanup.
        return run_.effectsSucceeded=boneInPublishedView(caller->x.promoted(),caller->y.promoted(),
            MotionBinary64::fromInteger(ordinaryStepViewLeftQ12_/4096),
            MotionBinary64::fromInteger(ordinaryStepViewTopQ12_/4096),
            MotionBinary64::fromInteger(surfaceWidth),MotionBinary64::fromInteger(surfaceHeight),visible);
    }
    template<unsigned Capacity,class Frames> bool spawnBoneImpact(
        StoredBoneRegistration<instanceCapacity,Capacity>& bones,InstanceHandle bone,Frames frames){
        if(!run_.effectsSucceeded||!bones.belongsTo(instances_)||!bones.valid())return run_.effectsSucceeded=false;
        const auto* caller=bones.instances().find(bone);
        if(!caller)return run_.effectsSucceeded=false;
        const auto x=caller->x.promoted(),y=caller->y.promoted();
        if(scriptEffectsBinding_.effects!=&ownedScriptEffects_||scriptEffectsBinding_.registry!=&ownedScriptRegistration_){
            if(hasScriptEffectRecords())return run_.effectsSucceeded=false;
            scriptEffectsBinding_=ScriptEffectRuntimeBinding::bind<InstanceRecord>(ownedScriptEffects_,ownedScriptRegistration_);
        }
        auto ops=scriptSpawnOperations(ownedScriptEffects_,ownedScriptRegistration_,frames,[&](unsigned local){
            const auto id=ownedScriptRegistration_.sourceId(local);
            return id&&bones.assignEffect(bone,local,id);
        },run_.effectsSucceeded);
        return EffectSpawnScript::runNol(ops,x,y,85,{0x3fd33333u,0x33333333u});
    }
    template<unsigned Capacity,unsigned Registered>
    bool advanceScriptEffectAnimation(StoredScriptSpawnRegistration<instanceCapacity,Registered>& registration,
        ScriptSpawnEffects<Capacity>& effects,typename ScriptSpawnEffects<Capacity>::Phase phase){
        if(!run_.effectsSucceeded)return false;
        return run_.effectsSucceeded=registration.belongsTo(instances_)&&effects.advanceRegistered(phase,registration);
    }
    template<unsigned Capacity,unsigned Registered,class Pause> bool createTimeChange(
        StoredTimeChangeRegistration<instanceCapacity,Registered>& registration,TimeChangeEffects<Capacity>& changes,
        MotionBinary64 x,MotionBinary64 y,int& speed,Pause pause){
        if(!run_.effectsSucceeded)return false;
        run_.effectsSucceeded=registration.belongsTo(instances_)&&changes.boundTo(registration)&&
            changes.createAt(x,y,speed,pause);
        return run_.effectsSucceeded;
    }
    template<unsigned Capacity,unsigned Registered> bool advanceTimeChangeAlarm(
        StoredTimeChangeRegistration<instanceCapacity,Registered>& registration,TimeChangeEffects<Capacity>& changes,
        typename TimeChangeEffects<Capacity>::Phase phase,unsigned index,int& speed){
        if(!run_.effectsSucceeded)return false;
        if(!registration.belongsTo(instances_)||!changes.boundTo(registration)){run_.effectsSucceeded=false;return false;}
        changes.advanceAlarm(phase,index,speed);
        run_.effectsSucceeded=changes.succeeded();return run_.effectsSucceeded;
    }
    template<unsigned Capacity> bool emitTimeShards(TimeShardBurst& burst,TimeShardEffects<Capacity>& shards,
        RunnerImageValue x,RunnerImageValue y,MotionBinary64 xsp,MotionBinary64 ysp,bool lowSpec){
        if(!run_.effectsSucceeded)return false;
        run_.effectsSucceeded=burst.emit(x,y,xsp,ysp,lowSpec,random_,shards,
            [&](auto px,auto py,auto& id,auto& fx,auto& fy){
                InstancePlaceResult result;
                if(!playerTimeFieldInstance(px,py,result))return false;
                id=result.id;fx=result.x;fy=result.y;return true;
            },[](auto ax,auto ay,auto bx,auto by,auto& angle){
                return RunnerPointDirection::evaluate(ax,ay,bx,by,angle,portableAtan);
            },[](auto speed,auto angle,bool vertical,auto& result){
                MotionBinary64 vx,vy;
                if(!SmokeDirection::vector(speed,angle,vx,vy,smokeTrig))return false;
                result=vertical?vy:vx;return true;
            });
        return run_.effectsSucceeded;
    }
    template<unsigned Capacity,unsigned Registered,class Outside> bool stepTimeShards(
        StoredTimeShardRegistration<instanceCapacity,Registered>& registration,TimeShardEffects<Capacity>& shards,
        typename TimeShardEffects<Capacity>::Phase phase,bool stopped,Outside outside){
        if(!run_.effectsSucceeded)return false;
        run_.effectsSucceeded=registration.belongsTo(instances_)&&shards.phasesBoundTo(registration)&&
            shards.step(phase,stopped,random_,[&](auto local,const auto&,auto x,auto y,bool& hit){
                return registration.meeting(local,shards.sourceId(local),x,y,hit);
            },outside);
        return run_.effectsSucceeded;
    }
    template<unsigned Capacity,unsigned Registered> bool contactTimeShard(
        StoredTimeShardRegistration<instanceCapacity,Registered>& registration,TimeShardEffects<Capacity>& shards,
        typename TimeShardEffects<Capacity>::Handle handle,MotionBinary64 xsp,MotionBinary64 ysp){
        if(!run_.effectsSucceeded)return false;
        run_.effectsSucceeded=registration.belongsTo(instances_)&&shards.phasesBoundTo(registration)&&
            shards.playerContact(handle,xsp,ysp,random_);
        return run_.effectsSucceeded;
    }
    // Recovered fxTimeShard Collision objPlayer_n (2081). Unlike engine speed,
    // other.xsp/ysp are the player's private movement fields. This handler
    // observes only their signs, so the Q8/fifths numerators are sufficient.
    template<unsigned Capacity,unsigned Registered> bool dispatchTimeShardContact(
        StoredTimeShardRegistration<instanceCapacity,Registered>& registration,TimeShardEffects<Capacity>& shards,
        const RunnerCollisionEventContext::Frame& event){
        if(!run_.effectsSucceeded)return false;
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        if(!registration.belongsTo(instances_)||!mainPlayerInitialized_||roomId_!=mainRoomId||
            event.type!=4||event.code!=2081||event.owner!=238||event.subtype!=35||
            !self||!other||self->objectId!=238||other->objectId!=35){
            run_.effectsSucceeded=false;return false;
        }
        run_.effectsSucceeded=registration.dispatchContact(event.self,*self,shards,
            MotionBinary64::fromInteger(mainPlayerMotion_.speedXFixed()),
            MotionBinary64::fromInteger(mainPlayerMotion_.speedYFifths()),random_);
        return run_.effectsSucceeded;
    }
    // The selected bullet's private VM fields are not its engine hspeed/vspeed
    // or a difference between rounded positions. The default binding reads
    // retained source motion for this exact lifetime; no trajectory fallback.
    template<unsigned Capacity> bool dispatchLimboShardBulletCollision(
        StoredLimboShardRegistration<instanceCapacity,Capacity>& shards,
        const RunnerCollisionEventContext::Frame& event){
        return dispatchLimboShardBulletCollision(shards,event,[&](InstanceHandle shot,MotionBinary64& x,MotionBinary64& y){
            return sourceBulletVelocity(shot,x,y);
        });
    }
    template<unsigned Capacity,class ReadVelocity> bool dispatchLimboShardBulletCollision(
        StoredLimboShardRegistration<instanceCapacity,Capacity>& shards,
        const RunnerCollisionEventContext::Frame& event,ReadVelocity readVelocity){
        if(!run_.effectsSucceeded)return false;
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{};bool found=false;
        if(!shards.belongsTo(instances_)||!shards.valid()||!self||!other||!objectPack_||
           self->objectId!=379||!shards.state(event.self)||event.type!=4||event.subtype!=other->objectId||
           event.owner!=379||event.code!=2771||
           !objectPack_->collisionEvent(379,other->objectId,found,binding)||!found||
           binding.ownerObjectId!=379||binding.codeId!=2771)return run_.effectsSucceeded=false;
        if(!self->active||self->destroyRequested||shards.state(event.self)->noTouch)return true;
        MotionBinary64 xsp,ysp;
        if(!readVelocity(event.other,xsp,ysp)||!xsp.valid()||!ysp.valid())return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=shards.bulletContact(event.self,xsp,ysp,random_)&&run_.effectsSucceeded;
    }
    template<unsigned Capacity,class Effects> bool dispatchCorpsePlayerCollision(
        StoredCorpseRegistration<instanceCapacity,Capacity>& corpses,
        const RunnerCollisionEventContext::Frame& event,bool knifeFork,bool playerDead,Effects& effects){
        if(!run_.effectsSucceeded)return false;
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{};bool found=false;
        if(!corpses.belongsTo(instances_)||!corpses.valid()||!self||!other||!objectPack_||
           self->objectId!=263||other->objectId!=35||!corpses.state(event.self)||
           event.type!=4||event.subtype!=35||event.owner!=263||
           !objectPack_->collisionEvent(263,35,found,binding)||!found||
           binding.ownerObjectId!=263||binding.codeId!=event.code)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=corpses.dispatchPublished(event.self,[&](auto& state,auto& publish,bool& okay){
            return state.collidePlayerPublished(knifeFork,playerDead,publish,effects,okay);
        })&&run_.effectsSucceeded;
    }
    template<unsigned Capacity> bool dispatchConsumptionBloodCollision(
        StoredConsumptionBloodRegistration<instanceCapacity,Capacity>& particles,
        const RunnerCollisionEventContext::Frame& event){
        if(!run_.effectsSucceeded)return false;
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{};bool found=false;
        if(!particles.belongsTo(instances_)||!particles.valid()||!self||!other||!objectPack_||self->objectId!=223||
           !particles.state(*self)||event.type!=4||event.subtype!=other->objectId||
           !objectPack_->collisionEvent(223,other->objectId,found,binding)||!found||
           binding.ownerObjectId!=223||event.owner!=223||event.code!=binding.codeId||
           (event.code!=2007&&event.code!=2008))return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=particles.kill(*self)&&run_.effectsSucceeded;
    }
    template<unsigned BlastCapacity,unsigned BoneCapacity,unsigned FlashCapacity>
    bool dispatchBlastBoneCollision(StoredBlastRegistration<instanceCapacity,BlastCapacity>& blasts,
        StoredBoneRegistration<instanceCapacity,BoneCapacity>& bones,
        StoredFlashBallRegistration<instanceCapacity,FlashCapacity>& flashes,const RunnerCollisionEventContext::Frame& event){
        return dispatchBlastShootableCollision<269>(blasts,bones,flashes,event);
    }
    template<unsigned CorpseCapacity,unsigned BlastCapacity>
    bool dispatchCorpseBlastCollision(StoredCorpseRegistration<instanceCapacity,CorpseCapacity>& corpses,
        StoredBlastRegistration<instanceCapacity,BlastCapacity>& blasts,const RunnerCollisionEventContext::Frame& event){
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{};bool found=false;
        if(!run_.effectsSucceeded||!corpses.belongsTo(instances_)||!corpses.valid()||!blasts.belongsTo(instances_)||!blasts.valid()||
           !self||!other||!objectPack_||self->objectId!=263||other->objectId!=276||
           !corpses.handle(*self).generation||!blasts.handle(*other).generation||event.type!=4||event.subtype!=276||
           event.owner!=263||event.code!=2244||!objectPack_->collisionEvent(263,276,found,binding)||!found||
           binding.ownerObjectId!=263||binding.codeId!=2244)return run_.effectsSucceeded=false;
        return true; // Source objDeadBody/parentPBullet event has no instructions.
    }
    template<unsigned BlastCapacity,unsigned CorpseCapacity,unsigned FlashCapacity>
    bool dispatchBlastCorpseCollision(StoredBlastRegistration<instanceCapacity,BlastCapacity>& blasts,
        StoredCorpseRegistration<instanceCapacity,CorpseCapacity>& corpses,
        StoredFlashBallRegistration<instanceCapacity,FlashCapacity>& flashes,const RunnerCollisionEventContext::Frame& event){
        return dispatchBlastShootableCollision<263>(blasts,corpses,flashes,event);
    }
    template<int ObjectId,unsigned BlastCapacity,unsigned FlashCapacity,class Shootables>
    bool dispatchBlastShootableCollision(StoredBlastRegistration<instanceCapacity,BlastCapacity>& blasts,Shootables& shootables,
        StoredFlashBallRegistration<instanceCapacity,FlashCapacity>& flashes,const RunnerCollisionEventContext::Frame& event){
        static_assert(ObjectId==269||ObjectId==263,"registered shootable descendants only");
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{};bool found=false;
        if(!run_.effectsSucceeded||!blasts.belongsTo(instances_)||!blasts.valid()||!shootables.belongsTo(instances_)||!shootables.valid()||
           !flashes.belongsTo(instances_)||!flashes.valid()||!self||!other||!objectPack_||self->objectId!=276||other->objectId!=ObjectId||
           event.type!=4||event.subtype!=ObjectId||event.owner!=276||event.code!=2329||
           !objectPack_->collisionEvent(276,ObjectId,found,binding)||!found||binding.ownerObjectId!=276||binding.codeId!=2329)
            return run_.effectsSucceeded=false;
        const auto local=blasts.handle(*self);const auto* caller=blasts.instances().find(local);
        if(!caller||!shootables.handle(*other).generation)return run_.effectsSucceeded=false;
        const auto x=caller->x.promoted(),y=caller->y.promoted();const auto id=other->instanceId;
        return run_.effectsSucceeded=blasts.dispatch(local,[&](auto& actor,auto&,bool& good){
            return blastShootableContact(actor,id,x,y,[&](MotionBinary64& hp){bool unused;return shootables.readShootable(event.other,hp,unused);},
                [&](MotionBinary64 hp){return shootables.writeShootableHealth(event.other,hp);},
                [&](auto& state,auto px,auto py){return flashBurstPublished<1,0,0>(state,px,py,random_,
                    [&](int object,auto fx,auto fy){InstanceHandle child;return object==224&&flashes.create(fx,fy,random_,child);});},good);
        });
    }
    template<unsigned Capacity> bool dispatchBlastWallCollision(
        StoredBlastRegistration<instanceCapacity,Capacity>& blasts,const RunnerCollisionEventContext::Frame& event){
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{};PackedObject target{};bool found=false;
        if(!run_.effectsSucceeded||!blasts.belongsTo(instances_)||!blasts.valid()||!self||!other||!objectPack_||
           self->objectId!=276||event.type!=4||event.subtype!=other->objectId||event.owner!=276||event.code!=2330||
           !objectPack_->collisionEvent(276,other->objectId,found,binding)||!found||binding.ownerObjectId!=276||binding.codeId!=2330||
           !objectPack_->findObject(other->objectId,target))return run_.effectsSucceeded=false;
        const unsigned id=other->instanceId;
        return run_.effectsSucceeded=blasts.dispatch(blasts.handle(*self),[&](auto& actor,auto&,bool& good){
            return blastWallContact(actor,target.parentId,id,[&](MotionBinary64& hp){
                const auto* r=instances_.resolve(event.other);if(!r||r->objectId!=92)return false;hp=r->box.wallHp;return true;
            },[&](MotionBinary64 hp){return publishBoxHealth(event.other,hp);},good);
        });
    }
    template<unsigned Capacity> bool dispatchBlastPlayerCollision(
        StoredBlastRegistration<instanceCapacity,Capacity>& blasts,const RunnerCollisionEventContext::Frame& event){
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{};bool found=false;
        if(!run_.effectsSucceeded||!blasts.belongsTo(instances_)||!blasts.valid()||!self||!other||!objectPack_||
           self->objectId!=276||other->objectId!=35||event.type!=4||event.subtype!=35||event.owner!=276||event.code!=2332||
           !objectPack_->collisionEvent(276,35,found,binding)||!found||binding.ownerObjectId!=276||binding.codeId!=2332)
            return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=blasts.dispatch(blasts.handle(*self),[](auto& actor,auto&,bool& good){
            return actor.playerContact(good);
        });
    }
    template<unsigned BlastCapacity,unsigned SkeletonCapacity,class Frames,class Audio>
    bool dispatchBlastSkeletonCollisionWithEffects(StoredBlastRegistration<instanceCapacity,BlastCapacity>& blasts,
        StoredSkeletonRegistration<instanceCapacity,SkeletonCapacity>& skeletons,
        const RunnerCollisionEventContext::Frame& event,Frames frames,SoundPlayEnvironment& environment,Audio& audio){
        return dispatchBlastSkeletonCollision(blasts,skeletons,event,
            [&](auto& caller,auto x,auto y,int sprite,auto speed,auto angle,int depth){
                return spawnScriptEffect(x,y,sprite,speed,angle,MotionBinary64::fromInteger(depth),frames,caller.myFx);
            },environment,audio);
    }
    template<unsigned BlastCapacity,unsigned SkeletonCapacity,class Impact,class Audio>
    bool dispatchBlastSkeletonCollision(StoredBlastRegistration<instanceCapacity,BlastCapacity>& blasts,
        StoredSkeletonRegistration<instanceCapacity,SkeletonCapacity>& skeletons,
        const RunnerCollisionEventContext::Frame& event,Impact impact,SoundPlayEnvironment& environment,Audio& audio){
        if(!run_.effectsSucceeded)return false;
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{};bool found=false;
        if(!blasts.belongsTo(instances_)||!blasts.valid()||!skeletons.belongsTo(instances_)||!skeletons.valid()||
           !self||!other||!objectPack_||self->objectId!=276||other->objectId!=120||
           event.type!=4||event.subtype!=120||event.owner!=276||event.code!=2331||
           !objectPack_->collisionEvent(276,120,found,binding)||!found||binding.ownerObjectId!=276||binding.codeId!=2331)
            return run_.effectsSucceeded=false;
        const auto local=blasts.handle(*self),target=skeletons.handle(*other);
        const auto* caller=blasts.instances().find(local);if(!caller)return run_.effectsSucceeded=false;
        const auto x=caller->x.promoted(),y=caller->y.promoted();const auto id=other->instanceId;
        return run_.effectsSucceeded=blasts.dispatch(local,[&](auto& blast,auto& publish,bool& good){
            return skeletons.dispatch(target,[&](auto& actor,auto&,bool& targetGood){
                const bool ok=blastSkeletonContact(blast,actor,id,x,y,publish,[&]{return requestMomentDelay();},impact,environment,audio,good);
                return targetGood=targetGood&&ok;
            });
        });
    }
    template<unsigned Capacity> bool dispatchSmokeCollision(
        StoredSmokeRegistration<instanceCapacity,Capacity>& smoke,
        const RunnerCollisionEventContext::Frame& event){
        if(!run_.effectsSucceeded)return false;
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{};bool found=false;
        if(!smoke.belongsTo(instances_)||!smoke.valid()||!self||!other||!objectPack_||self->objectId!=225||
           !smoke.state(*self)||event.type!=4||event.subtype!=other->objectId||
           !objectPack_->collisionEvent(225,other->objectId,found,binding)||!found||
           binding.ownerObjectId!=225||event.owner!=225||event.code!=binding.codeId||
           (event.code!=2016&&event.code!=2017))return run_.effectsSucceeded=false;
        // parentWater's empty event overrides inherited parentWall behavior.
        // The collision wrapper owns pair selection/order; never aggregate hits.
        return run_.effectsSucceeded=event.code==2016||smoke.wallContact(*self);
    }
    template<unsigned Capacity> bool dispatchSkeletonFieldCollision(
        StoredSkeletonRegistration<instanceCapacity,Capacity>& skeletons,
        const RunnerCollisionEventContext::Frame& event){
        if(!run_.effectsSucceeded)return false;
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{};bool found=false;
        if(!skeletons.belongsTo(instances_)||!skeletons.valid()||!self||!other||!objectPack_||self->objectId!=120||
           event.type!=4||event.subtype!=other->objectId||event.owner!=83||event.code!=1066||
           !objectPack_->collisionEvent(120,other->objectId,found,binding)||!found||
           binding.ownerObjectId!=83||binding.codeId!=1066)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=skeletons.dispatch(skeletons.handle(*self),[](auto& actor,auto&,bool&){
            actor.collideTimeField();return true;
        });
    }
    template<unsigned Capacity,class PointDirection,class DirectionSetter>
    bool dispatchWhiteJellyEnemyCollision(StoredWhiteJellyStates<instanceCapacity,Capacity>& owner,
        const RunnerCollisionEventContext::Frame& event,PointDirection pointDirection,DirectionSetter directionSetter){
        if(!run_.effectsSucceeded)return false;
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{};bool found=false;
        if(!owner.belongsTo(instances_)||!owner.valid()||!self||!other||!objectPack_||self->objectId!=115||
           event.type!=4||event.subtype!=other->objectId||event.owner!=115||event.code!=1226||
           !objectPack_->collisionEvent(115,other->objectId,found,binding)||!found||
           binding.ownerObjectId!=115||binding.codeId!=1226)return run_.effectsSucceeded=false;
        struct Ops {
            StartupRuntime& world;InstanceHandle self,other;PointDirection& point;DirectionSetter& setter;
            bool read(bool vertical,MotionBinary64& out){
                const auto* r=world.instances_.resolve(other);if(!r)return false;
                RunnerImageValue value;const float coordinate=vertical?r->preciseY:r->preciseX;
                std::memcpy(&value.bits,&coordinate,4);out=value.promoted();return true;
            }
            bool otherX(MotionBinary64& out){return read(false,out);}
            bool otherY(MotionBinary64& out){return read(true,out);}
            bool pointDirection(MotionBinary64 x,MotionBinary64 y,MotionBinary64 tx,MotionBinary64 ty,MotionBinary64& out){
                return point(x,y,tx,ty,out);
            }
            bool direction(MotionBinary64 wanted,MotionBinary64& accepted){return setter(self,wanted,accepted);}
        } ops{*this,event.self,event.other,pointDirection,directionSetter};
        return run_.effectsSucceeded=owner.dispatch(event.self,[&](auto& state){return state.enemyContact(random_,ops);});
    }
    template<unsigned Capacity> bool dispatchWhiteJellyFieldCollision(
        StoredWhiteJellyStates<instanceCapacity,Capacity>& owner,const RunnerCollisionEventContext::Frame& event){
        if(!run_.effectsSucceeded)return false;
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{};bool found=false;
        if(!owner.belongsTo(instances_)||!owner.valid()||!self||!other||!objectPack_||self->objectId!=115||
           event.type!=4||event.subtype!=other->objectId||event.owner!=83||event.code!=1066||
           !objectPack_->collisionEvent(115,other->objectId,found,binding)||!found||
           binding.ownerObjectId!=83||binding.codeId!=1066)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=owner.dispatch(event.self,[](auto& state){
            if(!state.timefieldImmune)state.health=0;return true;
        });
    }
    template<unsigned Capacity,class MakePlayer> bool dispatchSkeletonCollision(
        StoredSkeletonRegistration<instanceCapacity,Capacity>& skeletons,
        const RunnerCollisionEventContext::Frame& event,MakePlayer makePlayer){
        if(event.code==1066)return dispatchSkeletonFieldCollision(skeletons,event);
        if(!run_.effectsSucceeded)return false;
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{};bool found=false;
        if(!skeletons.belongsTo(instances_)||!skeletons.valid()||!self||!other||!objectPack_||self->objectId!=120||
           event.type!=4||event.subtype!=other->objectId||
           !objectPack_->collisionEvent(120,other->objectId,found,binding)||!found||
           event.owner!=binding.ownerObjectId||event.code!=binding.codeId||
           !(event.owner==120&&event.code==1266))return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=skeletons.dispatch(skeletons.handle(*self),[&](auto& actor,auto&,bool& good){
            auto player=makePlayer(actor);return actor.collidePlayerPublished(player,good);
        })&&run_.effectsSucceeded;
    }
    template<unsigned Capacity,class MakePlayer> bool dispatchWhiteJellyPlayerCollision(
        StoredWhiteJellyStates<instanceCapacity,Capacity>& owner,const RunnerCollisionEventContext::Frame& event,
        MakePlayer makePlayer){
        if(!run_.effectsSucceeded)return false;
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{};bool found=false;
        if(!owner.belongsTo(instances_)||!owner.valid()||!self||!other||!objectPack_||self->objectId!=115||other->objectId!=35||
           event.type!=4||event.subtype!=35||event.owner!=115||event.code!=1227||
           !objectPack_->collisionEvent(115,35,found,binding)||!found||binding.ownerObjectId!=115||binding.codeId!=1227)
            return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=owner.dispatch(event.self,[&](auto& actor){
            auto player=makePlayer(actor);return actor.playerContact(player);
        })&&run_.effectsSucceeded;
    }
    template<unsigned Capacity,class Frames,class Delay,class Audio,class Atan>
    bool dispatchSkeletonBulletCollision(StoredSkeletonRegistration<instanceCapacity,Capacity>& skeletons,
        const RunnerCollisionEventContext::Frame& event,Frames frames,Delay delay,
        SoundPlayEnvironment& environment,Audio& audio,Atan atan){
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        const auto* bullet=retainedSourceBullet(event.self);PackedObjectEvent binding{};bool found=false;
        if(!run_.effectsSucceeded||!skeletons.belongsTo(instances_)||!skeletons.valid()||!self||!other||!bullet||
           !objectPack_||self->objectId!=261||other->objectId!=120||event.type!=4||event.subtype!=120||
           event.owner!=261||event.code!=2224||!objectPack_->collisionEvent(261,120,found,binding)||!found||
           binding.ownerObjectId!=261||binding.codeId!=2224)return run_.effectsSucceeded=false;
        const auto target=skeletons.handle(*other);const auto direction=bullet->sourceMotion.direction;
        const int damage=machineGunProfile(bullet->gemHigh).damage;
        const bool ok=skeletons.dispatch(target,[&](auto& actor,auto&,bool& good){
            struct Events {
                StartupRuntime& world;StoredSkeletonRegistration<instanceCapacity,Capacity>& skeletons;
                InstanceHandle shot,target;Frames& frames;Delay& delay;
                SoundPlayEnvironment& environment;Audio& audio;Atan& atan;bool& good;
                bool clearHitIgnore(){
                    auto* caller=world.instances_.resolve(shot);if(!caller)return false;
                    caller->bulletHitIgnore=false;return true;
                }
                bool destroyBullet(){return world.markBulletDestroyed(shot);}
                bool impactAngle(MotionBinary64& angle){
                    const auto* caller=world.retainedSourceBullet(shot);const auto* other=skeletons.instances().find(target);
                    return caller&&other&&RunnerPointDirection::evaluate(caller->sourceMotion.x.promoted(),
                        caller->sourceMotion.y.promoted(),other->x.promoted(),other->y.promoted(),angle,atan);
                }
                bool momentDelay(){return delay();}
                bool bulletHitSound(int sound,int priority,bool loop,bool cut){
                    auto* caller=world.instances_.resolve(shot);int voice=0;
                    return caller&&SoundPlayScript::plain(caller->sound,environment,audio,sound,priority,loop,cut,voice,good);
                }
                bool impactEffect(int sprite,MotionBinary64 speed,MotionBinary64 angle,int depth){
                    const auto* caller=world.retainedSourceBullet(shot);if(!caller)return false;
                    const auto x=caller->sourceMotion.x.promoted(),y=caller->sourceMotion.y.promoted();
                    return world.withBulletEffects(shot,frames,[&](auto& effects){
                        return EffectSpawnScript::run(effects,x,y,sprite,speed,angle,MotionBinary64::fromInteger(depth));
                    });
                }
            } events{*this,skeletons,event.self,target,frames,delay,environment,audio,atan,good};
            return actor.hitByMachineGunChecked(damage,direction,events,good);
        });
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    template<unsigned Capacity,class Frames,class Damage> bool dispatchBoneCollision(
        StoredBoneRegistration<instanceCapacity,Capacity>& bones,
        const RunnerCollisionEventContext::Frame& event,bool stopped,Frames frames,Damage damage){
        if(!run_.effectsSucceeded)return false;
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{};bool found=false;
        if(!bones.belongsTo(instances_)||!bones.valid()||!self||!other||!objectPack_||self->objectId!=269||
           event.type!=4||event.subtype!=other->objectId||
           !objectPack_->collisionEvent(269,other->objectId,found,binding)||!found||
           binding.ownerObjectId!=269||event.owner!=269||event.code!=binding.codeId||
           (event.code!=2291&&event.code!=2292))return run_.effectsSucceeded=false;
        const auto local=bones.handle(*self);
        return run_.effectsSucceeded=bones.dispatch(local,[&](auto& state,auto& publish,bool& good){
            return state.collideInWorld(event.code==2292,stopped,publish,
                [&]{return spawnBoneImpact(bones,local,frames);},damage,good);
        })&&run_.effectsSucceeded;
    }
    // objDitherAbove Collision objWall_n executes instance_destroy in OTHER.
    // Pair admission/order belongs to the collision wrapper, not this handler.
    // Only the admitted plain wall lifetime is supported here; other families
    // may require synchronous Destroy events before their destruction mark.
    bool dispatchDitherAboveCollision(const RunnerCollisionEventContext::Frame& event){
        if(!run_.effectsSucceeded)return false;
        const auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{},destroy{};bool found=false;
        if(!self||!other||!objectPack_||self->objectId!=61||other->objectId!=60||
            event.type!=4||event.subtype!=other->objectId||
            !objectPack_->collisionEvent(self->objectId,other->objectId,found,binding)||!found||
            binding.ownerObjectId!=61||binding.codeId!=991||event.owner!=61||event.code!=991||
            objectPack_->findEvent(other->objectId,1,0,destroy))return run_.effectsSucceeded=false;
        if(other->destroyRequested)return true;
        if(lifecycle_.hasOwner()&&!lifecycle_.markDestroyed(event.other,*other))return run_.effectsSucceeded=false;
        auto* marked=instances_.resolve(event.other);
        if(!run_.effectsSucceeded||!marked)return run_.effectsSucceeded=false;
        marked->destroyRequested=true;
        return true;
    }
    // Shared voice/channel state outlives room-local fields. Audio is checked
    // and synchronous; event selection must retain the inherited binding.
    int timeFieldWindVoice()const{return run_.fieldWind.voice;}
    template<class Audio> bool dispatchTimeFieldWind(const RunnerCollisionEventContext::Frame& event,
        MotionBinary64 volume,Audio& audio){
        return dispatchTimeFieldWind(event,run_.fieldWind,run_.overlapSounds[8],volume,audio);
    }
    // Explicit state provider retained for script/reference fixtures.
    template<class Audio> bool dispatchTimeFieldWind(const RunnerCollisionEventContext::Frame& event,
        TimeFieldWind& wind,int& uiChannel,MotionBinary64 volume,Audio& audio){
        if(!run_.effectsSucceeded)return false;
        auto* self=instances_.resolve(event.self);const auto* other=instances_.resolve(event.other);
        PackedObjectEvent binding{};bool found=false;
        if(!self||!other||!objectPack_||event.type!=4||event.subtype!=other->objectId||
            !objectPack_->collisionEvent(self->objectId,other->objectId,found,binding)||!found||
            binding.ownerObjectId!=255||event.owner!=binding.ownerObjectId||event.code!=binding.codeId){
            run_.effectsSucceeded=false;return false;
        }
        run_.effectsSucceeded=wind.contact(random_,self->windSound,self->sound,uiChannel,volume,audio);
        return run_.effectsSucceeded;
    }
    template<unsigned Capacity,unsigned Registered> bool retireTimeShard(
        StoredTimeShardRegistration<instanceCapacity,Registered>& registration,TimeShardEffects<Capacity>& shards,
        std::uint32_t id){
        if(!run_.effectsSucceeded)return false;
        if(registration.belongsTo(instances_))for(std::size_t i=0;i<instances_.size();++i){
            const auto& record=instances_.at(i);
            if(record.instanceId!=id)continue;
            run_.effectsSucceeded=registration.retireMarked(instances_.handleAt(i),record,shards);
            return run_.effectsSucceeded;
        }
        run_.effectsSucceeded=false;return false;
    }
    // Explicit pre-Draw cleanup boundary. Other effect families supply their
    // own retirement dispatch; they must not be silently erased as plain records.
    template<unsigned Capacity,unsigned Registered,class Other> bool cleanupDestroyed(
        StoredDebrisRegistration<instanceCapacity,Registered>& registration,DebrisEffects<Capacity>& effects,Other other){
        if(!run_.effectsSucceeded)return false;
        return run_.effectsSucceeded=registration.belongsTo(instances_)&&instances_.cleanupDestroyed(
            [&](InstanceHandle key,const InstanceRecord& record){
                return record.objectId==370?registration.retireMarked(key,record,effects):other(key,record);
            });
    }
    template<unsigned Capacity,unsigned Registered,class Other> bool cleanupDestroyed(
        StoredScriptSpawnRegistration<instanceCapacity,Registered>& registration,ScriptSpawnEffects<Capacity>& effects,
        Other other){
        if(!run_.effectsSucceeded)return false;
        run_.effectsSucceeded=registration.belongsTo(instances_)&&effects.boundTo(registration)&&
            instances_.cleanupDestroyed([&](InstanceHandle key,const InstanceRecord& record){
                return record.objectId==219?registration.retireMarked(key,record,effects):other(key,record);
            });
        return run_.effectsSucceeded;
    }
    template<unsigned Capacity,unsigned Registered,class Other> bool cleanupDestroyed(
        StoredTimeShardRegistration<instanceCapacity,Registered>& registration,TimeShardEffects<Capacity>& shards,
        Other other){
        if(!run_.effectsSucceeded)return false;
        run_.effectsSucceeded=registration.belongsTo(instances_)&&shards.phasesBoundTo(registration)&&
            instances_.cleanupDestroyed([&](InstanceHandle key,const InstanceRecord& record){
                return record.objectId==238?registration.retireMarked(key,record,shards):other(key,record);
            });
        return run_.effectsSucceeded;
    }
    template<unsigned Capacity,unsigned Registered,unsigned Changes,class Other> bool cleanupDestroyed(
        StoredTimeShardRegistration<instanceCapacity,Registered>& shards,
        StoredTimeChangeRegistration<instanceCapacity,Changes>& changes,TimeStopEffects<Capacity>& state,Other other){
        if(!run_.effectsSucceeded)return false;
        run_.effectsSucceeded=shards.belongsTo(instances_)&&changes.belongsTo(instances_)&&
            state.shards.phasesBoundTo(shards)&&state.changes.boundTo(changes)&&
            instances_.cleanupDestroyed([&](InstanceHandle key,const InstanceRecord& record){
                if(record.objectId==238)return shards.retireMarked(key,record,state.shards);
                if(record.objectId==211)return changes.retireMarked(key,record,state.changes);
                return other(key,record);
            });
        return run_.effectsSucceeded;
    }
    bool menuPlayerNapping() const { return menuPlayerNapping_; }
    int menuPlayerSpriteId() const { return menuPlayerSpriteId_; }
    float menuPlayerImageIndex() const { return menuPlayerImageIndex_; }
    int menuPlayerFacing() const { return menuPlayerFacing_; }
    float menuPlayerX() const { return menuPlayerX_; }
    float menuPlayerY() const { return menuPlayerY_; }
    bool menuPlayerGrounded() const { return menuPlayerGrounded_; }
    bool menuPlayerVisible() const { return !menuPlayerGoalStop_; }
    float menuCameraViewX() const { return menuCameraX_ - surfaceWidth / 2.0F; }
    float menuCameraViewY() const { return menuCameraY_ - surfaceHeight / 2.0F; }
    bool menuTitleReady() const { return menuTitleReady_; }
    bool menuTitleRevealStarted() const { return menuTitleFirstBoot_; }
    bool menuTeleportActive() const { return menuTeleportActive_; }
    bool enterSequenceActive() const { return enterSequenceActive_; }
    float enterSequenceY() const { return enterSequenceY_; }
    float playMenuYOffset() const { return playMenuYOffset_; }
    float playMenuTileY() const { return playMenuTileY_; }
    float playMenuDitherY() const { return playMenuDitherY_; }
    float playMenuRunFrame() const { return playMenuRunFrame_; }
    float playMenuSmoothX() const { return playMenuSmoothX_; }
    int playMenuCursor() const { return playMenuCursor_; }
    bool playMenuSelected() const { return playMenuSelected_; }
    int playStyle() const { return playStyle_; }
    float playMenuPlayerDescent() const { return playMenuPlayerDescent_; }
    float playMenuVerticalDitherY() const { return playMenuVerticalDitherY_; }
    float mainPlayerX() const { return mainPlayerX_; }
    // Pure query of the implemented area-one collision snapshot, room coordinates.
    LaserSightRay traceLaserSight(LaserSightRay::Coordinate x, LaserSightRay::Coordinate y,
        LaserSightRay::Coordinate bottom, std::uint32_t speedTurns, std::uint32_t aimTurns = 0) const;
    LaserSightRay mainLaserSightEndpoint() const {
        return traceLaserSight(LaserSightRay::fromQ8(mainPlayerMotion_.offsetXFixed(), mainPlayerOriginX_),
            LaserSightRay::fromQ8(mainPlayerMotion_.offsetYFixed(), mainPlayerOriginY_),
            {mainCameraRenderViewY() + surfaceHeight, 0},
            LaserSightRay::turnsFromQ8Degrees(mainPlayerMotion_.speedXFixed()));
    }
    float mainPlayerY() const { return mainPlayerY_; }
    float mainPlayerSpeedX() const { return mainPlayerSpeedX_; }
    float mainPlayerSpeedY() const { return mainPlayerSpeedY_; }
    bool mainPlayerGrounded() const { return mainPlayerGrounded_; }
    int mainPlayerFacing() const { return mainPlayerFacing_; }
    int mainPlayerSpriteId() const { return mainPlayerSpriteId_; }
    float mainPlayerImageIndex() const { return mainPlayerImageIndex_; }
    float mainCameraViewX() const { return static_cast<float>(mainCameraRenderViewX()); }
    float mainCameraViewY() const { return static_cast<float>(mainCameraRenderViewY()); }
    int mainCameraRenderViewX() const { return mainCameraMotion_.pixelX() - surfaceWidth / 2; }
    int mainCameraRenderViewY() const { return mainCameraMotion_.pixelY() - surfaceHeight / 2; }
    int mainAmmo() const { return mainAmmo_; }
    unsigned mainAmmoHudOffset() const { return run_.ammoHud.offset; }
    unsigned mainAmmoMeterOffset() const { return run_.ammoHud.meterOffset; }
    const AmmoGaugeRows& mainAmmoGauge() const { return run_.ammoGauge; }
    int mainAmmoCapacity() const { return mainPlayerMotion_.ammoCapacity(); }
    int mainPlayerHealth() const { return mainPlayerHealth_; }
    bool subtractPlayerHealth(MotionBinary64 amount){
        if(!run_.effectsSucceeded)return false;
        return run_.effectsSucceeded=downwell::subtractPlayerHealth(mainPlayerHealth_,amount);
    }
    bool emitPlayerDamageNumber(MotionBinary64 amount,MotionBinary64 direction){
        if(!run_.effectsSucceeded)return false;
        MotionBinary64 x,y;
        if(!mainPlayerInitialized_||!ContactCoordinates::playerGlobals(mainPlayerMotion_.frameStartXFixed(),
            mainPlayerMotion_.frameStartYFixed(),mainPlayerOriginX_,mainPlayerOriginY_,x,y))return run_.effectsSucceeded=false;
        return run_.createDamageNumber(x,y,amount,direction)!=0;
    }
    const PlayerDeathSequence& mainDeathSequence() const { return mainDeath_; }
    const RunGemStatistics& runGemStatistics() const { return run_.gems; }
    std::uint32_t runTimeTicks() const { return run_.clock.ticks; }
    const RunCombo& runCombo() const { return run_.combo; }
    bool grantConsumable(unsigned id) { return run_.consumables.grant(id, currencyAccount_); }
    bool grantUpgrade(unsigned id) {
        return run_.upgrades.grant(id, run_.consumables, currencyAccount_,
            mainPlayerMotion_, mainPlayerHealth_, run_.combo.healthReserve);
    }
    const UpgradeInventory& upgrades() const { return run_.upgrades; }
    const PlayerCompanionCreateState* companionCreateState(std::uint32_t id)const{
        if(!instances_.find(id))return nullptr;
        for(const auto& state:companionCreates_)if(state.sourceId==id)return &state;
        return nullptr;
    }
    unsigned droneNumber()const{return droneNumber_;}
    int mainNextMuzzleOffset() const { return mainEmitter_.leg; }
    const Consumables& consumables() const { return run_.consumables; }
    enum class RisingTextPhase {alarm,step,cleanup};
    template<class Effects> bool collideScatteredPlayer(InstanceHandle key,Effects& effects){
        auto* r=instances_.resolve(key);
        if(!run_.effectsSucceeded||!r||r->objectId!=289)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=ScatteredEvents::playerContact(r->scattered,random_,effects)&&run_.effectsSucceeded;
    }
    template<unsigned Capacity,class Position,class Player,class Audio>
    bool collideScatteredPlayerWithBlast(InstanceHandle key,StoredBlastRegistration<instanceCapacity,Capacity>& blasts,
        Position position,Player& player,SoundPlayEnvironment& environment,Audio& audio){
        auto* r=instances_.resolve(key);
        if(!run_.effectsSucceeded||!r||r->objectId!=289||!blasts.belongsTo(instances_))return run_.effectsSucceeded=false;
        auto blast=[&]{MotionBinary64 x,y;InstanceHandle child;
            return position(key,x,y)&&spawnBlast(blasts,x,y,child);
        };
        auto events=registeredScatteredContact(*this,r->scattered,player,blast,r->sound,environment,audio,run_.effectsSucceeded);
        return collideScatteredPlayer(key,events);
    }
    template<unsigned Capacity,class Position,class Tile,class Audio,class Tick>
    bool withScatteredEvents(StoredCurrencyRegistration<instanceCapacity,Capacity>& gems,
        Position position,Tile tile,SoundPlayEnvironment& environment,Audio& audio,Tick tick){
        if(!run_.effectsSucceeded||scatteredStep_||!gems.belongsTo(instances_)||!gems.valid())
            return run_.effectsSucceeded=false;
        auto step=[&](InstanceHandle key){return stepScattered(key,gems,position,tile,environment,audio);};
        scatteredStepContext_=&step;
        scatteredStep_=[](void* p,InstanceHandle h){return (*static_cast<decltype(step)*>(p))(h);};
        struct Reset {StartupRuntime& w;~Reset(){w.scatteredStep_=nullptr;w.scatteredStepContext_=nullptr;}} reset{*this};
        const bool ok=tick();return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&gems.valid();
    }
    template<unsigned Capacity,class Position,class Tile,class Audio>
    bool stepScattered(InstanceHandle key,StoredCurrencyRegistration<instanceCapacity,Capacity>& gems,
        Position position,Tile tile,SoundPlayEnvironment& environment,Audio& audio){
        auto* caller=instances_.resolve(key);
        if(!run_.effectsSucceeded||!caller||caller->objectId!=289||!gems.belongsTo(instances_))return run_.effectsSucceeded=false;
        struct Effects {
            StartupRuntime& w;InstanceHandle key;StoredCurrencyRegistration<instanceCapacity,Capacity>& gems;
            Position& position;Tile& tile;SoundPlayEnvironment& environment;Audio& audio;InstanceHandle debris{};
            InstanceRecord* caller(){auto* r=w.instances_.resolve(key);return r&&r->objectId==289?r:nullptr;}
            bool outOfView(){
                MotionBinary64 x,y;if(!position(key,x,y))return false;
                auto view=MotionBinary64::fromInteger(w.ordinaryStepViewTopQ12_);
                if(!view.zero())view.high-=12u<<20;
                auto top=view,bottom=view;int order;
                if(!top.add(MotionBinary64::fromInteger(-160))||!compareRunnerReal(y,top,order))return false;
                bool outside=order<0;
                if(!outside){if(!bottom.add(MotionBinary64::fromInteger(3000))||!compareRunnerReal(y,bottom,order))return false;outside=order>0;}
                if(!outside||w.roomId_!=mainRoomId)return true;
                auto* r=caller();if(!r)return false;if(r->destroyRequested)return true;
                if(w.lifecycle_.hasOwner()&&!w.lifecycle_.markDestroyed(key,*r))return false;
                r->destroyRequested=true;r->outOfViewDestroyRequested=true;return true;
            }
            bool currency(int amount){
                return w.spawnRegisteredCurrency(gems,[&](auto& out){
                    if(!position(key,out.x,out.y))return false;const auto* r=caller();if(!r)return false;
                    out.objectIndex=289;out.moniAmount=r->boxReward.moniAmount;out.myGem=r->boxReward.myGem;
                    out.atrState=r->boxReward.atrState;out.stomped=MotionBinary64::fromInteger(r->scattered.stomped);return true;
                },[&](auto v){auto* r=caller();if(!r)return false;r->boxReward.moniAmount=v;return true;},
                [&](bool v){auto* r=caller();if(!r)return false;r->boxReward.atrState=v;return true;},
                [&](std::uint32_t v){auto* r=caller();if(!r)return false;r->boxReward.myGem=v;return true;},
                MotionBinary64::fromInteger(amount),tile);
            }
            bool attractGem(){const auto* r=caller();const auto* child=r?w.instances_.find(r->boxReward.myGem):nullptr;
                return child&&gems.attracted(gems.handle(*child),true);}
            bool createDebris(){MotionBinary64 x,y;return position(key,x,y)&&w.spawnDebris(x,y,w.run_.stage.area,debris);}
            bool debrisYSpeed(MotionBinary64 v){return w.ownedDebrisRegistration_.setYSpeed(debris,w.ownedDebrisEffects_,v);}
            bool sound(int id,int priority,int loop,int cut){auto* r=caller();int voice;
                return r&&SoundPlayScript::plain(r->sound,environment,audio,id,priority,loop!=0,cut!=0,voice,w.run_.effectsSucceeded);}
            bool sprite(int id){auto* r=caller();if(!r||!w.lifecycle_.sprite(key,*r,id))return false;r->spriteId=id;return true;}
        } effects{*this,key,gems,position,tile,environment,audio};
        return run_.effectsSucceeded=ScatteredEvents::step(caller->scattered,random_,effects)&&run_.effectsSucceeded;
    }
    template<unsigned Capacity,class Lookup,class Tick>
    bool withShopItemEvents(StoredShopChildRegistration<instanceCapacity,Capacity>& owner,Lookup lookup,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(shopItemStep_||!owner.belongsTo(instances_)||!owner.valid())return run_.effectsSucceeded=false;
        auto step=[&](const InstanceRecord& record){const auto key=owner.handle(record);return key.generation&&owner.stepItem(key,lookup);};
        shopItemStepContext_=&step;
        shopItemStep_=[](void* p,const InstanceRecord& r){return (*static_cast<decltype(step)*>(p))(r);};
        struct Reset {StartupRuntime& world;~Reset(){world.shopItemStep_=nullptr;world.shopItemStepContext_=nullptr;}} reset{*this};
        const bool ok=tick();return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&owner.valid();
    }
    template<unsigned Capacity,class Ops,class Render>
    bool withRisingTextDraw(StoredShopChildRegistration<instanceCapacity,Capacity>& owner,Ops& ops,Render render){
        if(risingTextDraw_||!owner.belongsTo(instances_)||!owner.valid())return false;
        auto draw=[&](const InstanceRecord& record){const auto key=owner.handle(record);return key.generation&&owner.drawRisingText(key,ops);};
        risingTextDrawContext_=&draw;
        risingTextDraw_=[](void* p,const InstanceRecord& r){return (*static_cast<decltype(draw)*>(p))(r);};
        struct Reset {StartupRuntime& world;~Reset(){world.risingTextDraw_=nullptr;world.risingTextDrawContext_=nullptr;}} reset{*this};
        return render()&&owner.valid();
    }
    bool drawRisingText(const InstanceRecord& record)const{
        return record.objectId==212&&risingTextDraw_&&risingTextDraw_(risingTextDrawContext_,record);
    }
    template<unsigned Capacity,class Tick>
    bool withRisingTextEvents(StoredShopChildRegistration<instanceCapacity,Capacity>& owner,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(risingTextDispatch_||!owner.belongsTo(instances_)||!owner.valid())return run_.effectsSucceeded=false;
        auto dispatch=[&](const InstanceRecord& r,RisingTextPhase phase,unsigned number){
            const auto key=owner.handle(r);if(!key.generation)return false;
            if(phase==RisingTextPhase::alarm)return owner.risingTextAlarmEvent(key,number);
            if(phase==RisingTextPhase::cleanup)return owner.retire(key);
            return owner.stepRisingText(key);
        };
        risingTextContext_=&dispatch;
        risingTextDispatch_=[](void* p,const InstanceRecord& r,RisingTextPhase phase,unsigned number){
            return (*static_cast<decltype(dispatch)*>(p))(r,phase,number);
        };
        struct Reset {StartupRuntime& world;~Reset(){world.risingTextDispatch_=nullptr;world.risingTextContext_=nullptr;}} reset{*this};
        const bool ok=tick();return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&owner.valid();
    }
    template<unsigned Capacity,class Audio>
    StoredShopCreationOwner<instanceCapacity,Capacity,Audio> shopCreationOwner(const ObjectPack& objects,Audio& audio){
        return {instances_,objects,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded,audio};
    }
    // Borrow a concrete room owner for synchronous builder Shop births. This
    // scope does not admit later shop events; they require their own handlers.
    template<unsigned Capacity,class Audio,class Execute>
    bool withRegisteredShopCreation(StoredShopCreationOwner<instanceCapacity,Capacity,Audio>& owner,Execute execute){
        if(!run_.effectsSucceeded)return false;
        if(shopCreate_||!owner.belongsTo(instances_,objectPack_)||!owner.valid())return run_.effectsSucceeded=false;
        auto create=[&](int x,int y){
            return run_.createRegisteredShop(x,y,[this](unsigned lo,unsigned hi){return lo+random_.irandomSmall(hi-lo);},
                [this]{return random_.chooseIndexSmall(3);},mainPlayerHealth_,owner);
        };
        shopCreateContext_=&create;
        shopCreate_=[](void* context,int x,int y){return (*static_cast<decltype(create)*>(context))(x,y);};
        struct Reset {StartupRuntime& world;~Reset(){world.shopCreate_=nullptr;world.shopCreateContext_=nullptr;}} reset{*this};
        const bool ok=execute();
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok&&owner.valid();
    }
    ShopWorld::Instance* registerShop(int x, int y) {
        if(!run_.effectsSucceeded)return nullptr;
        if(shopCreate_)return shopCreate_(shopCreateContext_,x,y);
        return run_.createShop(x, y, [this](unsigned lo, unsigned hi) {
            return lo + random_.irandomSmall(hi - lo);
        }, [this] { return random_.chooseIndexSmall(3); }, mainPlayerHealth_);
    }
    const ShopWorld& shops() const { return run_.shops; }
    const FlashBallEffects<32>& flashEffects() const {return run_.flashEffects;}
    const CorpseExpiryEffects<32>& corpseExpiryEffects()const{return run_.corpseExpiryEffects;}
    const PlayerDamageNumbers<8>& damageNumbers()const{return run_.damageNumbers;}
    template<class Expiry,class Number> bool visitForegroundEffects(Expiry expiry,Number number)const{
        return run_.visitForegroundEffects(expiry,number);
    }
    bool emitDamageNumber(MotionBinary64 x,MotionBinary64 y,MotionBinary64 amount,MotionBinary64 direction){
        return run_.createDamageNumber(x,y,amount,direction)!=0;
    }
    bool emitCorpseExpiry(MotionBinary64 x,MotionBinary64 y){return run_.emitCorpseExpiry(x,y);}
    const SmokeEffects<32>& smokeEffects() const {return run_.smokeEffects;}
    bool emitBoxSmoke(MotionBinary64 x,MotionBinary64 y){return run_.emitBoxSmoke(x,y,random_);}
    bool emitBoxFlash(MotionBinary64 x,MotionBinary64 y){return run_.emitBoxFlash(x,y,random_);}
    bool effectsSucceeded()const{return run_.effectsSucceeded;}
    const WallBackgroundTiles<2048>& wallBackgroundTiles()const{return run_.wallBackground;}
    bool addWallBackground(MotionBinary64 x,MotionBinary64 y){return run_.addWallBackground(x,y);}
    bool emitSmokeBurst(MotionBinary64 x,MotionBinary64 y){return run_.emitSmokeBurst(x,y,random_);}
    bool emitFlashBurst(MotionBinary64 x,MotionBinary64 y) {
        return run_.emitFlashBurst(x,y,random_);
    }
    bool registerSideCameraLocker(int x, int y) { return run_.sideCameraLockers.create(x, y); }
    const OrdinaryTimeFields<64>& timeFields() const {return run_.timeFields;}
    // Runtime object-82 creation (Timeout), unlike adoption of a builder's
    // already consumed Create sample. Publish the room identity before RNG.
    bool spawnOrdinaryTimeField(int x,int y,InstanceHandle& child);
    template<class Caller,class Events> bool typicalPlayerDamage(Caller& caller,
        MotionBinary64 amount,MotionBinary64 horizontal,MotionBinary64 vertical,Events& events){
        if(!run_.effectsSucceeded)return false;
        return run_.effectsSucceeded=caller.execute(amount,horizontal,vertical,events,
            [&](int x,int y){InstanceHandle child;return spawnOrdinaryTimeField(x,y,child);});
    }
    bool cameraSideLocked() const { return mainCameraMotion_.sideLocked; }
    std::uint64_t sideCameraDigest() const { return run_.sideCameraLockers.digest(mainCameraMotion_); }
    void registerLevelExit(int y);
    bool levelExitPresent() const { return run_.levelExitPresent; }
    const LevelExit& levelExit() const { return run_.levelExit; }
    const GoalTimeField& goalField() const { return run_.goalField; }
    const GoalTimeField* startTimeField() const { return run_.startFieldPresent?&run_.startField:nullptr; }
    unsigned goalFieldBirthBoundary() const { return run_.goalFieldBirthBoundary; }
    bool goalStopped() const { return run_.goalStopped; }
    bool exitDitherRequested() const { return run_.exitDitherRequested; }
    const ExitDither& exitDither() const { return run_.exitDither; }
    bool levelResultRequested() const { return run_.levelResultRequested; }
    // Scene owner retires old presentation before entering. While active,
    // update via stepLevelClear, not the main-room event scheduler.
    bool enterLevelClear(unsigned style, bool heartGoal);
    template<class Emit>
    bool stepLevelClear(bool& confirm, bool leftPressed, bool rightPressed, bool& heartGoal, Emit emit) {
        if (!run_.effectsSucceeded || !run_.levelClearActive || run_.nextLevelRequested) return false;
        ++tick_;
        const bool ok = run_.stepLevelClearPositioned(confirm, leftPressed, rightPressed, heartGoal,
            currencyAccount_, mainPlayerMotion_, mainPlayerHealth_, random_, emit,
            [this](int x,int y){return createResultDither(x,y);},
            [this](auto event){return this->advanceResultRoomAlarms(event);},
            [this](std::int32_t y){return publishResultPosition(y);},
            [this]{return publishResultDither();});
        if(!ok)return run_.effectsSucceeded=false;
        mainAmmo_ = mainPlayerMotion_.ammo();
        return ok;
    }
    bool levelClearActive() const { return run_.levelClearActive; }
    bool nextLevelRequested() const { return run_.nextLevelRequested; }
    bool loadNextAreaOneLevel(const Input& input);
    const LevelClearController& levelClear() const { return run_.levelClear; }
    const LevelClearPresentation::Frame& levelClearFrame() const { return run_.levelClearFrame; }
    const RunStage& runStage() const { return run_.stage; }
    int mainPlayerHealthMaximum() const { return run_.combo.healthReserve.maximum; }
    const HealthReserve& mainHealthReserve() const { return run_.combo.healthReserve; }
    const DeathResults& deathResults() const { return run_.results; }
    int mainPlayerInvulnerability() const { return mainPlayerInvulnerability_; }
    bool mainPlayerVisible() const { return playerDamageVisible(mainPlayerInvulnerability_); }
    std::uint32_t mainShotsFired() const { return mainShotsFired_; }
    std::uint32_t mainBlocksDestroyed() const { return mainBlocksDestroyed_; }
    std::uint32_t mainLandings() const { return mainLandings_; }
    std::uint32_t mainEnemiesActivated() const { return mainEnemiesActivated_; }
    std::uint32_t mainInstancesRetired() const { return mainInstancesRetired_; }
    std::uint32_t mainEnemiesDefeated() const { return run_.kills; }
    std::uint32_t mainJumps() const { return mainJumps_; }
    std::size_t mainBulletCount() const { return mainBulletCount_; }
    const MainBulletState& mainBulletAt(std::size_t index) const {
        return mainBullets_[index];
    }
    std::size_t mainCurrencyCount() const { return mainCurrencyCount_; }
    const MainCurrencyState& mainCurrencyAt(std::size_t index) const {
        return mainCurrencyItems_[index];
    }
    std::uint32_t mainCurrencyTotal() const { return currencyAccount_.currency; }
    template<class Controls> auto currencyGainOperations(Controls controls){
        return CurrencyGainAccountOperations<Controls>(currencyAccount_,controls,&run_.gemStreakRise);
    }
    bool drawGemStreakRise(std::int32_t& worldY) const {
        return run_.gemStreakRise.draw(currencyAccount_.streak,mainDeathSequence().active,
            MotionBinary64::fromInteger(mainCameraRenderViewY()),true,worldY);
    }
    std::uint32_t mainGameGemTotal() const { return currencyAccount_.gameGem; }
    std::uint32_t mainGemStreak() const { return currencyAccount_.streak; }
    std::uint32_t mainGemStreakTimer() const { return currencyAccount_.timer; }
    bool mainBuilderInitialized() const { return mainBuilderInitialized_; }
    int mainBuilderY() const { return mainBuilderY_; }
    int mainBuilderGenerationCount() const { return mainBuilderGenerationCount_; }
    std::uint32_t mainBuilderRowsGenerated() const {
        return mainBuilderRowsGenerated_;
    }
    int mainBuilderRightRoom(std::size_t index) const {
        return mainBuilderRightRooms_[index];
    }
    int mainBuilderStartPattern() const { return mainBuilderStartPattern_; }
    bool mainBuilderStartReversed() const { return mainBuilderStartReversed_; }
    std::uint32_t mainBuilderStartRowsGenerated() const {
        return mainBuilderStartRowsGenerated_;
    }
    int mainBuilderLastMainPattern() const { return mainBuilderLastMainPattern_; }
    bool mainBuilderLastPatternReversed() const {
        return mainBuilderLastPatternReversed_;
    }
    std::uint32_t mainBuilderMainPatternsGenerated() const {
        return mainBuilderMainPatternsGenerated_;
    }
    int mainBuilderLastRightPattern() const { return mainBuilderLastRightPattern_; }
    std::uint32_t mainBuilderSideRoomsGenerated() const {
        return mainBuilderSideRoomsGenerated_;
    }
    std::uint32_t mainBuilderShopsGenerated() const {
        return mainBuilderShopsGenerated_;
    }
    bool mainBuilderShopSuppressed() const { return mainBuilderShopMade_; }
    bool mainBuilderEndReached() const { return mainBuilderEndReached_; }
    int mainBuilderEndPattern() const { return mainBuilderEndPattern_; }
    std::uint32_t mainBuilderSpecialObjectsGenerated() const {
        return mainBuilderSpecialObjectsGenerated_;
    }
    std::uint32_t randomSeed() const { return random_.seed(); }
    std::uint64_t randomDrawCount() const { return random_.drawCount(); }
    const GameMakerRandom& randomState() const { return random_; }
    // Room-entry handoff: pair with randomState() before the first builder Step.
    const AreaOneBuilder& mainBuilderState() const { return areaOneBuilder_; }
    long mainEnemyTargetX() const { return mainPlayerMotion_.frameStartXFixed(); }
    long mainEnemyTargetY() const { return mainPlayerMotion_.frameStartYFixed(); }
    std::uint64_t stateDigest() const;
#ifdef DOWNWELL_TRACE_AREA_ONE
    void traceSetMainCameraViewY(float viewY) {
        mainCameraY_ = viewY + surfaceHeight / 2.0F;
        mainCameraTargetY_ = mainCameraY_;
        mainCameraMotion_.y = mainCameraMotion_.targetY =
            static_cast<long>(mainCameraY_ * 4096.0F);
    }
#endif

private:
    bool publishBuilderRow(int y);
    void applyFrogAlarm(InstanceRecord& instance, const FrogAlarmEffect& effect);
    void controllerCreate(PlatformProfile profile);
    void initRoomCreate();
    void initRoomAlarm0();
    void initRoomEndStep(bool advancePressed);
    void stepInternal(bool advancePressed, bool left, bool right, bool fire,
                      bool anyInput);
    void updateMenuPlayer(InstanceRecord& instance);
    void updateMenuCamera();
    void updateMainPlayer(InstanceRecord& instance);
    void updateMainCamera(InstanceRecord& instance);
    bool publishMainCameraPosition(InstanceRecord& instance);
    bool readEntryCameraX(int room,MotionBinary64& value)const;
    bool readEntryCameraCoordinate(int room,bool vertical,MotionBinary64& value)const;
    bool stepDitherAbove(InstanceRecord& instance);
    bool publishDitherAboveCoordinate(InstanceHandle key,bool vertical,RunnerImageValue value);
    bool ditherAboveLowSpec_=false,ditherAboveEnding_=false;
    bool createOwnedEntryDither(int room);
    void updateMainBuilder(InstanceRecord& instance);
    void syncMainBuilderState();
    void syncMainPlayerState();
    bool mainSolidAt(float x, float y) const;
    void emitMainBullet(int x,int y);
    void updateMainBullets();
    bool publishMainBullet(MainBulletState& bullet,bool angle);
    bool endStepMainBullet(InstanceRecord& instance);
    bool stepSourceMainBullet(InstanceRecord& instance);
    bool cleanupMainOwnedDestroyed();
    bool clearMainBullets();
    bool publishMainInactivation(const InstanceRecord& record);
    bool publishMainEnemyImage(std::size_t index,unsigned phase);
    bool publishMainEnemyTransform(std::size_t index,RunnerTransformField field,int value);
    void spawnMainCurrency(float x, float y, std::int32_t amount,
                           bool attracted = false);
    void updateMainCurrency();
    void updateMainLifecycle(bool endStep, long publishedViewTopQ12);
    void updateMainEnemy(InstanceRecord& enemy);
    void updateCrawler(InstanceRecord& enemy, bool alarm);
    void updateMainEnemyInteractions();
    InstanceRecord* mainBulletEnemyCollision(const ProjectileSegment& path);
    InstanceRecord* mainBulletCollision(const ProjectileSegment& path);
    void dispatchInstancePhase(InstanceRecord& instance,
                               RoomInstancePhase phase);
    bool dispatchObjectEvent(InstanceRecord& instance,
                             std::int32_t eventType,
                             std::int32_t eventSubtype,
                             bool advancePressed = false);
    void dispatchRoomCreation(std::int32_t roomId);
    void flushDeferredInstances(InstanceStore<instanceCapacity>& instances,
                                std::int32_t roomId);
    bool createLevelExitChild(LevelExit::Effect effect,int x,int y);
    void executeCode(InstanceRecord* instance,
                     std::int32_t codeId,
                     bool advancePressed = false);
    void gotoRoom(int roomId);
    void loadRoom(int roomId, bool firstGameRoom);
    struct ResultEntryRequest { unsigned style; bool heartGoal; };
    const ResultEntryRequest* resultEntryRequest_=nullptr;
    bool createResultDither(int viewX,int viewY);
    bool publishResultDither();
    bool publishResultPosition(std::int32_t y);
    InstanceHandle resultDither_{};
    template<class Emit> bool advanceResultRoomAlarms(Emit emit){
        const auto healthy=[&]{return run_.effectsSucceeded&&deferredCommandsSucceeded_&&roomLoadSucceeded_;};
        if(!objectPack_||roomId_!=resultRoomId||!healthy())return false;
        if(!EventScheduler<instanceCapacity>::visitAlarmPhaseChecked(instances_,
            [&](InstanceRecord& record,unsigned alarm){
                PackedObjectEvent binding;
                if(!objectPack_->findEvent(record.objectId,2,alarm,binding))return true;
                if(record.objectId==325){
                    if(record.instanceId!=102522)return false;
                    return run_.levelClear.selection.advanceAlarmChecked(alarm,emit);
                }
                if(record.objectId==68){
                    if(instances_.resolve(resultDither_)!=&record||alarm!=0)return false;
                    run_.exitDither.alarm0=record.alarms.get(0);
                    run_.exitDither.advanceAlarm();record.alarms.set(0,run_.exitDither.alarm0);
                    return true;
                }
                if(!record.alarms.advanceOne(alarm))return true;
                const auto missing=unimplementedCodeCalls_;
                return dispatchObjectEvent(record,2,alarm)&&missing==unimplementedCodeCalls_&&healthy();
            },healthy))return false;
        return instances_.reconcileObjectMembership([&](InstanceHandle key,const InstanceRecord& record,bool member){
            return lifecycle_.membership(key,record,member)&&healthy();
        })&&roomLayers_.serviceDepths()&&healthy();
    }
    void resetSceneState();
    InstanceRecord* initRoomInstance();
    const InstanceRecord* initRoomInstance() const;

    std::uint64_t tick_ = 0;
    int roomId_ = entryRoomId;
    int roomDestination_ = 0;
    int showSplash_ = 0;
    InstanceStore<instanceCapacity> instances_;
    StoredSpawnerStates<instanceCapacity,8> spawners_{instances_};
    void* spawnerContext_=nullptr;
    bool (*spawnerDispatch_)(void*,InstanceHandle)=nullptr;
    void* whiteJellyContext_=nullptr;
    bool (*whiteJellyDispatch_)(void*,InstanceHandle,int)=nullptr;
    RunnerLayerIds layerIds_;
    StoredRoomLayers<instanceCapacity,256,4096,8> roomLayers_{layerIds_};
    bool layerBindingSucceeded_;
    InstanceLifecycleBinding lifecycle_;
    ScriptEffectRuntimeBinding scriptEffectsBinding_;
    BoxStepDispatch boxStepDispatch_;
    ScriptEffectRuntimeBinding debrisBinding_;
    const DebrisAnimation* (*debrisLookup_)(const void*,InstanceHandle,const InstanceRecord&)=nullptr;
    template<class Effects,class Registry> void bindDebrisDraw(Effects& effects,Registry& registry){
        debrisBinding_=ScriptEffectRuntimeBinding::bind<InstanceRecord>(effects,registry);
        debrisLookup_=[](const void* p,InstanceHandle key,const InstanceRecord& record){
            return static_cast<const Registry*>(p)->drawEffect(key,record);
        };
    }
    bool timeStopRoomWidth(int& width);
    bool hasDebrisRecords()const{
        for(std::size_t i=0;i<instances_.size();++i)if(instances_.at(i).objectId==370)return true;
        return false;
    }
    bool clearRoomDebris(){
        if(hasDebrisRecords()&&!debrisBinding_.clearRoom())return run_.effectsSucceeded=false;
        debrisBinding_={};return true;
    }
    void* debrisEventContext_=nullptr;
    bool (*debrisEventDispatch_)(void*,const InstanceRecord&,StepEvent)=nullptr;
    bool hasScriptEffectRecords()const{
        for(std::size_t i=0;i<instances_.size();++i)if(instances_.at(i).objectId==219)return true;
        return false;
    }
    bool clearRoomScriptEffects(){
        if(hasScriptEffectRecords()&&(!run_.effectsSucceeded||!scriptEffectsBinding_.clearRoom()))return run_.effectsSucceeded=false;
        scriptEffectsBinding_={};return true;
    }
    void* collisionContext_=nullptr;
    bool (*collisionDispatch_)(void*)=nullptr;
    void* scriptAnimationContext_=nullptr;
    bool (*scriptAnimationDispatch_)(void*,InstanceHandle,const InstanceRecord&)=nullptr;
    bool (*droneDispatch_)(void*,const InstanceRecord&,DronePhase)=nullptr;
    void* currencyContext_=nullptr;
    bool (*currencyDispatch_)(void*,const InstanceRecord&,CurrencyPhase,unsigned)=nullptr;
    bool (*boneDispatch_)(void*,const InstanceRecord&,BonePhase)=nullptr;
    void* boneContext_=nullptr;
    bool (*bloodDispatch_)(void*,const InstanceRecord&,BloodPhase)=nullptr;
    void* bloodContext_=nullptr;
    bool (*skeletonDispatch_)(void*,const InstanceRecord&,SkeletonPhase,unsigned)=nullptr;
    void* skeletonContext_=nullptr;
    bool (*levelNumberDispatch_)(void*,const InstanceRecord&,bool,unsigned)=nullptr;
    void* levelNumberContext_=nullptr;
    void* levelNumberDrawContext_=nullptr;
    bool (*levelNumberDraw_)(void*,const InstanceRecord&,LevelNumberDrawOps&)=nullptr;
    bool (*corpseDispatch_)(void*,const InstanceRecord&,CorpsePhase,unsigned)=nullptr;
    void* corpseContext_=nullptr;
    void* corpseExpiryContext_=nullptr;
    void* corpseDrawContext_=nullptr;
    const CorpseAnimation* (*corpseDrawState_)(void*,const InstanceRecord&)=nullptr;
    void* smokeContext_=nullptr;
    void* limboShardContext_=nullptr;
    void* limboShardDrawContext_=nullptr;
    const LimboShard* (*limboShardDrawState_)(void*,const InstanceRecord&)=nullptr;
    bool (*limboShardDispatch_)(void*,const InstanceRecord&,LimboShardPhase)=nullptr;
    void* consumptionBloodContext_=nullptr;
    void* flashBallContext_=nullptr;
    bool (*flashBallDispatch_)(void*,const InstanceRecord&,bool)=nullptr;
    int* momentDelayRoomSpeed_=nullptr;
    bool (*consumptionBloodDispatch_)(void*,const InstanceRecord&,ConsumptionBloodPhase,unsigned)=nullptr;
    bool (*smokeDispatch_)(void*,const InstanceRecord&,SmokePhase,unsigned)=nullptr;
    bool (*blastDispatch_)(void*,const InstanceRecord&,BlastPhase,unsigned)=nullptr;
    void* blastContext_=nullptr;
    bool (*corpseExpiryDispatch_)(void*,const InstanceRecord&,bool)=nullptr;
    void* droneContext_=nullptr;
    void* timeEffectContext_=nullptr;
    void* entryDitherContext_=nullptr;
    void* entryDitherCreateContext_=nullptr;
    bool (*entryDitherCreate_)(void*,int)=nullptr;
    bool (*entryDitherDispatch_)(void*,InstanceRecord&,bool)=nullptr;
    bool (*timeEffectDispatch_)(void*,InstanceRecord&,TimeEffectPhase,unsigned)=nullptr;
    const RoomPack* roomPack_ = nullptr;
    const ObjectPack* objectPack_ = nullptr;
    // Object event bindings are immutable. Cache inherited alarm masks, not
    // per-instance alarm values; IDs outside this cache use normal lookup.
    std::array<std::int16_t,512> alarmEventMasks_{};
    static_assert(AlarmBank::count < 16, "Alarm cache reserves negative values for misses");
    const BuilderPatternPack* builderPatternPack_ = nullptr;
    std::uint32_t randomSeed_ = defaultRandomSeed;
    GameMakerRandom::Lookahead randomLookahead_;
    GameMakerRandom random_;
    PlatformProfile profile_ = PlatformProfile::desktop;
    int initTimes_ = 0;
    int achievementLoginCalls_ = 0;
    int splashSpeed_ = 180;
    int splashDitherTenths_ = 110;
    bool roomDestinationSet_ = false;
    bool roomLoadSucceeded_ = true;
    std::uint32_t implementedCodeCalls_ = 0;
    std::uint32_t unimplementedCodeCalls_ = 0;
    int cameraViewWidth_ = 0;
    RoomEntryViews roomEntryViews_;
    int cameraViewHeight_ = 0;
    bool controlsLocked_ = false;
    std::uint32_t soundGainRequests_ = 0;
    std::uint32_t soundPlayRequests_ = 0;
    std::uint32_t ditherFadeSpawnRequests_ = 0;
    // A single 21x18 recovered side-room pattern can emit more than 128
    // objects before GameMaker returns to the step loop.
    DeferredInstanceQueue<512> deferredInstances_;
    std::uint32_t nextRuntimeInstanceId_ = 105187;
    void* shopCreateContext_=nullptr;
    void* shopItemStepContext_=nullptr;
    void* scatteredStepContext_=nullptr;
    bool (*scatteredStep_)(void*,InstanceHandle)=nullptr;
    bool (*shopItemStep_)(void*,const InstanceRecord&)=nullptr;
    void* risingTextContext_=nullptr;
    void* risingTextDrawContext_=nullptr;
    bool (*risingTextDraw_)(void*,const InstanceRecord&)=nullptr;
    bool (*risingTextDispatch_)(void*,const InstanceRecord&,RisingTextPhase,unsigned)=nullptr;
    ShopWorld::Instance* (*shopCreate_)(void*,int,int)=nullptr;
    bool deferredCommandsSucceeded_ = true;
    bool inputLeft_ = false;
    bool inputRight_ = false;
    bool inputLeftPressed_ = false;
    bool inputRightPressed_ = false;
    bool inputFire_ = false;
    bool inputFirePressed_ = false;
    bool inputFireReleased_ = false;
    bool anyInput_ = false;
    bool menuPlayerInitialized_ = false;
    bool menuPlayerNapping_ = false;
    float menuPlayerX_ = 112.0F;
    float menuPlayerY_ = 512.0F;
    float menuPlayerSpeedX_ = 0.0F;
    float menuPlayerSpeedY_ = 0.0F;
    bool menuPlayerGrounded_ = true;
    bool menuPlayerGoalStop_ = false;
    int menuPlayerFacing_ = 1;
    int menuPlayerSpriteId_ = 31;
    float menuPlayerImageIndex_ = 0.0F;
    float menuPlayerImageSpeed_ = 0.1F;
    float menuCameraX_ = 112.0F;
    float menuCameraY_ = 512.0F;
    bool menuTitleFirstBoot_ = false;
    bool menuTitleReady_ = false;
    float menuTitleStartY_ = 432.0F;
    bool menuTeleportActive_ = false;
    bool menuTeleportSequenceStarted_ = false;
    float menuCameraLockerY_ = 448.0F;
    bool enterSequenceActive_ = false;
    bool enterSequenceBlackSet_ = false;
    float enterSequenceY_ = 448.0F;
    float enterSequenceSpeedY_ = 0.0F;
    float playMenuYOffset_ = -128.0F;
    float playMenuTileY_ = 0.0F;
    float playMenuDitherY_ = 80.0F;
    float playMenuRunFrame_ = 0.0F;
    float playMenuSmoothX_ = 0.0F;
    int playMenuCursor_ = 0;
    bool playMenuSelected_ = false;
    int playStyle_ = 0;
    float playMenuPlayerDescent_ = 0.0F;
    float playMenuPlayerSpeedY_ = 0.0F;
    float playMenuVerticalDitherY_ = -320.0F;
    bool mainPlayerInitialized_ = false;
    PlayerMotion mainPlayerMotion_{false};
    int mainPlayerOriginX_ = 240;
    int mainPlayerOriginY_ = 176;
    // Presentation/diagnostic mirrors; only syncMainPlayerState writes these
    // during simulation. PlayerMotion owns the actual player state.
    float mainPlayerX_ = 240.0F;
    float mainPlayerY_ = 176.0F;
    float mainPlayerSpeedX_ = 0.0F;
    float mainPlayerSpeedY_ = 0.0F;
    bool mainPlayerGrounded_ = false;
    int mainPlayerFacing_ = 1;
    int mainPlayerSpriteId_ = 37;
    float mainPlayerImageIndex_ = 2.0F;
    bool mainJumpShootLocked_ = false;
    bool mainShotDelayed_ = true;
    bool mainAirborneShot_ = false;
    int mainAmmo_ = 8;
    int mainPlayerHealth_ = 4;
    PlayerDeathSequence mainDeath_;
    RunSession run_;
    // One balloon and two drones per implemented player. Recycle only after
    // the corresponding real instance lifetime has left the room store.
    std::array<PlayerCompanionCreateState,3> companionCreates_{};
    unsigned droneNumber_=0;
    PlayerCompanionCreateState* initializeCompanionCreate(const InstanceRecord& instance);
    int mainPlayerInvulnerability_ = 0;
    MachineGunEmitter mainEmitter_;
    std::uint32_t mainShotsFired_ = 0;
    std::uint32_t mainBlocksDestroyed_ = 0;
    std::uint32_t mainLandings_ = 0;
    int mainActivationY_ = 0;
    std::uint32_t mainEnemiesActivated_ = 0;
    std::uint32_t mainInstancesRetired_ = 0;
    std::uint32_t mainJumps_ = 0;
    std::array<MainBulletState, mainBulletCapacity> mainBullets_{};
    bool (*sourceBulletDispatch_)(void*,InstanceHandle,MachineGunSourceMotion&)=nullptr;
    void* sourceBulletContext_=nullptr;
    std::size_t mainBulletCount_ = 0;
    std::array<MainCurrencyState, mainCurrencyCapacity> mainCurrencyItems_{};
    std::size_t mainCurrencyCount_ = 0;
    CurrencyAccount currencyAccount_;
    AreaOneBuilder areaOneBuilder_;
    bool mainBuilderInitialized_ = false;
    int mainBuilderY_ = 704;
    int mainBuilderGenerationCount_ = -30;
    std::uint32_t mainBuilderRowsGenerated_ = 0;
    std::array<int, 2> mainBuilderRightRooms_{{-1, -1}};
    int mainBuilderStartPattern_ = -1;
    bool mainBuilderStartReversed_ = false;
    std::uint32_t mainBuilderStartRowsGenerated_ = 0;
    int mainBuilderLastMainPattern_ = -1;
    bool mainBuilderLastPatternReversed_ = false;
    std::uint32_t mainBuilderMainPatternsGenerated_ = 0;
    bool mainBuilderSpawnerMade_ = false;
    int mainBuilderLastRightPattern_ = -1;
    std::uint32_t mainBuilderSideRoomsGenerated_ = 0;
    int mainBuilderLastShopPattern_ = -1;
    std::uint32_t mainBuilderShopsGenerated_ = 0;
    std::uint32_t mainBuilderRightRoomsMade_ = 0;
    bool mainBuilderShopMade_ = true;
    bool mainBuilderEndReached_ = false;
    int mainBuilderEndPattern_ = -1;
    int mainBuilderChunkOrCrate_ = 0;
    std::uint32_t mainBuilderSpecialObjectsGenerated_ = 0;
    bool mainCameraInitialized_ = false;
    float mainCameraX_ = 240.0F;
    MainCameraMotion mainCameraMotion_;
    long ordinaryStepViewTopQ12_ = 34L * 4096;
    long ordinaryStepViewLeftQ12_ = 160L * 4096;
    float mainCameraY_ = 176.0F;
    float mainCameraTargetY_ = 176.0F;
    // Stable world-owned storage; the registry outlives its borrowed effects.
    StoredEntryDitherRegistration<instanceCapacity,2> ownedEntryRegistration_{
        instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded,!lifecycle_.hasOwner()};
    StoredScriptSpawnRegistration<instanceCapacity,scriptSpawnEffectCapacity> ownedScriptRegistration_{
        instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    ScriptSpawnEffects<scriptSpawnEffectCapacity> ownedScriptEffects_;
    StoredDebrisRegistration<instanceCapacity,debrisEffectCapacity> ownedDebrisRegistration_{
        instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    DebrisEffects<debrisEffectCapacity> ownedDebrisEffects_;
};

} // namespace downwell

#endif
