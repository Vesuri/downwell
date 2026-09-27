#ifndef DOWNWELL_CORE_STARTUP_RUNTIME_H
#define DOWNWELL_CORE_STARTUP_RUNTIME_H

#include "AreaOneBuilder.h"
#include "InstanceStore.h"
#include "DeferredInstanceQueue.h"
#include "GameMakerRandom.h"
#include "GameplayLimits.h"
#include "GemMotion.h"
#include "ProjectileAnimation.h"
#include "ProjectileSegment.h"
#include "LaserSightRay.h"
#include "MachineGunEmitter.h"
#include "CurrencyAccount.h"
#include "MainCameraMotion.h"
#include "PlayerDeathSequence.h"
#include "RunSession.h"
#include "PlayerLifeRules.h"
#include "Input.h"
#include "PlayerMotion.h"
#include "StoredTimeShardRegistration.h"
#include "StoredTimeChangeRegistration.h"
#include "TimeChangeEffects.h"
#include "TimeStopEffects.h"
#include "TimeShardBurst.h"
#include "RunnerCollisionEventContext.h"
#include "SmokeDirection.h"
#include "SmokeTrig.h"
#include <array>
#include <cstdint>

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
    // Call at a room-transition boundary, after consuming the results action.
    // Seed is explicit so randomize can be recorded/replayed by the platform.
    bool restartFromResults(DeathMenuSequence::Action action, std::uint32_t seed);
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
    bool hasInstance(std::uint32_t instanceId) const {
        return instances_.find(instanceId) != nullptr;
    }
    const InstanceRecord* instance(std::uint32_t instanceId) const {
        return instances_.find(instanceId);
    }
    const InstanceRecord& instanceAt(std::size_t index) const {
        return instances_.at(index);
    }
    bool roomDestinationSet() const { return roomDestinationSet_; }
    bool roomLoadSucceeded() const { return roomLoadSucceeded_; }
    bool recoveredEventBindingsActive() const { return objectPack_ != nullptr; }
    std::uint32_t implementedCodeCalls() const { return implementedCodeCalls_; }
    std::uint32_t unimplementedCodeCalls() const { return unimplementedCodeCalls_; }
    int cameraViewWidth() const { return cameraViewWidth_; }
    int cameraViewHeight() const { return cameraViewHeight_; }
    int backgroundMusicId() const { return backgroundMusicId_; }
    bool controlsLocked() const { return controlsLocked_; }
    std::uint32_t soundGainRequests() const { return soundGainRequests_; }
    std::uint32_t soundPlayRequests() const { return soundPlayRequests_; }
    std::uint32_t ditherFadeSpawnRequests() const {
        return ditherFadeSpawnRequests_;
    }
    std::uint32_t nextRuntimeInstanceId() const { return nextRuntimeInstanceId_; }
    bool deferredCommandsSucceeded() const { return deferredCommandsSucceeded_; }
    // Engine x/y setter for collision response. Keeps private actor motion and
    // preciseX/Y motion caches untouched; the collision owner stores exact x/y.
    bool publishCollisionPosition(InstanceHandle h,RunnerImageValue x,RunnerImageValue y);
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
    // Scoped animation/alarm/Step ownership for one ordinary world tick. No borrowed
    // callback survives the call. The optional collision pass shares this tick;
    // callers must route all admitted handlers, not silently discard unknowns.
    // The optional
    // transition callback runs at the shared player's source Begin Step boundary.
    struct NoTimeTransition {bool operator()(const InstanceRecord&,const PlayerMotion&,int,int)const{return true;}};
    struct NoCollisionPass {bool operator()()const{return true;}};
    enum class TimeEffectPhase {animation,alarm,step,playerBegin};
    template<unsigned Capacity,unsigned Registered,unsigned Changes,class Transition=NoTimeTransition,class Collisions=NoCollisionPass> bool stepWithTimeStopEffects(
        StoredTimeShardRegistration<instanceCapacity,Registered>& shards,
        StoredTimeChangeRegistration<instanceCapacity,Changes>& changes,TimeStopEffects<Capacity>& state,
        const Input& input,bool advancePressed=false,Transition transition={},Collisions collisions={}){
        if(!run_.effectsSucceeded)return false;
        if(timeEffectDispatch_||!state.succeeded||!state.shards.succeeded()||!state.changes.succeeded()||
            !shards.belongsTo(instances_)||!changes.belongsTo(instances_)||
            !state.shards.phasesBoundTo(shards)||!state.changes.boundTo(changes)){
            run_.effectsSucceeded=false;return false;
        }
        auto dispatch=[&](InstanceRecord& record,TimeEffectPhase phase,unsigned index){
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
        const bool ok=stepWithCollisionPass(input,advancePressed,collisions);
        timeEffectDispatch_=nullptr;timeEffectContext_=nullptr;
        return ok;
    }
    // Explicit Begin Step boundary; the scheduler supplies synchronous audio,
    // shake, pause and oxygen effects. Registrations must
    // outlive the effect owner and be retired before room teardown.
    template<unsigned Capacity,unsigned Registered,unsigned Changes,class Audio,class Emit>
    bool beginTimeStop(StoredTimeShardRegistration<instanceCapacity,Registered>& shards,
        StoredTimeChangeRegistration<instanceCapacity,Changes>& changes,TimeStopEffects<Capacity>& state,
        const typename TimeStopEffects<Capacity>::PlayerPose& pose,
        bool lowSpec,Audio audio,Emit emit){
        if(!run_.effectsSucceeded)return false;
        run_.effectsSucceeded=shards.belongsTo(instances_)&&changes.belongsTo(instances_)&&
            state.shards.phasesBoundTo(shards)&&state.changes.boundTo(changes)&&
            state.beginStepChecked(roomId_==11||roomId_==14,[&](bool& outside){
                    return playerOutsideTimeStopSides(pose.xx,outside);
                },pose,lowSpec,random_,
                [&](bool& hit){return playerTimeFieldMeeting(pose.xx,pose.yy,hit);},
                [&](auto x,auto y,auto& id,auto& fx,auto& fy){
                    InstancePlaceResult result;
                    if(!playerTimeFieldInstance(x,y,result))return false;
                    id=result.id;fx=result.x;fy=result.y;return true;
                },audio,emit);
        return run_.effectsSucceeded;
    }
    // The caller must keep the returned bridge alive and retire its shards
    // before changing rooms. Phase dispatch is intentionally not supplied here.
    template<unsigned Capacity> StoredTimeShardRegistration<instanceCapacity,Capacity> timeShardRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
    }
    template<unsigned Capacity> StoredTimeChangeRegistration<instanceCapacity,Capacity> timeChangeRegistration(){
        return {instances_,objectPack_,lifecycle_,nextRuntimeInstanceId_,roomId_,run_.effectsSucceeded};
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
    int mainNextMuzzleOffset() const { return mainEmitter_.leg; }
    const Consumables& consumables() const { return run_.consumables; }
    ShopWorld::Instance* registerShop(int x, int y) {
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
    bool effectsSucceeded()const{return run_.effectsSucceeded;}
    bool emitSmokeBurst(MotionBinary64 x,MotionBinary64 y){return run_.emitSmokeBurst(x,y,random_);}
    bool emitFlashBurst(MotionBinary64 x,MotionBinary64 y) {
        return run_.emitFlashBurst(x,y,random_);
    }
    bool registerSideCameraLocker(int x, int y) { return run_.sideCameraLockers.create(x, y); }
    const OrdinaryTimeFields<64>& timeFields() const {return run_.timeFields;}
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
    bool enterLevelClear(unsigned style, bool heartGoal) {
        if (!run_.enterLevelClear(style, heartGoal, mainPlayerHealth_, mainPlayerMotion_, random_)) return false;
        mainAmmo_ = mainPlayerMotion_.ammo();
        return true;
    }
    template<class Emit>
    bool stepLevelClear(bool& confirm, bool leftPressed, bool rightPressed, bool& heartGoal, Emit emit) {
        if (!run_.levelClearActive || run_.nextLevelRequested) return false;
        ++tick_;
        const bool ok = run_.stepLevelClear(confirm, leftPressed, rightPressed, heartGoal,
            currencyAccount_, mainPlayerMotion_, mainPlayerHealth_, random_, emit);
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
    void updateMainBuilder(InstanceRecord& instance);
    void syncMainBuilderState();
    void syncMainPlayerState();
    bool mainSolidAt(float x, float y) const;
    void emitMainBullet(int x,int y);
    void updateMainBullets();
    bool publishMainInactivation(const InstanceRecord& record);
    bool publishMainEnemyImage(std::size_t index,unsigned phase);
    bool publishMainEnemyTransform(std::size_t index,RunnerTransformField field,int value);
    void spawnMainCurrency(float x, float y, std::int32_t amount,
                           bool attracted = false);
    void updateMainCurrency();
    void updateMainLifecycle();
    void updateMainEnemies();
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
    void resetSceneState();
    InstanceRecord* initRoomInstance();
    const InstanceRecord* initRoomInstance() const;

    std::uint64_t tick_ = 0;
    int roomId_ = entryRoomId;
    int roomDestination_ = 0;
    int showSplash_ = 0;
    InstanceStore<instanceCapacity> instances_;
    InstanceLifecycleBinding lifecycle_;
    void* collisionContext_=nullptr;
    bool (*collisionDispatch_)(void*)=nullptr;
    void* timeEffectContext_=nullptr;
    bool (*timeEffectDispatch_)(void*,InstanceRecord&,TimeEffectPhase,unsigned)=nullptr;
    const RoomPack* roomPack_ = nullptr;
    const ObjectPack* objectPack_ = nullptr;
    // Object event bindings are immutable. Cache inherited alarm masks, not
    // per-instance alarm values; IDs outside this cache use normal lookup.
    std::array<std::int16_t,512> alarmEventMasks_{};
    static_assert(AlarmBank::count < 16, "Alarm cache reserves negative values for misses");
    const BuilderPatternPack* builderPatternPack_ = nullptr;
    std::uint32_t randomSeed_ = defaultRandomSeed;
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
    int cameraViewHeight_ = 0;
    int backgroundMusicId_ = -1;
    bool controlsLocked_ = false;
    std::uint32_t soundGainRequests_ = 0;
    std::uint32_t soundPlayRequests_ = 0;
    std::uint32_t ditherFadeSpawnRequests_ = 0;
    // A single 21x18 recovered side-room pattern can emit more than 128
    // objects before GameMaker returns to the step loop.
    DeferredInstanceQueue<512> deferredInstances_;
    std::uint32_t nextRuntimeInstanceId_ = 105186;
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
    float mainCameraY_ = 176.0F;
    float mainCameraTargetY_ = 176.0F;
};

} // namespace downwell

#endif
