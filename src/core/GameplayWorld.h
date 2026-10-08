#ifndef DOWNWELL_CORE_GAMEPLAY_WORLD_H
#define DOWNWELL_CORE_GAMEPLAY_WORLD_H

#include "PlayerMotion.h"
#include "PreparedRoomTerrain.h"
#include "ScriptEffectRuntimeBinding.h"
#include "Profile.h"
#include "core/GameMakerRandom.h"
#include "core/StateDigest.h"
#include "core/GameplayLimits.h"
#include "core/GemMotion.h"
#include "core/ProjectileAnimation.h"
#include "core/MachineGunTrajectory.h"
#include "core/ProjectileSegment.h"
#include "core/LaserSightRay.h"
#include "core/MachineGunEmitter.h"
#include "core/CurrencyAccount.h"
#include "core/MainCameraMotion.h"
#include "core/PlayerDeathSequence.h"
#include "core/RunSession.h"
#include "core/GameplayGoalFields.h"
#include "core/GameplayExitControllers.h"
#include "core/GameplayEntryDithers.h"
#include "core/DroneBulletInstances.h"
#include "core/BoneProjectileInstances.h"
#include "core/SkeletonInstances.h"
#include "core/RegisteredSkeletonThrow.h"
#include "core/BoneViewRetirement.h"
#include "core/GameplayScriptSpawnInstances.h"
#include "core/GameplayDebrisInstances.h"
#include "core/GameplayTimeChangeRegistration.h"
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
    };
    using PlayerLifecycleBinding = downwell::RecordLifecycleBinding<PlayerInstance>;
    // When supplied, entryRooms is borrowed for this world and its run resets;
    // it must remain immutable and prepares rmMain's initial view cache before
    // player lifecycle callbacks. The terrain handoff reuses that same snapshot.
    explicit GameplayWorld(unsigned initialRingOrigin,
                              bool startsGrounded = true,
                              int landingOffset = 0,
                              bool followCamera = false,
                              const EnemyLifecycleBinding& enemyLifecycle = {},
                              const BreakableLifecycleBinding& breakableLifecycle = {},
                              const PlayerLifecycleBinding& playerLifecycle = {},
                              downwell::GameplayTerrainInstances* terrainInstances = nullptr,
                              const downwell::RoomPack* entryRooms = nullptr);
    const downwell::GameplayTerrainInstances* terrainInstances()const{return terrainInstances_;}
    // Without terrain instances, wall/platform Creates (60, 93) have no world
    // state: createGeneratedTerrain reduces to the backend's own publication.
    bool terrainCreatesBackendOnly()const{return run_.effectsSucceeded&&!terrainInstances_;}
    const downwell::GameplayScriptSpawnInstances* scriptSpawnInstances()const{return scriptSpawnInstances_;}
    const downwell::GameplayDebrisInstances* debrisInstances()const{return debrisInstances_;}
    const downwell::NativeTimeChangeRegistration* timeChangeInstances()const{return timeChangeInstances_;}
    const downwell::GameplayEntryDithers* entryDithers()const{return entryDithers_;}
    using DroneBullets=downwell::DroneBulletInstances<32>;
    static constexpr unsigned droneBulletCapacity=32;
    const DroneBullets* droneBullets()const{return droneBullets_;}
    bool bindDroneBullets(DroneBullets& bullets){
        if(droneBullets_||!bullets.valid()||bullets.size())return false;
        droneBullets_=&bullets;return true;
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
    template<class MakeEffects> bool stepSkeletonRoomEffect(downwell::InstanceHandle key,bool stopped,
        downwell::MotionBinary64 playerX,downwell::MotionBinary64 playerY,MakeEffects makeEffects){
        if(!run_.effectsSucceeded||!skeletons_||!skeletons_->valid())return run_.effectsSucceeded=false;
        const bool ok=skeletons_->dispatchPublished(key,[&](auto& actor,auto& throwing,auto& publish,bool& good){
            auto effects=makeEffects(actor,throwing);
            return actor.stepPublished(stopped,playerX,playerY,publish,effects,good);
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
    bool bindEntryDithers(downwell::GameplayEntryDithers& entries){
        if(entryDithers_||!entries.valid()||entries.size())return false;
        entryDithers_=&entries;return true;
    }
    bool bindTimeChangeInstances(downwell::NativeTimeChangeRegistration& registry);
    template<class Pause> bool createTimeChange(downwell::TimeChangeEffects<8>& effects,
        downwell::MotionBinary64 x,downwell::MotionBinary64 y,int& speed,Pause pause){
        if(!run_.effectsSucceeded)return false;
        if(!timeChangeInstances_||!effects.boundTo(*timeChangeInstances_)||
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
    bool bindDebrisInstances(downwell::GameplayDebrisInstances& instances){
        if(debrisInstances_||!instances.valid()||instances.size())return false;
        debrisInstances_=&instances;return true;
    }
    // Borrowed fixed storage must outlive the registered particles and retry.
    // Natural combat admission follows after source Step/Draw integration.
    template<unsigned Capacity>
    bool spawnDebris(downwell::DebrisEffects<Capacity>& effects,downwell::MotionBinary64 x,
        downwell::MotionBinary64 y,int area,downwell::InstanceHandle& result){
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
        const auto h=effects.createRegistered(x,y,area,enemyRandom_,*debrisInstances_);
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
    const downwell::ScriptSpawnEffects<downwell::scriptSpawnEffectCapacity>& scriptEffects()const{return ownedScriptEffects_;}
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
    enum class DrawActor { enemy, bullet, scriptEffect, debris, player, entryDither, bone, skeleton };
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
    void configureRoomAnimation(bool (*callback)(void*),void* context){roomAnimation_=callback;roomAnimationContext_=context;}
    void configureDroneEndStep(bool (*callback)(void*,long),void* context){droneEndStep_=callback;droneEndStepContext_=context;}
    bool advanceEnemyRoomAnimation(downwell::InstanceHandle key);
    bool advanceEnemyRoomAlarm(downwell::InstanceHandle key,unsigned alarm);
    bool advanceBoxRoomAlarm(downwell::InstanceHandle key,unsigned alarm);
    void configureRoomAlarms(bool (*callback)(void*),void* context){roomAlarms_=callback;roomAlarmsContext_=context;}
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
        for(unsigned i=0;i<count;++i){
            downwell::PackedRoomInstance decoded;
            if(!retained&&!rooms.roomInstance(room,i,decoded)){run_.effectsSucceeded=false;return false;}
            const auto& instance=retained?prepared->at(i):decoded;
            if(instance.objectId==35){
                // The native scheduler currently births its player before this
                // handoff. Retain the authored identity here without replaying
                // Create or inventing a numeric ID from its lifetime handle.
                // Full serialized cross-family birth ordering is still required.
                if(!instance.instanceId||!playerInstance_.lifetimeId||playerInstance_.sourceInstanceId){
                    run_.effectsSucceeded=false;return false;
                }
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
        }
        return true;
    }
    const downwell::RoomEntryViews::View* roomEntryView(unsigned slot=0)const{
        return roomEntryViews_.get(slot);
    }
    downwell::InstanceHandle playerHandle() const { return {0,playerInstance_.lifetimeId}; }
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
    bool publishBoxHealth(downwell::InstanceHandle h,downwell::MotionBinary64 health){
        if(!run_.effectsSucceeded||!findBreakable(h)||!health.valid())return run_.effectsSucceeded=false;
        breakables_[h.slot].state.wallHp=health;return true;
    }
    int overlapSound(unsigned channel)const{return channel<11?run_.overlapSounds[channel]:-1;}
    template<class Audio> bool playBoxBreakSound(downwell::InstanceHandle h,downwell::MotionBinary64 volume,Audio& audio){
        if(!run_.effectsSucceeded||!findBreakable(h))return run_.effectsSucceeded=false;
        auto& box=breakables_[h.slot];
        return run_.playBoxBreakSound(box.sound,box.state.breakSound,volume,audio);
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
    // Engine destruction mark, after any game Destroy/CleanUp callbacks.
    // Keep the typed record until the owner's Draw-entry cleanup.
    bool markEnemyDestroyed(downwell::InstanceHandle handle);
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
        const downwell::BreakableState* creation=nullptr);
    bool appendGeneratedBreakable(int x,int y,const downwell::GeneratedBreakableCreate& creation);
    void configureSolidResolver(SolidResolver resolver, void* context, SegmentResolver segment = nullptr,
                                SolidResolver mask = nullptr,WallContactResolver walls = nullptr) {
        solidResolver_ = resolver;
        segmentResolver_ = segment;
        maskResolver_ = mask;
        wallContactResolver_ = walls;
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
    unsigned gameGemTotal() const { return currencyAccount_.gameGem; }
    unsigned gemStreak() const { return currencyAccount_.streak; }
    unsigned gemStreakTimer() const { return currencyAccount_.timer; }
    int health() const { return health_; }
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
    bool bindTimeFieldLifecycle(downwell::OrdinaryTimeFields<64>::Binding binding){
        if(!run_.effectsSucceeded)return false;
        run_.effectsSucceeded=run_.timeFields.bindLifecycle(binding);return run_.effectsSucceeded;
    }
    bool cameraSideLocked() const { return cameraMotion_.sideLocked; }
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
    bool retireMarkedGoalField(downwell::InstanceHandle h){
        return run_.effectsSucceeded=run_.effectsSucceeded&&goalFieldInstances_.retireMarked(h);
    }
    // Finish the supported room-entry Create sequence. The indexed owner
    // reconciles then sweeps in room order; unindexed fixtures own local cleanup.
    bool createRoomStartField(){
        if(!run_.effectsSucceeded)return false;
        downwell::InstanceHandle h;
        if(!goalFieldInstances_.create(240,192,371,&h))return run_.effectsSucceeded=false;
        if(!run_.createStartField(240,192,enemyRandom_))return false;
        if(!run_.startFieldPresent&&!goalFieldInstances_.requestDestroy(h))return run_.effectsSucceeded=false;
        if(!reconcilePhase(PhaseBoundary::roomEntry))return false;
        return run_.effectsSucceeded=goalFieldInstances_.cleanupMarked();
    }
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
        if(!run_.effectsSucceeded||!clearBulletOwnership()||!goalFieldInstances_.clear()||!exitControllers_.clear())return run_.effectsSucceeded=false;
        return run_.enterLevelClear(style, heartGoal, health_, player_, enemyRandom_);
    }
    template<class Emit>
    bool stepLevelClear(bool& confirm, bool leftPressed, bool rightPressed, bool& heartGoal, Emit emit) {
        if (!run_.levelClearActive || run_.nextLevelRequested) return false;
        ++tick_;
        return run_.stepLevelClear(confirm, leftPressed, rightPressed, heartGoal,
            currencyAccount_, player_, health_, enemyRandom_, emit);
    }
    bool levelClearActive() const { return run_.levelClearActive; }
    bool nextLevelRequested() const { return run_.nextLevelRequested; }
    // Retire the old display first. This resets only room-local runtime state;
    // the owner must rebuild terrain and rebind generation/collision callbacks.
    bool prepareNextAreaOneLevel(const downwell::Input& input, unsigned initialRingOrigin);
    template<class Builder>
    bool initializeAreaOneBuilder(Builder& builder, int x, int y, unsigned style = 0) {
        if(!run_.effectsSucceeded)return false;
        if (run_.stage.area != 1 || run_.stage.level < 1 || run_.stage.level > 3 || style > 4) return false;
        enemyRandom_.prepareLookahead(randomLookahead_);
        downwell::AreaOneBuilder::Setup setup;
        setup.level = unsigned(run_.stage.level);
        setup.style = style;
        setup.member = run_.upgrades.levels[14] != 0;
        builder.resetRoom(x, y, enemyRandom_, setup);
        return createRoomStartField();
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
    bool (*droneEndStep_)(void*,long)=nullptr;
    void* droneEndStepContext_=nullptr;
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
    downwell::GameplayScriptSpawnInstances* scriptSpawnInstances_=nullptr;
    downwell::GameplayDebrisInstances* debrisInstances_=nullptr;
    downwell::NativeTimeChangeRegistration* timeChangeInstances_=nullptr;
    downwell::GameplayEntryDithers* entryDithers_=nullptr;
    DroneBullets* droneBullets_=nullptr;
    Bones* bones_=nullptr;
    Skeletons* skeletons_=nullptr;
    downwell::TimeChangeEffects<8>* timeChangeEffects_=nullptr;
    int* timeChangeSpeed_=nullptr; // Borrowed source room-speed global, not PAL catch-up.
    bool clearRoomTimeChanges();
    bool roomTerrainLoaded_=false;
    downwell::RoomEntryViews roomEntryViews_;
    // Borrowed resource pack survives in-place run recreation. Its initial
    // view cache must be available before any player birth callbacks.
    const downwell::RoomPack* entryRooms_=nullptr;
    PlayerInstance playerInstance_;
    std::uint32_t nextPlayerLifetimeId_=1;
    bool birthPlayerOwnership();
    bool retirePlayerOwnership();
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
    downwell::GameplayGoalFields goalFieldInstances_;
    downwell::GameplayExitControllers exitControllers_;
    downwell::MachineGunEmitter emitter_;
    downwell::CurrencyAccount currencyAccount_;
    SolidResolver solidResolver_;
    SegmentResolver segmentResolver_ = nullptr;
    SolidResolver maskResolver_ = nullptr;
    WallContactResolver wallContactResolver_ = nullptr;
    void* solidResolverContext_;
    PlayerMotion::LandingResolver landingResolver_ = nullptr;
    void* landingResolverContext_ = nullptr;
    Breakable breakables_[breakableCapacity];
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
