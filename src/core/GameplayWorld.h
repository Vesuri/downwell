#ifndef DOWNWELL_CORE_GAMEPLAY_WORLD_H
#define DOWNWELL_CORE_GAMEPLAY_WORLD_H

#include "PlayerMotion.h"
#include "RunAudioBinding.h"
#include "EncounterInput.h"
#include "GameplayFrame.h"
#include "PreparedRoomTerrain.h"
#include "ScriptEffectRuntimeBinding.h"
#include "BoxStepDispatch.h"
#include "EyefishMotion.h"
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
#include "core/GameplayScattered.h"
#include "core/GameplaySpawners.h"
#include "core/GameplayWhiteJellies.h"
#include "core/GameplayRedBalls.h"
#include "core/GameplayShopInstances.h"
#include "core/CompactShopSpeech.h"
#include "core/MovingFxInstances.h"
#include "core/RegisteredScatteredContact.h"
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
#include "core/PortableAtan.h"
#include "core/RegisteredSkeletonThrow.h"
#include "core/RegisteredSkeletonStomp.h"
#include "core/BoneViewRetirement.h"
#include "core/GameplayScriptSpawnInstances.h"
#include "core/CorpseExpiryInstances.h"
#include "core/CorpseInstances.h"
#include "core/CompactUpgrades.h"
#include "core/CorpseEffects.h"
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
#include "core/PixelCollisionMath.h"
#include "core/SolidCellGrid.h"
#include "core/BatHitState.h"
#include "core/BatBehavior.h"
#include "core/InstanceHandle.h"
#include "core/InstanceLifecycle.h"
#include "core/GameplayTerrainInstances.h"
#include "core/NativeTerrainLifetimes.h"
#include "core/RunnerStepSnapshot.h"
#include "core/CompactRoomEffects.h"
#include "core/CompactScenery.h"
#include "core/CompactGuns.h"
#include "core/CompactHud.h"
#include "core/RangedShot.h"
#include "core/GunTable.h"

#include <cstdint>

enum {
    gameplayWorldBulletCapacity = downwell::gameplayCompactBulletCapacity,
    gameplayWorldDestroyedCapacity = downwell::gameplayRepaintCapacity,
    gameplayWorldEnemyCapacity = 64,
    gameplayWorldGemCapacity = downwell::gameplayGemCapacity
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
        // W1.4 compact guns: kind 0 is the retained usual-gun trajectory,
        // 1 bulletRanged (261), 2 bulletLaser (260). Stats are bulPropertyCheck's
        // birth copy (bdmg, sprite), not the live gun.
        enum Kind : unsigned char {legacy=0,ranged=1,laser=2};
        unsigned char kind=legacy;
        int damage=9,sprite=459;
        downwell::RangedShot shot{};
        // bulletLaser: hitIgnoreCheck memory (enemy lifetimes) and wallMet.
        std::uint32_t laserHits[4]{};
        unsigned char laserHitCount=0;
        bool wallMet=false;
        // Offline frame ID: the nearest quarter turn of image_angle (bDir)
        // in the high byte; bulletLaser never sets image_angle. The usual
        // gun's 267..273 spread is pixel-proven against the 270 page; other
        // guns' turns are nearest-page approximations (W1.4 note).
        unsigned drawFrame()const{
            if(kind==legacy)return 768+animation.frame();
            if(kind==laser)return shot.frame();
            return (((shot.direction+0x20000000u)>>30)&3u)*256+shot.frame();
        }
    };
    using BulletLifecycleBinding=downwell::RecordLifecycleBinding<BulletState>;
    // How the crawler, snail and walker records hold their coordinates. The
    // reference build keeps the source scripts' single fifths-of-Q8 long.
#ifdef DOWNWELL_REFERENCE_ENEMY_UPDATES
    using EnemyMotionMath=downwell::FixedCollisionMath;
#else
    using EnemyMotionMath=downwell::PixelCollisionMath;
#endif
    struct EnemyState {
        // Family-local lifetime key; never a recovered Windows instance ID.
        std::uint32_t lifetimeId = 0;
        std::uint32_t sourceId = 0;
        // Stable ownership slot, independent of the compacted traversal index.
        unsigned short identitySlot = 0xffffu;
        // The tick this record last entered its room layer (Create while
        // active, or activation). Runner layers insert at the head, so an
        // unowned build draws same-depth enemies newest entry first.
        std::uint32_t activatedTick = 0;
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
        downwell::BasicCrawlerMotion<EnemyMotionMath> crawler;
        downwell::BasicSnailMotion<EnemyMotionMath> snail;
        downwell::SnailHitState snailHit;
        downwell::BatHitState batHit;
        unsigned short batDirectionFraction;
        downwell::BatActivity batActivity;
        downwell::BasicWalkingEnemyMotion<EnemyMotionMath> walker;
        downwell::EyefishMotion eyefish; // enmEyefish 114's xx, yy and xsp
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
        // Step left image_speed at 0 under pTimeStop: the next animation
        // phase holds the frame.
        bool frozen=false;
        // Latest scrBulletImpact bdmg; takenImpact is bdmg/5 (subparentEnemy
        // Create clears it), read by scrDeadBody.
        int takenImpactDamage=0;
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
        bool frozen=false; // objCurrency's stopped branch set image_speed 0
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
#ifdef DOWNWELL_TERRAIN_SOURCE_RECORDS
    using NativeTerrain=downwell::NativeTerrainLifetimes<downwell::GameplayTerrainInstances::capacity,true>;
#else
    using NativeTerrain=downwell::NativeTerrainLifetimes<downwell::GameplayTerrainInstances::capacity>;
#endif
    const NativeTerrain* nativeTerrainInstances()const{return nativeTerrainInstances_;}
    NativeTerrain* nativeTerrainInstances(){return nativeTerrainInstances_;}
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
    template<class Producer> bool constructNativeTerrainRun(Producer producer){
        return nativeTerrainInstances_&&nativeTerrainInstances_->constructRun(
            downwell::terrainTileForArea(run_.stage.area),run_.effectsSucceeded,producer);
    }
    bool constructNativeTerrainPrepared(NativeTerrain::PreparedCells cells){
        return nativeTerrainInstances_&&nativeTerrainInstances_->constructPrepared(
            downwell::terrainTileForArea(run_.stage.area),run_.effectsSucceeded,cells);
    }
    // The tile NativeTerrainLifetimes::reserveLeased/publishLeased take.
    int nativeTerrainTile()const{return downwell::terrainTileForArea(run_.stage.area);}
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
#ifdef DOWNWELL_WORLD_ONE_ONLY
        (void)instances;return false;
#else
        if(limboShards_||!instances.valid()||instances.size())return false;
        limboShards_=&instances;return true;
#endif
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
    const downwell::GameplayScattered* scattered()const{return scattered_;}
    const downwell::GameplaySpawners* spawners()const{return spawners_;}
    const downwell::GameplayWhiteJellies* whiteJellies()const{return whiteJellies_;}
    const downwell::GameplayRedBalls* redBalls()const{return redBalls_;}
    const downwell::GameplayShopInstances* shopInstances()const{return shopInstances_;}
    bool bindShopInstances(downwell::GameplayShopInstances& records){
        if(shopInstances_||!records.valid()||records.size())return false;
        shopInstances_=&records;return true;
    }
    bool advanceShopRoomAlarm(downwell::InstanceHandle key,unsigned number){
        if(!run_.effectsSucceeded||!shopInstances_||!shopInstances_->valid())return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=shopInstances_->advanceAlarm(key,number,[&](unsigned lo,unsigned hi){
            return lo+enemyRandom_.irandomSmall(hi-lo);
        })&&run_.effectsSucceeded;
    }
    template<class Environment> bool stepShopKeeperWithWorld(downwell::InstanceHandle key,Environment& environment){
        if(!run_.effectsSucceeded||!shopInstances_||!shopInstances_->valid())return run_.effectsSucceeded=false;
        struct Inputs {
            GameplayWorld& world;Environment& external;
            bool playerX(downwell::MotionBinary64& x){
                downwell::MotionBinary64 y;
                return downwell::ContactCoordinates::playerGlobals(world.player_.frameStartXFixed(),
                    world.player_.frameStartYFixed(),240,176,x,y);
            }
            bool area(unsigned& value){value=unsigned(world.run_.stage.area);return true;}
            bool criminal(bool& value){return external.criminal(value);}
            bool createEnemy(int object,downwell::MotionBinary64 x,downwell::MotionBinary64 y){return external.createEnemy(object,x,y);}
        } inputs{*this,environment};
        return run_.effectsSucceeded=shopInstances_->stepKeeper(key,inputs,[&](unsigned lo,unsigned hi){
            return lo+enemyRandom_.irandomSmall(hi-lo);
        })&&run_.effectsSucceeded;
    }
    template<class Ops> bool drawRegisteredRedBall(downwell::InstanceHandle key,Ops& ops)const{
        return redBalls_&&redBalls_->dispatch(key,[&](auto& actor){return actor.draw(ops);});
    }
    bool bindRedBalls(downwell::GameplayRedBalls& records){
        if(redBalls_||!records.valid()||records.size())return false;
        redBalls_=&records;return true;
    }
    using MovingEffects=downwell::MovingFxInstances<32>;
    const MovingEffects* movingEffects()const{return movingEffects_;}
    bool bindMovingEffects(MovingEffects& records){
        if(movingEffects_||!records.valid()||records.size())return false;
        movingEffects_=&records;return true;
    }
    bool bindWhiteJellies(downwell::GameplayWhiteJellies& records){
        if(whiteJellies_||!records.valid()||records.size())return false;
        whiteJellies_=&records;return true;
    }
    bool bindSpawners(downwell::GameplaySpawners& records){
        if(spawners_||!records.valid()||records.size())return false;
        spawners_=&records;return true;
    }
    bool bindScattered(downwell::GameplayScattered& records){
        if(scattered_||!records.valid()||records.size())return false;
        scattered_=&records;return true;
    }
    template<class Allocate,class Delay,class Player,class Audio>
    bool collideScatteredPlayerWithBlast(downwell::InstanceHandle key,Allocate& allocate,Delay delay,Player& player,
        downwell::SoundPlayEnvironment& environment,Audio& audio){
        if(!run_.effectsSucceeded||!scattered_)return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=scattered_->dispatch(key,[&](auto& state){
            auto blast=[&]{
                unsigned id=0;if(!allocate(id))return false;
                const auto* r=scattered_->find(key);downwell::InstanceHandle child;
                return r&&createBlastWithSourceId(id,r->x.promoted(),r->y.promoted(),delay,child);
            };
            auto events=downwell::registeredScatteredContact(*this,state,player,blast,state.sound,environment,audio,run_.effectsSucceeded);
            return scattered_->playerContact(key,enemyRandom_,events);
        })&&run_.effectsSucceeded;
    }
    // Room dispatch supplies the current view/room and the shared numeric-ID
    // cursor. These are real registered emissions, not the legacy effect pools.
    template<class Allocate,class Queries,class Audio>
    bool stepRegisteredScattered(downwell::InstanceHandle key,const downwell::ObjectPack& objects,
        int room,downwell::MotionBinary64 viewY,Allocate& allocate,Queries& queries,
        downwell::SoundPlayEnvironment& environment,Audio& audio){
        if(!run_.effectsSucceeded||!scattered_||!scattered_->find(key))return run_.effectsSucceeded=false;
        struct Effects {
            GameplayWorld& world;downwell::GameplayScattered& store;downwell::InstanceHandle key;
            const downwell::ObjectPack& objects;int room;downwell::MotionBinary64 view;
            Allocate& allocate;Queries& queries;downwell::SoundPlayEnvironment& environment;Audio& audio;
            downwell::InstanceHandle debris{};
            bool outOfView(){
                const auto* r=store.find(key);if(!r)return false;
                auto top=view,bottom=view;int order;
                if(!top.add(downwell::MotionBinary64::fromInteger(-160))||
                   !downwell::compareRunnerReal(r->y.promoted(),top,order))return false;
                bool outside=order<0;
                if(!outside){
                    if(!bottom.add(downwell::MotionBinary64::fromInteger(3000))||
                       !downwell::compareRunnerReal(r->y.promoted(),bottom,order))return false;
                    outside=order>0;
                }
                return !outside||room!=11||store.markDestroyed(key);
            }
            bool currency(int amount){
                return world.spawnRegisteredCurrency(objects,[&](auto& out){
                    const auto* r=store.find(key);if(!r)return false;
                    out.x=r->x.promoted();out.y=r->y.promoted();out.objectIndex=289;
                    out.stomped=downwell::MotionBinary64::fromInteger(r->stomped);
                    out.moniAmount=r->reward.moniAmount;out.atrState=r->reward.atrState;
                    out.myGem=r->reward.myGem;return true;
                },[&](auto value){return store.dispatch(key,[&](auto& r){r.reward.moniAmount=value;return true;});},
                [&](bool value){return store.dispatch(key,[&](auto& r){r.reward.atrState=value;return true;});},
                [&](std::uint32_t value){return store.dispatch(key,[&](auto& r){r.reward.myGem=value;return true;});},
                downwell::MotionBinary64::fromInteger(amount),[&](std::uint32_t& id){return allocate(id);},queries);
            }
            bool attractGem(){
                const auto* r=store.find(key);if(!r||!world.currencyInstances_)return false;
                return world.currencyInstances_->dispatch(world.currencyInstances_->handleForSourceId(r->reward.myGem),
                    [](auto& state,bool&){state.attracted=true;return true;});
            }
            bool createDebris(){
                std::uint32_t id=0;if(!allocate(id))return false;
                const auto* r=store.find(key);
                return r&&world.spawnDebrisWithSourceId(id,r->x.promoted(),r->y.promoted(),world.run_.stage.area,debris);
            }
            bool debrisYSpeed(downwell::MotionBinary64 value){
                const auto* r=world.debrisInstances_?world.debrisInstances_->find(debris):nullptr;
                auto* effect=r?world.ownedDebrisEffects_.get(r->lifetimeId):nullptr;
                if(!effect||r->effect!=effect||!value.valid())return false;
                effect->ysp=value;return true;
            }
            bool sound(int id,int priority,int loop,int cut){
                return store.dispatch(key,[&](auto& r){
                    int voice;environment.y=r.y.promoted();
                    return downwell::SoundPlayScript::plain(r.sound,environment,audio,id,priority,loop!=0,cut!=0,
                        voice,world.run_.effectsSucceeded);
                });
            }
            bool sprite(int id){return store.sprite(key,id);}
        } effects{*this,*scattered_,key,objects,room,viewY,allocate,queries,environment,audio};
        return run_.effectsSucceeded=scattered_->step(key,enemyRandom_,effects)&&run_.effectsSucceeded;
    }
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
    bool sourceFallDepth(downwell::MotionBinary64& value)const{
        if(!run_.effectsSucceeded||!playerChildrenStarted_)return false;
        value=run_.fallDepth;return true;
    }
    bool publishPlayerFallDepth(){
        if(!run_.effectsSucceeded||!playerChildrenStarted_||!playerControllers_)return run_.effectsSucceeded=false;
        const downwell::GameplayPlayerControllers::Record* meter=nullptr;
        for(unsigned i=0;i<downwell::GameplayPlayerControllers::capacity;++i){
            const auto* r=playerControllers_->find(playerControllers_->handle(i));
            if(!r||r->objectId!=66||!r->active||r->destroyRequested)continue;
            if(meter)return run_.effectsSucceeded=false;
            meter=r;
        }
        downwell::MotionBinary64 x,y;
        if(!meter||!downwell::ContactCoordinates::playerGlobals(player_.offsetXFixed(),player_.offsetYFixed(),240,176,x,y))
            return run_.effectsSucceeded=false;
        return run_.publishFallDepth(y,meter->y);
    }
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
    // Before any generated Create except callback-free terrain publication:
    // lands newline rows the builder controller deferred.
    void publishPendingBuilderPosition(){if(builderController_)builderController_->publishPosition();}
    void configureDitherAboveEnvironment(bool lowSpec,bool ending){ditherLowSpec_=lowSpec;ditherEnding_=ending;}
    bool adoptDitherAbove(const downwell::GameplayDitherAbove::Snapshot& snapshot){
        if(!run_.effectsSucceeded||!ditherAbove_||!ditherAbove_->adopt(snapshot))return run_.effectsSucceeded=false;
        configureDitherAboveEnvironment(snapshot.lowSpec,snapshot.ending);return true;
    }
    bool bindDitherAbove(downwell::GameplayDitherAbove& actor){
        if(ditherAbove_||!actor.valid()||actor.handle().generation)return false;
        ditherAbove_=&actor;return true;
    }
    // A build without a collision owner keeps object 61 in the world.
    bool bindOwnedDitherAbove(){return bindDitherAbove(ownedDitherAbove_);}
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
#ifdef DOWNWELL_WORLD_ONE_ONLY
        (void)skeletons;return false;
#else
        if(skeletons_||!skeletons.valid()||skeletons.size())return false;
        skeletons_=&skeletons;return true;
#endif
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
    template<class Ops> bool collideWhiteJellyEnemy(downwell::InstanceHandle key,Ops& ops){
        if(!run_.effectsSucceeded||!whiteJellies_||!whiteJellies_->valid())return run_.effectsSucceeded=false;
        const bool ok=whiteJellies_->dispatch(key,[&](auto& actor){return actor.enemyContact(enemyRandom_,ops);});
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    template<class Ops> bool collideRedBallEnemy(downwell::InstanceHandle key,Ops& ops){
        if(!run_.effectsSucceeded||!redBalls_||!redBalls_->valid())return run_.effectsSucceeded=false;
        const bool ok=redBalls_->dispatch(key,[&](auto& actor){return actor.enemyContact(enemyRandom_,ops);});
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    template<class Ops> bool stepWhiteJellyRoomEffect(downwell::InstanceHandle key,bool stopped,Ops& ops){
        if(!run_.effectsSucceeded||!whiteJellies_||!whiteJellies_->valid())return run_.effectsSucceeded=false;
        const bool ok=whiteJellies_->step(key,stopped,enemyRandom_,ops);
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    template<class Ops> bool stepRedBallRoomEffect(downwell::InstanceHandle key,bool stopped,Ops& ops){
        if(!run_.effectsSucceeded||!redBalls_||!redBalls_->valid())return run_.effectsSucceeded=false;
        const bool ok=redBalls_->step(key,stopped,enemyRandom_,ops);
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    template<class Execute,class Tick> bool withWhiteJellyEffectAlarms(Execute execute,Tick tick){
        if(!run_.effectsSucceeded||whiteJellyAlarmDispatch_)return run_.effectsSucceeded=false;
        whiteJellyAlarmContext_=&execute;
        whiteJellyAlarmDispatch_=[](void* p,downwell::InstanceHandle key,
            downwell::GameplayWhiteJellies::Record& actor,downwell::GameMakerRandom& random){
            return (*static_cast<Execute*>(p))(key,actor,random);
        };
        const bool ok=tick();whiteJellyAlarmDispatch_=nullptr;whiteJellyAlarmContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    bool advanceWhiteJellyRoomAlarm(downwell::InstanceHandle key,unsigned alarm){
        if(!run_.effectsSucceeded||!whiteJellies_)return run_.effectsSucceeded=false;
        const bool ok=whiteJellies_->advanceAlarm(key,alarm,[&](auto& actor){
            return whiteJellyAlarmDispatch_&&whiteJellyAlarmDispatch_(whiteJellyAlarmContext_,key,actor,enemyRandom_);
        });
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    template<class MakeOps> bool createWhiteJellyWithSourceId(unsigned source,
        downwell::RunnerImageValue x,downwell::RunnerImageValue y,MakeOps makeOps,downwell::InstanceHandle& key){
        key={};
        if(!run_.effectsSucceeded||!whiteJellies_||!whiteJellies_->valid())return run_.effectsSucceeded=false;
        const bool ok=whiteJellies_->create(source,x,y,enemyRandom_,makeOps,key);
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    template<class MakeOps> bool createRedBallWithSourceId(unsigned source,
        downwell::RunnerImageValue x,downwell::RunnerImageValue y,MakeOps makeOps,downwell::InstanceHandle& key){
        key={};
        if(!run_.effectsSucceeded||!redBalls_||!redBalls_->valid())return run_.effectsSucceeded=false;
        const bool ok=redBalls_->create(source,x,y,enemyRandom_,makeOps,key);
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    template<class Execute,class Tick> bool withRedBallEffectAlarms(Execute execute,Tick tick){
        if(!run_.effectsSucceeded||redBallAlarmDispatch_)return run_.effectsSucceeded=false;
        redBallAlarmContext_=&execute;
        redBallAlarmDispatch_=[](void* p,downwell::InstanceHandle key,
            downwell::GameplayRedBalls::Record& actor,downwell::GameMakerRandom& random){
            return (*static_cast<Execute*>(p))(key,actor,random);
        };
        const bool ok=tick();redBallAlarmDispatch_=nullptr;redBallAlarmContext_=nullptr;
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    bool advanceRedBallRoomAlarm(downwell::InstanceHandle key,unsigned alarm){
        if(!run_.effectsSucceeded||!redBalls_)return run_.effectsSucceeded=false;
        const bool ok=redBalls_->advanceAlarm(key,alarm,[&](auto& actor){
            return redBallAlarmDispatch_&&redBallAlarmDispatch_(redBallAlarmContext_,key,actor,enemyRandom_);
        });
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    template<class Ops> bool stepSpawnerRoomEffect(downwell::InstanceHandle key,Ops& ops){
        if(!run_.effectsSucceeded||!spawners_||!spawners_->valid())return run_.effectsSucceeded=false;
        const bool ok=spawners_->step(key,enemyRandom_,ops);
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
    enum class DrawActor { enemy, bullet, scriptEffect, debris, player, entryDither, bone, skeleton, ditherAbove, timeShard, corpseExpiry, smoke, corpse, consumptionBlood, levelNumber, limboShard, flashBall, blast, scattered, whiteJelly, movingFx, redBall, shop };
    template<class Visit> bool visitRegisteredEnemyShades(Visit visit)const{
        if((!bones_||!bones_->size())&&(!skeletons_||!skeletons_->size())&&(!whiteJellies_||!whiteJellies_->size())&&(!redBalls_||!redBalls_->size()))return true;
        return visitRoomActorsReadOnly([&](DrawActor kind,downwell::InstanceHandle key,auto){
            if(kind==DrawActor::bone){const auto* r=bones_->find(key);return r&&visit(r->x.promoted(),r->y.promoted());}
            if(kind==DrawActor::skeleton){const auto* r=skeletons_->find(key);return r&&visit(r->x.promoted(),r->y.promoted());}
            if(kind==DrawActor::whiteJelly){const auto* r=whiteJellies_->find(key);return r&&visit(r->x.promoted(),r->y.promoted());}
            if(kind==DrawActor::redBall){const auto* r=redBalls_->find(key);return r&&visit(r->x.promoted(),r->y.promoted());}
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
    template<bool RetainOrder=true,class Emit> bool visitRegisteredEffectSurfaces(const downwell::EffectMaskAssets& assets,
        const downwell::EffectSurfacePlacement::View& view,Emit emit)const{
        // These surface emitters do not run mutable Draw events. Share the
        // actor renderer's mutation-invalidated order, but read all artwork,
        // positions and frames live. The full traversal remains the oracle.
        const auto visitor=[&](DrawActor kind,downwell::InstanceHandle key,auto){
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
        };
        // Without a room owner there are no registered families to walk.
        bool ok=!roomActorDraw_;
        if(!ok){
            if constexpr(RetainOrder)ok=visitRoomActorsReadOnly(visitor);
            else ok=visitRoomActors(visitor);
        }
        // Compact objDeadBody 263 and object-370 debris have no room actors:
        // draw them after them, in object order. W1.9's objDrone (depth
        // 10000) precedes them; explosionSmall (0) and bulletExplosion1
        // (-60000) follow.
        if(ok&&drone_.live){
            const auto x=downwell::MotionBinary64::fromInteger(int((drone_.x+0x8000)>>16));
            const auto y=downwell::MotionBinary64::fromInteger(int((drone_.y+0x8000)>>16));
            downwell::EffectSurfacePlacement::Draw placement;downwell::EffectMaskAssets::Asset asset;
            if(!downwell::EffectSurfacePlacement::fx(x,y,view,false,false,true,placement))return false;
            if(placement.target!=downwell::EffectSurfacePlacement::hidden&&
               (!assets.drone(drone_.frame(),asset)||!emit(asset,x,y)))return false;
        }
        if(ok&&compactCorpses_.size()&&!compactCorpses_.visitNewest([&](auto,const downwell::CorpseAnimation& corpse){
            if(ok&&!downwell::CorpseDraw::surface(corpse,assets,view,emit))ok=false;
        }))return false;
        if(ok&&compactDebris_.size()&&!compactDebris_.visitNewest([&](auto,const downwell::DebrisAnimation& effect){
            if(ok&&!downwell::DebrisDraw::surface(effect,assets,view,emit))ok=false;
        }))return false;
        // decoyBalloon (depth -10000) sits between explosionSmall and
        // bulletExplosion1; its string is the platform's line.
        static const int explosionObjects[]={274,272};
        for(int object:explosionObjects){
          if(ok&&object==272&&balloon_.live){
            const auto x=downwell::MotionBinary64::fromInteger(balloon_.x),y=downwell::MotionBinary64::fromInteger(balloon_.y);
            downwell::EffectSurfacePlacement::Draw placement;downwell::EffectMaskAssets::Asset asset;
            if(!downwell::EffectSurfacePlacement::fx(x,y,view,false,false,true,placement))return false;
            if(placement.target!=downwell::EffectSurfacePlacement::hidden&&
               (!assets.balloon(unsigned(balloon_.sprite),balloon_.frame(),asset)||!emit(asset,x,y)))return false;
          }
          // playerDraw's jetpack flame (the player's depth, -50000): sprite by
          // thirds of jetFrame (sprJetL, M, S), image jetFrame, at (x, y + 16).
          if(ok&&object==272&&player_.jetpacking()&&!goalStopped()){
            const unsigned third=player_.jetHalves()/60;
            const unsigned sprite=third==0?21:third==1?22:23;
            const auto x=downwell::MotionBinary64::fromInteger(240+int(downwell::roundGameMakerQ8(player_.offsetXFixed())));
            const auto y=downwell::MotionBinary64::fromInteger(176+16+int(downwell::roundGameMakerQ8(player_.offsetYFixed())));
            downwell::EffectSurfacePlacement::Draw placement;downwell::EffectMaskAssets::Asset asset;
            if(!downwell::EffectSurfacePlacement::fx(x,y,view,false,false,true,placement))return false;
            if(placement.target!=downwell::EffectSurfacePlacement::hidden&&
               (!assets.jetFlame(sprite,(player_.jetHalves()/2)%3,asset)||!emit(asset,x,y)))return false;
          }
          for(unsigned i=blastEnd_;ok&&i-->0;){
            const auto& b=blasts_[i];
            if(!b.live||b.object!=object)continue;
            const auto x=downwell::MotionBinary64::fromInteger(b.x),y=downwell::MotionBinary64::fromInteger(b.y);
            downwell::EffectSurfacePlacement::Draw placement;downwell::EffectMaskAssets::Asset asset;
            if(!downwell::EffectSurfacePlacement::fx(x,y,view,false,false,true,placement))return false;
            if(placement.target==downwell::EffectSurfacePlacement::hidden)continue;
            // GameMaker mirrors, then rotates: a mirrored page turns the other way.
            const bool mirrored=b.xscale<0;
            const unsigned turn=unsigned(mirrored?(4-b.turn)&3:b.turn);
            if(!assets.explosion(unsigned(b.sprite),mirrored,turn,b.frame(),asset)||!emit(asset,x,y))return false;
          }
        }
        return ok;
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
    template<class Audio,class Body> bool withRunAudio(Audio& audio,Body body){
        if(runAudio_.stopAll)return false;
        runAudio_=downwell::RunAudioBinding::bind(audio);
        struct Leave {downwell::RunAudioBinding& binding;~Leave(){binding={};}} leave{runAudio_};
        return body();
    }
    // Split-loop session scope: the caller owns these callbacks until Draw
    // entry or frame completion. Never coexist with a reference lexical pass.
    bool bindEncounterUpdate(const void* owner,downwell::RunAudioBinding audio,
        void* collisionContext,bool (*collision)(void*)){
        if(!run_.effectsSucceeded||!owner||!audio.stopAll||!collision||
           encounterUpdateOwner_||runAudio_.stopAll||collisionPass_)
            return run_.effectsSucceeded=false;
        encounterUpdateOwner_=owner;runAudio_=audio;
        collisionPassContext_=collisionContext;collisionPass_=collision;
        return true;
    }
    bool unbindEncounterUpdate(const void* owner){
        if(!owner||encounterUpdateOwner_!=owner||collisionPassExecuting_)
            return run_.effectsSucceeded=false;
        encounterUpdateOwner_=nullptr;runAudio_={};
        collisionPassContext_=nullptr;collisionPass_=nullptr;
        return true;
    }
    bool boundEncounterUpdate(const void* owner)const{return owner&&encounterUpdateOwner_==owner;}
    bool bindEncounterInput(downwell::EncounterInput* input){
        if(input&&encounterInput_&&input!=encounterInput_)return false;
        encounterInput_=input;run_.shops.borrowedInput=input?&input->publication():nullptr;
        return true;
    }
    bool boundEncounterInput(const downwell::EncounterInput* input)const{
        return encounterInput_==input&&run_.shops.borrowedInput==(input?&input->publication():nullptr);
    }
    bool advanceEnemyRoomAnimation(downwell::InstanceHandle key);
    bool advanceEnemyRoomAlarm(downwell::InstanceHandle key,unsigned alarm);
    // The event part of advanceEnemyRoomAlarm, for a resolved record and an
    // alarm index below AlarmBank::count.
    bool advanceEnemyAlarm(EnemyState& enemy,unsigned alarm);
    // advanceEnemyRoomAlarm for each roster entry with the alarm's event, in
    // order, for Alarm 0 or 1. Each record resolves once; one whose counter
    // is negative, or that is inactive or marked, changes nothing and is
    // skipped. Eligible(shared) is the caller's capture filter; Valid() runs
    // after each dispatched event, as the captured walk checks its owner.
    template<class Entry,class Eligible,class Valid>
    bool advanceRosterEnemyAlarm(const Entry* entries,unsigned count,unsigned alarm,unsigned base,
                                 Eligible eligible,Valid valid){
        const unsigned bit=1u<<alarm;
        for(unsigned e=0;e<count;++e){
            const auto& entry=entries[e];
            if(!(entry.alarms&bit)||!eligible(entry.handle))continue;
            const downwell::InstanceHandle local{entry.handle.slot-base,entry.handle.generation};
            auto* record=const_cast<EnemyState*>(resolveEnemyLifetime(local));
            if(!run_.effectsSucceeded||!record)return run_.effectsSucceeded=false;
            if((alarm?record->alarm1:record->alarm0)<0||!record->active||record->destroyRequested)continue;
            if(!advanceEnemyAlarm(*record,alarm)||!valid())return false;
        }
        return true;
    }
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
        if(alarm==0)run_.advanceExitDitherAlarm();
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
            // Step under pTimeStop set image_speed 0: this advance holds.
            if(!enemy->frozen)
                downwell::advanceEnemyImageQ8(enemy->imagePhase,enemy->imageRemainder,enemy->imageSpeed);
            // phaseIndex rejects this before the native no-op publisher too.
            if(enemy->imagePhase>0x7fffffffu)return run_.effectsSucceeded=false;
        }while(cursor.within(first,last));
        return true;
    }
    // The same advance for every enemy whose local slot is room-active, in
    // record order, for an animation population of native enemies alone.
    template<class Active> bool advanceActiveEnemyAnimations(Active active){
        for(unsigned i=0;i<enemyCount_;++i){
            auto& enemy=enemies_[i];
            if(enemy.identitySlot>=enemyCapacity||!active(enemy.identitySlot))continue;
            // Step under pTimeStop set image_speed 0: this advance holds.
            if(enemy.frozen)continue;
            downwell::advanceEnemyImageQ8(enemy.imagePhase,enemy.imageRemainder,enemy.imageSpeed);
            if(enemy.imagePhase>0x7fffffffu)return run_.effectsSucceeded=false;
        }
        return run_.effectsSucceeded;
    }
    template<class Cursor,class Identity>
    bool advanceNativeBulletAnimations(Cursor& cursor,Identity identity,unsigned first,unsigned last){
        do {
            const auto slot=cursor.take();
            auto* bullet=const_cast<BulletState*>(resolveBulletLifetime({slot-first,identity(slot)}));
            if(!bullet||!run_.effectsSucceeded)return false;
            if(!advanceBulletAnimation(*bullet))return run_.effectsSucceeded=false;
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
    // builder Step births, then dispatch up to exclusive object boundaries.
    // The owning collision adapter retains handles and rechecks eligibility;
    // no family-local scan may discover children born during this Step.
    template<class Capture,class Execute,class Tick>
    bool withRegisteredEncounterSteps(Capture capture,Execute execute,Tick tick){
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
    // This adapter owns Alarm 1 only, irrespective of the larger immutable
    // event table on object 35. Keep the dispatcher's admitted columns tied
    // to this ownership contract, not a snapshot of the live countdown.
    static constexpr unsigned playerDamageAlarmColumns=1u<<1;
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
        breakables_[h.slot].state.wallHp=health;observeBoxHealth(breakables_[h.slot]);return true;
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
    // Conservative lifetime proof already maintained by alarm publication.
    // Once armed, keep the dispatcher even after the countdown expires.
    bool boxAlarmsNeverArmed()const{return !boxAlarmsArmed_;}
    // Native boxes never re-register or compact. A prefix plus generation
    // ceiling captures the same lifetimes without copying their handles.
    // The ceiling also excludes replacement slots after a room/config reset.
    struct BoxAlarmPrefix {unsigned end=0;std::uint32_t lastLifetime=0;};
    BoxAlarmPrefix captureBoxAlarms()const{return {breakableCount_,nextBreakableLifetimeId_-1};}
    bool advanceBoxAlarmPrefix(BoxAlarmPrefix phase){
        if(!run_.effectsSucceeded)return false;
#ifndef DOWNWELL_REFERENCE_ENEMY_UPDATES
        if(!boxAlarmsArmed_)return true; // Every counter is still -1.
#endif
        const unsigned end=phase.end<breakableCount_?phase.end:breakableCount_;
        for(unsigned i=0;i<end;++i){
            auto& box=breakables_[i];
            // A negative counter is untouched whatever the box's state, and
            // native boxes rarely arm Alarm 0: test it first.
            if(box.alarm0<0||!box.lifetimeId||box.lifetimeId>phase.lastLifetime||!box.active||box.destroyRequested)continue;
            if(downwell::AlarmBank::advanceValue(box.alarm0)){box.state.alarmZero();observeBoxHealth(box);}
        }
        return true;
    }
    bool publishBoxAlarm0(downwell::InstanceHandle h,std::int32_t value){
        if(!run_.effectsSucceeded||!findBreakable(h))return run_.effectsSucceeded=false;
        if(value>=0)boxAlarmsArmed_=true;
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
                                PlatformContactResolver platforms = nullptr,
                                downwell::SolidCornerQuery::Corners corners = nullptr) {
        if(resolver!=solidResolver_||context!=solidResolverContext_)solidCells_={};
        solidResolver_ = resolver;
        solidCorners_=corners;
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
    // The cells behind the configured solid resolver, which must answer
    // exactly as SolidCells does for every sample, boxes included. Native
    // enemy motion reads them directly; a different resolver drops them.
    void configureSolidCells(const downwell::SolidCellGrid& cells){solidCells_=cells;}
    bool configureEnemy(unsigned index, int objectId, int spriteId,
                        int x, int y, int speedX, int speedY,
                        int health, int reward,
                        int motionState = 0, int auxiliaryState = 0,
                        int alarm0 = -1, int alarm1 = -1,
                        int direction = 0, int moveSpeed = 0,
                        bool active = true, bool grounded = false,
                        bool awake = false,std::uint32_t sourceId=0);
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
        if (command.objectId == 78) return !compactSpawnerAdmitted || createCompactSpawner(command.y);
        if (!terrain(command)) return false;
        // Compact world-one scenery. The registered Scattered16 owner adopts
        // only through createGeneratedInstanceWithSourceIds below.
        if ((command.objectId == 289 || command.objectId == 293 || command.objectId == 296) &&
            compactRoomEffectsAdmitted)
            return createCompactScenery(command);
        if (command.objectId == 109 && compactGunsAdmitted) return createGunModule(command);
        if (command.objectId == 92) {
            DW_GENERATION_SCOPE(profileGeneratedBreakable);
            const int x = command.x - 240, y = command.y - 176;
            const auto state=command.boxCreate();
            return appendBreakables(&x, &y, 1,&state);
        }
        const auto profile = downwell::enemyCreateProfile(command.objectId);
        if (!profile.health) return true;
        return adoptGeneratedEnemy(command,objects,0);
    }
    bool adoptGeneratedEnemyWithSourceId(std::uint32_t sourceId,const downwell::DeferredCreateInstance& command,
        const downwell::ObjectPack& objects){
        if(!run_.effectsSucceeded)return false;
        if(!sourceId||sourceId>0x7fffffffu||!downwell::enemyCreateProfile(command.objectId).health)
            return run_.effectsSucceeded=false;
        return run_.effectsSucceeded=adoptGeneratedEnemy(command,objects,sourceId);
    }
    // Source-identity handoff for a synchronous builder command. The caller
    // supplies the room-wide allocator, including nested shop/exit Creates.
    // Do not reserve a batch: publication can synchronously create children.
    // Terrain receives the allocated ID even when its native owner is a run.
    // Unsupported families fail instead of silently consuming an ID without
    // constructing an actor. The legacy handoff above remains the reference
    // until the enclosing frontend supplies its complete creation stream.
    template<class Allocate,class Terrain,class Shop>
    bool createGeneratedInstanceWithSourceIds(const downwell::DeferredCreateInstance& command,
        const downwell::ObjectPack& objects,Allocate& allocate,Terrain terrain,Shop shop){
        if(!run_.effectsSucceeded)return false;
        if(command.objectId==277)
            return run_.effectsSucceeded=shop(command,allocate)&&run_.effectsSucceeded;
        if(command.objectId==306)return registerLevelExitWithSourceIds(command.y,
            [&](int object,unsigned& id){return allocate(object,id);});
        if(command.objectId==78){
            // The current run frontend admits normal cavern mode only.
            if(!spawners_||run_.stage.area!=1||run_.stage.level<1||run_.stage.level>3)
                return run_.effectsSucceeded=false;
            unsigned id=0;downwell::InstanceHandle key;
            return run_.effectsSucceeded=allocate(78,id)&&run_.effectsSucceeded&&
                spawners_->createCaverns(id,command.x,command.y,run_.stage.level,false,key);
        }
        const bool wall=command.objectId==60||command.objectId==93||command.objectId==99;
        const bool enemy=downwell::enemyCreateProfile(command.objectId).health!=0;
        if(!wall&&!enemy&&command.objectId!=92&&command.objectId!=82&&command.objectId!=62&&command.objectId!=289)
            return run_.effectsSucceeded=false;
        unsigned source=0;
        if(!allocate(command.objectId,source)||!run_.effectsSucceeded||!source||source>0x7fffffffu)
            return run_.effectsSucceeded=false;
        if(command.objectId==82)return adoptGeneratedTimeFieldWithSourceId(source,command);
        if(command.objectId==62)return registerSideCameraLocker(command.x,command.y,source);
        if(!terrain(command,source)||!run_.effectsSucceeded)return run_.effectsSucceeded=false;
        if(command.objectId==289)return run_.effectsSucceeded=scattered_&&scattered_->adopt(source,command);
        if(wall){
            if(terrainInstances_&&
                (!terrainInstances_->append(command.objectId,command.x,command.y,command.active,source)||
                 !terrainInstances_->createProperties(terrainInstances_->handle(terrainInstances_->size()-1),
                    downwell::terrainTileForArea(run_.stage.area),false,enemyRandom_)))
                return run_.effectsSucceeded=false;
            return true;
        }
        if(command.objectId==92){
            const int x=command.x-240,y=command.y-176;const auto state=command.boxCreate();
            const std::uint32_t sourceId=source;
            return run_.effectsSucceeded=appendBreakables(&x,&y,1,&state,&sourceId);
        }
        return adoptGeneratedEnemyWithSourceId(source,command,objects);
    }
private:
    bool adoptGeneratedEnemy(const downwell::DeferredCreateInstance& command,
        const downwell::ObjectPack& objects,std::uint32_t sourceId){
        const auto profile=downwell::enemyCreateProfile(command.objectId);
        DW_GENERATION_SCOPE(profileGeneratedEnemy);
        downwell::PackedObject object;
        return objects.findObject(command.objectId, object) &&
            configureEnemy(enemyCount_, command.objectId, object.spriteId,
                command.x - 240, command.y - 176,
                command.integerVelocityX, command.integerVelocityY, profile.health, profile.reward,
                command.motionState, command.auxiliaryState,
                command.alarms.get(0), command.alarms.get(1),
                command.integerDirection, command.integerMoveSpeed,
                command.active, command.grounded, true,sourceId);
    }
public:
    std::uint64_t randomDrawCount() const { return enemyRandom_.drawCount(); }
    const downwell::GameMakerRandom& randomState() const { return enemyRandom_; }
    bool prepareRandomIdleWord(){return run_.effectsSucceeded&&enemyRandom_.prepareIdleWord();}

    // Pure query of the implemented area-one snapshot, in room coordinates.
    downwell::LaserSightRay traceLaserSight(downwell::LaserSightRay::Coordinate x,
        downwell::LaserSightRay::Coordinate y, downwell::LaserSightRay::Coordinate bottom,
        std::uint32_t speedTurns, std::uint32_t aimTurns = 0) const;
    downwell::LaserSightRay laserSightEndpoint(bool aimed = true) const {
        using Ray = downwell::LaserSightRay;
        return traceLaserSight(Ray::fromQ8(player_.offsetXFixed(), 240),
            Ray::fromQ8(player_.offsetYFixed(), 176),
            Ray::fromQ8(viewCameraYQ8(), 176 + 142),
            Ray::turnsFromQ8Degrees(player_.speedXFixed()),
            // playerDraw's ll = 270 + aimAngle (scrAimControl's spread guns).
            Ray::turnsFromQ8Degrees(aimed ? aimAngle_*256 : 0));
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
    bool usesBulletDestruction(const BulletLifecycleBinding& binding)const{
        return bulletLifecycle_.sameDestruction(binding);
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
    const downwell::SmokeEffects<downwell::RunSession::smokeCapacity>& smokeEffects() const {return run_.smokeEffects;}
    bool emitBoxSmoke(downwell::MotionBinary64 x,downwell::MotionBinary64 y){return run_.emitBoxSmoke(x,y,enemyRandom_);}
    bool emitBoxFlash(downwell::MotionBinary64 x,downwell::MotionBinary64 y){return run_.emitBoxFlash(x,y,enemyRandom_);}
    bool effectsSucceeded()const{return run_.effectsSucceeded;}
    const downwell::WallBackgroundTiles<downwell::RunSession::wallTileCapacity>& wallBackgroundTiles()const{return run_.wallBackground;}
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
    // W1.10 persistence: the save file's records enter the run once at
    // startup (scrLoadSave) and survive Retry; the backend writes them back.
    // global.playStyle and global.hardMode for a new run, before its first
    // room loads. objPlayer_n Create at 1-1 calls styleUpdate: style 2 starts
    // (and stays capped) at 6 HP, the others at 4.
    bool beginRun(unsigned style, bool hard) {
        if (style > 4 || run_.stage.area != 1 || run_.stage.level != 1) return false;
        run_.playStyle = style; run_.hardMode = hard;
        player_.setPlayStyle(int(style));
        const int hp = style == 2 ? 6 : 4;
        health_ = hp; run_.combo.healthReserve.maximum = hp;
        // The first boot's 1-1 was generated before style select: its
        // spawner (not yet stepped) takes the chosen mode's channels.
        if (compactSpawnerLive_ && !compactSpawner_.scriptActive &&
            !compactSpawner_.createCaverns(run_.stage.level, hard)) return false;
        return true;
    }
    bool hardMode() const { return run_.hardMode; }
    unsigned playStyle() const { return run_.playStyle; }
    downwell::RunRecords runRecords() const { return run_.records(); }
    void restoreRunRecords(const downwell::RunRecords& r) {
        run_.gems.lifetime = r.lifetimeGems; run_.gems.record = r.bestRunGems;
        run_.combo.state.record = r.bestCombo;
    }
    // saveStats' gem subset ran for this run (first death movement).
    bool runGemsRecorded() const { return run_.gems.recorded; }
    // The slice endpoint ends the run without a death: record as EndMenu's
    // saveStats does. Returns false if this run was already recorded.
    bool recordRunEnd() { return run_.gems.observeDeathMovement(true, currencyAccount_.gameGem); }
    bool grantConsumable(unsigned id) { return run_.consumables.grant(id, currencyAccount_); }
    bool grantUpgrade(unsigned id) {
        return run_.upgrades.grant(id, run_.consumables, currencyAccount_,
            player_, health_, run_.combo.healthReserve);
    }
    const downwell::UpgradeInventory& upgrades() const { return run_.upgrades; }
    int nextMuzzleOffset() const { return emitter_.leg; }
    const downwell::Consumables& consumables() const { return run_.consumables; }
    downwell::ShopWorld::Instance* registerShop(int x, int y) {
        if(!run_.effectsSucceeded)return nullptr;
        return run_.createShop(x, y, [this](unsigned lo, unsigned hi) {
            return lo + enemyRandom_.irandomSmall(hi - lo);
        }, [this] { return enemyRandom_.chooseIndexSmall(3); }, health_);
    }
    // The enclosing creation stream owns parent/child IDs and lifetime
    // publication. Stock and farewell must use this world's RNG and current
    // run state, not a copied seed or a backend's reconstructed inventory.
    // A failed synchronous Create is terminal; never fall back to compact
    // stock after publishing a prefix of the shop's children.
    template<class Owner>
    downwell::ShopWorld::Instance* registerShopWithOwner(int x,int y,Owner& owner){
        if(!run_.effectsSucceeded)return nullptr;
        return run_.createRegisteredShop(x,y,[this](unsigned lo,unsigned hi){
            return lo+enemyRandom_.irandomSmall(hi-lo);
        },[this]{return enemyRandom_.chooseIndexSmall(3);},health_,owner);
    }
    template<class Owner,class Lookup>
    downwell::ShopWorld::Instance* registerShopWithOwner(int x,int y,Owner& owner,Lookup& lookup){
        if(!run_.effectsSucceeded)return nullptr;
        return run_.createRegisteredShop(x,y,[this](unsigned lo,unsigned hi){
            return lo+enemyRandom_.irandomSmall(hi-lo);
        },[this]{return enemyRandom_.chooseIndexSmall(3);},health_,owner,lookup);
    }
    const downwell::ShopWorld& shops() const { return run_.shops; }
    // W2.3: the run's sound requests (the backend drains them each frame)
    // and the voice model behind them.
    downwell::SoundRequests& soundRequests() { return run_.soundRequests; }
    // Scripts outside the world's step (pause menu, death and results
    // menus): soundPlayOL(id, 90, 0, 1, "UI") and objPauseMenu's
    // audio_pause_all / audio_resume_all (Resume also re-pauses the song in a
    // void room).
    int playUiSound(int id) { return run_.soundPlayOL(id, 90, false, true, "UI"); }
    void pauseAllSounds() { run_.pauseAllSounds(); }
    void resumeAllSounds() {
        run_.resumeAllSounds();
        if (run_.timeStopped) { auto audio = run_.audio(); audio.pause(run_.backgroundMusicId); }
    }
    const downwell::SoundVoices& soundVoices() const { return run_.voices; }
    // The Audio concept over the run's voices and ring for a call's
    // OLchannel category and cut flag (encounter owners outside the step).
    downwell::SoundRequestAudio runSoundAudio(int category, bool cut) { return run_.audio(category, cut); }
    template<class Body> bool withShopStepState(Body body){
        if(!run_.effectsSucceeded)return false;
        return run_.effectsSucceeded=body(run_.shops.inputState(),currencyAccount_)&&run_.effectsSucceeded;
    }
    bool registerSideCameraLocker(int x, int y,unsigned sourceId=0) {
        return run_.effectsSucceeded=run_.effectsSucceeded&&run_.sideCameraLockers.create(x,y,sourceId);
    }
    bool bindSideCameraLockerLifecycle(downwell::SideCameraLockers::Binding binding){
        return run_.effectsSucceeded=run_.effectsSucceeded&&run_.sideCameraLockers.bindLifecycle(binding);
    }
    const downwell::SideCameraLockers& sideCameraLockers()const{return run_.sideCameraLockers;}
    const downwell::OrdinaryTimeFields<64>& timeFields() const {return run_.timeFields;}
    template<class Audio> bool duckMusic(downwell::MusicDuck& caller,int duration,
        downwell::MotionBinary64 amount,Audio& audio){return run_.duckMusic(caller,duration,amount,audio);}
    bool createTimeFieldWithSourceId(std::uint32_t source,int x,int y){
        if(!run_.effectsSucceeded)return false;
        return run_.effectsSucceeded=run_.timeFields.createWithSourceId(source,x,y,enemyRandom_);
    }
    // Builder commands already carry the radius sample. Re-running Create
    // here would consume an extra RNG draw. The source ID must come from the
    // enclosing birth stream, not from the field's opaque lifetime key.
    bool adoptGeneratedTimeFieldWithSourceId(std::uint32_t source,const downwell::DeferredCreateInstance& command){
        if(!run_.effectsSucceeded)return false;
        if(!source||source>0x7fffffffu||command.objectId!=82)return run_.effectsSucceeded=false;
        return run_.adoptTimeField(command.x,command.y,command.timeFieldRadiusWord(),source);
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
        return registerLevelExitAllocated(y,[](int,unsigned& id){id=0;return true;});
    }
    // The enclosing source stream allocates the trigger before its Create,
    // then each synchronous child separately. Never reserve three IDs: child
    // lifecycle callbacks can themselves consume IDs from the same stream.
    template<class Allocate> bool registerLevelExitWithSourceIds(int y,Allocate allocate){
        return registerLevelExitAllocated(y,[&](int object,unsigned& id){
            return allocate(object,id)&&id&&id<=0x7fffffffu;
        });
    }
private:
    template<class Allocate> bool registerLevelExitAllocated(int y,Allocate allocate){
        unsigned parent=0;
        if(!run_.effectsSucceeded||!allocate(306,parent)||!exitControllers_.create(306,240,y,parent))
            return run_.effectsSucceeded=false;
        return run_.createLevelExit(480,y,enemyRandom_,cameraMotion_,
            [&](downwell::LevelExit::Effect effect,int x,int childY){
                unsigned child=0;
                if(!allocate(effect==downwell::LevelExit::Effect::roomEndSetter?74:255,child))return false;
                return effect==downwell::LevelExit::Effect::roomEndSetter?
                    exitControllers_.create(74,x,childY,child):goalFieldInstances_.create(x,childY,255,nullptr,child);
            });
    }
public:
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
        // scrResult consumes global.dUp, not dUpHeld. Keep the explicit
        // publication for isolated callers, but production borrows the session.
        auto* edges=encounterInput_?&encounterInput_->publication():nullptr;
        bool& publishedConfirm=edges?edges->confirm:confirm;
        if(edges){leftPressed=edges->left;rightPressed=edges->right;}
        ++tick_;run_.soundRequests.setTick(tick_);
        run_.voices.advance(); // W2.3: the Result room's source steps keep the audio clock
        return run_.stepLevelClearPositioned(publishedConfirm, leftPressed, rightPressed, heartGoal,
            currencyAccount_, player_, health_, enemyRandom_, emit,[](int,int){return true;},
            [this](auto event){
                if(!resultRoomAlarms_){
                    run_.advanceExitDitherAlarm();return run_.levelClear.advanceAlarmsChecked(event);
                }
                return resultRoomAlarms_(resultRoomAlarmsContext_,
                    [](void* context,downwell::LevelClearSelection::Effect effect,unsigned value){
                        return (*static_cast<decltype(event)*>(context))(effect,value);
                    },&event);
            },[this](std::int32_t y){return !resultInstance_||resultInstance_->publishY(y);},[]{return true;},
            [this]{return run_.resultStepWind(enemyRandom_);});
    }
    bool levelClearActive() const { return run_.levelClearActive; }
    bool nextLevelRequested() const { return run_.nextLevelRequested; }
    // Retire the old display first. This resets only room-local runtime state;
    // the owner must rebuild terrain and rebind generation/collision callbacks.
    bool prepareNextAreaOneLevel(const downwell::Input& input, unsigned initialRingOrigin);
    // Result Alarm 1 / scrNextLevel, stopping before rmMain loads area 2.
    bool finishWorldOneSlice();
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
        setup.hard = run_.hardMode;
        run_.playStyle = style;
        player_.setPlayStyle(int(style));
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
    downwell::RunAudioBinding runAudio_;
    const void* encounterUpdateOwner_=nullptr;
    downwell::EncounterInput* encounterInput_=nullptr;
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
#ifdef DOWNWELL_WORLD_ONE_ONLY
    // World-one builds never bind later-area families; their paths fold away.
    static constexpr LimboShards* limboShards_=nullptr;
#else
    LimboShards* limboShards_=nullptr;
#endif
    RegisteredFlash* registeredFlash_=nullptr;
    downwell::GameplayDebrisInstances* debrisInstances_=nullptr;
    downwell::NativeTimeChangeRegistration* timeChangeInstances_=nullptr;
    downwell::NativeTimeShardRegistration* timeShardInstances_=nullptr;
    downwell::TimeShardEffects<64>* timeShardEffects_=nullptr;
    bool clearRoomTimeShards();
    downwell::GameplayEntryDithers* entryDithers_=nullptr;
    downwell::GameplayDitherAbove* ditherAbove_=nullptr;
    downwell::GameplayDitherAbove ownedDitherAbove_;
    downwell::GameplayBuilderController* builderController_=nullptr;
    downwell::GameplayPlayerControllers* playerControllers_=nullptr;
    downwell::GameplayScattered* scattered_=nullptr;
    downwell::GameplaySpawners* spawners_=nullptr;
    downwell::GameplayWhiteJellies* whiteJellies_=nullptr;
    downwell::GameplayRedBalls* redBalls_=nullptr;
    downwell::GameplayShopInstances* shopInstances_=nullptr;
    bool (*redBallAlarmDispatch_)(void*,downwell::InstanceHandle,downwell::GameplayRedBalls::Record&,downwell::GameMakerRandom&)=nullptr;
    void* redBallAlarmContext_=nullptr;
    bool (*whiteJellyAlarmDispatch_)(void*,downwell::InstanceHandle,downwell::GameplayWhiteJellies::Record&,downwell::GameMakerRandom&)=nullptr;
    void* whiteJellyAlarmContext_=nullptr;
    MovingEffects* movingEffects_=nullptr;
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
#ifdef DOWNWELL_WORLD_ONE_ONLY
    static constexpr Skeletons* skeletons_=nullptr;
#else
    Skeletons* skeletons_=nullptr;
#endif
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
    bool publishEnemySprite(EnemyState& enemy,int sprite);
    // With a record-owned appearance these setters are no-ops, bar the
    // image's range check: take them here, without a call per Step.
    bool publishEnemyImage(const EnemyState& enemy,unsigned phase){
        if(!enemyLifecycle_.recordOwnsAppearance())return publishOwnedEnemyImage(enemy,phase);
        if(phase<=0x7fffffffu)return true;
        return run_.effectsSucceeded=false;
    }
    bool publishEnemyTransform(const EnemyState& enemy,downwell::RunnerTransformField field,int value){
        return enemyLifecycle_.recordOwnsAppearance()||publishOwnedEnemyTransform(enemy,field,value);
    }
    bool publishOwnedEnemyImage(const EnemyState& enemy,unsigned phase);
    bool publishOwnedEnemyTransform(const EnemyState& enemy,downwell::RunnerTransformField field,int value);
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
    // W2.6: enmFrog's Collision with enmFrog after the enemy Steps.
    void bumpCompactFrogs();
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
    // Only publishBoxAlarm0 arms a box's Alarm 0 (time stop extends a positive
    // one). Never cleared, so the box prefix walk resumes for the rest of the
    // world's life once any box has been armed.
    bool boxAlarmsArmed_=false;
    // Every write of a box's health reports here; also never cleared. While
    // no box has been exhausted, the unbound box Step has nothing to destroy.
    bool boxesMayBeExhausted_=false;
    void observeBoxHealth(const Breakable& box){if(box.state.exhausted())boxesMayBeExhausted_=true;}
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
    downwell::SolidCornerQuery::Corners solidCorners_=nullptr;
    downwell::SolidCellGrid solidCells_{};
    // An aligned box answers at its centre, the cell's top-left plus 8.
    struct LiveBoxCell {
        const GameplayWorld* world;
        bool operator()(int left,int top)const{return world->breakableActiveAt(left+8,top+8);}
    };
    downwell::SolidCells<LiveBoxCell> solidCells()const{return {&solidCells_,{this}};}
    bool nativeSolidCells()const{
#ifdef DOWNWELL_REFERENCE_ENEMY_UPDATES
        return false;
#else
        return solidCells_.valid();
#endif
    }
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
    // 16x16 cells over x -256..256, y 0..8192. Breakables on the eight
    // slice routes sit at x -208..208; any box outside the grid (or off
    // the 16-pixel lattice) drops the index and every query scans instead.
    static constexpr int breakableIndexLeft = 256, breakableIndexColumns = 32;
    std::uint16_t breakableIndex_[breakableIndexColumns * 512];
    int indexedBreakable(int x, int y) const;
    // W1.4 point query: the live, unretired box whose 16x16 cell holds (x,y).
    // Uses the cell index when ready; the native-box build has none, so it
    // scans as bulletSolidContact does.
    int breakableAt(int x, int y) const;
    int playerFloorMaterial() const;
    void playShotSound(int ammoBefore);
    bool playPlayerHurt();
    bool compactTimeoutField();
    void playEnemySound(int sound,int roomY);
    void soundPlayOLAt(int sound,int priority,bool loop,bool cut,const char* channel,int roomY);
    void compactLevelBeginMusic();
    downwell::TimeStopAudio timeStopAudio_; // bgmDuck's caller fields (unused until WM)
    unsigned blocksDestroyed_;
    std::uint64_t checkpoint_;
    downwell::StateDigest trajectory_;

    void spawnReward(const EnemyState& enemy, bool attracted);

    // Compact spawner 78 and its source-exact steering children 115/118.
    // Each live steering record owns one pool slot through auxiliaryState,
    // keyed by its enemy lifetime; Step/alarm/contact events run the shared
    // WhiteJellyStep/RedBallStep state machines. The registered stores above
    // remain the reference owners and are not used by this compact world.
    union SteeringActor {
        downwell::WhiteJellyStep jelly;
        downwell::RedBallStep ball;
        SteeringActor():jelly(){}
    };
    static constexpr unsigned steeringCapacity=24;
    SteeringActor steering_[steeringCapacity];
    std::uint32_t steeringOwner_[steeringCapacity]{};
    downwell::SpawnerStep compactSpawner_;
    downwell::MotionBinary64 compactSpawnerY_{};
    int compactSpawnerMask_=-1;
    bool compactSpawnerLive_=false;
    unsigned steeringPopulation_=0; // Owned pool slots; gates the contact pass.
    bool generationEndReached_=false;
    int depthMeterY_=0; // objDepthMeter's published y: the player birth y.
    struct SteeringOps;
    struct SpawnerOps;
    void resetCompactSpawning();
    downwell::SteeringEnemyBirthState* steeringActor(const EnemyState& enemy);
    void releaseSteering(const EnemyState& enemy);
    bool createCompactSpawner(int y);
    bool stepCompactSpawner(long generationCameraY);
    bool createSpawnedEnemy(int object,downwell::MotionBinary64 x,downwell::MotionBinary64 y);
    bool stepSteeringEnemy(EnemyState& enemy);
    bool createEyefish(int x,int y);
    bool stepEyefish(EnemyState& enemy);
    bool advanceSteeringAlarm(EnemyState& enemy,unsigned alarm);
    bool hitSteeringEnemy(EnemyState& enemy,std::uint32_t directionTurns,int damage);
    bool collideSteeringEnemies();
    bool steeringWallMeeting(int mask,downwell::MotionBinary64 x,downwell::MotionBinary64 y,bool& hit);
    bool steeringEnemyMeeting(int mask,downwell::MotionBinary64 x,downwell::MotionBinary64 y,bool& hit);

    // Compact world-one effect chains (W1.3): box destruction, scenery
    // (Scattered16 289, gemChunk 293, TorchFire 295, Torch 296), debris 370,
    // and the script-spawned no-loop/moving effects they create. These run
    // only while the registered owners above are unbound.
    using CompactEffects=downwell::CompactRoomEffects<downwell::gameplayCompactNoLoopCapacity,downwell::gameplayCompactMovingCapacity>;
    CompactEffects compactEffects_;
    // Object 370 born by compact boxes and scenery, apart from the registered
    // debris owner's storage.
    downwell::DebrisEffects<downwell::gameplayCompactDebrisCapacity> compactDebris_;
    // Compact objDeadBody 263 (W1.6): the world-one families' scrDeadBody
    // corpses, apart from the registered skeleton corpse owner.
    downwell::CorpseEffects<downwell::gameplayCompactCorpseCapacity> compactCorpses_;
    bool compactEnemyDeath(EnemyState& enemy,bool stomped);
    bool compactBulletImpact(EnemyState& enemy,downwell::MotionBinary64 x,downwell::MotionBinary64 y,
        std::uint32_t direction,int damage,bool laser);
    bool compactImmuneSpark(downwell::MotionBinary64 x,downwell::MotionBinary64 y);
    bool compactBloodfx(downwell::MotionBinary64 x,downwell::MotionBinary64 y);
    void compactMomentDelay();
    // material: soundBullet's index (2 foliage terrain, 3 rock, 4 metal).
    bool compactBulletWallHit(const downwell::ProjectileSegment& path,int material=2);
    // W2.6: shop children on a local bullet path (ShopWorld::wallMaterialOn).
    int shopWallOn(const downwell::ProjectileSegment& path,bool cases)const;
    bool compactWallEffect(downwell::MotionBinary64 x,downwell::MotionBinary64 y,int sprite,int depth);
    bool compactCosmeticEffect(downwell::MotionBinary64 x,downwell::MotionBinary64 y,int sprite,downwell::MotionBinary64 speed,int depth);
    void compactDamageSlowdown();
    void advanceCompactControllerAlarm();
    int compactControllerAlarm0_=0;
public:
    // Pose in the player's Q8 offset space; room x/y = 240/176 + round(offset).
    struct CompactAfterImage{int sprite;unsigned frame;int facing;long xFixed,yFixed;};
    static constexpr unsigned afterImageCount=9;
private:
    void resetCompactAfterImages(int x,int y);
    void drawCompactAfterImages();
    // A ring, newest at afterImageHead_: Alarm 7 copies the pose the last
    // Draw showed (evaluated once per update, shared with the backends)
    // instead of re-evaluating the player and shifting nine records.
    CompactAfterImage afterImages_[afterImageCount];
    CompactAfterImage presentedPose_{31,0,1,0,0};
    unsigned liveRewards_=0;
    unsigned afterImageHead_=0;
    int afterImageDx_[afterImageCount]={},afterImageDy_[afterImageCount]={};
    unsigned afterImagesDrawn_=0;
    void compactLevelBeginCue();
    bool createCompactComboReward();
    void advanceCompactHudAlarms();
    void advanceCompactHudDraw();
    downwell::CompactLevelTitle compactTitle_;
    void createCompactShopSpeech();
    downwell::CompactShopSpeech shopSpeech_[downwell::ShopWorld::capacity];
    unsigned shopSpeechLive_=0;
    downwell::CompactShopSpeechText shopSpeechText_;
    downwell::CompactComboRewardRecord compactRewards_[4]; // compactComboRewardCapacity
    downwell::CompactGemStreakHud gemHud_;
    const downwell::AreaTitle* compactAreaTitles_=nullptr;
    unsigned compactAreaTitleCount_=0;
    const char* compactLanguage_="english";
    bool compactCorpseMeeting(downwell::MotionBinary64 x,downwell::MotionBinary64 y,bool platforms,bool& hit);
    bool stepCompactCorpses(decltype(compactCorpses_.beginPhase()) phase);
    bool advanceCompactCorpseAlarm(unsigned alarm);
    bool endStepCompactCorpses(long viewTop);
    // W1.9 upgrade owners (CompactUpgrades.h); GameplayWorldUpgrades.inc.
    downwell::CompactBlastRecord blasts_[downwell::gameplayCompactBlastCapacity];
    unsigned blastEnd_=0;
    std::uint32_t blastSerial_=0;
    bool createCompactBlast(int object,int x,int y,int sprite=0);
    bool compactExplodingKnees(int x,int y,int smokeX,int smokeY,bool smoke);
    bool compactRocketJump(int x,int y,int smokeX,int smokeY);
    void advanceCompactBlastAnimation();
    bool stepCompactBlasts(std::uint32_t lastSerial);
    bool collideCompactBlasts();
    bool blastEnemy(downwell::CompactBlastRecord& blast,EnemyState& enemy);
    bool blastShootables(downwell::CompactBlastRecord& blast,const downwell::ContactBounds& bounds);
    bool blastWalls(downwell::CompactBlastRecord& blast,const downwell::ContactBounds& bounds);
    void clearCompactUpgrades();
    // objDrone 249 (Drone) and bulletDrone 266 (its shot, Gunpowder Blocks').
    downwell::CompactDroneRecord drone_;
    bool firedThisStep_=false; // global.pFired as objDrone's Step sees it
    int jetSound_=-1; // objPlayer_n.jetpacksnd (Safety Jetpack's loop)
    bool createCompactCompanions(int x,int y);
    BulletState* createDroneBullet(int x,int y,int degrees,int damage);
    void advanceCompactDroneAnimation(){if(drone_.live)++drone_.phaseHalves;}
    bool advanceCompactDroneAlarm();
    void stepCompactDrone();
    // decoyBalloon 256 (Heart Balloon): held on a string; global.ballooning
    // is a live, held balloon.
    downwell::CompactBalloonRecord balloon_;
    bool balloonHeld()const{return balloon_.live&&balloon_.held;}
    bool stepCompactBalloon();
    bool advanceCompactBalloonAnimation();
    bool collideCompactBalloon();
    bool compactSolidPoint(int x,int y)const;
    // fxBlood 223: Knife and Fork's drops at the player (sprWaterSplash 92).
    downwell::CompactBloodRecord bloods_[downwell::gameplayCompactBloodCapacity];
    unsigned bloodEnd_=0;
    bool createCompactBlood(downwell::MotionBinary64 x,downwell::MotionBinary64 y);
    bool eatCompactCorpses();
    bool advanceCompactBloodAnimation();
    bool stepCompactBlood();
    bool collideCompactBlood();
    bool checkKnifeAndFork();
    downwell::CompactScenery scenery_;
    int sceneryRepaintX_[downwell::gameplaySceneryRepaintCapacity]{};
    int sceneryRepaintY_[downwell::gameplaySceneryRepaintCapacity]{};
    unsigned sceneryRepaintCount_=0;
    struct BoxOps;
    struct SceneryOps;
    // Compact guns (W1.4): the seven bStatInitLevel1 guns, their shots,
    // casings, laser trails, GunModule pickups and their texts.
    downwell::CompactGuns guns_;
    int aimAngle_=0;           // objPlayer_n.aimAngle (scrAimControl)
    int shotAngle_=270;        // scrPlayerShootN's shotAngle, reused by Alarm 3
    int burstAlarm_=-1;        // objPlayer_n alarm[3] (scrBurstFire)
    unsigned burstCount_=0;
    bool gunsActive_()const{return compactGunsAdmitted;}
    const downwell::GunStats& currentGun()const{return downwell::gunStats(run_.gunType,run_.gemHigh);}
    BulletState* birthBullet(int x,int y,int sprite);
    bool emitPlayerBullets(int muzzleX,int muzzleY,int playerY,int angle,bool lastGetsLaserSight);
    bool createCompactBullet(int x,int y,int angle,bool laserSight);
    bool firePlayerGun(int playerX,int playerY,int muzzleX,int muzzleY);
    bool burstFire();
    bool advanceBulletAnimation(BulletState& bullet);
    bool stepRangedBullet(BulletState& bullet,long generationCameraY);
    bool stepLaserBullet(BulletState& bullet,long generationCameraY);
    bool typicalBulletDamage(EnemyState& enemy,int damage,std::uint32_t direction,
        downwell::MotionBinary64 x,downwell::MotionBinary64 y,bool& consumed);
    bool collideHotCasings();
    bool bulletEnemyContact(BulletState& bullet,const downwell::ProjectileSegment& path,bool& consumed);
    bool bulletSolidContact(BulletState& bullet,const downwell::ProjectileSegment& path,bool& consumed);
    bool createGunModule(const downwell::DeferredCreateInstance& command);
    bool advanceCompactGunAlarms();
    bool advanceCompactGunAnimation();
    bool stepGunModules();
    bool stepCasings();
    bool stepModuleTexts();
    bool collideGunModules();
    bool pickupGunModule(downwell::CompactGunModuleRecord& module);
    bool retireCompactGuns(long viewTopQ12);
    bool drawCompactGunTexts();
    bool hitGunModule(const downwell::ContactBounds& shot,int damage,bool& consumed);
    bool reverseEngineerModule(downwell::CompactGunModuleRecord& m);
    // Compact time-stop (W1.5): objPlayer_n's checkStop/pTimeStop block,
    // scrTimeShard's fxTimeShard 238 and fxTimeChange 211's room-speed ramp.
    // Field queries use integer contact rectangles against the serialized
    // 82/255/371 masks instead of the runner's per-pixel sampling.
    downwell::TimeStopEffects<downwell::gameplayCompactShardCapacity> timeStop_;
    unsigned pauseMilliseconds_=0;
    struct FieldHit{bool hit=false;int x=0,y=0;};
    bool compactTimeStop()const{return compactTimeStopAdmitted&&!playerTimeTransition_;}
    bool stopped()const{return timeStop_.player.stopped;}
    FieldHit fieldAt(const downwell::ContactBounds& bounds)const;
    // Ordinary fields sorted by centre y, rebuilt when the builder adds
    // fields: a query tests only the 48-pixel y window, then keeps the lowest
    // creation index so instance_place order is unchanged.
    struct FieldIndex{std::uint32_t birth=0;unsigned count=~0u;unsigned char order[64];int y[64];};
    mutable FieldIndex fieldIndex_;
    bool refreshFieldIndex()const;
    bool fieldNearY(int y,int reach)const;
    // TimeStopBound for a rectangle and its published x: true when it runs.
    bool timeStopBound(const downwell::ContactBounds& bounds,int x)const;
    bool timeStopBound(int sprite,int x,int y)const;
    bool beginCompactTimeStop(int publishedX,int publishedY);
    bool stepTimeShards(downwell::TimeShardEffects<downwell::gameplayCompactShardCapacity>::Phase phase);
    using TimeChangePhase=decltype(timeStop_.changes.beginPhase());
    using TimeShardPhase=decltype(timeStop_.shards.beginPhase());
    bool advanceTimeEffectAlarm(TimeChangePhase changePhase,TimeShardPhase shardPhase,unsigned alarm);
    bool collideTimeShards();
    bool collideEnemyTimeFields();
    bool freezeEnemyStep(EnemyState& enemy);
    bool holdStoppedBoxAlarms();
    bool resetCompactTimeStop();
    bool resetCompactScenery();
    bool createCompactScenery(const downwell::DeferredCreateInstance& command);
    bool advanceCompactSceneryAlarm(unsigned alarm);
    bool advanceCompactSceneryAnimation();
    bool stepCompactBox(unsigned index);
    bool stepCompactScenery();
    bool collideCompactScenery();
    bool hitGemChunk(const downwell::ProjectileSegment& path,int damage,bool& hit,bool& consumed);
    bool hitCompactShootable(const downwell::ProjectileSegment& path,const BulletState& bullet,bool& consumed);
    bool publishSceneryRepaint(int x,int y);
    int spawnCompactCurrency(int amount,int x,int y,bool attracted);
    int spawnCompactBurstGem(int x,int y,long xRange,long xLow,long yRange,long yLow,int amount,bool large);
    bool attractCompactGem(int slot);
    bool spawnCompactDebris(downwell::MotionBinary64 x,downwell::MotionBinary64 y,std::uint32_t& handle);
    bool solidMeeting(int mask,downwell::MotionBinary64 x,downwell::MotionBinary64 y,bool platforms,bool& hit);
    bool compactInView(int x,int y,bool& visible)const;
public:
    const CompactEffects& compactEffects()const{return compactEffects_;}
    const downwell::DebrisEffects<downwell::gameplayCompactDebrisCapacity>& compactDebris()const{return compactDebris_;}
    const downwell::CorpseEffects<downwell::gameplayCompactCorpseCapacity>& compactCorpses()const{return compactCorpses_;}
    const downwell::CompactScenery& compactScenery()const{return scenery_;}
    const downwell::CompactGuns& compactGuns()const{return guns_;}
    const downwell::CompactBlastRecord* compactBlasts(unsigned& end)const{end=blastEnd_;return blasts_;}
    const downwell::CompactBloodRecord* compactBlood(unsigned& end)const{end=bloodEnd_;return bloods_;}
    const downwell::CompactDroneRecord& compactDrone()const{return drone_;}
    const downwell::CompactBalloonRecord& compactBalloon()const{return balloon_;}
    // Effect-surface producers among the W1.9 owners.
    bool compactSurfaceUpgrades()const{
        if(drone_.live||balloon_.live||player_.jetpacking())return true;
        for(unsigned i=0;i<blastEnd_;++i)if(blasts_[i].live&&blasts_[i].object!=276)return true;
        return false;
    }
    // W1.7 compact combo and controller HUD. The platform supplies the
    // language's area titles (langString("area" + n)); without them the
    // title draws nothing, as the stored levelNumber owner is absent too.
    void configureCompactAreaTitles(const downwell::AreaTitle* titles,unsigned count,const char* language){
        compactAreaTitles_=titles;compactAreaTitleCount_=count;compactLanguage_=language;
    }
    const downwell::CompactLevelTitle& compactLevelTitle()const{return compactTitle_;}
    // The merchant's speech strings in the configured language; without
    // them the speech records live and move but draw nothing.
    bool configureCompactShopSpeech(const downwell::ShopLanguageEntry* entries,unsigned count,const char* language){
        return shopSpeechText_.configure(entries,count,language);
    }
    // One fxRisingText per shop (indexed like ShopWorld::instances).
    const downwell::CompactShopSpeech* compactShopSpeech()const{return shopSpeech_;}
    unsigned compactShopSpeechLive()const{return shopSpeechLive_;}
    static constexpr unsigned compactComboRewardCapacity=4;
    const downwell::CompactComboRewardRecord* compactComboRewards()const{return compactRewards_;}
    unsigned compactComboRewardsLive()const{return liveRewards_;}
    unsigned comboCount()const{return run_.combo.state.count;}
    // scrDrawHud4x3Top's streak block and scrDrawHud4x3's currencyText, once
    // per Draw; returns whether the block draws.
    bool drawGemStreakHud(){
        gemHud_.countCurrency(currencyAccount_.currency);
        return gemHud_.draw(currencyAccount_.streak,currencyAccount_.timer,dead(),stopped());
    }
    const downwell::CompactGemStreakHud& gemStreakHud()const{return gemHud_;}
    unsigned currencyText()const{return gemHud_.currencyText;}
    // playerDraw's gem-high afterimages (shaderAfterImage1: white texels
    // become red on odd room columns), oldest first: draw index i from
    // count-1 down to 0 at (x + dx, y + dy).
    unsigned afterImagesDrawn()const{return afterImagesDrawn_;}
    // i = 0 is the newest record.
    const CompactAfterImage& afterImage(unsigned i)const{
        unsigned at=afterImageHead_+i;if(at>=afterImageCount)at-=afterImageCount;return afterImages_[at];
    }
    // The player's sprite, subimage, xscale and room position as this
    // update's Draw shows them (set at the Draw boundary).
    const CompactAfterImage& presentedPlayerPose()const{return presentedPose_;}
    int afterImageDx(unsigned i)const{return afterImageDx_[i];}
    int afterImageDy(unsigned i)const{return afterImageDy_[i];}
    // global.pTimeStop, room_speed and the sleep() milliseconds requested
    // since the last call (fxTimeChange Create). PAL frontends pace with them.
    bool timeStopped()const{return stopped();}
    int roomSpeed()const{return timeStop_.roomSpeed;}
    unsigned takePauseMilliseconds(){const unsigned ms=pauseMilliseconds_;pauseMilliseconds_=0;return ms;}
    const downwell::TimeShardEffects<downwell::gameplayCompactShardCapacity>& compactTimeShards()const{return timeStop_.shards;}
    int gunType()const{return run_.gunType;}
    int aimAngle()const{return aimAngle_;}
    // Current builder art for one generated scenery decoration, by its
    // Create position. False: the instance no longer draws (destroyed).
    bool sceneryAppearance(int objectId,int x,int y,int& sprite,int& frame)const;
    // gemChunk is a parentWall until it opens and clears its mask.
    bool gemChunkSolidAt(int x,int y)const;
    // Player terrain queries in room coordinates (mask 569: -9..+9, -7..+8).
    bool gemChunkBlocks(int x,int y)const{
        for(unsigned i=0;i<scenery_.gemChunkEnd;++i){const auto& r=scenery_.gemChunks[i];
            if(r.live&&!r.opened&&x>=r.x-9&&x<r.x+10&&y>=r.y-7&&y<r.y+9)return true;
        }
        return false;
    }
    // Landing offset on an unopened chunk's top for the player's -4..+3 span.
    int gemChunkLanding(int x,int minimumOffsetY,int landing)const{
        for(unsigned i=0;i<scenery_.gemChunkEnd;++i){const auto& r=scenery_.gemChunks[i];
            const int top=r.y-191;
            if(r.live&&!r.opened&&x+3>=r.x-9&&x-4<r.x+10&&top>=minimumOffsetY&&top<landing)landing=top;
        }
        return landing;
    }
    // Paired host comparisons whose StartupRuntime reference does not run
    // object 78 clear this before generation. Production never changes it.
    static inline bool compactSpawnerAdmitted=true;
    // Likewise for references predating W1.3's box bursts and builder
    // scenery: boxes keep the removal-only subset, 289/293/296 stay inert.
    static inline bool compactRoomEffectsAdmitted=true;
    // Likewise for references predating W1.4: the retained usual-gun shot
    // (legacy trajectory, no casing, shake or nonAuto) and an inert 109.
    static inline bool compactGunsAdmitted=true;
    // Likewise for references predating W1.5: pTimeStop never turns on.
    static inline bool compactTimeStopAdmitted=true;
    // W1.6 effects that draw RNG (death blood, smoke, flash and corpse, bullet
    // impacts, empty-charge smoke). Comparisons against references without
    // those chains turn this off; hit-stop, dust and wall hits stay.
    static inline bool compactDamageEffectsAdmitted=true;
    // objBuilder.endReached, published by the generation frontend.
    void publishGenerationEndReached(bool reached){generationEndReached_=reached;}
    // Red-ball Draw's eye, after the most recent Step. False when absent or
    // before any sector has initialized it (the source has no default).
    bool redBallEye(const EnemyState& enemy,int& sprite,int& mirror)const;
};

#endif
