#ifndef DOWNWELL_CORE_GAMEPLAY_WORLD_H
#define DOWNWELL_CORE_GAMEPLAY_WORLD_H

#include "PlayerMotion.h"
#include "PreparedRoomTerrain.h"
#include "ScriptEffectRuntimeBinding.h"
#include "BoxStepDispatch.h"
#include "Profile.h"
#include "core/GameMakerRandom.h"
#include "core/StateDigest.h"
#include "core/GameplayLimits.h"
#include "core/GemMotion.h"
#include "core/ProjectileAnimation.h"
#include "core/MachineGunTrajectory.h"
#include "core/MachineGunSourceStep.h"
#include "core/RunnerPointDirection.h"
#include "core/ProjectileSegment.h"
#include "core/LaserSightRay.h"
#include "core/MachineGunEmitter.h"
#include "core/CurrencyAccount.h"
#include "core/CurrencyGainAccountOperations.h"
#include "core/MainCameraMotion.h"
#include "core/PlayerDeathSequence.h"
#include "core/RunSession.h"
#include "core/GameplayGoalFields.h"
#include "core/GameplayExitControllers.h"
#include "core/GameplayEntryDithers.h"
#include "core/GameplayDitherAbove.h"
#include "core/GameplayBuilderController.h"
#include "core/GameplayPlayerControllers.h"
#include "core/RunControllerInstances.h"
#include "core/GameplayResultInstance.h"
#include "core/LevelNumberInstances.h"
#include "core/LimboShardInstances.h"
#include "core/FlashBallInstances.h"
#include "core/LevelNumberText.h"
#include "core/PlayerChildCreation.h"
#include "core/DroneBulletInstances.h"
#include "core/BoneProjectileInstances.h"
#include "core/SkeletonInstances.h"
#include "core/BloodInstances.h"
#include "core/CurrencyInstances.h"
#include "core/CurrencySpawnScript.h"
#include "core/CurrencyRewardFields.h"
#include "core/CurrencyCollisionBounds.h"
#include "core/RegisteredSkeletonThrow.h"
#include "core/RegisteredSkeletonStomp.h"
#include "core/BoneViewRetirement.h"
#include "core/GameplayScriptSpawnInstances.h"
#include "core/CorpseExpiryInstances.h"
#include "core/CorpseInstances.h"
#include "core/ConsumptionBloodInstances.h"
#include "core/CorpseDraw.h"
#include "core/SmokeInstances.h"
#include "core/BlastInstances.h"
#include "core/SmokeDraw.h"
#include "core/DebrisDraw.h"
#include "core/TimeShardDraw.h"
#include "core/GameplayDebrisInstances.h"
#include "core/GameplayTimeChangeRegistration.h"
#include "core/GameplayTimeShardRegistration.h"
#include "core/TimeStopEffects.h"
#include "core/ScriptSpawnOperations.h"
#include "core/EffectSpawnScript.h"
#include "core/AreaOneBuilder.h"
#include "core/EnemyCreateProfile.h"
#include "core/EnemyPresentation.h"
#include "core/ObjectPack.h"
#include "core/RoomEntryViews.h"
#include "core/PlayerLifeRules.h"
#include "core/FrogHorizontalMotion.h"
#include "core/FrogHitState.h"
#include "core/FrogPresentation.h"
#include "core/FrogBehavior.h"
#include "core/CrawlerMotion.h"
#include "core/SnailMotion.h"
#include "core/SnailHitState.h"
#include "core/WalkingEnemyMotion.h"
#include "core/BatHitState.h"
#include "core/BatBehavior.h"
#include "core/InstanceHandle.h"
#include "core/InstanceLifecycle.h"
#include "core/GameplayTerrainInstances.h"
#include "core/NativeTerrainLifetimes.h"
#include "core/RunnerStepSnapshot.h"

#include <cstdint>

enum {
    gameplayWorldBulletCapacity = downwell::gameplayBulletCapacity,
    gameplayWorldDestroyedCapacity = gameplayWorldBulletCapacity + 1,
    gameplayWorldEnemyCapacity = 64,
    gameplayWorldGemCapacity = downwell::gameplayGemCapacity
};

struct GameplayFrame {
    PlayerMotionDelta playerMotion;
    bool ammoChanged;
    bool healthChanged;
    bool currencyChanged;
    bool streakChanged;
    bool bulletChanged;
    bool scrolled;
    int scrollDelta;
    bool cameraXChanged;
    int cameraXDelta;
    unsigned destroyedBlockCount;
    int blockX[gameplayWorldDestroyedCapacity];
    int blockY[gameplayWorldDestroyedCapacity];
};

// Portable fixed-point world extracted from the native runtime. Terrain and
// generation are callbacks; no renderer, OS, DMA or platform types are owned.
// Ring-origin bookkeeping remains for compatibility during SDL migration.
namespace downwell {
class GameplayCollisionOwner;
using NativeTimeChangeRegistration=GameplayTimeChangeRegistration<8,GameplayCollisionOwner>;
using NativeTimeShardRegistration=GameplayTimeShardRegistration<64,GameplayCollisionOwner>;
}
class GameplayWorld {
public:
    static constexpr unsigned ringHeight = 320;
    static constexpr unsigned bulletCapacity = gameplayWorldBulletCapacity;
    static constexpr unsigned destroyedCapacity = gameplayWorldDestroyedCapacity;
    static constexpr unsigned enemyCapacity = gameplayWorldEnemyCapacity;
    static constexpr unsigned gemCapacity = gameplayWorldGemCapacity;
    static constexpr unsigned breakableCapacity = 512;
    typedef bool (*SolidResolver)(void* context, int x, int y);
    typedef bool (*SegmentResolver)(void* context, const downwell::ProjectileSegment& path);
    using WallContactResolver = bool (*)(void*,const downwell::ContactBounds&,unsigned&);
    using PlatformContactResolver = bool (*)(void*,const downwell::ContactBounds&,bool&);
    // Called once after existing alarms and camera Step, before bullets and
    // enemy motion. Receives the shared RNG and ceil(world-space view bottom).
    // A false return stops this frame and future steps until explicitly rebound;
    // completed Create effects are retained, not rolled back.
    using GenerationStep = bool (*)(void*, GameplayWorld&,
                                    downwell::GameMakerRandom&, int);
    void configureGenerationStep(GenerationStep step, void* context) {
        generationStep_ = step;
        generationContext_ = context;
        generationSucceeded_ = true;
    }
    bool generationSucceeded() const { return generationSucceeded_; }
    // Recreate the implemented gameplay world without a large stack temporary.
    // Owner must rebuild terrain and rebind callbacks before stepping again.
    downwell::ResultsDestination restartFromResults(downwell::DeathMenuSequence::Action action,
        std::uint32_t seed, unsigned initialRingOrigin);
    template<unsigned Capacity> downwell::ResultsDestination restartFromResultsWithScriptEffects(
        downwell::DeathMenuSequence::Action action,std::uint32_t seed,unsigned initialRingOrigin,
        downwell::ScriptSpawnEffects<Capacity>& effects){
        using downwell::ResultsDestination;
        if(!run_.effectsSucceeded||downwell::resultsDestination(action)==ResultsDestination::none)return ResultsDestination::none;
        if(!scriptSpawnInstances_||!effects.boundTo(*scriptSpawnInstances_)||!effects.valid()||!effects.clear(*scriptSpawnInstances_)){
            run_.effectsSucceeded=false;return ResultsDestination::none;
        }
        return restartFromResults(action,seed,initialRingOrigin);
    }
    void confirmFirstStage() { if (run_.stage.area == 0) run_.stage.advance(1); }

    struct BulletState {
        // Internal lifetime key, not a Windows instance/resource ID or slot.
        std::uint32_t lifetimeId = 0;
        unsigned short identitySlot = 0xffffu;
        bool destroyRequested = false;
        int x;
        int y;
        bool active;
        bool gemHigh = false;
        downwell::ProjectileAnimation animation;
        downwell::MachineGunTrajectory trajectory;
        downwell::SoundPlayState sound{};
        downwell::InstanceHandle myFx{};
        downwell::MachineGunSourceMotion sourceMotion{};
        bool sourceOwned=false;
        bool hitIgnore=false;
    };
    using BulletLifecycleBinding=downwell::RecordLifecycleBinding<BulletState>;
    struct EnemyState {
        // Family-local lifetime key; never a recovered Windows instance ID.
        std::uint32_t lifetimeId = 0;
        // Stable ownership slot, independent of the compacted traversal index.
        unsigned short identitySlot = 0xffffu;
        int objectId;
        int spriteId;
        int maskId=-1; // Bat OBJT default; flying Step later copies sprite_index.
        int x;
        int y;
        int health;
        int reward;
        int facing;
        long preciseX;
        long preciseY;
        long speedX;
        long speedY;
        unsigned char gravityRemainder; // fifths of one Q8 velocity unit
        unsigned char gravityPositionRemainder; // fifth-Q8 integration residual
        downwell::FrogHorizontalMotion frogHorizontal;
        downwell::FrogHitState frogHit;
        downwell::FrogPresentation frogPresentation;
        downwell::CrawlerMotion crawler;
        downwell::SnailMotion snail;
        downwell::SnailHitState snailHit;
        downwell::BatHitState batHit;
        unsigned short batDirectionFraction;
        downwell::BatActivity batActivity;
        downwell::WalkingEnemyMotion walker;
        int motionState;
        int auxiliaryState;
        int alarm0;
        int alarm1;
        int direction;
        int moveSpeed;
        unsigned imagePhase;
        unsigned imageSpeed; // twentieths of a frame
        unsigned char imageRemainder; // fifths of one Q8 phase unit
        unsigned imageFrame;
        bool active;
        bool awake;
        bool grounded;
        bool frogLanded;
        bool activationPending;
        bool destroyRequested=false;
        int imageScaleY() const { return objectId == 117 ? snail.verticalFlip : 1; }
        unsigned drawFrame() const {
            if (objectId == 119)
                return imageFrame + (crawler.imageDirection % 4u) * 256u;
            // Source enmSnail Step updates image_yscale only after nonzero
            // movement. Creation/time-stop/zero speed retain the stored flip.
            // Frame 256+n is host-generated, for both planar and DMA assets.
            return imageFrame + (imageScaleY() < 0 ? 256u : 0u);
        }
    };
    struct GemState {
        std::uint32_t lifetimeId = 0;
        downwell::GemMotion motion;
        int x;
        int y;
        int amount;
        unsigned age;
        long preciseX;
        long preciseY;
        long speedX;
        long speedY;
        bool large;
        bool attracted;
        bool active;
    };

    using EnemyLifecycleBinding = downwell::RecordLifecycleBinding<EnemyState>;
    struct Breakable {
        int x;
        int y;
        bool active;
        std::uint32_t lifetimeId;
        bool destroyRequested=false;
        downwell::BreakableState state{};
        std::int32_t alarm0=-1;
        downwell::InstanceHandle myFx{}; // Internal lifetime, never a VM numeric ID.
        downwell::SoundPlayState sound{};
        // Native ownership chronology only; simulation identity remains
        // lifetimeId. Mutable because lifecycle observers receive const data.
        mutable std::uint32_t nativeRegistration=0;
        std::uint32_t sourceId=0; // VM numeric identity; never the native slot/order.
    };
    using BreakableLifecycleBinding = downwell::RecordLifecycleBinding<Breakable>;
    struct PlayerInstance {
        int x=240,y=176; // Published world coordinates, not motion accumulators.
        bool active=true;
        std::uint32_t lifetimeId=0;
        int scaleX=1;
        // Serialized room identity, independent of the reusable native slot.
        // Zero until the resource-backed room handoff reaches the player.
        std::uint32_t sourceInstanceId=0;
        std::uint32_t cameraSourceId=0;
        std::int32_t damageAlarm=-1; // Object 35 Alarm 1; other alarms are not admitted here.
    };
    using PlayerLifecycleBinding = downwell::RecordLifecycleBinding<PlayerInstance>;
    // When supplied, entryRooms is borrowed for this world and its run resets;
    // it must remain immutable and prepares rmMain's initial view cache before
    // player lifecycle callbacks. The terrain handoff reuses that same snapshot.
    // Fresh native rooms use authoredRoom so ownership can be attached before
    // Create and registration occurs in the serialized walk. Construction is
    // retained for already-entered standalone simulations and replay fixtures.
    enum class PlayerBirthTiming { construction, authoredRoom };
    explicit GameplayWorld(unsigned initialRingOrigin,
                              bool startsGrounded = true,
                              int landingOffset = 0,
                              bool followCamera = false,
                              const EnemyLifecycleBinding& enemyLifecycle = {},
                              const BreakableLifecycleBinding& breakableLifecycle = {},
                              const PlayerLifecycleBinding& playerLifecycle = {},
                              downwell::GameplayTerrainInstances* terrainInstances = nullptr,
                              const downwell::RoomPack* entryRooms = nullptr,
                              PlayerBirthTiming playerBirthTiming = PlayerBirthTiming::construction);
    bool awaitingPlayerBirth()const{return playerBirthPending_;}
    const downwell::GameplayTerrainInstances* terrainInstances()const{return terrainInstances_;}
    using NativeTerrain=downwell::NativeTerrainLifetimes<downwell::GameplayTerrainInstances::capacity>;
    const NativeTerrain* nativeTerrainInstances()const{return nativeTerrainInstances_;}
    bool bindNativeTerrain(NativeTerrain& terrain){
        if(!run_.effectsSucceeded||terrainInstances_||!terrain.valid()||
            (nativeTerrainInstances_&&nativeTerrainInstances_!=&terrain))return run_.effectsSucceeded=false;
        if(!terrain.bind(nativeTerrainBinding_))return run_.effectsSucceeded=false;
        nativeTerrainInstances_=&terrain;return true;
    }
    bool configureNativeTerrainBinding(NativeTerrain::Binding binding){
        if(!run_.effectsSucceeded||(nativeTerrainInstances_&&!nativeTerrainInstances_->bind(binding)))return run_.effectsSucceeded=false;
        nativeTerrainBinding_=binding;return true;
    }
    // Without generic terrain records, the backend publishes both its raster
    // and any bound native identity runs; do not execute generic Create twice.
    bool terrainCreatesBackendOnly()const{return run_.effectsSucceeded&&!terrainInstances_;}
    bool finishNativeTerrainCreate(downwell::InstanceHandle h){
        return run_.effectsSucceeded=run_.effectsSucceeded&&nativeTerrainInstances_&&
            nativeTerrainInstances_->createProperties(h,downwell::terrainTileForArea(run_.stage.area));
    }
    bool publishNativeTerrainIndexed(downwell::InstanceHandle h){
        return run_.effectsSucceeded=run_.effectsSucceeded&&nativeTerrainInstances_&&
            nativeTerrainInstances_->publishIndexed(h);
    }
    bool publishNativeTerrainQueryCache(downwell::InstanceHandle h,int x,int y,bool dirty){
        return run_.effectsSucceeded=run_.effectsSucceeded&&nativeTerrainInstances_&&
            nativeTerrainInstances_->publishCachedOffset(h,x,y)&&
            (!dirty||nativeTerrainInstances_->publishQueryDirty(h));
    }
    bool publishNativeTerrainBounds(downwell::InstanceHandle h,std::uint32_t flags){
        return run_.effectsSucceeded=run_.effectsSucceeded&&nativeTerrainInstances_&&
            nativeTerrainInstances_->publishRefreshedBounds(h,flags);
    }
    // Only a source-ordered scheduler-selected wall may enter here. Ops must
    // use the source spatial queries and publish renderer/collision changes;
    // this is not a separate terrain-only End Step pass.
    template<class Ops> bool stepNativeTerrainWall(downwell::InstanceHandle h,Ops& ops){
        return stepNativeTerrainWall(h,run_.stage.area,enemyRandom_,ops);
    }
    template<class Ops> bool stepNativeTerrainWall(downwell::InstanceHandle h,int area,downwell::GameMakerRandom& random,Ops& ops){
        return run_.effectsSucceeded=run_.effectsSucceeded&&nativeTerrainInstances_&&
            nativeTerrainInstances_->wallEvent(h,area,random,ops);
    }
    bool destroyNativeTerrain(downwell::InstanceHandle h){
        return run_.effectsSucceeded=run_.effectsSucceeded&&nativeTerrainInstances_&&nativeTerrainInstances_->markDestroyed(h);
    }
    bool activateNativeTerrain(downwell::InstanceHandle h,bool active){
        return run_.effectsSucceeded=run_.effectsSucceeded&&nativeTerrainInstances_&&nativeTerrainInstances_->activation(h,active);
    }
    bool retireNativeTerrain(downwell::InstanceHandle h){
        return run_.effectsSucceeded=run_.effectsSucceeded&&nativeTerrainInstances_&&nativeTerrainInstances_->retire(h);
    }
    bool rebindNativeTerrainOrder(downwell::InstanceHandle h,std::uint32_t order){
        return run_.effectsSucceeded=run_.effectsSucceeded&&nativeTerrainInstances_&&nativeTerrainInstances_->rebindOrder(h,order);
    }
    const downwell::GameplayScriptSpawnInstances* scriptSpawnInstances()const{return scriptSpawnInstances_;}
    using RegisteredExpiry=downwell::CorpseExpiryInstances<32>;
    using LevelNumbers=downwell::LevelNumberInstances<4>;
    const LevelNumbers* levelNumbers()const{return levelNumbers_;}
    bool bindLevelNumbers(LevelNumbers& instances){
        if(levelNumbers_||!instances.valid()||instances.size())return false;
        levelNumbers_=&instances;return true;
    }
    using ConsumptionParticles=downwell::ConsumptionBloodInstances<32>;
    using LimboShards=downwell::LimboShardInstances<32>;
    using RegisteredFlash=downwell::FlashBallInstances<32>;
    const RegisteredFlash* registeredFlash()const{return registeredFlash_;}
    bool bindRegisteredFlash(RegisteredFlash& instances){
        if(registeredFlash_||!instances.valid()||instances.size())return false;
        registeredFlash_=&instances;return true;
    }
    template<class Allocate> bool emitBlastContactFlash(downwell::SmokeBurstCaller& caller,
        downwell::MotionBinary64 x,downwell::MotionBinary64 y,Allocate allocate){
        if(!run_.effectsSucceeded)return false;
        return run_.effectsSucceeded=registeredFlash_&&downwell::flashBurstPublished<1,0,0>(
            caller,x,y,enemyRandom_,[&](int object,auto px,auto py){
                unsigned id=0;downwell::InstanceHandle child;
                return object==224&&allocate(id)&&registeredFlash_->create(id,px,py,enemyRandom_,child);
            });
    }
    template<class Allocate> bool emitSkeletonFlash(downwell::SkeletonActor& actor,Allocate allocate){
        if(!run_.effectsSucceeded)return false;
        return run_.effectsSucceeded=registeredFlash_&&downwell::flashBurstPublished<6,2,10>(
            actor.smokeScript,actor.xx,actor.yy,enemyRandom_,[&](int object,auto x,auto y){
                unsigned id=0;downwell::InstanceHandle child;
                return object==224&&allocate(id)&&registeredFlash_->create(id,x,y,enemyRandom_,child);
            });
    }
    const LimboShards* limboShards()const{return limboShards_;}
    bool bindLimboShards(LimboShards& instances){
        if(limboShards_||!instances.valid()||instances.size())return false;
        limboShards_=&instances;return true;
    }
    bool createLimboShardWithSourceId(std::uint32_t id,downwell::MotionBinary64 x,downwell::MotionBinary64 y,
        bool lowSpec,downwell::InstanceHandle& child){
        return run_.effectsSucceeded=run_.effectsSucceeded&&limboShards_&&
            limboShards_->create(id,x,y,lowSpec,enemyRandom_,child);
    }
    template<class Queries> bool stepLimboShard(downwell::InstanceHandle key,bool allowed,
        const downwell::LimboShard::Player& player,Queries& queries){
        return run_.effectsSucceeded=run_.effectsSucceeded&&limboShards_&&
            limboShards_->step(key,allowed,player,enemyRandom_,queries);
    }
    bool collideLimboShard(downwell::InstanceHandle key,downwell::MotionBinary64 xsp,downwell::MotionBinary64 ysp){
        return run_.effectsSucceeded=run_.effectsSucceeded&&limboShards_&&
            limboShards_->bulletContact(key,xsp,ysp,enemyRandom_);
    }
    const ConsumptionParticles* consumptionParticles()const{return consumptionParticles_;}
    bool bindConsumptionParticles(ConsumptionParticles& instances){
        if(consumptionParticles_||!instances.valid()||instances.size())return false;
        consumptionParticles_=&instances;return true;
    }
    using RegisteredCorpse=downwell::CorpseInstances<32>;
    const RegisteredCorpse* registeredCorpses()const{return registeredCorpses_;}
    template<class Publish,class Allocate,class Tile> bool emitSkeletonCorpse(Publish& publish,
        downwell::MotionBinary64 impact,Allocate allocate,Tile tile){
        if(!run_.effectsSucceeded||!registeredCorpses_)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=publish.deadBody([&](auto& source){
            return registeredCorpses_->createFromDeathAllocated(source,206,impact,102,
                run_.upgrades.levels[3]!=0,enemyRandom_,tile,allocate);
        });
    }
    bool bindRegisteredCorpses(RegisteredCorpse& instances){
        if(registeredCorpses_||!instances.valid()||instances.size())return false;
        registeredCorpses_=&instances;return true;
    }
    using RegisteredBlasts=downwell::BlastInstances<8>;
    const RegisteredBlasts* registeredBlasts()const{return registeredBlasts_;}
    bool bindRegisteredBlasts(RegisteredBlasts& instances){
        if(registeredBlasts_||!instances.valid()||instances.size())return false;
        registeredBlasts_=&instances;return true;
    }
    template<class Delay> bool createBlastWithSourceId(unsigned id,downwell::MotionBinary64 x,
        downwell::MotionBinary64 y,Delay delay,downwell::InstanceHandle& key){
        if(!run_.effectsSucceeded||!registeredBlasts_)return run_.effectsSucceeded=false;
        struct Services {
            GameplayWorld& world;Delay& delay;
            bool shake(int a,int d){return world.requestCameraShake(a,d);}
            bool momentDelay(){return delay();}
            bool choose(unsigned count,unsigned& choice){
                if(count!=2)return false;choice=world.enemyRandom_.chooseIndexSmall(2);return true;
            }
        } services{*this,delay};
        return run_.effectsSucceeded=registeredBlasts_->create(id,x,y,services,key);
    }
    using RegisteredSmoke=downwell::SmokeInstances<32>;
    const RegisteredSmoke* registeredSmoke()const{return registeredSmoke_;}
    bool bindRegisteredSmoke(RegisteredSmoke& instances){
        if(registeredSmoke_||!instances.valid()||instances.size())return false;
        registeredSmoke_=&instances;return true;
    }
    template<class Allocate> bool emitSkeletonSmoke(downwell::SkeletonActor& actor,Allocate allocate){
        if(!run_.effectsSucceeded||!registeredSmoke_)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=downwell::smokeBurstPublished<5,1>(actor.smokeScript,actor.xx,actor.yy,
            enemyRandom_,[&](int object,auto x,auto y){
                unsigned id=0;downwell::InstanceHandle child;
                return object==225&&allocate(id)&&id&&registeredSmoke_->create(id,x,y,enemyRandom_,child);
            });
    }
    template<class Allocate> bool emitRegisteredSmoke(downwell::EmitSmokeScript& caller,
        downwell::MotionBinary64 x,downwell::MotionBinary64 y,downwell::MotionBinary64 direction,
        downwell::MotionBinary64 speed,Allocate allocate){
        if(!run_.effectsSucceeded||!registeredSmoke_)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=caller.execute(x,y,direction,speed,
            [&](auto px,auto py,std::uint32_t& id){downwell::InstanceHandle child;unsigned allocated=0;
                if(!allocate(allocated)||!allocated||!registeredSmoke_->create(allocated,px,py,enemyRandom_,child))return false;
                id=allocated;return true;},
            [&](unsigned id,auto value){return registeredSmoke_->setDirection(registeredSmoke_->sourceHandle(id),value);},
            [&](unsigned id,auto value){return registeredSmoke_->setSpeed(registeredSmoke_->sourceHandle(id),value);});
    }
    bool stepSmokeRoomEffect(downwell::InstanceHandle key){
        return run_.effectsSucceeded=run_.effectsSucceeded&&registeredSmoke_&&registeredSmoke_->step(key,enemyRandom_);
    }
    const RegisteredExpiry* registeredExpiry()const{return registeredExpiry_;}
    bool bindRegisteredExpiry(RegisteredExpiry& instances){
        if(registeredExpiry_||!instances.valid()||instances.size())return false;
        registeredExpiry_=&instances;return true;
    }
    const downwell::GameplayDebrisInstances* debrisInstances()const{return debrisInstances_;}
    const downwell::NativeTimeChangeRegistration* timeChangeInstances()const{return timeChangeInstances_;}
    const downwell::NativeTimeShardRegistration* timeShardInstances()const{return timeShardInstances_;}
    const downwell::GameplayEntryDithers* entryDithers()const{return entryDithers_;}
    const downwell::GameplayDitherAbove* ditherAbove()const{return ditherAbove_;}
    const downwell::GameplayBuilderController* builderController()const{return builderController_;}
    const downwell::GameplayPlayerControllers* playerControllers()const{return playerControllers_;}
    using RunControllers=downwell::RunControllerInstances<4>;
    const downwell::GameplayResultInstance* resultInstance()const{return resultInstance_;}
    bool bindResultInstance(downwell::GameplayResultInstance& instance){
        if(resultInstance_||!instance.valid()||instance.handle().generation)return false;
        resultInstance_=&instance;return true;
    }
    const RunControllers* runControllers()const{return runControllers_;}
    bool bindRunControllers(RunControllers& records){
        if(runControllers_||!records.valid()||records.size())return false;
        runControllers_=&records;return true;
    }
    bool bindControllerRoomSpeed(int& speed){
        if(!run_.effectsSucceeded||(timeChangeSpeed_&&timeChangeSpeed_!=&speed)||
            (controllerRoomSpeed_&&controllerRoomSpeed_!=&speed))return run_.effectsSucceeded=false;
        controllerRoomSpeed_=&speed;return true;
    }
    template<class Visit> bool requestMomentDelay(Visit controllers){
        if(!run_.effectsSucceeded)return false;
        return run_.effectsSucceeded=runControllers_&&controllerRoomSpeed_&&runControllers_->momentDelay(*controllerRoomSpeed_,controllers);
    }
    template<class Visit,class Apply> bool visitDamageControllers(Visit controllers,Apply apply){
        if(!run_.effectsSucceeded)return false;
        return run_.effectsSucceeded=runControllers_&&runControllers_->visitDamageControllers(controllers,apply);
    }
    void configurePlayerDamageSlowdown(bool (*apply)(void*),void* context){
        playerDamageSlowdown_=apply;playerDamageSlowdownContext_=context;
    }
    bool applyPlayerDamageSlowdown(){
        return run_.effectsSucceeded=run_.effectsSucceeded&&playerDamageSlowdown_&&
            playerDamageSlowdown_(playerDamageSlowdownContext_);
    }
    template<class Visit> bool applyPlayerDamageSlowdown(Visit controllers){
        return visitDamageControllers(controllers,[&](auto& controller){
            if(!controllerRoomSpeed_)return false;
            *controllerRoomSpeed_=40;
            controller.setSlow(23);return controller.setAlarm(0,40);
        });
    }
    bool advanceControllerRoomAlarm(downwell::InstanceHandle key,unsigned alarm){
        return run_.effectsSucceeded=run_.effectsSucceeded&&runControllers_&&controllerRoomSpeed_&&
            runControllers_->advanceAlarm(key,alarm,*controllerRoomSpeed_);
    }
    bool bindPlayerControllers(downwell::GameplayPlayerControllers& records){
        if(playerControllers_||!records.valid()||records.size())return false;
        playerControllers_=&records;return true;
    }
    // Explicit rmMain player Create composition. The enclosing source-ordered
    // allocator must provide each ID; this cannot infer IDs from typed handles.
    template<class Allocate> bool createPlayerChildren(Allocate allocate){
        if(!run_.effectsSucceeded)return false;
        downwell::PackedRoom room;
        const auto* view=roomEntryViews_.get(0);
        if(playerChildrenStarted_||!playerInstance_.lifetimeId||!playerInstance_.sourceInstanceId||
            !playerControllers_||playerControllers_->size()||!entryDithers_||entryDithers_->size()||
            !entryRooms_||!entryRooms_->findRoom(11,room)||!view)return run_.effectsSucceeded=false;
        playerChildrenStarted_=true;
        downwell::RunnerImageValue px,py;
        if(!px.assignInteger(playerInstance_.x)||!py.assignInteger(playerInstance_.y))return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=downwell::PlayerChildCreation::run(11,playerInstance_.x,playerInstance_.y,
            run_.upgrades.levels[15],run_.upgrades.levels[9],
            [&](int object,int x,int y)->std::uint32_t{
                unsigned id=0;downwell::InstanceHandle child;downwell::RunnerImageValue cx,cy;
                if(!allocate(object,id)||!id||!cx.assignInteger(x)||!cy.assignInteger(y))return 0;
                const bool ok=object==64?playerControllers_->createCamera(id,cx,cy,room.width,false,px,py,child):
                    object==66?playerControllers_->create(object,id,cx,cy,child):
                    playerControllers_->createCompanion(object,id,cx,cy,child);
                return ok?id:0;
            },[&](unsigned value){droneNumber_=value;return true;},
            [&](unsigned id){return playerControllers_->reverseDrone(playerControllers_->sourceHandle(id));},
            [&](unsigned id,bool freeCam,bool clearAutoScroll){
                playerInstance_.cameraSourceId=id;
                return playerControllers_->cameraModes(playerControllers_->sourceHandle(id),freeCam,clearAutoScroll);
            },[&](int){
                unsigned id=0;downwell::InstanceHandle child;
                if(!allocate(73,id)||!id)return false;
                return entryDithers_->create(id,[&](downwell::MotionBinary64& x){
                    const auto* camera=playerControllers_->find(playerControllers_->sourceHandle(playerInstance_.cameraSourceId));
                    if(!camera)return false;
                    x=camera->x.promoted();return true;
                },[&](downwell::MotionBinary64& y){y=view->y.promoted();return true;},child);
            });
    }
    unsigned droneNumber()const{return droneNumber_;}
    // Borrow the enclosing source-ID stream only for this room load. Children
    // execute synchronously at the authored player birth, not after the room.
    template<class Allocate,class Load> bool withPlayerChildCreation(Allocate allocate,Load load){
        if(!run_.effectsSucceeded||!playerBirthPending_||roomTerrainLoaded_||playerChildCreate_)
            return run_.effectsSucceeded=false;
        playerChildCreateContext_=&allocate;
        playerChildCreate_=[](void* context,GameplayWorld& world){
            return world.createPlayerChildren(*static_cast<Allocate*>(context));
        };
        const bool ok=load();
        playerChildCreate_=nullptr;playerChildCreateContext_=nullptr;
        return run_.effectsSucceeded=ok&&run_.effectsSucceeded&&playerChildrenStarted_&&!playerBirthPending_;
    }
    bool bindBuilderController(downwell::GameplayBuilderController& actor){
        if(builderController_||!actor.valid()||actor.handle().generation)return false;
        builderController_=&actor;return true;
    }
    bool publishBuilderPosition(int x,int y){
        if(!run_.effectsSucceeded)return false;
        // Unindexed generator fixtures have no room/controller ownership.
        return run_.effectsSucceeded=!builderController_||builderController_->position(x,y);
    }
    void configureDitherAboveEnvironment(bool lowSpec,bool ending){ditherLowSpec_=lowSpec;ditherEnding_=ending;}
    bool adoptDitherAbove(const downwell::GameplayDitherAbove::Snapshot& snapshot){
        if(!run_.effectsSucceeded||!ditherAbove_||!ditherAbove_->adopt(snapshot))return run_.effectsSucceeded=false;
        configureDitherAboveEnvironment(snapshot.lowSpec,snapshot.ending);return true;
    }
    bool bindDitherAbove(downwell::GameplayDitherAbove& actor){
        if(ditherAbove_||!actor.valid()||actor.handle().generation)return false;
        ditherAbove_=&actor;return true;
    }
    // Explicit authored-room admission until the complete controller Create,
    // Draw and collision stream is connected; never allocate a synthetic ID.
    bool createAuthoredDitherAbove(){
        if(!run_.effectsSucceeded||!entryRooms_||!ditherAbove_)return run_.effectsSucceeded=false;
        downwell::PackedRoom room;downwell::PackedRoomInstance source{};bool found=false;
        if(!entryRooms_->findRoom(11,room))return run_.effectsSucceeded=false;
        for(unsigned i=0;i<room.instanceCount;++i){
            downwell::PackedRoomInstance r;
            if(!entryRooms_->roomInstance(room,i,r))return run_.effectsSucceeded=false;
            if(r.objectId!=61)continue;
            if(found)return run_.effectsSucceeded=false;
            found=true;source=r;
        }
        return run_.effectsSucceeded=found&&ditherAbove_->create(source.instanceId,source.x,source.y);
    }
    using DroneBullets=downwell::DroneBulletInstances<32>;
    static constexpr unsigned droneBulletCapacity=32;
    const DroneBullets* droneBullets()const{return droneBullets_;}
    bool bindDroneBullets(DroneBullets& bullets){
        if(droneBullets_||!bullets.valid()||bullets.size())return false;
        droneBullets_=&bullets;return true;
    }
    using Blood=downwell::BloodInstances<32>;
    using Currency=downwell::CurrencyInstances<128>;
    static constexpr unsigned currencyCapacity=128;
    const Currency* currencyInstances()const{return currencyInstances_;}
    bool bindCurrency(Currency& currency){
        if(currencyInstances_||!currency.valid()||currency.size())return false;
        currencyInstances_=&currency;return true;
    }
    template<class Queries> bool createCurrencyWithSourceId(unsigned id,downwell::MotionBinary64 x,
        downwell::MotionBinary64 y,Queries& queries,downwell::InstanceHandle& key){
        if(!run_.effectsSucceeded||!currencyInstances_)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=currencyInstances_->create(id,x,y,run_.stage.area,run_.upgrades.levels[10]!=0,enemyRandom_,queries,key);
    }
    // The room stream must allocate numeric IDs; never synthesize one from a
    // currency pool slot. Caller accessors retain their own generation checks
    // and must read published coordinates, not private movement caches.
    template<class Read,class WriteAmount,class WriteAttracted,class WriteChild,class Allocate,class Queries>
    bool spawnRegisteredCurrency(const downwell::ObjectPack& objects,Read read,WriteAmount writeAmount,
        WriteAttracted writeAttracted,WriteChild writeChild,downwell::MotionBinary64 amount,
        Allocate allocate,Queries& queries){
        if(!run_.effectsSucceeded||!currencyInstances_||!currencyInstances_->valid())return run_.effectsSucceeded=false;
        struct Ops {
            using Handle=std::uint32_t;
            GameplayWorld& world;const downwell::ObjectPack& objects;Allocate& allocate;Queries& queries;
            bool parent(int object,int& value){
                downwell::PackedObject info;if(!objects.findObject(object,info))return false;
                value=info.parentId;return true;
            }
            bool create(int object,downwell::MotionBinary64 x,downwell::MotionBinary64 y,Handle& id){
                Handle next=0;downwell::InstanceHandle key;
                if(object!=248||!allocate(next)||!world.createCurrencyWithSourceId(next,x,y,queries,key))return false;
                const auto* r=world.currencyInstances_->find(key);if(!r)return false;
                id=r->sourceId;return true;
            }
            bool amount(Handle id,int value){return world.currencyInstances_->dispatch(
                world.currencyInstances_->handleForSourceId(id),[&](auto& s,bool&){s.getAmount=value;return true;});}
            bool size(Handle id,int value){return world.currencyInstances_->dispatch(
                world.currencyInstances_->handleForSourceId(id),[&](auto& s,bool&){s.size=value;return true;});}
            bool attracted(Handle id,bool value){return world.currencyInstances_->dispatch(
                world.currencyInstances_->handleForSourceId(id),[&](auto& s,bool&){s.attracted=value;return true;});}
        } ops{*this,objects,allocate,queries};
        return downwell::CurrencySpawnScript::runRetained(read,writeAmount,writeAttracted,writeChild,amount,ops,run_.effectsSucceeded);
    }
    static constexpr unsigned bloodCapacity=32;
    template<class Allocate,class Queries> bool emitSkeletonCurrency(const downwell::ObjectPack& objects,
        downwell::SkeletonActor& actor,Allocate allocate,Queries& queries){
        return spawnRegisteredCurrency(objects,[&](auto& out){return actor.readCurrencyReward(out);},
            [&](downwell::MotionBinary64 value){actor.reward.moniAmount=value;return true;},
            [&](bool value){actor.reward.atrState=value;return true;},
            [&](std::uint32_t value){actor.reward.myGem=value;return true;},
            downwell::MotionBinary64::fromInteger(actor.money),allocate,queries);
    }
    bool currencyWallMeeting(downwell::InstanceHandle key,downwell::MotionBinary64 x,
        downwell::MotionBinary64 y,bool& hit){
        const auto* r=currencyInstances_?currencyInstances_->find(key):nullptr;
        downwell::ContactBounds bounds;
        if(!run_.effectsSucceeded||!r||!wallContactResolver_||
            !downwell::CurrencyCollisionBounds::calculate(r->maskId<0?r->spriteId:r->maskId,x,y,bounds))
            return run_.effectsSucceeded=false;
        unsigned count=0;
        if(!wallContactResolver_(solidResolverContext_,bounds,count))return run_.effectsSucceeded=false;
        bool found=count!=0;
        if(!run_.shops.visitWallContacts(bounds,[&](){found=true;return true;}))return run_.effectsSucceeded=false;
        hit=found;return true;
    }
    bool currencyPlatformMeeting(downwell::InstanceHandle key,downwell::MotionBinary64 x,
        downwell::MotionBinary64 y,bool& hit){
        const auto* r=currencyInstances_?currencyInstances_->find(key):nullptr;
        downwell::ContactBounds bounds;bool found=false;
        if(!run_.effectsSucceeded||!r||!platformContactResolver_||
            !downwell::CurrencyCollisionBounds::calculate(r->maskId<0?r->spriteId:r->maskId,x,y,bounds)||
            !platformContactResolver_(solidResolverContext_,bounds,found))return run_.effectsSucceeded=false;
        hit=found;return true;
    }
    bool nativeCurrencyQueriesReady()const{
        return run_.effectsSucceeded&&currencyOnlyPlatforms_&&currencyEmptyWater_&&wallContactResolver_&&platformContactResolver_;
    }
    bool currencyWaterAbsent()const{return run_.effectsSucceeded&&currencyEmptyWater_;}
    enum class CurrencyPhase { alarm, step };
    using CurrencyStepOrder=downwell::RunnerStepSnapshot<currencyCapacity>;
    void configureCurrencyStepOrder(bool (*capture)(void*,CurrencyStepOrder&),void* context,
        bool (*eligible)(void*,downwell::InstanceHandle,bool&)){
        currencyOrderCapture_=capture;currencyOrderContext_=context;currencyStepEligible_=eligible;
    }
    template<class Execute,class Tick> bool withCurrencyEvents(Execute execute,Tick tick){
        if(!run_.effectsSucceeded||currencyEventDispatch_)return run_.effectsSucceeded=false;
        currencyEventContext_=&execute;
        currencyEventDispatch_=[](void* p,downwell::InstanceHandle key,CurrencyPhase phase,unsigned alarm){
            return (*static_cast<Execute*>(p))(key,phase,alarm);
        };
        const bool ok=tick();currencyEventDispatch_=nullptr;currencyEventContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    bool advanceCurrencyRoomAlarm(downwell::InstanceHandle key,unsigned alarm){
        if(!run_.effectsSucceeded||!currencyInstances_||!currencyEventDispatch_)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=currencyInstances_->advanceAlarm(key,alarm,[&](auto&,unsigned index,bool&){
            return currencyEventDispatch_(currencyEventContext_,key,CurrencyPhase::alarm,index);
        });
    }
    template<class Ops> bool collectRegisteredCurrency(downwell::InstanceHandle key,Ops& ops){
        if(!run_.effectsSucceeded||!currencyInstances_)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=currencyInstances_->collect(key,enemyRandom_,ops);
    }
    template<class Audio> bool playCurrencyPickupSound(downwell::InstanceHandle key,downwell::MotionBinary64 volume,Audio& audio){
        if(!run_.effectsSucceeded||!currencyInstances_)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=currencyInstances_->dispatchSound(key,[&](auto& sound){
            return run_.playCurrencyPickupSound(sound,volume,audio);
        });
    }
    template<class Queries,class Math,class View,class Audio>
    bool stepCurrencyRoomEffect(downwell::InstanceHandle key,bool stopped,int width,
        downwell::MotionBinary64 playerX,downwell::MotionBinary64 playerY,Queries& queries,Math& math,View view,
        Audio& audio,downwell::MotionBinary64 volume,bool inWater){
        if(!run_.effectsSucceeded||!currencyInstances_)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=currencyInstances_->stepSource(key,enemyRandom_,stopped,width,
            playerX,playerY,queries,math,view,audio,volume,inWater);
    }
    const Blood* bloodInstances()const{return blood_;}
    bool bindBlood(Blood& blood){
        if(blood_||!blood.valid()||blood.size())return false;
        blood_=&blood;return true;
    }
    template<class Caller> bool createBloodWithSourceId(unsigned id,downwell::MotionBinary64 x,
        downwell::MotionBinary64 y,downwell::MotionBinary64 angle,Caller caller,downwell::InstanceHandle& key){
        if(!run_.effectsSucceeded||!blood_||!blood_->create(id,x,y,angle,caller,key))return run_.effectsSucceeded=false;
        return true;
    }
    template<class Allocate> bool emitSkeletonBlood(downwell::SkeletonActor& actor,Allocate allocate){
        if(!run_.effectsSucceeded||!blood_)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=downwell::bloodBurst(downwell::MotionBinary64::fromInteger(actor.x),
            downwell::MotionBinary64::fromInteger(actor.y),enemyRandom_,[&](auto x,auto y,auto angle){
                unsigned id=0;downwell::InstanceHandle child;
                if(!allocate(id)||!id)return false;
                return createBloodWithSourceId(id,x,y,angle,[&](downwell::InstanceHandle key){
                    const auto* record=blood_->find(key);if(!record)return false;
                    actor.myFx=record->sourceId;return true;
                },child);
            });
    }
    template<class Query> bool stepBloodRoomEffect(downwell::InstanceHandle key,Query query){
        if(!run_.effectsSucceeded||!blood_||!blood_->step(key,query))return run_.effectsSucceeded=false;
        return true;
    }
    using Bones=downwell::BoneProjectileInstances<32>;
    using Skeletons=downwell::SkeletonInstances<32>;
    static constexpr unsigned skeletonCapacity=32;
    const Skeletons* skeletons()const{return skeletons_;}
    bool bindSkeletons(Skeletons& skeletons){
        if(skeletons_||!skeletons.valid()||skeletons.size())return false;
        skeletons_=&skeletons;return true;
    }
    bool createSkeletonWithSourceId(unsigned id,int x,int y,downwell::InstanceHandle& key){
        if(!run_.effectsSucceeded||!skeletons_||!skeletons_->create(id,x,y,key))return run_.effectsSucceeded=false;
        return true;
    }
    template<unsigned Capacity,class MakeEffects> bool stepSkeletonRoomEffect(downwell::InstanceHandle key,
        const downwell::TimeStopEffects<Capacity>& stop,MakeEffects makeEffects){
        downwell::MotionBinary64 x,y;
        if(!stop.succeeded||!downwell::ContactCoordinates::playerGlobals(player_.frameStartXFixed(),
            player_.frameStartYFixed(),240,176,x,y))return run_.effectsSucceeded=false;
        return stepSkeletonRoomEffect(key,stop.player.stopped,x,y,makeEffects);
    }
    template<class MakeEffects> bool stepSkeletonRoomEffect(downwell::InstanceHandle key,bool stopped,
        downwell::MotionBinary64 playerX,downwell::MotionBinary64 playerY,MakeEffects makeEffects){
        return stepSkeletonRoomEffectPublished(key,stopped,playerX,playerY,
            [&](auto& actor,auto& throwing,auto&){return makeEffects(actor,throwing);});
    }
    template<class MakeEffects> bool stepSkeletonRoomEffectPublished(downwell::InstanceHandle key,bool stopped,
        downwell::MotionBinary64 playerX,downwell::MotionBinary64 playerY,MakeEffects makeEffects){
        if(!run_.effectsSucceeded||!skeletons_||!skeletons_->valid())return run_.effectsSucceeded=false;
        const bool ok=skeletons_->dispatchPublished(key,[&](auto& actor,auto& throwing,auto& publish,bool& good){
            auto effects=makeEffects(actor,throwing,publish);
            return actor.stepPublished(stopped,playerX,playerY,publish,effects,good);
        });
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    template<class MakePlayer> bool collideSkeletonRoomEffect(downwell::InstanceHandle key,bool timeField,MakePlayer makePlayer){
        if(!run_.effectsSucceeded||!skeletons_||!skeletons_->valid())return run_.effectsSucceeded=false;
        const bool ok=skeletons_->dispatch(key,[&](auto& actor,auto&,bool& good){
            if(timeField){actor.collideTimeField();return true;}
            auto player=makePlayer(actor);return actor.collidePlayerPublished(player,good);
        });
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    static constexpr unsigned boneCapacity=32;
    const Bones* bones()const{return bones_;}
    bool bindBones(Bones& bones){
        if(bones_||!bones.valid()||bones.size())return false;
        bones_=&bones;return true;
    }
    // Until the native room allocator includes all controller/effect births,
    // require an explicit source ID rather than inventing it from a handle.
    bool createBoneWithSourceId(unsigned id,int x,int y,downwell::InstanceHandle& key){
        if(!run_.effectsSucceeded||!bones_||!bones_->create(id,x,y,enemyRandom_,key))return run_.effectsSucceeded=false;
        return true;
    }
    template<class Sound> bool throwRegisteredBoneWithSourceId(unsigned id,
        downwell::RegisteredSkeletonThrow& caller,int x,int y,bool separated,int facing,bool& shot,Sound sound){
        if(!bones_||!bones_->valid())return run_.effectsSucceeded=false;
        return caller.run(x,y,separated,facing,shot,run_.effectsSucceeded,
            [&](int px,int py,downwell::InstanceHandle& key){
                if(!createBoneWithSourceId(id,px,py,key))return false;
                caller.myBoneSourceId=id;return true;
            },
            [&](downwell::InstanceHandle key,auto event){return bones_->dispatch(key,event);},enemyRandom_,sound);
    }
    template<class Audio> bool throwSkeletonBoneWithSourceId(unsigned id,downwell::SkeletonActor& actor,
        downwell::RegisteredSkeletonThrow& caller,downwell::SoundPlayEnvironment& environment,Audio& audio){
        return throwSkeletonBoneAllocated(actor,caller,[id](std::uint32_t& source){source=id;return true;},environment,audio);
    }
    template<class Allocate,class Audio> bool throwSkeletonBoneAllocated(downwell::SkeletonActor& actor,
        downwell::RegisteredSkeletonThrow& caller,Allocate allocate,downwell::SoundPlayEnvironment& environment,Audio& audio){
        if(!bones_||!bones_->valid())return run_.effectsSucceeded=false;
        return caller.runAimed(actor.x,actor.y,actor.shot,run_.effectsSucceeded,
            [&](int x,int y,downwell::InstanceHandle& key){
                std::uint32_t id=0;
                if(!allocate(id)||!id||id>0x7fffffffu||!createBoneWithSourceId(id,x,y,key))return false;
                caller.myBoneSourceId=id;return true;
            },[&](downwell::InstanceHandle key,auto event){return bones_->dispatch(key,event);},enemyRandom_,
            [&](int sound){return playSkeletonSound(actor,sound,environment,audio);},
            [&](bool& separated,int& facing){
                downwell::MotionBinary64 px,py;
                if(!downwell::ContactCoordinates::playerGlobals(player_.frameStartXFixed(),player_.frameStartYFixed(),240,176,px,py))return false;
                const auto x=downwell::MotionBinary64::fromInteger(actor.x);
                separated=x.high!=px.high||x.low!=px.low;facing=actor.facing;return true;
            });
    }
    bool bindEntryDithers(downwell::GameplayEntryDithers& entries){
        if(entryDithers_||!entries.valid()||entries.size())return false;
        entryDithers_=&entries;return true;
    }
    bool bindTimeChangeInstances(downwell::NativeTimeChangeRegistration& registry);
    bool bindTimeShardInstances(downwell::NativeTimeShardRegistration& registry);
    bool createTimeShard(downwell::TimeShardEffects<64>& effects,downwell::MotionBinary64 x,
                         downwell::MotionBinary64 y,bool lowSpec);
    // Borrow the same banks for synchronous transition Create and subsequent
    // room animation/alarms/cleanup. They must outlive their registered children.
    template<class Side,class Field,class Lookup,class Audio,class Emit>
    bool beginTimeStop(downwell::TimeStopEffects<64>& state,
        const downwell::TimeStopEffects<64>::PlayerPose& pose,bool mainRoom,bool lowSpec,
        Side side,Field field,Lookup lookup,Audio audio,Emit emit){
        if(!run_.effectsSucceeded)return false;
        if(!state.succeeded||!state.shards.succeeded()||!state.changes.succeeded()||
           !timeShardInstances_||!timeChangeInstances_||
           !state.shards.phasesBoundTo(*timeShardInstances_)||!state.changes.boundTo(*timeChangeInstances_)||
           (timeShardInstances_->size()&&timeShardEffects_!=&state.shards)||
           (timeChangeInstances_->size()&&(timeChangeEffects_!=&state.changes||timeChangeSpeed_!=&state.roomSpeed))||
           (controllerRoomSpeed_&&controllerRoomSpeed_!=&state.roomSpeed))return run_.effectsSucceeded=false;
        timeShardEffects_=&state.shards;timeChangeEffects_=&state.changes;timeChangeSpeed_=&state.roomSpeed;
        return run_.effectsSucceeded=state.beginStepWithAudioChecked(mainRoom,side,pose,lowSpec,enemyRandom_,
            field,lookup,audio,[&](downwell::PlayerTimeStop::Effect effect){
                if(effect==downwell::PlayerTimeStop::Effect::shake&&!requestTimeStopCameraShake())return false;
                if(effect==downwell::PlayerTimeStop::Effect::oxygenFloor&&!run_.floorTimeStopOxygen())return false;
                return emit(effect);
            })&&run_.effectsSucceeded;
    }
    bool advanceTimeShardRoomAlarm(downwell::InstanceHandle key,unsigned index);
    bool advanceTimeShardRoomAnimation(downwell::InstanceHandle key);
    bool retireTimeShardRoomEffect(downwell::InstanceHandle key);
    bool collideTimeShardRoomEffect(downwell::InstanceHandle key);
    bool timeShardOutsideView(downwell::RunnerImageValue publishedY,bool& retire);
    template<class Outside> bool stepTimeShardRoomEffect(downwell::InstanceHandle key,bool stopped,Outside outside){
        return run_.effectsSucceeded=run_.effectsSucceeded&&timeShardInstances_&&timeShardEffects_&&
            timeShardInstances_->dispatchStep(key,*timeShardEffects_,stopped,enemyRandom_,outside);
    }
    template<class Pause> bool createTimeChange(downwell::TimeChangeEffects<8>& effects,
        downwell::MotionBinary64 x,downwell::MotionBinary64 y,int& speed,Pause pause){
        if(!run_.effectsSucceeded)return false;
        if(!timeChangeInstances_||!effects.boundTo(*timeChangeInstances_)||
           (controllerRoomSpeed_&&controllerRoomSpeed_!=&speed)||
           (timeChangeInstances_->size()&&(timeChangeEffects_!=&effects||timeChangeSpeed_!=&speed)))
            return run_.effectsSucceeded=false;
        timeChangeEffects_=&effects;timeChangeSpeed_=&speed;
        return run_.effectsSucceeded=effects.createAt(x,y,speed,pause);
    }
    bool advanceTimeChangeRoomAlarm(downwell::InstanceHandle key,unsigned index);
    bool retireTimeChangeRoomEffect(downwell::InstanceHandle key);
    const downwell::DebrisEffects<downwell::debrisEffectCapacity>& debrisEffects()const{return ownedDebrisEffects_;}
    bool spawnDebris(downwell::MotionBinary64 x,downwell::MotionBinary64 y,int area,downwell::InstanceHandle& result){
        return spawnDebris(ownedDebrisEffects_,x,y,area,result);
    }
    bool spawnDebrisWithSourceId(std::uint32_t id,downwell::MotionBinary64 x,downwell::MotionBinary64 y,
        int area,downwell::InstanceHandle& result){
        result={};if(!id||id>0x7fffffffu)return run_.effectsSucceeded=false;
        return spawnDebris(ownedDebrisEffects_,x,y,area,result,id);
    }
    bool bindDebrisInstances(downwell::GameplayDebrisInstances& instances){
        if(debrisInstances_||!instances.valid()||instances.size())return false;
        debrisInstances_=&instances;return true;
    }
    // Borrowed fixed storage must outlive the registered particles and retry.
    // Natural combat admission follows after source Step/Draw integration.
    template<unsigned Capacity>
    bool spawnDebris(downwell::DebrisEffects<Capacity>& effects,downwell::MotionBinary64 x,
        downwell::MotionBinary64 y,int area,downwell::InstanceHandle& result,std::uint32_t sourceId=0){
        if(!run_.effectsSucceeded)return false;
        if(!debrisInstances_||!roomAnimation_)return run_.effectsSucceeded=false;
        if(debrisBinding_.effects!=&effects){
            if(debrisInstances_->size())return run_.effectsSucceeded=false;
            debrisBinding_=downwell::ScriptEffectRuntimeBinding::bind<downwell::GameplayDebrisInstances::Record>(effects,*debrisInstances_);
            debrisAlarm_=[](void* p,downwell::GameplayDebrisInstances& registry,downwell::InstanceHandle key,unsigned index){
                auto& bank=*static_cast<downwell::DebrisEffects<Capacity>*>(p);
                const auto* record=registry.find(key);
                return record&&registry.dispatchAlarm(key,*record,bank,bank.beginPhase(),index);
            };
        }
        auto lifecycle=debrisInstances_->sourceBirth(sourceId);
        const auto h=sourceId?effects.createPublishedRegistered(x,y,area,enemyRandom_,lifecycle):
            effects.createRegistered(x,y,area,enemyRandom_,*debrisInstances_);
        if(!h)return run_.effectsSucceeded=false;
        result=debrisInstances_->handle(h);return run_.effectsSucceeded=result.generation!=0;
    }
    bool advanceDebrisRoomAnimation(downwell::InstanceHandle key){
        const auto* record=debrisInstances_?debrisInstances_->find(key):nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&record&&debrisBinding_.advance(key,record);
    }
    bool retireDebrisRoomEffect(downwell::InstanceHandle key){
        const auto* record=debrisInstances_?debrisInstances_->find(key):nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&record&&debrisBinding_.retire(key,record);
    }
    bool advanceOwnedDebrisRoomAlarm(downwell::InstanceHandle key,
        downwell::DebrisEffects<downwell::debrisEffectCapacity>::Phase phase,unsigned index){
        return advanceDebrisRoomAlarm(ownedDebrisEffects_,key,phase,index);
    }
    bool advanceBoundDebrisRoomAlarm(downwell::InstanceHandle key,unsigned index){
        return run_.effectsSucceeded=run_.effectsSucceeded&&debrisInstances_&&debrisAlarm_&&debrisBinding_.effects&&
            debrisAlarm_(debrisBinding_.effects,*debrisInstances_,key,index);
    }
    template<class Stopped,class Meeting>
    bool stepOwnedDebrisRoomEffect(downwell::InstanceHandle key,
        downwell::DebrisEffects<downwell::debrisEffectCapacity>::Phase phase,Stopped stopped,Meeting meeting){
        return stepDebrisRoomEffect(ownedDebrisEffects_,key,phase,stopped,meeting);
    }
    using DebrisStepOrder=downwell::RunnerStepSnapshot<downwell::debrisEffectCapacity>;
    void configureDebrisStepOrder(bool (*capture)(void*,DebrisStepOrder&),void* context){
        debrisOrderCapture_=capture;debrisOrderContext_=context;
    }
    // Scoped source-state adapters; no stack callback survives this tick.
    // Natural callers must supply the real per-particle TimeStopBound query.
    template<class Stopped,class Meeting>
    bool stepWithOwnedDebris(std::uint8_t input,Stopped stopped,Meeting meeting){
        if(!run_.effectsSucceeded)return false;
        if(debrisStepDispatch_||!debrisOrderCapture_||
            (debrisInstances_&&debrisInstances_->size()&&debrisBinding_.effects!=&ownedDebrisEffects_))
            return run_.effectsSucceeded=false;
        auto dispatch=[&](downwell::InstanceHandle key){
            return stepOwnedDebrisRoomEffect(key,debrisStepPhase_,stopped,meeting);
        };
        debrisStepContext_=&dispatch;
        debrisStepDispatch_=[](void* p,downwell::InstanceHandle key){return (*static_cast<decltype(dispatch)*>(p))(key);};
        step(input);
        debrisStepDispatch_=nullptr;debrisStepContext_=nullptr;
        return effectsSucceeded();
    }
    template<unsigned Capacity>
    bool advanceDebrisRoomAlarm(downwell::DebrisEffects<Capacity>& effects,downwell::InstanceHandle key,
        typename downwell::DebrisEffects<Capacity>::Phase phase,unsigned index){
        if(!run_.effectsSucceeded)return false;
        const auto* record=debrisInstances_?debrisInstances_->find(key):nullptr;
        return run_.effectsSucceeded=record&&debrisBinding_.effects==&effects&&
            debrisInstances_->dispatchAlarm(key,*record,effects,phase,index);
    }
    template<unsigned Capacity,class Stopped,class Meeting>
    bool stepDebrisRoomEffect(downwell::DebrisEffects<Capacity>& effects,downwell::InstanceHandle key,
        typename downwell::DebrisEffects<Capacity>::Phase phase,Stopped stopped,Meeting meeting){
        if(!run_.effectsSucceeded)return false;
        const auto* record=debrisInstances_?debrisInstances_->find(key):nullptr;
        return run_.effectsSucceeded=record&&debrisBinding_.effects==&effects&&
            debrisInstances_->dispatchStep(key,*record,effects,phase,enemyRandom_,stopped,meeting);
    }
    bool bindScriptSpawnInstances(downwell::GameplayScriptSpawnInstances& instances){
        if(scriptSpawnInstances_||!instances.valid()||instances.size())return false;
        scriptSpawnInstances_=&instances;return true;
    }
    template<unsigned Capacity,class Frames>
    bool spawnScriptEffect(downwell::ScriptSpawnEffects<Capacity>& effects,
        downwell::MotionBinary64 x,downwell::MotionBinary64 y,int sprite,downwell::MotionBinary64 speed,
        downwell::MotionBinary64 angle,downwell::MotionBinary64 depth,Frames frames,downwell::InstanceHandle& myFx){
        return spawnScriptEffectWithCaller(effects,x,y,sprite,speed,angle,depth,frames,
            [&](downwell::InstanceHandle child){myFx=child;return true;});
    }
    template<unsigned Capacity,class Frames,class Caller>
    bool spawnScriptEffectWithCaller(downwell::ScriptSpawnEffects<Capacity>& effects,
        downwell::MotionBinary64 x,downwell::MotionBinary64 y,int sprite,downwell::MotionBinary64 speed,
        downwell::MotionBinary64 angle,downwell::MotionBinary64 depth,Frames frames,Caller caller){
        DW_PROFILE_SCOPE(profileEffectBirth);
        if(!run_.effectsSucceeded)return false;
        if(!scriptSpawnInstances_){run_.effectsSucceeded=false;return false;}
        auto& registration=*scriptSpawnInstances_;
        if(scriptEffectsBinding_.effects!=&effects){
            if(registration.size())return run_.effectsSucceeded=false;
            scriptEffectsBinding_=downwell::ScriptEffectRuntimeBinding::bind<downwell::GameplayScriptSpawnInstances::Record>(effects,registration);
        }
        auto ops=downwell::scriptSpawnOperations(effects,registration,frames,[&](unsigned local){
            const auto key=registration.handle(local);return key.generation&&caller(key);
        },run_.effectsSucceeded);
        return downwell::EffectSpawnScript::run(ops,x,y,sprite,speed,angle,depth);
    }
    template<class Frames> bool spawnScriptEffectWithSourceId(std::uint32_t id,
        downwell::MotionBinary64 x,downwell::MotionBinary64 y,int sprite,downwell::MotionBinary64 speed,
        downwell::MotionBinary64 angle,downwell::MotionBinary64 depth,Frames frames,std::uint32_t& myFx){
        if(!run_.effectsSucceeded||!scriptSpawnInstances_)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=scriptSpawnInstances_->withSourceId(id,[&]{
            return spawnScriptEffectWithCaller(ownedScriptEffects_,x,y,sprite,speed,angle,depth,frames,
                [&](downwell::InstanceHandle child){const auto* r=scriptSpawnInstances_->find(child);
                    if(!r||r->sourceId!=id)return false;myFx=r->sourceId;return true;
                });
        });
    }
    const downwell::ScriptSpawnEffects<downwell::scriptSpawnEffectCapacity>& scriptEffects()const{return ownedScriptEffects_;}
    template<class Execute,class Tick> bool withSourceBulletSteps(Execute execute,Tick tick){
        if(!run_.effectsSucceeded||!bulletLifecycle_.hasOwner()||sourceBulletDispatch_)return run_.effectsSucceeded=false;
        for(unsigned i=0;i<bulletSlotEnd_;++i){const auto& bullet=bullets_[i];
            if(bullet.identitySlot<bulletCapacity&&!bullet.sourceOwned&&bullet.trajectory.initialized)
                return run_.effectsSucceeded=false;
        }
        sourceBulletContext_=&execute;
        sourceBulletDispatch_=[](void* context,downwell::InstanceHandle shot,downwell::MachineGunSourceMotion& motion){
            return (*static_cast<Execute*>(context))(shot,motion);
        };
        const bool ok=tick();sourceBulletDispatch_=nullptr;sourceBulletContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    bool sourceBulletVelocity(downwell::InstanceHandle shot,downwell::MotionBinary64& x,downwell::MotionBinary64& y)const{
        const auto* bullet=resolveBulletLifetime(shot);
        if(!run_.effectsSucceeded||!bullet||!bullet->sourceOwned||!bullet->sourceMotion.initialized)return false;
        const auto& motion=bullet->sourceMotion;
        if(!motion.xsp.valid()||!motion.ysp.valid())return false;
        x=motion.xsp;y=motion.ysp;return true;
    }
    template<class Ops,class Frames>
    bool stepSourceBullet(downwell::InstanceHandle shot,downwell::MachineGunSourceMotion& motion,Ops& ops,Frames frames){
        const auto* caller=resolveBulletLifetime(shot);
        if(!run_.effectsSucceeded||!caller)return run_.effectsSucceeded=false;
        const bool high=caller->gemHigh;
        if(!motion.initialized)motion.zeroSpread=caller->trajectory.zeroSpread;
        return withBulletEffects(shot,frames,[&](auto& effects){
            return downwell::MachineGunSourceStep::run(motion,enemyRandom_,high,effects,ops,run_.effectsSucceeded);
        });
    }
    template<class Frames,class Execute>
    bool withBulletEffects(downwell::InstanceHandle shot,Frames frames,Execute execute){
        if(!run_.effectsSucceeded||!scriptSpawnInstances_)return run_.effectsSucceeded=false;
        auto ops=bulletEffectOperations(shot,ownedScriptEffects_,*scriptSpawnInstances_,frames);
        if(!run_.effectsSucceeded)return false;
        const bool ok=execute(ops);return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    // bulletRanged Collision subparentEnemy: self remains the retained
    // bullet after instance_destroy; other is the registered skeleton.
    template<class Frames,class Delay,class Audio,class Atan>
    bool hitSkeletonWithSourceBullet(downwell::InstanceHandle shot,downwell::InstanceHandle target,
        Frames frames,Delay delay,downwell::SoundPlayEnvironment& environment,Audio& audio,Atan atan){
        const auto* bullet=resolveBulletLifetime(shot);
        if(!run_.effectsSucceeded||!bullet||!bullet->sourceOwned||!bullet->sourceMotion.initialized||
           !skeletons_||!skeletons_->valid()||!skeletons_->find(target))return run_.effectsSucceeded=false;
        const int damage=downwell::machineGunProfile(bullet->gemHigh).damage;
        const auto direction=bullet->sourceMotion.direction;
        const bool ok=skeletons_->dispatch(target,[&](auto& actor,auto&,bool& good){
            struct Events {
                GameplayWorld& world;downwell::InstanceHandle shot,target;
                Frames& frames;Delay& delay;downwell::SoundPlayEnvironment& environment;
                Audio& audio;Atan& atan;bool& good;
                bool clearHitIgnore(){
                    auto* caller=const_cast<BulletState*>(world.resolveBulletLifetime(shot));
                    if(!caller)return false;caller->hitIgnore=false;return true;
                }
                bool destroyBullet(){return world.markBulletDestroyed(shot);}
                bool impactAngle(downwell::MotionBinary64& angle){
                    const auto* caller=world.resolveBulletLifetime(shot);
                    const auto* other=world.skeletons_->find(target);
                    return caller&&other&&downwell::RunnerPointDirection::evaluate(
                        caller->sourceMotion.x.promoted(),caller->sourceMotion.y.promoted(),
                        other->x.promoted(),other->y.promoted(),angle,atan);
                }
                bool momentDelay(){return delay();}
                bool bulletHitSound(int sound,int priority,bool loop,bool cut){
                    auto* caller=const_cast<BulletState*>(world.resolveBulletLifetime(shot));int voice=0;
                    return caller&&downwell::SoundPlayScript::plain(caller->sound,environment,audio,
                        sound,priority,loop,cut,voice,good);
                }
                bool impactEffect(int sprite,downwell::MotionBinary64 speed,
                    downwell::MotionBinary64 angle,int depth){
                    const auto* caller=world.resolveBulletLifetime(shot);if(!caller)return false;
                    const auto x=caller->sourceMotion.x.promoted(),y=caller->sourceMotion.y.promoted();
                    return world.withBulletEffects(shot,frames,[&](auto& effects){
                        return downwell::EffectSpawnScript::run(effects,x,y,sprite,speed,angle,
                            downwell::MotionBinary64::fromInteger(depth));
                    });
                }
            } events{*this,shot,target,frames,delay,environment,audio,atan,good};
            return actor.hitByMachineGunChecked(damage,direction,events,good);
        });
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    template<unsigned Capacity,class Frames>
    auto bulletEffectOperations(downwell::InstanceHandle shot,downwell::ScriptSpawnEffects<Capacity>& effects,
        downwell::GameplayScriptSpawnInstances& registration,Frames frames){
        if(!resolveBulletLifetime(shot)||scriptSpawnInstances_!=&registration||!registration.valid())
            run_.effectsSucceeded=false;
        if(run_.effectsSucceeded&&scriptEffectsBinding_.effects!=&effects){
            if(registration.size())run_.effectsSucceeded=false;
            else scriptEffectsBinding_=downwell::ScriptEffectRuntimeBinding::bind<downwell::GameplayScriptSpawnInstances::Record>(effects,registration);
        }
        return downwell::scriptSpawnOperations(effects,registration,frames,[this,shot,&registration](unsigned local){
            auto* caller=const_cast<BulletState*>(resolveBulletLifetime(shot));
            const auto child=registration.handle(local);
            if(!caller||!child.generation)return false;
            caller->myFx=child;return true;
        },run_.effectsSucceeded);
    }
    template<class Frames> bool spawnBoxScriptEffect(downwell::InstanceHandle box,
        downwell::MotionBinary64 x,downwell::MotionBinary64 y,int sprite,downwell::MotionBinary64 speed,
        downwell::MotionBinary64 angle,downwell::MotionBinary64 depth,Frames frames){
        if(!run_.effectsSucceeded||!findBreakable(box))return run_.effectsSucceeded=false;
        return spawnScriptEffectWithCaller(ownedScriptEffects_,x,y,sprite,speed,angle,depth,frames,
            [&](downwell::InstanceHandle child){
                if(!findBreakable(box))return false;
                breakables_[box.slot].myFx=child;return true;
            });
    }
    bool boneInView(downwell::InstanceHandle bone,bool& visible);
    template<class Frames> bool stepBoneRoomEffect(downwell::InstanceHandle key,bool stopped,Frames frames){
        if(!run_.effectsSucceeded||!bones_||!bones_->valid())return run_.effectsSucceeded=false;
        const bool ok=bones_->dispatch(key,[&](auto& state,auto& publisher,bool& good){
            return state.stepInWorld(stopped,publisher,
                [&]{return spawnBoneImpact(key,frames);},
                [&](bool& visible){return boneInView(key,visible);},good);
        });
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    template<class Frames,class Damage> bool collideBoneRoomEffect(downwell::InstanceHandle key,
        bool player,bool stopped,Frames frames,Damage damage){
        if(!run_.effectsSucceeded||!bones_||!bones_->valid())return run_.effectsSucceeded=false;
        const bool ok=bones_->dispatch(key,[&](auto& state,auto& publish,bool& good){
            return state.collideInWorld(player,stopped,publish,
                [&]{return spawnBoneImpact(key,frames);},damage,good);
        });
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    template<class Frames> bool spawnBoneImpact(downwell::InstanceHandle bone,Frames frames){
        if(!run_.effectsSucceeded||!bones_||!scriptSpawnInstances_)return run_.effectsSucceeded=false;
        const auto* caller=bones_->find(bone);
        if(!caller)return run_.effectsSucceeded=false;
        const auto x=caller->x.promoted(),y=caller->y.promoted();
        auto& registration=*scriptSpawnInstances_;
        if(scriptEffectsBinding_.effects!=&ownedScriptEffects_){
            if(registration.size())return run_.effectsSucceeded=false;
            scriptEffectsBinding_=downwell::ScriptEffectRuntimeBinding::bind<downwell::GameplayScriptSpawnInstances::Record>(ownedScriptEffects_,registration);
        }
        auto ops=downwell::scriptSpawnOperations(ownedScriptEffects_,registration,frames,[&](unsigned local){
            return registration.handle(local).generation&&bones_->assignEffect(bone,local);
        },run_.effectsSucceeded);
        return downwell::EffectSpawnScript::runNol(ops,x,y,85,{0x3fd33333u,0x33333333u});
    }
    template<class Frames> bool spawnScriptEffect(downwell::MotionBinary64 x,downwell::MotionBinary64 y,int sprite,
        downwell::MotionBinary64 speed,downwell::MotionBinary64 angle,downwell::MotionBinary64 depth,
        Frames frames,downwell::InstanceHandle& myFx){
        return spawnScriptEffect(ownedScriptEffects_,x,y,sprite,speed,angle,depth,frames,myFx);
    }
    template<unsigned Capacity>
    bool advanceScriptEffectAnimation(downwell::ScriptSpawnEffects<Capacity>& effects,
        typename downwell::ScriptSpawnEffects<Capacity>::Phase phase){
        if(!run_.effectsSucceeded)return false;
        return run_.effectsSucceeded=scriptSpawnInstances_&&effects.advanceRegistered(phase,*scriptSpawnInstances_);
    }
    enum class PhaseBoundary { beginStep, alarm, step, collision, endStep, roomEntry };
    enum class DrawActor { enemy, bullet, scriptEffect, debris, player, entryDither, bone, skeleton, ditherAbove, timeShard, corpseExpiry, smoke, corpse, consumptionBlood, levelNumber, limboShard, flashBall, blast };
    template<class Visit> bool visitRegisteredEnemyShades(Visit visit)const{
        if((!bones_||!bones_->size())&&(!skeletons_||!skeletons_->size()))return true;
        return visitRoomActorsReadOnly([&](DrawActor kind,downwell::InstanceHandle key,auto){
            if(kind==DrawActor::bone){const auto* r=bones_->find(key);return r&&visit(r->x.promoted(),r->y.promoted());}
            if(kind==DrawActor::skeleton){const auto* r=skeletons_->find(key);return r&&visit(r->x.promoted(),r->y.promoted());}
            return true;
        });
    }
    template<class Visit> bool visitBoneDraw(Visit visit)const{
        if(!bones_||!bones_->size())return true;
        return visitRoomActorsReadOnly([&](DrawActor kind,downwell::InstanceHandle key,auto){
            if(kind!=DrawActor::bone)return true;
            const auto* record=bones_->find(key);
            return record&&visit(record->x.promoted(),record->y.promoted());
        });
    }
    using DrawActorVisitor=bool (*)(void*,DrawActor,downwell::InstanceHandle,downwell::RunnerImageValue);
    using RoomActorDraw=bool (*)(const void*,DrawActorVisitor,void*);
    // Borrowed owner traversal, not a snapshot or a family-local birth sort.
    // Drawing must not mutate room membership; failure stops further visits.
    void configureRoomActorDraw(RoomActorDraw callback,const void* context,RoomActorDraw readOnly=nullptr){roomActorDraw_=callback;roomActorDrawContext_=context;roomActorReadOnlyDraw_=readOnly;}
    bool hasRoomActorDraw()const{return roomActorDraw_!=nullptr;}
    template<class Visit> bool visitRoomActors(Visit visit)const{
        return roomActorDraw_&&roomActorDraw_(roomActorDrawContext_,
            [](void* p,DrawActor kind,downwell::InstanceHandle key,downwell::RunnerImageValue depth){return (*static_cast<Visit*>(p))(kind,key,depth);},&visit);
    }
    // Rendering command collection may retain ordering between ownership
    // mutations. The visitor must not alter lifetimes, flags, depth or sprites.
    // Positions and animation frames are still read live by the visitor.
    template<class Visit> bool visitRoomActorsReadOnly(Visit visit)const{
        const auto callback=roomActorReadOnlyDraw_?roomActorReadOnlyDraw_:roomActorDraw_;
        return callback&&callback(roomActorDrawContext_,
            [](void* p,DrawActor kind,downwell::InstanceHandle key,downwell::RunnerImageValue depth){return (*static_cast<Visit*>(p))(kind,key,depth);},&visit);
    }
    template<class Visit> bool visitDebrisDraw(Visit visit)const{
        if(!debrisInstances_||!debrisInstances_->size())return true;
        return visitRoomActors([&](DrawActor kind,downwell::InstanceHandle key,auto){
            if(kind!=DrawActor::debris)return true;
            const auto* record=debrisInstances_->find(key);
            return record&&record->effect&&visit(*record->effect);
        });
    }
    template<class Ops> bool drawLevelNumber(downwell::InstanceHandle key,Ops& ops)const{
        return levelNumbers_&&levelNumbers_->draw(key,ops);
    }
    // Unlike drawFx particles, these objects draw to surfaceFx even during
    // death/low-spec rendering. Keep them in one source-ordered traversal so
    // the entry cover follows the title. Draw may mutate title animation.
    template<class Visit> bool visitUnconditionalSurfaceDraw(Visit visit)const{
        return visitRoomActors([&](DrawActor kind,downwell::InstanceHandle key,auto depth){
            return (kind!=DrawActor::levelNumber&&kind!=DrawActor::entryDither)||visit(kind,key,depth);
        });
    }
    template<class Emit> bool visitRegisteredEffectSurfaces(const downwell::EffectMaskAssets& assets,
        const downwell::EffectSurfacePlacement::View& view,Emit emit)const{
        return visitRoomActors([&](DrawActor kind,downwell::InstanceHandle key,auto){
            if(kind==DrawActor::corpse){
                const auto* r=registeredCorpses_?registeredCorpses_->find(key):nullptr;
                return r&&downwell::CorpseDraw::surface(r->state,assets,view,emit);
            }
            if(kind==DrawActor::smoke){
                const auto* r=registeredSmoke_?registeredSmoke_->find(key):nullptr;
                return r&&downwell::SmokeDraw::surface(r->state.imageIndex,r->x,r->y,assets,view,emit);
            }
            if(kind==DrawActor::debris){
                const auto* r=debrisInstances_?debrisInstances_->find(key):nullptr;
                return r&&r->effect&&downwell::DebrisDraw::surface(*r->effect,assets,view,emit);
            }
            if(kind==DrawActor::timeShard){
                const auto* r=timeShardForDraw(key);
                return r&&downwell::TimeShardDraw::surface(*r,assets,view,emit);
            }
            return true;
        });
    }
    const downwell::NativeTimeShardRegistration::Record* timeShardForDraw(downwell::InstanceHandle key)const{
        const auto* record=timeShardInstances_?timeShardInstances_->find(key):nullptr;
        if(!record||!record->initialized||!timeShardEffects_||record->destroyRequested||
            timeShardEffects_->sourceId(key.generation)!=record->sourceId)return nullptr;
        return record;
    }
    template<class Visit> bool visitTimeShardDraw(Visit visit)const{
        if(!timeShardInstances_||!timeShardInstances_->size())return true;
        return visitRoomActors([&](DrawActor kind,downwell::InstanceHandle key,auto){
            if(kind!=DrawActor::timeShard)return true;
            const auto* effect=timeShardForDraw(key);return effect&&visit(*effect);
        });
    }
    void configureRoomAnimation(bool (*callback)(void*),void* context){roomAnimation_=callback;roomAnimationContext_=context;}
    void configureRegisteredActorEndStep(bool (*callback)(void*,long),void* context,bool ownsOrdinaryRetirement=false){
        registeredActorEndStep_=callback;registeredActorEndStepContext_=context;
        registeredOrdinaryEndStep_=callback&&ownsOrdinaryRetirement;
    }
    // Scoped ordered collision pass after Step reconciliation, before End
    // Step. The caller owns collection, dispatch and its persistent workspace.
    // Legacy contact loops are not replaced merely by installing this hook.
    template<class Pass,class Tick> bool withCollisionPass(Pass pass,Tick tick){
        if(!run_.effectsSucceeded||collisionPass_)return run_.effectsSucceeded=false;
        collisionPassContext_=&pass;
        collisionPass_=[](void* context){return (*static_cast<Pass*>(context))();};
        const bool ok=tick();collisionPass_=nullptr;collisionPassContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    bool advanceEnemyRoomAnimation(downwell::InstanceHandle key);
    bool advanceEnemyRoomAlarm(downwell::InstanceHandle key,unsigned alarm);
    bool advanceBoxRoomAlarm(downwell::InstanceHandle key,unsigned alarm);
    void configureRoomAlarms(bool (*callback)(void*),void* context){roomAlarms_=callback;roomAlarmsContext_=context;}
    using ResultAlarmEmit=bool (*)(void*,downwell::LevelClearSelection::Effect,unsigned);
    void configureResultRoomAlarms(bool (*callback)(void*,ResultAlarmEmit,void*),void* context){
        resultRoomAlarms_=callback;resultRoomAlarmsContext_=context;
    }
    bool advanceResultRoomAlarm(downwell::InstanceHandle key,unsigned alarm,ResultAlarmEmit emit,void* context){
        const auto* r=resultInstance_?resultInstance_->find(key):nullptr;
        if(!run_.effectsSucceeded||!run_.levelClearActive||!r||!r->active||r->destroyRequested||!emit)
            return run_.effectsSucceeded=false;
        // The admitted Result singleton follows object 37 and the existing
        // scalar object-68 wipe. A new wipe born by this Alarm 0 is not stepped
        // by this alarm phase. Full registered wipe ownership remains separate.
        if(alarm==0)run_.exitDither.advanceAlarm();
        return run_.effectsSucceeded=run_.levelClear.selection.advanceAlarmChecked(alarm,
            [&](auto event,unsigned value){return emit(context,event,value)&&run_.effectsSucceeded;});
    }
    bool advanceBulletRoomAnimation(downwell::InstanceHandle key);
    // Only the native room owner uses these: its image publication callbacks
    // are no-ops and its typed records are the authoritative image state.
    // Consume room-order family runs, never compact-array order. No borrowed
    // record survives an iteration or an effect handler between runs.
    template<class Cursor,class Identity>
    bool advanceNativeEnemyAnimations(Cursor& cursor,Identity identity,unsigned first,unsigned last){
        do {
            const auto slot=cursor.take();
            auto* enemy=const_cast<EnemyState*>(resolveEnemyLifetime({slot-first,identity(slot)}));
            if(!enemy||!run_.effectsSucceeded)return false;
            downwell::advanceEnemyImageQ8(enemy->imagePhase,enemy->imageRemainder,enemy->imageSpeed);
            // phaseIndex rejects this before the native no-op publisher too.
            if(enemy->imagePhase>0x7fffffffu)return run_.effectsSucceeded=false;
        }while(cursor.within(first,last));
        return true;
    }
    template<class Cursor,class Identity>
    bool advanceNativeBulletAnimations(Cursor& cursor,Identity identity,unsigned first,unsigned last){
        do {
            const auto slot=cursor.take();
            auto* bullet=const_cast<BulletState*>(resolveBulletLifetime({slot-first,identity(slot)}));
            if(!bullet||!run_.effectsSucceeded)return false;
            if(!bullet->animation.advance())return run_.effectsSucceeded=false;
        }while(cursor.within(first,last));
        return true;
    }
    bool advanceScriptRoomAnimation(downwell::InstanceHandle key){
        if(!run_.effectsSucceeded)return false;
        const auto* record=scriptSpawnInstances_?scriptSpawnInstances_->find(key):nullptr;
        return run_.effectsSucceeded=record&&(scriptAnimation_?scriptAnimation_(scriptAnimationContext_,key):scriptEffectsBinding_.advance(key,record));
    }
    bool retireScriptRoomEffect(downwell::InstanceHandle key){
        const auto* record=scriptSpawnInstances_?scriptSpawnInstances_->find(key):nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&record&&scriptEffectsBinding_.retire(key,record);
    }
    template<unsigned Capacity> bool stepWithScriptEffects(downwell::ScriptSpawnEffects<Capacity>& effects,std::uint8_t input){
        if(!run_.effectsSucceeded)return false;
        if(!roomAnimation_||scriptAnimation_||!scriptSpawnInstances_||!effects.boundTo(*scriptSpawnInstances_)||!effects.valid())
            return run_.effectsSucceeded=false;
        const auto phase=effects.beginPhase();
        auto dispatch=[&](downwell::InstanceHandle key){
            const auto* record=scriptSpawnInstances_->find(key);
            return record&&effects.advanceRegisteredOne(key.generation,phase,*scriptSpawnInstances_);
        };
        scriptAnimationContext_=&dispatch;
        scriptAnimation_=[](void* context,downwell::InstanceHandle key){return (*static_cast<decltype(dispatch)*>(context))(key);};
        step(input);
        scriptAnimation_=nullptr;scriptAnimationContext_=nullptr;
        return run_.effectsSucceeded&&generationSucceeded_;
    }
    using EnemyStepOrder=downwell::RunnerStepSnapshot<enemyCapacity>;
    // Borrow an encounter dispatcher for a complete tick. Capture once before
    // builder Step births, then enter the two source object-table positions.
    // The owning collision adapter retains handles and rechecks eligibility;
    // no family-local scan may discover children born during this Step.
    template<class Capture,class Execute,class Tick>
    bool withSkeletonBoneSteps(Capture capture,Execute execute,Tick tick){
        if(!run_.effectsSucceeded||skeletonBoneSteps_)return run_.effectsSucceeded=false;
        auto dispatch=[&](int object){return object<0?capture():execute(object);};
        skeletonBoneContext_=&dispatch;
        skeletonBoneSteps_=[](void* context,int object){
            return (*static_cast<decltype(dispatch)*>(context))(object);
        };
        const bool okay=tick();
        skeletonBoneSteps_=nullptr;skeletonBoneContext_=nullptr;
        return run_.effectsSucceeded=okay&&run_.effectsSucceeded;
    }
    // Optional revision covers capture membership, registration order, object
    // identity and exclusions. UINT32_MAX disables reuse (including saturation).
    // Dispatch still resolves current lifetimes/flags; this retains only order.
    void configureEnemyStepOrder(bool (*capture)(void*,EnemyStepOrder&),void* context,
        std::uint32_t (*revision)(void*)=nullptr){
        enemyOrderCapture_=capture;enemyOrderContext_=context;
        enemyOrderRevision_=revision;enemyOrderValid_=false;
    }
    void configurePhaseBoundary(bool (*callback)(void*,PhaseBoundary),void* context){
        phaseBoundary_=callback;phaseBoundaryContext_=context;
    }
    // RoomPack serializes instanceCreationOrder, not object/position order.
    // This terrain-only handoff does not execute the complete room lifecycle.
    template<class Terrain>
    bool loadRoomTerrain(const downwell::RoomPack& rooms,const downwell::PackedRoom& room,Terrain terrain,
                         const downwell::PreparedRoomTerrain* prepared=nullptr){
        return loadRoomTerrainCreates(rooms,room,terrain,prepared,[](const auto&){return true;});
    }
    // Fresh room creation, not adoption of an already-executed stored room.
    // Admit the authored builder Create, dither and field at their serialized
    // boundaries, not before or after the terrain batch. The no-callback
    // overload is for callers that already own an initialized builder.
    template<class Terrain>
    bool loadRoomTerrainWithStartField(const downwell::RoomPack& rooms,const downwell::PackedRoom& room,
        Terrain terrain,const downwell::PreparedRoomTerrain* prepared=nullptr){
        return loadRoomTerrainWithStartField(rooms,room,terrain,prepared,[](const auto&){return true;});
    }
    template<class Terrain,class BuilderCreate>
    bool loadRoomTerrainWithStartField(const downwell::RoomPack& rooms,const downwell::PackedRoom& room,
        Terrain terrain,const downwell::PreparedRoomTerrain* prepared,BuilderCreate builderCreate){
        bool seen=false,aboveSeen=false;
        if(!loadRoomTerrainCreates(rooms,room,terrain,prepared,[&](const auto& instance){
            if(instance.objectId==65)return builderCreate(instance);
            if(instance.objectId==61){
                if(aboveSeen||!instance.instanceId)return false;
                aboveSeen=true;
                // Unindexed terrain-only fixtures do not install this owner.
                // Every native GameplayCollisionOwner binds it before entry.
                return !ditherAbove_||ditherAbove_->create(instance.instanceId,instance.x,instance.y);
            }
            if(instance.objectId!=371)return true;
            if(seen||!instance.instanceId||instance.x!=240||instance.y!=192)return false;
            seen=true;return createRoomStartFieldRecord(instance.instanceId);
        }))return false;
        if(!seen||(ditherAbove_&&!aboveSeen))return run_.effectsSucceeded=false;
        return finishRoomStartField();
    }
private:
    template<class Terrain,class Create>
    bool loadRoomTerrainCreates(const downwell::RoomPack& rooms,const downwell::PackedRoom& room,Terrain terrain,
        const downwell::PreparedRoomTerrain* prepared,Create create){
        if(!run_.effectsSucceeded)return false;
        if(roomTerrainLoaded_||!rooms.valid()){run_.effectsSucceeded=false;return false;}
        roomTerrainLoaded_=true; // A partial load cannot be retried.
        bool entryViewsReady=false;
#ifndef DOWNWELL_REFERENCE_ROOM_VIEWS
        // Construction already published this immutable rmMain snapshot before
        // player Create. It is not the followed camera or a script-writable
        // view. Re-decoding it here publishes no additional source state.
        entryViewsReady=entryRooms_==&rooms&&room.roomId==11&&roomEntryViews_.get(0);
#endif
        if(!entryViewsReady&&!roomEntryViews_.load(rooms,room.roomId)){
            run_.effectsSucceeded=false;return false;
        }
        const bool retained=prepared&&prepared->matches(rooms,room);
        const unsigned count=retained?prepared->count():room.instanceCount;
        bool playerSeen=false;
        for(unsigned i=0;i<count;++i){
            downwell::PackedRoomInstance decoded;
            if(!retained&&!rooms.roomInstance(room,i,decoded)){run_.effectsSucceeded=false;return false;}
            const auto& instance=retained?prepared->at(i):decoded;
            if(instance.objectId==65&&builderController_&&
                !builderController_->create(instance.instanceId,instance.x,instance.y))
                return run_.effectsSucceeded=false;
            if(instance.objectId==35&&playerBirthPending_){
                if(!birthPlayerOwnership())return false;
                playerBirthPending_=false;
                if(playerChildCreate_&&!playerChildCreate_(playerChildCreateContext_,*this))
                    return run_.effectsSucceeded=false;
                // Preserve the initial/Retry presentation snapshot. Later
                // rooms retain their unadvanced camera until ordinary Step.
                if(!tick_)initializeMainCameraFrame();
            }
            if(!create(instance))return run_.effectsSucceeded=false;
            if(instance.objectId==35){
                // Either just born at this boundary or already supplied by an
                // entered-world fixture. Retain the authored ID without a
                // second birth or inventing it from the lifetime handle.
                if(playerSeen||!instance.instanceId||!playerInstance_.lifetimeId||
                    (playerInstance_.sourceInstanceId&&playerInstance_.sourceInstanceId!=instance.instanceId)){
                    run_.effectsSucceeded=false;return false;
                }
                playerSeen=true;
                playerInstance_.sourceInstanceId=instance.instanceId;
                continue;
            }
            if(instance.objectId!=60&&instance.objectId!=93&&instance.objectId!=99)continue;
            if(!terrain(instance)||(terrainInstances_&&(!terrainInstances_->appendRoom(instance)||
                !terrainInstances_->createProperties(terrainInstances_->handle(terrainInstances_->size()-1),
                    downwell::terrainTileForArea(run_.stage.area),false,enemyRandom_)||
                !terrainInstances_->roomCreation(terrainInstances_->handle(terrainInstances_->size()-1))))){
                run_.effectsSucceeded=false;return false;
            }
            if(nativeTerrainInstances_){
                const auto h=nativeTerrainInstances_->handle(nativeTerrainInstances_->size()-1);
                if(!finishNativeTerrainCreate(h)||!nativeTerrainInstances_->roomCreation(h))return run_.effectsSucceeded=false;
            }
        }
        return true;
    }
public:
    const downwell::RoomEntryViews::View* roomEntryView(unsigned slot=0)const{
        return roomEntryViews_.get(slot);
    }
    downwell::InstanceHandle playerHandle() const { return {0,playerInstance_.lifetimeId}; }
    bool playerDamaged()const{return run_.playerDamaged;}
    bool playerDeathActive()const{return death_.active;}
    bool playerHealthPositive()const{return health_>0;}
    bool noDamageAchievement()const{return run_.noDamage;}
    bool areaNoDamageAchievement()const{return run_.areaNoDamage;}
    void clearNoDamageAchievement(){run_.noDamage=false;}
    void clearAreaNoDamageAchievement(){run_.areaNoDamage=false;}
    void setPlayerDamaged(){run_.playerDamaged=true;}
    unsigned playerHit()const{return run_.playerHit;}
    void setPlayerHit(){run_.setPlayerHit();}
    void playerDamageEndStep(){run_.playerDamageEndStep();}
    bool setPlayerDamageAlarm(downwell::InstanceHandle key,int ticks){
        if(!run_.effectsSucceeded)return false;
        if(key.slot||!key.generation||key.generation!=playerInstance_.lifetimeId)return run_.effectsSucceeded=false;
        playerInstance_.damageAlarm=ticks;return true;
    }
    bool playerDamageAlarmPending(downwell::InstanceHandle key,unsigned alarm,bool& pending)const{
        if(key.slot||!key.generation||key.generation!=playerInstance_.lifetimeId)return false;
        pending=alarm==1&&playerInstance_.damageAlarm>=0;return true;
    }
    bool advancePlayerDamageAlarm(downwell::InstanceHandle key,unsigned alarm){
        bool pending=false;
        if(!run_.effectsSucceeded)return false;
        if(alarm!=1||!playerDamageAlarmPending(key,alarm,pending))return run_.effectsSucceeded=false;
        if(downwell::AlarmBank::advanceValue(playerInstance_.damageAlarm))run_.expirePlayerDamage();
        return true;
    }
    bool playerTimeFieldInstance(downwell::RunnerImageValue x,downwell::RunnerImageValue y,
                                 downwell::InstancePlaceResult& result){
        if(!run_.effectsSucceeded)return false;
        run_.effectsSucceeded=playerInstance_.lifetimeId&&playerLifecycle_.instancePlace(
            playerHandle(),playerInstance_,82,x.promoted(),y.promoted(),result);
        return run_.effectsSucceeded;
    }
    const PlayerInstance* findPlayer(downwell::InstanceHandle handle) const {
        return handle.slot==0&&handle.generation&&handle.generation==playerInstance_.lifetimeId
            ? &playerInstance_ : nullptr;
    }
    unsigned breakableSlots() const { return breakableCount_; }
    const Breakable& breakableSlot(unsigned slot) const { return breakables_[slot]; }
    const Breakable* findBreakable(downwell::InstanceHandle handle) const {
        if(handle.slot>=breakableCount_||!handle.generation)return nullptr;
        const auto& box=breakables_[handle.slot];
        return box.lifetimeId==handle.generation ? &box : nullptr;
    }
    const downwell::CurrencyRewardFields* boxReward(downwell::InstanceHandle h)const{
        return findBreakable(h)?&boxRewards_[h.slot]:nullptr;
    }
    template<class Position,class Allocate,class Queries>
    bool spawnBoxCurrency(const downwell::ObjectPack& objects,downwell::InstanceHandle box,
        downwell::MotionBinary64 amount,Position position,Allocate allocate,Queries& queries){
        auto retained=[&]()->downwell::CurrencyRewardFields*{
            return findBreakable(box)?&boxRewards_[box.slot]:nullptr;
        };
        return spawnRegisteredCurrency(objects,[&](downwell::CurrencySpawnSource<std::uint32_t>& out){
            if(!retained()||!position(box,out.x,out.y))return false;
            // Position callbacks may synchronously retire or replace a caller.
            const auto* r=retained();if(!r)return false;
            out.objectIndex=92;out.moniAmount=r->moniAmount;out.myGem=r->myGem;
            out.atrState=r->atrState;out.stomped={};return true;
        },[&](auto value){auto* r=retained();if(!r)return false;r->moniAmount=value;return true;},
          [&](bool value){auto* r=retained();if(!r)return false;r->atrState=value;return true;},
          [&](std::uint32_t value){auto* r=retained();if(!r)return false;r->myGem=value;return true;},
          amount,allocate,queries);
    }
    bool publishBoxHealth(downwell::InstanceHandle h,downwell::MotionBinary64 health){
        if(!run_.effectsSucceeded||!findBreakable(h)||!health.valid())return run_.effectsSucceeded=false;
        breakables_[h.slot].state.wallHp=health;return true;
    }
    template<class Controls,class Position,class Allocate,class Queries>
    auto boxCurrencyOperations(const downwell::ObjectPack& objects,downwell::InstanceHandle box,
        Controls controls,Position position,Allocate allocate,Queries& queries){
        struct Operations:Controls {
            GameplayWorld& world;const downwell::ObjectPack& objects;downwell::InstanceHandle box;
            Position position;Allocate allocate;Queries& queries;
            Operations(GameplayWorld& w,const downwell::ObjectPack& o,downwell::InstanceHandle h,
                Controls c,Position p,Allocate a,Queries& q)
                :Controls(c),world(w),objects(o),box(h),position(p),allocate(a),queries(q){}
            bool currency(int amount){return world.spawnBoxCurrency(objects,box,
                downwell::MotionBinary64::fromInteger(amount),position,allocate,queries);}
        };
        return Operations(*this,objects,box,controls,position,allocate,queries);
    }
    int overlapSound(unsigned channel)const{return channel<11?run_.overlapSounds[channel]:-1;}
    template<class Audio> bool playSkeletonStompSound(downwell::SkeletonActor& actor,
        downwell::SoundPlayEnvironment& environment,Audio& audio){
        return run_.playSkeletonStompSound(actor.sound,environment,audio);
    }
    template<class Allocate,class Events,class Audio>
    bool stompSkeleton(downwell::SkeletonActor& actor,Allocate allocate,Events& events,
        downwell::SoundPlayEnvironment& environment,Audio& audio,bool& good){
        if(!run_.effectsSucceeded)return good=false;
        const bool ok=downwell::registeredSkeletonStomp(*this,actor,events,[&]{return emitSkeletonBlood(actor,allocate);},
            [&](auto x,auto y,auto direction,auto speed){return emitRegisteredSmoke(actor.smokeEmitter,x,y,direction,speed,allocate);},
            environment,audio,good);
        return run_.effectsSucceeded=ok;
    }
    template<class Allocate,class Delay,class Events,class Audio>
    bool stompSkeletonWithBlast(downwell::SkeletonActor& actor,Allocate allocate,Delay delay,Events& events,
        downwell::SoundPlayEnvironment& environment,Audio& audio,bool& good){
        if(!run_.effectsSucceeded)return good=false;
        const bool ok=downwell::registeredSkeletonStomp(*this,actor,events,[&]{return emitSkeletonBlood(actor,allocate);},
            [&](auto x,auto y,auto direction,auto speed){return emitRegisteredSmoke(actor.smokeEmitter,x,y,direction,speed,allocate);},
            [&](auto x,auto y){unsigned id=0;downwell::InstanceHandle child;
                return allocate(id)&&createBlastWithSourceId(id,x,y,delay,child);
            },environment,audio,good);
        return run_.effectsSucceeded=ok;
    }
    unsigned chooseSkeletonExplosion(unsigned count){
        if(!run_.effectsSucceeded||count!=3){run_.effectsSucceeded=false;return 3;}
        return enemyRandom_.chooseIndexSmall(3);
    }
    template<class Audio> bool playSkeletonSound(downwell::SkeletonActor& actor,int sound,
        downwell::SoundPlayEnvironment& environment,Audio& audio){
        if(!run_.effectsSucceeded)return false;
        auto top=downwell::MotionBinary64::fromInteger(boneViewTopQ8_);
        if(!top.zero())top.high-=8u<<20;
        environment.viewY=top;
        environment.viewHeight=downwell::MotionBinary64::fromInteger(284);
        environment.y=downwell::MotionBinary64::fromInteger(actor.y);
        return run_.playSkeletonSound(actor.sound,sound,environment,audio);
    }
    template<class Material,class Audio> bool playBulletWallSound(downwell::InstanceHandle shot,
        downwell::SoundPlayEnvironment& environment,Material material,Audio& audio){
        auto* caller=const_cast<BulletState*>(resolveBulletLifetime(shot));
        if(!run_.effectsSucceeded||!caller)return run_.effectsSucceeded=false;
        return run_.playBulletWallSound(caller->sound,environment,enemyRandom_,material,audio);
    }
    bool noKillAchievement()const{return run_.noKill;}
    downwell::MotionBinary64 oxygen()const{return run_.oxygen;}
    bool setOxygen(downwell::MotionBinary64 value){return run_.setOxygen(value);}
    bool firstLand()const{return run_.firstLand;}
    int backgroundMusicId()const{return run_.backgroundMusicId;}
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
    bool beginLevelCue(bool& firstLand,bool playerTimeStop,int& bgm,downwell::MotionBinary64 x,
        downwell::MotionBinary64 y,Create create,AreaMusic music,downwell::SoundPlayState& caller,
        downwell::SoundPlayEnvironment& env,Audio& audio){
        return run_.beginLevelCue(firstLand,playerTimeStop,bgm,x,y,create,music,caller,env,audio);
    }
    template<class Allocate,class AreaMusic,class Audio>
    bool beginLevelCueWithTitle(bool& firstLand,bool stopped,int& bgm,downwell::MotionBinary64 x,
        downwell::MotionBinary64 y,Allocate allocate,const downwell::LevelNumberTextContext& text,
        AreaMusic music,downwell::SoundPlayState& caller,downwell::SoundPlayEnvironment& env,Audio& audio){
        return run_.beginLevelCue(firstLand,stopped,bgm,x,y,[&](int object,auto px,auto py){
            unsigned id=0;downwell::InstanceHandle child;
            return object==372&&levelNumbers_&&allocate(id)&&levelNumbers_->create(id,px,py,
                [&](auto,auto& state){return text.initialize(state,run_.stage);},child);
        },music,caller,env,audio);
    }
    template<unsigned TimeCapacity,class Allocate,class Audio>
    bool beginLevelCueWithTitle(const downwell::TimeStopEffects<TimeCapacity>& stop,downwell::MotionBinary64 x,
        downwell::MotionBinary64 y,Allocate allocate,const downwell::LevelNumberTextContext& text,
        downwell::SoundPlayState& caller,downwell::SoundPlayEnvironment& env,Audio& audio){
        return beginLevelCueWithTitle(run_.firstLand,stop.player.stopped,run_.backgroundMusicId,x,y,
            allocate,text,downwell::RunSession::areaMusic,caller,env,audio);
    }
    template<unsigned TimeCapacity,class Allocate,class Delay,class Audio>
    bool recordSkeletonDeath(downwell::SkeletonActor& actor,bool cutGunshot,const downwell::TimeStopEffects<TimeCapacity>& stop,
        Allocate allocate,Delay delay,const downwell::LevelNumberTextContext& text,
        downwell::SoundPlayEnvironment& environment,Audio& audio){
        return recordEnemyDeath(cutGunshot,audio,delay,[&]{
            return beginLevelCueWithTitle(stop,downwell::MotionBinary64::fromInteger(actor.x),
                downwell::MotionBinary64::fromInteger(actor.y),allocate,text,actor.sound,environment,audio);
        });
    }
    template<class Audio> bool playBoxChainSound(downwell::InstanceHandle h,downwell::MotionBinary64 volume,bool inWater,Audio& audio){
        if(!run_.effectsSucceeded||!findBreakable(h))return run_.effectsSucceeded=false;
        return run_.playBoxChainSound(breakables_[h.slot].sound,volume,inWater,audio);
    }
    template<class Audio> bool playBoxBreakSound(downwell::InstanceHandle h,downwell::MotionBinary64 volume,Audio& audio){
        if(!run_.effectsSucceeded||!findBreakable(h))return run_.effectsSucceeded=false;
        auto& box=breakables_[h.slot];
        return run_.playBoxBreakSound(box.sound,box.state.breakSound,volume,audio);
    }
    template<class Audio> bool playBoxBreakSound(downwell::InstanceHandle h,int sound,downwell::MotionBinary64 volume,Audio& audio){
        if(!run_.effectsSucceeded||!findBreakable(h))return run_.effectsSucceeded=false;
        return run_.playBoxBreakSound(breakables_[h.slot].sound,sound,volume,audio);
    }
    // False only when no live box counts this alarm down: every box's own
    // Alarm 0 is unset. Boxes have no other alarm, which stays conservative.
    bool boxAlarmMayDispatch(unsigned alarm)const{
        if(alarm!=0)return true;
        for(unsigned i=0;i<breakableCount_;++i)
            if(breakables_[i].lifetimeId&&breakables_[i].alarm0>=0)return true;
        return false;
    }
    // Native boxes never re-register or compact. A prefix plus generation
    // ceiling captures the same lifetimes without copying their handles.
    // The ceiling also excludes replacement slots after a room/config reset.
    struct BoxAlarmPrefix {unsigned end=0;std::uint32_t lastLifetime=0;};
    BoxAlarmPrefix captureBoxAlarms()const{return {breakableCount_,nextBreakableLifetimeId_-1};}
    bool advanceBoxAlarmPrefix(BoxAlarmPrefix phase){
        if(!run_.effectsSucceeded)return false;
        const unsigned end=phase.end<breakableCount_?phase.end:breakableCount_;
        for(unsigned i=0;i<end;++i){
            auto& box=breakables_[i];
            if(!box.lifetimeId||box.lifetimeId>phase.lastLifetime||!box.active||box.destroyRequested)continue;
            if(downwell::AlarmBank::advanceValue(box.alarm0))box.state.alarmZero();
        }
        return true;
    }
    bool publishBoxAlarm0(downwell::InstanceHandle h,std::int32_t value){
        if(!run_.effectsSucceeded||!findBreakable(h))return run_.effectsSucceeded=false;
        breakables_[h.slot].alarm0=value;return true;
    }
    template<class Query> bool stopBoxAlarm0(downwell::InstanceHandle h,Query timeBound){
        if(!run_.effectsSucceeded||!findBreakable(h))return run_.effectsSucceeded=false;
        bool timeAllowed=false;
        if(!timeBound(timeAllowed))return run_.effectsSucceeded=false;
        if(!run_.effectsSucceeded||!findBreakable(h))return run_.effectsSucceeded=false;
        auto& box=breakables_[h.slot];
        return run_.effectsSucceeded=box.state.stopAlarmZero(timeAllowed,box.alarm0);
    }
    bool publishBoxHit(downwell::InstanceHandle h){
        if(!run_.effectsSucceeded||!findBreakable(h))return run_.effectsSucceeded=false;
        breakables_[h.slot].state.hit=true;return true;
    }
    GameplayFrame step(std::uint8_t packedInput);
    // Execute one selected legacy enemy lifetime without walking its family.
    // The caller owns source phase ordering and the phase's published view.
    // This preserves the existing behavior pending the complete mixed Step
    // dispatcher; it does not claim original motion/collision equivalence.
    bool stepEnemyRoomEffect(downwell::InstanceHandle key,int publishedCameraX,long generationCameraY);
    // Same selected-lifetime boundary for the retained projectile Step. Its
    // approximate trajectory and legacy contacts are not source collision
    // dispatch; keep those fidelity gates when wiring the mixed scheduler.
    bool stepBulletRoomEffect(downwell::InstanceHandle key,long generationCameraY,bool& bulletChanged);
    // Borrow only for this tick scope, as with the stored event composition.
    // The transition runs after published x/y and interaction observation,
    // before later player work, room alarms and ordinary Step. It must not
    // reset or replace the world; source Create effects are permitted.
    template<class Transition,class Tick>
    bool withPlayerTimeTransition(Transition transition,Tick tick){
        if(!run_.effectsSucceeded)return false;
        if(playerTimeTransition_)return run_.effectsSucceeded=false;
        playerTimeContext_=&transition;
        playerTimeTransition_=[](void* context,const PlayerInstance& player,
                                const PlayerMotion& motion,int x,int y){
            return (*static_cast<Transition*>(context))(player,motion,x,y);
        };
        const bool ok=tick();
        playerTimeTransition_=nullptr;playerTimeContext_=nullptr;
        return run_.effectsSucceeded=ok&&run_.effectsSucceeded;
    }
    // Engine destruction mark, after any game Destroy/CleanUp callbacks.
    // Keep the typed record until the owner's Draw-entry cleanup.
    bool markEnemyDestroyed(downwell::InstanceHandle handle);
    template<class Read,class Operations,class Tick>
    bool withBoxDestruction(Read read,Operations operations,Tick tick){
        return boxStepDispatch_.run(run_.effectsSucceeded,read,operations,tick);
    }
    bool markBreakableDestroyed(downwell::InstanceHandle handle);
    bool retireMarkedEnemy(downwell::InstanceHandle handle);
    bool retireMarkedBreakable(downwell::InstanceHandle handle);
    void initializeEntryInput(bool fireHeld) { player_.initializeEntryInput(fireHeld); }
    void configureLandingProfile(const int* offsets, unsigned count) {
        player_.configureLandingProfile(offsets, count);
    }
    void configureLandingResolver(PlayerMotion::LandingResolver resolver,
                                  void* context) {
        landingResolver_ = resolver;
        landingResolverContext_ = context;
        player_.configureLandingResolver(resolver ? [](void* opaque, int x, int minimumY) {
            const auto& runtime = *static_cast<GameplayWorld*>(opaque);
            const int nearest = runtime.landingResolver_(runtime.landingResolverContext_, x, minimumY);
            return runtime.run_.shops.playerLanding(x + 240, minimumY + 176, nearest + 176) - 176;
        } : static_cast<PlayerMotion::LandingResolver>(nullptr), this);
    }
    void configureBreakables(const int* x, const int* y, unsigned count);
    // Append a newly generated batch without reviving destroyed earlier blocks.
    // Reject capacity failure before changing any state.
    bool appendBreakables(const int* x, const int* y, unsigned count,
        const downwell::BreakableState* creation=nullptr,const std::uint32_t* sourceIds=nullptr);
    bool appendGeneratedBreakable(int x,int y,const downwell::GeneratedBreakableCreate& creation,
        std::uint32_t sourceId=0);
    void configureSolidResolver(SolidResolver resolver, void* context, SegmentResolver segment = nullptr,
                                SolidResolver mask = nullptr,WallContactResolver walls = nullptr,
                                PlatformContactResolver platforms = nullptr) {
        solidResolver_ = resolver;
        segmentResolver_ = segment;
        maskResolver_ = mask;
        wallContactResolver_ = walls;
        platformContactResolver_ = platforms;
        solidResolverContext_ = context;
        player_.configureSolidResolver(resolver ? [](void* opaque, int x, int y) {
            const auto& runtime = *static_cast<GameplayWorld*>(opaque);
            return (runtime.solidResolver_ && runtime.solidResolver_(runtime.solidResolverContext_, x, y)) ||
                runtime.run_.shops.playerSolidAt(x + 240, y + 176);
        } : static_cast<SolidResolver>(nullptr), this, -236, 236,
        resolver && mask ? [](void* opaque, int x, int y) {
            const auto& runtime = *static_cast<GameplayWorld*>(opaque);
            return runtime.maskResolver_(runtime.solidResolverContext_, x, y) ||
                runtime.run_.shops.playerSolidAt(x+236, y+172) ||
                runtime.run_.shops.playerSolidAt(x+243, y+172) ||
                runtime.run_.shops.playerSolidAt(x+236, y+183) ||
                runtime.run_.shops.playerSolidAt(x+243, y+183);
        } : static_cast<SolidResolver>(nullptr));
    }
    bool configureEnemy(unsigned index, int objectId, int spriteId,
                        int x, int y, int speedX, int speedY,
                        int health, int reward,
                        int motionState = 0, int auxiliaryState = 0,
                        int alarm0 = -1, int alarm1 = -1,
                        int direction = 0, int moveSpeed = 0,
                        bool active = true, bool grounded = false,
                        bool awake = false);
    void configureEnemyRandom(const downwell::GameMakerRandom& random) {
        enemyRandom_ = random;
        enemyRandom_.prepareLookahead(randomLookahead_);
    }
    // Synchronous builder handoff. Shop/exit Create effects must occur before
    // the next pattern cell draws RNG. The backend owns terrain representation;
    // this world owns breakable and actor registration and coordinate mapping.
    // Trusted homogeneous-run sink: it may publish backend terrain and fail,
    // but must not mutate/query the collision owner, create other instances,
    // or change stage/area. Those callbacks use createGeneratedTerrain instead.
    // The core owner borrows this interval to share setup, never future cells.
    template<class Terrain>
    bool createGeneratedTerrainRun(const downwell::AreaOneBuilder::TerrainRun& run,Terrain terrain){
        return run.executeEach([&](const downwell::DeferredCreateInstance& command){
            return createGeneratedTerrain(command,terrain);
        });
    }
    template<class Terrain>
    bool createGeneratedTerrain(const downwell::DeferredCreateInstance& command,Terrain terrain){
        if(!run_.effectsSucceeded||(command.objectId!=60&&command.objectId!=93&&command.objectId!=99))return false;
        if(!terrain(command))return false;
        if(terrainInstances_&&
            (!terrainInstances_->append(command.objectId,command.x,command.y,command.active)||
             !terrainInstances_->createProperties(terrainInstances_->handle(terrainInstances_->size()-1),
                 downwell::terrainTileForArea(run_.stage.area),false,enemyRandom_))){
            run_.effectsSucceeded=false;return false;
        }
        return true;
    }
    template<class Terrain>
    bool createGeneratedInstance(const downwell::DeferredCreateInstance& command,
                                 const downwell::ObjectPack& objects, Terrain terrain) {
        if(command.objectId==60||command.objectId==93||command.objectId==99)
            return createGeneratedTerrain(command,terrain);
        if(!run_.effectsSucceeded)return false;
        // rmMain has no authored water or non-platform thin walls. Retain
        // attempted unsupported births even when the provisional generator
        // handoff below has no actor implementation; absence must not be faked.
        switch(command.objectId){
            case 87:case 100:case 318:case 320:case 324:currencyOnlyPlatforms_=false;break;
            case 88:case 94:case 95:case 96:case 103:currencyEmptyWater_=false;break;
            default:break;
        }
        if (command.objectId == 277) {
            DW_GENERATION_SCOPE(profileGeneratedShop);
            return registerShop(command.x, command.y) != nullptr;
        }
        if (command.objectId == 306 && !registerLevelExit(command.y)) return false;
        if (command.objectId == 82)
            return run_.adoptTimeField(command.x,command.y,command.timeFieldRadiusWord());
        if (command.objectId == 62) return registerSideCameraLocker(command.x, command.y);
        if (!terrain(command)) return false;
        if (command.objectId == 92) {
            DW_GENERATION_SCOPE(profileGeneratedBreakable);
            const int x = command.x - 240, y = command.y - 176;
            const auto state=command.boxCreate();
            return appendBreakables(&x, &y, 1,&state);
        }
        const auto profile = downwell::enemyCreateProfile(command.objectId);
        if (!profile.health) return true;
        DW_GENERATION_SCOPE(profileGeneratedEnemy);
        downwell::PackedObject object;
        return objects.findObject(command.objectId, object) &&
            configureEnemy(enemyCount_, command.objectId, object.spriteId,
                command.x - 240, command.y - 176,
                command.integerVelocityX, command.integerVelocityY, profile.health, profile.reward,
                command.motionState, command.auxiliaryState,
                command.alarms.get(0), command.alarms.get(1),
                command.integerDirection, command.integerMoveSpeed,
                command.active, command.grounded, true);
    }
    std::uint64_t randomDrawCount() const { return enemyRandom_.drawCount(); }
    const downwell::GameMakerRandom& randomState() const { return enemyRandom_; }

    // Pure query of the implemented area-one snapshot, in room coordinates.
    downwell::LaserSightRay traceLaserSight(downwell::LaserSightRay::Coordinate x,
        downwell::LaserSightRay::Coordinate y, downwell::LaserSightRay::Coordinate bottom,
        std::uint32_t speedTurns, std::uint32_t aimTurns = 0) const;
    downwell::LaserSightRay laserSightEndpoint() const {
        using Ray = downwell::LaserSightRay;
        return traceLaserSight(Ray::fromQ8(player_.offsetXFixed(), 240),
            Ray::fromQ8(player_.offsetYFixed(), 176),
            Ray::fromQ8(viewCameraYQ8(), 176 + 142),
            Ray::turnsFromQ8Degrees(player_.speedXFixed()));
    }
    const PlayerMotion& player() const { return player_; }
    unsigned tick() const { return tick_; }
    unsigned ringOrigin() const { return ringOrigin_; }
    unsigned bulletCount() const;
    unsigned bulletSlotEnd() const { return bulletSlotEnd_; }
    unsigned gemSlotEnd() const { return gemSlotEnd_; }
    const BulletState& bulletAt(unsigned index) const;
    const BulletState* resolveBulletLifetime(downwell::InstanceHandle h)const{
        if(h.generation&&h.slot<bulletCapacity&&bulletIdentitySlots_[h.slot])
            for(unsigned i=0;i<bulletSlotEnd_;++i)
                if(bullets_[i].identitySlot==h.slot&&bullets_[i].lifetimeId==h.generation)return &bullets_[i];
        return nullptr;
    }
    bool bindBulletLifecycle(BulletLifecycleBinding binding){
        if(bulletSlotEnd_||!run_.effectsSucceeded)return false;bulletLifecycle_=binding;return true;
    }
    bool retireMarkedBullet(downwell::InstanceHandle h);
    // Destruction is visible to collision queries immediately, but the
    // executing record remains resolvable until room cleanup.
    bool markBulletDestroyed(downwell::InstanceHandle h);
    // Builtin collision x/y only. The executing motion owner commits its
    // private fields after each successful setter, not inside this callback.
    bool publishBulletCollisionPosition(downwell::InstanceHandle h,
        downwell::RunnerImageValue x,downwell::RunnerImageValue y);
    bool publishBulletAngle(downwell::InstanceHandle h,downwell::RunnerImageValue angle);
    const BulletState* findBullet(std::uint32_t lifetimeId) const {
        if(lifetimeId)for(unsigned i=0;i<bulletSlotEnd_;++i)
            if(bullets_[i].active && bullets_[i].lifetimeId==lifetimeId)return &bullets_[i];
        return nullptr;
    }
    const EnemyState& enemyAt(unsigned index) const;
    const EnemyState* findEnemy(std::uint32_t lifetimeId) const {
        if(lifetimeId)for(unsigned i=0;i<enemyCount_;++i)
            if((enemies_[i].active || enemies_[i].activationPending) &&
               enemies_[i].lifetimeId==lifetimeId)return &enemies_[i];
        return nullptr;
    }
    downwell::InstanceHandle enemyHandle(std::uint32_t lifetimeId) const {
        const auto* enemy=findEnemy(lifetimeId);
        return enemy?downwell::InstanceHandle{enemy->identitySlot,enemy->lifetimeId}:downwell::InstanceHandle{};
    }
    const EnemyState* findEnemy(downwell::InstanceHandle handle) const {
        const auto* enemy=resolveEnemyLifetime(handle);
        return enemy&&(enemy->active||enemy->activationPending)?enemy:nullptr;
    }
    // Lifetime resolution must not discard inactive records before collision
    // dispatch applies its own source-specific exclusion checks.
    const EnemyState* resolveEnemyLifetime(downwell::InstanceHandle handle) const {
        if(handle.slot>=enemyCapacity||!handle.generation||!enemySlotIndex_[handle.slot])return nullptr;
        const unsigned index=enemySlotIndex_[handle.slot]-1;
        if(index>=enemyCount_)return nullptr;
        const auto& enemy=enemies_[index];
        return enemy.identitySlot==handle.slot&&enemy.lifetimeId==handle.generation?&enemy:nullptr;
    }
    const downwell::FlashBallEffects<32>& flashEffects() const {return run_.flashEffects;}
    const downwell::CorpseExpiryEffects<32>& corpseExpiryEffects()const{return run_.corpseExpiryEffects;}
    const downwell::PlayerDamageNumbers<8>& damageNumbers()const{return run_.damageNumbers;}
    template<class Expiry,class Number> bool visitForegroundEffects(Expiry expiry,Number number)const{
        return run_.visitForegroundEffects(expiry,number);
    }
    bool emitDamageNumber(downwell::MotionBinary64 x,downwell::MotionBinary64 y,
        downwell::MotionBinary64 amount,downwell::MotionBinary64 direction){
        return run_.createDamageNumber(x,y,amount,direction)!=0;
    }
    bool emitCorpseExpiry(downwell::MotionBinary64 x,downwell::MotionBinary64 y){return run_.emitCorpseExpiry(x,y);}
    const downwell::SmokeEffects<32>& smokeEffects() const {return run_.smokeEffects;}
    bool emitBoxSmoke(downwell::MotionBinary64 x,downwell::MotionBinary64 y){return run_.emitBoxSmoke(x,y,enemyRandom_);}
    bool emitBoxFlash(downwell::MotionBinary64 x,downwell::MotionBinary64 y){return run_.emitBoxFlash(x,y,enemyRandom_);}
    bool effectsSucceeded()const{return run_.effectsSucceeded;}
    const downwell::WallBackgroundTiles<2048>& wallBackgroundTiles()const{return run_.wallBackground;}
    bool addWallBackground(downwell::MotionBinary64 x,downwell::MotionBinary64 y){return run_.addWallBackground(x,y);}
    bool emitSmokeBurst(downwell::MotionBinary64 x,downwell::MotionBinary64 y){
        return run_.emitSmokeBurst(x,y,enemyRandom_);
    }
    // Effect positions use original absolute room coordinates, not player offsets.
    bool emitFlashBurst(downwell::MotionBinary64 x,downwell::MotionBinary64 y) {
        return run_.emitFlashBurst(x,y,enemyRandom_);
    }
    unsigned enemyCount() const { return enemyCount_; }
    unsigned enemiesDefeated() const { return run_.kills; }
    unsigned gemCount() const;
    const GemState& gemAt(unsigned index) const;
    const GemState* findGem(std::uint32_t lifetimeId) const {
        if(lifetimeId)for(unsigned i=0;i<gemSlotEnd_;++i)
            if(gems_[i].active && gems_[i].lifetimeId==lifetimeId)return &gems_[i];
        return nullptr;
    }
    unsigned currency() const { return currencyAccount_.currency; }
    template<class Controls> auto currencyGainOperations(Controls controls){
        return downwell::CurrencyGainAccountOperations<Controls>(currencyAccount_,controls,&run_.gemStreakRise);
    }
    bool drawGemStreakRise(std::int32_t viewY,std::int32_t& worldY) {
        return run_.gemStreakRise.draw(currencyAccount_.streak,dead(),
            downwell::MotionBinary64::fromInteger(viewY),true,worldY);
    }
    unsigned gameGemTotal() const { return currencyAccount_.gameGem; }
    unsigned gemStreak() const { return currencyAccount_.streak; }
    unsigned gemStreakTimer() const { return currencyAccount_.timer; }
    int health() const { return health_; }
    bool subtractPlayerHealth(downwell::MotionBinary64 amount){
        if(!run_.effectsSucceeded)return false;
        return run_.effectsSucceeded=downwell::subtractPlayerHealth(health_,amount);
    }
    bool emitPlayerDamageNumber(downwell::MotionBinary64 amount,downwell::MotionBinary64 direction){
        if(!run_.effectsSucceeded)return false;
        downwell::MotionBinary64 x,y;
        if(!playerInstance_.lifetimeId||!downwell::ContactCoordinates::playerGlobals(player_.frameStartXFixed(),
            player_.frameStartYFixed(),240,176,x,y))return run_.effectsSucceeded=false;
        return run_.createDamageNumber(x,y,amount,direction)!=0;
    }
    unsigned ammoHudOffset() const { return run_.ammoHud.offset; }
    unsigned ammoMeterOffset() const { return run_.ammoHud.meterOffset; }
    const downwell::AmmoGaugeRows& ammoGauge() const { return run_.ammoGauge; }
    unsigned invulnerability() const { return invulnerability_; }
    bool dead() const { return death_.active; }
    unsigned deathFrames() const { return death_.frames; }
    bool deathMenuReady() const { return death_.menuReady; }
    const downwell::PlayerDeathSequence& deathSequence() const { return death_; }
    const downwell::RunGemStatistics& runGemStatistics() const { return run_.gems; }
    std::uint32_t runTimeTicks() const { return run_.clock.ticks; }
    const downwell::RunCombo& runCombo() const { return run_.combo; }
    bool grantConsumable(unsigned id) { return run_.consumables.grant(id, currencyAccount_); }
    bool grantUpgrade(unsigned id) {
        return run_.upgrades.grant(id, run_.consumables, currencyAccount_,
            player_, health_, run_.combo.healthReserve);
    }
    const downwell::UpgradeInventory& upgrades() const { return run_.upgrades; }
    int nextMuzzleOffset() const { return emitter_.leg; }
    const downwell::Consumables& consumables() const { return run_.consumables; }
    downwell::ShopWorld::Instance* registerShop(int x, int y) {
        return run_.createShop(x, y, [this](unsigned lo, unsigned hi) {
            return lo + enemyRandom_.irandomSmall(hi - lo);
        }, [this] { return enemyRandom_.chooseIndexSmall(3); }, health_);
    }
    const downwell::ShopWorld& shops() const { return run_.shops; }
    bool registerSideCameraLocker(int x, int y) { return run_.sideCameraLockers.create(x, y); }
    const downwell::OrdinaryTimeFields<64>& timeFields() const {return run_.timeFields;}
    template<class Audio> bool duckMusic(downwell::MusicDuck& caller,int duration,
        downwell::MotionBinary64 amount,Audio& audio){return run_.duckMusic(caller,duration,amount,audio);}
    bool createTimeFieldWithSourceId(std::uint32_t source,int x,int y){
        if(!run_.effectsSucceeded)return false;
        return run_.effectsSucceeded=run_.timeFields.createWithSourceId(source,x,y,enemyRandom_);
    }
    // Allocation belongs to the complete room creation stream. Request it
    // inside each admitted instance_create, not before immunity/Timeout/stop
    // checks or once for the whole player WITH traversal.
    template<class Caller,class Events,class Allocate> bool typicalPlayerDamage(Caller& caller,
        downwell::MotionBinary64 amount,downwell::MotionBinary64 horizontal,
        downwell::MotionBinary64 vertical,Events& events,Allocate allocate){
        if(!run_.effectsSucceeded)return false;
        return run_.effectsSucceeded=caller.execute(amount,horizontal,vertical,events,[&](int x,int y){
            std::uint32_t source=0;
            return allocate(source)&&createTimeFieldWithSourceId(source,x,y);
        });
    }
    bool bindTimeFieldLifecycle(downwell::OrdinaryTimeFields<64>::Binding binding){
        if(!run_.effectsSucceeded)return false;
        run_.effectsSucceeded=run_.timeFields.bindLifecycle(binding);return run_.effectsSucceeded;
    }
    bool cameraSideLocked() const { return cameraMotion_.sideLocked; }
    bool requestTimeStopCameraShake(){
        return requestCameraShake(2,2);
    }
    int cameraShakeAmount()const{return cameraMotion_.shakeAmount;}
    bool requestCameraShake(int amount,int duration){
        if(!run_.effectsSucceeded)return false;
        if(amount<0||amount>32767||duration<0)return run_.effectsSucceeded=false;
        if(!followCamera_)return run_.effectsSucceeded=false;
        cameraMotion_.requestShake(amount);
        if(cameraShakeAlarm_<duration)cameraShakeAlarm_=duration;
        return true;
    }
    std::uint64_t sideCameraDigest() const { return run_.sideCameraLockers.digest(cameraMotion_); }
    // Read-only rendering hint: entering a camera column may collide with a
    // locker at a different height. No RNG or live camera state is changed.
    int previewCameraViewY(int horizontalOffset) const {
        auto camera = cameraMotion_;
        camera.step((240L + horizontalOffset) * 256, 176L * 256 + player_.offsetYFixed(),
                    dead(), run_.sideCameraLockers.overlaps(camera));
        run_.sideCameraLockers.collide(camera);
        return camera.pixelY() - 142;
    }
    const downwell::GameplayGoalFields& goalFieldInstances()const{return goalFieldInstances_;}
    int timeFieldWindVoice()const{return run_.fieldWind.voice;}
    template<class Audio> bool contactGoalFieldWind(downwell::InstanceHandle key,
        downwell::MotionBinary64 volume,Audio& audio){
        return contactGoalFieldWind(key,run_.fieldWind,run_.overlapSounds[8],volume,audio);
    }
    template<class Audio> bool contactGoalFieldWind(downwell::InstanceHandle key,downwell::TimeFieldWind& wind,
        int& channel,downwell::MotionBinary64 volume,Audio& audio){
        return run_.effectsSucceeded=run_.effectsSucceeded&&
            goalFieldInstances_.contactWind(key,wind,enemyRandom_,channel,volume,audio);
    }
    bool retireMarkedGoalField(downwell::InstanceHandle h){
        return run_.effectsSucceeded=run_.effectsSucceeded&&goalFieldInstances_.retireMarked(h);
    }
    // Finish the supported room-entry Create sequence. The indexed owner
    // reconciles then sweeps in room order; unindexed fixtures own local cleanup.
    bool createRoomStartField(){
        if(!run_.effectsSucceeded)return false;
        unsigned sourceId=0;
        if(entryRooms_){
            downwell::PackedRoom room;
            if(!entryRooms_->findRoom(11,room))return run_.effectsSucceeded=false;
            for(unsigned i=0;i<room.instanceCount;++i){
                downwell::PackedRoomInstance instance;
                if(!entryRooms_->roomInstance(room,i,instance))return run_.effectsSucceeded=false;
                if(instance.objectId!=371)continue;
                if(sourceId||!instance.instanceId||instance.x!=240||instance.y!=192)return run_.effectsSucceeded=false;
                sourceId=instance.instanceId;
            }
            if(!sourceId)return run_.effectsSucceeded=false;
        }
        return createRoomStartFieldRecord(sourceId)&&finishRoomStartField();
    }
private:
    bool createRoomStartFieldRecord(unsigned sourceId){
        downwell::InstanceHandle h;
        if(!goalFieldInstances_.create(240,192,371,&h,sourceId))return run_.effectsSucceeded=false;
        if(!run_.createStartField(240,192,enemyRandom_))return false;
        if(!run_.startFieldPresent&&!goalFieldInstances_.requestDestroy(h))return run_.effectsSucceeded=false;
        return true;
    }
    bool finishRoomStartField(){
        if(!reconcilePhase(PhaseBoundary::roomEntry))return false;
        return run_.effectsSucceeded=goalFieldInstances_.cleanupMarked();
    }
public:
    bool bindGoalFieldLifecycle(downwell::GameplayGoalFields::Binding binding){
        return run_.effectsSucceeded=run_.effectsSucceeded&&goalFieldInstances_.bind(binding);
    }
    bool registerLevelExit(int y) {
        if(!run_.effectsSucceeded||!exitControllers_.create(306,240,y))return run_.effectsSucceeded=false;
        return run_.createLevelExit(480,y,enemyRandom_,cameraMotion_,
            [&](downwell::LevelExit::Effect effect,int x,int childY){
                return effect==downwell::LevelExit::Effect::roomEndSetter?
                    exitControllers_.create(74,x,childY):goalFieldInstances_.create(x,childY);
            });
    }
    const downwell::GameplayExitControllers& exitControllers()const{return exitControllers_;}
    bool bindExitControllerLifecycle(downwell::GameplayExitControllers::Binding binding){
        return run_.effectsSucceeded=run_.effectsSucceeded&&exitControllers_.bind(binding);
    }
    bool levelExitPresent() const { return run_.levelExitPresent; }
    const downwell::LevelExit& levelExit() const { return run_.levelExit; }
    const downwell::GoalTimeField& goalField() const { return run_.goalField; }
    const downwell::GoalTimeField* startTimeField() const { return run_.startFieldPresent?&run_.startField:nullptr; }
    unsigned goalFieldBirthBoundary() const { return run_.goalFieldBirthBoundary; }
    bool goalStopped() const { return run_.goalStopped; }
    bool exitDitherRequested() const { return run_.exitDitherRequested; }
    const downwell::ExitDither& exitDither() const { return run_.exitDither; }
    bool levelResultRequested() const { return run_.levelResultRequested; }
    // Scene owner must retire old DMA before entry; use this update path
    // instead of gameplay step until the next-room request is consumed.
    bool enterLevelClear(unsigned style, bool heartGoal) {
        if(!run_.levelResultRequested||run_.levelClearActive)return false;
        // Reject an unsupported selection before irreversibly retiring actors.
        if(style>4||2+run_.upgrades.levels[6]-(style==2?1:0)>=4)return false;
        if(!retireGameplayRoom())return false;
        if(resultInstance_){
            downwell::PackedRoom room;downwell::PackedRoomInstance authored{};
            if(!entryRooms_||!entryRooms_->findRoom(16,room)||room.instanceCount!=1||
               !entryRooms_->roomInstance(room,0,authored)||authored.objectId!=325||
               !resultInstance_->create(authored.instanceId,authored.x,authored.y)||
               !resultInstance_->position(authored.x,142))return run_.effectsSucceeded=false;
        }
        return run_.enterLevelClear(style, heartGoal, health_, player_, enemyRandom_);
    }
    template<class Emit>
    bool stepLevelClear(bool& confirm, bool leftPressed, bool rightPressed, bool& heartGoal, Emit emit) {
        if (!run_.effectsSucceeded || !run_.levelClearActive || run_.nextLevelRequested) return false;
        ++tick_;
        return run_.stepLevelClearPositioned(confirm, leftPressed, rightPressed, heartGoal,
            currencyAccount_, player_, health_, enemyRandom_, emit,[](int,int){return true;},
            [this](auto event){
                if(!resultRoomAlarms_){
                    run_.exitDither.advanceAlarm();return run_.levelClear.advanceAlarmsChecked(event);
                }
                return resultRoomAlarms_(resultRoomAlarmsContext_,
                    [](void* context,downwell::LevelClearSelection::Effect effect,unsigned value){
                        return (*static_cast<decltype(event)*>(context))(effect,value);
                    },&event);
            },[this](std::int32_t y){return !resultInstance_||resultInstance_->publishY(y);},[]{return true;});
    }
    bool levelClearActive() const { return run_.levelClearActive; }
    bool nextLevelRequested() const { return run_.nextLevelRequested; }
    // Retire the old display first. This resets only room-local runtime state;
    // the owner must rebuild terrain and rebind generation/collision callbacks.
    bool prepareNextAreaOneLevel(const downwell::Input& input, unsigned initialRingOrigin);
    template<class Builder>
    bool initializeAreaOneBuilder(Builder& builder, int x, int y, unsigned style = 0) {
        return resetAreaOneBuilder(builder,x,y,style)&&createRoomStartField();
    }
    template<class Builder>
    bool resetAreaOneBuilder(Builder& builder, int x, int y, unsigned style = 0) {
        if(!run_.effectsSucceeded)return false;
        if (run_.stage.area != 1 || run_.stage.level < 1 || run_.stage.level > 3 || style > 4) return false;
        enemyRandom_.prepareLookahead(randomLookahead_);
        downwell::AreaOneBuilder::Setup setup;
        setup.level = unsigned(run_.stage.level);
        setup.style = style;
        setup.member = run_.upgrades.levels[14] != 0;
        builder.resetRoom(x, y, enemyRandom_, setup);
        return true;
    }
    const downwell::LevelClearController& levelClear() const { return run_.levelClear; }
    const downwell::LevelClearPresentation::Frame& levelClearFrame() const { return run_.levelClearFrame; }
    const downwell::RunStage& runStage() const { return run_.stage; }
    int healthMaximum() const { return run_.combo.healthReserve.maximum; }
    const downwell::HealthReserve& healthReserve() const { return run_.combo.healthReserve; }
    const downwell::DeathResults& deathResults() const { return run_.results; }
    bool playerVisible() const { return downwell::playerDamageVisible(invulnerability_); }
    bool bulletActive() const { return bulletCount() != 0; }
    int bulletX() const;
    int bulletY() const;
    int cameraOffsetY() const { return cameraPixelY_; }
    int cameraOffsetX() const { return cameraPixelX_; }
    bool breakableActiveAt(int x, int y) const;
    unsigned blocksDestroyed() const { return blocksDestroyed_; }
    std::uint64_t checkpoint() const { return checkpoint_; }
    std::uint64_t trajectoryDigest() const { return trajectory_.value(); }

private:
    EnemyLifecycleBinding enemyLifecycle_;
    downwell::ScriptEffectRuntimeBinding scriptEffectsBinding_;
    downwell::ScriptEffectRuntimeBinding debrisBinding_;
    bool (*debrisAlarm_)(void*,downwell::GameplayDebrisInstances&,downwell::InstanceHandle,unsigned)=nullptr;
    bool (*roomAlarms_)(void*)=nullptr;
    bool (*resultRoomAlarms_)(void*,ResultAlarmEmit,void*)=nullptr;
    void* resultRoomAlarmsContext_=nullptr;
    void* roomAlarmsContext_=nullptr;
    downwell::DebrisEffects<downwell::debrisEffectCapacity> ownedDebrisEffects_;
    DebrisStepOrder debrisStepOrder_;
    downwell::DebrisEffects<downwell::debrisEffectCapacity>::Phase debrisStepPhase_{};
    bool (*debrisOrderCapture_)(void*,DebrisStepOrder&)=nullptr;
    void* debrisOrderContext_=nullptr;
    bool (*debrisStepDispatch_)(void*,downwell::InstanceHandle)=nullptr;
    void* debrisStepContext_=nullptr;
    bool clearRoomDebris(){
        if(debrisInstances_&&debrisInstances_->size()&&!debrisBinding_.clearRoom())return run_.effectsSucceeded=false;
        debrisBinding_={};return true;
    }
    downwell::ScriptSpawnEffects<downwell::scriptSpawnEffectCapacity> ownedScriptEffects_;
    bool clearRoomScriptEffects(){
        if(scriptSpawnInstances_&&scriptSpawnInstances_->size()&&
            (!run_.effectsSucceeded||!scriptEffectsBinding_.clearRoom()))return run_.effectsSucceeded=false;
        scriptEffectsBinding_={};return true;
    }
    bool (*roomAnimation_)(void*)=nullptr;
    void* roomAnimationContext_=nullptr;
    bool (*registeredActorEndStep_)(void*,long)=nullptr;
    bool (*sourceBulletDispatch_)(void*,downwell::InstanceHandle,downwell::MachineGunSourceMotion&)=nullptr;
    bool (*skeletonBoneSteps_)(void*,int)=nullptr;
    void* skeletonBoneContext_=nullptr;
    void* sourceBulletContext_=nullptr;
    bool registeredOrdinaryEndStep_=false;
    void* registeredActorEndStepContext_=nullptr;
    bool (*collisionPass_)(void*)=nullptr;
    void* collisionPassContext_=nullptr;
    bool collisionPassExecuting_=false;
    RoomActorDraw roomActorDraw_=nullptr;
    RoomActorDraw roomActorReadOnlyDraw_=nullptr;
    const void* roomActorDrawContext_=nullptr;
    bool (*scriptAnimation_)(void*,downwell::InstanceHandle)=nullptr;
    void* scriptAnimationContext_=nullptr;
    bool (*phaseBoundary_)(void*,PhaseBoundary)=nullptr;
    void* phaseBoundaryContext_=nullptr;
    bool reconcilePhase(PhaseBoundary phase){
        if(!run_.effectsSucceeded)return false;
        if(phaseBoundary_&&!phaseBoundary_(phaseBoundaryContext_,phase)){
            run_.effectsSucceeded=false;return false;
        }
        return true;
    }
    BreakableLifecycleBinding breakableLifecycle_;
    PlayerLifecycleBinding playerLifecycle_;
    downwell::GameplayTerrainInstances* terrainInstances_=nullptr;
    NativeTerrain* nativeTerrainInstances_=nullptr;
    NativeTerrain::Binding nativeTerrainBinding_{};
    downwell::GameplayScriptSpawnInstances* scriptSpawnInstances_=nullptr;
    RegisteredExpiry* registeredExpiry_=nullptr;
    RegisteredSmoke* registeredSmoke_=nullptr;
    RegisteredBlasts* registeredBlasts_=nullptr;
    RegisteredCorpse* registeredCorpses_=nullptr;
    ConsumptionParticles* consumptionParticles_=nullptr;
    LimboShards* limboShards_=nullptr;
    RegisteredFlash* registeredFlash_=nullptr;
    downwell::GameplayDebrisInstances* debrisInstances_=nullptr;
    downwell::NativeTimeChangeRegistration* timeChangeInstances_=nullptr;
    downwell::NativeTimeShardRegistration* timeShardInstances_=nullptr;
    downwell::TimeShardEffects<64>* timeShardEffects_=nullptr;
    bool clearRoomTimeShards();
    downwell::GameplayEntryDithers* entryDithers_=nullptr;
    downwell::GameplayDitherAbove* ditherAbove_=nullptr;
    downwell::GameplayBuilderController* builderController_=nullptr;
    downwell::GameplayPlayerControllers* playerControllers_=nullptr;
    RunControllers* runControllers_=nullptr;
    downwell::GameplayResultInstance* resultInstance_=nullptr;
    LevelNumbers* levelNumbers_=nullptr;
    int* controllerRoomSpeed_=nullptr; // Same global as timeChangeSpeed_, but persists beyond effect cleanup.
    bool (*playerDamageSlowdown_)(void*)=nullptr;
    void* playerDamageSlowdownContext_=nullptr;
    bool playerChildrenStarted_=false;
    bool (*playerChildCreate_)(void*,GameplayWorld&)=nullptr;
    void* playerChildCreateContext_=nullptr;
    unsigned droneNumber_=0;
    bool ditherLowSpec_=false,ditherEnding_=false;
    DroneBullets* droneBullets_=nullptr;
    Bones* bones_=nullptr;
    Skeletons* skeletons_=nullptr;
    Blood* blood_=nullptr;
    downwell::TimeChangeEffects<8>* timeChangeEffects_=nullptr;
    int* timeChangeSpeed_=nullptr; // Borrowed source room-speed global, not PAL catch-up.
    bool clearRoomTimeChanges();
    bool roomTerrainLoaded_=false;
    downwell::RoomEntryViews roomEntryViews_;
    // Borrowed resource pack survives in-place run recreation. Its initial
    // view cache must be available before any player birth callbacks.
    const downwell::RoomPack* entryRooms_=nullptr;
    PlayerInstance playerInstance_;
    bool (*playerTimeTransition_)(void*,const PlayerInstance&,const PlayerMotion&,int,int)=nullptr;
    void* playerTimeContext_=nullptr;
    PlayerBirthTiming playerBirthTiming_=PlayerBirthTiming::construction;
    bool playerBirthPending_=false;
    std::uint32_t nextPlayerLifetimeId_=1;
    bool birthPlayerOwnership();
    bool retirePlayerOwnership();
    bool retireGameplayRoom();
    bool publishPlayerPosition(int x,int y);
    bool retireBreakable(unsigned index);
    bool clearBreakableOwnership();
    void prepareBreakableSlotsForReuse();
    std::uint32_t nextBreakableLifetimeId_ = 1;
    bool publishEnemyActivation(const EnemyState& enemy, bool active);
    bool publishEnemyImage(const EnemyState& enemy,unsigned phase);
    bool publishEnemyTransform(const EnemyState& enemy,downwell::RunnerTransformField field,int value);
    bool retireEnemyOwnership(EnemyState& enemy);
    bool clearEnemyOwnership();
    bool retireBulletOwnership(BulletState& bullet);
    bool stepBulletEffect(BulletState& bullet,long generationCameraY,bool& bulletChanged);
    bool clearBulletOwnership();
    // Normal gameplay reads camMain's published pixel position. Preserve the
    // synthetic replay fixture's explicit camera track when following is off.
    long viewCameraYQ8() const { return followCamera_ ? cameraPixelY_ * 256L : cameraY_; }
    void initializeMainCameraFrame();
    void applyFrogAlarm(EnemyState& enemy, const downwell::FrogAlarmEffect& effect);
    void advanceEnemyAlarms();
    void updateCrawler(EnemyState& enemy, bool alarm, int cameraX, long cameraYQ8);
    GenerationStep generationStep_ = nullptr;
    void* generationContext_ = nullptr;
    bool generationSucceeded_ = true;

    PlayerMotion player_;
    unsigned tick_;
    unsigned ringOrigin_;
    bool followCamera_;
    long cameraY_;
    downwell::MainCameraMotion cameraMotion_;
    int cameraShakeAlarm_=-1; // camMain's only alarm; global identity remains separate.
    int cameraPixelY_;
    int cameraPixelX_;
    // Event-phase snapshot in absolute room coordinates, not the camera
    // target being updated later in this tick. Normal following is pixelized.
    int boneViewLeft_=160;
    long boneViewTopQ8_=34L*256;
    int activationY_;
    BulletState bullets_[bulletCapacity];
    bool bulletIdentitySlots_[bulletCapacity]{};
    BulletLifecycleBinding bulletLifecycle_{};
    std::uint32_t nextBulletLifetimeId_ = 1;
    std::uint32_t nextEnemyLifetimeId_ = 1, nextGemLifetimeId_ = 1;
    unsigned bulletSlotEnd_ = 0, gemSlotEnd_ = 0;
    EnemyState enemies_[enemyCapacity];
    EnemyStepOrder enemyStepOrder_; // Owner-resident: never a 68000 stack snapshot.
    std::uint32_t (*enemyOrderRevision_)(void*)=nullptr;
    std::uint32_t enemyOrderCapturedRevision_=0;
    bool enemyOrderValid_=false;
    bool (*enemyOrderCapture_)(void*,EnemyStepOrder&)=nullptr;
    void* enemyOrderContext_=nullptr;
    bool captureEnemyStepOrder();
    // Index + 1; zero means no lifetime owns this slot. This is lookup only,
    // never traversal order. Update moved records during stable compaction.
    static_assert(enemyCapacity<=65535,"enemy index must fit its slot directory");
    unsigned short enemySlotIndex_[enemyCapacity]{};
    GemState gems_[gemCapacity];
    unsigned enemyCount_;
    downwell::GameMakerRandom::Lookahead randomLookahead_;
    downwell::GameMakerRandom enemyRandom_;
    int health_;
    unsigned invulnerability_;
    downwell::PlayerDeathSequence death_;
    downwell::RunSession run_;
    downwell::BoxStepDispatch boxStepDispatch_;
    downwell::GameplayGoalFields goalFieldInstances_;
    downwell::GameplayExitControllers exitControllers_;
    downwell::MachineGunEmitter emitter_;
    downwell::CurrencyAccount currencyAccount_;
    Currency* currencyInstances_=nullptr;
    SolidResolver solidResolver_;
    SegmentResolver segmentResolver_ = nullptr;
    SolidResolver maskResolver_ = nullptr;
    WallContactResolver wallContactResolver_ = nullptr;
    PlatformContactResolver platformContactResolver_ = nullptr;
    bool currencyOnlyPlatforms_=true,currencyEmptyWater_=true;
    CurrencyStepOrder currencyStepOrder_;
    bool (*currencyOrderCapture_)(void*,CurrencyStepOrder&)=nullptr;
    void* currencyOrderContext_=nullptr;
    bool (*currencyStepEligible_)(void*,downwell::InstanceHandle,bool&)=nullptr;
    bool (*currencyEventDispatch_)(void*,downwell::InstanceHandle,CurrencyPhase,unsigned)=nullptr;
    void* currencyEventContext_=nullptr;
    void* solidResolverContext_;
    PlayerMotion::LandingResolver landingResolver_ = nullptr;
    void* landingResolverContext_ = nullptr;
    Breakable breakables_[breakableCapacity];
    // Cold dynamic script variables: do not widen the hot Breakable stride or
    // copy them into ordinary Step snapshots. Reset at slot-reuse boundaries.
    downwell::CurrencyRewardFields boxRewards_[breakableCapacity]{};
    unsigned breakableCount_;
    bool breakableIndexReady_ = false;
    std::uint16_t breakableIndex_[64 * 512];
    int indexedBreakable(int x, int y) const;
    unsigned blocksDestroyed_;
    std::uint64_t checkpoint_;
    downwell::StateDigest trajectory_;

    void spawnReward(const EnemyState& enemy, bool attracted);
};

#endif
