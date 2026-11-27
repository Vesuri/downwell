#include "GameplayWorld.h"
#include "GameplayCollisionOwner.h"
#include "PlayerBoxContact.h"
#include "PlayerHorizontalBinary64.h"
#include "DivideBinary64.h"
#include "NarrowAtan.h"
#include "UnitSequenceFrames.h"
#include "GameplayPlayerStep.h"
#include "core/MainRoomLifetime.h"
#include "core/MainRoomActivation.h"
#include "Profile.h"

#include "core/PortCheckpoint.h"
#include "core/GameMakerRounding.h"
#include "core/EnemyPresentation.h"
#include "core/EnemyCreateProfile.h"
#include "core/EnemyStepPresentation.h"
#include "core/EnemyContact.h"
#include "core/EnemyContactResponse.h"
#include "core/CurrencyReward.h"
#include "core/CurrencyStep.h"
#include "core/EnemyAlarmEvents.h"
#include "core/AlarmBank.h"
#include "core/FrogBehavior.h"
#include "core/FrogMotion.h"
#include "core/FrogSourceStep.h"
#include "core/StandardBulletStep.h"
#include "core/StandardBulletImpact.h"
#include "core/PlayerLifeRules.h"
#include "core/StableInstanceSlots.h"
#include "core/FrogVerticalMotion.h"
#include "core/BatMotion.h"
#include "core/Replay.h"
#include "core/SolidCollision.h"
#include "core/NarrowArithmetic.h"
#include "PlacementNew.h"
#include "core/ProjectilePublication.h"
#include "core/BoxDestructionScript.h"
#include "core/CurrencyCollisionBounds.h"
#include "core/BoneViewRetirement.h"
namespace {
// W3.20: binary64 producers of the Q16 effect owners convert at the call;
// these go as stages 3 and 4 convert the producers themselves.
bool q16(downwell::MotionBinary64 v,downwell::Fixed16& out){return downwell::Fixed16::fromBinary64(v,out);}
// Image speeds stay binary32 (the source's image_speed), so the animation
// timeline is unchanged: 0.5, 0.4, 0.3 and 1.
constexpr downwell::RunnerImageValue half{0x3f000000u},fourTenths{0x3ecccccdu},threeTenths{0x3e99999au},unitSpeed{0x3f800000u};
}

downwell::ResultsDestination GameplayWorld::restartFromResults(
        downwell::DeathMenuSequence::Action action, std::uint32_t seed, unsigned initialRingOrigin) {
    const auto destination = downwell::resultsDestination(action);
    if (destination == downwell::ResultsDestination::none) return destination;
    if(!runAudio_.reset()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    const auto runAudio=runAudio_;
    const auto encounterUpdateOwner=encounterUpdateOwner_;
    const auto encounterCollision=collisionPass_;
    auto* const encounterCollisionContext=collisionPassContext_;
    auto* const encounterInput=encounterInput_;
    if(encounterInput)encounterInput->resetPublication();
    if(levelNumbers_&&!levelNumbers_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(limboShards_&&!limboShards_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(registeredFlash_&&!registeredFlash_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(resultInstance_&&!resultInstance_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(!clearRoomTimeChanges()||!clearRoomTimeShards())return downwell::ResultsDestination::none;
    if(entryDithers_&&!entryDithers_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    compactEntryLive_=false;
    if(ditherAbove_&&!ditherAbove_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(builderController_&&!builderController_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(playerControllers_&&!playerControllers_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(scattered_&&!scattered_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(spawners_&&!spawners_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(whiteJellies_&&!whiteJellies_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(redBalls_&&!redBalls_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(shopInstances_&&!shopInstances_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(movingEffects_&&!movingEffects_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(droneBullets_&&!droneBullets_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(bones_&&!bones_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(registeredExpiry_&&!registeredExpiry_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(registeredSmoke_&&!registeredSmoke_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(registeredBlasts_&&!registeredBlasts_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(registeredCorpses_&&!registeredCorpses_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(consumptionParticles_&&!consumptionParticles_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(skeletons_&&!skeletons_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(blood_&&!blood_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(currencyInstances_&&!currencyInstances_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(!clearRoomDebris())return downwell::ResultsDestination::none;
    if(!clearRoomScriptEffects())return downwell::ResultsDestination::none;
    if(!run_.effectsSucceeded||!clearBulletOwnership())return downwell::ResultsDestination::none;
    if (!run_.effectsSucceeded || !clearEnemyOwnership() || !clearBreakableOwnership() || !retirePlayerOwnership()) return downwell::ResultsDestination::none;
    if(!run_.timeFields.clear()||!run_.sideCameraLockers.clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(!goalFieldInstances_.clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(!exitControllers_.clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    if(terrainInstances_&&!terrainInstances_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    const auto terrainInstances=terrainInstances_;
    if(nativeTerrainInstances_&&!nativeTerrainInstances_->clear()){run_.effectsSucceeded=false;return downwell::ResultsDestination::none;}
    const auto nativeTerrainInstances=nativeTerrainInstances_;
    const auto nativeTerrainBinding=nativeTerrainBinding_;
    const auto entryRooms=entryRooms_;
    const auto playerBirthTiming=playerBirthTiming_;
    const auto scriptSpawnInstances=scriptSpawnInstances_;
    const auto registeredExpiry=registeredExpiry_;
    const auto registeredSmoke=registeredSmoke_;
    const auto registeredBlasts=registeredBlasts_;
    const auto registeredCorpses=registeredCorpses_;
    const auto consumptionParticles=consumptionParticles_;
    const auto debrisInstances=debrisInstances_;
    const auto timeChangeInstances=timeChangeInstances_;
    const auto timeShardInstances=timeShardInstances_;
    const auto entryDithers=entryDithers_;
    const auto ditherAbove=ditherAbove_;
    const auto builderController=builderController_;
    const auto playerControllers=playerControllers_;
    const auto scattered=scattered_;
    const auto spawners=spawners_;
    const auto whiteJellies=whiteJellies_;
    const auto redBalls=redBalls_;
    const auto shopInstances=shopInstances_;
    const auto movingEffects=movingEffects_;
    const auto runControllers=runControllers_;
    const auto resultInstance=resultInstance_;
    const auto levelNumbers=levelNumbers_;
    const auto limboShards=limboShards_;
    const auto areaTitles=compactAreaTitles_;const auto areaTitleCount=compactAreaTitleCount_;
    const auto language=compactLanguage_;
    const auto shopSpeechText=shopSpeechText_;
    const auto registeredFlash=registeredFlash_;
    const auto controllerRoomSpeed=controllerRoomSpeed_;
    const auto droneBullets=droneBullets_;
    const auto bones=bones_;
    const auto skeletons=skeletons_;
    const auto blood=blood_;
    const auto currencyInstances=currencyInstances_;
    const auto currencyOrderCapture=currencyOrderCapture_;void* const currencyOrderContext=currencyOrderContext_;
    const auto currencyStepEligible=currencyStepEligible_;
    const auto phaseBoundary=phaseBoundary_;
    const auto roomAnimation=roomAnimation_;
    const auto actorEndStep=registeredActorEndStep_;void* const actorEndStepContext=registeredActorEndStepContext_;
    const bool actorEndStepOwnsOrdinary=registeredOrdinaryEndStep_;
    const auto roomAlarms=roomAlarms_;void* const roomAlarmsContext=roomAlarmsContext_;
    const auto resultRoomAlarms=resultRoomAlarms_;void* const resultRoomAlarmsContext=resultRoomAlarmsContext_;
    void* const roomAnimationContext=roomAnimationContext_;
    const auto roomActorDraw=roomActorDraw_;
    const auto roomActorReadOnlyDraw=roomActorReadOnlyDraw_;
    const auto roomActorDrawContext=roomActorDrawContext_;
    const auto enemyOrderCapture=enemyOrderCapture_;
    const auto enemyOrderRevision=enemyOrderRevision_;
    const auto debrisOrderCapture=debrisOrderCapture_;void* const debrisOrderContext=debrisOrderContext_;
    void* const enemyOrderContext=enemyOrderContext_;
    void* const phaseBoundaryContext=phaseBoundaryContext_;
    const auto playerLifecycle=playerLifecycle_;
    const auto nextPlayerIdentity=nextPlayerLifetimeId_;
    const auto enemyLifecycle=enemyLifecycle_;
    const auto breakableLifecycle=breakableLifecycle_;
    const auto nextBreakableIdentity=nextBreakableLifetimeId_;
    const auto records = run_.records();
    const int backgroundMusic=run_.backgroundMusicId;
    const auto controllerRise=run_.gemStreakRise;
    const auto nextBulletIdentity=nextBulletLifetimeId_;
    const auto bulletLifecycle=bulletLifecycle_;
    const auto nextEnemyIdentity=nextEnemyLifetimeId_;
    const auto nextGemIdentity=nextGemLifetimeId_;
    const auto fieldBirths=run_.timeFields.birthSequence();
    const auto fieldLifecycle=run_.timeFields.lifecycle();
    const auto lockerBirths=run_.sideCameraLockers.birthSequence();
    const auto lockerLifecycle=run_.sideCameraLockers.lifecycle();
    const auto goalBirths=goalFieldInstances_.birthSequence();
    const auto goalLifecycle=goalFieldInstances_.binding();
    const auto exitBirths=exitControllers_.birthSequence();
    const auto exitLifecycle=exitControllers_.binding();
    const auto numberBirths=run_.damageNumbers.birthSequence();
    const auto expiryBirths=run_.corpseExpiryEffects.birthSequence();
    const auto scriptBirths=ownedScriptEffects_.birthSequence();
    const auto debrisBirths=ownedDebrisEffects_.birthSequence();
    const auto smokeBirths=run_.smokeEffects.birthSequence();
    const auto bloodBirths=run_.bloodEffects.birthSequence();
    const auto flashBirths=run_.flashEffects.birthSequence();
    const auto foregroundBirth=run_.nextForegroundBirth;
    this->~GameplayWorld();
    new (this) GameplayWorld(initialRingOrigin, false, 1201 - 176, true, enemyLifecycle,breakableLifecycle,{},terrainInstances,entryRooms,playerBirthTiming);
    runAudio_=runAudio;
    // The split frame's borrowed environment survives in-place Retry, just
    // like its input publication. No device/resource is copied into the world.
    if(encounterUpdateOwner){
        encounterUpdateOwner_=encounterUpdateOwner;
        collisionPass_=encounterCollision;collisionPassContext_=encounterCollisionContext;
    }
    bindEncounterInput(encounterInput);
    run_.backgroundMusicId=backgroundMusic;
    nativeTerrainInstances_=nativeTerrainInstances;
    nativeTerrainBinding_=nativeTerrainBinding;
    run_.gemStreakRise=controllerRise; // Persistent controller survives Retry.
    nextBreakableLifetimeId_=nextBreakableIdentity;
    scriptSpawnInstances_=scriptSpawnInstances;
    registeredExpiry_=registeredExpiry;
    registeredSmoke_=registeredSmoke;
    registeredBlasts_=registeredBlasts;
    registeredCorpses_=registeredCorpses;
    consumptionParticles_=consumptionParticles;
    debrisInstances_=debrisInstances;
    timeChangeInstances_=timeChangeInstances;
    timeShardInstances_=timeShardInstances;
    entryDithers_=entryDithers;
    ditherAbove_=ditherAbove;
    builderController_=builderController;
    playerControllers_=playerControllers;
    scattered_=scattered;
    spawners_=spawners;
    whiteJellies_=whiteJellies;
    redBalls_=redBalls;
    shopInstances_=shopInstances;
    movingEffects_=movingEffects;
    runControllers_=runControllers;
    resultInstance_=resultInstance;
    levelNumbers_=levelNumbers;
#ifndef DOWNWELL_WORLD_ONE_ONLY
    limboShards_=limboShards;
#endif
    configureCompactAreaTitles(areaTitles,areaTitleCount,language);
    shopSpeechText_=shopSpeechText;
    registeredFlash_=registeredFlash;
    controllerRoomSpeed_=controllerRoomSpeed;
    droneBullets_=droneBullets;
    bones_=bones;
#ifndef DOWNWELL_WORLD_ONE_ONLY
    skeletons_=skeletons;
#endif
    blood_=blood;
    currencyInstances_=currencyInstances;
    configureCurrencyStepOrder(currencyOrderCapture,currencyOrderContext,currencyStepEligible);
    configurePhaseBoundary(phaseBoundary,phaseBoundaryContext);
    configureRoomAnimation(roomAnimation,roomAnimationContext);
    configureRegisteredActorEndStep(actorEndStep,actorEndStepContext,actorEndStepOwnsOrdinary);
    configureRoomAlarms(roomAlarms,roomAlarmsContext);
    configureResultRoomAlarms(resultRoomAlarms,resultRoomAlarmsContext);
    configureRoomActorDraw(roomActorDraw,roomActorDrawContext,roomActorReadOnlyDraw);
    configureEnemyStepOrder(enemyOrderCapture,enemyOrderContext,enemyOrderRevision);
    configureDebrisStepOrder(debrisOrderCapture,debrisOrderContext);
    nextBulletLifetimeId_=nextBulletIdentity;
    bulletLifecycle_=bulletLifecycle;
    nextEnemyLifetimeId_=nextEnemyIdentity;
    nextGemLifetimeId_=nextGemIdentity;
    // Construction above has no bound player owner. Recreate its lifetime
    // only after restoring the run and the preserved monotonic key sequence.
    playerLifecycle_=playerLifecycle;
    playerInstance_.lifetimeId=0;
    nextPlayerLifetimeId_=nextPlayerIdentity;
    run_.restart(action, records);
    if(!run_.timeFields.restoreEmptyBirthSequence(fieldBirths))run_.effectsSucceeded=false;
    if(!run_.timeFields.bindLifecycle(fieldLifecycle))run_.effectsSucceeded=false;
    if(!run_.sideCameraLockers.restoreEmptyBirthSequence(lockerBirths)||!run_.sideCameraLockers.bindLifecycle(lockerLifecycle))run_.effectsSucceeded=false;
    if(!goalFieldInstances_.restoreEmptyBirthSequence(goalBirths)||!goalFieldInstances_.bind(goalLifecycle))run_.effectsSucceeded=false;
    if(!exitControllers_.restoreEmptyBirthSequence(exitBirths)||!exitControllers_.bind(exitLifecycle))run_.effectsSucceeded=false;
    if(!run_.smokeEffects.restoreEmptyBirthSequence(smokeBirths)||
       !run_.bloodEffects.restoreEmptyBirthSequence(bloodBirths)||
       !run_.flashEffects.restoreEmptyBirthSequence(flashBirths))run_.effectsSucceeded=false;
    if(!run_.damageNumbers.restoreEmptyBirthSequence(numberBirths)||
       !run_.corpseExpiryEffects.restoreEmptyBirthSequence(expiryBirths))run_.effectsSucceeded=false;
    if(!ownedScriptEffects_.restoreEmptyBirthSequence(scriptBirths))run_.effectsSucceeded=false;
    if(!ownedDebrisEffects_.restoreEmptyBirthSequence(debrisBirths))run_.effectsSucceeded=false;
    run_.nextForegroundBirth=foregroundBirth;
    enemyRandom_.setSeed(seed);
    // The temporary owner-free construction prepared a presentation frame.
    // Restore pre-Step camera state before publishing the real room birth.
    cameraMotion_.reset(240,176);
    cameraShakeAlarm_=-1;
    cameraY_=cameraPixelY_=cameraPixelX_=cameraSteadyX_=0;
    ringOrigin_=initialRingOrigin%ringHeight;
    // ResetPlayer: audio_resume_all, audio_stop_all, then the confirm sound.
    // The rebuilt run starts an empty ring and voice model; the backend must
    // drain the old ring before this call.
    run_.resumeAllSounds();
    run_.audio().stopAll();
    run_.soundPlayOL(321,90,false,true,"UI");
    if(playerBirthPending_)return destination;
    if(!birthPlayerOwnership())return downwell::ResultsDestination::none;
    initializeMainCameraFrame();
    return destination;
}

bool GameplayWorld::bindTimeChangeInstances(downwell::NativeTimeChangeRegistration& registry){
    if(timeChangeInstances_||!registry.valid()||registry.size())return false;
    timeChangeInstances_=&registry;return true;
}
bool GameplayWorld::bindTimeShardInstances(downwell::NativeTimeShardRegistration& registry){
    if(timeShardInstances_||!registry.valid()||registry.size())return false;
    timeShardInstances_=&registry;return true;
}
bool GameplayWorld::createTimeShard(downwell::TimeShardEffects<64>& effects,downwell::MotionBinary64 x,
                                   downwell::MotionBinary64 y,bool lowSpec){
    if(!run_.effectsSucceeded)return false;
    if(!timeShardInstances_||!effects.phasesBoundTo(*timeShardInstances_)||
       (timeShardInstances_->size()&&timeShardEffects_!=&effects))return run_.effectsSucceeded=false;
    timeShardEffects_=&effects;
    return run_.effectsSucceeded=effects.create(x,y,lowSpec,enemyRandom_)!=0;
}
bool GameplayWorld::advanceTimeShardRoomAlarm(downwell::InstanceHandle key,unsigned index){
    if(!run_.effectsSucceeded)return false;
    auto* record=timeShardInstances_?timeShardInstances_->schedulerRecord(key):nullptr;
    if(!record||!timeShardEffects_||index>2)return run_.effectsSucceeded=false;
    if(!record->active||record->destroyRequested)return true;
    if(!record->alarms.advanceOne(index))return true;
    return run_.effectsSucceeded=timeShardInstances_->dispatchAlarm(key,*timeShardEffects_,index);
}
bool GameplayWorld::advanceTimeShardRoomAnimation(downwell::InstanceHandle key){
    return run_.effectsSucceeded=run_.effectsSucceeded&&timeShardInstances_&&timeShardEffects_&&
        timeShardInstances_->dispatchAnimation(key,*timeShardEffects_);
}
bool GameplayWorld::retireTimeShardRoomEffect(downwell::InstanceHandle key){
    return run_.effectsSucceeded=run_.effectsSucceeded&&timeShardInstances_&&timeShardEffects_&&
        timeShardInstances_->retireMarked(key,*timeShardEffects_);
}
bool GameplayWorld::collideTimeShardRoomEffect(downwell::InstanceHandle key){
    // Collision 2081 reads only sign(other.xsp/ysp), not engine hspeed/vspeed.
    // The private fixed-point numerators preserve those signs exactly.
    return run_.effectsSucceeded=run_.effectsSucceeded&&timeShardInstances_&&timeShardEffects_&&
        timeShardInstances_->dispatchContact(key,*timeShardEffects_,
            downwell::MotionBinary64::fromInteger(player_.speedXFixed()),
            downwell::MotionBinary64::fromInteger(player_.speedYFifths()),enemyRandom_);
}
bool GameplayWorld::clearRoomTimeShards(){
    if(!run_.effectsSucceeded)return false;
    if(timeShardInstances_&&timeShardInstances_->size()&&(!timeShardEffects_||!timeShardEffects_->clear()))
        return run_.effectsSucceeded=false;
    timeShardEffects_=nullptr;return true;
}
bool GameplayWorld::advanceTimeChangeRoomAlarm(downwell::InstanceHandle key,unsigned index){
    if(!run_.effectsSucceeded)return false;
    auto* record=timeChangeInstances_?timeChangeInstances_->schedulerRecord(key):nullptr;
    if(!record||!timeChangeEffects_||!timeChangeSpeed_||index>1)return run_.effectsSucceeded=false;
    if(!record->active||record->destroyRequested)return true;
    if(!record->alarms.advanceOne(index))return true;
    return run_.effectsSucceeded=timeChangeInstances_->dispatchAlarm(key,*timeChangeEffects_,index,*timeChangeSpeed_);
}
bool GameplayWorld::retireTimeChangeRoomEffect(downwell::InstanceHandle key){
    return run_.effectsSucceeded=run_.effectsSucceeded&&timeChangeInstances_&&timeChangeEffects_&&
        timeChangeInstances_->retireMarked(key,*timeChangeEffects_);
}
bool GameplayWorld::clearRoomTimeChanges(){
    if(!run_.effectsSucceeded)return false;
    if(timeChangeInstances_&&timeChangeInstances_->size()&&
       (!timeChangeEffects_||!timeChangeEffects_->clear()))return run_.effectsSucceeded=false;
    timeChangeEffects_=nullptr;timeChangeSpeed_=nullptr;return true;
}

bool GameplayWorld::retireGameplayRoom() {
    // Retire registered room-local lifetimes at departure, before Result
    // Create. Persistent controllers and run globals deliberately survive.
    // This also handles explicit test/adaptor births made before the next
    // room load; an already retired lifetime must not receive a second callback.
    if(levelNumbers_&&!levelNumbers_->clear())return run_.effectsSucceeded=false;
    compactTitle_={};for(auto& r:compactRewards_)r={};liveRewards_=0;
    for(auto& r:shopSpeech_)r={};shopSpeechLive_=0;
    if(limboShards_&&!limboShards_->clear())return run_.effectsSucceeded=false;
    if(registeredFlash_&&!registeredFlash_->clear())return run_.effectsSucceeded=false;
    if(resultInstance_&&!resultInstance_->clear())return run_.effectsSucceeded=false;
    if(!clearRoomTimeChanges()||!clearRoomTimeShards())return false;
    if(entryDithers_&&!entryDithers_->clear())return run_.effectsSucceeded=false;
    compactEntryLive_=false;
    if(ditherAbove_&&!ditherAbove_->clear())return run_.effectsSucceeded=false;
    if(builderController_&&!builderController_->clear())return run_.effectsSucceeded=false;
    if(playerControllers_&&!playerControllers_->clear())return run_.effectsSucceeded=false;
    if(scattered_&&!scattered_->clear())return run_.effectsSucceeded=false;
    if(spawners_&&!spawners_->clear())return run_.effectsSucceeded=false;
    if(whiteJellies_&&!whiteJellies_->clear())return run_.effectsSucceeded=false;
    if(redBalls_&&!redBalls_->clear())return run_.effectsSucceeded=false;
    if(shopInstances_&&!shopInstances_->clear())return run_.effectsSucceeded=false;
    if(movingEffects_&&!movingEffects_->clear())return run_.effectsSucceeded=false;
    if(droneBullets_&&!droneBullets_->clear())return run_.effectsSucceeded=false;
    if(bones_&&!bones_->clear())return run_.effectsSucceeded=false;
    if(registeredExpiry_&&!registeredExpiry_->clear())return run_.effectsSucceeded=false;
    if(registeredSmoke_&&!registeredSmoke_->clear())return run_.effectsSucceeded=false;
    if(registeredBlasts_&&!registeredBlasts_->clear())return run_.effectsSucceeded=false;
    if(registeredCorpses_&&!registeredCorpses_->clear())return run_.effectsSucceeded=false;
    if(consumptionParticles_&&!consumptionParticles_->clear())return run_.effectsSucceeded=false;
    if(skeletons_&&!skeletons_->clear())return run_.effectsSucceeded=false;
    if(blood_&&!blood_->clear())return run_.effectsSucceeded=false;
    if(currencyInstances_&&!currencyInstances_->clear())return run_.effectsSucceeded=false;
    if(!clearRoomDebris())return false;
    if(!clearRoomScriptEffects())return false;
    if (!run_.effectsSucceeded || !clearEnemyOwnership() || !clearBreakableOwnership() || !retirePlayerOwnership()) return false;
    if(!clearBulletOwnership())return false;
    if(!goalFieldInstances_.clear()){run_.effectsSucceeded=false;return false;}
    if(!exitControllers_.clear()){run_.effectsSucceeded=false;return false;}
    if(terrainInstances_&&!terrainInstances_->clear()){run_.effectsSucceeded=false;return false;}
    roomTerrainLoaded_=false;
    if(nativeTerrainInstances_&&!nativeTerrainInstances_->clear())return run_.effectsSucceeded=false;
    if(!run_.timeFields.clear())return run_.effectsSucceeded=false;
    if(!run_.sideCameraLockers.clear())return run_.effectsSucceeded=false;
    if(!resetCompactScenery())return false;
    resetCompactSpawning();
    return true;
}

bool GameplayWorld::finishWorldOneSlice() {
    if (!run_.effectsSucceeded || !run_.levelClearActive || !run_.nextLevelRequested ||
        run_.stage.area != 1 || run_.stage.level != 3) return false;
    if (!retireGameplayRoom()) return false;
    // scrNextLevel advances globals before room_goto. Do not create the
    // catacomb builder, player, title instance or audio voice here: those
    // belong to the next room and levelBeginCue, outside this slice.
    return run_.advanceAfterLevelClear();
}

bool GameplayWorld::prepareNextAreaOneLevel(const downwell::Input& input, unsigned initialRingOrigin) {
    if (!run_.levelClearActive || !run_.nextLevelRequested ||
        run_.stage.area != 1 || run_.stage.level >= 3) return false;
    if(!retireGameplayRoom())return false;
    if (!run_.advanceAfterLevelClear()) return false;
    // Do not reconstruct this large runtime or copy RunSession onto the task
    // stack: its run globals remain in place while old world ownership retires.
    run_.shops.reset();
    run_.flashEffects.clear();
    run_.smokeEffects.clear();
    run_.bloodEffects.clear();
    run_.clearCorpseExpiry();
    run_.clearDamageNumbers();
    player_.enterRoom(input.fire, 1201 - 176);
    emitter_.reset(run_.upgrades.levels[12] != 0);
    run_.ammoHud.previousShots = run_.ammoHud.previousRecharges = 0;
    currencyAccount_.playerCreated();
    death_ = {};
    invulnerability_ = 0;
    prepareBreakableSlotsForReuse();
    enemyCount_ = breakableCount_ = blocksDestroyed_ = 0;
    bulletSlotEnd_ = gemSlotEnd_ = 0;
    for (unsigned i = 0; i < bulletCapacity; ++i) bullets_[i].active = false;
    for (unsigned i = 0; i < enemyCapacity; ++i) {
        enemies_[i].active = false;
        enemies_[i].identitySlot = 0xffffu;
        enemySlotIndex_[i] = 0;
    }
    for (unsigned i = 0; i < gemCapacity; ++i) gems_[i].active = false;
    generationStep_ = nullptr; generationContext_ = nullptr;
    generationSucceeded_ = true;
    solidResolver_ = nullptr; solidCorners_=nullptr; solidResolverContext_ = nullptr; wallContactResolver_ = nullptr; platformContactResolver_ = nullptr;
    solidCells_ = {};
    segmentResolver_ = nullptr;
    maskResolver_ = nullptr;
    landingResolver_ = nullptr; landingResolverContext_ = nullptr;
    breakableIndexReady_ = false;
    followCamera_ = true;
    cameraMotion_.reset(240, 176);
    cameraShakeAlarm_=-1;
    cameraY_ = cameraPixelY_ = cameraPixelX_ = cameraSteadyX_ = activationY_ = 0;
    ringOrigin_ = initialRingOrigin % ringHeight;
    playerBirthPending_=playerBirthTiming_==PlayerBirthTiming::authoredRoom;
    return playerBirthPending_||birthPlayerOwnership();
}

GameplayWorld::GameplayWorld(unsigned initialRingOrigin,
                                   bool startsGrounded,
                                   int landingOffset,
                                   bool followCamera, const EnemyLifecycleBinding& enemyLifecycle,
                                   const BreakableLifecycleBinding& breakableLifecycle,
                                   const PlayerLifecycleBinding& playerLifecycle,
                                   downwell::GameplayTerrainInstances* terrainInstances,
                                   const downwell::RoomPack* entryRooms,PlayerBirthTiming playerBirthTiming)
    : enemyLifecycle_(enemyLifecycle), breakableLifecycle_(breakableLifecycle), playerLifecycle_(playerLifecycle), terrainInstances_(terrainInstances), player_(startsGrounded, landingOffset), tick_(0),
      ringOrigin_(initialRingOrigin % ringHeight),
      followCamera_(followCamera), cameraY_(0), cameraPixelY_(0),
      cameraPixelX_(0), cameraSteadyX_(0), activationY_(0),
      enemyCount_(0), enemyRandom_(2),
      health_(4), invulnerability_(0),
      solidResolver_(0),
      solidResolverContext_(0),
      breakableCount_(0), blocksDestroyed_(0), checkpoint_(0)
{
    unsigned index;
    run_.reset();
    enemyRandom_.prepareLookahead(randomLookahead_);
    entryRooms_=entryRooms;
    playerBirthTiming_=playerBirthTiming;
    playerBirthPending_=playerBirthTiming==PlayerBirthTiming::authoredRoom;
    if(playerBirthPending_&&!entryRooms_){run_.effectsSucceeded=false;return;}
    // The selected runner copies serialized views before instance events.
    // This adapter enters rmMain; following the camera is a separate phase.
    if(entryRooms_&&!roomEntryViews_.load(*entryRooms_,11)){
        run_.effectsSucceeded=false;return;
    }
    run_.stage.advance(1); // This runtime starts at the confirmed scrNextLevel(1) entry.
    currencyAccount_.playerCreated();
    for (index = 0; index < bulletCapacity; ++index) {
        bullets_[index].x = 0;
        bullets_[index].y = 0;
        bullets_[index].active = false;
    }
    for (index = 0; index < enemyCapacity; ++index) {
        enemies_[index].objectId = -1;
        enemies_[index].spriteId = -1;
        enemies_[index].x = 0;
        enemies_[index].y = 0;
        enemies_[index].health = 0;
        enemies_[index].reward = 0;
        enemies_[index].facing = 1;
        enemies_[index].preciseX = 0;
        enemies_[index].preciseY = 0;
        enemies_[index].speedX = 0;
        enemies_[index].speedY = 0;
        enemies_[index].gravityRemainder = 0;
        enemies_[index].gravityPositionRemainder = 0;
        enemies_[index].motionState = 0;
        enemies_[index].auxiliaryState = 0;
        enemies_[index].alarm0 = -1;
        enemies_[index].alarm1 = -1;
        enemies_[index].direction = 0;
        enemies_[index].moveSpeed = 0;
        enemies_[index].imagePhase = 0;
        enemies_[index].imageSpeed = 0;
        enemies_[index].imageRemainder = 0;
        enemies_[index].imageFrame = 0;
        enemies_[index].active = false;
        enemies_[index].awake = false;
        enemies_[index].grounded = false;
        enemies_[index].frogLanded = true;
        enemies_[index].activationPending = false;
    }
    for (index = 0; index < gemCapacity; ++index) {
        gems_[index].x = 0;
        gems_[index].y = 0;
        gems_[index].amount = 0;
        gems_[index].age = 0;
        gems_[index].preciseX = 0;
        gems_[index].preciseY = 0;
        gems_[index].speedX = 0;
        gems_[index].speedY = 0;
        gems_[index].large = false;
        gems_[index].attracted = false;
        gems_[index].active = false;
    }
    if(!playerBirthPending_&&birthPlayerOwnership())initializeMainCameraFrame();
}

void GameplayWorld::initializeMainCameraFrame(){
    if(followCamera_){
        // Compatibility entry snapshot includes camMain's first ordinary Step,
        // but creation callbacks must observe the preceding, unadvanced state.
        cameraMotion_.step(240 * 256L,176 * 256L);
        cameraY_=(cameraMotion_.y-176L*4096)/16;
        cameraPixelY_=cameraMotion_.pixelY()-176;
        ringOrigin_=(ringOrigin_+(unsigned)cameraPixelY_)%ringHeight;
    }
    boneViewLeft_=160+cameraPixelX_;
    boneViewTopQ8_=34L*256+viewCameraYQ8();
}

bool GameplayWorld::birthPlayerOwnership() {
    if(!run_.effectsSucceeded||!nextPlayerLifetimeId_) {
        run_.effectsSucceeded=false;return false;
    }
    std::uint32_t sourceId=0;
    if(entryRooms_){
        downwell::PackedRoom room;
        if(!entryRooms_->findRoom(11,room))return run_.effectsSucceeded=false;
        for(unsigned i=0;i<room.instanceCount;++i){
            downwell::PackedRoomInstance instance;
            if(!entryRooms_->roomInstance(room,i,instance))return run_.effectsSucceeded=false;
            if(instance.objectId!=35)continue;
            if(sourceId||!instance.instanceId)return run_.effectsSucceeded=false;
            sourceId=instance.instanceId;
        }
        if(!sourceId)return run_.effectsSucceeded=false;
    }
    playerInstance_={240+int(downwell::roundGameMakerQ8(player_.offsetXFixed())),
                     176+int(downwell::roundGameMakerQ8(player_.offsetYFixed())),
                     true,nextPlayerLifetimeId_++};
    // Authored room identity precedes the birth callback. This does not
    // allocate a dynamic ID or stand in for the remaining room Create order.
    playerInstance_.sourceInstanceId=sourceId;
    playerChildrenStarted_=false;
    depthMeterY_=playerInstance_.y;
    if(!playerLifecycle_.birth(playerHandle(),playerInstance_)) {
        run_.effectsSucceeded=false;return false;
    }
    run_.playerCreated();
    resetCompactAfterImages(playerInstance_.x,playerInstance_.y);
    return createCompactCompanions(playerInstance_.x,playerInstance_.y);
}

bool GameplayWorld::retirePlayerOwnership() {
    if(!playerInstance_.lifetimeId)return true;
    if(!playerLifecycle_.retire(playerHandle(),playerInstance_)) {
        run_.effectsSucceeded=false;return false;
    }
    playerInstance_.lifetimeId=0;playerInstance_.active=false;return true;
}

bool GameplayWorld::publishPlayerPosition(int x,int y) {
    if(!playerInstance_.lifetimeId||!playerLifecycle_.integerPosition(playerHandle(),playerInstance_,x,y)) {
        run_.effectsSucceeded=false;return false;
    }
    playerInstance_.x=x;playerInstance_.y=y;return true;
}

void GameplayWorld::spawnReward(const EnemyState& enemy, bool attracted)
{
    (void)spawnCompactCurrency(enemy.reward,enemy.x,enemy.y,attracted);
}

unsigned GameplayWorld::gemCount() const
{
    unsigned count = 0;
    for (unsigned index = 0; index < gemSlotEnd_; ++index)
        if (gems_[index].active)
            ++count;
    return count;
}

const GameplayWorld::GemState&
GameplayWorld::gemAt(unsigned index) const
{
    return gems_[index < gemCapacity ? index : 0];
}

bool GameplayWorld::publishOwnedEnemyTransform(const EnemyState& enemy,downwell::RunnerTransformField field,int value) {
    if(enemyLifecycle_.integerTransform({enemy.identitySlot,enemy.lifetimeId},enemy,field,value,
                                        {0x3f800000u},0xffffffu))return true;
    run_.effectsSucceeded=false;return false;
}

bool GameplayWorld::publishEnemySprite(EnemyState& enemy,int sprite) {
    // The owner then holds the record's sprite: an unchanged one plans no change.
    if(enemyLifecycle_.recordOwnsAppearance()&&sprite==enemy.spriteId)return true;
    if(!enemyLifecycle_.sprite({enemy.identitySlot,enemy.lifetimeId},enemy,sprite)){
        run_.effectsSucceeded=false;return false;
    }
    enemy.spriteId=sprite;return true;
}

bool GameplayWorld::publishOwnedEnemyImage(const EnemyState& enemy,unsigned phase) {
    if(enemyLifecycle_.phaseIndex({enemy.identitySlot,enemy.lifetimeId},enemy,phase))return true;
    run_.effectsSucceeded=false;return false;
}

bool GameplayWorld::publishEnemyActivation(const EnemyState& enemy, bool active) {
    if (enemyLifecycle_.activation({enemy.identitySlot,enemy.lifetimeId},enemy,active)) return true;
    run_.effectsSucceeded=false;
    return false;
}

bool GameplayWorld::retireEnemyOwnership(EnemyState& enemy) {
    releaseSteering(enemy);
    if (enemy.identitySlot>=enemyCapacity) return true;
    if (!enemyLifecycle_.retire({enemy.identitySlot,enemy.lifetimeId},enemy)) {
        run_.effectsSucceeded=false;
        return false;
    }
    enemySlotIndex_[enemy.identitySlot]=0;
    enemy.identitySlot=0xffffu;
    return true;
}

bool GameplayWorld::clearEnemyOwnership() {
    for(unsigned i=0;i<enemyCount_;++i)
        if(!retireEnemyOwnership(enemies_[i])) return false;
    return true;
}

bool GameplayWorld::retireBulletOwnership(BulletState& bullet){
    if(bullet.identitySlot>=bulletCapacity)return true;
    if(!bulletLifecycle_.retire({bullet.identitySlot,bullet.lifetimeId},bullet))return run_.effectsSucceeded=false;
    bulletIdentitySlots_[bullet.identitySlot]=false;
    bullet.identitySlot=0xffffu;bullet.active=false;return true;
}
bool GameplayWorld::clearBulletOwnership(){
    for(unsigned i=0;i<bulletSlotEnd_;++i)if(!retireBulletOwnership(bullets_[i]))return false;
    return true;
}
bool GameplayWorld::retireMarkedBullet(downwell::InstanceHandle h){
    auto* bullet=const_cast<BulletState*>(resolveBulletLifetime(h));
    if(!run_.effectsSucceeded||!bullet||!bullet->destroyRequested)return run_.effectsSucceeded=false;
    return retireBulletOwnership(*bullet);
}
bool GameplayWorld::markBulletDestroyed(downwell::InstanceHandle h){
    auto* bullet=const_cast<BulletState*>(resolveBulletLifetime(h));
    if(!run_.effectsSucceeded||!bullet)return run_.effectsSucceeded=false;
    if(bullet->destroyRequested)return true;
    if(!bulletLifecycle_.markDestroyed(h,*bullet))return run_.effectsSucceeded=false;
    bullet->destroyRequested=true;bullet->active=false;return true;
}
bool GameplayWorld::publishBulletCollisionPosition(downwell::InstanceHandle h,
    downwell::RunnerImageValue x,downwell::RunnerImageValue y){
    const auto* bullet=resolveBulletLifetime(h);
    if(!run_.effectsSucceeded||!bullet||!downwell::RunnerSpatialSplit::valid(x)||
       !downwell::RunnerSpatialSplit::valid(y)||!bulletLifecycle_.publishedPosition(h,*bullet,x,y))
        return run_.effectsSucceeded=false;
    // Source setters may run after birth but before the first Step admits
    // source motion. Keep their absolute binary32 values even in that window.
    auto* current=const_cast<BulletState*>(resolveBulletLifetime(h));if(!current)return run_.effectsSucceeded=false;
    current->sourceMotion.x=x;current->sourceMotion.y=y;
    if(current->sourceOwned){
        std::int32_t px,py;
        if(!x.roundPixel(px)||!y.roundPixel(py))return run_.effectsSucceeded=false;
        current->x=px-240;current->y=py-176;
    }
    return true;
}
bool GameplayWorld::publishBulletAngle(downwell::InstanceHandle h,downwell::RunnerImageValue angle){
    const auto* bullet=resolveBulletLifetime(h);
    if(!run_.effectsSucceeded||!bullet||!downwell::RunnerSpatialSplit::valid(angle)||
       !bulletLifecycle_.transform(h,*bullet,downwell::RunnerTransformField::angle,angle,{0x3f800000u},0xffffffu))
        return run_.effectsSucceeded=false;
    return true;
}

bool GameplayWorld::configureEnemy(unsigned index, int objectId,
                                      int spriteId, int x, int y,
                                      int speedX, int speedY,
                                      int health, int reward,
                                      int motionState, int auxiliaryState,
                                      int alarm0, int alarm1,
                                      int direction, int moveSpeed,
                                      bool active, bool grounded, bool awake,std::uint32_t sourceId)
{
    if (index >= enemyCapacity || !run_.effectsSucceeded)
        return false;
    if(sourceId>0x7fffffffu)return run_.effectsSucceeded=false;
    for(unsigned i=0;sourceId&&i<enemyCount_;++i)
        if(i!=index&&enemies_[i].lifetimeId&&enemies_[i].sourceId==sourceId)return run_.effectsSucceeded=false;
    if (!nextEnemyLifetimeId_) {
        run_.effectsSucceeded = false;
        return false;
    }
    EnemyState& enemy = enemies_[index];
    unsigned slot=enemy.identitySlot;
    if(index<enemyCount_ && !retireEnemyOwnership(enemy))return false;
    if(index>=enemyCount_ || slot>=enemyCapacity){
        slot=0;
        while(slot<enemyCapacity&&enemySlotIndex_[slot])++slot;
    }
    if(slot==enemyCapacity){run_.effectsSucceeded=false;return false;}
    enemySlotIndex_[slot]=static_cast<unsigned short>(index+1);
    enemy.identitySlot=static_cast<unsigned short>(slot);
    enemy.lifetimeId = nextEnemyLifetimeId_++;
    enemy.sourceId = sourceId;
    enemy.objectId = objectId;
    enemy.spriteId = spriteId;
    enemy.maskId=-1;
    enemy.x = x;
    enemy.y = y;
    enemy.health = health;
    enemy.reward = reward;
    enemy.facing = speedX < 0 ? -1 : 1;
    enemy.preciseX = (long)x * 256L;
    enemy.frogHorizontal.position = (long)x * 65536L;
    enemy.frogHorizontal.velocity = (long)speedX * 256L;
    enemy.frogHit = {};
    enemy.crawler.initialize(x, y);
    enemy.crawler.latched = grounded;
    enemy.snail.initialize(x, y, speedY >= 0);
    enemy.snailHit = {};
    enemy.batHit = {};
    enemy.batDirectionFraction = 0;
    enemy.batActivity = {};
    enemy.walker.initialize(x, y, speedX * 5L, speedY * 5L, grounded);
    enemy.preciseY = (long)y * 256L;
    enemy.speedX = speedX;
    enemy.speedY = speedY;
    enemy.gravityRemainder = 0;
    enemy.gravityPositionRemainder = 0;
    enemy.motionState = motionState;
    enemy.auxiliaryState = auxiliaryState;
    enemy.alarm0 = alarm0;
    enemy.alarm1 = alarm1;
    enemy.direction = direction;
    enemy.moveSpeed = moveSpeed;
    enemy.imagePhase = 0;
    enemy.imageRemainder = 0;
    enemy.imageSpeed = downwell::enemyCreateProfile(objectId).animationRate;
    enemy.imageFrame = 0;
    enemy.active = active;
    enemy.activatedTick = tick_;
    enemy.destroyRequested = false;
    enemy.takenImpactDamage = 0;
    enemy.awake = awake;
    enemy.grounded = grounded;
    enemy.frogLanded = true;
    enemy.frogPresentation = {};
    enemy.activationPending = !active;
    if (enemyCount_ <= index)
        enemyCount_ = index + 1;
    if(!enemyLifecycle_.birth({enemy.identitySlot,enemy.lifetimeId},enemy)) {
        run_.effectsSucceeded=false;
        return false;
    }
    return true;
}

const GameplayWorld::EnemyState&
GameplayWorld::enemyAt(unsigned index) const
{
    return enemies_[index < enemyCount_ ? index : 0];
}

unsigned GameplayWorld::bulletCount() const
{
    unsigned count = 0;
    unsigned index;
    for (index = 0; index < bulletSlotEnd_; ++index)
        if (bullets_[index].active)
            ++count;
    return count;
}

const GameplayWorld::BulletState&
GameplayWorld::bulletAt(unsigned index) const
{
    return bullets_[index < bulletCapacity ? index : 0];
}

int GameplayWorld::bulletX() const
{
    unsigned index;
    for (index = 0; index < bulletSlotEnd_; ++index)
        if (bullets_[index].active)
            return bullets_[index].x;
    return 0;
}

int GameplayWorld::bulletY() const
{
    unsigned index;
    for (index = 0; index < bulletSlotEnd_; ++index)
        if (bullets_[index].active)
            return bullets_[index].y;
    return 0;
}

downwell::LaserSightRay GameplayWorld::traceLaserSight(
    downwell::LaserSightRay::Coordinate x, downwell::LaserSightRay::Coordinate y,
    downwell::LaserSightRay::Coordinate bottom, std::uint32_t speedTurns,
    std::uint32_t aimTurns) const {
    using downwell::ProjectileSegment;
    const auto local = [](const ProjectileSegment& path) {
        auto x = path.x, y = path.y;
        x.pixel -= 240; y.pixel -= 176;
        return ProjectileSegment(x, y, path.dx, path.dy);
    };
    return downwell::LaserSightRay::trace(x, y, bottom, speedTurns,
        [&](const ProjectileSegment& roomPath) {
            if (run_.shops.rayWallIntersects(roomPath)) return true;
            const auto path = local(roomPath);
            for (unsigned i = 0; i < breakableCount_; ++i) {
                const auto& block = breakables_[i];
                if (block.active && !block.destroyRequested && path.intersects(block.x-8, block.x+8, block.y-8, block.y+8))
                    return true;
            }
            if (segmentResolver_) return segmentResolver_(solidResolverContext_, path);
            if (!solidResolver_) return false;
            const int left = path.x.pixel < path.endX.pixel ? path.x.pixel : path.endX.pixel;
            const int right = path.x.pixel > path.endX.pixel ? path.x.pixel : path.endX.pixel;
            const int top = path.y.pixel < path.endY.pixel ? path.y.pixel : path.endY.pixel;
            const int bottom = path.y.pixel > path.endY.pixel ? path.y.pixel : path.endY.pixel;
            for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x)
                if (solidResolver_(solidResolverContext_, x, y) && path.intersects(x,x+1,y,y+1))
                    return true;
            return false;
        }, [&](const ProjectileSegment& roomPath) {
            const auto path = local(roomPath);
            for (unsigned i = 0; i < enemyCount_; ++i) {
                const auto& enemy = enemies_[i];
                if (!enemy.active) continue;
                const downwell::ContactActor contact{enemy.x*256L, enemy.y*256L,
                    enemy.objectId == 117 ? enemy.motionState : enemy.facing,
                    enemy.imageScaleY(), downwell::enemyContactMask(enemy.objectId, enemy.spriteId,enemy.maskId)};
                if (path.intersects(contact)) return true;
            }
            return false;
        }, aimTurns);
}

void GameplayWorld::configureBreakables(const int* x, const int* y,
                                            unsigned count)
{
    if(!run_.effectsSucceeded)return;
    if (x == 0 || y == 0)
        return;
    if (count > breakableCapacity) {run_.effectsSucceeded=false;return;}
    if(!clearBreakableOwnership())return;
    prepareBreakableSlotsForReuse();
    breakableCount_ = 0;
    breakableIndexReady_ = true;
    for (auto& cell : breakableIndex_) cell = 0;
    blocksDestroyed_ = 0;
    if(!appendBreakables(x, y, count))run_.effectsSucceeded=false;
}

bool GameplayWorld::retireBreakable(unsigned index) {
    auto& box=breakables_[index];
    if(!box.lifetimeId)return true;
    if(!breakableLifecycle_.retire({index,box.lifetimeId},box)) {
        run_.effectsSucceeded=false;return false;
    }
    box.lifetimeId=0;box.active=false;return true;
}

bool GameplayWorld::markEnemyDestroyed(downwell::InstanceHandle handle) {
    auto* enemy=const_cast<EnemyState*>(resolveEnemyLifetime(handle));
    if(!run_.effectsSucceeded||!enemy){run_.effectsSucceeded=false;return false;}
    if(enemy->destroyRequested)return true;
    if(!enemyLifecycle_.markDestroyed(handle,*enemy)){run_.effectsSucceeded=false;return false;}
    enemy->destroyRequested=true;return true;
}

bool GameplayWorld::markBreakableDestroyed(downwell::InstanceHandle handle) {
    auto* box=const_cast<Breakable*>(findBreakable(handle));
    if(!run_.effectsSucceeded||!box){run_.effectsSucceeded=false;return false;}
    if(box->destroyRequested)return true;
    if(breakableLifecycle_.hasOwner()&&!breakableLifecycle_.markDestroyed(handle,*box)){run_.effectsSucceeded=false;return false;}
    box->destroyRequested=true;return true;
}

bool GameplayWorld::retireMarkedEnemy(downwell::InstanceHandle handle) {
    auto* enemy=const_cast<EnemyState*>(resolveEnemyLifetime(handle));
    if(!run_.effectsSucceeded||!enemy||!enemy->destroyRequested){run_.effectsSucceeded=false;return false;}
    const unsigned index=unsigned(enemy-enemies_);
    if(!retireEnemyOwnership(*enemy))return false;
    for(unsigned next=index+1;next<enemyCount_;++next){
        enemies_[next-1]=enemies_[next];
        if(enemies_[next-1].identitySlot<enemyCapacity)
            enemySlotIndex_[enemies_[next-1].identitySlot]=static_cast<unsigned short>(next);
    }
    --enemyCount_;
    enemies_[enemyCount_].identitySlot=0xffffu;
    enemies_[enemyCount_].lifetimeId=0;
    enemies_[enemyCount_].active=false;
    return true;
}

bool GameplayWorld::retireMarkedBreakable(downwell::InstanceHandle handle) {
    const auto* box=findBreakable(handle);
    if(!run_.effectsSucceeded||!box||!box->destroyRequested){run_.effectsSucceeded=false;return false;}
    return retireBreakable(handle.slot);
}

bool GameplayWorld::clearBreakableOwnership() {
    for(unsigned i=0;i<breakableCount_;++i)
        if(!retireBreakable(i))return false;
    return true;
}

void GameplayWorld::prepareBreakableSlotsForReuse(){
    for(unsigned i=0;i<breakableCount_;++i)boxRewards_[i]={};
#ifndef DOWNWELL_REFERENCE_BREAKABLES
    // Append-only slots are constructed once before gameplay. Reused slots
    // are reset here, at the room/configuration boundary after retirement,
    // never in a live generation burst. Individual retirement keeps its
    // observable dead record intact until that boundary.
    static const Breakable initial{0,0,false,0};
    for(unsigned i=0;i<breakableCount_;++i)breakables_[i]=initial;
#endif
}

bool GameplayWorld::appendBreakables(const int* x, const int* y, unsigned count,
    const downwell::BreakableState* creation,const std::uint32_t* sourceIds)
{
    if(!run_.effectsSucceeded)return false;
    if (count > breakableCapacity - breakableCount_ || (count && (!x || !y)))
        return false;
    if(count&&(!nextBreakableLifetimeId_||count-1>0xffffffffu-nextBreakableLifetimeId_))return false;
    if(sourceIds)for(unsigned i=0;i<count;++i){
        if(!sourceIds[i]||sourceIds[i]>0x7fffffffu)return run_.effectsSucceeded=false;
        for(unsigned j=0;j<i;++j)if(sourceIds[j]==sourceIds[i])return run_.effectsSucceeded=false;
        for(unsigned j=0;j<breakableCount_;++j)
            if(breakables_[j].lifetimeId&&breakables_[j].sourceId==sourceIds[i])return run_.effectsSucceeded=false;
    }
    for (unsigned source = 0; source < count; ++source) {
        const unsigned index = breakableCount_++;
        breakables_[index].sourceId=sourceIds?sourceIds[source]:0;
#ifndef DOWNWELL_REFERENCE_BREAKABLES
        // The unused pool slot already holds default script state. Only
        // actual Create inputs and its new identity are live work.
        auto& record=breakables_[index];
        record.x=x[source];record.y=y[source];record.active=true;
        if(creation)record.state=creation[source];
        observeBoxHealth(record);
        record.lifetimeId=nextBreakableLifetimeId_++;
        auto* allocation=breakableLifecycle_.nativeAllocation();
        if(!(allocation?allocation->publish(record.nativeRegistration,index):
             breakableLifecycle_.birth({index,record.lifetimeId},record))) {
#else
        breakables_[index].x = x[source];
        breakables_[index].y = y[source];
        breakables_[index].active = true;
        breakables_[index].destroyRequested = false;
        breakables_[index].state=creation?creation[source]:downwell::BreakableState{};
        observeBoxHealth(breakables_[index]);
        breakables_[index].alarm0=-1;
        breakables_[index].myFx={};
        breakables_[index].sound={};
        breakables_[index].nativeRegistration=0;
        breakables_[index].lifetimeId = nextBreakableLifetimeId_++;
        if(!breakableLifecycle_.birth({index,breakables_[index].lifetimeId},breakables_[index])) {
#endif
            run_.effectsSucceeded=false;return false;
        }
        if ((x[source] & 15) || (y[source] & 15) || x[source] < -breakableIndexLeft ||
            x[source] >= breakableIndexLeft || y[source] < 0 || y[source] >= 8192) {
            breakableIndexReady_ = false;
        } else if (breakableIndexReady_) {
            auto& cell = breakableIndex_[(y[source] >> 4) * breakableIndexColumns + ((x[source] + breakableIndexLeft) >> 4)];
            if (cell) breakableIndexReady_ = false;
            cell = index + 1;
        }
    }
    return true;
}

bool GameplayWorld::appendGeneratedBreakable(int x,int y,const downwell::GeneratedBreakableCreate& creation,
    std::uint32_t sourceId){
#ifndef DOWNWELL_REFERENCE_BREAKABLES
    if(auto* allocation=breakableLifecycle_.nativeAllocation()){
        if(!run_.effectsSucceeded||breakableCount_==breakableCapacity||!nextBreakableLifetimeId_)return false;
        if(sourceId){
            if(sourceId>0x7fffffffu)return run_.effectsSucceeded=false;
            for(unsigned i=0;i<breakableCount_;++i)
                if(breakables_[i].lifetimeId&&breakables_[i].sourceId==sourceId)return run_.effectsSucceeded=false;
        }
        const unsigned index=breakableCount_++;
        auto& record=breakables_[index];
        record.x=x;record.y=y;record.active=true;
        record.sourceId=sourceId;
        record.state.breakSound=static_cast<unsigned short>(creation.sound);
        record.state.haveGem=static_cast<unsigned char>(creation.gems);
        record.lifetimeId=nextBreakableLifetimeId_++;
        if(!allocation->publish(record.nativeRegistration,index)){
            run_.effectsSucceeded=false;return false;
        }
        if((x&15)||(y&15)||x < -breakableIndexLeft||x>=breakableIndexLeft||y<0||y>=8192)breakableIndexReady_=false;
        else if(breakableIndexReady_){
            auto& cell=breakableIndex_[(y>>4)*breakableIndexColumns+((x+breakableIndexLeft)>>4)];
            if(cell)breakableIndexReady_=false;
            cell=index+1;
        }
        return true;
    }
#endif
    // The retained family's bindings may read the builder; the native
    // allocation above runs no observer.
    publishPendingBuilderPosition();
    downwell::BreakableState state;
    state.breakSound=static_cast<unsigned short>(creation.sound);
    state.haveGem=static_cast<unsigned char>(creation.gems);
    return appendBreakables(&x,&y,1,&state,sourceId?&sourceId:nullptr);
}

bool GameplayWorld::breakableActiveAt(int x, int y) const
{
    if (breakableIndexReady_) {
        const int index = indexedBreakable(x, y);
        return index >= 0 && breakables_[index].x == x && breakables_[index].y == y &&
               breakables_[index].active&&!breakables_[index].destroyRequested;
    }
    DW_PROFILE_COUNT(counterBreakableFallback);
    unsigned index;
    for (index = 0; index < breakableCount_; ++index)
        if (breakables_[index].x == x && breakables_[index].y == y)
            return breakables_[index].active&&!breakables_[index].destroyRequested;
    return false;
}

int GameplayWorld::indexedBreakable(int x, int y) const {
    x += breakableIndexLeft + 8;
    y += 8;
    if (x < 0 || x >= 2 * breakableIndexLeft || y < 0 || y >= 8192) return -1;
    return (int)breakableIndex_[(y >> 4) * breakableIndexColumns + (x >> 4)] - 1;
}

void GameplayWorld::applyFrogAlarm(EnemyState& enemy, const downwell::FrogAlarmEffect& effect)
{
    if (effect.preparation >= 0) enemy.auxiliaryState = effect.preparation;
    if (effect.setJumpSpeed) enemy.motionState = effect.jumpSpeed;
    if (effect.setX) {
        enemy.frogHorizontal.velocity = effect.xQ16;
        enemy.speedX = downwell::FrogHorizontalMotion::toQ8(effect.xQ16);
    }
    if (effect.setY) {
        enemy.speedY = downwell::FrogVerticalMotion::floorDivide(effect.yFifthsQ8, 5);
        enemy.gravityRemainder = static_cast<unsigned char>(effect.yFifthsQ8 - enemy.speedY * 5);
    }
    if (effect.alarm == 0) enemy.alarm0 = effect.ticks;
    else enemy.alarm1 = effect.ticks;
    // Alarm 0's jump (116) or Alarm 1's prepare (113), from the frog itself.
    if (effect.sound >= 0) playEnemySound(effect.sound, enemy.y + 176);
}

// enmFrog Collision with enmFrog: of two overlapping frogs, the lower id (the
// earlier one in step order) acts, once per overlapping frog. If grounded it
// jumps as Alarm 0 does (ysp = -jumpsp, xsp = nsp, soundPlayOL(116, 50, 0, 1,
// "enemymove")); prepare = 0 and alarm[1] = 90 either way. No RNG.
void GameplayWorld::bumpCompactFrogs()
{
    // One pass collects the live frogs in step order with their bounds (the
    // serialized mask 166 for every frog); a jump changes speeds, not
    // positions, so the bounds hold for the whole pass. Owner-resident, never
    // a 68000 stack array.
    static EnemyState* frog[enemyCapacity];
    static downwell::ContactBounds box[enemyCapacity];
    unsigned frogs = 0;
    for (unsigned i = 0; i < enemyStepOrder_.size(); ++i) {
        auto* enemy = const_cast<EnemyState*>(resolveEnemyLifetime(enemyStepOrder_.handle(i)));
        if (!enemy || enemy->objectId != 113 || !enemy->active || enemy->destroyRequested) continue;
        frog[frogs] = enemy;
        box[frogs++] = downwell::contactWorldBounds({enemy->x * 256L, enemy->y * 256L, enemy->facing,
            enemy->imageScaleY(), 166});
    }
    for (unsigned i = 0; i + 1 < frogs; ++i)
        for (unsigned j = i + 1; j < frogs; ++j) {
            if (!downwell::contactBoundsOverlap(box[i], box[j])) continue;
            auto effect = downwell::frogJumpAlarm(frog[i]->grounded, frog[i]->motionState);
            effect.alarm = 1;
            effect.ticks = 90;
            applyFrogAlarm(*frog[i], effect);
        }
}

bool GameplayWorld::captureEnemyStepOrder(){
#ifndef DOWNWELL_REFERENCE_ENEMY_ORDER_CAPTURE
    // The unbound order is a function of the active records' slot, lifetime,
    // activation tick and object, in array order: if none changed, the
    // captured and sorted order stands.
    if(!enemyOrderCapture_&&unboundOrderKnown_){
        unsigned n=0;bool same=true;
        for(unsigned i=0;i<enemyCount_&&same;++i){
            const auto& e=enemies_[i];
            if(!e.active)continue;
            same=n<unboundOrderKeys_&&unboundOrderKey_[n].matches(e);++n;
        }
        if(same&&n==unboundOrderKeys_)return true;
    }
    unboundOrderKnown_=false;
#endif
    const auto revision=enemyOrderRevision_?enemyOrderRevision_(enemyOrderContext_):0xffffffffu;
    if(enemyOrderCapture_&&enemyOrderValid_&&revision!=0xffffffffu&&
       revision==enemyOrderCapturedRevision_)return true;
    enemyOrderValid_=false;
    enemyStepOrder_.clear();
    if(enemyOrderCapture_){
        if(!enemyOrderCapture_(enemyOrderContext_,enemyStepOrder_))return run_.effectsSucceeded=false;
    }else{
        // Unbound component worlds have no external registration owner. The
        // runner appends an instance to its object's list on Create and again
        // on instance_activate, so step active records by their last entry
        // (activatedTick), then by creation order. The array is compacted in
        // creation order, and activation sweeps it in that order.
        unsigned short order[enemyCapacity];unsigned count=0;
        for(unsigned i=0;i<enemyCount_;++i){
            if(!enemies_[i].active)continue;
            const auto tick=enemies_[i].activatedTick;unsigned at=count++;
            while(at&&enemies_[order[at-1]].activatedTick>tick){order[at]=order[at-1];--at;}
            order[at]=static_cast<unsigned short>(i);
        }
        for(unsigned n=0;n<count;++n){const auto& e=enemies_[order[n]];
            if(!enemyStepOrder_.append(e.identitySlot,e.lifetimeId,e.objectId))return run_.effectsSucceeded=false;
        }
#ifndef DOWNWELL_REFERENCE_ENEMY_ORDER_CAPTURE
        unboundOrderKeys_=0;
        for(unsigned i=0;i<enemyCount_;++i)if(enemies_[i].active)unboundOrderKey_[unboundOrderKeys_++].take(enemies_[i]);
        unboundOrderKnown_=true;
#endif
    }
    enemyStepOrder_.sort();
    enemyOrderCapturedRevision_=revision;enemyOrderValid_=true;return true;
}

void GameplayWorld::advanceEnemyAlarms()
{
    DW_PROFILE_PHASE(profileEnemyAlarms);
    for (unsigned enemyIndex = 0; enemyIndex < enemyCount_; ++enemyIndex) {
        EnemyState& enemy = enemies_[enemyIndex];
        if (!enemy.active)
            continue;
        if (enemy.objectId == 113) enemy.frogHit.advanceFrame();
        if (enemy.objectId == 117) enemy.snailHit.advanceFrame();
        if (enemy.objectId == 116) enemy.batHit.advanceFrame();
    }
    // objControlerN 37 is the first object at alarm index 0.
    advanceCompactControllerAlarm();
    advanceCompactHudAlarms();
    if(roomAlarms_){
        if(!roomAlarms_(roomAlarmsContext_)){run_.effectsSucceeded=false;return;}
        // Compact fxTimeChange 211, fxTimeShard 238, gemChunk 293 and debris
        // 370 follow the room's objects at every index. Their alarm events
        // draw no RNG, so dispatching them after the room's later indices
        // preserves the stream.
        if(timeStop_.changes.size()||timeStop_.shards.size()){
            const auto changePhase=timeStop_.changes.beginPhase();
            const auto shardPhase=timeStop_.shards.beginPhase();
            for(unsigned alarm=0;alarm<3;++alarm)if(!advanceTimeEffectAlarm(changePhase,shardPhase,alarm))return;
        }
        // Compact objDeadBody 263 (Alarms 0-3) likewise draws no RNG.
        if(compactCorpses_.size())
            for(unsigned alarm=0;alarm<4;++alarm)if(!advanceCompactCorpseAlarm(alarm))return;
        if(scenery_.gemChunkEnd||compactDebris_.size())
            for(unsigned alarm=0;alarm<3;++alarm)if(!advanceCompactSceneryAlarm(alarm))return;
        return;
    }
    // Match the runner's global alarm-index order. Dispatch each event before
    // counting down later slots: frog Alarm 0 writes Alarm 1, for example.
    // Object order within each index: boxes 92, enemies 111-119, then the
    // compact gemChunk 293 and breakablesDebris 370, with fxTimeChange 211
    // and fxTimeShard 238 then objDeadBody 263 between the enemies and the
    // scenery. Only corpses have an Alarm 3.
    const bool timeEffects=timeStop_.changes.size()||timeStop_.shards.size();
    const auto changePhase=timeStop_.changes.beginPhase();
    const auto shardPhase=timeStop_.shards.beginPhase();
    const unsigned alarms=compactCorpses_.size()?4:scenery_.gemChunkEnd||compactDebris_.size()||timeStop_.shards.size()?3:2;
    // Every enemy alarm event acts on its own enemy (its state, alarms, RNG
    // draws, sounds and spawned effects). The unbound step order (activation
    // tick, then object) therefore decides only the order of events due at
    // the same index: with at most one due, the others merely count down and
    // the order is neither captured nor sorted. Births during the index wait.
#ifndef DOWNWELL_REFERENCE_ENEMY_ALARM_ORDER
    const bool unorderedAlarms=!enemyOrderCapture_;
#else
    const bool unorderedAlarms=false;
#endif
    bool ordered=false;
    const unsigned enemyEnd=enemyCount_;
    for (unsigned alarm = 0; alarm < alarms; ++alarm) {
      if(alarm==0){
        if(!run_.effectsSucceeded)return;
        // A slot's own handle always resolves: no lookup per box.
        for(unsigned i=0;i<breakableCount_;++i){
            auto& box=breakables_[i];
            if(box.alarm0<0||!box.lifetimeId||!box.active||box.destroyRequested)continue;
            if(downwell::AlarmBank::advanceValue(box.alarm0)){
                box.state.alarmZero();observeBoxHealth(box);
                if(!run_.effectsSucceeded)return;
            }
        }
      }
      if(alarm<2){
        unsigned due=0;
        if(unorderedAlarms&&!ordered)
            for(unsigned i=0;i<enemyEnd&&due<2;++i){
                const auto& enemy=enemies_[i];
                if(!enemy.active||enemy.destroyRequested||
                   !((downwell::enemyAlarmEvents(enemy.objectId)>>alarm)&1u))continue;
                due+=(alarm?enemy.alarm1:enemy.alarm0)==1;
            }
        if(unorderedAlarms&&!ordered&&due<2){
            if(!run_.effectsSucceeded)return;
            for(unsigned i=0;i<enemyEnd;++i){
                auto& enemy=enemies_[i];
                if(!enemy.active||enemy.destroyRequested)continue;
                if(!advanceEnemyAlarm(enemy,alarm))return;
            }
        }else{
            if(!ordered){if(!captureEnemyStepOrder())return;ordered=true;}
            for (unsigned i=0;i<enemyStepOrder_.size();++i) {
                if(!resolveEnemyLifetime(enemyStepOrder_.handle(i)))continue;
                if(!advanceEnemyRoomAlarm(enemyStepOrder_.handle(i),alarm))return;
            }
        }
      }
      if(timeEffects&&alarm<3&&!advanceTimeEffectAlarm(changePhase,shardPhase,alarm))return;
      if(compactCorpses_.size()&&!advanceCompactCorpseAlarm(alarm))return;
      if(!advanceCompactSceneryAlarm(alarm))return;
    }
}

bool GameplayWorld::advanceTimeEffectAlarm(TimeChangePhase changePhase,TimeShardPhase shardPhase,unsigned alarm){
    if(alarm<2)timeStop_.changes.advanceAlarm(changePhase,alarm,timeStop_.roomSpeed);
    timeStop_.shards.advanceAlarm(shardPhase,alarm);
    if(!timeStop_.changes.succeeded()||!timeStop_.shards.succeeded())return run_.effectsSucceeded=false;
    return true;
}

bool GameplayWorld::advanceBoxRoomAlarm(downwell::InstanceHandle key,unsigned alarm){
    if(!run_.effectsSucceeded||!findBreakable(key)||alarm>=downwell::AlarmBank::count)
        return run_.effectsSucceeded=false;
    auto& box=breakables_[key.slot];
    if(!box.active||box.destroyRequested||alarm!=0)return true;
    if(downwell::AlarmBank::advanceValue(box.alarm0)){box.state.alarmZero();observeBoxHealth(box);}
    return true;
}

bool GameplayWorld::advanceEnemyRoomAlarm(downwell::InstanceHandle key,unsigned alarm){
        auto* live=const_cast<EnemyState*>(resolveEnemyLifetime(key));
        if(!run_.effectsSucceeded||!live||alarm>=downwell::AlarmBank::count)return run_.effectsSucceeded=false;
        return advanceEnemyAlarm(*live,alarm);
}

bool GameplayWorld::advanceEnemyAlarm(EnemyState& enemy,unsigned alarm){
        if (!enemy.active||enemy.destroyRequested) return true;
        const unsigned events = downwell::enemyAlarmEvents(enemy.objectId);
        const bool alarm0Fired = alarm == 0 && downwell::AlarmBank::advanceValue(enemy.alarm0, (events & 1) != 0);
        const bool alarm1Fired = alarm == 1 && downwell::AlarmBank::advanceValue(enemy.alarm1, (events & 2) != 0);

        if (enemy.objectId == 112 && alarm1Fired) {
            enemy.auxiliaryState = -enemy.auxiliaryState;
            enemy.alarm1 = 120 + (int)enemyRandom_.irandomSmall(180);
        } else if (enemy.objectId == 113) {
            if (alarm0Fired)
                applyFrogAlarm(enemy, downwell::frogJumpAlarm(enemy.grounded, enemy.motionState));
            if (alarm1Fired) {
                const int direction = downwell::frogPreparationDirection(
                    enemy.grounded, enemy.x * 256L, enemy.y * 256L,
                    player_.frameStartXFixed(), (cameraPixelX_ - 80) * 256L,
                    viewCameraYQ8() - 142 * 256L, (cameraPixelX_ + 80) * 256L,
                    viewCameraYQ8() + 142 * 256L);
                applyFrogAlarm(enemy, downwell::frogPreparationAlarm(direction));
            }
        } else if (enemy.objectId == 116 && alarm0Fired) {
            enemy.batHit.alarm();
        } else if (enemy.objectId == 119 && alarm0Fired) {
            updateCrawler(enemy, true, cameraPixelX_, viewCameraYQ8());
            enemy.alarm0 = 2;
        } else if ((enemy.objectId == 115 || enemy.objectId == 118 || enemy.objectId == 173) && (alarm0Fired || alarm1Fired)) {
            return advanceSteeringAlarm(enemy, alarm0Fired ? 0 : 1);
        }
        return run_.effectsSucceeded;
}

void GameplayWorld::updateCrawler(EnemyState& enemy, bool alarm, int cameraX, long cameraYQ8)
{
    // Only a latched alarm and an unlatched Step read the view.
    bool visible = false;
    if (alarm == enemy.crawler.latched) {
        const long cx = cameraX * 4096L;
        const long cy = cameraYQ8 * 16;
        visible = downwell::CrawlerMotion::inView(enemy.x, enemy.y,
            cx - 80L * 4096, cy - 142L * 4096, cx + 80L * 4096, cy + 142L * 4096,
            alarm ? 0 : 64);
    }
    using Math=EnemyMotionMath;
    using Crawler=decltype(enemy.crawler);
    const auto move = [&](auto solid) {
        const auto meeting = [&solid](Math::Number x, Math::Number y) { return Crawler::meetsTiles(x, y, solid); };
        if (alarm) enemy.crawler.alarm(enemy.motionState > 0, visible, meeting);
        else enemy.crawler.step(enemy.motionState > 0, visible, meeting);
    };
    if (nativeSolidCells()) move(solidCells());
    else move([this](int px, int py) { return solidResolver_ && solidResolver_(solidResolverContext_, px, py); });
    enemy.preciseX = Math::truncatedQ8(enemy.crawler.x);
    enemy.preciseY = Math::truncatedQ8(enemy.crawler.y);
    enemy.speedX = Math::truncatedQ8(enemy.crawler.vx);
    enemy.speedY = Math::truncatedQ8(enemy.crawler.vy);
    enemy.grounded = enemy.crawler.latched;
}

bool GameplayWorld::advanceEnemyRoomAnimation(downwell::InstanceHandle key){
    auto* enemy=const_cast<EnemyState*>(resolveEnemyLifetime(key));
    if(!run_.effectsSucceeded||!enemy)return run_.effectsSucceeded=false;
    // Step under pTimeStop set image_speed 0: this advance holds.
    if(!enemy->frozen)
        downwell::advanceEnemyImageQ8(enemy->imagePhase,enemy->imageRemainder,enemy->imageSpeed);
    return publishEnemyImage(*enemy,enemy->imagePhase);
}
bool GameplayWorld::advanceBulletRoomAnimation(downwell::InstanceHandle key){
    auto* bullet=const_cast<BulletState*>(resolveBulletLifetime(key));
    if(!run_.effectsSucceeded||!bullet)return run_.effectsSucceeded=false;
    return run_.effectsSucceeded=advanceBulletAnimation(*bullet);
}

bool GameplayWorld::boneInView(downwell::InstanceHandle bone,bool& visible){
    if(!run_.effectsSucceeded||!bones_||!bones_->valid())return run_.effectsSucceeded=false;
    const auto* caller=bones_->find(bone);
    if(!caller)return run_.effectsSucceeded=false;
    auto top=downwell::MotionBinary64::fromInteger(boneViewTopQ8_);
    if(!top.zero())top.high-=8u<<20; // Exact Q8 conversion, including fixture camera tracks.
    return run_.effectsSucceeded=downwell::boneInPublishedView(caller->x.promoted(),caller->y.promoted(),
        downwell::MotionBinary64::fromInteger(boneViewLeft_),top,
        downwell::MotionBinary64::fromInteger(160),downwell::MotionBinary64::fromInteger(284),visible);
}

bool GameplayWorld::timeShardOutsideView(downwell::RunnerImageValue publishedY,bool& retire){
    if(!run_.effectsSucceeded)return false;
    // GameplayWorld owns rmMain. outOfView(0,320) reads the published camera
    // cache captured at frame entry, not camMain's subsequently updated y.
    // Keep the caller's old published y, including fractional birth positions.
    auto top=downwell::MotionBinary64::fromInteger(boneViewTopQ8_/256);
    int order;
    if(!downwell::compareRunnerReal(publishedY.promoted(),top,order))return run_.effectsSucceeded=false;
    if(order<0){retire=true;return true;}
    if(!top.add(downwell::MotionBinary64::fromInteger(320))||
       !downwell::compareRunnerReal(publishedY.promoted(),top,order))return run_.effectsSucceeded=false;
    retire=order>0;return true;
}

// The compact shards' published y is a whole pixel; the view top is
// boneViewTopQ8_/256 as above, and the comparisons are exact.
bool GameplayWorld::timeShardOutsideView(std::int32_t publishedY,bool& retire)const{
    if(!run_.effectsSucceeded)return false;
    const long top=boneViewTopQ8_/256;
    retire=publishedY<top||publishedY>top+320;return true;
}

bool GameplayWorld::stepEnemyRoomEffect(downwell::InstanceHandle key,int publishedCameraX,long generationCameraY)
{
    auto* live=const_cast<EnemyState*>(resolveEnemyLifetime(key));
    if(!live)return true;
    EnemyState& enemy = *live;
    if (!enemy.active||enemy.destroyRequested)
        return true;
    // Steering families and the eyefish step whenever active; they have no
    // awake gate.
    if (enemy.objectId == 115 || enemy.objectId == 118 || enemy.objectId == 173)
        return stepSteeringEnemy(enemy);
    if (enemy.objectId == 114)
        return stepEyefish(enemy);
    // Each family's Step body is inside if (!global.pTimeStop); the
    // presentation after it still runs, on the unchanged motion state.
    const bool frozen=compactTimeStop()&&stopped();
    enemy.frozen=frozen;
    if (frozen && !freezeEnemyStep(enemy)) return false;
    if (!frozen && enemy.objectId == 113)
        enemy.imageSpeed = enemy.frogPresentation.beginStepRate();
    if (!frozen && !enemy.awake && enemy.y - cameraPixelY_ <= 254)
        enemy.awake = true;
    if ((!enemy.awake && enemy.objectId != 113 && enemy.objectId != 111 && enemy.objectId != 112 && enemy.objectId != 116) || solidResolver_ == 0)
        return true;
    if (frozen) {
    } else if (enemy.objectId == 111 || enemy.objectId == 112) {
        const bool turtle = enemy.objectId == 112;
        DW_PROFILE_SCOPE(profileEnemyWalker);
        const long cx = publishedCameraX * 4096L;
        const long cy = generationCameraY * 16;
        const bool visible = downwell::CrawlerMotion::inView(enemy.x, enemy.y,
            cx - 80L * 4096, cy - 142L * 4096, cx + 80L * 4096, cy + 142L * 4096,
            turtle ? 64 : 32);
        const auto presentation=[this,&enemy](int sprite,int scale){
            enemy.grounded=enemy.walker.grounded;
            if(!publishEnemySprite(enemy,sprite))return false;
            if(scale){
                if(!publishEnemyTransform(enemy,downwell::RunnerTransformField::scaleX,scale))return false;
                enemy.facing=scale;
            }
            return true;
        };
        const auto walk=[&](auto solid,EnemyMotionMath::Number floorLimit){
            return enemy.walker.step(turtle, downwell::groundTurtleWalking(enemy.auxiliaryState), visible,
                [&](EnemyMotionMath::Number x, EnemyMotionMath::Number y) {
                    return enemy.walker.meetsTiles(x, y, turtle, solid);
                },presentation,floorLimit);
        };
        // The cell grid ends the terrain (W1.12); a resolver has no end, so
        // its count stops 4,096 pixels down.
        if(!(nativeSolidCells() ?
             walk(solidCells(),EnemyMotionMath::pixels(int(solidCells_.height)-solidCells_.originY+32)) :
             walk(downwell::SolidCornerQuery{solidResolver_,solidResolverContext_,solidCorners_},
                  enemy.walker.y+EnemyMotionMath::pixels(4096))))return false;
        using Math=EnemyMotionMath;
        enemy.preciseX = Math::floorQ8(enemy.walker.x);
        enemy.preciseY = Math::floorQ8(enemy.walker.y);
        enemy.speedX = Math::floorQ8(enemy.walker.vx);
        enemy.speedY = Math::floorQ8(enemy.walker.vy);
        enemy.gravityPositionRemainder = static_cast<unsigned char>(Math::floorQ8Remainder(enemy.walker.y));
        enemy.gravityRemainder = static_cast<unsigned char>(Math::floorQ8Remainder(enemy.walker.vy));
        enemy.grounded = enemy.walker.grounded;
    } else if (enemy.objectId == 116) {
        downwell::BatMotion bat{enemy.preciseX, enemy.preciseY, enemy.speedX, enemy.speedY,
            enemy.direction, (enemy.moveSpeed * 40 + 128) / 256, enemy.facing,
            enemy.batDirectionFraction};
        DW_PROFILE_SCOPE(profileEnemyBat);
        bat.collisionMask=downwell::enemyContactMask(enemy.objectId,enemy.spriteId,enemy.maskId);
        const auto effects = downwell::stepBatBehavior(bat, enemy.batHit, enemy.batActivity,
            enemy.x, enemy.y, bat.collisionMask,
            balloonHeld()?(balloon_.x-240)*256L:player_.frameStartXFixed(),
            balloonHeld()?(balloon_.y-176)*256L:player_.frameStartYFixed(),
            player_.frameStartYFixed(), enemyRandom_,
            downwell::SolidCornerQuery{solidResolver_,solidResolverContext_,solidCorners_},[this,&enemy](int scale){
                if(!publishEnemyTransform(enemy,downwell::RunnerTransformField::scaleX,scale))return false;
                enemy.facing=scale;return true;
            });
        enemy.preciseX = bat.x;
        enemy.preciseY = bat.y;
        enemy.speedX = bat.speedX;
        enemy.speedY = bat.speedY;
        enemy.direction = bat.direction;
        enemy.batDirectionFraction = bat.directionFraction;
        enemy.facing = bat.facing;
        enemy.moveSpeed = (bat.speedFortieths * 256 + 20) / 40;
        if(!effects.succeeded)return false;
        // enmBat Step: the hit's sfxBatHit, then a sfxBatFly per wake test.
        if (effects.hit) playEnemySound(112, enemy.y + 176);
        for (unsigned n = 0; n < effects.wakeSounds; ++n) playEnemySound(110, enemy.y + 176);
        if (effects.hit) {
            enemy.alarm0 = 3;
            if(!publishEnemyImage(enemy,0))return false;
            enemy.imagePhase = 0;
            enemy.imageRemainder = 0;
        }
    } else if (enemy.objectId == 113) {
        const bool integrate = downwell::frogMotionVisible(enemy.x * 256L, enemy.y * 256L,
            (publishedCameraX - 80) * 256L, generationCameraY - 142 * 256L,
            (publishedCameraX + 80) * 256L, generationCameraY + 142 * 256L);
        DW_PROFILE_SCOPE(profileEnemyFrog);
        downwell::FrogVerticalMotion vertical{
            enemy.preciseY * 5 + enemy.gravityPositionRemainder,
            enemy.speedY * 5 + enemy.gravityRemainder, enemy.grounded};
        struct FrogOps {
            GameplayWorld& world;EnemyState& enemy;
            // Mask 166 (symmetric, so image_xscale does not move it) at room
            // offsets: x in Q16, y in fifths of Q8, whose fraction maps to
            // Q16 exactly at whole pixels and halves.
            bool meeting(int object,std::int32_t x,std::int32_t y,bool& hit){
                std::int32_t whole=y/1280,fraction=y%1280;
                if(fraction<0){fraction+=1280;--whole;}
                downwell::ContactBounds bounds;
                if(!downwell::CurrencyCollisionBounds::calculateMaskQ16(
                       downwell::enemyContactMask(113,enemy.spriteId),x+240*65536,
                       (whole+176)*65536+fraction*256/5,bounds))
                    return world.run_.effectsSucceeded=false;
                if(object==57)return world.steeringWallContact(bounds,hit);
                if(object==56)return world.solidContact(bounds,true,hit);
                // parentThinwall 87: rmMain's only thin walls are the
                // generated platforms; worlds without the platform resolver
                // (host fixtures) have none, as solidContact assumes.
                if(object!=87||!world.currencyOnlyPlatforms_)return world.run_.effectsSucceeded=false;
                hit=false;
                return !world.platformContactResolver_||
                    world.platformContactResolver_(world.solidResolverContext_,bounds,hit)||
                    (world.run_.effectsSucceeded=false);
            }
            bool hit(){
                enemy.auxiliaryState = 0;
                enemy.alarm0 = 0;
                enemy.alarm1 = 60;
                enemy.speedX = downwell::FrogHorizontalMotion::toQ8(enemy.frogHorizontal.velocity);
                if(!world.publishEnemyImage(enemy,0))return false;
                enemy.imagePhase = 0;
                enemy.imageRemainder = 0;
                return true;
            }
            bool scale(int sign){
                if(!world.publishEnemyTransform(enemy,downwell::RunnerTransformField::scaleX,sign))return false;
                enemy.facing=sign;return true;
            }
            void landing(bool grounded){
                if(downwell::frogLandingEdge(grounded,enemy.frogLanded))enemy.alarm1=60;
            }
        } ops{*this,enemy};
        downwell::FrogSourceStep source;
        const auto effects = source.step(enemy.frogHorizontal, vertical, enemy.frogHit, integrate, ops);
        enemy.preciseY = downwell::FrogVerticalMotion::floorDivide(vertical.position, 5);
        enemy.gravityPositionRemainder = static_cast<unsigned char>(vertical.position - enemy.preciseY * 5);
        enemy.speedY = downwell::FrogVerticalMotion::floorDivide(vertical.velocity, 5);
        enemy.gravityRemainder = static_cast<unsigned char>(vertical.velocity - enemy.speedY * 5);
        enemy.grounded = vertical.grounded;
        enemy.preciseX = downwell::FrogHorizontalMotion::toQ8(enemy.frogHorizontal.position);
        enemy.speedX = downwell::FrogHorizontalMotion::toQ8(enemy.frogHorizontal.velocity);
        if(!effects.succeeded)return false;
        if (effects.hit) playEnemySound(115, enemy.y + 176); // sfxFrogHit
    } else if (enemy.objectId == 117) {
        DW_PROFILE_SCOPE(profileEnemySnail);
        const int hitAlarm = enemy.snailHit.consume(enemyRandom_);
        if (hitAlarm >= 0) {
            playEnemySound(120, enemy.y + 176); // sfxSlugHit
            if(!publishEnemyImage(enemy,0))return false;
            enemy.imagePhase = 0; enemy.imageRemainder = 0;
            enemy.alarm0 = 0; enemy.alarm1 = hitAlarm;
        }
        const auto transform=[this,&enemy](downwell::RunnerTransformField field,int value){return publishEnemyTransform(enemy,field,value);};
        const bool alive = nativeSolidCells() ? enemy.snail.step(solidCells(),false,transform) :
            enemy.snail.step([this](int x, int y) {
                return solidResolver_(solidResolverContext_, x, y);
            },false,transform);
        if(!run_.effectsSucceeded)return false;
        if (!alive && !publishEnemyActivation(enemy,false)) return false;
        enemy.active = alive;
        if (!enemy.active)
            return true;
        enemy.preciseX = EnemyMotionMath::truncatedQ8(enemy.snail.x);
        enemy.preciseY = EnemyMotionMath::truncatedQ8(enemy.snail.y);
        enemy.speedY = EnemyMotionMath::truncatedQ8(enemy.snail.vy);
        enemy.motionState = enemy.facing = enemy.snail.facing;
    } else if (enemy.objectId == 119) {
        DW_PROFILE_SCOPE(profileEnemyCrawler);
        updateCrawler(enemy, false, publishedCameraX, generationCameraY);
    }
    DW_PROFILE_SCOPE(profileEnemyFinalize);
    // World origins (240,176) are even integers, so ties-to-even is
    // unchanged by storing these positions relative to the origin.
    int publishedX, publishedY;
#ifndef DOWNWELL_REFERENCE_ENEMY_UPDATES
    // Only the bat publishes its rounded Q8 position; the others replace it.
    if (enemy.objectId == 113 || enemy.objectId == 111 || enemy.objectId == 112 ||
        enemy.objectId == 119 || enemy.objectId == 117) publishedX = publishedY = 0; else
#endif
    {
        publishedX = (int)downwell::roundGameMakerQ8(enemy.preciseX);
        publishedY = (int)downwell::roundGameMakerQ8(enemy.preciseY);
    }
    if (enemy.objectId == 113) {
        publishedX = (int)downwell::frogPublishedPixel(enemy.frogHorizontal.position,65536);
        publishedY = (int)downwell::frogPublishedPixel(
            enemy.preciseY*5+enemy.gravityPositionRemainder,1280);
    }
    if (enemy.objectId == 111 || enemy.objectId == 112) {
        publishedX = enemy.walker.publishedX;
        publishedY = enemy.walker.publishedY;
    }
    if (enemy.objectId == 119) {
        publishedX = enemy.crawler.pixelX();
        publishedY = enemy.crawler.pixelY();
    }
    if (enemy.objectId == 117) {
        publishedX = EnemyMotionMath::roundPixel(enemy.snail.x);
        publishedY = EnemyMotionMath::roundPixel(enemy.snail.y);
    }
    { DW_PROFILE_SCOPE(profileEnemyPositionPublish);
    if(!enemyLifecycle_.recordOwnsAppearance()&&
       !enemyLifecycle_.integerPosition({enemy.identitySlot,enemy.lifetimeId},enemy,
                                       publishedX+240,publishedY+176)) {
        run_.effectsSucceeded=false;return false;
    }
    }
    enemy.x=publishedX;enemy.y=publishedY;
    if(enemy.objectId==119 &&
       !publishEnemyTransform(enemy,downwell::RunnerTransformField::angle,enemy.crawler.imageDirection*90))return false;
    if (enemy.objectId!=116 && enemy.objectId!=113 && enemy.speedX != 0)
        enemy.facing = enemy.speedX < 0 ? -1 : 1;
    const auto nextSprite = downwell::finishEnemyPresentation(
        {enemy.objectId, enemy.auxiliaryState, enemy.grounded, enemy.speedY < 0,
         enemy.frogHit.remaining != 0, enemy.snailHit.stunned},
        enemy.frogPresentation, enemy.batActivity, enemy.batHit,
        enemy.imagePhase, enemy.imageRemainder);
    { DW_PROFILE_SCOPE(profileEnemyPresentationPublish);
    if(enemy.objectId!=111 && enemy.objectId!=112 && !publishEnemySprite(enemy,nextSprite))return false;
    enemy.spriteId=nextSprite;
    if(enemy.objectId==116 && !downwell::finishBatFields(enemy.batActivity,nextSprite,
        [this,&enemy](int mask){
            if(!enemyLifecycle_.mask({enemy.identitySlot,enemy.lifetimeId},enemy,mask)){
                run_.effectsSucceeded=false;return false;
            }
            enemy.maskId=mask;return true;
        },[this,&enemy](int scale){
            if(!publishEnemyTransform(enemy,downwell::RunnerTransformField::scaleX,scale))return false;
            enemy.facing=scale;return true;
        },[&enemy]{enemy.direction=270;enemy.batDirectionFraction=0;enemy.moveSpeed=512;}))return false;
    if(!publishEnemyImage(enemy,enemy.imagePhase))return false;
    }
    enemy.imageFrame = enemy.imagePhase >> 8;
    return true;
}

bool GameplayWorld::stepBulletRoomEffect(downwell::InstanceHandle key,long generationCameraY,bool& bulletChanged)
{
    auto* bullet=const_cast<BulletState*>(resolveBulletLifetime(key));
    return !bullet||stepBulletEffect(*bullet,generationCameraY,bulletChanged);
}

bool GameplayWorld::stepBulletEffect(BulletState& bullet,long generationCameraY,bool& bulletChanged)
{
    if(!bullet.active||bullet.destroyRequested)return true;
    bulletChanged = true;
    if(sourceBulletDispatch_){
        if(!bullet.sourceOwned){
            if(bullet.trajectory.initialized||!bullet.sourceMotion.reset(bullet.sourceMotion.x.promoted(),
                bullet.sourceMotion.y.promoted(),bullet.trajectory.zeroSpread))return run_.effectsSucceeded=false;
            bullet.sourceOwned=true;
        }
        const bool ok=sourceBulletDispatch_(sourceBulletContext_,{bullet.identitySlot,bullet.lifetimeId},bullet.sourceMotion);
        return run_.effectsSucceeded=run_.effectsSucceeded&&ok;
    }
    if(bullet.kind==BulletState::ranged)return stepRangedBullet(bullet,generationCameraY);
    if(bullet.kind==BulletState::laser)return stepLaserBullet(bullet,generationCameraY);
    const long cameraCentreQ12 = 176L * 4096 + generationCameraY * 16;
    bool alive=false;
    if(!downwell::stepMachineGunProjectileChecked(bullet, enemyRandom_,
        [this, &bullet](const downwell::ProjectileSegment& path) {
            bool consumed=false;
            if(!bulletEnemyContact(bullet,path,consumed))return true;
            return consumed;
        }, [this, &bullet](const downwell::ProjectileSegment& path) {
            bool consumed=false;
            if(!bulletSolidContact(bullet,path,consumed))return true;
            return consumed;
        },alive,[this]{return run_.effectsSucceeded;},176,cameraCentreQ12-142L*4096,284,
        [this,&bullet]{
            DW_PROFILE_SCOPE(profileBulletPublication);
            if(!bulletLifecycle_.hasOwner())return true;
            downwell::RunnerImageValue angle;
            if(!downwell::ProjectilePublication::angle(bullet.trajectory.direction,angle)||
                !publishBulletAngle({bullet.identitySlot,bullet.lifetimeId},angle))
                return run_.effectsSucceeded=false;
            return true;
        },!registeredOrdinaryEndStep_))return false;
    if(bulletLifecycle_.hasOwner()){
        DW_PROFILE_SCOPE(profileBulletPublication);
        const downwell::InstanceHandle h{bullet.identitySlot,bullet.lifetimeId};
        downwell::RunnerImageValue x,y;
        if(!downwell::ProjectilePublication::coordinate(bullet.trajectory.x,240,x)||
            !downwell::ProjectilePublication::coordinate(bullet.trajectory.y,176,y)||
            !publishBulletCollisionPosition(h,x,y)){
            run_.effectsSucceeded=false;return false;
        }
    }
    if(!alive){
        const downwell::InstanceHandle h{bullet.identitySlot,bullet.lifetimeId};
        if(bulletLifecycle_.hasOwner()){
            if(!markBulletDestroyed(h))return false;
        }else if(!retireBulletOwnership(bullet))return false;
    }
    return true;
}

GameplayFrame GameplayWorld::step(std::uint8_t packedInput)
{
    if(!sourceBulletDispatch_)for(unsigned i=0;i<bulletSlotEnd_;++i){const auto& b=bullets_[i];
        if(b.identitySlot<bulletCapacity&&b.active&&!b.destroyRequested&&b.sourceOwned){run_.effectsSucceeded=false;return {};}
    }
    if(playerBirthPending_){run_.effectsSucceeded=false;return {};}
    if(collisionPassExecuting_){run_.effectsSucceeded=false;return {};}
    if (run_.levelClearActive || !run_.effectsSucceeded || !generationSucceeded_) return {}; // Result owner uses stepLevelClear.
    advanceCompactHudDraw();
    boneViewLeft_=160+cameraPixelX_;
    boneViewTopQ8_=34L*256+viewCameraYQ8();
    downwell::GameMakerRandom::FrameRefill refill{enemyRandom_};
    if(scriptSpawnInstances_&&scriptSpawnInstances_->size())scriptEffectsBinding_.beginAnimation();
    else scriptEffectsBinding_={};
    if(debrisInstances_&&debrisInstances_->size())debrisBinding_.beginAnimation();
    else debrisBinding_={};
    // Only valid unit-sequence state is admitted through the effect owner.
    {
    DW_PROFILE_SCOPE(profileStandaloneEffectAnimation);
    // Currency has no Animation End callback; only gems present at this
    // boundary participate. Rewards born later in this tick must not advance.
    for(unsigned i=0;i<gemSlotEnd_;++i)
        if(gems_[i].active&&!gems_[i].frozen)gems_[i].motion.animation.advance();
    run_.advanceFlashAnimation();
    run_.advanceSmokeAnimation();
    run_.advanceCorpseExpiryAnimation();
    sceneryRepaintCount_=0;
    (void)advanceCompactSceneryAnimation();
    if(!guns_.empty())(void)advanceCompactGunAnimation();
    // fxTimeShard 238 has no Animation End event.
    if(timeStop_.shards.size()&&!timeStop_.shards.advanceAnimation(timeStop_.shards.beginPhase()))
        run_.effectsSucceeded=false;
    // Nor does objDeadBody 263: its admitted sprites have one frame, so the
    // timeline only wraps within [0, 1) and every draw is frame 0. The
    // compact corpses skip it and keep image_index 0 (W3 fixed point);
    // the registered corpses still advance CorpseAnimation.
    if(blastEnd_)advanceCompactBlastAnimation();
    if(bloodEnd_&&!advanceCompactBloodAnimation())return {};
    advanceCompactDroneAnimation();
    if(balloon_.live&&!advanceCompactBalloonAnimation())return {};
    }
    if(!run_.effectsSucceeded)return {};
    {
    DW_PROFILE_SCOPE(profileRoomAnimation);
    if(roomAnimation_){
        if(!roomAnimation_(roomAnimationContext_)){run_.effectsSucceeded=false;return {};}
    }else{
    for(unsigned i=0;i<bulletSlotEnd_;++i)if(bullets_[i].active){
        if(!advanceBulletAnimation(bullets_[i])){run_.effectsSucceeded=false;return {};}
    }
    // Runner advances existing instances' animation before Begin Step/alarms.
    // In particular, an Alarm/Step frame reset must not then be advanced again.
    for (unsigned i=0;i<enemyCount_;++i) if (enemies_[i].active&&!enemies_[i].frozen) {
        downwell::advanceEnemyImageQ8(enemies_[i].imagePhase,enemies_[i].imageRemainder,
                                     enemies_[i].imageSpeed);
        if(!publishEnemyImage(enemies_[i],enemies_[i].imagePhase))return {};
    }
    }
    }
    DW_PROFILE_PHASE(profilePlayer);
    // Wrappers own a runtime pointer; rebind after any host-side runtime copy.
    if (solidResolver_) configureSolidResolver(solidResolver_, solidResolverContext_, segmentResolver_, maskResolver_,wallContactResolver_,platformContactResolver_,solidCorners_);
    if (landingResolver_) configureLandingResolver(landingResolver_, landingResolverContext_);
    // camera_get_view_y reads the runner's published cache, not camMain.y.
    // Normal following refreshes before Draw; post-Step refresh is dirty-only.
    const long generationCameraY = viewCameraYQ8();
    const int publishedCameraX = cameraPixelX_;
    const int oldAmmo = player_.ammo();
    const int oldAmmoCapacity = player_.ammoCapacity();
    const int oldHealth = health_;
    const int oldHealthMaximum = healthMaximum();
    const unsigned oldHeartPieces = healthReserve().pieces;
    const unsigned oldCurrency = currencyAccount_.currency;
    const unsigned oldGemStreak = currencyAccount_.streak;
    const unsigned oldShots = player_.shots();
    const int ammoBeforeStep=player_.ammo(); // scrShotSound reads stammo before the charge is spent
    bool bulletChanged = false;
    unsigned destroyedBlockCount = 0;
    int destroyedBlockX[destroyedCapacity] = {0};
    int destroyedBlockY[destroyedCapacity] = {0};

    ++tick_;
    firedThisStep_=false;
    // W2.3: the audio clock of this source step (see SoundVoices).
    run_.soundRequests.setTick(tick_);
    run_.startRechargeSounds(player_);
    run_.voices.advance();
    // objControlerN (37) runs after objPlayer_n (35), so it sees this
    // update's pTimeStop. Apply the previous value here; the player's
    // transition below corrects the streak timer on the frames it changes.
    const auto streakBeforeStep=currencyAccount_.streak;
    const auto streakTimerBeforeStep=currencyAccount_.timer;
    const bool stoppedBeforeStep=stopped();
    currencyAccount_.beginStep(false,stoppedBeforeStep);
    {
        // scrControlBeginstepCheck: sfxGemBoostEnter on reaching gem high,
        // sfxGemBoostEnd on leaving it unless dead.
        const bool wasHigh=run_.gemHigh;
        run_.selectWeaponLevel(currencyAccount_);
        if(run_.gemHigh&&!wasHigh)run_.soundPlay(158,90,false,true);
        else if(!run_.gemHigh&&wasHigh&&!death_.active)run_.soundPlay(160,90,false,true);
    }
    run_.consumables.beginStep(player_, health_, run_.combo.healthReserve);
    if(!checkKnifeAndFork())return {};
    downwell::Input input = downwell::unpackReplayInput(packedInput);
    run_.results.beginStep(input,encounterInput_?&encounterInput_->publication():nullptr);
    run_.playResultsStepSounds();
    // objPlayer_n Begin Step advances activatey once, before gravity/motion.
    // Native camera offsets map to world view bottom 318 + cameraY.
    const auto activation = downwell::advanceMainRoomActivationChecked(activationY_,
        318L * 256L + viewCameraYQ8(), enemyCount_,
        [this](unsigned i) {
            const auto& enemy = enemies_[i];
            if (enemy.active || !enemy.activationPending)
                return downwell::ActivationInstance{enemy.objectId, {}, enemy.active, enemy.activationPending};
            const auto bounds = downwell::contactWorldBounds({(enemy.x + 240L) * 256, (enemy.y + 176L) * 256,
                enemy.objectId == 117 ? enemy.motionState : enemy.facing,
                enemy.imageScaleY(), downwell::enemyContactMask(enemy.objectId, enemy.spriteId,enemy.maskId)});
            return downwell::ActivationInstance{enemy.objectId, bounds,
                                               enemy.active, enemy.activationPending};
        },
        [this](unsigned i) {
            if (!publishEnemyActivation(enemies_[i],true)) return false;
            enemies_[i].active = true;
            enemies_[i].activatedTick = tick_;
            enemies_[i].activationPending = false;
            return true;
        }, true);
    if(!activation.succeeded){run_.effectsSucceeded=false;return {};}
    // scrPlayerShootN captures instance x/y before this Begin Step moves xx/yy.
    const int muzzleX = (int)downwell::roundGameMakerQ8(player_.offsetXFixed());
    const int muzzleY = (int)downwell::roundGameMakerQ8(player_.offsetYFixed()) + 4;
    struct HeadBreakContext {
        GameplayWorld* runtime;
    } headBreak = {this};
    player_.configureBreakBoxResolver([](void* opaque, long x, long y, bool& found) {
        auto& context = *static_cast<HeadBreakContext*>(opaque);
        auto& runtime = *context.runtime;
        downwell::PlayerBoxContact::Bounds bounds;
        if(!downwell::PlayerBoxContact::prepare(x,y,240,176,bounds))return false;
        for (unsigned i = 0; i < runtime.breakableCount_; ++i) {
            Breakable& box = runtime.breakables_[i];
            if(!box.active||box.destroyRequested)continue;
            bool hit=false;
            if(!downwell::PlayerBoxContact::testPrepared(bounds,box.x+240,box.y+176,hit))return false;
            if (hit) {
                box.state.wallHp={}; // scrWallCol leaves destruction to Step.
                runtime.observeBoxHealth(box);
                found=true;
                return true;
            }
        }
        found=false;return true;
    }, &headBreak);
    if(gunsActive_()){
        const auto& gun=currentGun();
        player_.configureGun(gun.consumption,gun.rof,gun.delayKill,gun.recoilFifths);
    }
    // scrPlayerShootN's emitSmoke reads global.plx/ply from before this Step.
    const unsigned emptyShotsBefore=player_.emptyShots(),emptyMessagesBefore=player_.emptyMessages();
    const unsigned landingsBefore=player_.landings();
    const int emptyX=240+int(downwell::roundGameMakerQ8(player_.offsetXFixed()));
    const int emptyY=176+int(downwell::roundGameMakerQ8(player_.offsetYFixed()));
    // scrPlayerInit reads pugRocket at each room's player Create; no grant
    // happens inside a room.
    player_.setRocketJump(run_.upgrades.levels[18]!=0);
    player_.setBallooning(balloonHeld());
    player_.setJetpack(run_.upgrades.levels[22]!=0);
    // objPlayer_n Begin Step: the jetpack loop follows last Step's jetpacking
    // (soundPlay(343, 80, 1, 1) once; stopped and forgotten otherwise).
    if(player_.jetpacking()){if(jetSound_<0)jetSound_=run_.soundPlay(343,80,true,true);}
    else if(jetSound_>=0){run_.stopSound(jetSound_);jetSound_=-1;}
    PlayerMotionDelta motion = downwell::stepGameplayPlayer(player_, death_, run_,
        currencyAccount_, health_, enemyRandom_, input, 240, 176,
        cameraMotion_.pixelX() - 80, cameraMotion_.pixelY() - 142,false,
        [this](int x,int y){return publishPlayerPosition(x,y);},
        [this](int value){
            if(!playerLifecycle_.integerTransform(playerHandle(),playerInstance_,downwell::RunnerTransformField::scaleX,
                    value,{0x3f800000u},0xffffffu))return false;
            playerInstance_.scaleX=value;return true;
        },[this](downwell::RunSession& run,downwell::MotionBinary64 x,downwell::MotionBinary64 y,int facing,bool& hit){
            if(!playerLifecycle_.hasPlaceMeeting())return run.playerGoalContact(x,y,facing,hit);
            return playerInstance_.lifetimeId&&playerLifecycle_.placeMeeting(playerHandle(),playerInstance_,255,
                downwell::MotionBinary64::fromInteger(playerInstance_.x),
                downwell::MotionBinary64::fromInteger(playerInstance_.y),hit);
        },[this](const PlayerMotion& motion,int x,int y){
            if(!playerTimeTransition_)return !compactTimeStop()||beginCompactTimeStop(x,y);
            return run_.effectsSucceeded&&
                playerTimeTransition_(playerTimeContext_,playerInstance_,motion,x,y)&&run_.effectsSucceeded;
        },[this](PlayerMotion& motion){
            return run_.playPlayerSounds(motion,enemyRandom_,death_.active,[this]{return playerFloorMaterial();},run_.timeStopped,
                [this](PlayerMotion& player){
                    return compactRocketJump(240+int(downwell::roundGameMakerQ8(player.rocketX())),
                        176+int(downwell::roundGameMakerQ8(player.rocketY())),
                        240+int(downwell::roundGameMakerQ8(player.frameStartXFixed())),
                        176+int(downwell::roundGameMakerQ8(player.frameStartYFixed())));
                });
        });
    player_.configureBreakBoxResolver(nullptr, nullptr);
    if(!run_.effectsSucceeded)return {};
    if(run_.shops.allocatedCount)createCompactShopSpeech();
    // levelBeginCue at an airborne landing precedes comboDone (scrWallCol).
    if(player_.landings()!=landingsBefore)compactLevelBeginCue();
    if(run_.combo.completion.rewardCreated&&!createCompactComboReward())return {};
    if(run_.combo.completion.momentDelay)compactMomentDelay(); // comboDone
    // objJumpFx 95 and objJumpSmallerFx 94: image_speed 0.5, destroyed at
    // Animation End, no RNG. The smaller puff draws itself and a mirrored
    // copy (62094) unless emitTo selects one side.
    for(unsigned i=0;i<player_.dustCount();++i){
        using Real=downwell::MotionBinary64;
        const auto& d=player_.dust(i);
        const Real x=Real::fromInteger(240+int(downwell::roundGameMakerQ8(d.x)));
        const Real y=Real::fromInteger(176+int(downwell::roundGameMakerQ8(d.y)));
        if(d.jump){if(!compactCosmeticEffect(x,y,95,{0x3fe00000u,0},0))return {};continue;}
        if(d.emitTo!=1&&!compactCosmeticEffect(x,y,94,{0x3fe00000u,0},0))return {};
        if(d.emitTo!=-1&&!compactCosmeticEffect(x,y,62094,{0x3fe00000u,0},0))return {};
    }
    // The first empty shot since landing: scrRisingText(plx, ply, "EMPTY!").
    // A full notice pool drops the cosmetic text.
    if(player_.emptyMessages()!=emptyMessagesBefore)
        if(auto* n=downwell::CompactGuns::append(guns_.notices,guns_.noticeEnd)){
            *n={};n->live=true;n->empty=true;n->x=emptyX;n->y=long(emptyY)<<16;n->birth=tick_;
        }
    // Empty charge: emitSmoke(plx, ply+4, 315, 4) and (225, 4). The puffs
    // follow the whole player Step here; the shot is its last RNG user.
    if(player_.emptyShots()!=emptyShotsBefore&&compactDamageEffectsAdmitted){
        using Q=downwell::Fixed16;
        const Q x=Q::fromInteger(emptyX),y=Q::fromInteger(emptyY+4);
        static const int directions[]={315,225};
        for(int direction:directions)
            if(!run_.smokeEffects.emitDirected(x,y,Q::fromInteger(direction),Q::fromInteger(4),enemyRandom_))
                {run_.effectsSucceeded=false;return {};}
    }
    if(player_.emptyShots()!=emptyShotsBefore)run_.soundPlay(26,90,false,true); // sfxGunEmpty
    if(stopped()!=stoppedBeforeStep){
        currencyAccount_.streak=streakBeforeStep;currencyAccount_.timer=streakTimerBeforeStep;
        currencyAccount_.beginStep(false,stopped());
    }
    if (player_.shots() != oldShots) firedThisStep_=true; // scrPlayerShootN: pFired = 1
    if (player_.shots() != oldShots && gunsActive_()) {
        playShotSound(ammoBeforeStep);
        if(!firePlayerGun(muzzleX,muzzleY-4,muzzleX,muzzleY))return {};
        // The shot that spends the last charge: soundPlay(26, 90, 0, 1).
        if(player_.ammo()==0)run_.soundPlay(26,90,false,true);
        bulletChanged=true;
    } else if (player_.shots() != oldShots) {
        playShotSound(ammoBeforeStep);
        const int muzzleOffset = emitter_.nextOffset();
        // Marked records remain resolvable until Draw cleanup; never compact
        // them away merely because gameplay/rendering has stopped using them.
        unsigned index=0;
        for(unsigned read=0;read<bulletSlotEnd_;++read)if(bullets_[read].identitySlot<bulletCapacity){
            if(index!=read)bullets_[index]=bullets_[read];++index;
        }
        bulletSlotEnd_ = index;
        if (index >= bulletCapacity || !nextBulletLifetimeId_) {
            run_.effectsSucceeded = false;
            return {};
        }
        BulletState& bullet = bullets_[index];
        unsigned identity=0;while(identity<bulletCapacity&&bulletIdentitySlots_[identity])++identity;
        if(identity==bulletCapacity){run_.effectsSucceeded=false;return {};}
        bullet={};bullet.identitySlot=identity;bulletIdentitySlots_[identity]=true;
        bullet.lifetimeId=nextBulletLifetimeId_++;
        bullet.active = true;
        bulletSlotEnd_ = index + 1;
        bullet.x = muzzleX + muzzleOffset;
        bullet.y = muzzleY;
        bullet.gemHigh = run_.gemHigh;
        bullet.damage = downwell::machineGunProfile(bullet.gemHigh).damage;
        bullet.sprite = downwell::machineGunProfile(bullet.gemHigh).sprite;
        bullet.animation = {};
        bullet.trajectory.reset(bullet.x, bullet.y, run_.upgrades.levels[12] != 0);
        const downwell::InstanceHandle handle{identity,bullet.lifetimeId};
        if(!bullet.sourceMotion.reset(downwell::MotionBinary64::fromInteger(bullet.x+240),
                downwell::MotionBinary64::fromInteger(bullet.y+176),bullet.trajectory.zeroSpread)||
            !bulletLifecycle_.birth(handle,bullet)||
            !bulletLifecycle_.sprite(handle,bullet,downwell::machineGunProfile(bullet.gemHigh).sprite)){
            run_.effectsSucceeded=false;return {};
        }
        bulletChanged = true;
    }

    // Alarms follow Begin Step movement, before builder Step/Create events.
    // Runner 0x50dc89 reconciles pending room membership after the complete
    // Begin Step dispatcher, not after each individual activation request.
    if(ditherAbove_&&!ditherAbove_->beginStepPublishedPixels(cameraMotion_.pixelX(),[&]{return cameraMotion_.pixelY();},
        ditherLowSpec_,ditherEnding_)){run_.effectsSucceeded=false;return {};}
    if(!reconcilePhase(PhaseBoundary::beginStep))return {};
    const auto damageNumberAlarmPhase=run_.damageNumbers.beginPhase();
    // Object 221 Alarm 0 only destroys itself: no RNG or cross-family effects.
    run_.advanceDamageNumberAlarm(damageNumberAlarmPhase,0);
    if(downwell::AlarmBank::advanceValue(cameraShakeAlarm_))cameraMotion_.stopShake();
    advanceEnemyAlarms();
    if(run_.effectsSucceeded&&gunsActive_()&&!advanceCompactGunAlarms())return {};
    if(drone_.live&&!advanceCompactDroneAlarm())return {};
    // fxRisingText 212 Alarms 0 and 1; no RNG, no cross-family effects.
    if(shopSpeechLive_)for(unsigned index=0;index<2;++index)
        for(auto& r:shopSpeech_)if(r.rise.live&&!r.rise.alarmEvent(index))--shopSpeechLive_;
    if(!reconcilePhase(PhaseBoundary::alarm))return {};
    currencyStepOrder_.clear();
    if(currencyInstances_&&currencyInstances_->size()&&(!currencyOrderCapture_||!currencyEventDispatch_||
        !currencyOrderCapture_(currencyOrderContext_,currencyStepOrder_))){run_.effectsSucceeded=false;return {};}
    const auto smokeStepPhase=run_.smokeEffects.beginPhase();
    const auto damageNumberStepPhase=run_.damageNumbers.beginPhase();
    // Original Step takes one creation cutoff before any object handler.
    // Boxes are stable slots with monotonic lifetimes, so a prefix and ceiling
    // exclude both generation births and replacements without a handle array.
    const unsigned boxStepEnd=breakableCount_;
    const std::uint32_t boxStepLastLifetime=nextBreakableLifetimeId_-1;
    const std::uint32_t bulletStepLastLifetime=nextBulletLifetimeId_-1;
    const std::uint32_t gemStepLastLifetime=nextGemLifetimeId_-1;
    const auto compactEffectStepPhase=compactEffects_.beginPhase();
    const auto compactDebrisStepPhase=compactDebris_.beginPhase();
    const auto shardStepPhase=timeStop_.shards.beginPhase();
    const auto corpseStepPhase=compactCorpses_.beginPhase();
    const std::uint32_t blastStepLastSerial=blastSerial_;
    if(!captureEnemyStepOrder())return {};
    if(playerChildrenStarted_&&!publishPlayerFallDepth())return {};
    if(skeletonBoneSteps_&&!skeletonBoneSteps_(skeletonBoneContext_,-1)){
        run_.effectsSucceeded=false;return {};
    }
    if(debrisStepDispatch_){
        debrisStepOrder_.clear();debrisStepPhase_=ownedDebrisEffects_.beginPhase();
        if(!debrisOrderCapture_(debrisOrderContext_,debrisStepOrder_)){run_.effectsSucceeded=false;return {};}
        debrisStepOrder_.sort();
    }
    // Capture before ordinary Step: later births must not inherit a turn.
    // This family's monotonic generation is its registration order, not its
    // numeric VM ID. Reused storage slots must not reorder two live covers.
    downwell::InstanceHandle entryStep[downwell::GameplayEntryDithers::capacity]{};
    unsigned entryStepCount=0;
    if(entryDithers_){
        if(!entryDithers_->valid()){run_.effectsSucceeded=false;return {};}
        for(unsigned i=0;i<downwell::GameplayEntryDithers::capacity;++i){
            const auto key=entryDithers_->handle(i);const auto* r=entryDithers_->find(key);
            if(r&&r->active&&!r->destroyRequested)entryStep[entryStepCount++]=key;
        }
        if(entryStepCount==2&&entryStep[0].generation>entryStep[1].generation){
            const auto first=entryStep[0];entryStep[0]=entryStep[1];entryStep[1]=first;
        }
    }
    // objPlayer_n Step (35) runs scrAimControl before camMain (64).
    if(gunsActive_()){
        const auto& gun=currentGun();
        if(input.left){aimAngle_-=gun.aimAcceleration;if(aimAngle_>0)aimAngle_-=gun.aimDeceleration;}
        if(input.right){aimAngle_+=gun.aimAcceleration;if(aimAngle_<0)aimAngle_+=gun.aimDeceleration;}
        if(!input.left&&!input.right){
            if(aimAngle_)aimAngle_-=aimAngle_>0?gun.aimDeceleration:-gun.aimDeceleration;
            if(aimAngle_>-2&&aimAngle_<2)aimAngle_=0;
        }
        if(aimAngle_>gun.aimLimit)aimAngle_=gun.aimLimit;
        if(aimAngle_<-gun.aimLimit)aimAngle_=-gun.aimLimit;
    }
    DW_PROFILE_PHASE(profilePlayer);
    int scrollDelta = 0;
    int cameraXDelta = 0;
    if (followCamera_) {
        const int oldCameraSteadyX = cameraSteadyX_;
        DW_PROFILE_SCOPE(profileCamera);
        const int oldCameraPixelY = cameraPixelY_;
        cameraMotion_.step(240L * 256 + player_.offsetXFixed(),
                           176L * 256 + player_.offsetYFixed(), dead(),
                           run_.sideCameraLockers.overlaps(cameraMotion_));
        cameraMotion_.sampleShake(enemyRandom_);
        cameraPixelX_ = cameraMotion_.pixelX() - 240;
        // The frame reports the steady view: a shake moves only the display.
        cameraSteadyX_ = cameraMotion_.steadyPixelX() - 240;
        cameraXDelta = cameraSteadyX_ - oldCameraSteadyX;
        cameraY_ = (cameraMotion_.y - 176L * 4096) / 16;
        cameraPixelY_ = cameraMotion_.pixelY() - 176;
        scrollDelta = cameraPixelY_ - oldCameraPixelY;
        motion.deltaY -= scrollDelta;
    } else if ((tick_ & 1u) == 0) {
        scrollDelta = 1;
    }
    if (scrollDelta != 0) {
        int origin = (int)ringOrigin_ + scrollDelta;
        origin %= (int)ringHeight;
        if (origin < 0) origin += (int)ringHeight;
        ringOrigin_ = (unsigned)origin;
    }

    // Step's object table is ascending: camMain (64), objBuilder (65),
    // objEnterDither (73). All family cutoffs above precede the builder's births.
    if (generationStep_ && generationSucceeded_) {
        DW_PROFILE_SCOPE(profileGeneration);
        const long bottom = generationCameraY + 318L * 256L;
        const int bottomCeiling = bottom >= 0 ? (int)((bottom + 255) / 256)
                                              : -(int)((-bottom) / 256);
        const auto generate=[&]{return generationStep_(generationContext_, *this,
                                                      enemyRandom_, bottomCeiling);};
        generationSucceeded_ = builderController_?builderController_->withGeneration(generate):generate();
        if (!generationSucceeded_) return {};
    }

    // camMain publishes round(xxFinal) to x before this VM read, not its
    // fractional Q12 easing accumulator (nor the view's left edge). Full source camera
    // arithmetic and source-ordered native controller births remain separate.
    for(unsigned i=0;i<entryStepCount;++i){
        const auto key=entryStep[i];
        if(!entryDithers_->find(key))continue;
        if(!entryDithers_->step(key,[&](downwell::RunnerImageValue& x){
            return x.assignInteger(cameraMotion_.pixelX());
        })){run_.effectsSucceeded=false;return {};}
    }
    if(compactEntryLive_&&!compactEntry_.step([&](downwell::RunnerImageValue& x){
        return x.assignInteger(cameraMotion_.pixelX());
    })){run_.effectsSucceeded=false;return {};}

    // Spawner 78 follows builder/entry work and precedes breakable Step.
    if(compactSpawnerLive_&&!stepCompactSpawner(generationCameraY))return {};
    if(skeletonBoneSteps_&&!skeletonBoneSteps_(skeletonBoneContext_,92)){
        run_.effectsSucceeded=false;return {};
    }
    // objBox_n (92) observes health after player Begin Step and before bullet
    // Step (261). A bound source adapter also runs healthy boxes' alarmStop.
    // The unbound path retains the old destruction boundary until all effect,
    // audio and time-stop owners can be supplied by the gameplay scene.
    // Healthy boxes' Step is alarmStop(0): it only matters while stopped.
    if(stopped()&&!boxStepDispatch_.bound()&&!holdStoppedBoxAlarms())return {};
    unsigned boxStepLimit=boxStepEnd;
#ifndef DOWNWELL_REFERENCE_ENEMY_UPDATES
    // Unbound, the Step only destroys exhausted boxes; none has been yet.
    if(!boxStepDispatch_.bound()&&!boxesMayBeExhausted_)boxStepLimit=0;
#endif
    for(unsigned i=0;i<boxStepLimit&&i<breakableCount_;++i){
        auto& box=breakables_[i];
        if(!box.lifetimeId||box.lifetimeId>boxStepLastLifetime||!box.active||box.destroyRequested)continue;
        if(boxStepDispatch_.bound()){
            const downwell::InstanceHandle key{i,box.lifetimeId};
            const bool ok=boxStepDispatch_.step(key);
            const auto* retained=findBreakable(key);
            if(retained&&retained->destroyRequested){
                ++blocksDestroyed_;
                if(destroyedBlockCount==destroyedCapacity){run_.effectsSucceeded=false;return {};}
                destroyedBlockX[destroyedBlockCount]=retained->x;
                destroyedBlockY[destroyedBlockCount++]=retained->y;
            }
            if(!ok||!run_.effectsSucceeded){run_.effectsSucceeded=false;return {};}
            continue;
        }
        if(!box.state.exhausted())continue;
        if(destroyedBlockCount==destroyedCapacity){run_.effectsSucceeded=false;return {};}
        const int boxX=box.x,boxY=box.y;
        // Healthy boxes only ran alarmStop(0) above.
        if(compactRoomEffectsAdmitted){if(!stepCompactBox(i))return {};}
        else if(!markBreakableDestroyed({i,box.lifetimeId}))return {};
        destroyedBlockX[destroyedBlockCount]=boxX;
        destroyedBlockY[destroyedBlockCount++]=boxY;
        ++blocksDestroyed_;
    }

    // GunModule (109) follows the boxes, before the enemies (111-119).
    if(guns_.moduleEnd&&!stepGunModules())return {};
    // Existing native families follow their source object table order. This
    // does not admit the omitted actor/effect handlers or replace legacy
    // bullet contacts with the original Collision event pass.
    DW_PROFILE_PHASE(profileEnemyMotion);
    for (unsigned i=0;i<enemyStepOrder_.size();++i){
        if(skeletonBoneSteps_&&!skeletonBoneSteps_(skeletonBoneContext_,enemyStepOrder_.object(i))){
            run_.effectsSucceeded=false;return {};
        }
        if(!stepEnemyRoomEffect(enemyStepOrder_.handle(i),publishedCameraX,generationCameraY))return {};
    }
    bumpCompactFrogs();
    // parentMovingFx (220) follows the enemies, before damage numbers (221).
    if(compactEffects_.movingCount()&&!compactEffects_.stepMoving(compactEffectStepPhase,
        [this](const CompactEffects::Effect& fx,bool& moving){
            moving=!stopped()||timeStopBound(fx.spriteId,fx.x,fx.y);return true;
        })){
        run_.effectsSucceeded=false;return {};
    }

    // Flush remaining registered enemies and blood before damage numbers.
    if(skeletonBoneSteps_&&!skeletonBoneSteps_(skeletonBoneContext_,221)){
        run_.effectsSucceeded=false;return {};
    }

    DW_PROFILE_PHASE(profileEnemyContact);
    run_.stepDamageNumbers(damageNumberStepPhase);
    // fxBlood 223 follows the damage numbers (221).
    if(bloodEnd_&&!stepCompactBlood())return {};
    if(skeletonBoneSteps_&&!skeletonBoneSteps_(skeletonBoneContext_,226)){
        run_.effectsSucceeded=false;return {};
    }
    run_.stepSmoke(smokeStepPhase,enemyRandom_);
    if(!run_.effectsSucceeded)return {};
    if(timeStop_.shards.size()&&!stepTimeShards(shardStepPhase))return {};
    // Time shards (238) follow all smoke (225), before currency (248).
    if(skeletonBoneSteps_&&!skeletonBoneSteps_(skeletonBoneContext_,248)){
        run_.effectsSucceeded=false;return {};
    }
    for(unsigned i=0;i<currencyStepOrder_.size();++i){
        const auto key=currencyStepOrder_.handle(i);const auto* r=currencyInstances_->find(key);
        if(!r||!r->active||r->destroyRequested)continue;
        bool eligible=false;
        if(!currencyStepEligible_||!currencyStepEligible_(currencyOrderContext_,key,eligible)){
            run_.effectsSucceeded=false;return {};
        }
        if(!eligible)continue;
        if(!currencyEventDispatch_||!currencyEventDispatch_(currencyEventContext_,key,CurrencyPhase::step,0)){
            run_.effectsSucceeded=false;return {};
        }
    }
    DW_PROFILE_PHASE(profileGems);
    for (unsigned gemIndex = 0; gemIndex < gemSlotEnd_; ++gemIndex) {
        GemState& gem = gems_[gemIndex];
        if (!gem.active||gem.lifetimeId>gemStepLastLifetime)
            continue;
        // objCurrency's stopped branch: image_speed 0, alarms held and
        // obtainable set; the dflash toggle still runs. Attracted gems home.
        gem.frozen=!gem.attracted&&!timeStopBound(gem.large?134:133,gem.x+240,gem.y+176);
        if(gem.frozen){
            gem.motion.lifetime.freeze();
            if(run_.collectFrozenCurrency(gem.motion,gem.large,static_cast<unsigned>(gem.amount),player_,currencyAccount_)
               ==downwell::CurrencyStepResult::collected)gem.active=false;
            continue;
        }
        const auto result = run_.collectCurrency(gem.motion, gem.large, static_cast<unsigned>(gem.amount),
            player_, currencyAccount_,
            [this](int x, int y) { return solidResolver_ && solidResolver_(solidResolverContext_, x, y); }, enemyRandom_,
            boneViewTopQ8_-176L*256);
        if (result != downwell::CurrencyStepResult::alive) {
            gem.active = false;
            continue;
        }
        gem.age = gem.motion.lifetime.age;
        gem.preciseX = downwell::GemMotion::q8(gem.motion.x);
        gem.preciseY = downwell::GemMotion::q8(gem.motion.y);
        gem.speedX = downwell::GemMotion::q8(gem.motion.vx);
        gem.speedY = downwell::GemMotion::q8(gem.motion.vy);
        gem.attracted = gem.motion.attracted;
        gem.x = downwell::GemMotion::rounded(gem.motion.x);
        gem.y = downwell::GemMotion::rounded(gem.motion.y);
    }

    // objDrone 249 follows currency (248).
    stepCompactDrone();
    // decoyBalloon 256 follows the Drone, before the usual bullet (261).
    if(balloon_.live&&!stepCompactBalloon())return {};
    // A shot born in player Begin Step is eligible here; one born during this
    // ordinary Step is not. Currency (248) precedes the usual bullet (261).
    {
        DW_PROFILE_PHASE(profileBullets);
        for(unsigned bulletIndex=0;bulletIndex<bulletSlotEnd_;++bulletIndex)
            if(bullets_[bulletIndex].lifetimeId<=bulletStepLastLifetime&&
                !stepBulletEffect(bullets_[bulletIndex],generationCameraY,bulletChanged))return {};
    }

    // bulletCasing (267) follows the bullets (260/261).
    if(guns_.casingEnd&&!stepCasings())return {};
    DW_PROFILE_PHASE(profileLifecycle);
    // Corpse/bone/blast (263/269/276) follow usual bullets, before debris 370.
    if(skeletonBoneSteps_&&!skeletonBoneSteps_(skeletonBoneContext_,370)){
        run_.effectsSucceeded=false;return {};
    }
    // Compact objDeadBody 263, then Scattered16/gemChunk/TorchFire/Torch
    // (289-296), then compact debris 370.
    if(compactCorpses_.size()&&!stepCompactCorpses(corpseStepPhase))return {};
    // bulletExplosion1 272, explosionSmall 274 and bulletBlast 276.
    if(blastEnd_&&!stepCompactBlasts(blastStepLastSerial))return {};
    if(!scenery_.empty()&&!stepCompactScenery())return {};
    if(compactDebris_.size()&&!compactDebris_.step(compactDebrisStepPhase,enemyRandom_,
        [this](auto,const downwell::DebrisParticle& d,bool& frozen){ // !TimeStopBound, mask 598
            if(!stopped()){frozen=false;return true;}
            frozen=!timeStopBound(598,d.pixelX(),d.pixelY());return true;
        },
        [this](auto,const downwell::DebrisParticle&,std::int32_t x,std::int32_t y,bool& hit){
            downwell::ContactBounds bounds;
            if(!run_.effectsSucceeded||!downwell::CurrencyCollisionBounds::calculateMaskQ16(598,x,y,bounds))
                return run_.effectsSucceeded=false;
            return solidContact(bounds,true,hit);
        })){run_.effectsSucceeded=false;return {};}
    if(debrisStepDispatch_)for(unsigned i=0;i<debrisStepOrder_.size();++i){
        const auto key=debrisStepOrder_.handle(i);
        const auto* record=debrisInstances_?debrisInstances_->find(key):nullptr;
        if(!record||!record->active||record->destroyRequested)continue;
        if(!debrisStepDispatch_(debrisStepContext_,key)){run_.effectsSucceeded=false;return {};}
    }
    // moduleText (373) and itemNotif (374) follow debris (370).
    if((guns_.textEnd||guns_.noticeEnd||shopSpeechLive_)&&!stepModuleTexts())return {};
    if(skeletonBoneSteps_&&!skeletonBoneSteps_(skeletonBoneContext_,0x7fffffff)){
        run_.effectsSucceeded=false;return {};
    }
    // Only discard inactive tails; holes stay in creation order until the
    // existing stable compaction at the next birth. Never reorder live slots.
    while (bulletSlotEnd_ && bullets_[bulletSlotEnd_-1].identitySlot>=bulletCapacity) --bulletSlotEnd_;
    while (gemSlotEnd_ && !gems_[gemSlotEnd_-1].active) --gemSlotEnd_;
    run_.exitDither.step(34 + downwell::floorQ8Coordinate(generationCameraY), 284);
    // Moving the followed instance does not dirty the camera cache. End Step
    // still reads the previous Draw's view, including after a Collision snap.
    const long viewTop = 34L * 4096 + generationCameraY * 16;
    if(!reconcilePhase(PhaseBoundary::step))return {};
    // Collision work follows the complete ordinary Step and membership flush.
    // These retained adapters still need replacement by the mixed source pass.
    run_.collideSmokeWalls([&](const downwell::ContactBounds& bounds,auto emit){
        unsigned count=0;
        if(wallContactResolver_ && !wallContactResolver_(solidResolverContext_,bounds,~0u,count))return false;
        for(unsigned i=0;i<count;++i)if(!emit())return false;
        return true;
    });
    if(!run_.effectsSucceeded)return {};
    // GunModule's Collision with objPlayer_n (object 109 before the enemies).
    if(guns_.moduleEnd&&!collideGunModules())return {};
    if (!downwell::stepEnemyContacts(enemyCount_, player_, health_, invulnerability_,
        run_.kills, run_.combo.state, run_.upgrades.levels[7] != 0,
        [this](unsigned i) -> EnemyState& { return enemies_[i]; },
        [](const EnemyState& enemy) {
            return downwell::EnemyContactLocation{{
            enemy.preciseX, enemy.preciseY,
            enemy.objectId == 117 ? enemy.motionState : enemy.facing,
            enemy.imageScaleY(),
            downwell::enemyContactMask(enemy.objectId, enemy.spriteId,enemy.maskId)}, enemy.x, enemy.y};
        },
        [this](EnemyState& enemy) {
            // scrEnemyStomped's soundPlayOL(122, 50, 0, 1, "footsteps")
            // precedes the death the compact world runs here.
            run_.soundPlayOL(122,50,false,true,"footsteps");
            return compactEnemyDeath(enemy, true);
        },
        [](downwell::EnemyContact) {},
        [this](int x,int y) {
            compactDamageSlowdown();
            return run_.contactDamageNumber(player_.frameStartXFixed(),player_.frameStartYFixed(),
                x,y,240,176)&&playPlayerHurt()&&compactTimeoutField();
        },[this](const EnemyState& enemy){return publishEnemyActivation(enemy,false);})) return {};
    if (steeringPopulation_ && !collideSteeringEnemies()) return {};
    run_.startRechargeSounds(player_); // stomps and contact knockback (scrFjump)
    // subparentEnemy's Collision with parentTimeField, then fxTimeShard 238.
    if (compactTimeStop() && (run_.timeFields.size() || run_.levelExitPresent || run_.startFieldPresent) &&
        !collideEnemyTimeFields()) return {};
    if (timeStop_.shards.size() && !collideTimeShards()) return {};
    // goalTimeField 255 (with its child startTimeField) precedes scenery 289.
    if (run_.goalWindPossible() &&
        !run_.collideGoalFieldWind(enemyRandom_,240+int(downwell::roundGameMakerQ8(player_.offsetXFixed())),
            176+int(downwell::roundGameMakerQ8(player_.offsetYFixed())),player_.facing())) return {};
    // Blasts and explosions (272-276) precede scenery 289.
    // Collision events in object order: fxBlood 223, decoyBalloon 256,
    // objDeadBody 263, the blasts 272-276, then the scenery.
    if (bloodEnd_ && !collideCompactBlood()) return {};
    if (balloon_.live && !collideCompactBalloon()) return {}; // decoyBalloon 256
    if (!dead() && !eatCompactCorpses()) return {};
    if (run_.upgrades.levels[8] && guns_.casingEnd && !collideHotCasings()) return {};
    if (blastEnd_ && !collideCompactBlasts()) return {};
    if (!scenery_.empty() && !collideCompactScenery()) return {};
    run_.collideLevelExit(240 + downwell::roundGameMakerQ8(player_.offsetXFixed()),
        176 + downwell::roundGameMakerQ8(player_.offsetYFixed()), death_.active);
    if(collisionPass_){
        collisionPassExecuting_=true;
        const bool passed=collisionPass_(collisionPassContext_);
        collisionPassExecuting_=false;
        if(!passed||!run_.effectsSucceeded){run_.effectsSucceeded=false;return {};}
    }
    if (followCamera_ && run_.sideCameraLockers.count) {
        run_.sideCameraLockers.collide(cameraMotion_);
        const int collisionScroll = cameraMotion_.pixelY() - 176 - cameraPixelY_;
        cameraY_ = (cameraMotion_.y - 176L * 4096) / 16;
        cameraPixelY_ += collisionScroll;
        scrollDelta += collisionScroll;
        motion.deltaY -= collisionScroll;
        if (collisionScroll) {
            int origin = (int(ringOrigin_) + collisionScroll) % int(ringHeight);
            if (origin < 0) origin += int(ringHeight);
            ringOrigin_ = unsigned(origin);
        }
    }
    if(!reconcilePhase(PhaseBoundary::collision))return {};
    if(!registeredOrdinaryEndStep_){
        unsigned retiredEnemies = 0;
        const bool enemiesRetired=downwell::retireMainRoomObjectsChecked(enemyCount_, 11, 1, viewTop,
            [this, &retiredEnemies](unsigned i) {
                const auto& enemy = enemies_[i - retiredEnemies];
                return downwell::LifetimeInstance{enemy.objectId, enemy.y + 176, enemy.active};
            },
            [this, &retiredEnemies](unsigned i) {
                const unsigned index = i - retiredEnemies;
                if(!retireEnemyOwnership(enemies_[index]))return false;
                for (unsigned next = index + 1; next < enemyCount_; ++next){
                    enemies_[next - 1] = enemies_[next];
                    if(enemies_[next-1].identitySlot<enemyCapacity)
                        enemySlotIndex_[enemies_[next-1].identitySlot]=static_cast<unsigned short>(next);
                }
                --enemyCount_;
                enemies_[enemyCount_].identitySlot=0xffffu;
                ++retiredEnemies;
                return true;
            },downwell::MainRoomLifetimePhase::endStep,true);
        if(!enemiesRetired){run_.effectsSucceeded=false;return {};}
        const bool boxesRetired=downwell::retireMainRoomObjectsChecked(breakableCount_, 11, 1, viewTop,
            [this](unsigned i) {
                const auto& box = breakables_[i];
                return downwell::LifetimeInstance{92, box.y + 176, box.active&&!box.destroyRequested};
            },
            [this](unsigned i) { return retireBreakable(i); });
        if(!boxesRetired){run_.effectsSucceeded=false;return {};}
    }
    if(registeredActorEndStep_&&!registeredActorEndStep_(registeredActorEndStepContext_,viewTop)){run_.effectsSucceeded=false;return {};}
    if(!guns_.empty()&&(!retireCompactGuns(viewTop)||!drawCompactGunTexts()))return {};
    if(compactCorpses_.size()&&!endStepCompactCorpses(viewTop))return {};

#if !defined(DOWNWELL_PLATFORM_AMIGA) || defined(DOWNWELL_REPLAY_VERIFY) || defined(DOWNWELL_SCENE_VERIFY)
    const bool firstBulletActive = bulletActive();
    checkpoint_ = downwell::portCheckpointDigest(
        tick_, packedInput, player_, ringOrigin_, firstBulletActive,
        bulletX(), bulletY());
    if (run_.sideCameraLockers.count || cameraMotion_.sideLocked) {
        downwell::StateDigest extended;
        extended.addU64(checkpoint_);
        extended.addU32(0x534c4f43);
        extended.addU64(sideCameraDigest());
        checkpoint_ = extended.value();
    }
    if(cameraMotion_.shaking||cameraMotion_.shakeX||cameraMotion_.shakeY||cameraShakeAlarm_>=0){
        downwell::StateDigest extended;
        extended.addU64(checkpoint_);extended.addU32(0x5348414b);
        extended.addBool(cameraMotion_.shaking);extended.addI32(cameraMotion_.shakeAmount);
        extended.addI32(cameraMotion_.shakeX);extended.addI32(cameraMotion_.shakeY);
        extended.addI32(cameraShakeAlarm_);checkpoint_=extended.value();
    }
    trajectory_.addU64(checkpoint_);
#endif

        run_.shops.endStep(34 + downwell::floorQ8Coordinate(generationCameraY));
    GameplayFrame frame = {};
    // parentTimeField/goalTimeField/startTimeField Draw: no radius draw
    // while pTimeStop.
    run_.gameplayPresentationStep(enemyRandom_,stopped());
    drawCompactAfterImages();
    if(!reconcilePhase(PhaseBoundary::endStep))return {};
    frame.playerMotion = motion;
    DW_PROFILE_PHASE(profileHudState);
    const bool ammoMotionChanged = run_.ammoHud.step(player_.shots(), player_.recharges());
    run_.ammoGauge.step(player_.ammo(), player_.ammoCapacity());
    frame.ammoChanged = player_.ammo() != oldAmmo || player_.ammoCapacity() != oldAmmoCapacity || ammoMotionChanged || run_.ammoGauge.visuallyChanged();
    frame.healthChanged = health_ != oldHealth || healthMaximum() != oldHealthMaximum ||
                          healthReserve().pieces != oldHeartPieces;
    frame.currencyChanged = currencyAccount_.currency != oldCurrency;
    frame.streakChanged = currencyAccount_.streak != oldGemStreak;
    frame.bulletChanged = bulletChanged;
    frame.scrolled = scrollDelta != 0;
    frame.scrollDelta = scrollDelta;
    frame.cameraXChanged = cameraXDelta != 0;
    frame.cameraXDelta = cameraXDelta;
    // Scenery art changes reuse the 16x16 repaint list after box removals.
    for(unsigned i=0;i<sceneryRepaintCount_;++i){
        if(destroyedBlockCount==destroyedCapacity){run_.effectsSucceeded=false;return {};}
        destroyedBlockX[destroyedBlockCount]=sceneryRepaintX_[i];
        destroyedBlockY[destroyedBlockCount++]=sceneryRepaintY_[i];
    }
    sceneryRepaintCount_=0;
    frame.destroyedBlockCount = destroyedBlockCount;
    for (unsigned index = 0; index < destroyedBlockCount; ++index) {
        frame.blockX[index] = destroyedBlockX[index];
        frame.blockY[index] = destroyedBlockY[index];
    }
    DW_PROFILE_PHASE(profileSimulation);
    return frame;
}

// ---- Compact spawner 78 and steering families 115/118/173 ---------------

namespace {
using SteeringReal=downwell::MotionBinary64;
SteeringReal exactQ8(long q8){
    auto value=SteeringReal::fromInteger(static_cast<std::int32_t>(q8));
    if(!value.zero())value.high-=8u<<20; // Exact division by 256.
    return value;
}
bool steeringObject(int object){return object==115||object==118||object==173;}
}

struct GameplayWorld::SteeringOps {
    using Image=downwell::RunnerImageValue;
    GameplayWorld& world;
    EnemyState* enemy; // Null during Create: the record is configured after it.
    downwell::SteeringFixedState& actor;
    int alarm0=-1,alarm1=-1;
    // plx/ply in Q16: the frame-start Q8 position plus the room offset.
    bool playerQ16(bool vertical,std::int32_t& value){
        const long q8=vertical?world.player_.frameStartYFixed():world.player_.frameStartXFixed();
        if(q8<-30000L*256||q8>30000L*256)return false;
        value=std::int32_t(q8)*256+(vertical?176:240)*65536;return true;
    }
    // eplx/eply alias plx/ply unless pugDecoy has a held balloon.
    bool enemyPlayerQ16(bool vertical,std::int32_t& value){
        if(world.balloonHeld()){
            const int pixel=vertical?world.balloon_.y:world.balloon_.x;
            if(pixel<-32000||pixel>32000)return false;
            value=pixel*65536;return true;
        }
        return playerQ16(vertical,value);
    }
    bool position(bool,Image value,Image& accepted){
        if(!downwell::RunnerSpatialSplit::valid(value))return false;
        accepted=value;return true;
    }
    bool imageSpeed(Image value,Image& accepted){
        if((value.bits&0x7f800000u)==0x7f800000u)return false; // Not finite.
        accepted=value;return true;
    }
    bool alarm(int number,int ticks){
        if(number==2)return true; // Neither family has an Alarm 2 event.
        if(number==0)(enemy?enemy->alarm0:alarm0)=ticks;
        else if(number==1)(enemy?enemy->alarm1:alarm1)=ticks;
        else return false;
        return true;
    }
    bool imageIndex(int value){
        if(!enemy||value)return false;
        enemy->imagePhase=0;enemy->imageRemainder=0;
        return world.publishEnemyImage(*enemy,0);
    }
    bool sound(int id,int priority,int loop,int cut,const char* channel){
        if(!enemy)return false;
        world.soundPlayOLAt(id,priority,loop!=0,cut!=0,channel,enemy->y+176);
        return world.run_.effectsSucceeded;
    }
    bool meetingQ16(std::int32_t x,std::int32_t y,int object,bool& hit){
        downwell::ContactBounds bounds;
        if(!enemy||object!=57||!downwell::CurrencyCollisionBounds::calculateMaskQ16(
            downwell::enemyContactMask(enemy->objectId,enemy->spriteId),x,y,bounds))
            return world.run_.effectsSucceeded=false;
        return world.steeringWallContact(bounds,hit);
    }
    bool enemyDeath(){ // scrEnemyDeath, gunshotStop included
        world.run_.combo.state.enemyDefeated();world.run_.stopSound(world.run_.overlapSounds[1]);
        ++world.run_.kills;world.compactLevelBeginCue();return true;
    }
    // scrSmoke(x,y,1,0) and scrFlashballfx(x,y,1,0,0) create objects with
    // Create RNG; emitMovingFx 704 has none.
    bool smokeQ16(downwell::Fixed16 x,downwell::Fixed16 y){return world.emitBoxSmoke(x,y);}
    bool flashQ16(downwell::Fixed16 x,downwell::Fixed16 y){return world.run_.emitFlash<1,0,0>(x,y,world.enemyRandom_);}
    bool movingFxQ16(downwell::Fixed16 x,downwell::Fixed16 y,int sprite,Image rate,downwell::Fixed16 angle,downwell::Fixed16 speed){
        return world.compactEffects_.spawnMoving(x,y,sprite,rate,angle,speed)||(world.run_.effectsSucceeded=false);
    }
    bool currency(int money){ // scrCurrencySpawn: atrState follows stomped.
        if(!enemy)return false;
        enemy->reward=money;world.spawnReward(*enemy,actor.stomped);
        return world.run_.effectsSucceeded;
    }
    // Step-time instance_destroy: like the other compact families' deaths,
    // retire the record from every room pass by deactivating it. The owner
    // accepts destruction marks only at its End Step boundary.
    bool destroy(){
        if(!enemy||!world.publishEnemyActivation(*enemy,false))return false;
        enemy->active=false;enemy->activationPending=false;return true;
    }
    // The sprite assignment after instance_destroy has no observer left.
    bool sprite(int value){return enemy&&(!enemy->active||world.publishEnemySprite(*enemy,value));}
    bool xscale(int sign){
        if(!enemy||!world.publishEnemyTransform(*enemy,downwell::RunnerTransformField::scaleX,sign))return false;
        enemy->facing=sign;return true;
    }
    bool inView(bool& visible){ // scrInView(0,0,0): strict, builtin x/y.
        const int left=160+world.cameraOffsetX(),top=34+world.cameraOffsetY();
        visible=actor.pixelX>left&&actor.pixelX<left+160&&actor.pixelY>top&&actor.pixelY<top+284;
        return true;
    }
    bool effectAlarmReset(int ticks){
        if(!enemy)return false;
        enemy->alarm0=ticks;return true;
    }
};

void GameplayWorld::resetCompactSpawning(){
    for(unsigned i=0;i<steeringCapacity;++i)steeringOwner_[i]=0;
    steeringPopulation_=0;compactSpawnerLive_=false;compactSpawnerMask_=-1;generationEndReached_=false;
}

downwell::SteeringFixedState* GameplayWorld::steeringActor(const EnemyState& enemy){
    const int slot=enemy.auxiliaryState;
    if(!steeringObject(enemy.objectId)||slot<0||unsigned(slot)>=steeringCapacity||
       !enemy.lifetimeId||steeringOwner_[slot]!=enemy.lifetimeId)return nullptr;
    if(enemy.objectId==115)return &steering_[slot].jelly;
    return &steering_[slot].ball;
}

void GameplayWorld::releaseSteering(const EnemyState& enemy){
    if(steeringActor(enemy)){steeringOwner_[enemy.auxiliaryState]=0;--steeringPopulation_;}
}

bool GameplayWorld::redBallEye(const EnemyState& enemy,int& sprite,int& mirror)const{
    auto* actor=const_cast<GameplayWorld*>(this)->steeringActor(enemy);
    if(!actor||(enemy.objectId!=118&&enemy.objectId!=173))return false;
    const auto& ball=steering_[enemy.auxiliaryState].ball;
    if(!ball.eyeInitialized)return false;
    sprite=ball.eye;mirror=ball.eyeX;return true;
}

bool GameplayWorld::createCompactSpawner(int y){
    // scrBuilder's spawner: spawnerSet1, or hardSet1 in hard mode.
    if(compactSpawnerLive_||run_.stage.area!=1||
       !compactSpawner_.createCaverns(run_.stage.level,run_.hardMode))return run_.effectsSucceeded=false;
    compactSpawnerY_=downwell::MotionBinary64::fromInteger(y);
    compactSpawnerMask_=-1;compactSpawnerLive_=true;
    return true;
}

bool GameplayWorld::steeringWallMeeting(int mask,downwell::MotionBinary64 x,downwell::MotionBinary64 y,bool& hit){
    downwell::ContactBounds bounds;
    if(!downwell::CurrencyCollisionBounds::calculateMask(mask,x,y,bounds))return run_.effectsSucceeded=false;
    return steeringWallContact(bounds,hit);
}

bool GameplayWorld::steeringWallContact(const downwell::ContactBounds& bounds,bool& hit){
    // parentWall 57: walls and boxes (resolver) plus the compact shop walls.
    unsigned count=0;
    if(wallContactResolver_){
        if(!wallContactResolver_(solidResolverContext_,bounds,1,count))return run_.effectsSucceeded=false;
    }else if(solidResolver_){
        // Worlds without the instance resolver: grid walls and boxes answer
        // every covered pixel, in room offsets, as the bullet fallback does.
        for(int py=bounds.top;py<bounds.bottom&&!count;++py)
            for(int px=bounds.left;px<bounds.right&&!count;++px)
                if(solidResolver_(solidResolverContext_,px-240,py-176))count=1;
    }else return run_.effectsSucceeded=false;
    bool found=count!=0;
    if(!found&&!run_.shops.visitWallContacts(bounds,[&](){found=true;return true;}))return run_.effectsSucceeded=false;
    hit=found;return true;
}

bool GameplayWorld::steeringEnemyMeeting(int mask,downwell::MotionBinary64 x,downwell::MotionBinary64 y,bool& hit){
    // subparentEnemy 83: every compact family. Deactivated records are absent.
    downwell::ContactBounds bounds;
    if(!downwell::CurrencyCollisionBounds::calculateMask(mask,x,y,bounds))return run_.effectsSucceeded=false;
    hit=false;
    for(unsigned i=0;i<enemyCount_&&!hit;++i){
        const auto& enemy=enemies_[i];
        if(!enemy.active||enemy.destroyRequested)continue;
        hit=downwell::contactBoundsOverlap(bounds,downwell::contactWorldBounds({(enemy.x+240L)*256,(enemy.y+176L)*256,
            enemy.objectId==117?enemy.motionState:enemy.facing,enemy.imageScaleY(),
            downwell::enemyContactMask(enemy.objectId,enemy.spriteId,enemy.maskId)}));
    }
    return true;
}

struct GameplayWorld::SpawnerOps {
    using Real=downwell::MotionBinary64;
    GameplayWorld& world;long cameraQ8;
    bool playerY(Real& value){
        Real x;return downwell::ContactCoordinates::playerGlobals(world.player_.frameStartXFixed(),
            world.player_.frameStartYFixed(),240,176,x,value);
    }
    bool area(int& value){value=world.run_.stage.area;return true;}
    bool fallDepth(Real& value){
        if(world.playerChildrenStarted_)return world.sourceFallDepth(value);
        // global.falldepth = yy - objDepthMeter.y; the meter keeps the birth
        // y. yy is the Q8 player offset plus 176, so the difference is a Q8
        // integer and its binary64 is exact: fromInteger, then /256.
        const std::int32_t offset=world.player_.offsetYFixed(),meter=176-world.depthMeterY_;
        if(offset< -0x20000000||offset>0x20000000||meter< -0x100000||meter>0x100000)return false;
        const std::int32_t q8=offset+meter*256;
        value=Real::fromInteger(q8);
        if(q8)value.high-=8u<<20;
        return true;
    }
    bool death(bool& value){value=world.dead();return true;}
    bool endReached(bool& value){value=world.generationEndReached_;return true;}
    bool enemyCount(int& value){
        value=0;
        for(unsigned i=0;i<world.enemyCount_;++i)
            if(world.enemies_[i].active&&!world.enemies_[i].destroyRequested)++value;
        return true;
    }
    bool objectMask(int object,int& value){
        switch(object){
            case 114:value=278;return true; // enmEyefish's OBJT mask
            case 115:value=174;return true;
            case 118:case 173:value=153;return true;
            case 116:value=-1;return true; // enmBat has no explicit mask.
            default:return false;
        }
    }
    bool mask(int value){world.compactSpawnerMask_=value;return true;}
    bool meeting(Real x,Real y,int object,bool& hit){
        const int mask=world.compactSpawnerMask_<0?702:world.compactSpawnerMask_;
        if(object==57)return world.steeringWallMeeting(mask,x,y,hit);
        if(object==83)return world.steeringEnemyMeeting(mask,x,y,hit);
        return false;
    }
    // Compact walls are never replaced by depth-100 tiles: where the original
    // finds a tile, this world still has the wall, which obstructs the same
    // retry. The final wall-only test then rejects both identically.
    bool tile(int depth,Real,Real,bool& hit){hit=false;return depth==100;}
    bool viewY(Real& value){
        value=exactQ8(cameraQ8);return value.add(Real::fromInteger(34));
    }
    bool viewHeight(Real& value){value=Real::fromInteger(284);return true;}
    bool create(int object,Real x,Real y,unsigned& id){
        id=1;return world.createSpawnedEnemy(object,x,y);
    }
    bool deactivate(unsigned){return true;} // Children are created inactive.
};

bool GameplayWorld::stepCompactSpawner(long generationCameraY){
    SpawnerOps ops{*this,generationCameraY};
    if(!compactSpawner_.step(compactSpawnerY_,enemyRandom_,ops)||!run_.effectsSucceeded)
        return run_.effectsSucceeded=false;
    return true;
}

bool GameplayWorld::createSpawnedEnemy(int object,downwell::MotionBinary64 x,downwell::MotionBinary64 y){
    std::int32_t px,py;
    if(!run_.effectsSucceeded||!x.roundPixel(px)||!y.roundPixel(py)||enemyCount_>=enemyCapacity)
        return run_.effectsSucceeded=false;
    if(object==114)return createEyefish(px,py);
    if(object==116){
        // enmBat Create, as AreaOneBuilder's: one discarded word, then heading.
        enemyRandom_.nextU32();
        const int direction=int(enemyRandom_.irandomSmall(359));
        const auto profile=downwell::enemyCreateProfile(116);
        return configureEnemy(enemyCount_,116,153,px-240,py-176,0,0,profile.health,profile.reward,
            0,0,-1,-1,direction,320,false,false,true,0);
    }
    if(!steeringObject(object))return run_.effectsSucceeded=false;
    unsigned slot=0;
    while(slot<steeringCapacity&&steeringOwner_[slot])++slot;
    if(slot==steeringCapacity)return run_.effectsSucceeded=false;
    downwell::SteeringFixedState* actor;
    if(object==115)actor=new(&steering_[slot].jelly) downwell::WhiteJellyFixedStep{};
    else{actor=new(&steering_[slot].ball) downwell::RedBallFixedStep{};steering_[slot].ball.neon=object==173;}
    SteeringOps ops{*this,nullptr,*actor};
    const bool born=object==115?steering_[slot].jelly.create(px,py,enemyRandom_,ops):
        steering_[slot].ball.create(px,py,enemyRandom_,ops);
    if(!born||!run_.effectsSucceeded)return run_.effectsSucceeded=false;
    const std::int32_t bx=actor->pixelX,by=actor->pixelY;
    // instance_deactivate_object follows Create: born inactive and pending.
    if(!configureEnemy(enemyCount_,object,object==115?176:object==118?702:352,bx-240,by-176,0,0,actor->health,actor->money,
        0,int(slot),ops.alarm0,ops.alarm1,0,0,false,false,true,0))return false;
    auto& enemy=enemies_[enemyCount_-1];
    enemy.imageSpeed=4; // imgSp 0.2, in twentieths.
    steeringOwner_[slot]=enemy.lifetimeId;++steeringPopulation_;
    return true;
}

// enmEyefish Create after subparentEnemy's prefix (move_snap leaves the
// spawner's 16-pixel grid as it is): xsp = choose(0.5, -0.5). image_xscale
// stays 1 until its first Step. Born inactive and pending, like the others.
bool GameplayWorld::createEyefish(int px,int py){
    const bool left=enemyRandom_.chooseIndexSmall(2)!=0;
    const auto profile=downwell::enemyCreateProfile(114);
    if(!configureEnemy(enemyCount_,114,289,px-240,py-176,0,0,profile.health,profile.reward,
        0,0,-1,-1,0,0,false,false,true,0))return false;
    auto& enemy=enemies_[enemyCount_-1];
    enemy.eyefish.create(px,py,left);
    enemy.speedX=left?-128:128; // xsp, read by scrDeadBody
    return true;
}

// enmEyefish Step. Its Alarm 1 is never armed, hitStun is never set (bullets
// are refused, and a stomp kills at the contact in this world), so the
// sprite stays sprEyefish; under pTimeStop only image_speed stops.
bool GameplayWorld::stepEyefish(EnemyState& enemy){
    const bool frozen=compactTimeStop()&&stopped();
    enemy.frozen=frozen;
    if(!frozen){
        std::int32_t px;
        if(!enemy.eyefish.step([this](downwell::MotionBinary64 x,downwell::MotionBinary64 y,int,bool& hit){
               return steeringWallMeeting(289,x,y,hit);},px)||!run_.effectsSucceeded)
            return run_.effectsSucceeded=false;
        const int scale=enemy.eyefish.left()?-1:1; // xscaleXsp
        enemy.speedX=scale*128;
        if(!enemyLifecycle_.recordOwnsAppearance()&&
           !enemyLifecycle_.integerPosition({enemy.identitySlot,enemy.lifetimeId},enemy,px,enemy.y+176))
            return run_.effectsSucceeded=false;
        enemy.x=px-240;enemy.preciseX=enemy.x*256L;
        if(enemy.facing!=scale){
            if(!publishEnemyTransform(enemy,downwell::RunnerTransformField::scaleX,scale))return false;
            enemy.facing=scale;
        }
    }
    while((enemy.imagePhase>>8)>=4u)enemy.imagePhase-=4u<<8;
    if(!publishEnemyImage(enemy,enemy.imagePhase))return false;
    enemy.imageFrame=enemy.imagePhase>>8;
    return true;
}

bool GameplayWorld::stepSteeringEnemy(EnemyState& enemy){
    auto* actor=steeringActor(enemy);
    if(!actor)return run_.effectsSucceeded=false;
    SteeringOps ops{*this,&enemy,*actor};
    // Under pTimeStop their Step only sets image_speed = 0 and checks ehp.
    const bool frozen=compactTimeStop()&&stopped();
    enemy.frozen=frozen;
    const bool stepped=enemy.objectId==115?steering_[enemy.auxiliaryState].jelly.step(frozen,enemyRandom_,ops):
        steering_[enemy.auxiliaryState].ball.step(frozen,enemyRandom_,ops);
    if(!stepped||!run_.effectsSucceeded)return run_.effectsSucceeded=false;
    enemy.health=actor->health;
    if(!enemy.active){releaseSteering(enemy);enemy.auxiliaryState=-1;return true;}
    const std::int32_t px=actor->pixelX,py=actor->pixelY;
    if(!enemyLifecycle_.recordOwnsAppearance()&&
       !enemyLifecycle_.integerPosition({enemy.identitySlot,enemy.lifetimeId},enemy,px,py))
        return run_.effectsSucceeded=false;
    enemy.x=px-240;enemy.y=py-176;
    enemy.preciseX=enemy.x*256L;enemy.preciseY=enemy.y*256L;
    // image_index wraps at the current sprite's frame count.
    const unsigned frames=enemy.spriteId==175||enemy.spriteId==703?1u:4u;
    while((enemy.imagePhase>>8)>=frames)enemy.imagePhase-=frames<<8;
    if(!publishEnemyImage(enemy,enemy.imagePhase))return false;
    enemy.imageFrame=enemy.imagePhase>>8;
    // Draw's eye sector depends only on Step state; record it here. A first
    // Draw in the unmatched (359,360) sector has no eye to draw.
    if(enemy.objectId==118||enemy.objectId==173)(void)steering_[enemy.auxiliaryState].ball.updateEye();
    return true;
}

bool GameplayWorld::advanceSteeringAlarm(EnemyState& enemy,unsigned alarm){
    auto* actor=steeringActor(enemy);
    if(!actor)return run_.effectsSucceeded=false;
    if(alarm==1){actor->hitStun=false;return true;}
    if(enemy.objectId==115)return run_.effectsSucceeded=false; // Never armed.
    SteeringOps ops{*this,&enemy,*actor};
    return run_.effectsSucceeded=steering_[enemy.auxiliaryState].ball.effectAlarm(stopped(),enemyRandom_,ops)&&
        run_.effectsSucceeded;
}

bool GameplayWorld::hitSteeringEnemy(EnemyState& enemy,std::uint32_t directionTurns,int damage){
    // scrTypicalBulletDamage: hit, hitDir = bDir, hitDmg, then ehp -= bdmg.
    // Death follows in the enemy's own Step, not at the impact.
    auto* actor=steeringActor(enemy);
    if(!actor)return run_.effectsSucceeded=false;
    actor->hitDirection16=downwell::SteeringFixedState::turnsDegrees(directionTurns);
    actor->hit=true;actor->hitDamage+=damage;actor->health-=damage;
    enemy.health=actor->health;
    compactMomentDelay(); // scrDamageEnemy
    return true;
}

bool GameplayWorld::collideSteeringEnemies(){
    // objPlayer_n contacts: typStompable (115) and typical damage (118, 173).
    for(unsigned i=0;i<enemyCount_;++i){
        auto& enemy=enemies_[i];
        if(!steeringObject(enemy.objectId)||!enemy.active||enemy.destroyRequested||enemy.health<=0)continue;
        auto* actor=steeringActor(enemy);
        if(!actor)return run_.effectsSucceeded=false;
        const long stompY=actor->stompCeilQ8()-176L*256;
        const downwell::ContactActor contact{enemy.x*256L,enemy.y*256L,enemy.facing,1,
            downwell::enemyContactMask(enemy.objectId,enemy.spriteId)};
        bool active=true;int health=enemy.health;decltype(run_.kills) defeated=0;
        bool succeeded=true;
        const auto result=downwell::applyEnemyContact(enemy.objectId,contact,enemy.x,enemy.y,active,health,
            player_,health_,invulnerability_,defeated,[]{return true;},run_.upgrades.levels[7]!=0,
            [&](int x,int y){
                compactDamageSlowdown();
                return succeeded=run_.contactDamageNumber(player_.frameStartXFixed(),player_.frameStartYFixed(),
                    x,y,240,176)&&playPlayerHurt()&&compactTimeoutField();
            },downwell::NoContactRetirement{},stompY);
        if(!succeeded||!run_.effectsSucceeded)return run_.effectsSucceeded=false;
        if(result==downwell::EnemyContact::stomp){
            run_.soundPlayOL(122,50,false,true,"footsteps");
            // scrEnemyStomped: scrDamageToEnemy(100), hit and stomped; the
            // death, combo and reward follow in the jelly's next Step.
            actor->health-=100;actor->hit=true;actor->stomped=true;
            enemy.health=actor->health;
            if(run_.upgrades.levels[0]&&!compactExplodingKnees(enemy.x+240,enemy.y+176,
                240+int(downwell::roundGameMakerQ8(player_.frameStartXFixed())),
                176+int(downwell::roundGameMakerQ8(player_.frameStartYFixed())),true))return false;
        }
    }
    // Collision events with other enemies: jelly and neon red ball vs
    // subparentEnemy, red ball vs red ball. All read the other's published
    // builtin x/y.
    const auto otherBounds=[](const EnemyState& other){
        return downwell::contactWorldBounds({other.x*256L,other.y*256L,
            other.objectId==117?other.motionState:other.facing,other.imageScaleY(),
            downwell::enemyContactMask(other.objectId,other.spriteId,other.maskId)});
    };
#ifndef DOWNWELL_REFERENCE_STEERING_BOUNDS
    // Each enemy's published bounds, computed once per update. A contact
    // event changes only its own steering enemy (sprite, facing, activity;
    // never the published x/y), whose entry is recomputed after its events.
    downwell::ContactBounds others[enemyCapacity];
    bool known[enemyCapacity]{};
    const auto bounds=[&](unsigned j)->const downwell::ContactBounds&{
        if(!known[j]){others[j]=otherBounds(enemies_[j]);known[j]=true;}
        return others[j];
    };
#else
    const auto bounds=[&](unsigned j){return otherBounds(enemies_[j]);};
#endif
    for(unsigned i=0;i<enemyCount_;++i){
        auto& self=enemies_[i];
        if(!steeringObject(self.objectId)||!self.active||self.destroyRequested)continue;
        const auto selfBounds=downwell::contactWorldBounds({self.x*256L,self.y*256L,self.facing,1,
            downwell::enemyContactMask(self.objectId,self.spriteId)});
#ifndef DOWNWELL_REFERENCE_STEERING_BOUNDS
        known[i]=false;
#endif
        for(unsigned j=0;j<enemyCount_&&j<enemyCapacity;++j){
            const auto& other=enemies_[j];
            if(j==i||!other.active||other.destroyRequested||(self.objectId==118&&other.objectId!=118))continue;
            if(!downwell::contactBoundsOverlap(selfBounds,bounds(j)))continue;
            auto* actor=steeringActor(self);
            if(!actor)return run_.effectsSucceeded=false;
            struct CollisionOps : SteeringOps {
                const EnemyState& other;
                bool otherPixel(bool vertical,std::int32_t& value){value=vertical?other.y+176:other.x+240;return true;}
            } ops{{*this,&self,*actor},other};
            const bool collided=self.objectId==115?steering_[self.auxiliaryState].jelly.enemyContact(enemyRandom_,ops):
                steering_[self.auxiliaryState].ball.enemyContact(enemyRandom_,ops);
            if(!collided||!run_.effectsSucceeded)return run_.effectsSucceeded=false;
        }
#ifndef DOWNWELL_REFERENCE_STEERING_BOUNDS
        known[i]=false;
#endif
    }
    return true;
}

// ---- Compact W1.3 chains: boxes, scenery, debris and script effects ----

namespace {
using SceneryReal=downwell::MotionBinary64;
std::uint32_t bulletDirection(const GameplayWorld::BulletState& bullet){
    return bullet.kind==GameplayWorld::BulletState::legacy?bullet.trajectory.direction:bullet.shot.direction;
}
// Shots face their bDir; masks rotate by the nearest quarter turn, as the
// machine gun's 267..273 always did. Local (x,y) maps to (y,-x) at 270.
downwell::ContactBounds rotatedShotBounds(int sprite,std::uint32_t direction,int x,int y){
    const auto l=downwell::contactSpriteBounds(sprite);
    switch(((direction+0x20000000u)>>30)&3u){
        case 0:return {x+l.left,x+l.right,y+l.top,y+l.bottom};
        case 1:return {x+l.top,x+l.bottom,y-l.right+1,y-l.left+1};
        case 2:return {x-l.right+1,x-l.left+1,y-l.bottom+1,y-l.top+1};
        default:return {x-l.bottom+1,x-l.top+1,y+l.left,y+l.right};
    }
}
}

bool GameplayWorld::resetCompactScenery(){
    compactEffects_.clear();scenery_.clear();sceneryRepaintCount_=0;
    // scrPlayerInit: aimAngle = 0, burstCount = 0; room instances are gone.
    guns_.clear();aimAngle_=0;burstCount_=0;burstAlarm_=-1;
    if(!compactDebris_.clear())return run_.effectsSucceeded=false;
    clearCompactUpgrades();
    {struct Hooks{bool retire(std::uint32_t,const downwell::CorpseAnimation&){return true;}} hooks;
        if(!compactCorpses_.clear(hooks))return run_.effectsSucceeded=false;}
    // Result Create clears pTimeStop; the next room's player is new.
    return resetCompactTimeStop();
}

bool GameplayWorld::publishSceneryRepaint(int x,int y){
    if(sceneryRepaintCount_==downwell::gameplaySceneryRepaintCapacity)return run_.effectsSucceeded=false;
    sceneryRepaintX_[sceneryRepaintCount_]=x-240;sceneryRepaintY_[sceneryRepaintCount_++]=y-176;
    return true;
}

int GameplayWorld::spawnCompactCurrency(int amount,int x,int y,bool attracted){
    if(!run_.effectsSucceeded)return -1;
    unsigned next=downwell::compactCurrencySlots(gems_,gemSlotEnd_);
    unsigned available=gemCapacity-next;
    // Exhausted keys cannot alias a retired lifetime. Bound births before
    // Create consumes RNG, just as for exhausted storage capacity.
    const std::uint32_t identities=nextGemLifetimeId_?0xffffffffu-nextGemLifetimeId_+1u:0u;
    if(identities<available)available=identities;
    int last=-1;
    const auto reward=downwell::spawnCurrencyReward(amount,x,y,attracted,available,enemyRandom_,
        [&](const downwell::CurrencyBirth& birth){
        last=int(next);
        GemState& gem=gems_[next++];
        gem.lifetimeId=nextGemLifetimeId_++;
        gem.large=birth.large;gem.amount=birth.amount;gem.age=0;
        gem.x=x;gem.y=y;gem.preciseX=long(x)*256L;gem.preciseY=long(y)*256L;
        gem.motion=birth.motion;
        gem.speedX=downwell::GemMotion::q8(gem.motion.vx);
        gem.speedY=downwell::GemMotion::q8(gem.motion.vy);
        gem.attracted=attracted;gem.active=true;
    },run_.upgrades.levels[10]!=0,run_.stage.area);
    gemSlotEnd_=next;
    if(!reward.complete){run_.effectsSucceeded=false;return -1;}
    return last;
}

// gemChunk's with-blocks: objCurrency Create (with its RNG), then ugrav
// 0.045, xsp/ysp random_range draws, Alarm 0 = 20, Alarm 3 = 60, and the
// opened chunk's getAmount 10/size 1. ugravhard is not modelled by GemMotion.
int GameplayWorld::spawnCompactBurstGem(int x,int y,long xRange,long xLow,long yRange,long yLow,int amount,bool large){
    if(!run_.effectsSucceeded)return -1;
    const unsigned next=downwell::compactCurrencySlots(gems_,gemSlotEnd_);
    gemSlotEnd_=next;
    if(next>=gemCapacity||!nextGemLifetimeId_||xRange<=0||yRange<=0){run_.effectsSucceeded=false;return -1;}
    GemState& gem=gems_[next];
    gem.motion.initialize(x,y,false,enemyRandom_,run_.upgrades.levels[10]!=0,run_.stage.area);
    gem.motion.gravity=576;
    gem.motion.vx=long(enemyRandom_.randomIntegerSmall(std::uint32_t(xRange)))+xLow;
    gem.motion.vy=long(enemyRandom_.randomIntegerSmall(std::uint32_t(yRange)))+yLow;
    gem.motion.lifetime.obtainAt=20;gem.motion.lifetime.suckAt=60;
    gem.lifetimeId=nextGemLifetimeId_++;
    gem.large=large;gem.amount=amount;gem.age=0;
    gem.x=x;gem.y=y;gem.preciseX=long(x)*256L;gem.preciseY=long(y)*256L;
    gem.speedX=downwell::GemMotion::q8(gem.motion.vx);
    gem.speedY=downwell::GemMotion::q8(gem.motion.vy);
    gem.attracted=false;gem.active=true;
    gemSlotEnd_=next+1;
    return int(next);
}

bool GameplayWorld::attractCompactGem(int slot){
    if(slot<0||unsigned(slot)>=gemSlotEnd_||!gems_[slot].active)return run_.effectsSucceeded=false;
    gems_[slot].motion.attracted=true;gems_[slot].attracted=true;return true;
}

bool GameplayWorld::spawnCompactDebris(downwell::Fixed16 x,downwell::Fixed16 y,std::uint32_t& handle){
    if(!run_.effectsSucceeded)return run_.effectsSucceeded=false;
    handle=compactDebris_.create(x,y,run_.stage.area,enemyRandom_);
    return run_.effectsSucceeded=handle!=0;
}

// sParentSolid (56) or parentWall (57) place_meeting for a rectangle mask:
// generated walls, live boxes and shop walls, plus thin platforms for 56.
// Whether a projectile's line meets unbreakable terrain: the segment
// resolver when configured, otherwise the point resolver over the line's
// bounds (fixtures that compare the two). Shots and casings both ask here.
bool GameplayWorld::segmentSolid(const downwell::ProjectileSegment& path)const{
    if(!solidResolver_)return false;
    if(segmentResolver_)return segmentResolver_(solidResolverContext_,path);
    const int left=path.x.pixel<path.endX.pixel?path.x.pixel:path.endX.pixel;
    const int right=path.x.pixel>path.endX.pixel?path.x.pixel:path.endX.pixel;
    const int top=path.y.pixel<path.endY.pixel?path.y.pixel:path.endY.pixel;
    const int bottom=path.y.pixel>path.endY.pixel?path.y.pixel:path.endY.pixel;
    for(int y=top;y<=bottom;++y)
        for(int x=left;x<=right;++x)
            if(solidResolver_(solidResolverContext_,x,y)&&path.intersects(x,x+1,y,y+1))return true;
    return false;
}

bool GameplayWorld::solidMeeting(int mask,downwell::MotionBinary64 x,downwell::MotionBinary64 y,bool platforms,bool& hit){
    downwell::ContactBounds bounds;
    if(!run_.effectsSucceeded||!maskBounds_.calculate(mask,x,y,bounds))
        return run_.effectsSucceeded=false;
    return solidContact(bounds,platforms,hit);
}

bool GameplayWorld::solidContact(const downwell::ContactBounds& bounds,bool platforms,bool& hit){
    bool found=false;
    if(wallContactResolver_&&(!platforms||platformContactResolver_)){
        unsigned count=0;
        if(!wallContactResolver_(solidResolverContext_,bounds,1,count))return run_.effectsSucceeded=false;
        found=count!=0;
    }else if(solidResolver_){
        // Worlds with only a point resolver (host fixtures) scan the mask's
        // pixel box, as compactCorpseMeeting does; without any resolver
        // nothing is solid. The game always installs contact resolvers.
        for(int py=bounds.top;py<bounds.bottom&&!found;++py)
            for(int px=bounds.left;px<bounds.right&&!found;++px)
                found=solidResolver_(solidResolverContext_,px-240,py-176);
        platforms=false;
    }else platforms=false;
    if(!found&&!run_.shops.visitWallContacts(bounds,[&](){found=true;return true;}))return run_.effectsSucceeded=false;
    if(!found&&platforms&&!platformContactResolver_(solidResolverContext_,bounds,found))return run_.effectsSucceeded=false;
    hit=found;return true;
}

// scrInView(0,0,0) against the published view of this Step.
bool GameplayWorld::compactInView(int x,int y,bool& visible)const{
    auto top=downwell::MotionBinary64::fromInteger(boneViewTopQ8_);
    if(!top.zero())top.high-=8u<<20;
    return downwell::boneInPublishedView(downwell::MotionBinary64::fromInteger(x),downwell::MotionBinary64::fromInteger(y),
        downwell::MotionBinary64::fromInteger(boneViewLeft_),top,
        downwell::MotionBinary64::fromInteger(160),downwell::MotionBinary64::fromInteger(284),visible);
}

struct GameplayWorld::SceneryOps {
    using Real=downwell::MotionBinary64;
    GameplayWorld& world;
    int x,y;                // caller's builtin position, room coordinates
    int gem=-1;             // myGem: the caller's latest objCurrency birth
    std::uint32_t debris=0; // the latest breakablesDebris birth
    bool changed=false,destroyed=false;
    bool fail(){return world.run_.effectsSucceeded=false;}
    bool sound(int id,int priority,int loop,int cut){
        world.run_.soundPlay(id,priority,loop!=0,cut!=0);
        return world.run_.effectsSucceeded;
    }
    bool movingFx(downwell::Fixed16 px,downwell::Fixed16 py,int sprite,downwell::RunnerImageValue rate,
        downwell::Fixed16 angle,downwell::Fixed16 speed){
        return world.compactEffects_.spawnMoving(px,py,sprite,rate,angle,speed)||fail();
    }
    bool appearanceChanged(){changed=true;return true;}
    // scrOutofview: instance_destroy outside the rmMain band; the event goes on.
    bool outOfView(int py,bool& retire){
        const long top=world.boneViewTopQ8_/256;
        retire=py<top-160||py>top+3000;
        if(retire)destroyed=true;
        return true;
    }
    bool outOfView(){bool retire;return outOfView(y,retire);}
    // scrCurrencySpawn: neither 289 nor 295 is a subparentEnemy child.
    bool currency(int amount){gem=world.spawnCompactCurrency(amount,x-240,y-176,false);return gem>=0;}
    bool currency(int amount,int){return currency(amount);}
    bool attractGem(){return world.attractCompactGem(gem);}
    bool createDebris(){return world.spawnCompactDebris(downwell::Fixed16::fromInteger(x),downwell::Fixed16::fromInteger(y),debris);}
    bool debrisYSpeed(downwell::Fixed16 value){
        auto* effect=world.compactDebris_.get(debris);
        if(!effect||!effect->setYSpeed(value))return fail();
        return true;
    }
    bool sprite(int){changed=true;return true;}
    // Collision with objPlayer_n: objPlayer_n.ysp > 0, then scrFjump/scrSShake.
    bool playerYSpeed(Real& value){value=Real::fromInteger(int(world.player_.speedYFifths()));return true;}
    bool playStyle(int& style){style=int(world.run_.playStyle);return true;}
    bool jump(int,Real){world.player_.stompBounce();return true;}
    bool shake(int amount,int duration){return world.requestCameraShake(amount,duration);}
    bool explodingKnees(bool& knees){knees=world.run_.upgrades.levels[0]!=0;return true;}
    bool createBlast(){return world.createCompactBlast(276,x,y);}
    // Torch: place_meeting with its own sprite mask.
    bool solid(int px,int py,bool& hit){return world.solidMeeting(568,Real::fromInteger(px),Real::fromInteger(py),true,hit);}
    bool wall(int px,int py,bool& hit){return world.solidMeeting(568,Real::fromInteger(px),Real::fromInteger(py),false,hit);}
    bool destroy(){destroyed=true;return true;}
    bool createFire(int px,int py){
        auto* fire=downwell::CompactScenery::append(world.scenery_.fires,world.scenery_.fireEnd);
        if(!fire)return fail();
        fire->create(px,py);fire->birth=world.tick_;return true;
    }
    bool inView(int px,int py,bool& visible){return world.compactInView(px,py,visible)||fail();}
    // gemChunk.
    bool openMask(){return true;} // gemChunkSolidAt reads the opened flag.
    bool flash(downwell::Fixed16 px,downwell::Fixed16 py){return world.run_.emitFlash<3,0,5>(px,py,world.enemyRandom_);}
    bool burstGem(long xRange,long xLow,long yRange,long yLow,int amount,bool large){
        gem=world.spawnCompactBurstGem(x-240,y-176,xRange,xLow,yRange,yLow,amount,large);
        return gem>=0;
    }
};

namespace {
bool repaintGemChunk(GameplayWorld& world,const downwell::CompactGemChunkRecord& r,
                     bool (GameplayWorld::*publish)(int,int)){
    return (world.*publish)(r.x-8,r.y-8)&&(world.*publish)(r.x+8,r.y-8)&&
           (world.*publish)(r.x-8,r.y+8)&&(world.*publish)(r.x+8,r.y+8);
}
}

bool GameplayWorld::createCompactScenery(const downwell::DeferredCreateInstance& command){
    if(!run_.effectsSucceeded)return run_.effectsSucceeded=false;
    auto& sc=scenery_;
    if(command.objectId==289){
        auto* r=downwell::CompactScenery::append(sc.scattered,sc.scatteredEnd);
        if(!r)return run_.effectsSucceeded=false;
        r->x=command.x;r->y=command.y;r->scaleX=command.integerScaleX<0?-1:1;r->birth=tick_;
        r->scatterType=command.integerImageIndex;
        return true;
    }
    if(command.objectId==293){
        auto* r=downwell::CompactScenery::append(sc.gemChunks,sc.gemChunkEnd);
        if(!r)return run_.effectsSucceeded=false;
        r->x=command.x;r->y=command.y;r->birth=tick_;
        r->alarm[1]=command.alarms.get(1);r->alarm[2]=command.alarms.get(2);
        return true;
    }
    if(command.objectId==296){
        auto* r=downwell::CompactScenery::append(sc.torches,sc.torchEnd);
        if(!r)return run_.effectsSucceeded=false;
        r->x=command.x;r->y=command.y;r->birth=tick_;
        return true;
    }
    return true;
}

bool GameplayWorld::advanceCompactSceneryAlarm(unsigned alarm){
    if(alarm<3)for(unsigned i=0;i<scenery_.gemChunkEnd;++i){
        auto& r=scenery_.gemChunks[i];
        if(!r.live||!downwell::AlarmBank::advanceValue(r.alarm[alarm]))continue;
        SceneryOps ops{*this,r.x,r.y};
        const bool ok=alarm==0?r.alarm0(enemyRandom_,ops):alarm==1?r.alarm1(enemyRandom_,ops):r.alarm2(enemyRandom_,ops);
        if(!ok||!run_.effectsSucceeded)return run_.effectsSucceeded=false;
        if(ops.changed&&!repaintGemChunk(*this,r,&GameplayWorld::publishSceneryRepaint))return false;
    }
    if(alarm<3&&compactDebris_.size()&&
       !compactDebris_.advanceAlarm(compactDebris_.beginPhase(),alarm))return run_.effectsSucceeded=false;
    return true;
}

bool GameplayWorld::advanceCompactSceneryAnimation(){
    if(compactEffects_.size()&&!compactEffects_.advanceAnimation(compactEffects_.beginPhase()))
        return run_.effectsSucceeded=false;
    for(unsigned i=0;i<scenery_.gemChunkEnd;++i){
        auto& r=scenery_.gemChunks[i];
        if(!r.live||!r.imageSpeed)continue;
        const unsigned before=r.frame();
        r.advanceImage();
        if(r.frame()!=before&&!repaintGemChunk(*this,r,&GameplayWorld::publishSceneryRepaint))return false;
    }
    if(compactDebris_.size()&&
       !compactDebris_.advanceAnimation(compactDebris_.beginPhase()))return run_.effectsSucceeded=false;
    return true;
}

// Object order: Scattered16 289, gemChunk 293, TorchFire 295, Torch 296.
// Births of this Step (builder rows, torch fires) wait for the next one.
bool GameplayWorld::stepCompactScenery(){
    auto& sc=scenery_;
    for(unsigned i=0;i<sc.scatteredEnd;++i){
        auto& r=sc.scattered[i];
        if(!r.live||r.birth==tick_)continue;
        SceneryOps ops{*this,r.x,r.y};
        if(!downwell::ScatteredEvents::step(r,enemyRandom_,ops)||!run_.effectsSucceeded)return run_.effectsSucceeded=false;
        if(ops.destroyed)r.live=false;
        if(ops.changed&&!publishSceneryRepaint(r.x,r.y))return false;
    }
    for(unsigned i=0;i<sc.gemChunkEnd;++i){
        auto& r=sc.gemChunks[i];
        if(!r.live||r.birth==tick_)continue;
        SceneryOps ops{*this,r.x,r.y};
        if(!r.step(enemyRandom_,ops)||!run_.effectsSucceeded)return run_.effectsSucceeded=false;
        if(ops.changed&&!repaintGemChunk(*this,r,&GameplayWorld::publishSceneryRepaint))return false;
    }
    for(unsigned i=0;i<sc.fireEnd;++i){
        auto& r=sc.fires[i];
        if(!r.live||r.birth==tick_)continue;
        if(!timeStopBound(710,r.x,r.y))continue; // The whole Step is TimeStopBound.
        SceneryOps ops{*this,r.x,r.y};
        if(!r.step(enemyRandom_,ops)||!run_.effectsSucceeded)return run_.effectsSucceeded=false;
        if(ops.destroyed)r.live=false;
    }
    const long top=boneViewTopQ8_/256;
    for(unsigned i=0;i<sc.torchEnd;++i){
        auto& r=sc.torches[i];
        if(!r.live||r.birth==tick_)continue;
        if(r.allSet){
            // No further events: the record only keeps its art. Release it
            // once well above every view that could still repaint it.
            if(r.y<top-320)r.live=false;
            continue;
        }
        SceneryOps ops{*this,r.x,r.y};
        if(!r.step(ops)||!run_.effectsSucceeded)return run_.effectsSucceeded=false;
        if(ops.destroyed)r.placement=-1;
        if(ops.changed&&!publishSceneryRepaint(r.x,r.y+1))return false;
    }
    return true;
}

// Collision events with objPlayer_n, after the ordinary Step.
bool GameplayWorld::collideCompactScenery(){
    if(dead())return true;
    const auto player=downwell::contactWorldBounds({240L*256+player_.offsetXFixed(),176L*256+player_.offsetYFixed(),1,1,31});
    auto& sc=scenery_;
    for(unsigned i=0;i<sc.scatteredEnd;++i){
        auto& r=sc.scattered[i];
        if(!r.live||!downwell::contactBoundsOverlap(player,downwell::contactPixelBounds(r.x,r.y,r.scaleX,1,550)))continue;
        SceneryOps ops{*this,r.x,r.y};
        if(!downwell::ScatteredEvents::playerContact(r,enemyRandom_,ops)||!run_.effectsSucceeded)return run_.effectsSucceeded=false;
    }
    for(unsigned i=0;i<sc.fireEnd;++i){
        auto& r=sc.fires[i];
        if(!r.live||!r.masked||!r.active||player_.speedYFifths()<=0||
           !downwell::contactBoundsOverlap(player,downwell::contactPixelBounds(r.x,r.y,1,1,710)))continue;
        r.objHp=0;r.stomped=1;
        player_.stompBounce(); // scrFjump(0,2.5), or 1.5 for style 2.
        if(!requestCameraShake(2,4))return false;
        if(run_.upgrades.levels[0]){
            if(!createCompactBlast(276,r.x,r.y))return false;
            run_.soundPlay(91+int(enemyRandom_.chooseIndexSmall(3)),60,false,true);
        }
    }
    return true;
}

// scrBulCheckSolid's subparentSolidWall line: an unopened gem chunk takes
// the damage while wallHp > 0 and stops the shot; at 0 the shot passes.
bool GameplayWorld::hitGemChunk(const downwell::ProjectileSegment& path,int damage,bool& hit,bool& consumed){
    hit=consumed=false;
    for(unsigned i=0;i<scenery_.gemChunkEnd;++i){
        auto& r=scenery_.gemChunks[i];
        if(!r.live||r.opened)continue;
        const auto b=downwell::contactPixelBounds(r.x-240,r.y-176,1,1,569);
        if(!path.intersects(b.left,b.right,b.top,b.bottom))continue;
        hit=true;
        if(r.wallHp>0){r.wallHp-=damage;r.hit=1;consumed=true;}
        return true;
    }
    return true;
}

// Without a wall on the line: place_meeting(x,y,parentShootableThing).
bool GameplayWorld::hitCompactShootable(const downwell::ProjectileSegment& path,const BulletState& bullet,bool& consumed){
    consumed=false;
    downwell::RunnerImageValue bx,by;std::int32_t px,py;
    if(!downwell::ProjectilePublication::coordinate(path.x,240,bx)||
       !downwell::ProjectilePublication::coordinate(path.y,176,by)||
       !bx.roundPixel(px)||!by.roundPixel(py))return run_.effectsSucceeded=false;
    const auto shot=rotatedShotBounds(bullet.sprite,bulletDirection(bullet),px,py);
    const int damage=bullet.damage;
    if(guns_.moduleEnd){
        bool module=false;
        if(!hitGunModule(shot,damage,module))return false;
        if(module){consumed=true;return true;}
    }
    auto& sc=scenery_;
    for(unsigned i=0;i<sc.scatteredEnd;++i){
        auto& r=sc.scattered[i];
        if(!r.live||!downwell::contactBoundsOverlap(shot,downwell::contactPixelBounds(r.x,r.y,r.scaleX,1,550)))continue;
        if(r.objHp<=0)return true;
        r.objHp-=damage;r.hit=1;
        downwell::RunnerImageValue angle;downwell::Fixed16 qx,qy,qa;
        if(!downwell::ProjectilePublication::angle(bulletDirection(bullet),angle)||
           !downwell::Fixed16::fromBinary32(bx.bits,qx)||!downwell::Fixed16::fromBinary32(by.bits,qy)||
           !downwell::Fixed16::fromBinary32(angle.bits,qa)||!compactEffects_.spawnNoLoop(qx,qy,605,fourTenths,qa))
            return run_.effectsSucceeded=false;
        consumed=true;return true;
    }
    for(unsigned i=0;i<sc.fireEnd;++i){
        auto& r=sc.fires[i];
        if(!r.live||!r.masked||!downwell::contactBoundsOverlap(shot,downwell::contactPixelBounds(r.x,r.y,1,1,710)))continue;
        if(r.objHp<=0)return true;
        r.objHp-=damage;r.hit=1; // parentShootableThing's hitEffect stays 0.
        consumed=true;return true;
    }
    // objDeadBody (maskDead 599) is shootable only with Rest in Pieces: its
    // objHp is 0 otherwise. Its Create sets hitEffect, as Scattered16's does.
    if(!run_.upgrades.levels[3]||!compactCorpses_.size())return true;
    bool met=false,ok=true;
    struct Hooks{} hooks;
    if(!compactCorpses_.dispatch(compactCorpses_.beginPhase(),[&](auto,downwell::CorpseAnimation& corpse){
        if(met)return true;
        std::int32_t cx,cy;
        if(!corpse.x.roundPixel(cx)||!corpse.y.roundPixel(cy))return false;
        if(!downwell::contactBoundsOverlap(shot,downwell::contactPixelBounds(cx,cy,1,1,599)))return true;
        met=true;
        int order;
        if(!downwell::compareRunnerReal(corpse.health,{},order))return false;
        if(order<=0)return true;
        if(!corpse.health.add(downwell::MotionBinary64::fromInteger(-damage)))return false;
        downwell::RunnerImageValue angle;downwell::Fixed16 qx,qy,qa;
        ok=downwell::ProjectilePublication::angle(bulletDirection(bullet),angle)&&
           downwell::Fixed16::fromBinary32(bx.bits,qx)&&downwell::Fixed16::fromBinary32(by.bits,qy)&&
           downwell::Fixed16::fromBinary32(angle.bits,qa)&&compactEffects_.spawnNoLoop(qx,qy,605,fourTenths,qa);
        consumed=true;return ok;
    },hooks))return run_.effectsSucceeded=false;
    return true;
}

bool GameplayWorld::sceneryAppearance(int objectId,int x,int y,int& sprite,int& frame)const{
    if(objectId==289){
        for(unsigned i=0;i<scenery_.scatteredEnd;++i){const auto& r=scenery_.scattered[i];
            if(r.live&&r.x==x&&r.y==y){sprite=r.spriteId;return true;}
        }
    }else if(objectId==293){
        for(unsigned i=0;i<scenery_.gemChunkEnd;++i){const auto& r=scenery_.gemChunks[i];
            if(r.live&&r.x==x&&r.y==y){sprite=r.sprite;frame=int(r.frame());return true;}
        }
    }else if(objectId==296){
        for(unsigned i=0;i<scenery_.torchEnd;++i){const auto& r=scenery_.torches[i];
            if(!r.live||r.x!=x||r.y!=y||!r.allSet)continue;
            if(r.placement<0)return false;
            frame=r.placement;return true;
        }
    }
    return true;
}

bool GameplayWorld::gemChunkSolidAt(int x,int y)const{
    for(unsigned i=0;i<scenery_.gemChunkEnd;++i){const auto& r=scenery_.gemChunks[i];
        if(r.live&&r.x==x&&r.y==y)return !r.opened;
    }
    return true;
}

struct GameplayWorld::BoxOps {
    using Real=downwell::MotionBinary64;
    using Handle=std::uint32_t;
    GameplayWorld& world;downwell::InstanceHandle key;
    bool fail(){return world.run_.effectsSucceeded=false;}
    // objBox_n Step: the break on waterThings (channel 7: no view test) and
    // the gunpowder chain's sfxUpgradeGunpowder_Bullet.
    bool soundOverlap(int id,int priority,int loop,int cut,const char* channel){
        world.run_.soundPlayOL(id,priority,loop!=0,cut!=0,channel);
        return world.run_.effectsSucceeded;
    }
    bool sound(int id,int priority,int loop,int cut){
        world.run_.soundPlay(id,priority,loop!=0,cut!=0);
        return world.run_.effectsSucceeded;
    }
    bool smoke(Real x,Real y,int count,int vary){
        downwell::Fixed16 qx,qy;
        return count==1&&vary==0&&q16(x,qx)&&q16(y,qy)&&world.emitBoxSmoke(qx,qy);
    }
    bool flash(Real x,Real y,int count,int vary,int radius){
        downwell::Fixed16 qx,qy;
        return count==1&&vary==0&&radius==2&&q16(x,qx)&&q16(y,qy)&&world.emitBoxFlash(qx,qy);
    }
    bool effect(Real x,Real y,int sprite,Real speed,Real angle,int){
        downwell::Fixed16 qx,qy,qa;downwell::RunnerImageValue r;
        return (q16(x,qx)&&q16(y,qy)&&r.assign(speed)&&q16(angle,qa)&&
            world.compactEffects_.spawnNoLoop(qx,qy,sprite,r,qa))||fail();
    }
    // global.ug[5] (Gunpowder Blocks): bulletDrone 266 at the box, then
    // Alarm 0 = 5 on each boxed neighbour 16 pixels away.
    bool chainReaction(){return world.run_.upgrades.levels[5]!=0;}
    GameplayWorld::BulletState* shot=nullptr;
    bool create(int object,Real x,Real y,Handle& child){
        downwell::Fixed16 qx,qy;
        if(object==370)return q16(x,qx)&&q16(y,qy)&&world.spawnCompactDebris(qx,qy,child);
        std::int32_t px,py;
        if(object!=266||!x.roundPixel(px)||!y.roundPixel(py))return fail();
        shot=world.createDroneBullet(px,py,270,4);
        child=shot?shot->lifetimeId:0;
        return shot!=nullptr;
    }
    bool myBullet(Handle h){return shot&&shot->lifetimeId==h;}
    bool bulletDirection(Handle h,int degrees){
        if(!myBullet(h))return fail();
        shot->shot.direction=downwell::RangedShot::degreesToTurns(degrees);return true;
    }
    bool bulletDamage(Handle h,int damage){if(!myBullet(h))return fail();shot->damage=damage;return true;}
    // instance_place(x, y, objBox_n) with the caller's 16-pixel mask.
    bool place(int object,Real x,Real y,Handle& target,bool& found){
        std::int32_t px,py;
        if(object!=92||!x.roundPixel(px)||!y.roundPixel(py))return fail();
        found=false;target=0;
        for(unsigned i=0;i<world.breakableCount_;++i){const auto& b=world.breakables_[i];
            if(b.active&&!b.destroyRequested&&i!=key.slot&&b.x+240>px-16&&b.x+240<px+16&&b.y+176>py-16&&b.y+176<py+16){
                found=true;target=i+1;break;
            }
        }
        return true;
    }
    bool target(Handle){return true;}
    bool targetAlarm(Handle h,int alarm,int value){
        if(!h||alarm!=0||h>world.breakableCount_)return fail();
        const auto& b=world.breakables_[h-1];
        return world.publishBoxAlarm0({h-1,b.lifetimeId},value);
    }
    bool targetChecked(Handle,bool){return fail();} // autoTile is never set
    bool destroySelf(){return world.markBreakableDestroyed(key);}
    bool lowSpec(){return false;}
    int area(){return world.run_.stage.area;}
    // scrCurrencySpawn at the box: objBox_n is not a subparentEnemy child.
    bool currency(int amount){
        const auto* box=world.findBreakable(key);
        return box?world.spawnCompactCurrency(amount,box->x,box->y,false)>=0:fail();
    }
    bool alarmStop(int alarm){return alarm==0;} // No pTimeStop in this world.
};

bool GameplayWorld::stepCompactBox(unsigned index){
    const downwell::InstanceHandle key{index,breakables_[index].lifetimeId};
    BoxOps ops{*this,key};
    bool good=run_.effectsSucceeded;
    downwell::BoxDestructionScript::runRetained([&](downwell::BoxDestructionScript::Caller& caller){
        const auto* box=findBreakable(key);
        if(!box)return false;
        caller.state=box->state;
        caller.x=downwell::MotionBinary64::fromInteger(box->x+240);
        caller.y=downwell::MotionBinary64::fromInteger(box->y+176);
        return true;
    },ops,good);
    return run_.effectsSucceeded=good&&run_.effectsSucceeded;
}

// ---------------------------------------------------------------------------
// W1.4 compact guns: bStatInitLevel1's seven guns, bulletRanged/bulletLaser
// shots, bulletCasing, fxLaserTrail, GunModule, moduleText and itemNotif.
// Room coordinates are offset by (240,176) as for the other compact owners.
// The shot, pickup and letter sounds go through the run's SoundRequests
// (W2.3); wall-hit, blood and impact effects are W1.6; upgrades 1 (Reverse Engineering) and 8 (Hot
// Casing) are W1.9.

namespace {
using GunReal=downwell::MotionBinary64;
}

int GameplayWorld::breakableAt(int x,int y)const{
    if(breakableIndexReady_){
        const int block=indexedBreakable(x,y);
        return block>=0&&breakables_[block].active&&!breakables_[block].destroyRequested?block:-1;
    }
    for(unsigned i=0;i<breakableCount_;++i){const auto& b=breakables_[i];
        if(b.active&&!b.destroyRequested&&x>=b.x-8&&x<b.x+8&&y>=b.y-8&&y<b.y+8)return int(i);}
    return -1;
}

// soundFootstep/soundLand's instance_place(x, y + 2, sParentSolid).material
// as the material index, probing the mask's bottom row two pixels down
// (centre first): shop furniture is metal and the keeper's invisible wall
// rock, boxes breakable, gem chunks rock,
// thin platforms thin, walls foliage. Audio only: it picks the sample.
int GameplayWorld::playerFloorMaterial()const{
    const int px=player_.offsetX(),y=player_.offsetY()+9;
    const int probes[]={px,px-4,px+3};
    for(const int x:probes){
        if(const int shop=run_.shops.wallMaterialAt(x+240,y+176,true);shop>=0)return shop;
        if(breakableAt(x,y)>=0)return 0;
        if(gemChunkBlocks(x+240,y+176))return 3;
        if(solidResolver_&&solidResolver_(solidResolverContext_,x,y))return 2;
    }
    return 1; // Landed with no wall below: a thin platform.
}

GameplayWorld::BulletState* GameplayWorld::birthBullet(int x,int y,int sprite){
    // Marked records remain resolvable until Draw cleanup; never compact
    // them away merely because gameplay/rendering has stopped using them.
    unsigned index=0;
    for(unsigned read=0;read<bulletSlotEnd_;++read)if(bullets_[read].identitySlot<bulletCapacity){
        if(index!=read)bullets_[index]=bullets_[read];++index;
    }
    bulletSlotEnd_=index;
    if(index>=bulletCapacity||!nextBulletLifetimeId_){run_.effectsSucceeded=false;return nullptr;}
    BulletState& bullet=bullets_[index];
    unsigned identity=0;while(identity<bulletCapacity&&bulletIdentitySlots_[identity])++identity;
    if(identity==bulletCapacity){run_.effectsSucceeded=false;return nullptr;}
    bullet={};bullet.identitySlot=identity;bulletIdentitySlots_[identity]=true;
    bullet.lifetimeId=nextBulletLifetimeId_++;
    bullet.active=true;
    bulletSlotEnd_=index+1;
    bullet.x=x;bullet.y=y;bullet.gemHigh=run_.gemHigh;bullet.sprite=sprite;
    bullet.trajectory.reset(x,y,false);
    const downwell::InstanceHandle handle{identity,bullet.lifetimeId};
    if(!bullet.sourceMotion.reset(GunReal::fromInteger(x+240),GunReal::fromInteger(y+176),false)||
       !bulletLifecycle_.birth(handle,bullet)||!bulletLifecycle_.sprite(handle,bullet,sprite)){
        run_.effectsSucceeded=false;return nullptr;
    }
    return &bullet;
}

// instance_create(x, y, global.pBulObject); myBullet.bDir = angle.
bool GameplayWorld::createCompactBullet(int x,int y,int angle,bool laserSight){
    const auto& gun=currentGun();
    BulletState* bullet=birthBullet(x,y,gun.sprite);
    if(!bullet)return false;
    bullet->damage=gun.damage;
    if(gun.object==260){
        // bulletLaser Create: x/y rounded, xxx = x + irandom_range(-2,2).
        bullet->kind=BulletState::laser;
        (void)enemyRandom_.irandomSmall(4);
        bullet->shot={};
        bullet->shot.x={x,0};bullet->shot.y={y,0};
        bullet->shot.direction=downwell::RangedShot::degreesToTurns(angle);
        bullet->shot.imageSpeed={0x3f800000u};bullet->shot.animation=true;
        if(angle!=270)return run_.effectsSucceeded=false; // aimLimit and accuracy are zero
        return true;
    }
    bullet->kind=BulletState::ranged;
    if(!bullet->shot.create(enemyRandom_,x,y,angle,gun,laserSight&&run_.upgrades.levels[12]!=0))
        return run_.effectsSucceeded=false;
    return true;
}

// scrPlayerEmitBullet(muzzlex, muzzley, angle): shootLeg flips per call; the
// muzzle effect sits at the player's y + 12. Laser Sight's bdirRand = 0 is
// written to scrPlayerShootN's myBullet: the last bullet of its first call.
bool GameplayWorld::emitPlayerBullets(int muzzleX,int muzzleY,int playerY,int angle,bool lastGetsLaserSight){
    const auto& gun=currentGun();
    const int emitX=muzzleX+emitter_.nextOffset();
    if(!compactEffects_.spawnNoLoop(downwell::Fixed16::fromInteger(emitX+240),downwell::Fixed16::fromInteger(playerY+12+176),
            gun.muzzle,half,downwell::Fixed16::fromInteger(angle)))
        return run_.effectsSucceeded=false;
    if(!createCompactBullet(emitX,muzzleY,angle,false))return false;
    for(int i=1;i<=gun.sp1&&gun.spType;++i){
        const int spread=downwell::muls16(gun.sp2,i); // gunTable: sp1 <= 2, sp2 <= 14
        if(gun.spType==1){
            if(!createCompactBullet(emitX,muzzleY,angle+spread,false)||
               !createCompactBullet(emitX,muzzleY,angle-spread,false))return false;
        }else if(!createCompactBullet(emitX+spread,muzzleY-2,angle,false)||
                 !createCompactBullet(emitX-spread,muzzleY-2,angle,false))return false;
    }
    if(lastGetsLaserSight&&run_.upgrades.levels[12]!=0){
        BulletState& last=bullets_[bulletSlotEnd_-1];
        last.shot.zeroSpread=true;
        // bulletRanged Create already lengthened decTimer for every bullet.
    }
    return true;
}

// scrPDamage after the damage number: soundPlay(5, 90, 0, 0). Its
// bgmDuck(500, 0.7) is music (WM).
bool GameplayWorld::playPlayerHurt(){
    run_.soundPlay(5,90,false,false);
    return run_.effectsSucceeded;
}

// scrTypicalDamage's pugTime block (Timeout): a parentTimeField at the
// player's position, unless global.pTimeStop is already on.
bool GameplayWorld::compactTimeoutField(){
    if(!run_.upgrades.levels[17]||timeStop_.player.stopped)return run_.effectsSucceeded;
    return run_.createTimeField(240+int(downwell::roundGameMakerQ8(player_.offsetXFixed())),
        176+int(downwell::roundGameMakerQ8(player_.offsetYFixed())),enemyRandom_);
}

// soundPlayOL from a caller at room y against the published view (top,
// height 284): enemymove and waterEnemy calls outside it are silent.
void GameplayWorld::soundPlayOLAt(int sound,int priority,bool loop,bool cut,const char* channel,int roomY){
    auto top=downwell::MotionBinary64::fromInteger(boneViewTopQ8_);
    if(!top.zero())top.high-=8u<<20;
    run_.soundPlayOL(sound,priority,loop,cut,channel,downwell::MotionBinary64::fromInteger(roomY),
        top,downwell::MotionBinary64::fromInteger(284));
}
void GameplayWorld::playEnemySound(int sound,int roomY){soundPlayOLAt(sound,50,false,true,"enemymove",roomY);}

// scrShotSound: the gun's bStatSound on the gunshot channel; at half charge
// or less (stammo before this shot) the low-ammo blip, and the gun asset's
// pitch rises by 0.8 (area 1). The gunshot channel's bgmDuck is music (WM).
void GameplayWorld::playShotSound(int ammoBefore){
    const int sound=currentGun().sound;
    run_.soundPlayOL(sound,80,false,true,"gunshot");
    auto pitch=downwell::audio_level::one; // audio_sound_get_pitch(sndsnd), dry
    if(ammoBefore*2<=player_.ammoCapacity()){
        pitch=downwell::AudioLevel::raw(117965); // 1 + 0.8
        run_.soundPlayOL(90,85,false,true,"UI");
    }
    run_.setSoundPitch(sound,pitch);
}

// scrPlayerShootN's charged branch after PlayerMotion's recoil and charge:
// shake, bullets, bulletCasing, then the burst.
bool GameplayWorld::firePlayerGun(int playerX,int playerY,int muzzleX,int muzzleY){
    const auto& gun=currentGun();
    shotAngle_=270+aimAngle_;
    if(followCamera_&&!requestCameraShake(gun.shake,gun.shakeDuration))return false;
    if(!emitPlayerBullets(muzzleX,muzzleY,playerY,shotAngle_,true))return false;
    auto* casing=downwell::CompactGuns::append(guns_.casings,guns_.casingEnd);
    if(!casing)return run_.effectsSucceeded=false;
    casing->create(enemyRandom_,playerX+240,playerY+176,player_.facing());casing->birth=tick_;
    if(run_.upgrades.levels[8])casing->makeHot(run_.upgrades.levels[8]);
    if(gun.burst){
        if(gun.burstRate>0){
            if(int(burstCount_)<gun.burstAmount){burstAlarm_=gun.burstRate;++burstCount_;}
            else burstCount_=0;
        }else for(int i=0;i<gun.burstAmount;++i)
            if(!emitPlayerBullets(muzzleX,muzzleY,playerY,shotAngle_,false))return false;
    }
    return run_.effectsSucceeded;
}

// objPlayer_n Alarm 3: scrBurstFire at the published position.
bool GameplayWorld::burstFire(){
    const auto& gun=currentGun();
    const int x=int(downwell::roundGameMakerQ8(player_.offsetXFixed()));
    const int y=int(downwell::roundGameMakerQ8(player_.offsetYFixed()));
    if(!emitPlayerBullets(x,y+4,y,shotAngle_,false))return false;
    firedThisStep_=true; // scrBurstFire: pFired = 1
    if(followCamera_&&!requestCameraShake(gun.shake,gun.shakeDuration))return false;
    playShotSound(player_.ammo()); // scrShotSound: stammo is already spent
    player_.setVerticalSpeedFifths(gun.recoilFifths);
    auto* casing=downwell::CompactGuns::append(guns_.casings,guns_.casingEnd);
    if(!casing)return run_.effectsSucceeded=false;
    casing->create(enemyRandom_,x+240,y+176,player_.facing());casing->birth=tick_;
    if(run_.upgrades.levels[8])casing->makeHot(run_.upgrades.levels[8]);
    if(int(burstCount_)<gun.burstAmount){burstAlarm_=gun.burstRate;++burstCount_;}
    else burstCount_=0;
    return run_.effectsSucceeded;
}

bool GameplayWorld::advanceBulletAnimation(BulletState& bullet){
    const downwell::InstanceHandle key{bullet.identitySlot,bullet.lifetimeId};
    if(bullet.kind==BulletState::legacy)
        return bullet.animation.advance()&&bulletLifecycle_.imageIndex(key,bullet,bullet.animation.index);
    bool ended=false;
    if(!bullet.shot.advanceAnimation(bullet.kind==BulletState::laser?13:6,ended)||
       !bulletLifecycle_.imageIndex(key,bullet,bullet.shot.imageIndex))return false;
    // bulletRanged Other 7 (Animation End): action_kill_object.
    if(ended&&bullet.kind==BulletState::ranged&&!bullet.destroyRequested){
        if(bulletLifecycle_.hasOwner())return markBulletDestroyed(key);
        return retireBulletOwnership(bullet);
    }
    return true;
}

bool GameplayWorld::bulletEnemyContact(BulletState& bullet,const downwell::ProjectileSegment& path,bool& consumed){
    DW_PROFILE_SCOPE(profileBulletEnemyQuery);
    consumed=false;
    for(unsigned enemyIndex=0;enemyIndex<enemyCount_;++enemyIndex){
        EnemyState& enemy=enemies_[enemyIndex];
        const downwell::ContactActor contact{
            enemy.x*256L,enemy.y*256L,
            enemy.objectId==117?enemy.motionState:enemy.facing,
            enemy.imageScaleY(),
            downwell::enemyContactMask(enemy.objectId,enemy.spriteId,enemy.maskId)};
        if(!enemy.active||enemy.health<=0||!path.intersects(contact))continue;
        downwell::RunnerImageValue bx,by;
        if(!downwell::ProjectilePublication::coordinate(path.x,240,bx)||
           !downwell::ProjectilePublication::coordinate(path.y,176,by))return run_.effectsSucceeded=false;
        return typicalBulletDamage(enemy,bullet.damage,bulletDirection(bullet),bx.promoted(),by.promoted(),consumed);
    }
    return true;
}

// scrTypicalBulletDamage from a shot (or a Hot Casing) at (x, y) room pixels.
bool GameplayWorld::typicalBulletDamage(EnemyState& enemy,int damage,std::uint32_t direction,
    downwell::MotionBinary64 x,downwell::MotionBinary64 y,bool& consumed){
    consumed=true;
    // soundPlay(29, 90, 0, 1) after the damage (a compact death runs inside
    // the damage below, so it plays first).
    const bool immune=enemy.objectId==112||enemy.objectId==114; // bulletImmune
    if(!immune)run_.soundPlay(29,90,false,true);
    if(steeringObject(enemy.objectId))
        return hitSteeringEnemy(enemy,direction,damage)||(run_.effectsSucceeded=false);
    // scrBulletImpact precedes the (here immediate) death's scrDeadBody.
    if(!immune)enemy.takenImpactDamage=damage;
    const auto impact=downwell::applyBulletDamage(
        enemy,run_.kills,enemyRandom_,
        [this,&enemy]{run_.combo.state.enemyDefeated();return compactEnemyDeath(enemy,false);},
        damage,direction,
        [this](const EnemyState& record){return publishEnemyActivation(record,false);});
    if(impact.kind==downwell::BulletImpactKind::failed)return run_.effectsSucceeded=false;
    if(impact.kind==downwell::BulletImpactKind::immune){
        if(!compactImmuneSpark(x,y))return false;
        run_.soundPlay(impact.sound,50,false,true); // choose(30, 31)
    }
    if((impact.kind==downwell::BulletImpactKind::damaged||impact.kind==downwell::BulletImpactKind::defeated)&&
       !compactBulletImpact(enemy,x,y,direction,damage,false))return false;
    consumed=impact.kind!=downwell::BulletImpactKind::ignored;
    return run_.effectsSucceeded;
}

bool GameplayWorld::bulletSolidContact(BulletState& bullet,const downwell::ProjectileSegment& path,bool& consumed){
    DW_PROFILE_SCOPE(profileBulletSolidQuery);
    consumed=false;
    unsigned firstBlock=0,endBlock=breakableCount_;
    if(breakableIndexReady_){
        // Every 16-pixel tile the segment's bounding box spans, including
        // corner-adjacent tiles containing neither endpoint. Faster guns
        // (shotgun 18) cross more than the usual gun's two.
        int candidate=-1;
        const int left=path.x.pixel<path.endX.pixel?path.x.pixel:path.endX.pixel;
        const int right=path.x.pixel>path.endX.pixel?path.x.pixel:path.endX.pixel;
        const int top=path.y.pixel<path.endY.pixel?path.y.pixel:path.endY.pixel;
        const int bottom=path.y.pixel>path.endY.pixel?path.y.pixel:path.endY.pixel;
        for(int column=(left+8)>>4;column<=((right+8)>>4);++column)
          for(int row=(top+8)>>4;row<=((bottom+8)>>4);++row){
            const int next=indexedBreakable(column*16,row*16);
            if(next<0)continue;
            const auto& block=breakables_[next];
            if(block.active&&!block.destroyRequested&&path.intersects(block.x-8,block.x+8,block.y-8,block.y+8)&&
               (candidate<0||next<candidate))candidate=next;
        }
        firstBlock=candidate<0?0:unsigned(candidate);
        endBlock=candidate<0?0:firstBlock+1;
    }
    for(unsigned blockIndex=firstBlock;blockIndex<endBlock;++blockIndex){
        Breakable& block=breakables_[blockIndex];
        // Indexed selection already proved this exact hit.
        if(block.active&&!block.destroyRequested&&(breakableIndexReady_||path.intersects(block.x-8,block.x+8,
                                                   block.y-8,block.y+8))){
            bool applied=false;
            if(!block.state.bulletHit(GunReal::fromInteger(bullet.damage),applied))return run_.effectsSucceeded=false;
            observeBoxHealth(block);
            consumed=applied;return true;
        }
    }
    if(scenery_.gemChunkEnd){
        bool hit=false;
        if(!hitGemChunk(path,bullet.damage,hit,consumed))return false;
        if(hit)return true;
    }
    // An ordinary, non-wave bullet also stops at unbreakable walls.
    if(segmentSolid(path)){consumed=true;return compactBulletWallHit(path);}
    // W2.6: shopTable and objInvisWall are subparentSolidWall too.
    if(const int material=shopWallOn(path,false);material>=0){consumed=true;return compactBulletWallHit(path,material);}
    if((!scenery_.empty()||guns_.moduleEnd)&&!hitCompactShootable(path,bullet,consumed))return false;
    return true;
}

// bulletRanged Step: first-Step spread, deceleration, lengthdir, then
// scrBulCheckSolid (here after the retained enemy-first contact) and motion.
bool GameplayWorld::stepRangedBullet(BulletState& bullet,long generationCameraY){
    const bool first=!bullet.shot.allSet;
    bullet.shot.prepareStep(enemyRandom_);
    const downwell::InstanceHandle h{bullet.identitySlot,bullet.lifetimeId};
    if(first&&bulletLifecycle_.hasOwner()){
        downwell::RunnerImageValue angle;
        if(!downwell::ProjectilePublication::angle(bullet.shot.direction,angle)||!publishBulletAngle(h,angle))
            return run_.effectsSucceeded=false;
    }
    const downwell::ProjectileSegment path(bullet.shot.x,bullet.shot.y,bullet.shot.velocityX,bullet.shot.velocityY);
    bool consumed=false;
    if(!bulletEnemyContact(bullet,path,consumed))return false;
    if(!consumed&&!bulletSolidContact(bullet,path,consumed))return false;
    bullet.shot.advance();
    bullet.x=bullet.shot.x.pixel;bullet.y=bullet.shot.y.pixel;
    if(bulletLifecycle_.hasOwner()){
        DW_PROFILE_SCOPE(profileBulletPublication);
        downwell::RunnerImageValue x,y;
        if(!downwell::ProjectilePublication::coordinate(bullet.shot.x,240,x)||
           !downwell::ProjectilePublication::coordinate(bullet.shot.y,176,y)||
           !publishBulletCollisionPosition(h,x,y))return run_.effectsSucceeded=false;
    }
    const long cameraCentreQ12=176L*4096+generationCameraY*16;
    const bool outside=!registeredOrdinaryEndStep_&&
        downwell::fractionalBulletOutsideView(bullet.shot.y,176,cameraCentreQ12-142L*4096,284);
    if(!consumed&&!outside)return true;
    if(bulletLifecycle_.hasOwner())return markBulletDestroyed(h);
    return retireBulletOwnership(bullet);
}

// bulletLaser Step: the instant ray. Laser aim and spread are always zero,
// so the ray is vertical: d2wx stays 0 and xadd's halving is inert.
bool GameplayWorld::stepLaserBullet(BulletState& bullet,long generationCameraY){
    auto& shot=bullet.shot;
    if(!shot.allSet){(void)enemyRandom_.nextU32();shot.allSet=true;} // random_range(-0,0)
    const int x=shot.x.pixel,y=shot.y.pixel;
    const long top=(34L*256+generationCameraY)/256-176;
    const long bottom=top+284;
    const auto enemyAt=[&](const downwell::ContactBounds& box){
        for(unsigned i=0;i<enemyCount_;++i){const auto& e=enemies_[i];
            if(!e.active||e.health<=0)continue;
            const auto b=downwell::contactWorldBounds({e.x*256L,e.y*256L,e.objectId==117?e.motionState:e.facing,
                e.imageScaleY(),downwell::enemyContactMask(e.objectId,e.spriteId,e.maskId)});
            if(downwell::contactBoundsOverlap(box,b))return true;
        }
        return false;
    };
    const auto solidPoint=[&](int px,int py){
        if(solidResolver_&&solidResolver_(solidResolverContext_,px,py))return true;
        return breakableAt(px,py)>=0||gemChunkBlocks(px+240,py+176)||
            run_.shops.wallMaterialAt(px+240,py+176,true)>=0; // W2.6: every shop child is parentWall
    };
    const auto shootableAt=[&](const downwell::ContactBounds& box){
        for(unsigned i=0;i<guns_.moduleEnd;++i){const auto& m=guns_.modules[i];
            if(m.live&&m.objHp>0&&downwell::contactBoundsOverlap(box,downwell::contactPixelBounds(m.x-240,m.y-176,1,1,m.sprite())))return true;}
        for(unsigned i=0;i<scenery_.scatteredEnd;++i){const auto& r=scenery_.scattered[i];
            if(r.live&&r.objHp>0&&downwell::contactBoundsOverlap(box,downwell::contactPixelBounds(r.x-240,r.y-176,r.scaleX,1,550)))return true;}
        return false;
    };
    const auto mask=[&](int py){return downwell::contactPixelBounds(x,py,1,1,585);};
    const auto lineHits=[&](int to){
        for(int py=y;py<=to;++py)if(solidPoint(x,py))return true;
        return false;
    };
    int distance=0,add=8;
    for(unsigned guard=0;guard<512;++guard){
        const int ahead=distance+add;
        if(enemyAt({x-8,x+9,y,y+ahead+1})||lineHits(y+ahead)||shootableAt({x,x+1,y,y+ahead+1})){
            add/=2;if(add<1)add=1;
        }
        distance+=add;
        if(enemyAt(mask(y+distance))||solidPoint(x,y+distance)||shootableAt(mask(y+distance)))break;
        if(y+distance>bottom||y+distance<top)break;
    }
    // Trails every 8 pixels from the start; the first Step's index falls
    // from round(d/20)+2 (8 beyond 120) by 0.25, later Steps keep 4.
    int quarterIndex=4*4;
    if(!bullet.wallMet){
        if(distance<120){
            int whole=std::uint16_t(distance)/20;const int remainder=std::uint16_t(distance)%20; // 0 <= distance < 120
            if(remainder>10||(remainder==10&&(whole&1)))++whole; // round(): ties to even
            quarterIndex=(whole+2)*4;
        }else quarterIndex=8*4;
    }
    for(int trail=0;;trail+=8){
        auto* record=downwell::CompactGuns::append(guns_.trails,guns_.trailEnd);
        if(record){
            record->x=x+240;record->y=y+trail+176;record->sprite=bullet.sprite;record->birth=tick_;
            GunReal value=GunReal::fromInteger(quarterIndex);
            if(quarterIndex)value.high-=2u<<20; // exact division by four
            if(!record->index.assign(value))return run_.effectsSucceeded=false;
        } // A full trail store drops only presentation (no RNG or state).
        if(!bullet.wallMet&&quarterIndex>0)--quarterIndex;
        if(trail+8>distance)break;
    }
    bullet.wallMet=true;
    shot.y.pixel+=distance;
    bullet.x=x;bullet.y=shot.y.pixel;
    const downwell::InstanceHandle h{bullet.identitySlot,bullet.lifetimeId};
    bool destroy=false;
    const int py=shot.y.pixel;
    // position_meeting(x, y, parentWall): a box or gem chunk takes bdmg each
    // Step and the piercing laser stays; an ordinary wall ends it.
    if(solidPoint(x,py)&&!enemyAt({x,x+1,py,py+1})){
        const int block=breakableAt(x,py);
        if(block>=0){
            auto& box=breakables_[block];
            if(box.state.wallHp.high||box.state.wallHp.low){
                bool applied=false;
                if(!box.state.bulletHit(GunReal::fromInteger(bullet.damage),applied))return run_.effectsSucceeded=false;
                observeBoxHealth(box);
            }
        }else if(gemChunkBlocks(x+240,py+176)){
            bool hit=false,consumed=false;
            const downwell::ProjectileSegment point(shot.x,shot.y,0,0);
            if(!hitGemChunk(point,bullet.damage,hit,consumed))return false;
        }else{
            // scrEffectSpawn(x, y, hitWallFx 96, 0.5, 0, -10000),
            // scrSmokefx(x, y, 1, 0), then soundPlay(15, 30, 0, 1).
            destroy=true;
            const auto wx=downwell::MotionBinary64::fromInteger(x+240),wy=downwell::MotionBinary64::fromInteger(py+176);
            if(!compactWallEffect(wx,wy,96,-10000))return false;
            if(!emitBoxSmoke(downwell::Fixed16::fromInteger(x+240),downwell::Fixed16::fromInteger(py+176)))
                return run_.effectsSucceeded=false;
            run_.soundPlay(15,30,false,true);
        }
    }else{
        const auto box=mask(py);
        for(unsigned i=0;i<guns_.moduleEnd;++i){auto& m=guns_.modules[i];
            if(m.live&&m.objHp>0&&downwell::contactBoundsOverlap(box,downwell::contactPixelBounds(m.x-240,m.y-176,1,1,m.sprite()))){
                m.objHp-=bullet.damage;break;}}
    }
    // Collision with subparentEnemy, once per enemy (hitIgnoreCheck).
    if(!destroy)for(unsigned i=0;i<enemyCount_&&!destroy;++i){
        EnemyState& enemy=enemies_[i];
        if(!enemy.active||enemy.health<=0)continue;
        const auto b=downwell::contactWorldBounds({enemy.x*256L,enemy.y*256L,enemy.objectId==117?enemy.motionState:enemy.facing,
            enemy.imageScaleY(),downwell::enemyContactMask(enemy.objectId,enemy.spriteId,enemy.maskId)});
        if(!downwell::contactBoundsOverlap(mask(py),b))continue;
        downwell::RunnerImageValue lx,ly;
        if(!downwell::ProjectilePublication::coordinate(shot.x,240,lx)||
           !downwell::ProjectilePublication::coordinate(shot.y,176,ly))return run_.effectsSucceeded=false;
        if(enemy.objectId==112||enemy.objectId==114){ // bulletImmune: the 112 spark, then choose(30,31).
            destroy=true;if(!compactImmuneSpark(lx.promoted(),ly.promoted()))return false;
            run_.soundPlay(30+int(enemyRandom_.chooseIndexSmall(2)),50,false,true);break;
        }
        bool ignored=false;
        for(unsigned k=0;k<bullet.laserHitCount;++k)if(bullet.laserHits[k]==enemy.lifetimeId)ignored=true;
        if(ignored)continue;
        if(bullet.laserHitCount<4)bullet.laserHits[bullet.laserHitCount++]=enemy.lifetimeId;
        if(enemy.health>bullet.damage)destroy=true;
        if(steeringObject(enemy.objectId)){
            run_.soundPlay(16,50,false,true); // sndEnemyhurt
            if(!hitSteeringEnemy(enemy,shot.direction,bullet.damage))return false;
            continue;
        }
        // scrBloodfx, scrDamageEnemy, scrFlashballfx(x,y,1,0,2), soundPlay(16),
        // scrBulletImpact: the hit precedes the (here immediate) death.
        if(!compactBulletImpact(enemy,lx.promoted(),ly.promoted(),shot.direction,bullet.damage,true))return false;
        const auto impact=downwell::applyBulletDamage(enemy,run_.kills,enemyRandom_,
            [this,&enemy]{run_.combo.state.enemyDefeated();return compactEnemyDeath(enemy,false);},
            bullet.damage,shot.direction,[this](const EnemyState& record){return publishEnemyActivation(record,false);});
        if(impact.kind==downwell::BulletImpactKind::failed)return run_.effectsSucceeded=false;
    }
    if(bulletLifecycle_.hasOwner()){
        downwell::RunnerImageValue px,pyv;
        if(!downwell::ProjectilePublication::coordinate(shot.x,240,px)||
           !downwell::ProjectilePublication::coordinate(shot.y,176,pyv)||
           !publishBulletCollisionPosition(h,px,pyv))return run_.effectsSucceeded=false;
    }
    const long cameraCentreQ12=176L*4096+generationCameraY*16;
    if(!destroy&&!registeredOrdinaryEndStep_&&
       downwell::fractionalBulletOutsideView(shot.y,176,cameraCentreQ12-142L*4096,284))destroy=true;
    if(!destroy)return true;
    if(bulletLifecycle_.hasOwner())return markBulletDestroyed(h);
    return retireBulletOwnership(bullet);
}

bool GameplayWorld::createGunModule(const downwell::DeferredCreateInstance& command){
    if(!run_.effectsSucceeded)return false;
    auto* m=downwell::CompactGuns::append(guns_.modules,guns_.moduleEnd);
    if(!m)return run_.effectsSucceeded=false;
    m->x=command.x;m->y=command.y;m->yy=long(command.y)<<16;m->birth=tick_;
    m->type=command.gunModuleType();m->gun=command.gunModuleGun();
    // global.area == 5 forces hearts; world one never reaches it.
    if(!downwell::validGun(m->gun))return run_.effectsSucceeded=false;
    return true;
}

// Alarm phase, after the room's own alarms (none of these draw RNG except
// objPlayer_n Alarm 3, the first object at index 3).
bool GameplayWorld::advanceCompactGunAlarms(){
    for(unsigned index=0;index<4;++index){
        if(index==0)for(unsigned i=0;i<bulletSlotEnd_;++i){auto& b=bullets_[i];
            if(b.active&&!b.destroyRequested&&b.kind==BulletState::ranged)b.shot.alarm();}
        if(index==3&&burstAlarm_>0&&--burstAlarm_==0){burstAlarm_=-1;if(!burstFire())return false;}
        for(unsigned i=0;i<guns_.moduleEnd;++i)if(guns_.modules[i].live)guns_.modules[i].alarmEvent(index);
        if(index<2)for(unsigned i=0;i<guns_.casingEnd;++i)if(guns_.casings[i].live)(void)guns_.casings[i].alarmEvent(index);
        for(unsigned i=0;i<guns_.textEnd;++i)if(guns_.texts[i].live){
            auto& t=guns_.texts[i];const int shown=t.shown;
            (void)t.alarmEvent(index);
            if(t.shown!=shown)run_.soundPlay(334,80,false,true); // Alarm 0: the next letter
        }
        if(index<2)for(unsigned i=0;i<guns_.noticeEnd;++i)if(guns_.notices[i].live)(void)guns_.notices[i].alarmEvent(index);
    }
    return run_.effectsSucceeded;
}

bool GameplayWorld::advanceCompactGunAnimation(){
    for(unsigned i=0;i<guns_.casingEnd;++i)if(guns_.casings[i].live)guns_.casings[i].advanceAnimation();
    for(unsigned i=0;i<guns_.trailEnd;++i){auto& t=guns_.trails[i];
        if(t.live&&t.birth!=tick_+1&&!t.advanceAnimation())return run_.effectsSucceeded=false;}
    return true;
}

// GunModule Step: gravity above the view bottom, scrCheckCollisionWith(56)
// (xsp is always 0 here: only the vertical branch can act), roundPosition.
bool GameplayWorld::stepGunModules(){
    const long top=boneViewTopQ8_/256;
    for(unsigned i=0;i<guns_.moduleEnd;++i){
        auto& m=guns_.modules[i];
        if(!m.live||m.birth==tick_)continue;
        if(m.y<top+284)m.ysp+=13107; // global.grav 0.2
        const long rounded=(m.yy+(1L<<15))>>16;
        if(m.ysp){
            const int sign=m.ysp>0?1:-1;
            const long step=((m.ysp>0?m.ysp:-m.ysp)+65535)>>16;
            bool hit=false;
            if(!solidMeeting(m.sprite(),GunReal::fromInteger(m.x),GunReal::fromInteger(int(rounded+(sign>0?step:-step))),true,hit))return false;
            if(hit){
                long move=0;const long limit=m.ysp>0?m.ysp:-m.ysp;
                for(;;){
                    bool next=false;
                    if(!solidMeeting(m.sprite(),GunReal::fromInteger(m.x),GunReal::fromInteger(int(rounded+move+sign)),true,next))return false;
                    if(next)break;
                    move+=sign;
                    if((move<0?-move:move)<<16>=limit){move=0;break;}
                }
                m.yy=(rounded+move)<<16;
                m.ysp=0;
            }
        }
        m.yy+=m.ysp;
        m.y=int((m.yy+(1L<<15))>>16);
        if(run_.upgrades.levels[1]&&m.objHp<=0&&!reverseEngineerModule(m))return false;
    }
    return true;
}

// GunModule Step's pugEngineer block (Reverse Engineering): a shot-down
// module puffs (scrSmokefx 1, scrFlashballfx 1) at its position, hops (ysp
// -2), is unobtainable for 30 Steps, and rerolls its type (choose(0, 1)) and
// gun (irandom(bulletMaxNum) until it differs). objHp 9999 keeps it whole.
bool GameplayWorld::reverseEngineerModule(downwell::CompactGunModuleRecord& m){
    const auto x=downwell::Fixed16::fromInteger(m.x),y=downwell::Fixed16::raw(std::int32_t(m.yy));
    if(!emitBoxSmoke(x,y)||!run_.emitFlash<1,0,0>(x,y,enemyRandom_))return false;
    m.obtainable=false;m.alarm0=30;m.ysp=-(2L<<16);
    m.type=int(enemyRandom_.chooseIndexSmall(2));
    int gun;
    do gun=int(enemyRandom_.irandomSmall(downwell::gunMaximumNumber));while(gun==m.gun);
    m.gun=gun;m.objHp=9999;
    return run_.effectsSucceeded;
}

// bulletCasing Step: flash, gravity, wall bounce, scrBulCheckSolid, motion.
bool GameplayWorld::stepCasings(){
    for(unsigned i=0;i<guns_.casingEnd;++i){
        auto& c=guns_.casings[i];
        if(!c.live||c.birth==tick_)continue;
        c.beginStep();
        bool wall=false;
        if(!solidMeeting(c.sprite(),GunReal::fromInteger(int((c.x+c.xsp+(1L<<15))>>16)),
                         GunReal::fromInteger(int((c.y+(1L<<15))>>16)),false,wall))return false;
        if(wall)c.xsp=-(c.xsp*7)/10;
        if(c.xsp>-3277&&c.xsp<3277)c.xsp=c.xsp>0?3277:c.xsp<0?-3277:0; // 0.05
        // scrBulCheckSolid with bdmg 0: any wall or shootable on the line
        // ends the casing; a wall shows the parentPBullet hitWallFx 101.
        const auto q20=[](long q16){
            downwell::RangedShot::Coordinate c{int(q16>>16),std::uint32_t(q16&65535)<<4};return c;};
        const downwell::ProjectileSegment path(q20(c.x-(240L<<16)),q20(c.y-(176L<<16)),
            std::int32_t(c.xsp*16),std::int32_t(c.ysp*16));
        bool hit=false;
        hit=segmentSolid(path);
        if(hit&&!compactBulletWallHit(path))return false;
        if(!hit){const int material=shopWallOn(path,false);
            if(material>=0){hit=true;if(!compactBulletWallHit(path,material))return false;}}
        if(!hit&&scenery_.gemChunkEnd){bool chunk=false,consumed=false;
            if(!hitGemChunk(path,0,chunk,consumed))return false;hit=chunk;}
        if(!hit){
            const int block=breakableAt(path.endX.pixel,path.endY.pixel);
            if(block>=0&&path.intersects(breakables_[block].x-8,breakables_[block].x+8,breakables_[block].y-8,breakables_[block].y+8))hit=true;
        }
        if(hit){c.live=false;continue;}
        if(timeStopBound(c.sprite(),int((c.x+(1L<<15))>>16),int((c.y+(1L<<15))>>16))){c.x+=c.xsp;c.y+=c.ysp;}
    }
    return true;
}

bool GameplayWorld::stepModuleTexts(){
    for(unsigned i=0;i<guns_.textEnd;++i){auto& t=guns_.texts[i];
        if(t.live&&t.birth!=tick_)t.step();}
    for(unsigned i=0;i<guns_.noticeEnd;++i){auto& n=guns_.notices[i];
        if(n.live&&n.birth!=tick_)n.step();}
    if(shopSpeechLive_)for(auto& r:shopSpeech_)if(r.rise.live&&r.rise.birth!=tick_)r.rise.step();
    return true;
}

// Shop Step's scrRisingTextShop(msgx, keeper.y - 18, text): msgx is
// keeper.x + 2 and the keeper stands at the counter's x + 18.
void GameplayWorld::createCompactShopSpeech(){
    for(unsigned i=0;i<downwell::ShopWorld::capacity;++i){
        const auto& shop=run_.shops.instances[i];
        if(!shop.allocated||shop.sourceId||shop.feedback.message==downwell::ShopFeedback::Message::none)continue;
        auto& r=shopSpeech_[i];
        if(!r.rise.live)++shopSpeechLive_;
        r={};r.rise.live=true;r.rise.x=shop.x+20;r.rise.y=long(shop.y-18)<<16;r.rise.birth=tick_;
        r.text=shopSpeechText_.text(shop.feedback.message,shop.farewellVariant);
        for(r.length=0;r.text&&r.text[r.length];)++r.length;
    }
}

bool GameplayWorld::collideGunModules(){
    if(dead())return true;
    const auto player=downwell::contactWorldBounds({240L*256+player_.offsetXFixed(),176L*256+player_.offsetYFixed(),1,1,31});
    for(unsigned i=0;i<guns_.moduleEnd;++i){
        auto& m=guns_.modules[i];
        if(!m.live||!m.obtainable||
           !downwell::contactBoundsOverlap(player,downwell::contactPixelBounds(m.x,m.y,1,1,m.sprite())))continue;
        if(!pickupGunModule(m))return false;
    }
    return true;
}

// GunModule Collision with objPlayer_n.
bool GameplayWorld::pickupGunModule(downwell::CompactGunModuleRecord& m){
    run_.soundPlay(28,50,false,true); // sfxPickupWeapon
    // scrEffectSpawn(plx, ply, sprFxAmmoCharge, 1, 0, -50500)
    const int px=240+int(downwell::roundGameMakerQ8(player_.offsetXFixed()));
    const int py=176+int(downwell::roundGameMakerQ8(player_.offsetYFixed()));
    if(!compactEffects_.spawnNoLoop(downwell::Fixed16::fromInteger(px),downwell::Fixed16::fromInteger(py),111,
            unitSpeed,{}))return run_.effectsSucceeded=false;
    run_.gunType=m.gun;
    int amount=2;
    if(m.type==0)player_.increaseAmmoCapacity(2);
    else{
        // hpGain 1 with the heart-piece overflow of the source.
        amount=1;
        (void)run_.combo.healthReserve.gain(health_,1);
    }
    auto* text=downwell::CompactGuns::append(guns_.texts,guns_.textEnd);
    if(!text)return run_.effectsSucceeded=false;
    text->x=m.x;text->y=m.y;text->gun=m.gun;text->type=m.type;text->amount=amount;text->birth=tick_;
    m.live=false;
    return true;
}

// End Step: scrOutofview for modules, scrOutofview + scrBulletOov for
// casings. Texts and notices have no End Step.
bool GameplayWorld::retireCompactGuns(long viewTopQ12){
    const long top=viewTopQ12/4096;
    for(unsigned i=0;i<guns_.moduleEnd;++i){auto& m=guns_.modules[i];
        if(m.live&&(m.y<top-160||m.y>top+3000))m.live=false;}
    for(unsigned i=0;i<guns_.casingEnd;++i){auto& c=guns_.casings[i];
        const long y=(c.y+(1L<<15))>>16;
        if(c.live&&(y<top-160||y>top+3000||y>top+284+32||y<top-100))c.live=false;}
    return true;
}

// Draw-time state of moduleText (frame advance on exit, itemNotif birth).
bool GameplayWorld::drawCompactGunTexts(){
    for(unsigned i=0;i<guns_.textEnd;++i){auto& t=guns_.texts[i];
        if(!t.live||!t.drawEvent())continue;
        auto* n=downwell::CompactGuns::append(guns_.notices,guns_.noticeEnd);
        if(!n)return run_.effectsSucceeded=false;
        n->x=t.x;n->y=long(t.y+4)<<16;n->type=t.type;n->amount=t.amount;n->birth=tick_+1;
        // allShown: sfxShopBuyBtry (ammo) or sfxShopBuyHeart (HP).
        if(t.type==0||t.type==1)run_.soundPlay(t.type==0?105:106,100,false,true);
    }
    return true;
}

bool GameplayWorld::hitGunModule(const downwell::ContactBounds& shot,int damage,bool& consumed){
    consumed=false;
    for(unsigned i=0;i<guns_.moduleEnd;++i){auto& m=guns_.modules[i];
        if(!m.live||!downwell::contactBoundsOverlap(shot,downwell::contactPixelBounds(m.x,m.y,1,1,m.sprite())))continue;
        if(m.objHp<=0)return true;
        m.objHp-=damage;consumed=true;return true;
    }
    return true;
}

// ---- Compact time-stop (W1.5) ------------------------------------------

GameplayWorld::FieldHit GameplayWorld::fieldAt(const downwell::ContactBounds& b)const{
    const auto relative=[&](int x,int y){return downwell::ContactBounds{b.left-x,b.right-x,b.top-y,b.bottom-y};};
    FieldHit hit;
    const auto goal=[&](const downwell::GoalTimeField& f){
        if(!downwell::GoalTimeField::meetsRelativeBounds(relative(f.x,f.y)))return false;
        hit={true,f.x,f.y};return true;
    };
    const auto ordinary=[&](unsigned i){
        const auto* f=run_.timeFields.at(i);
        if(!f||!downwell::OrdinaryTimeField::meetsRelativeBounds(relative(f->x,f->y)))return false;
        hit={true,f->x,f->y};return true;
    };
    // instance_place order: the room's placed startTimeField, then the
    // builder's fields with the exit's goalTimeField at its birth boundary.
    if(run_.startFieldPresent&&goal(run_.startField))return hit;
    const unsigned count=run_.timeFields.size();
    const unsigned boundary=run_.levelExitPresent&&run_.goalFieldBirthBoundary<count?run_.goalFieldBirthBoundary:count;
    unsigned first=count; // Lowest creation index whose mask meets the bounds.
    if(refreshFieldIndex()){
        const auto& index=fieldIndex_;
        unsigned lo=0,hi=count; // First centre with y > top-24.
        while(lo<hi){const unsigned mid=(lo+hi)/2;if(index.y[mid]>b.top-24)hi=mid;else lo=mid+1;}
        for(unsigned k=lo;k<count&&index.y[k]<b.bottom+24;++k){
            const unsigned i=index.order[k];
            if(i<first){const auto* f=run_.timeFields.at(i);
                if(f&&downwell::OrdinaryTimeField::meetsRelativeBounds(relative(f->x,f->y)))first=i;}
        }
    }else{
        for(unsigned i=0;i<count;++i){const auto* f=run_.timeFields.at(i);
            if(f&&downwell::OrdinaryTimeField::meetsRelativeBounds(relative(f->x,f->y))){first=i;break;}}
    }
    if(first<boundary){ordinary(first);return hit;}
    if(run_.levelExitPresent&&goal(run_.goalField))return hit;
    if(first<count)ordinary(first);
    return hit;
}

// Conservative: false only when no field mask can meet a rectangle whose
// rows lie within reach of y.
bool GameplayWorld::fieldNearY(int y,int reach)const{
    const auto near=[&](int fy,int radius){const int d=fy-y;return d<radius+reach&&-d<radius+reach;};
    if(run_.startFieldPresent&&near(run_.startField.y,128))return true;
    if(run_.levelExitPresent&&near(run_.goalField.y,128))return true;
    const unsigned count=run_.timeFields.size();
    if(!count)return false;
    if(!refreshFieldIndex())return true;
    unsigned lo=0,hi=count;
    while(lo<hi){const unsigned mid=(lo+hi)/2;if(fieldIndex_.y[mid]>y-reach-24)hi=mid;else lo=mid+1;}
    return lo<count&&fieldIndex_.y[lo]<y+reach+24;
}

bool GameplayWorld::refreshFieldIndex()const{
    auto& index=fieldIndex_;
    const unsigned count=run_.timeFields.size();
    const std::uint32_t birth=run_.timeFields.birthSequence().next;
    if(index.count==count&&index.birth==birth)return true;
    if(count>sizeof index.order){index.count=~0u;return false;}
    for(unsigned i=0;i<count;++i){ // Stable insertion sort; births are nearly y-ordered.
        const auto* f=run_.timeFields.at(i);
        if(!f){index.count=~0u;return false;}
        unsigned k=i;
        while(k&&index.y[k-1]>f->y){index.y[k]=index.y[k-1];index.order[k]=index.order[k-1];--k;}
        index.y[k]=f->y;index.order[k]=static_cast<unsigned char>(i);
    }
    index.count=count;index.birth=birth;return true;
}

bool GameplayWorld::timeStopBound(const downwell::ContactBounds& bounds,int x)const{
    // TimeStopBound: unstopped, inside any field, or strictly beside the shaft.
    if(!stopped()||fieldAt(bounds).hit)return true;
    return x<160||x>480-160;
}

bool GameplayWorld::timeStopBound(int sprite,int x,int y)const{
    if(!stopped())return true;
    const auto l=downwell::contactSpriteBounds(sprite);
    return timeStopBound({x+l.left,x+l.right,y+l.top,y+l.bottom},x);
}

// objPlayer_n Begin Step blocks 46..79 at private xx/yy, after the published
// x/y. The burst's instance_place uses those published builtins.
bool GameplayWorld::beginCompactTimeStop(int publishedX,int publishedY){
    using Stop=decltype(timeStop_);
    const long xxQ8=240L*256+player_.offsetXFixed(),yyQ8=176L*256+player_.offsetYFixed();
    const int facing=player_.facing();
    const bool outside=xxQ8<160L*256||xxQ8>320L*256;
    // Mask 31 spans a dozen rows; the coarse test spares most updates' query.
    const bool inField=fieldNearY(int(yyQ8>>8),32)&&
        fieldAt(downwell::contactWorldBounds({xxQ8,yyQ8,facing,1,31})).hit;
    // Running, unchecked and still wanting neither: the block emits nothing.
    if(!inField&&!outside&&!timeStop_.player.checkStop&&!timeStop_.player.stopped){
        run_.timeStopped=false;return true;
    }
    typename Stop::PlayerPose pose{};
    const bool wanted=inField||outside;
    if(wanted!=timeStop_.player.checkStop||wanted!=timeStop_.player.stopped){
        // Only transitions emit shards: build the exact pose just for them.
        if(!pose.fromQ8(publishedX,publishedY,xxQ8,yyQ8,player_.speedXFixed(),player_.speedYFifths()))
            return run_.effectsSucceeded=false;
    }
    const bool ok=timeStop_.beginStepWithAudioChecked(true,[&](bool& o){o=outside;return true;},pose,false,
        enemyRandom_,[&](bool& hit){hit=inField;return true;},
        [&](downwell::RunnerImageValue,downwell::RunnerImageValue,std::int32_t& id,
            std::int32_t& fx,std::int32_t& fy){
            const auto field=fieldAt(downwell::contactWorldBounds({publishedX*256L,publishedY*256L,facing,1,31}));
            id=field.hit?1:-4;fx=field.x;fy=field.y;return true;
        },[&](downwell::PlayerTimeStop::AudioQuery query,bool& value){
            auto audio=run_.audio();
            return downwell::TimeStopAudio::query(query,run_.backgroundMusicId,run_.fieldWind.voice,audio,value);
        },
        [&](downwell::PlayerTimeStop::Effect effect){
            using Effect=downwell::PlayerTimeStop::Effect;
            if(downwell::TimeStopAudio::handles(effect)){
                // Void enter/exit, the wind's stop and the song's pause and
                // resume; bgmDuck(200, 0.7) is music (WM).
                if(effect==Effect::duckMusic)return true;
                downwell::SoundPlayEnvironment env;auto audio=run_.audio();
                return timeStopAudio_.emit(effect,run_.soundCaller,env,run_.stage.area,
                    run_.backgroundMusicId,run_.fieldWind.voice,audio)&&run_.effectsSucceeded;
            }
            if(effect==Effect::shake)return requestTimeStopCameraShake();
            if(effect==Effect::oxygenFloor)return run_.floorTimeStopOxygen();
            if(effect==Effect::change)pauseMilliseconds_+=30; // fxTimeChange Create: sleep(30)
            return true;
        });
    if(!ok||!run_.effectsSucceeded)return run_.effectsSucceeded=false;
    run_.timeStopped=stopped();
    return true;
}

bool GameplayWorld::stepTimeShards(decltype(timeStop_.shards)::Phase phase){
    const bool ok=timeStop_.shards.step(phase,stopped(),enemyRandom_,
        [this](auto,const downwell::TimeShardParticle&,std::int32_t x,std::int32_t y,bool& hit){
            downwell::ContactBounds bounds; // sprite 91's mask from the Q16 position
            if(!run_.effectsSucceeded||!downwell::CurrencyCollisionBounds::calculateMaskQ16(91,x,y,bounds))
                return run_.effectsSucceeded=false;
            return solidContact(bounds,true,hit);
        },[this](auto,const downwell::TimeShardParticle& effect,bool& retire){
            return timeShardOutsideView(effect.y.v,retire);
        });
    return (ok&&run_.effectsSucceeded)||(run_.effectsSucceeded=false);
}

// fxTimeShard's Collision with objPlayer_n, at both published positions.
bool GameplayWorld::collideTimeShards(){
    const int px=240+int(downwell::roundGameMakerQ8(player_.offsetXFixed()));
    const int py=176+int(downwell::roundGameMakerQ8(player_.offsetYFixed()));
    const auto player=downwell::contactWorldBounds({px*256L,py*256L,player_.facing(),1,31});
    const auto l=downwell::contactSpriteBounds(91);
    std::uint32_t touched[downwell::gameplayCompactShardCapacity];unsigned count=0;
    bool valid=true;
    timeStop_.shards.visitNewest([&](std::uint32_t handle,const downwell::TimeShardParticle& shard){
        std::int32_t sx,sy;
        if(!shard.x.roundPixel(sx)||!shard.y.roundPixel(sy)){valid=false;return;}
        if(!shard.noTouch&&downwell::contactBoundsOverlap(player,{sx+l.left,sx+l.right,sy+l.top,sy+l.bottom}))
            touched[count++]=handle;
    });
    if(!valid)return run_.effectsSucceeded=false;
    const auto xsp=exactQ8(player_.speedXFixed());
    const auto ysp=downwell::MotionBinary64::fromInteger(player_.speedYFifths()); // Only its sign is read.
    for(unsigned i=0;i<count;++i)
        if(!timeStop_.shards.playerContact(touched[i],xsp,ysp,enemyRandom_))return run_.effectsSucceeded=false;
    return true;
}

// subparentEnemy's Collision with parentTimeField: ehp = 0 (no world-one
// enemy is timefieldImmune). Steering families die in their next Step;
// the other compact families retire at once, as their bullet deaths do.
bool GameplayWorld::collideEnemyTimeFields(){
    for(unsigned i=0;i<enemyCount_;++i){
        auto& enemy=enemies_[i];
        if(!enemy.active||enemy.destroyRequested||enemy.health<=0)continue;
        if(!fieldNearY(enemy.y+176,128))continue; // World-one masks reach far less.
        const auto bounds=downwell::contactWorldBounds({(enemy.x+240L)*256,(enemy.y+176L)*256,
            enemy.objectId==117?enemy.motionState:enemy.facing,enemy.imageScaleY(),
            downwell::enemyContactMask(enemy.objectId,enemy.spriteId,enemy.maskId)});
        if(!fieldAt(bounds).hit)continue;
        if(steeringObject(enemy.objectId)){
            auto* actor=steeringActor(enemy);
            if(!actor)return run_.effectsSucceeded=false;
            actor->health=0;enemy.health=0;continue;
        }
        enemy.health=0;
        run_.combo.state.enemyDefeated();
        if(!compactEnemyDeath(enemy,false))return false;
        if(!run_.effectsSucceeded||!publishEnemyActivation(enemy,false))return run_.effectsSucceeded=false;
        enemy.active=false;++run_.kills;
    }
    return true;
}

// The stopped branches of the compact families' Steps: image_speed = 0
// (EnemyState::frozen), the frog's and snail's alarm holds, the crawler's
// alarmStop(1), and the pending hits their skipped blocks would consume.
bool GameplayWorld::freezeEnemyStep(EnemyState& enemy){
    const auto hold=[](int& alarm){if(alarm>0)++alarm;};
    switch(enemy.objectId){
    case 113:
        hold(enemy.alarm0);hold(enemy.alarm1);
        if(enemy.frogHit.remaining)++enemy.frogHit.remaining;
        if(enemy.frogHit.ready)enemy.frogHit.pending=true;
        break;
    case 117:
        hold(enemy.snailHit.alarm2);
        if(enemy.snailHit.ready)enemy.snailHit.pending=true;
        break;
    case 116:
        if(enemy.batHit.ready){
            enemy.batHit.pending=true;enemy.batHit.pendingDirection=enemy.batHit.direction;
            enemy.batHit.pendingFraction=enemy.batHit.fraction;
        }
        break;
    case 119:
        if(!timeStopBound(downwell::contactWorldBounds({(enemy.x+240L)*256,(enemy.y+176L)*256,enemy.facing,1,278}),
            enemy.x+240)){hold(enemy.alarm0);hold(enemy.alarm1);}
        break;
    default:break;
    }
    return true;
}

// objBox_n Step's alarmStop(0) for every live box, not only exhausted ones.
bool GameplayWorld::holdStoppedBoxAlarms(){
    for(unsigned i=0;i<breakableCount_;++i){auto& box=breakables_[i];
        if(box.active&&!box.destroyRequested&&box.alarm0>0&&!timeStopBound(585,box.x+240,box.y+176))++box.alarm0;
    }
    return true;
}

bool GameplayWorld::resetCompactTimeStop(){
    pauseMilliseconds_=0;run_.timeStopped=false;
    return timeStop_.reset()||(run_.effectsSucceeded=false);
}

// ---- Compact enemy deaths and corpses (W1.6) ----------------------------

// scrBloodfx (arguments unused): emitMovingFx(x,y,84,0.5,0+random(20),2) and
// emitMovingFx(x,y,84,0.5,180-random(20),2). parentMovingFx Create draws none.
bool GameplayWorld::compactBloodfx(downwell::MotionBinary64 x,downwell::MotionBinary64 y){
    using Real=downwell::MotionBinary64;
    for(unsigned side=0;side<2;++side){
        Real offset,angle=Real::fromInteger(side?180:0);
        if(!downwell::randomBinary64(enemyRandom_.nextU32(),Real::fromInteger(20),offset))
            return run_.effectsSucceeded=false;
        if(side&&!offset.zero())offset.high^=0x80000000u;
        downwell::Fixed16 qx,qy,qa;
        if(!angle.add(offset)||!q16(x,qx)||!q16(y,qy)||!q16(angle,qa)||
           !compactEffects_.spawnMoving(qx,qy,84,half,qa,downwell::Fixed16::fromInteger(2)))
            return run_.effectsSucceeded=false;
    }
    return true;
}

// The bullet-immune branch: scrEffectSpawn(x,y,112,0.3,0,10000).
bool GameplayWorld::compactImmuneSpark(downwell::MotionBinary64 x,downwell::MotionBinary64 y){
    downwell::Fixed16 qx,qy;
    return (q16(x,qx)&&q16(y,qy)&&compactEffects_.spawnNoLoop(qx,qy,112,threeTenths,{}))||(run_.effectsSucceeded=false);
}

// scrTypicalBulletDamage (usual and spread shots) or bulletLaser's Collision
// with subparentEnemy, after the damage itself: the impact for scrDeadBody,
// then the 605 splash (usual) or blood, a flash and sndEnemyhurt (laser).
bool GameplayWorld::compactBulletImpact(EnemyState& enemy,downwell::MotionBinary64 x,downwell::MotionBinary64 y,
    std::uint32_t direction,int damage,bool laser){
    enemy.takenImpactDamage=damage;
    compactMomentDelay(); // scrDamageEnemy
    if(!compactDamageEffectsAdmitted){if(laser)run_.soundPlay(16,50,false,true);return run_.effectsSucceeded;}
    downwell::Fixed16 flashX,flashY;
    if(laser)return compactBloodfx(x,y)&&q16(x,flashX)&&q16(y,flashY)&&run_.emitFlash<1,0,2>(flashX,flashY,enemyRandom_)&&
        (run_.soundPlay(16,50,false,true),run_.effectsSucceeded);
    downwell::RunnerImageValue angle;downwell::Fixed16 qx,qy,qa;
    if(!downwell::ProjectilePublication::angle(direction,angle)||!q16(x,qx)||!q16(y,qy)||
       !downwell::Fixed16::fromBinary32(angle.bits,qa)||!compactEffects_.spawnNoLoop(qx,qy,605,fourTenths,qa))
        return run_.effectsSucceeded=false;
    return true;
}

// The !alive block shared by snake 111, turtle 112, frog 113, bat 116, snail
// 117 and crawler 119: scrBloodfx, scrSmokefx(1), scrFlashballfx(1),
// scrCurrencySpawn, then scrDeadBody(dead sprite, takenImpact, 102). A stomp
// first runs scrEnemyStomped's scrBloodfx. The compact families die at the
// contact rather than in their next Step, so the chain runs there.
bool GameplayWorld::compactEnemyDeath(EnemyState& enemy,bool stomped){
    using Real=downwell::MotionBinary64;
    int sprite;
    switch(enemy.objectId){
        case 111:sprite=165;break;case 112:sprite=337;break;case 113:sprite=172;break;
        case 116:sprite=158;break;case 117:sprite=179;break;case 119:sprite=281;break;
        case 114:sprite=291;break;
        default:return run_.effectsSucceeded=false;
    }
    // scrEnemyDeath: gunshotStop (toggleGuncut is on) before momentDelay.
    run_.stopSound(run_.overlapSounds[1]);
    compactMomentDelay();
    compactLevelBeginCue();
    const Real x=Real::fromInteger(enemy.x+240),y=Real::fromInteger(enemy.y+176);
    // The eyefish's smoke and flash take its private xx/yy; it has no sound.
    const bool eyefish=enemy.objectId==114;
    const Real fx=eyefish?enemy.eyefish.xx:x,fy=eyefish?enemy.eyefish.yy:y;
    int deathSound;
    switch(enemy.objectId){
        case 111:deathSound=121;break;case 112:deathSound=123;break;case 113:deathSound=114;break;
        case 116:deathSound=111;break;case 117:deathSound=119;break;case 114:deathSound=-1;break;
        default:deathSound=124;break;
    }
    // The family's last act before instance_destroy.
    struct DeathSound {GameplayWorld& world;int sound,y;~DeathSound(){if(world.run_.effectsSucceeded&&sound>=0)world.playEnemySound(sound,y);}}
        lastAct{*this,deathSound,enemy.y+176};
    // scrEnemyStomped: scrBloodfx(3, 0), then pugExplodingKnees; the death
    // the compact world runs at the contact follows.
    if(stomped&&compactDamageEffectsAdmitted&&!compactBloodfx(x,y))return false;
    if(stomped&&run_.upgrades.levels[0]&&!compactExplodingKnees(enemy.x+240,enemy.y+176,
        240+int(downwell::roundGameMakerQ8(player_.frameStartXFixed())),
        176+int(downwell::roundGameMakerQ8(player_.frameStartYFixed())),true))return false;
    if(!compactDamageEffectsAdmitted){spawnReward(enemy,stomped);return run_.effectsSucceeded;}
    downwell::Fixed16 qx,qy;
    if(!compactBloodfx(x,y)||!q16(fx,qx)||!q16(fy,qy)||!emitBoxSmoke(qx,qy)||!run_.emitFlash<1,0,0>(qx,qy,enemyRandom_))return false;
    spawnReward(enemy,stomped);
    if(!run_.effectsSucceeded)return false;
    downwell::DeadBodySource source;
    source.x=x;source.y=y;source.xsp=exactQ8(enemy.speedX);
    source.imageXscale=Real::fromInteger(enemy.objectId==117?enemy.motionState:enemy.facing);
    Real impact;
    if(!downwell::divideBinary64(Real::fromInteger(enemy.takenImpactDamage),Real::fromInteger(5),impact))
        return run_.effectsSucceeded=false;
    struct Queries {
        GameplayWorld& world;
        // place_meeting(parentWall) with maskDead 599 at the birth position.
        bool wall(Real px,Real py,bool& blocked){return world.compactCorpseMeeting(px,py,false,blocked);}
        // Compact walls are never replaced by depth-100 tiles (see the
        // spawner's tile query): where the original finds a tile, this world
        // still has the wall.
        bool tile(Real,Real,bool& hit){hit=false;return true;}
        bool publish(downwell::RunnerImageValue,downwell::RunnerImageValue){return true;}
    };
    struct Hooks {
        GameplayWorld& world;
        bool birth(std::uint32_t,const downwell::CorpseAnimation&){return true;}
        Queries queries(std::uint32_t,const downwell::CorpseAnimation&){return {world};}
        bool sprite(std::uint32_t,const downwell::CorpseAnimation&){return true;}
        bool facing(std::uint32_t,const downwell::CorpseAnimation&){return true;}
    } hooks{*this};
    // global.pugRip (Rest in Pieces): the corpse takes objHp 1 once active.
    return compactCorpses_.createFromDeath(source,sprite,impact,102,run_.upgrades.levels[3]!=0,enemyRandom_,hooks)||
        (run_.effectsSucceeded=false);
}

// maskDead 599 against walls (and, for sParentSolid, platforms). The mask
// is symmetric about the origin, so image_xscale does not move its edges.
bool GameplayWorld::compactCorpseMeeting(downwell::MotionBinary64 x,downwell::MotionBinary64 y,bool platforms,bool& hit){
    if(wallContactResolver_&&(!platforms||platformContactResolver_))return solidMeeting(599,x,y,platforms,hit);
    downwell::ContactBounds bounds;
    if(!downwell::CurrencyCollisionBounds::calculateMask(599,x,y,bounds))return run_.effectsSucceeded=false;
    return compactCorpseContact(bounds,platforms,hit);
}

// Worlds with only a point resolver (host fixtures) scan the mask's pixel
// box; without any resolver nothing is solid.
bool GameplayWorld::compactCorpseContact(const downwell::ContactBounds& bounds,bool platforms,bool& hit){
    if(wallContactResolver_&&(!platforms||platformContactResolver_))return solidContact(bounds,platforms,hit);
    hit=false;
    if(!solidResolver_)return true;
    for(int py=bounds.top;py<bounds.bottom&&!hit;++py)
        for(int px=bounds.left;px<bounds.right&&!hit;++px)
            hit=solidResolver_(solidResolverContext_,px-240,py-176);
    return true;
}

// objDeadBody Step: TimeStopBound motion against sParentSolid, or the held
// alarms; dflash in both. With Rest in Pieces (global.pugRip, upgrade 3) a
// shot-down active corpse becomes bulletExplosion1 272 and three
// explosionSmall 274 at (x, yy); the compact blast records keep whole
// pixels, so yy is rounded.
bool GameplayWorld::stepCompactCorpses(decltype(compactCorpses_.beginPhase()) phase){
    struct Events {
        GameplayWorld& world;
        bool meeting(std::int32_t x,std::int32_t y,bool& hit){
            downwell::ContactBounds bounds;
            if(!downwell::CurrencyCollisionBounds::calculateMaskQ16(599,x,y,bounds))return world.run_.effectsSucceeded=false;
            return world.compactCorpseContact(bounds,true,hit);
        }
        int area(){return world.run_.stage.area;}
        int restInPieces(){return world.run_.upgrades.levels[3];}
        bool create(int object,std::int32_t px,std::int32_t py){
            return (object==272||object==274)&&world.createCompactBlast(object,px,py);
        }
        void destroy(){}
    } events{*this};
    struct Hooks{} hooks;
    return compactCorpses_.dispatch(phase,[&](auto,downwell::CorpseParticle& corpse){
        bool frozen=false;
        if(compactTimeStop()&&stopped()){
            std::int32_t px,py;
            if(!corpse.x.roundPixel(px)||!corpse.y.roundPixel(py))return false;
            frozen=!timeStopBound(599,px,py);
        }
        return corpse.step(frozen,events);
    },hooks)||(run_.effectsSucceeded=false);
}

// Alarm 0 creates fxJobutsu 242 with explosionFx (102) and destroys the
// corpse; 1 starts the flashing, 2 clears hit, 3 activates. None draws RNG.
bool GameplayWorld::advanceCompactCorpseAlarm(unsigned alarm){
    struct Hooks{} hooks;
    return compactCorpses_.dispatch(compactCorpses_.beginPhase(),[&](auto,downwell::CorpseAnimation& corpse){
        return run_.advanceCorpseAlarm(corpse,alarm,[](int id){return id==102?12u:0u;},[]{});
    },hooks)||(run_.effectsSucceeded=false);
}

// End Step scrOutofview, then release every corpse destroyed this update.
bool GameplayWorld::endStepCompactCorpses(long viewTop){
    struct Events{void destroy(){}} events;
    struct Hooks{bool retire(std::uint32_t,const downwell::CorpseAnimation&){return true;}} hooks;
    const long top=viewTop/16;
    return (compactCorpses_.dispatch(compactCorpses_.beginPhase(),[&](auto,downwell::CorpseParticle& corpse){
        return corpse.endStep(true,top,events);
    },hooks)&&compactCorpses_.retireRequested(hooks))||(run_.effectsSucceeded=false);
}

// objControlerN 37 room_speed writers (momentDelay and scrPDamage's WITH
// body) and its Alarm 0, which restores 60. The registered build owns these
// through RunControllers; the compact world keeps the controller's alarm.
// room_speed is the time-stop global, which the Amiga pacing presents.
void GameplayWorld::compactMomentDelay(){
    if(runControllers_||timeStop_.roomSpeed!=60)return;
    timeStop_.roomSpeed=30;compactControllerAlarm0_=1;
}
void GameplayWorld::compactDamageSlowdown(){
    if(runControllers_)return;
    timeStop_.roomSpeed=40;compactControllerAlarm0_=40;
}
void GameplayWorld::advanceCompactControllerAlarm(){
    if(compactControllerAlarm0_>0&&--compactControllerAlarm0_==0)timeStop_.roomSpeed=60;
}

// scrBulCheckSolid's unbreakable-wall branch: step by a tenth of the velocity
// (at most 101 times) until collision_point meets the wall, then
// scrEffectSpawn(hitWallFx 101, 0.5, 0, 0). The bullet is destroyed either
// way, so the effect's position never feeds back into the simulation. The
// tenth truncates toward zero in Q20 (GameMaker divides the double).
int GameplayWorld::shopWallOn(const downwell::ProjectileSegment& path,bool cases)const{
    if(!run_.shops.allocatedCount)return -1;
    auto x=path.x,y=path.y;x.pixel+=240;y.pixel+=176;
    return run_.shops.wallMaterialOn(downwell::ProjectileSegment(x,y,path.dx,path.dy),cases);
}
bool GameplayWorld::compactBulletWallHit(const downwell::ProjectileSegment& path,int material){
    auto x=path.x,y=path.y;
    const std::int32_t vx=path.dx/10,vy=path.dy/10;
    if(!solidResolver_)return true;
    // collision_point(x, y, subparentSolidWall): terrain, or near a shop its
    // table and invisible wall.
    const auto solid=[&](int px,int py){return solidResolver_(solidResolverContext_,px,py)||
        (run_.shops.allocatedCount&&run_.shops.wallMaterialAt(px+240,py+176,false)>=0);};
    for(unsigned step=0;!solid(x.pixel,y.pixel);){
        y.advance(vy);x.advance(vx);
        if(++step>100)break;
    }
    const auto real=[](const downwell::ProjectileSegment::Coordinate& c,int origin){
        downwell::MotionBinary64 value=downwell::MotionBinary64::fromInteger(c.pixel+origin),fraction;
        if(c.fraction&&(!downwell::divideBinary64(downwell::MotionBinary64::fromInteger(int(c.fraction)),
               downwell::MotionBinary64::fromInteger(downwell::MachineGunTrajectory::unit),fraction)||
           !value.add(fraction)))return downwell::MotionBinary64::fromInteger(c.pixel+origin);
        return value;
    };
    if(!compactWallEffect(real(x,240),real(y,176),101,0))return false;
    // soundBullet(thatWall): objWall_n is foliage, a single-sample table
    // whose irandom_range(1, 1) still consumes two words; the shop's rock
    // wall and metal table choose between two (initSfxBhit).
    const int first=material==3?34:material==4?32:29;
    run_.soundPlayOL(first+int(enemyRandom_.irandomSmall(material>=3?1:0)),60,false,true,"gunwall");
    return run_.effectsSucceeded;
}

// A full no-loop pool drops the cosmetic puff rather than latching failure.
bool GameplayWorld::compactWallEffect(downwell::MotionBinary64 x,downwell::MotionBinary64 y,int sprite,int depth){
    return compactCosmeticEffect(x,y,sprite,{0x3fe00000u,0},depth);
}
bool GameplayWorld::compactCosmeticEffect(downwell::MotionBinary64 x,downwell::MotionBinary64 y,int sprite,
    downwell::MotionBinary64 speed,int depth){
    if(compactEffects_.noLoopFull())return true;
    downwell::Fixed16 qx,qy;downwell::RunnerImageValue r;(void)depth;
    if(!q16(x,qx)||!q16(y,qy)||!r.assign(speed)||!compactEffects_.spawnNoLoop(qx,qy,sprite,r,{}))
        return run_.effectsSucceeded=false;
    return true;
}

// levelBeginCue (scrPlMovement's landing, scrEnemyDeath): outside pTimeStop,
// the first call of a level creates levelNumber 372 and sets firstLand. Area
// 5 only sets firstLand. BGM and sfxFalling are W2. A stored-owner cue
// (beginLevelCueWithTitle) sets firstLand first, so neither creates twice.
void GameplayWorld::compactLevelBeginCue(){
    if(run_.firstLand||stopped())return;
    run_.firstLand=true;
    if(run_.stage.area==5||!compactAreaTitles_)return;
    const downwell::AreaTitle* title=nullptr;
    for(unsigned i=0;i<compactAreaTitleCount_&&!title;++i){
        const auto& t=compactAreaTitles_[i];
        if(t.area!=run_.stage.area||!t.language)continue;
        const char *a=t.language,*b=compactLanguage_;
        while(*a&&*a==*b){++a;++b;}
        if(*a==*b)title=&t;
    }
    if(!compactTitle_.create(title?title->characters:nullptr,title?title->length:0,run_.stage.level,run_.hardMode))
        compactTitle_={};
    compactLevelBeginMusic();
}
// levelBeginCue after levelNumber's Create: stop sfxFalling, then start the
// area's song unless it plays (music is silent until WM; the request and
// its voice are kept so the log and the voice count match the original).
void GameplayWorld::compactLevelBeginMusic(){
    run_.stopSound(186);
    int music;
    if(!downwell::RunSession::areaMusic(run_.stage.area,music))return;
    run_.backgroundMusicId=music;
    if(run_.voices.playing(music))return;
    run_.setSoundGain(music,downwell::audio_level::one);
    run_.soundPlayOL(music,100,true,true,"music");
}

// comboDone's instance_create(xx, view y + 64 + 64, comboRewardText); Create
// destroys it at once while global.death. A full store drops the text.
bool GameplayWorld::createCompactComboReward(){
    // Create's milestone sounds precede its global.death destroy: sfxCombo1
    // with nonGemGainGem's sfxGemGet, sfxCombo2, sfxCombo3 with gainHp's.
    const unsigned mask=run_.combo.reward.soundMask;
    if(mask&1){run_.soundPlayOL(315,80,false,true,"UI");run_.soundPlay(167,80,false,true);}
    if(mask&2)run_.soundPlayOL(316,80,false,true,"UI");
    if(mask&4){run_.soundPlayOL(317,80,false,true,"UI");run_.soundPlay(3,90,false,true);}
    if(dead())return true;
    for(auto& r:compactRewards_)if(!r.live){
        const long viewX=long(160+cameraPixelX_)<<16,viewY=long(34+cameraPixelY_)<<16;
        r.create((240L<<16)+(run_.combo.rewardXQ8<<8),viewX,viewY,viewY+(142L<<16),
            run_.combo.reward.combo,int(run_.combo.reward.showBonusMax));
        ++liveRewards_;
        return true;
    }
    return true;
}

// Alarms, then Step (relativey against the camera, the flash toggle).
void GameplayWorld::advanceCompactHudAlarms(){
    // objPlayer_n Alarm 7 (afterImageInterval 1): shift the history, then
    // record the previous Step's sprite, subimage, xscale and position, which
    // are what that update's Draw showed.
    if(playerInstance_.lifetimeId){
        afterImageHead_=afterImageHead_?afterImageHead_-1:afterImageCount-1;
        afterImages_[afterImageHead_]=presentedPose_;
    }
    compactTitle_.advanceAlarms();
    // objEnterDither 73 Alarm 0: instance_destroy, before its Step.
    if(compactEntryLive_){
        if(!compactEntry_.advanceAlarm())run_.effectsSucceeded=false;
        else if(compactEntry_.destroyRequested)compactEntryLive_=false;
    }
    if(!liveRewards_)return;
    const long camera=long(34+142+cameraPixelY_)<<16;
    for(auto& r:compactRewards_)if(r.live){
        r.advanceAlarms();
        if(r.live)r.step(camera);else --liveRewards_;
    }
}

// objPlayer_n Create: x = the new camMain's x (its Create x, the player's),
// y = view_yview[0] of the room's serialized view.
bool GameplayWorld::createCompactEntryDither(){
    const auto* view=roomEntryViews_.get(0);
    compactEntry_={};compactEntryLive_=false;
    if(!view||!compactEntry_.create([&](downwell::RunnerImageValue& x){
            return x.assignInteger(playerInstance_.x);
        },[&](downwell::RunnerImageValue& y){y=view->y;return true;}))return false;
    compactEntryLive_=true;return true;
}

// The previous update's Draw mutations (letter offsets, slide-ins, flash).
void GameplayWorld::advanceCompactHudDraw(){
    compactTitle_.advanceDraw();
    if(liveRewards_)for(auto& r:compactRewards_)if(r.live)r.advanceDraw();
}

// scrPlayerInit: nine records of sprite 31, subimage 0, xscale 1 at x/y,
// and flickerTime 0.
void GameplayWorld::resetCompactAfterImages(int x,int y){
    const long xFixed=long(x-240)<<8,yFixed=long(y-176)<<8;
    for(auto& a:afterImages_)a={31,0,1,xFixed,yFixed};
    presentedPose_={31,0,1,xFixed,yFixed};
    afterImageHead_=0;
    afterImagesDrawn_=0;
    flickerTime_=0;lowHealthRed_=false; // flickerTime = 0
}

// playerDraw while gemStreak >= gemStreakThreshold and !global.death:
// round(min(8, 8 * timer / 300)) + 1 images, newest last, each jittered by
// irandom_range(-1,1) and irandom_range(-2,1) from the world RNG in Draw.
void GameplayWorld::drawCompactAfterImages(){
    afterImagesDrawn_=0;
    if(playerInstance_.lifetimeId){
        const bool deadPose=death_.movementDead();
        presentedPose_={player_.spriteId(deadPose),player_.spriteFrame(deadPose,death_.animation),
            player_.facing(),player_.offsetXFixed(),player_.offsetYFixed()};
    }
    lowHealthRed_=false;
    if(!playerInstance_.lifetimeId||run_.goalStopped)return;
    if(currencyAccount_.streak>=downwell::CompactGemStreakHud::threshold&&!dead()){
        const unsigned timer=currencyAccount_.timer;
        unsigned amount=timer>=300?8:unsigned(downwell::CompactGemStreakHud::roundRatio(8UL*timer,300));
        if(amount>8)amount=8;
        for(int i=int(amount);i>=0;--i){
            afterImageDx_[i]=int(enemyRandom_.irandomSmall(2))-1;
            afterImageDy_[i]=int(enemyRandom_.irandomSmall(3))-2;
        }
        afterImagesDrawn_=amount+1;
    }
    // Then, through !dFlash at global.playerHp == 1: flickerTime counts
    // Draws; from flickerAt 45 two shaderPlayerRed copies replace the
    // player, and at flickerAt + flickerDur (51) the count restarts. Each
    // draw_sprite_ext evaluates its arguments right to left: y's
    // irandom_range(-1,1), then x's irandom_range(-2,2).
    if(!playerVisible()||health_!=1)return;
    if(++flickerTime_<45)return;
    if(flickerTime_>=51)flickerTime_=0;
    for(unsigned i=0;i<2;++i){
        lowHealthDy_[i]=int(enemyRandom_.irandomSmall(2))-1;
        lowHealthDx_[i]=int(enemyRandom_.irandomSmall(4))-2;
    }
    lowHealthRed_=true;
}

#include "GameplayWorldUpgrades.inc"

// bulletCasing's subparentEnemy collision with Hot Casing: scrTypicalBulletDamage
// at bdmg 5 and the inherited bDir 0; the casing lives on.
bool GameplayWorld::collideHotCasings(){
    for(unsigned i=0;i<guns_.casingEnd;++i){const auto& c=guns_.casings[i];
        if(!c.live||!c.hot)continue;
        const int cx=int((c.x+(1L<<15))>>16),cy=int((c.y+(1L<<15))>>16);
        const auto casing=downwell::contactPixelBounds(cx,cy,1,1,467);
        for(unsigned e=0;e<enemyCount_;++e){auto& enemy=enemies_[e];
            if(!enemy.active||enemy.destroyRequested||enemy.health<=0)continue;
            const auto target=downwell::contactWorldBounds({(enemy.x+240L)*256,(enemy.y+176L)*256,
                enemy.objectId==117?enemy.motionState:enemy.facing,enemy.imageScaleY(),
                downwell::enemyContactMask(enemy.objectId,enemy.spriteId,enemy.maskId)});
            if(!downwell::contactBoundsOverlap(casing,target))continue;
            bool consumed=false;
            if(!typicalBulletDamage(enemy,c.damage,downwell::RangedShot::degreesToTurns(0),
                GunReal::fromInteger(cx),GunReal::fromInteger(cy),consumed))return false;
        }
    }
    return run_.effectsSucceeded;
}
