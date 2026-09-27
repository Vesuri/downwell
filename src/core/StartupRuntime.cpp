#include "StartupRuntime.h"
#include "RunnerSpatialSplit.h"
#include "PlayerBoxContact.h"
#include "GameplayPlayerStep.h"
#include "MainRoomLifetime.h"
#include "MainRoomActivation.h"
#include "BuilderPatternPack.h"
#include "EventScheduler.h"
#include "EnemyPresentation.h"
#include "EnemyCreateProfile.h"
#include "TitleParticleRandom.h"
#include "EnemyStepPresentation.h"
#include "EnemyContact.h"
#include "EnemyContactResponse.h"
#include "PlayerLifeRules.h"
#include "CurrencyReward.h"
#include "CurrencyStep.h"
#include "FrogBehavior.h"
#include "FrogMotion.h"
#include "StandardBulletStep.h"
#include "StandardBulletImpact.h"
#include "BatMotion.h"
#include "GameMakerRounding.h"
#include "ObjectPack.h"
#include "PackedRoomLoader.h"
#include "PlayerCollision.h"
#include "SolidCollision.h"
#include "RoomLoader.h"
#include "StateDigest.h"
#include <algorithm>
#include <cmath>

namespace downwell {

void StartupRuntime::applyFrogAlarm(InstanceRecord& instance, const FrogAlarmEffect& effect) {
    if (effect.preparation >= 0) instance.auxiliaryState = effect.preparation;
    if (effect.setJumpSpeed) instance.motionState = effect.jumpSpeed;
    if (effect.setX) {
        instance.frogHorizontal.velocity = effect.xQ16;
        instance.velocityX = effect.xQ16 / 65536.0F;
    }
    if (effect.setY) {
        instance.frogVertical.velocity = effect.yFifthsQ8;
        instance.velocityY = effect.yFifthsQ8 / 1280.0F;
    }
    instance.alarms.set(effect.alarm, effect.ticks);
    if (effect.sound >= 0) ++soundPlayRequests_;
}

StartupRuntime::StartupRuntime(PlatformProfile profile,
                               const RoomPack* roomPack,
                               const ObjectPack* objectPack,
                               std::uint32_t randomSeed,
                               const BuilderPatternPack* builderPatternPack,
                               InstanceLifecycleBinding lifecycle)
    : lifecycle_(lifecycle), roomPack_(roomPack), objectPack_(objectPack),
      builderPatternPack_(builderPatternPack),
      randomSeed_(randomSeed), random_(randomSeed) {
    reset(profile);
}

void StartupRuntime::reset(PlatformProfile profile) {
    if(!instances_.clear([this](InstanceHandle h,const InstanceRecord& record){return lifecycle_.retire(h,record);})){
        roomLoadSucceeded_=false;run_.effectsSucceeded=false;return;
    }
    tick_ = 0;
    profile_ = profile;
    random_.setSeed(randomSeed_); // Result of the Windows runner's randomize().
    roomId_ = entryRoomId;
    roomDestination_ = 0;
    showSplash_ = 0;
    initTimes_ = 0;
    achievementLoginCalls_ = 0;
    splashSpeed_ = 180;
    splashDitherTenths_ = 110;
    roomDestinationSet_ = false;
    roomLoadSucceeded_ = true;
    implementedCodeCalls_ = 0;
    unimplementedCodeCalls_ = 0;
    cameraViewWidth_ = 0;
    cameraViewHeight_ = 0;
    backgroundMusicId_ = -1;
    controlsLocked_ = false;
    soundGainRequests_ = 0;
    soundPlayRequests_ = 0;
    ditherFadeSpawnRequests_ = 0;
    deferredInstances_.clear();
    nextRuntimeInstanceId_ = roomPack_ != nullptr
                                 ? roomPack_->nextRuntimeInstanceId()
                                 : 105186;
    deferredCommandsSucceeded_ = true;
    alarmEventMasks_.fill(-1);
    inputLeft_ = false;
    inputRight_ = false;
    inputLeftPressed_ = false;
    inputRightPressed_ = false;
    inputFire_ = false;
    inputFirePressed_ = false;
    inputFireReleased_ = false;
    anyInput_ = false;
    resetSceneState();

    loadRoom(entryRoomId, true);
}

void StartupRuntime::resetSceneState() {
    menuPlayerInitialized_ = false;
    menuPlayerNapping_ = false;
    menuPlayerX_ = 112.0F;
    menuPlayerY_ = 512.0F;
    menuPlayerSpeedX_ = 0.0F;
    menuPlayerSpeedY_ = 0.0F;
    menuPlayerGrounded_ = true;
    menuPlayerGoalStop_ = false;
    menuPlayerFacing_ = 1;
    menuPlayerSpriteId_ = 31;
    menuPlayerImageIndex_ = 0.0F;
    menuPlayerImageSpeed_ = 0.1F;
    menuCameraX_ = 112.0F;
    menuCameraY_ = 512.0F;
    menuTitleFirstBoot_ = false;
    menuTitleReady_ = false;
    menuTitleStartY_ = 432.0F;
    menuTeleportActive_ = false;
    menuTeleportSequenceStarted_ = false;
    menuCameraLockerY_ = 448.0F;
    enterSequenceActive_ = false;
    enterSequenceBlackSet_ = false;
    enterSequenceY_ = 448.0F;
    enterSequenceSpeedY_ = 0.0F;
    playMenuYOffset_ = -128.0F;
    playMenuTileY_ = 0.0F;
    playMenuDitherY_ = 80.0F;
    playMenuRunFrame_ = 0.0F;
    playMenuSmoothX_ = 0.0F;
    playMenuCursor_ = 0;
    playMenuSelected_ = false;
    playStyle_ = 0;
    playMenuPlayerDescent_ = 0.0F;
    playMenuPlayerSpeedY_ = 0.0F;
    playMenuVerticalDitherY_ = -320.0F;
    mainPlayerInitialized_ = false;
    mainPlayerMotion_ = PlayerMotion(false);
    mainPlayerOriginX_ = 240;
    mainPlayerOriginY_ = 176;
    mainPlayerX_ = 240.0F;
    mainPlayerY_ = 176.0F;
    mainPlayerSpeedX_ = 0.0F;
    mainPlayerSpeedY_ = 0.0F;
    mainPlayerGrounded_ = false;
    mainPlayerFacing_ = 1;
    mainPlayerSpriteId_ = 37;
    mainPlayerImageIndex_ = 2.0F;
    mainJumpShootLocked_ = false;
    mainShotDelayed_ = true;
    mainAirborneShot_ = false;
    mainAmmo_ = 8;
    mainPlayerHealth_ = 4;
    mainDeath_ = {};
    run_.reset();
    mainPlayerInvulnerability_ = 0;
    mainEmitter_.reset(false);
    mainShotsFired_ = 0;
    mainBlocksDestroyed_ = 0;
    mainLandings_ = 0;
    mainActivationY_ = 0;
    mainEnemiesActivated_ = 0;
    mainInstancesRetired_ = 0;
    mainJumps_ = 0;
    mainBulletCount_ = 0;
    mainCurrencyCount_ = 0;
    currencyAccount_ = {};
    mainBuilderInitialized_ = false;
    mainBuilderY_ = 704;
    mainBuilderGenerationCount_ = -30;
    mainBuilderRowsGenerated_ = 0;
    mainBuilderRightRooms_ = {{-1, -1}};
    mainBuilderStartPattern_ = -1;
    mainBuilderStartReversed_ = false;
    mainBuilderStartRowsGenerated_ = 0;
    mainBuilderLastMainPattern_ = -1;
    mainBuilderLastPatternReversed_ = false;
    mainBuilderMainPatternsGenerated_ = 0;
    mainBuilderSpawnerMade_ = false;
    mainBuilderLastRightPattern_ = -1;
    mainBuilderSideRoomsGenerated_ = 0;
    mainBuilderLastShopPattern_ = -1;
    mainBuilderShopsGenerated_ = 0;
    mainBuilderRightRoomsMade_ = 0;
    mainBuilderShopMade_ = true;
    mainBuilderEndReached_ = false;
    mainBuilderEndPattern_ = -1;
    mainBuilderChunkOrCrate_ = 0;
    mainBuilderSpecialObjectsGenerated_ = 0;
    mainCameraInitialized_ = false;
    mainCameraX_ = 240.0F;
    mainCameraMotion_ = {};
    mainCameraY_ = 176.0F;
    mainCameraTargetY_ = 176.0F;
}

bool StartupRuntime::restartFromResults(DeathMenuSequence::Action action, std::uint32_t seed) {
    if (resultsDestination(action) == ResultsDestination::none) return false;
    const auto records = run_.records();
    resetSceneState();
    const auto destination = run_.restart(action, records);
    // randomize is an external entropy boundary. Replays explicitly supply it.
    random_.setSeed(seed);
    deferredInstances_.clear();
    showSplash_ = 0;
    roomDestinationSet_ = false;
    roomDestination_ = static_cast<int>(destination);
    controlsLocked_ = false;
    ++initTimes_;
    ++soundPlayRequests_; // ResetPlayer's soundPlayOL(321).
    if (auto* controller = instances_.find(controllerInstanceId))
        for (unsigned alarm = 0; alarm <= 6; ++alarm) controller->alarms.set(alarm, 0);
    gotoRoom(static_cast<int>(destination));
    return roomLoadSucceeded_;
}

void StartupRuntime::skipIntroForTesting() {
    // Run normal room Create initialization, including the garden RNG draws,
    // but never present or step the skipped scenes.
    showSplash_ = 0;
    gotoRoom(menuRoomId);
    if (roomLoadSucceeded_)
        gotoRoom(playMenuRoomId);
}

void StartupRuntime::skipToGameForTesting() {
    // Fast-forward the deterministic usual-style entry without presenting any
    // skipped frames. Garden updates consume RNG (not just room Create), so
    // jumping straight to PlayMenu produces a different world from the native
    // checkpoint. Do not repair this by hard-coding a random-stream offset.
    showSplash_ = 0;
    gotoRoom(menuRoomId);
    for (unsigned frame = 0; frame < 4096 && roomLoadSucceeded_ && !inMainRoom(); ++frame) {
        Input input;
        if (inMenu()) input.right = true;
        if (inPlayMenu() && playMenuYOffset_ == 0 && !playMenuSelected_)
            input.fire = true;
        step(input, input.fire);
    }
    // Treat a malformed/missing transition as failed initialization, not a
    // successful shortcut that silently leaves the user in a skipped scene.
    if (!inMainRoom()) roomLoadSucceeded_ = false;
}

void StartupRuntime::controllerCreate(PlatformProfile profile) {
    // objControlerN_Create_0 sets initTimes to zero, then scrInitialize()
    // increments it and establishes the platform profile.
    initTimes_ = 0;
    ++initTimes_;
    showSplash_ = 0;
    if (profile == PlatformProfile::desktop)
        showSplash_ = 1;
}

void StartupRuntime::initRoomCreate() {
    roomDestinationSet_ = false;
    splashSpeed_ = 180;
}

void StartupRuntime::step(bool advancePressed) {
    stepInternal(advancePressed, false, false, advancePressed, advancePressed);
}

bool StartupRuntime::publishCollisionPosition(InstanceHandle h,RunnerImageValue x,RunnerImageValue y){
    if(!run_.effectsSucceeded)return false;
    auto* record=instances_.resolve(h);
    std::int32_t drawX,drawY;
    if(!record||!RunnerSpatialSplit::valid(x)||!RunnerSpatialSplit::valid(y)||
       !x.promoted().roundPixel(drawX)||!y.promoted().roundPixel(drawY)||
       !lifecycle_.publishedPosition(h,*record,x,y)){
        run_.effectsSucceeded=false;return false;
    }
    record->x=drawX;record->y=drawY;
    return true;
}

void StartupRuntime::step(const Input& input, bool advancePressed) {
    stepInternal(advancePressed, input.left, input.right, input.fire,
                 input.left || input.right || input.fire || input.pause);
}

void StartupRuntime::stepInternal(bool advancePressed,
                                  bool left,
                                  bool right,
                                  bool fire,
                                  bool anyInput) {
    if (run_.levelClearActive || !run_.effectsSucceeded ||
        !roomLoadSucceeded_ || !deferredCommandsSucceeded_) return; // Result owner uses stepLevelClear.
    if(roomId_==mainRoomId)
        run_.advanceFlashAnimation();
    if(roomId_==mainRoomId)run_.advanceSmokeAnimation();
    if(roomId_==mainRoomId)run_.advanceCorpseExpiryAnimation();
    if(!run_.effectsSucceeded)return;
    ++tick_;
    inputLeftPressed_ = left && !inputLeft_;
    inputRightPressed_ = right && !inputRight_;
    inputFirePressed_ = fire && !inputFire_;
    inputFireReleased_ = !fire && inputFire_;
    inputLeft_ = left;
    inputRight_ = right;
    inputFire_ = fire;
    anyInput_ = anyInput;
    if (roomId_ == mainRoomId) {
        currencyAccount_.beginStep();
        run_.selectWeaponLevel(currencyAccount_);
    }
    if (roomId_ == mainRoomId) {
        run_.consumables.beginStep(mainPlayerMotion_, mainPlayerHealth_, run_.combo.healthReserve);
        Input resultsInput;
        resultsInput.left = left;
        resultsInput.right = right;
        resultsInput.fire = fire;
        run_.results.beginStep(resultsInput);
    }

    for (std::size_t i = 0; i < instances_.size(); ++i) {
        auto& instance = instances_.at(i);
        instance.enemyStepEligible=false;
        if (instance.active && instance.objectId == 113) instance.frogHit.advanceFrame();
        if (instance.active && instance.objectId == 117) instance.snailHit.advanceFrame();
        if (instance.active && instance.objectId == 116) instance.batHit.advanceFrame();
    }
    if(!instances_.visitAnimationChecked([this](InstanceHandle key,InstanceRecord& instance){
        if(!lifecycle_.capturePrevious(key,instance))return false;
        if(instance.objectId==238&&timeEffectDispatch_&&
            !timeEffectDispatch_(timeEffectContext_,instance,TimeEffectPhase::animation,0))return false;
        const bool enemyObject=instance.objectId==111 || instance.objectId==112 ||
            instance.objectId==113 || instance.objectId==116 || instance.objectId==117 || instance.objectId==119;
        if (enemyObject) {
            if (!instance.enemyImageInitialized) {
                instance.enemyImagePhase=static_cast<unsigned>(std::lround(instance.imageIndex*256));
                instance.enemyImageInitialized=true;
            }
            advanceEnemyImageQ8(instance.enemyImagePhase,instance.enemyImageRemainder,
                               static_cast<unsigned>(std::lround(instance.imageSpeed*20)));
            if(!lifecycle_.phaseIndex(key,instance,instance.enemyImagePhase))return false;
            instance.imageIndex=instance.enemyImagePhase/256.0F;
        }
        return true;
    })){run_.effectsSucceeded=false;return;}

    if (showSplash_ == 3 || showSplash_ == 4)
        splashDitherTenths_ = 110;
    else if (showSplash_ == 5 && splashDitherTenths_ >= 10)
        splashDitherTenths_ -= 3;
    else if (showSplash_ == 6 && splashDitherTenths_ <= 110)
        splashDitherTenths_ += 3;

    const auto alarmEvents = [this](const InstanceRecord& instance) {
        if (objectPack_ == nullptr)
            return instance.objectId == 36
                       ? static_cast<AlarmBank::FiredMask>(1U << 0)
                       : static_cast<AlarmBank::FiredMask>(0);
        const bool cached = instance.objectId >= 0 &&
            static_cast<std::size_t>(instance.objectId) < alarmEventMasks_.size();
        if (cached && alarmEventMasks_[instance.objectId] >= 0)
            return static_cast<AlarmBank::FiredMask>(alarmEventMasks_[instance.objectId]);
        AlarmBank::FiredMask mask = 0;
        PackedObjectEvent event;
        for (std::int32_t alarm = 0;
             alarm < static_cast<std::int32_t>(AlarmBank::count);
             ++alarm) {
            if (objectPack_->findEvent(instance.objectId, 2, alarm, event))
                mask |= static_cast<AlarmBank::FiredMask>(1U << alarm);
        }
        if (cached) alarmEventMasks_[instance.objectId] = static_cast<std::int16_t>(mask);
        return mask;
    };
    const auto handler = [this, advancePressed](InstanceRecord& instance,
                                                 StepEvent event) {
        if (event.phase==StepEventPhase::step) instance.enemyStepEligible=true;
        if (objectPack_ != nullptr) {
            if (event.phase == StepEventPhase::alarm) {
                dispatchObjectEvent(instance, 2, event.subtype, advancePressed);
                return;
            }
            std::int32_t subtype = 0;
            if (event.phase == StepEventPhase::beginStep)
                subtype = 1;
            if (event.phase == StepEventPhase::endStep)
                subtype = 2;
            dispatchObjectEvent(instance, 3, subtype, advancePressed);
            return;
        }
        if (instance.objectId == 36) {
            if (event.phase == StepEventPhase::alarm && event.subtype == 0)
                initRoomAlarm0();
            if (event.phase == StepEventPhase::endStep && roomId_ == entryRoomId)
                initRoomEndStep(advancePressed);
        }
    };
    auto smokeStepPhase=run_.smokeEffects.beginPhase();
    if(!EventScheduler<instanceCapacity>::runStepChecked(instances_, alarmEvents, handler,
        [&](StepEventPhase phase){
            if(roomId_==mainRoomId && phase==StepEventPhase::alarm)
                run_.advanceDamageNumberAlarm(run_.damageNumbers.beginPhase(),0);
            // This sprite-less effect only mutates itself. Execute at the
            // global Step boundary, before any collision/Step births.
            if(roomId_==mainRoomId && phase==StepEventPhase::step)
                run_.stepDamageNumbers(run_.damageNumbers.beginPhase());
            if(phase==StepEventPhase::step)smokeStepPhase=run_.smokeEffects.beginPhase();
        },[this](InstanceHandle h,const InstanceRecord& record,bool member){return lifecycle_.membership(h,record,member);},
        [this]{return run_.effectsSucceeded&&roomLoadSucceeded_&&deferredCommandsSucceeded_;},
        [this]{return !collisionDispatch_||collisionDispatch_(collisionContext_);})){
        run_.effectsSucceeded=false;return;
    }
    if (deferredInstances_.size() != 0)
        flushDeferredInstances(instances_, roomId_);
    if (!roomLoadSucceeded_ || !deferredCommandsSucceeded_ || !run_.effectsSucceeded) return;
    if (roomId_ == mainRoomId) {
        run_.exitDither.step(mainCameraRenderViewY(), surfaceHeight);
        updateMainBullets();
        if(!run_.effectsSucceeded)return;
        updateMainEnemies();
        run_.stepSmoke(smokeStepPhase,random_);
        run_.collideSmokeWalls([&](const ContactBounds& query,auto emit){
            for(std::size_t i=0;i<instances_.size();++i){
                const auto& wall=instances_.at(i);
                if(!wall.active || (wall.objectId!=60 && wall.objectId!=92 && wall.objectId!=293))continue;
                if((wall.scaleX!=1 && wall.scaleX!=-1) || (wall.scaleY!=1 && wall.scaleY!=-1) || wall.rotation!=0)return false;
                const auto bounds=contactWorldBounds({long(wall.x)*256,long(wall.y)*256,
                    int(wall.scaleX),int(wall.scaleY),wall.objectId==293?569:585});
                if(contactBoundsOverlap(query,bounds) && !emit())return false;
            }
            return true;
        });
        if(!run_.effectsSucceeded)return;
        updateMainEnemyInteractions();
        if(!run_.effectsSucceeded)return;
        run_.collideLevelExit(roundGameMakerCoordinate(mainPlayerX_),
            roundGameMakerCoordinate(mainPlayerY_), mainDeath_.active);
        updateMainCurrency();
        if(!run_.effectsSucceeded)return;
        updateMainLifecycle();
        if(!run_.effectsSucceeded)return;
        // Camera Collision follows ordinary Step/retirement, before shop End
        // Step and Draw. Earlier actors/generation keep their Step-time view.
        run_.sideCameraLockers.collide(mainCameraMotion_);
        mainCameraX_ = mainCameraMotion_.x / 4096.0F;
        mainCameraY_ = mainCameraMotion_.y / 4096.0F;
        mainCameraTargetY_ = mainCameraMotion_.targetY / 4096.0F;
        for (std::size_t i = 0; i < instances_.size(); ++i) {
            auto& camera = instances_.at(i);
            if (camera.active && camera.objectId == 64) {
                camera.x = mainCameraMotion_.pixelX();
                camera.y = mainCameraMotion_.pixelY();
            }
        }
        run_.shops.endStep(mainCameraRenderViewY());
        run_.ammoHud.step(mainShotsFired_, mainPlayerMotion_.recharges());
        run_.ammoGauge.step(mainAmmo_, mainPlayerMotion_.ammoCapacity());
        run_.gameplayPresentationStep(random_);
    }
}

void StartupRuntime::updateMenuPlayer(InstanceRecord& instance) {
    if (!menuPlayerInitialized_)
        return;

    if (menuPlayerNapping_) {
        if (anyInput_ && !controlsLocked_) {
            if (menuPlayerGrounded_)
                random_.irandom(3); // soundLand foliage-sample selection.
            menuPlayerNapping_ = false;
            menuPlayerY_ = 512.0F;
            menuPlayerSpriteId_ = 31; // sprPlayerIdle
            menuPlayerImageIndex_ = 0.0F;
            menuPlayerImageSpeed_ = 0.1F;
            menuPlayerGrounded_ = true;
            menuPlayerSpeedY_ = 0.0F;
        }
    } else {
        if (menuPlayerGoalStop_) {
            menuPlayerSpeedX_ = 0.0F;
            menuPlayerSpeedY_ = 0.0F;
        }
        if (controlsLocked_ || (inputLeft_ && inputRight_)) {
            menuPlayerSpeedX_ = 0.0F;
        } else if (!menuPlayerGoalStop_ && inputLeft_) {
            menuPlayerSpeedX_ -= 0.2F;
            if (menuPlayerSpeedX_ > 0.0F)
                menuPlayerSpeedX_ -= 2.0F;
            if (menuPlayerSpeedX_ < -2.0F)
                menuPlayerSpeedX_ = -2.0F;
            menuPlayerFacing_ = -1;
        } else if (!menuPlayerGoalStop_ && inputRight_) {
            menuPlayerSpeedX_ += 0.2F;
            if (menuPlayerSpeedX_ < 0.0F)
                menuPlayerSpeedX_ += 2.0F;
            if (menuPlayerSpeedX_ > 2.0F)
                menuPlayerSpeedX_ = 2.0F;
            menuPlayerFacing_ = 1;
        } else if (menuPlayerSpeedX_ > 0.0F) {
            menuPlayerSpeedX_ = menuPlayerSpeedX_ > 0.4F
                                    ? menuPlayerSpeedX_ - 0.4F : 0.0F;
        } else if (menuPlayerSpeedX_ < 0.0F) {
            menuPlayerSpeedX_ = menuPlayerSpeedX_ < -0.4F
                                    ? menuPlayerSpeedX_ + 0.4F : 0.0F;
        }

        if (!menuPlayerGoalStop_)
            menuPlayerX_ += menuPlayerSpeedX_;
        if (menuPlayerX_ < 16.0F)
            menuPlayerX_ = 16.0F;
        if (menuPlayerX_ > 400.0F)
            menuPlayerX_ = 400.0F;
        const bool overWell = menuPlayerX_ >= 300.0F && menuPlayerX_ <= 356.0F;
        if (menuPlayerGrounded_ && overWell)
            menuPlayerGrounded_ = false;
        if (!menuPlayerGrounded_ && !menuPlayerGoalStop_) {
            menuPlayerSpeedY_ += 0.2F;
            if (menuPlayerSpeedY_ > 7.0F)
                menuPlayerSpeedY_ = 7.0F;
            menuPlayerY_ += menuPlayerSpeedY_;
            if (menuPlayerY_ > 512.0F) {
                if (menuPlayerX_ < 300.0F) {
                    menuPlayerX_ = 300.0F;
                } else if (menuPlayerX_ > 356.0F) {
                    menuPlayerX_ = 356.0F;
                }
            }
        } else if (menuPlayerGrounded_) {
            menuPlayerY_ = 512.0F;
            menuPlayerSpeedY_ = 0.0F;
        }

        if (!menuPlayerGrounded_) {
            menuPlayerSpriteId_ = 37; // sprPlayerAir
            menuPlayerImageSpeed_ = 0.0F;
            if (menuPlayerSpeedY_ < 1.0F)
                menuPlayerImageIndex_ = 2.0F;
            else if (menuPlayerSpeedY_ < 2.0F)
                menuPlayerImageIndex_ = 3.0F;
            else
                menuPlayerImageIndex_ = 4.0F;
        } else {
            menuPlayerSpriteId_ = menuPlayerSpeedX_ == 0.0F ? 31 : 27;
            menuPlayerImageSpeed_ = menuPlayerSpeedX_ == 0.0F ? 0.1F : 0.3F;
        }

        // objTeleportMain's 160x32 touch area spans the bottom of the shaft.
        if (!menuTeleportActive_ && menuPlayerY_ >= 625.0F &&
            menuPlayerX_ >= 240.0F && menuPlayerX_ <= 400.0F) {
            menuTeleportActive_ = true;
            menuPlayerGoalStop_ = true;
            controlsLocked_ = true;
            menuPlayerSpeedX_ = 0.0F;
            menuPlayerSpeedY_ = 0.0F;
        }
    }

    if (menuPlayerImageSpeed_ > 0.0F)
        menuPlayerImageIndex_ += menuPlayerImageSpeed_;
    if (!menuPlayerNapping_ && menuPlayerGrounded_ &&
        menuPlayerSpriteId_ == 27) {
        if (menuPlayerImageIndex_ >= 8.0F)
            menuPlayerImageIndex_ = std::fmod(menuPlayerImageIndex_, 8.0F);
        const int frame = static_cast<int>(std::floor(menuPlayerImageIndex_));
        const int priorFrame = static_cast<int>(std::floor(
            menuPlayerImageIndex_ - menuPlayerImageSpeed_));
        if ((frame == 1 || frame == 5) && frame != priorFrame)
            random_.irandom(3); // soundFootstep foliage-sample selection.
    }
    instance.x = static_cast<std::int32_t>(std::round(menuPlayerX_));
    instance.y = static_cast<std::int32_t>(std::round(menuPlayerY_));
    instance.imageIndex = menuPlayerImageIndex_;
    instance.imageSpeed = menuPlayerImageSpeed_;
}

LaserSightRay StartupRuntime::traceLaserSight(LaserSightRay::Coordinate x,
    LaserSightRay::Coordinate y, LaserSightRay::Coordinate bottom,
    std::uint32_t speedTurns, std::uint32_t aimTurns) const {
    return LaserSightRay::trace(x, y, bottom, speedTurns,
        [&](const ProjectileSegment& path) {
            if (run_.shops.rayWallIntersects(path)) return true;
            for (std::size_t i = 0; i < instances_.size(); ++i) {
                const auto& wall = instances_.at(i);
                if (wall.active && (wall.objectId == 60 || wall.objectId == 92) &&
                    path.intersects(wall.x-8, wall.x+8, wall.y-8, wall.y+8)) return true;
            }
            return false;
        }, [&](const ProjectileSegment& path) {
            for (std::size_t i = 0; i < instances_.size(); ++i) {
                const auto& enemy = instances_.at(i);
                const int id = enemy.objectId;
                if (!enemy.active || !(id == 111 || id == 112 || id == 113 ||
                                      id == 116 || id == 117 || id == 119)) continue;
                const ContactActor contact{enemy.x*256L, enemy.y*256L,
                    enemy.scaleX < 0 ? -1 : 1, enemy.scaleY < 0 ? -1 : 1,
                    enemyContactMask(id, enemy.spriteId,enemy.maskId)};
                if (path.intersects(contact)) return true;
            }
            return false;
        }, aimTurns);
}

bool StartupRuntime::mainSolidAt(float x, float y) const {
    // A solid tile covers all sixteen pixel cells. Testing fractional
    // coordinates against inclusive integer bounds left subpixel cracks
    // between adjacent tiles (e.g. x=7.5 between centres 0 and 16).
    x = std::floor(x);
    y = std::floor(y);
    for (std::size_t index = 0; index < instances_.size(); ++index) {
        const InstanceRecord& wall = instances_.at(index);
        if (!wall.active || (wall.objectId != 60 && wall.objectId != 92))
            continue;
        if (x >= wall.x - 8.0F && x <= wall.x + 7.0F &&
            y >= wall.y - 8.0F && y <= wall.y + 7.0F)
            return true;
    }
    return false;
}

InstanceRecord* StartupRuntime::mainBulletCollision(const ProjectileSegment& path) {
    for (std::size_t index = 0; index < instances_.size(); ++index) {
        InstanceRecord& wall = instances_.at(index);
        if (!wall.active || (wall.objectId != 60 && wall.objectId != 92))
            continue;
        if (path.intersects(wall.x - 8, wall.x + 8, wall.y - 8, wall.y + 8))
            return &wall;
    }
    return nullptr;
}

InstanceRecord* StartupRuntime::mainBulletEnemyCollision(const ProjectileSegment& path) {
    for (std::size_t index = 0; index < instances_.size(); ++index) {
        InstanceRecord& enemy = instances_.at(index);
        const bool enemyObject = enemy.objectId == 111 ||
                                 enemy.objectId == 112 ||
                                 enemy.objectId == 113 ||
                                 enemy.objectId == 116 ||
                                 enemy.objectId == 117 ||
                                 enemy.objectId == 119;
        if (!enemy.active || !enemyObject || enemy.health <= 0)
            continue;
        const ContactActor contact{enemy.x * 256L, enemy.y * 256L,
            enemy.scaleX < 0 ? -1 : 1, enemy.scaleY < 0 ? -1 : 1,
            enemyContactMask(enemy.objectId, enemy.spriteId,enemy.maskId)};
        if (path.intersects(contact))
            return &enemy;
    }
    return nullptr;
}

void StartupRuntime::emitMainBullet(int x,int y) {
    if (!run_.effectsSucceeded) return;
    const int muzzleOffset = mainEmitter_.nextOffset();
    if (mainBulletCount_ >= mainBulletCapacity) {
        run_.effectsSucceeded = false;
        return;
    }
    mainBullets_[mainBulletCount_++] = {
        static_cast<float>(x + muzzleOffset),
        static_cast<float>(y + 4), run_.gemHigh, {}, {}
    };
    mainBullets_[mainBulletCount_ - 1].trajectory.reset(x + muzzleOffset, y + 4,
                                                      run_.upgrades.levels[12] != 0);
    ++soundPlayRequests_;
}

void StartupRuntime::syncMainPlayerState() {
    mainPlayerX_ = mainPlayerOriginX_ + mainPlayerMotion_.offsetXFixed() / 256.0F;
    mainPlayerY_ = mainPlayerOriginY_ + mainPlayerMotion_.offsetYFixed() / 256.0F;
    mainPlayerSpeedX_ = mainPlayerMotion_.speedXFixed() / 256.0F;
    mainPlayerSpeedY_ = mainPlayerMotion_.speedYFixed() / 256.0F;
    mainPlayerGrounded_ = mainPlayerMotion_.grounded();
    mainPlayerFacing_ = mainPlayerMotion_.facing();
    mainPlayerSpriteId_ = mainPlayerMotion_.spriteId(mainDeath_.movementDead());
    mainPlayerImageIndex_ = static_cast<float>(mainPlayerMotion_.spriteFrame(mainDeath_.movementDead(), mainDeath_.animation));
    mainJumpShootLocked_ = mainPlayerMotion_.jumpShootLocked();
    mainShotDelayed_ = mainPlayerMotion_.shotDelayed();
    mainAirborneShot_ = mainPlayerMotion_.airborneShot();
    mainAmmo_ = mainPlayerMotion_.ammo();
    mainJumps_ = mainPlayerMotion_.jumps();
    mainShotsFired_ = mainPlayerMotion_.shots();
}

bool StartupRuntime::publishMainEnemyTransform(std::size_t index,RunnerTransformField field,int value) {
    // Currently implemented enemy transforms use opaque white artwork.
    if(lifecycle_.integerTransform(instances_.handleAt(index),instances_.at(index),field,value,
                                   {0x3f800000u},0xffffffu))return true;
    run_.effectsSucceeded=false;return false;
}

bool StartupRuntime::publishMainEnemyImage(std::size_t index,unsigned phase) {
    if(lifecycle_.phaseIndex(instances_.handleAt(index),instances_.at(index),phase))return true;
    run_.effectsSucceeded=false;return false;
}

bool StartupRuntime::publishMainInactivation(const InstanceRecord& record) {
    // Keep the stored lifetime until its normal erase boundary, but remove
    // collision visibility before the response clears active. Do not use a
    // dormant activation request: dead actors must not become activationPending.
    for (std::size_t i=0;i<instances_.size();++i) {
        if (&instances_.at(i)!=&record) continue;
        if (lifecycle_.activation(instances_.handleAt(i),record,false)) return true;
        break;
    }
    run_.effectsSucceeded=false;
    return false;
}

void StartupRuntime::updateMainBullets() {
    std::size_t write = 0;
    for (std::size_t index = 0; index < mainBulletCount_; ++index) {
        MainBulletState bullet = mainBullets_[index];
        bool alive=false;
        const bool succeeded=stepMachineGunProjectileChecked(bullet, random_,
          [this, &bullet](const ProjectileSegment& path) {
            InstanceRecord* enemy = mainBulletEnemyCollision(path);
            if (enemy != nullptr) {
                const auto impact = applyStandardBulletImpact(*enemy, run_.kills, random_,
                    [this, enemy] {
                        run_.combo.state.enemyDefeated();
                        spawnMainCurrency(static_cast<float>(enemy->x),
                                          static_cast<float>(enemy->y),
                                          enemy->reward);
                        return run_.effectsSucceeded;
                    }, bullet.gemHigh, bullet.trajectory.direction,
                    [this](const InstanceRecord& record){return publishMainInactivation(record);});
                if (impact.kind == BulletImpactKind::ignored) return false;
                if (impact.kind == BulletImpactKind::failed) return true;
                ++soundPlayRequests_;
                return true;
            }
            return false;
          }, [this](const ProjectileSegment& path) {
            InstanceRecord* wall = mainBulletCollision(path);
            if (wall != nullptr) {
                if (wall->objectId == 92) {
                    // objBox_n starts with wallHp = 1. The usual gun's bdmg
                    // exhausts it on contact; its Step event destroys it.
                    if (!publishMainInactivation(*wall)) return true;
                    wall->active = false;
                    ++mainBlocksDestroyed_;
                    ++soundPlayRequests_;
                }
                return true;
            }
            return false;
          },alive,[this]{return run_.effectsSucceeded;},0,mainCameraMotion_.viewTopQ12(),surfaceHeight);
        if(!succeeded){
            // Keep the failed shot's completed initialization and all untouched
            // later shots, while retaining earlier successful retirements.
            mainBullets_[write++]=bullet;
            for(std::size_t next=index+1;next<mainBulletCount_;++next)mainBullets_[write++]=mainBullets_[next];
            mainBulletCount_=write;return;
        }
        if (alive)
            mainBullets_[write++] = bullet;
    }
    mainBulletCount_ = write;
}

void StartupRuntime::spawnMainCurrency(float x, float y, std::int32_t amount,
                                       bool attracted) {
    if(!run_.effectsSucceeded)return;
    const auto reward=spawnCurrencyReward(amount,
        static_cast<int>(roundGameMakerCoordinate(x)) - mainPlayerOriginX_,
        static_cast<int>(roundGameMakerCoordinate(y)) - mainPlayerOriginY_,
        attracted, static_cast<unsigned>(mainCurrencyCapacity - mainCurrencyCount_), random_,
        [this, x, y, attracted](const CurrencyBirth& birth) {
        MainCurrencyState& gem = mainCurrencyItems_[mainCurrencyCount_++];
        gem = MainCurrencyState{};
        gem.x = x;
        gem.y = y;
        gem.large = birth.large;
        gem.amount = birth.amount;
        gem.attracted = attracted;
        gem.motion = birth.motion;
        gem.speedX = gem.motion.vx / 12800.0F;
        gem.speedY = gem.motion.vy / 12800.0F;
    }, run_.upgrades.levels[10] != 0);
    if(!reward.complete)run_.effectsSucceeded=false;
}

void StartupRuntime::updateMainCurrency() {
    std::size_t write = 0;
    for (std::size_t index = 0; index < mainCurrencyCount_; ++index) {
        MainCurrencyState gem = mainCurrencyItems_[index];
        const auto result = run_.collectCurrency(gem.motion, gem.large, static_cast<unsigned>(gem.amount),
            mainPlayerMotion_, currencyAccount_,
            [this](int x, int y) { return mainSolidAt(mainPlayerOriginX_ + x, mainPlayerOriginY_ + y); }, random_);
        if (result == CurrencyStepResult::expired) continue;
        if (result == CurrencyStepResult::collected) {
            syncMainPlayerState();
            ++soundPlayRequests_;
            continue;
        }
        gem.age = gem.motion.lifetime.age;
        // objCurrency Step publishes round(xx/yy), not the private fractional
        // integration coordinates. Both adapters use the same signed ties-even
        // operation. Main-room origins (240,176) are even integer offsets.
        gem.x = mainPlayerOriginX_ + GemMotion::rounded(gem.motion.x);
        gem.y = mainPlayerOriginY_ + GemMotion::rounded(gem.motion.y);
        gem.speedX = gem.motion.vx / 12800.0F;
        gem.speedY = gem.motion.vy / 12800.0F;
        gem.attracted = gem.motion.attracted;
        mainCurrencyItems_[write++] = gem;
    }
    mainCurrencyCount_ = write;
}

void StartupRuntime::updateCrawler(InstanceRecord& enemy, bool alarm) {
    if (!enemy.crawler.initialized) enemy.crawler.initialize(enemy.x, enemy.y);
    const bool visible = CrawlerMotion::inView(enemy.x, enemy.y,
        mainCameraMotion_.viewLeftQ12(), mainCameraMotion_.viewTopQ12(),
        mainCameraMotion_.viewRightQ12(), mainCameraMotion_.viewBottomQ12(),
        alarm ? 0 : 64);
    const auto meeting = [this](long x, long y) {
        return CrawlerMotion::meetsTiles(x, y, [this](int px, int py) {
            return mainSolidAt(px, py);
        });
    };
    if (alarm) enemy.crawler.alarm(enemy.motionState > 0, visible, meeting);
    else enemy.crawler.step(enemy.motionState > 0, visible, meeting);
    enemy.preciseX = enemy.crawler.x / 1280.0F;
    enemy.preciseY = enemy.crawler.y / 1280.0F;
    enemy.velocityX = enemy.crawler.vx / 1280.0F;
    enemy.velocityY = enemy.crawler.vy / 1280.0F;
    enemy.grounded = enemy.crawler.latched;
}

void StartupRuntime::updateMainEnemies() {
    for (std::size_t index = 0; index < instances_.size(); ++index) {
        InstanceRecord& enemy = instances_.at(index);
        const bool enemyObject = enemy.objectId == 111 ||
                                 enemy.objectId == 112 ||
                                 enemy.objectId == 113 ||
                                 enemy.objectId == 116 ||
                                 enemy.objectId == 117 ||
                                 enemy.objectId == 119;
        if (!enemy.active || !enemyObject || enemy.health <= 0 || !enemy.enemyStepEligible)
            continue;
        if (!enemy.motionInitialized) {
            enemy.preciseX = static_cast<float>(enemy.x);
            enemy.preciseY = static_cast<float>(enemy.y);
            enemy.motionInitialized = true;
        }

        if (enemy.objectId == 113)
            enemy.imageSpeed = enemy.frogPresentation.beginStepRate() / 20.0F;
        if (enemy.objectId == 111 || enemy.objectId == 112) {
            if (!enemy.walker.initialized) {
                enemy.walker.initialize(enemy.x, enemy.y,
                    static_cast<long>(std::round(enemy.velocityX * 1280)),
                    static_cast<long>(std::round(enemy.velocityY * 1280)), enemy.grounded);
                enemy.walker.facing=enemy.scaleX<0?-1:1;
            }
            const bool turtle = enemy.objectId == 112;
            const bool visible = CrawlerMotion::inView(enemy.x, enemy.y,
                mainCameraMotion_.viewLeftQ12(), mainCameraMotion_.viewTopQ12(),
                mainCameraMotion_.viewRightQ12(), mainCameraMotion_.viewBottomQ12(),
                turtle ? 64 : 32);
            if(!enemy.walker.step(turtle, groundTurtleWalking(enemy.auxiliaryState), visible,
                [this, &enemy, turtle](long x, long y) {
                    return enemy.walker.meetsTiles(x, y, turtle,
                        [this](int px, int py) { return mainSolidAt(px, py); });
                },[this,index,&enemy](int sprite,int scale){
                    enemy.grounded=enemy.walker.grounded;
                    if(!lifecycle_.sprite(instances_.handleAt(index),enemy,sprite)){
                        run_.effectsSucceeded=false;return false;
                    }
                    enemy.spriteId=sprite;
                    if(scale){
                        if(!publishMainEnemyTransform(index,RunnerTransformField::scaleX,scale))return false;
                        enemy.scaleX=static_cast<float>(scale);
                    }
                    return true;
                }))return;
            enemy.preciseX = enemy.walker.x / 1280.0F;
            enemy.preciseY = enemy.walker.y / 1280.0F;
            enemy.velocityX = enemy.walker.vx / 1280.0F;
            enemy.velocityY = enemy.walker.vy / 1280.0F;
            enemy.grounded = enemy.walker.grounded;
        } else if (enemy.objectId == 113) {
            const bool integrate = frogMotionVisible(enemy.x * 256L, enemy.y * 256L,
                static_cast<long>(std::round(mainCameraViewX() * 256.0F)),
                static_cast<long>(std::round(mainCameraViewY() * 256.0F)),
                static_cast<long>(std::round((mainCameraViewX() + surfaceWidth) * 256.0F)),
                static_cast<long>(std::round((mainCameraViewY() + surfaceHeight) * 256.0F)));
            if (!enemy.frogMotionInitialized) {
                enemy.frogHorizontal.position = static_cast<long>(std::round((enemy.preciseX - mainPlayerOriginX_) * 65536.0F));
                enemy.frogHorizontal.velocity = static_cast<long>(std::round(enemy.velocityX * 65536.0F));
                enemy.frogVertical.position = static_cast<long>(std::round((enemy.preciseY - mainPlayerOriginY_) * 1280.0F));
                enemy.frogVertical.velocity = static_cast<long>(std::round(enemy.velocityY * 1280.0F));
                enemy.frogMotionInitialized = true;
            }
            enemy.frogVertical.grounded = enemy.grounded;
            const auto effects = stepFrogMotion(enemy.frogHorizontal, enemy.frogVertical,
                enemy.frogHit, integrate, [this](int x, int y) {
                    return mainSolidAt(mainPlayerOriginX_ + x, mainPlayerOriginY_ + y);
                },[this,index,&enemy]{
                enemy.auxiliaryState = 0;
                enemy.alarms.set(0, 0);
                enemy.alarms.set(1, 60);
                enemy.velocityX = enemy.frogHorizontal.velocity / 65536.0F;
                if(!publishMainEnemyImage(index,0))return false;
                enemy.imageIndex = 0;
                enemy.enemyImagePhase = 0; enemy.enemyImageRemainder = 0;
                ++soundPlayRequests_;
                return true;
            },[this,index,&enemy](int scale){
                if(!publishMainEnemyTransform(index,RunnerTransformField::scaleX,scale))return false;
                enemy.scaleX=static_cast<float>(scale);return true;
            },[&enemy](bool grounded){
                if(frogLandingEdge(grounded,enemy.frogLanded))enemy.alarms.set(1,60);
            });
            enemy.preciseY = mainPlayerOriginY_ + enemy.frogVertical.position / 1280.0F;
            enemy.velocityY = enemy.frogVertical.velocity / 1280.0F;
            enemy.grounded = enemy.frogVertical.grounded;
            enemy.preciseX = mainPlayerOriginX_ + enemy.frogHorizontal.position / 65536.0F;
            enemy.velocityX = enemy.frogHorizontal.velocity / 65536.0F;
            if(!effects.succeeded)return;
        } else if (enemy.objectId == 116) {
            BatMotion bat{
                static_cast<long>(std::round(enemy.preciseX * 256.0F)),
                static_cast<long>(std::round(enemy.preciseY * 256.0F)),
                static_cast<long>(std::round(enemy.velocityX * 256.0F)),
                static_cast<long>(std::round(enemy.velocityY * 256.0F)),
                static_cast<int>(enemy.direction),
                static_cast<int>(std::round(enemy.moveSpeed * 40.0F)),
                enemy.scaleX < 0 ? -1 : 1, enemy.batDirectionFraction
            };
            bat.collisionMask=enemyContactMask(enemy.objectId,enemy.spriteId,enemy.maskId);
            const auto effects = stepBatBehavior(bat, enemy.batHit, enemy.batActivity,
                enemy.x, enemy.y, bat.collisionMask,
                mainPlayerOriginX_ * 256L + mainPlayerMotion_.frameStartXFixed(),
                mainPlayerOriginY_ * 256L + mainPlayerMotion_.frameStartYFixed(),
                random_,
                [this](int x, int y) { return mainSolidAt(x, y); },
                [this,index,&enemy](int scale){
                    if(!publishMainEnemyTransform(index,RunnerTransformField::scaleX,scale))return false;
                    enemy.scaleX=static_cast<float>(scale);return true;
                });
            // Preserve completed turn/collision work even if the scale setter
            // rejects publication. The checked helper skips subsequent motion.
            enemy.preciseX = bat.x / 256.0F;
            enemy.preciseY = bat.y / 256.0F;
            enemy.velocityX = bat.speedX / 256.0F;
            enemy.velocityY = bat.speedY / 256.0F;
            enemy.direction = static_cast<float>(bat.direction);
            enemy.batDirectionFraction = bat.directionFraction;
            enemy.moveSpeed = bat.speedFortieths / 40.0F;
            enemy.scaleX = static_cast<float>(bat.facing);
            if(!effects.succeeded)return;
            if (effects.hit) {
                enemy.alarms.set(0, 3);
                if(!publishMainEnemyImage(index,0))return;
                enemy.imageIndex = 0;
                enemy.enemyImagePhase = 0; enemy.enemyImageRemainder = 0;
                ++soundPlayRequests_;
            }
            soundPlayRequests_ += effects.wakeSounds;
        } else if (enemy.objectId == 117) {
            const int hitAlarm = enemy.snailHit.consume(random_);
            if (hitAlarm >= 0) {
                if(!publishMainEnemyImage(index,0))return;
                enemy.imageIndex = 0;
                enemy.enemyImagePhase = 0; enemy.enemyImageRemainder = 0;
                enemy.alarms.set(0, 0); enemy.alarms.set(1, hitAlarm);
                ++soundPlayRequests_;
            }
            if (!enemy.snail.initialized)
                enemy.snail.initialize(enemy.x, enemy.y, enemy.velocityY >= 0);
            const bool alive = enemy.snail.step([this](int x, int y) { return mainSolidAt(x, y); },false,
                [this,index](RunnerTransformField field,int value){return publishMainEnemyTransform(index,field,value);});
            if(!run_.effectsSucceeded)return;
            if (!alive && !publishMainInactivation(enemy)) return;
            enemy.active = alive;
            if (!alive) continue;
            if (enemy.active) {
                enemy.preciseX = enemy.snail.x / 1280.0F;
                enemy.preciseY = enemy.snail.y / 1280.0F;
                enemy.velocityY = enemy.snail.vy / 1280.0F;
                enemy.motionState = enemy.snail.facing;
                enemy.scaleX = static_cast<float>(enemy.snail.facing);
                enemy.scaleY = static_cast<float>(enemy.snail.verticalFlip);
            }
        } else if (enemy.objectId == 119) {
            updateCrawler(enemy, false);
        }
        auto publishedX = static_cast<std::int32_t>(roundGameMakerCoordinate(enemy.preciseX));
        auto publishedY = static_cast<std::int32_t>(roundGameMakerCoordinate(enemy.preciseY));
        if ((enemy.objectId == 111 || enemy.objectId == 112) && enemy.walker.initialized) {
            publishedX = enemy.walker.publishedX;
            publishedY = enemy.walker.publishedY;
        }
        if (enemy.objectId == 117 && enemy.snail.initialized) {
            publishedX = static_cast<std::int32_t>(FixedCollisionMath::round(enemy.snail.x) / 1280);
            publishedY = static_cast<std::int32_t>(FixedCollisionMath::round(enemy.snail.y) / 1280);
        }
        if(!lifecycle_.integerPosition(instances_.handleAt(index),enemy,publishedX,publishedY)) {
            run_.effectsSucceeded=false;return;
        }
        enemy.x=publishedX;enemy.y=publishedY;
        if(enemy.objectId==119){
            // Step publishes image_angle after x/y; Alarm 0 never assigns it.
            if(!publishMainEnemyTransform(index,RunnerTransformField::angle,enemy.crawler.imageDirection*90))return;
            enemy.rotation=enemy.crawler.imageDirection*90;
        }
        const auto nextSprite = finishEnemyPresentation(
            {enemy.objectId, enemy.auxiliaryState, enemy.grounded, enemy.velocityY < 0.0F,
             enemy.frogHit.remaining != 0, enemy.snailHit.stunned},
            enemy.frogPresentation, enemy.batActivity, enemy.batHit,
            enemy.enemyImagePhase, enemy.enemyImageRemainder);
        if(enemy.objectId!=111 && enemy.objectId!=112 &&
           !lifecycle_.sprite(instances_.handleAt(index),enemy,nextSprite)) {
            run_.effectsSucceeded=false;return;
        }
        enemy.spriteId=nextSprite;
        if(enemy.objectId==116 && !finishBatFields(enemy.batActivity,nextSprite,
            [this,index,&enemy](int mask){
                if(!lifecycle_.mask(instances_.handleAt(index),enemy,mask)){
                    run_.effectsSucceeded=false;return false;
                }
                enemy.maskId=mask;return true;
            },[this,index,&enemy](int scale){
                if(!publishMainEnemyTransform(index,RunnerTransformField::scaleX,scale))return false;
                enemy.scaleX=static_cast<float>(scale);return true;
            },[&enemy]{enemy.direction=270;enemy.batDirectionFraction=0;enemy.moveSpeed=2;}))return;
        if(!publishMainEnemyImage(index,enemy.enemyImagePhase))return;
        enemy.imageIndex = enemy.enemyImagePhase / 256.0F;
    }
}

void StartupRuntime::updateMainEnemyInteractions() {
    if(!stepEnemyContacts(static_cast<unsigned>(instances_.size()), mainPlayerMotion_,
        mainPlayerHealth_, mainPlayerInvulnerability_, run_.kills,
        run_.combo.state, run_.upgrades.levels[7] != 0,
        [this](unsigned i) -> InstanceRecord& { return instances_.at(i); },
        [this](const InstanceRecord& enemy) {
            return EnemyContactLocation{{
            static_cast<long>(std::round((enemy.preciseX - mainPlayerOriginX_) * 256.0F)),
            static_cast<long>(std::round((enemy.preciseY - mainPlayerOriginY_) * 256.0F)),
            enemy.scaleX < 0 ? -1 : 1, enemy.scaleY < 0 ? -1 : 1,
            enemyContactMask(enemy.objectId, enemy.spriteId,enemy.maskId)},
                enemy.x - mainPlayerOriginX_, enemy.y - mainPlayerOriginY_};
        },
        [this](InstanceRecord& enemy) {
            spawnMainCurrency(static_cast<float>(enemy.x), static_cast<float>(enemy.y),
                              enemy.reward, true);
            return run_.effectsSucceeded;
        },
        [this](EnemyContact) {
            syncMainPlayerState();
            ++soundPlayRequests_;
        },
        [this](int x,int y) {
            return run_.contactDamageNumber(mainPlayerMotion_.frameStartXFixed(),
                mainPlayerMotion_.frameStartYFixed(),x,y,mainPlayerOriginX_,mainPlayerOriginY_);
        },[this](const InstanceRecord& enemy){return publishMainInactivation(enemy);}))return;
}

void StartupRuntime::updateMainLifecycle() {
    // scrOutofview destroys, rather than deactivates, the instance. Erasure
    // compacts records but preserves survivor handles/order. Translate the
    // original traversal index after each removal so adjacent deaths are not
    // skipped. No game callbacks run during this currently supported sweep.
    unsigned removed = 0;
    const bool succeeded=retireMainRoomObjectsChecked(static_cast<unsigned>(instances_.size()), roomId_, 1,
        mainCameraMotion_.viewTopQ12(),
        [this, &removed](unsigned i) {
            const auto& instance = instances_.at(i - removed);
            return LifetimeInstance{instance.objectId, instance.y, instance.active};
        },
        [this, &removed](unsigned i) {
            const auto id = instances_.at(i - removed).instanceId;
            if (!instances_.erase(id,[this](InstanceHandle h,const InstanceRecord& record){return lifecycle_.retire(h,record);})) return false;
            ++removed;
            ++mainInstancesRetired_;
            return true;
        });
    if(!succeeded)run_.effectsSucceeded=false;
}

void StartupRuntime::syncMainBuilderState() {
    mainBuilderY_ = areaOneBuilder_.y();
    mainBuilderGenerationCount_ = areaOneBuilder_.generationCount();
    mainBuilderRowsGenerated_ = areaOneBuilder_.rowsGenerated();
    mainBuilderRightRooms_[0] = areaOneBuilder_.rightRoom(0);
    mainBuilderRightRooms_[1] = areaOneBuilder_.rightRoom(1);
    mainBuilderStartPattern_ = areaOneBuilder_.startPattern();
    mainBuilderStartReversed_ = areaOneBuilder_.startReversed();
    mainBuilderStartRowsGenerated_ = areaOneBuilder_.startRowsGenerated();
    mainBuilderLastMainPattern_ = areaOneBuilder_.lastMainPattern();
    mainBuilderLastPatternReversed_ = areaOneBuilder_.lastPatternReversed();
    mainBuilderMainPatternsGenerated_ = areaOneBuilder_.mainPatternsGenerated();
    mainBuilderSpawnerMade_ = areaOneBuilder_.spawnerMade();
    mainBuilderLastRightPattern_ = areaOneBuilder_.lastRightPattern();
    mainBuilderSideRoomsGenerated_ = areaOneBuilder_.sideRoomsGenerated();
    mainBuilderLastShopPattern_ = areaOneBuilder_.lastShopPattern();
    mainBuilderShopsGenerated_ = areaOneBuilder_.shopsGenerated();
    mainBuilderRightRoomsMade_ = areaOneBuilder_.rightRoomsMade();
    mainBuilderShopMade_ = areaOneBuilder_.shopMade();
    mainBuilderEndReached_ = areaOneBuilder_.endReached();
    mainBuilderEndPattern_ = areaOneBuilder_.endPattern();
    mainBuilderChunkOrCrate_ = areaOneBuilder_.chunkOrCrate();
    mainBuilderSpecialObjectsGenerated_ =
        areaOneBuilder_.specialObjectsGenerated();
}

void StartupRuntime::updateMainBuilder(InstanceRecord& instance) {
    if (!mainBuilderInitialized_ || mainBuilderEndReached_)
        return;

    // scrBuilder only stays 128 pixels ahead of the visible bottom. On the
    // first boot, genCount -30..-2 selects a two-line empty pattern. Adjacent
    // patterns share their last row, so each call advances one 16-pixel row
    // and creates the two boundary pillars at the builder's current y.
    const float viewBottom = mainCameraViewY() + surfaceHeight;
    if (builderPatternPack_ == nullptr)
        return;

    AreaOneBuilderStep result;
    areaOneBuilder_.setPlayerHealth(mainPlayerHealth_);
    const bool generated = areaOneBuilder_.stepVisible(
        *builderPatternPack_, random_, static_cast<int>(std::ceil(viewBottom)),
        [](void* opaque, const DeferredCreateInstance& command) {
            StartupRuntime* runtime = static_cast<StartupRuntime*>(opaque);
            // Shop Create consumes stock/text RNG immediately, before the
            // builder visits its next cell. Do not defer it with plain tiles.
            if (command.objectId == 277)
                return runtime->registerShop(command.x, command.y) != nullptr;
            if (command.objectId == 82 && !runtime->run_.adoptTimeField(
                command.x,command.y,command.timeFieldRadiusWord()))return false;
            // Publish each child before the builder continues to the next
            // cell. Even ordinary terrain/actor births can fail or affect
            // instance queries; a pattern-end batch changes that ordering.
            if (!runtime->deferredInstances_.enqueueCreate(command)) return false;
            runtime->flushDeferredInstances(runtime->instances_, runtime->roomId_);
            return runtime->deferredCommandsSucceeded_&&runtime->run_.effectsSucceeded;
        },
        this, result);
    if (!generated) {
        deferredCommandsSucceeded_ = false;
        return;
    }
    instance.y = areaOneBuilder_.y();
    syncMainBuilderState();
}

bool StartupRuntime::timeShardOutsideView(RunnerImageValue publishedY,bool& retire){
    // fxTimeShard calls outOfView(0,320) before publishing rounded yy to y.
    // The script only destroys in rmMain; other rooms have no retirement.
    if(!run_.effectsSucceeded)return false;
    if(roomId_!=mainRoomId){retire=false;return true;}
    const auto view=MotionBinary64::fromInteger(mainCameraRenderViewY());int order;
    if(!compareRunnerReal(publishedY.promoted(),view,order)){run_.effectsSucceeded=false;return false;}
    if(order<0){retire=true;return true;}
    auto bottom=view;
    if(!bottom.add(MotionBinary64::fromInteger(320))||!compareRunnerReal(publishedY.promoted(),bottom,order)){
        run_.effectsSucceeded=false;return false;
    }
    retire=order>0;return true;
}

bool StartupRuntime::playerOutsideTimeStopSides(MotionBinary64 x,bool& outside){
    if(!run_.effectsSucceeded)return false;
    PackedRoom room;
    run_.effectsSucceeded=roomPack_&&roomPack_->findRoom(roomId_,room)&&
        TimeStopBound::outsideSides(x,room.width,outside);
    return run_.effectsSucceeded;
}

bool StartupRuntime::playerTimeFieldMeeting(MotionBinary64 x,MotionBinary64 y,bool& hit){
    if(!run_.effectsSucceeded)return false;
    for(std::size_t i=0;i<instances_.size();++i){
        const auto& player=instances_.at(i);
        if(player.objectId==35&&player.active&&!player.destroyRequested){
            run_.effectsSucceeded=lifecycle_.placeMeeting(instances_.handleAt(i),player,82,x,y,hit);
            return run_.effectsSucceeded;
        }
    }
    run_.effectsSucceeded=false;return false;
}

bool StartupRuntime::playerTimeFieldInstance(RunnerImageValue x,RunnerImageValue y,InstancePlaceResult& result){
    if(!run_.effectsSucceeded)return false;
    for(std::size_t i=0;i<instances_.size();++i){
        const auto& player=instances_.at(i);
        if(player.objectId==35&&player.active){
            run_.effectsSucceeded=lifecycle_.instancePlace(instances_.handleAt(i),player,82,x.promoted(),y.promoted(),result);
            return run_.effectsSucceeded;
        }
    }
    run_.effectsSucceeded=false;return false;
}

void StartupRuntime::updateMainPlayer(InstanceRecord& instance) {
    if (!mainPlayerInitialized_)
        return;

    // The first operation in recovered objPlayer_n Begin Step, before motion.
    const auto activation = advanceMainRoomActivationChecked(mainActivationY_,
        mainCameraMotion_.viewBottomQ12() / 16, instances_.size(),
        [this](unsigned index) {
            const auto& enemy = instances_.at(index);
            if (enemy.active || !enemy.activationPending)
                return ActivationInstance{enemy.objectId, {}, enemy.active, enemy.activationPending};
            int sprite = enemy.spriteId;
            if (sprite < 0 && objectPack_) {
                PackedObject object;
                if (objectPack_->findObject(enemy.objectId, object)) sprite = object.spriteId;
            }
            const auto bounds = contactWorldBounds({static_cast<long>(std::round(enemy.x * 256.0)),
                static_cast<long>(std::round(enemy.y * 256.0)), enemy.scaleX < 0 ? -1 : 1,
                enemy.scaleY < 0 ? -1 : 1, enemyContactMask(enemy.objectId, sprite,enemy.maskId)});
            return ActivationInstance{enemy.objectId, bounds,
                                      enemy.active, enemy.activationPending};
        },
        [this](unsigned index) {
            // The frontier enumerates live slots without structural mutation.
            // Use the request boundary; membership is reconciled after Begin Step.
            return instances_.requestActivation(instances_.handleAt(index),true,
                [this](InstanceHandle h,const InstanceRecord& record,bool active){return lifecycle_.activation(h,record,active);});
        });
    mainEnemiesActivated_ += activation.activated;
    if(!activation.succeeded){run_.effectsSucceeded=false;return;}

    const bool wasGrounded = mainPlayerMotion_.grounded();
    const unsigned priorShots = mainPlayerMotion_.shots();
    const unsigned priorJumps = mainPlayerMotion_.jumps();
    const int priorAmmo = mainPlayerMotion_.ammo();
    // Rebind adapters per update so copying a runtime cannot leave callbacks
    // pointing into the original instance store.
    mainPlayerMotion_.configureSolidResolver([](void* context, int x, int y) {
        const auto& runtime = *static_cast<StartupRuntime*>(context);
        return runtime.mainSolidAt(x + runtime.mainPlayerOriginX_,
                                   y + runtime.mainPlayerOriginY_) ||
            runtime.run_.shops.playerSolidAt(x + runtime.mainPlayerOriginX_,
                                            y + runtime.mainPlayerOriginY_);
    }, this, 4 - mainPlayerOriginX_, 476 - mainPlayerOriginX_);
    mainPlayerMotion_.configureLandingResolver([](void* context, int x, int minimumY) {
        const auto& runtime = *static_cast<StartupRuntime*>(context);
        const int worldX = x + runtime.mainPlayerOriginX_;
        int nearest = gameplayNoLanding;
        for (std::size_t i = 0; i < runtime.instances_.size(); ++i) {
            const auto& tile = runtime.instances_.at(i);
            if (!tile.active || (tile.objectId != 60 && tile.objectId != 92 &&
                                 tile.objectId != 93)) continue;
            const int landing = tile.y - 16 - runtime.mainPlayerOriginY_;
            if (landing >= minimumY && landing < nearest &&
                worldX + 3 >= tile.x - 8 && worldX - 4 <= tile.x + 7)
                nearest = landing;
        }
        return runtime.run_.shops.playerLanding(worldX, minimumY + runtime.mainPlayerOriginY_,
            nearest + runtime.mainPlayerOriginY_) - runtime.mainPlayerOriginY_;
    }, this);
    mainPlayerMotion_.configureBreakBoxResolver([](void* context, long x, long y, bool& found) {
        auto& runtime = *static_cast<StartupRuntime*>(context);
        PlayerBoxContact::Bounds bounds;
        if(!PlayerBoxContact::prepare(x,y,runtime.mainPlayerOriginX_,runtime.mainPlayerOriginY_,bounds))return false;
        for (std::size_t i = 0; i < runtime.instances_.size(); ++i) {
            auto& box = runtime.instances_.at(i);
            if(!box.active || box.objectId!=92)continue;
            bool hit=false;
            if(!PlayerBoxContact::testPrepared(bounds,box.x,box.y,hit))return false;
            if (hit) {
                if (!runtime.publishMainInactivation(box)) return false;
                box.active = false;
                ++runtime.mainBlocksDestroyed_;
                ++runtime.soundPlayRequests_;
                found=true;
                return true;
            }
        }
        found=false;return true;
    }, this);
    Input input;
    input.left = inputLeft_;
    input.right = inputRight_;
    input.fire = inputFire_;
    const int muzzleX=instance.x,muzzleY=instance.y;
    stepGameplayPlayer(mainPlayerMotion_, mainDeath_, run_, currencyAccount_,
        mainPlayerHealth_, random_, input, mainPlayerOriginX_, mainPlayerOriginY_,
        mainCameraRenderViewX(), mainCameraRenderViewY(), controlsLocked_,
        [this,&instance](int x,int y){
            for(std::size_t i=0;i<instances_.size();++i)if(&instances_.at(i)==&instance){
                if(!lifecycle_.integerPosition(instances_.handleAt(i),instance,x,y))return false;
                instance.x=x;instance.y=y;return true;
            }
            return false;
        },[this,&instance](int value){
            for(std::size_t i=0;i<instances_.size();++i)if(&instances_.at(i)==&instance){
                if(!lifecycle_.integerTransform(instances_.handleAt(i),instance,RunnerTransformField::scaleX,
                        value,{0x3f800000u},0xffffffu))return false;
                instance.scaleX=static_cast<float>(value);return true;
            }
            return false;
        },[this,&instance](RunSession& run,MotionBinary64 x,MotionBinary64 y,int facing,bool& hit){
            if(!lifecycle_.hasPlaceMeeting())return run.playerGoalContact(x,y,facing,hit);
            for(std::size_t i=0;i<instances_.size();++i)if(&instances_.at(i)==&instance)
                return lifecycle_.placeMeeting(instances_.handleAt(i),instance,255,
                    MotionBinary64::fromInteger(instance.x),MotionBinary64::fromInteger(instance.y),hit);
            return false;
        },[this,&instance](const PlayerMotion&,int,int){
            return !timeEffectDispatch_||timeEffectDispatch_(timeEffectContext_,instance,TimeEffectPhase::playerBegin,0);
        });
    mainPlayerMotion_.configureSolidResolver(nullptr, nullptr,
        4 - mainPlayerOriginX_, 476 - mainPlayerOriginX_);
    mainPlayerMotion_.configureLandingResolver(nullptr, nullptr);
    mainPlayerMotion_.configureBreakBoxResolver(nullptr, nullptr);
    if(!run_.effectsSucceeded)return;
    syncMainPlayerState();
    if (mainJumps_ != priorJumps) ++soundPlayRequests_;
    if (mainShotsFired_ != priorShots) emitMainBullet(muzzleX,muzzleY);
    if (!run_.effectsSucceeded) return;
    if (!wasGrounded && mainPlayerGrounded_) {
        ++mainLandings_;
        if (priorAmmo < mainAmmo_) ++soundPlayRequests_;
    }
    instance.imageIndex = mainPlayerImageIndex_;
}

void StartupRuntime::updateMainCamera(InstanceRecord& instance) {
    if (!mainCameraInitialized_ || !mainPlayerInitialized_)
        return;
    mainCameraMotion_.step(static_cast<long>(std::round(mainPlayerX_ * 256.0F)),
                           static_cast<long>(std::round(mainPlayerY_ * 256.0F)), mainDeath_.active,
                           run_.sideCameraLockers.overlaps(mainCameraMotion_));
    mainCameraX_ = mainCameraMotion_.x / 4096.0F;
    mainCameraY_ = mainCameraMotion_.y / 4096.0F;
    mainCameraTargetY_ = mainCameraMotion_.targetY / 4096.0F;
    instance.x = mainCameraMotion_.pixelX();
    instance.y = mainCameraMotion_.pixelY();
}

void StartupRuntime::updateMenuCamera() {
    if (!menuPlayerInitialized_)
        return;
    const bool inCameraLocker = menuPlayerX_ >= 238.0F &&
                                menuPlayerX_ <= 418.0F &&
                                menuPlayerY_ >= 248.0F &&
                                menuPlayerY_ <= 648.0F;
    float targetX = inCameraLocker ? 328.0F : menuPlayerX_;
    if (!inCameraLocker && menuPlayerSpeedX_ != 0.0F)
        targetX += 16.0F * menuPlayerFacing_;
    if (targetX < 80.0F)
        targetX = 80.0F;
    if (targetX > 336.0F)
        targetX = 336.0F;
    const float targetY = inCameraLocker ? menuCameraLockerY_
                                         : menuPlayerY_ - 32.0F;
    menuCameraX_ += (targetX - menuCameraX_) /
                    (inCameraLocker ? 15.0F : 10.0F);
    menuCameraY_ += (targetY - menuCameraY_) / 5.0F;
    if (menuCameraY_ < 142.0F)
        menuCameraY_ = 142.0F;
}

void StartupRuntime::dispatchInstancePhase(InstanceRecord& instance,
                                           RoomInstancePhase phase) {
    if (objectPack_ == nullptr) {
        if (phase == RoomInstancePhase::create && instance.objectId == 37)
            controllerCreate(profile_);
        if (phase == RoomInstancePhase::create && instance.objectId == 36)
            initRoomCreate();
        return;
    }

    switch (phase) {
    case RoomInstancePhase::preCreate:
        dispatchObjectEvent(instance, 14, 0);
        break;
    case RoomInstancePhase::create:
        dispatchObjectEvent(instance, 0, 0);
        break;
    case RoomInstancePhase::instanceCreation:
        if (instance.creationCodeId != -1)
            executeCode(&instance, instance.creationCodeId);
        break;
    case RoomInstancePhase::gameStart:
        dispatchObjectEvent(instance, 7, 2);
        break;
    case RoomInstancePhase::roomStart:
        dispatchObjectEvent(instance, 7, 4);
        break;
    }
}

bool StartupRuntime::dispatchObjectEvent(InstanceRecord& instance,
                                         std::int32_t eventType,
                                         std::int32_t eventSubtype,
                                         bool advancePressed) {
    PackedObjectEvent event;
    if (objectPack_ == nullptr ||
        !objectPack_->findEvent(instance.objectId, eventType, eventSubtype, event))
        return false;
    if(eventType==3&&eventSubtype==0&&instance.objectId==238&&timeEffectDispatch_){
        ++implementedCodeCalls_;
        run_.effectsSucceeded=timeEffectDispatch_(timeEffectContext_,instance,TimeEffectPhase::step,0);
        return run_.effectsSucceeded;
    }
    if(eventType==2&&(instance.objectId==211||instance.objectId==238)&&timeEffectDispatch_){
        ++implementedCodeCalls_;
        run_.effectsSucceeded=timeEffectDispatch_(timeEffectContext_,instance,TimeEffectPhase::alarm,unsigned(eventSubtype));
        return run_.effectsSucceeded;
    }
    executeCode(&instance, event.codeId, advancePressed);
    return true;
}

void StartupRuntime::dispatchRoomCreation(std::int32_t roomId) {
    std::int32_t codeId = -1;
    if (roomPack_ != nullptr) {
        PackedRoom room;
        if (roomPack_->findRoom(roomId, room))
            codeId = room.creationCodeId;
    } else if (roomId == entryRoomId) {
        codeId = 648;
    } else if (roomId == menuRoomId) {
        codeId = 714;
    }
    if (codeId != -1)
        executeCode(nullptr, codeId);
}

void StartupRuntime::executeCode(InstanceRecord* instance,
                                 std::int32_t codeId,
                                 bool advancePressed) {
    switch (codeId) {
    case 2731: // startTimeField Create: scales, radius, then first-level destruction
        ++implementedCodeCalls_;
        if(instance!=nullptr){
            for(std::size_t i=0;i<instances_.size();++i)if(&instances_.at(i)==instance){
                const auto handle=instances_.handleAt(i);
                if(!lifecycle_.integerTransform(handle,*instance,RunnerTransformField::scaleX,2,{0x3f800000u},0xffffffu)){
                    run_.effectsSucceeded=false;return;
                }
                instance->scaleX=2.0F;
                if(!lifecycle_.integerTransform(handle,*instance,RunnerTransformField::scaleY,2,{0x3f800000u},0xffffffu)){
                    run_.effectsSucceeded=false;return;
                }
                instance->scaleY=2.0F;
                if(!run_.createStartField(instance->x,instance->y,random_))return;
                if(!run_.startFieldPresent&&!instances_.erase(instance->instanceId,
                    [this](InstanceHandle h,const InstanceRecord& r){return lifecycle_.retire(h,r);}))run_.effectsSucceeded=false;
                return;
            }
            run_.effectsSucceeded=false;
        }
        return;
    case 2461: // objLineToResult Create -> inherited objLineToMain Create
        ++implementedCodeCalls_;
        if(instance!=nullptr){
            // The inherited event centers the parent before either child is
            // born. Its custom `active=0` is LevelExit::active, NOT the runner
            // instance activation flag.
            for(std::size_t i=0;i<instances_.size();++i)if(&instances_.at(i)==instance){
                if(!lifecycle_.integerPosition(instances_.handleAt(i),*instance,240,instance->y)){
                    run_.effectsSucceeded=false;return;
                }
                instance->x=240;instance->preciseX=240.0F;
                run_.createLevelExit(480,instance->y,random_,mainCameraMotion_,
                    [this](LevelExit::Effect effect,int x,int y){return createLevelExitChild(effect,x,y);});
                return;
            }
            run_.effectsSucceeded=false;
        }
        return;
    case 649: // gml_Room_rmPlayMenu_Create
        ++implementedCodeCalls_;
        cameraViewWidth_ = surfaceWidth;
        cameraViewHeight_ = surfaceHeight;
        return;
    case 674: // gml_Room_rmMain_Create
        ++implementedCodeCalls_;
        cameraViewWidth_ = surfaceWidth;
        cameraViewHeight_ = surfaceHeight;
        controlsLocked_ = false;
        return;
    case 648: // gml_Room_rmInit_Create
        ++implementedCodeCalls_;
        cameraViewWidth_ = surfaceWidth;
        cameraViewHeight_ = surfaceHeight;
        return;
    case 714: // gml_Room_rmMenu_Create
        ++implementedCodeCalls_;
        cameraViewWidth_ = surfaceWidth;
        cameraViewHeight_ = surfaceHeight;
        backgroundMusicId_ = 193; // bgmOverworld
        ++soundGainRequests_;
        ++soundPlayRequests_;
        ++ditherFadeSpawnRequests_; // object 382; applied by deferred creation later.
        if (objectPack_ != nullptr &&
            !deferredInstances_.enqueueCreate(382, 0, 0))
            deferredCommandsSucceeded_ = false;
        controlsLocked_ = true;
        return;
    case 836: // gml_Object_objPlayer_n_Create_0 (startup-visible subset)
        ++implementedCodeCalls_;
        if (instance != nullptr && objectPack_ != nullptr) {
            if (instance->roomId == menuRoomId) {
                const int napPose = static_cast<int>(random_.irandom(5));
                constexpr int napSprites[] = {11, 12, 14, 13, 15, 14};
                constexpr float napX[] = {112, 112, 290, 80, 295, 290};
                constexpr float napY[] = {506, 512, 496, 512, 496, 496};
                constexpr float napSpeed[] = {0.015F, 0.15F, 0.0F,
                                               0.0F, 0.25F, 0.0F};
                menuPlayerInitialized_ = true;
                menuPlayerNapping_ = true;
                menuPlayerX_ = napX[napPose];
                menuPlayerY_ = napY[napPose];
                menuPlayerSpriteId_ = napSprites[napPose];
                menuPlayerImageSpeed_ = napSpeed[napPose];
                instance->x = static_cast<int>(menuPlayerX_);
                instance->y = static_cast<int>(menuPlayerY_);
                menuCameraX_ = menuPlayerX_;
                menuCameraY_ = menuPlayerY_;
            } else if (instance->roomId == mainRoomId) {
                mainPlayerInitialized_ = true;
                mainPlayerOriginX_ = instance->x;
                mainPlayerOriginY_ = instance->y;
                mainPlayerMotion_.enterRoom(inputFire_);
                mainDeath_ = {};
                syncMainPlayerState();
                mainPlayerInvulnerability_ = 0;
                mainEmitter_.reset(run_.upgrades.levels[12] != 0);
                mainShotsFired_ = 0;
                // These are adapter event counters, not game globals. Rebase
                // them so a new instance is not mistaken for a shot/recharge.
                run_.ammoHud.previousShots = 0;
                run_.ammoHud.previousRecharges = 0;
                mainBlocksDestroyed_ = 0;
                mainLandings_ = 0;
                mainActivationY_ = 0;
                mainEnemiesActivated_ = 0;
                mainInstancesRetired_ = 0;
                mainJumps_ = 0;
                mainBulletCount_ = 0;
                mainCurrencyCount_ = 0;
                currencyAccount_.playerCreated();
                // The shared player now owns the default gun's countdown.
            }
            // scrPlayerInit's full state comes later. These are the two
            // instance_create calls made directly by the recovered event.
            if (!deferredInstances_.enqueueCreate(66, -100, instance->y) ||
                !deferredInstances_.enqueueCreate(64, instance->x, instance->y))
                deferredCommandsSucceeded_ = false;
        }
        return;
    case 849: // gml_Object_objPlayer_n_Step_1 (menu/rmMain movement subsets)
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            if (instance->roomId == menuRoomId)
                updateMenuPlayer(*instance);
            if (instance->roomId == mainRoomId)
                updateMainPlayer(*instance);
        }
        return;
    case 845: // gml_Object_objPlayer_n_Alarm_2
        ++implementedCodeCalls_;
        // Countdown is owned by PlayerMotion, not a second host alarm.
        return;
    case 1002: // gml_Object_objBuilder_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr && instance->roomId == mainRoomId) {
            mainBuilderInitialized_ = true;
            AreaOneBuilder::Setup setup;
            setup.level = run_.stage.level > 0 ? unsigned(run_.stage.level) : 1;
            setup.style = unsigned(playStyle_);
            setup.member = run_.upgrades.levels[14] != 0;
            areaOneBuilder_.reset(instance->x, instance->y, random_, setup);
            syncMainBuilderState();
        }
        return;
    case 1003: // gml_Object_objBuilder_Step_0
        ++implementedCodeCalls_;
        if (instance != nullptr && instance->roomId == mainRoomId)
            updateMainBuilder(*instance);
        return;
    case 1004: // gml_Object_objBuilder_PreCreate_0 (event_inherited; no parent)
        ++implementedCodeCalls_;
        return;
    case 985: // gml_Object_objWall_n_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr)
            instance->imageSpeed = 0.0F;
        return;
    case 1047: // gml_Object_Spawner_Create_0 / spawnerInit
        ++implementedCodeCalls_;
        return;
    case 1085: // gml_Object_objBox_n_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr)
            instance->imageSpeed = 0.0F;
        return;
    case 1090: // gml_Object_objThinPlatform_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            instance->imageIndex = 23.0F;
            instance->imageSpeed = 0.0F;
        }
        return;
    case 1193: // gml_Object_enmSnake_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            applyEnemyCreateStats(*instance, 111);
            if (!instance->motionInitialized) {
                instance->preciseX = static_cast<float>(instance->x);
                instance->preciseY = static_cast<float>(instance->y);
                instance->velocityX = 0.5F;
                instance->motionInitialized = true;
                instance->grounded = true;
            }
            instance->walker.initialize(instance->x, instance->y,
                static_cast<long>(std::round(instance->velocityX * 1280)),
                static_cast<long>(std::round(instance->velocityY * 1280)), instance->grounded);
        }
        return;
    case 1198: // gml_Object_enmGroundTurtle_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            applyEnemyCreateStats(*instance, 112);
            if (!instance->motionInitialized) {
                instance->preciseX = static_cast<float>(instance->x);
                instance->preciseY = static_cast<float>(instance->y);
                instance->velocityX = 0.25F;
                instance->motionInitialized = true;
                instance->grounded = true;
            }
            instance->walker.initialize(instance->x, instance->y,
                static_cast<long>(std::round(instance->velocityX * 1280)),
                static_cast<long>(std::round(instance->velocityY * 1280)), instance->grounded);
        }
        return;
    case 1204: // gml_Object_enmFrog_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            applyEnemyCreateStats(*instance, 113);
            if (!instance->motionInitialized) {
                instance->preciseX = static_cast<float>(instance->x);
                instance->preciseY = static_cast<float>(instance->y);
                instance->motionInitialized = true;
            }
            // Create assigns xx=x and yy=y before any Step can observe them.
            // Keep the shared motion state initialized at the same boundary.
            instance->frogHorizontal.position = static_cast<long>(std::round(
                (instance->preciseX - mainPlayerOriginX_) * 65536.0F));
            instance->frogHorizontal.velocity = 0;
            instance->frogVertical.position = static_cast<long>(std::round(
                (instance->preciseY - mainPlayerOriginY_) * 1280.0F));
            instance->frogVertical.velocity = 0;
            instance->frogVertical.grounded = false;
            instance->frogMotionInitialized = true;
        }
        return;
    case 1230: // gml_Object_enmBat_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            applyEnemyCreateStats(*instance, 116);
            if (!instance->motionInitialized) {
                instance->preciseX = static_cast<float>(instance->x);
                instance->preciseY = static_cast<float>(instance->y);
                instance->moveSpeed = 1.25F;
                instance->motionInitialized = true;
            }
        }
        return;
    case 1237: // gml_Object_enmSnail_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            applyEnemyCreateStats(*instance, 117);
            if (!instance->motionInitialized) {
                instance->preciseX = static_cast<float>(instance->x);
                instance->preciseY = static_cast<float>(instance->y);
                instance->velocityY = 0.1F;
                instance->motionInitialized = true;
            }
            instance->snail.initialize(instance->x, instance->y, instance->velocityY >= 0);
        }
        return;
    case 1252: // gml_Object_enmCrawler_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            applyEnemyCreateStats(*instance, 119);
            if (!instance->motionInitialized) {
                instance->preciseX = static_cast<float>(instance->x);
                instance->preciseY = static_cast<float>(instance->y);
                instance->moveSpeed = 1.0F;
                instance->motionState = 1;
                instance->motionInitialized = true;
            }
            instance->crawler.initialize(instance->x, instance->y);
        }
        return;
    case 1231: // gml_Object_enmBat_Alarm_0
        ++implementedCodeCalls_;
        if (instance != nullptr) instance->batHit.alarm();
        return;
    case 1199: // gml_Object_enmGroundTurtle_Alarm_1
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            instance->auxiliaryState *= -1;
            instance->alarms.set(1, 120 + static_cast<int>(random_.irandom(180)));
        }
        return;
    case 1206: // gml_Object_enmFrog_Alarm_1
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            const int direction = frogPreparationDirection(instance->grounded,
                static_cast<long>(instance->x) * 256L,
                static_cast<long>(instance->y) * 256L,
                mainPlayerOriginX_ * 256L + mainPlayerMotion_.frameStartXFixed(),
                static_cast<long>(std::round(mainCameraViewX() * 256.0F)),
                static_cast<long>(std::round(mainCameraViewY() * 256.0F)),
                static_cast<long>(std::round((mainCameraViewX() + surfaceWidth) * 256.0F)),
                static_cast<long>(std::round((mainCameraViewY() + surfaceHeight) * 256.0F)));
            applyFrogAlarm(*instance, frogPreparationAlarm(direction));
        }
        return;
    case 1207: // gml_Object_enmFrog_Alarm_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            applyFrogAlarm(*instance, frogJumpAlarm(instance->grounded, instance->motionState));
        }
        return;
    case 1254: // gml_Object_enmCrawler_Alarm_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            updateCrawler(*instance, true);
            instance->alarms.set(0, 2);
        }
        return;
    case 2385: // gml_Object_Scattered16_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr)
            instance->imageSpeed = 0.0F;
        return;
    case 865: // gml_Object_objInitRm_Create_0
        ++implementedCodeCalls_;
        initRoomCreate();
        return;
    case 866: // gml_Object_objInitRm_Alarm_0
        ++implementedCodeCalls_;
        initRoomAlarm0();
        return;
    case 867: // gml_Object_objInitRm_Step_2
        ++implementedCodeCalls_;
        if (roomId_ == entryRoomId)
            initRoomEndStep(advancePressed);
        return;
    case 869: // gml_Object_objControlerN_Create_0
        ++implementedCodeCalls_;
        controllerCreate(profile_);
        return;
    case 2435: // gml_Object_objTeleportMain_Create_0
        ++implementedCodeCalls_;
        menuTeleportActive_ = false;
        menuTeleportSequenceStarted_ = false;
        return;
    case 2436: // gml_Object_objTeleportMain_Alarm_1
        ++implementedCodeCalls_;
        if (objectPack_ != nullptr &&
            !deferredInstances_.enqueueCreate(69, 0, 0))
            deferredCommandsSucceeded_ = false;
        return;
    case 2438: // gml_Object_objTeleportMain_Step_1
        ++implementedCodeCalls_;
        if (instance != nullptr && menuTeleportActive_ && menuTitleReady_ &&
            !menuTeleportSequenceStarted_) {
            instance->alarms.set(1, 30);
            menuTeleportSequenceStarted_ = true;
        }
        return;
    case 967: // gml_Object_PlayMenu_Create_0 (no-save default)
        ++implementedCodeCalls_;
        playMenuCursor_ = playStyle_;
        playMenuYOffset_ = -128.0F;
        playMenuTileY_ = 0.0F;
        playMenuDitherY_ = 80.0F;
        playMenuRunFrame_ = 0.0F;
        playMenuSmoothX_ = 0.0F;
        playMenuSelected_ = false;
        playMenuPlayerDescent_ = 0.0F;
        playMenuPlayerSpeedY_ = 0.0F;
        playMenuVerticalDitherY_ = -320.0F;
        return;
    case 968: // gml_Object_PlayMenu_Step_1 plus deterministic Draw state
        ++implementedCodeCalls_;
        playMenuYOffset_ -= playMenuYOffset_ / 15.0F;
        if (std::abs(playMenuYOffset_) < 1.0F)
            playMenuYOffset_ = 0.0F;
        playMenuRunFrame_ += 0.25F;
        playMenuTileY_ -= 6.0F;
        if (playMenuTileY_ <= -16.0F)
            playMenuTileY_ += 16.0F;
        playMenuDitherY_ += 16.0F;
        if (playMenuYOffset_ == 0.0F && !playMenuSelected_) {
            if (inputRightPressed_ && playMenuCursor_ < 4)
                ++playMenuCursor_;
            if (inputLeftPressed_ && playMenuCursor_ > 0)
                --playMenuCursor_;
            // global.dUp is the edge-triggered Space/gamepad confirm action.
            // With no save data, only style zero is unlocked.
            if (advancePressed && playMenuCursor_ == 0) {
                playStyle_ = playMenuCursor_;
                playMenuSelected_ = true;
                ++soundPlayRequests_;
            }
        }
        playMenuSmoothX_ +=
            (playMenuCursor_ * 48.0F - playMenuSmoothX_) / 5.0F;
        if (std::abs(playMenuCursor_ * 48.0F - playMenuSmoothX_) < 1.0F)
            playMenuSmoothX_ = playMenuCursor_ * 48.0F;
        if (playMenuSelected_) {
            playMenuPlayerSpeedY_ += 0.05F;
            playMenuPlayerDescent_ += playMenuPlayerSpeedY_;
            if (playMenuVerticalDitherY_ < 320.0F) {
                playMenuVerticalDitherY_ += 10.0F;
            } else {
                run_.stage.advance(1); // PlayMenu Draw calls scrNextLevel(1).
                gotoRoom(mainRoomId);
            }
        }
        return;
    case 1014: // gml_Object_EnterSequence_Create_0
        ++implementedCodeCalls_;
        enterSequenceActive_ = true;
        enterSequenceBlackSet_ = false;
        enterSequenceY_ = menuCameraLockerY_;
        enterSequenceSpeedY_ = 0.0F;
        if (instance != nullptr)
            instance->y = static_cast<std::int32_t>(enterSequenceY_);
        return;
    case 1015: // gml_Object_EnterSequence_Alarm_0
        ++implementedCodeCalls_;
        gotoRoom(playMenuRoomId);
        return;
    case 1016: // gml_Object_EnterSequence_Step_0
        ++implementedCodeCalls_;
        if (instance != nullptr && enterSequenceActive_) {
            if (enterSequenceY_ < 1000.0F) {
                enterSequenceY_ += enterSequenceSpeedY_;
            } else if (!enterSequenceBlackSet_) {
                enterSequenceBlackSet_ = true;
                instance->alarms.set(0, 5);
            }
            enterSequenceSpeedY_ += 0.25F;
            menuCameraLockerY_ = enterSequenceY_;
            instance->y = static_cast<std::int32_t>(std::round(enterSequenceY_));
        }
        return;
    case 994: // gml_Object_sideCamLocker_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr && instance->roomId == mainRoomId) {
            instance->x = SideCameraLocker::createdX(instance->x);
            if (!registerSideCameraLocker(instance->x, instance->y)) deferredCommandsSucceeded_ = false;
        }
        return;
    case 997: // gml_Object_camMain_Create_0 (menu/rmMain camera subsets)
        ++implementedCodeCalls_;
        if (menuPlayerInitialized_) {
            menuCameraX_ = menuPlayerX_;
            menuCameraY_ = menuPlayerY_;
        }
        if (instance != nullptr && instance->roomId == mainRoomId) {
            mainCameraInitialized_ = true;
            mainCameraX_ = static_cast<float>(instance->x);
            mainCameraY_ = static_cast<float>(instance->y);
            mainCameraTargetY_ = mainCameraY_;
            mainCameraMotion_.reset(instance->x, instance->y);
        }
        return;
    case 999: // gml_Object_camMain_Step_0 (menu/rmMain camera subsets)
        ++implementedCodeCalls_;
        if (instance != nullptr && instance->roomId == mainRoomId)
            updateMainCamera(*instance);
        else
            updateMenuCamera();
        return;
    case 2478: { // gml_Object_objTree_Create_0
        ++implementedCodeCalls_;
        const std::uint32_t choice = random_.chooseIndexSmall(6);
        if (instance != nullptr) {
            instance->imageIndex = choice == 5 ? 1.0F : 0.0F;
            instance->imageSpeed = 0.0F;
        }
        return;
    }
    case 2480: // gml_Object_groundTrees_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            instance->imageSpeed = 0.0F;
            instance->imageIndex = static_cast<float>(random_.irandom(3));
        }
        return;
    case 2486: // gml_Object_objGrass_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            instance->spriteId = 635 + static_cast<std::int32_t>(
                random_.chooseIndexSmall(4));
            instance->imageSpeed =
                static_cast<float>(0.15 + random_.random(0.1));
            instance->imageIndex = static_cast<float>(random_.irandom(3));
            if (random_.random(10.0) < 1.0)
                instance->active = false;
        }
        return;
    case 2490: // gml_Object_objWell_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr && objectPack_ != nullptr) {
            const bool enqueued =
                deferredInstances_.enqueueCreate(98, instance->x, instance->y) &&
                deferredInstances_.enqueueCreate(98, instance->x + 16, instance->y) &&
                deferredInstances_.enqueueCreate(98, instance->x + 96, instance->y) &&
                deferredInstances_.enqueueCreate(98, instance->x + 112, instance->y) &&
                deferredInstances_.enqueueCreate(317, instance->x + 56,
                                                  instance->y - 80);
            if (!enqueued)
                deferredCommandsSucceeded_ = false;
        }
        return;
    case 2495: // gml_Object_objTitle_Create_0 (first-boot path)
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            instance->imageSpeed = 0.0F;
            instance->imageIndex = 0.0F;
            menuTitleFirstBoot_ = false;
            menuTitleReady_ = false;
            menuTitleStartY_ = static_cast<float>(instance->y);
        }
        return;
    case 2496: // gml_Object_objTitle_Alarm_2
        ++implementedCodeCalls_;
        if (instance != nullptr)
            instance->imageSpeed = 0.32F;
        return;
    case 2497: // gml_Object_objTitle_Alarm_1
        ++implementedCodeCalls_;
        menuTitleReady_ = true;
        return;
    case 2498: // gml_Object_objTitle_Alarm_0 (visual effects omitted)
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            consumeTitleBurstRandom(random_);
            instance->alarms.set(1, 75);
        }
        return;
    case 2499: // gml_Object_objTitle_Step_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            if (!menuPlayerNapping_ &&
                instance->x > menuCameraX_ - 4.0F &&
                instance->x < menuCameraX_ + 4.0F &&
                !menuTitleFirstBoot_) {
                instance->alarms.set(2, 15);
                menuTitleFirstBoot_ = true;
            }
            if (menuTitleFirstBoot_ && instance->imageIndex < 9.0F)
                consumeTitleRiseRandom(random_);
            if (instance->imageSpeed > 0.0F) {
                instance->imageIndex += instance->imageSpeed;
                if (instance->imageIndex >= 9.0F) {
                    instance->imageIndex = 9.0F;
                    instance->imageSpeed = 0.0F;
                    instance->alarms.set(0, 1);
                }
            }
            instance->y = static_cast<std::int32_t>(std::round(
                menuTitleStartY_ - 3.0F * (instance->imageIndex / 9.0F)));
        }
        return;
    case 2790: // gml_Object_ditherFade_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            instance->imageSpeed = 0.0F;
            instance->alarms.set(0, 30);
        }
        return;
    case 2791: // gml_Object_ditherFade_Alarm_0
        ++implementedCodeCalls_;
        if (instance != nullptr)
            instance->imageSpeed = 0.2F;
        controlsLocked_ = false;
        return;
    case 2794: // gml_Object_ditherFade_PreCreate_0 (event_inherited; no parent)
        ++implementedCodeCalls_;
        return;
    default:
        ++unimplementedCodeCalls_;
        return;
    }
}

void StartupRuntime::registerLevelExit(int y) {
    if(!deferredCommandsSucceeded_ || !run_.effectsSucceeded)return;
    if(objectPack_==nullptr){run_.createLevelExit(480,y,random_,mainCameraMotion_);return;}
    if(!deferredInstances_.enqueueCreate(306,240,y)){deferredCommandsSucceeded_=false;return;}
    flushDeferredInstances(instances_,roomId_);
}

bool StartupRuntime::createLevelExitChild(LevelExit::Effect effect,int x,int y) {
    // A separate queue permits synchronous child Create without reapplying the
    // parent's in-flight batch. These two children's PreCreate chains contain
    // only event_inherited, ending at no parent. Shared camera/radius writes
    // run immediately after this publication in RunSession::createLevelExit.
    DeferredInstanceQueue<1> child;
    const int objectId=effect==LevelExit::Effect::roomEndSetter?74:255;
    if(!objectPack_ || !child.enqueueCreate(objectId,x,y))return false;
    return child.applyChecked(instances_,*objectPack_,roomId_,nextRuntimeInstanceId_,
        [&](InstanceRecord& record){
            if(objectId==255){
                const auto handle=instances_.handleAt(instances_.size()-1);
                if(!lifecycle_.integerTransform(handle,record,RunnerTransformField::scaleX,2,
                                                {0x3f800000u},0xffffffu))return false;
                record.scaleX=2.0F;
                if(!lifecycle_.integerTransform(handle,record,RunnerTransformField::scaleY,2,
                                                {0x3f800000u},0xffffffu))return false;
                record.scaleY=2.0F;
            }
            return true;
        },lifecycle_)==ApplyInstanceCommandsResult::success;
}

void StartupRuntime::flushDeferredInstances(
    InstanceStore<instanceCapacity>& instances,
    std::int32_t roomId) {
    if (objectPack_ == nullptr || deferredInstances_.size() == 0)
        return;
    if(!deferredCommandsSucceeded_ || !run_.effectsSucceeded)return;
    const auto createHandler = [this](InstanceRecord& instance) {
        dispatchInstancePhase(instance, RoomInstancePhase::preCreate);
        if(!deferredCommandsSucceeded_ || !run_.effectsSucceeded)return false;
        dispatchInstancePhase(instance, RoomInstancePhase::create);
        return deferredCommandsSucceeded_ && run_.effectsSucceeded;
    };
    const ApplyInstanceCommandsResult result = deferredInstances_.applyChecked(
        instances, *objectPack_, roomId, nextRuntimeInstanceId_, createHandler,lifecycle_);
    deferredCommandsSucceeded_ = deferredCommandsSucceeded_ &&
                                 result == ApplyInstanceCommandsResult::success;
}

void StartupRuntime::initRoomAlarm0() {
    InstanceRecord* instance = initRoomInstance();
    if (instance == nullptr)
        return;
    ++showSplash_;
    instance->alarms.set(0, splashSpeed_);
    if (showSplash_ == 4)
        instance->alarms.set(0, 240);
    if (showSplash_ >= 5)
        instance->alarms.set(0, 60);
}

void StartupRuntime::initRoomEndStep(bool advancePressed) {
    if (!roomDestinationSet_) {
        ++achievementLoginCalls_;

        // The recovered code assigns rRand = 0. With all five weights set to
        // one, the first branch deterministically selects rmMenu (ID 20).
        roomDestination_ = menuRoomId;
        roomDestinationSet_ = true;
        if (showSplash_ != 0)
            initRoomInstance()->alarms.set(0, 10);
        else
            gotoRoom(roomDestination_);
    }

    if (showSplash_ != 0) {
        if (advancePressed) {
            ++showSplash_;
            initRoomInstance()->alarms.set(0, 60);
        }
        if (showSplash_ >= 7) {
            showSplash_ = 0;
            gotoRoom(roomDestination_);
        }
    }
}

bool StartupRuntime::loadNextAreaOneLevel(const Input& input) {
    // Later-area builders are not implemented: retain their pending request
    // rather than silently generating another area-one room with a new label.
    if (!run_.levelClearActive || !run_.nextLevelRequested ||
        run_.stage.area != 1 || run_.stage.level >= 3 || !roomPack_ || !objectPack_) return false;
    if (!run_.advanceAfterLevelClear()) return false;
    run_.shops.reset();
    run_.flashEffects.clear();
    run_.smokeEffects.clear();
    run_.bloodEffects.clear();
    run_.clearCorpseExpiry();
    run_.clearDamageNumbers();
    run_.timeFields.clear();
    run_.sideCameraLockers.clear();
    mainCameraInitialized_ = false;
    mainCameraMotion_ = {};
    inputFire_ = input.fire;
    inputLeft_ = input.left;
    inputRight_ = input.right;
    inputFirePressed_ = inputFireReleased_ = inputLeftPressed_ = inputRightPressed_ = false;
    gotoRoom(mainRoomId);
    return roomLoadSucceeded_ && deferredCommandsSucceeded_;
}

void StartupRuntime::gotoRoom(int roomId) {
    loadRoom(roomId, false);
}

void StartupRuntime::loadRoom(int roomId, bool firstGameRoom) {
    if(!deferredCommandsSucceeded_ || !run_.effectsSucceeded){roomLoadSucceeded_=false;return;}
    const auto instanceHandler = [this](InstanceRecord& instance,
                                        RoomInstancePhase phase) {
        dispatchInstancePhase(instance, phase);
        return deferredCommandsSucceeded_ && run_.effectsSucceeded;
    };
    const auto roomCreationHandler = [this](std::int32_t loadedRoomId) {
        dispatchRoomCreation(loadedRoomId);
        return deferredCommandsSucceeded_ && run_.effectsSucceeded;
    };
    const auto deferredHandler = [this, roomId](
                                     InstanceStore<instanceCapacity>& instances) {
        flushDeferredInstances(instances, roomId);
        return deferredCommandsSucceeded_ && run_.effectsSucceeded;
    };
    if (roomPack_ != nullptr) {
        roomLoadSucceeded_ = PackedRoomLoader<instanceCapacity>::enterChecked(
            instances_, *roomPack_, roomId, firstGameRoom,
            instanceHandler, roomCreationHandler,
            deferredHandler,objectPack_,lifecycle_) == PackedRoomLoadResult::success;
    } else {
        // Bounded fallback's rmInit instances, in recovered creation order.
        const RoomInstanceDefinition definitions[] = {
            {controllerInstanceId, 37, true},
            {initRoomInstanceId, 36, false}
        };
        roomLoadSucceeded_ = RoomLoader<instanceCapacity>::enterChecked(
            instances_, roomId, firstGameRoom ? definitions : nullptr,
            firstGameRoom ? 2 : 0, firstGameRoom,
            instanceHandler, roomCreationHandler,
            deferredHandler,lifecycle_) == LoadRoomResult::success;
    }
    if (roomLoadSucceeded_) {
        roomId_ = roomId;
        // Original room entry reconciles after Room Start (0x510813), before
        // the next frame's active-list animation traversal.
        roomLoadSucceeded_=instances_.reconcileObjectMembership(
            [this](InstanceHandle key,const InstanceRecord& record,bool member){
                return lifecycle_.membership(key,record,member);
            });
    }
}

InstanceRecord* StartupRuntime::initRoomInstance() {
    return instances_.find(initRoomInstanceId);
}

const InstanceRecord* StartupRuntime::initRoomInstance() const {
    return instances_.find(initRoomInstanceId);
}

bool StartupRuntime::controllerPersistent() const {
    const InstanceRecord* controller = instances_.find(controllerInstanceId);
    return controller != nullptr && controller->persistent;
}

int StartupRuntime::alarm0() const {
    const InstanceRecord* instance = initRoomInstance();
    return instance != nullptr ? instance->alarms.get(0) : AlarmBank::inactive;
}

std::uint64_t StartupRuntime::stateDigest() const {
    StateDigest digest;
    digest.addU32(47); // Offscreen destruction removes stored identities, not just activation.
    digest.addU64(tick_);
    digest.addI32(roomId_);
    digest.addI32(roomDestination_);
    digest.addI32(showSplash_);
    digest.addI32(initTimes_);
    digest.addI32(achievementLoginCalls_);
    digest.addI32(splashSpeed_);
    digest.addBool(roomDestinationSet_);
    digest.addBool(roomLoadSucceeded_);
    digest.addI32(cameraViewWidth_);
    digest.addI32(cameraViewHeight_);
    digest.addI32(backgroundMusicId_);
    digest.addBool(controlsLocked_);
    digest.addU32(soundGainRequests_);
    digest.addU32(soundPlayRequests_);
    digest.addU32(ditherFadeSpawnRequests_);
    digest.addU32(nextRuntimeInstanceId_);
    digest.addBool(deferredCommandsSucceeded_);
    digest.addU32(random_.seed());
    digest.addU64(random_.drawCount());
    digest.addU32(static_cast<std::uint32_t>(random_.stateIndex()));
    for (std::size_t index = 0; index < 16; ++index)
        digest.addU32(random_.stateWord(index));
    digest.addBool(menuPlayerInitialized_);
    digest.addBool(menuPlayerNapping_);
    digest.addF32(menuPlayerX_);
    digest.addF32(menuPlayerY_);
    digest.addF32(menuPlayerSpeedX_);
    digest.addF32(menuPlayerSpeedY_);
    digest.addBool(menuPlayerGrounded_);
    digest.addBool(menuPlayerGoalStop_);
    digest.addI32(menuPlayerFacing_);
    digest.addI32(menuPlayerSpriteId_);
    digest.addF32(menuPlayerImageIndex_);
    digest.addF32(menuPlayerImageSpeed_);
    digest.addF32(menuCameraX_);
    digest.addF32(menuCameraY_);
    digest.addBool(menuTitleFirstBoot_);
    digest.addBool(menuTitleReady_);
    digest.addF32(menuTitleStartY_);
    digest.addBool(menuTeleportActive_);
    digest.addBool(menuTeleportSequenceStarted_);
    digest.addF32(menuCameraLockerY_);
    digest.addBool(enterSequenceActive_);
    digest.addBool(enterSequenceBlackSet_);
    digest.addF32(enterSequenceY_);
    digest.addF32(enterSequenceSpeedY_);
    digest.addF32(playMenuYOffset_);
    digest.addF32(playMenuTileY_);
    digest.addF32(playMenuDitherY_);
    digest.addF32(playMenuRunFrame_);
    digest.addF32(playMenuSmoothX_);
    digest.addI32(playMenuCursor_);
    digest.addBool(playMenuSelected_);
    digest.addI32(playStyle_);
    digest.addF32(playMenuPlayerDescent_);
    digest.addF32(playMenuPlayerSpeedY_);
    digest.addF32(playMenuVerticalDitherY_);
    digest.addBool(mainPlayerInitialized_);
    if (mainPlayerInitialized_) {
        mainPlayerMotion_.appendStateDigest(digest);
        digest.addI32(mainPlayerOriginX_);
        digest.addI32(mainPlayerOriginY_);
    }
    digest.addF32(mainPlayerX_);
    digest.addF32(mainPlayerY_);
    digest.addF32(mainPlayerSpeedX_);
    digest.addF32(mainPlayerSpeedY_);
    digest.addBool(mainPlayerGrounded_);
    digest.addI32(mainPlayerFacing_);
    digest.addI32(mainPlayerSpriteId_);
    digest.addF32(mainPlayerImageIndex_);
    digest.addBool(mainJumpShootLocked_);
    digest.addBool(mainShotDelayed_);
    digest.addBool(mainAirborneShot_);
    digest.addI32(mainAmmo_);
    digest.addI32(mainPlayerHealth_);
    if (roomId_ == mainRoomId) { digest.addU32(run_.clock.ticks); digest.addBool(run_.gemHigh); }
    if (roomId_ == mainRoomId) run_.combo.appendState(digest);
    if (roomId_ == mainRoomId) { digest.addI32(run_.stage.area); digest.addI32(run_.stage.level); }
    if (mainDeath_.active) {
        digest.addBool(mainDeath_.started);
        digest.addBool(mainDeath_.priorFire);
        digest.addBool(mainDeath_.menuReady);
        digest.addU32(mainDeath_.frames);
        digest.addU32(mainDeath_.skipPresses);
        digest.addI32(mainDeath_.animation);
        digest.addI32(mainDeath_.alarm);
        digest.addU64(run_.gems.lifetime);
        digest.addU32(run_.gems.record);
        digest.addU32(run_.gems.savedRunGems);
        digest.addBool(run_.gems.recorded);
        run_.results.appendState(digest);
    }
    digest.addI32(mainPlayerInvulnerability_);
    digest.addI32(mainEmitter_.leg);
    digest.addU32(mainShotsFired_);
    digest.addU32(mainBlocksDestroyed_);
    digest.addU32(mainLandings_);
    digest.addI32(mainActivationY_);
    digest.addU32(mainEnemiesActivated_);
    digest.addU32(mainInstancesRetired_);
    digest.addU32(run_.kills);
    digest.addU32(mainJumps_);
    digest.addU32(static_cast<std::uint32_t>(mainBulletCount_));
    for (std::size_t index = 0; index < mainBulletCount_; ++index) {
        digest.addF32(mainBullets_[index].x);
        digest.addF32(mainBullets_[index].y);
        digest.addBool(mainBullets_[index].gemHigh);
        digest.addU64(mainBullets_[index].animation.index.mantissa);
        digest.addI32(mainBullets_[index].animation.index.exponent);
        const auto& motion = mainBullets_[index].trajectory;
        digest.addI32(motion.x.pixel); digest.addU32(motion.x.fraction);
        digest.addI32(motion.y.pixel); digest.addU32(motion.y.fraction);
        digest.addI32(motion.velocityX); digest.addI32(motion.velocityY);
        digest.addU32(motion.direction); digest.addBool(motion.initialized);
        digest.addBool(motion.zeroSpread);
    }
    digest.addU32(static_cast<std::uint32_t>(mainCurrencyCount_));
    for (std::size_t index = 0; index < mainCurrencyCount_; ++index) {
        const MainCurrencyState& gem = mainCurrencyItems_[index];
        digest.addF32(gem.x);
        digest.addF32(gem.y);
        digest.addF32(gem.speedX);
        digest.addF32(gem.speedY);
        digest.addI32(gem.amount);
        digest.addU32(gem.age);
        digest.addBool(gem.large);
        digest.addBool(gem.attracted);
        digest.addBool(gem.motion.attractorAtBirth);
        digest.addI32(gem.motion.x);
        digest.addI32(gem.motion.y);
        digest.addI32(gem.motion.vx);
        digest.addI32(gem.motion.vy);
        digest.addI32(gem.motion.homing.direction);
        digest.addI32(gem.motion.homing.turnFifths);
        digest.addI32(gem.motion.homing.speedEighths);
        digest.addI32(gem.motion.animation.rateTenths);
        digest.addU32(gem.motion.animation.phaseTenths);
    }
    digest.addU32(currencyAccount_.currency);
    digest.addU32(currencyAccount_.gameGem);
    digest.addU32(currencyAccount_.streak);
    digest.addU32(currencyAccount_.timer);
    digest.addBool(mainBuilderInitialized_);
    digest.addI32(mainBuilderY_);
    digest.addI32(mainBuilderGenerationCount_);
    digest.addU32(mainBuilderRowsGenerated_);
    digest.addI32(mainBuilderRightRooms_[0]);
    digest.addI32(mainBuilderRightRooms_[1]);
    digest.addI32(mainBuilderStartPattern_);
    digest.addBool(mainBuilderStartReversed_);
    digest.addU32(mainBuilderStartRowsGenerated_);
    digest.addI32(mainBuilderLastMainPattern_);
    digest.addU32(mainBuilderMainPatternsGenerated_);
    digest.addBool(mainBuilderSpawnerMade_);
    digest.addI32(mainBuilderLastRightPattern_);
    digest.addU32(mainBuilderSideRoomsGenerated_);
    digest.addI32(mainBuilderLastShopPattern_);
    digest.addU32(mainBuilderShopsGenerated_);
    digest.addU32(mainBuilderRightRoomsMade_);
    digest.addBool(mainBuilderShopMade_);
    digest.addBool(mainBuilderEndReached_);
    digest.addI32(mainBuilderEndPattern_);
    digest.addI32(mainBuilderChunkOrCrate_);
    digest.addU32(mainBuilderSpecialObjectsGenerated_);
    digest.addBool(mainCameraInitialized_);
    digest.addF32(mainCameraX_);
    digest.addF32(mainCameraY_);
    digest.addF32(mainCameraTargetY_);
    digest.addI32(mainCameraMotion_.x);
    digest.addI32(mainCameraMotion_.y);
    digest.addI32(mainCameraMotion_.targetX);
    digest.addI32(mainCameraMotion_.targetY);
    digest.addI32(mainCameraMotion_.slowdownX);
    digest.addI32(mainCameraMotion_.slowdownY);
    if (run_.sideCameraLockers.count || mainCameraMotion_.sideLocked) {
        digest.addU32(0x534c4f43); // Optional SLOC extension; unchanged empty-room hashes.
        digest.addU64(sideCameraDigest());
    }
    digest.addU32(static_cast<std::uint32_t>(instances_.size()));
    for (std::size_t index = 0; index < instances_.size(); ++index) {
        const InstanceRecord& instance = instances_.at(index);
        digest.addU32(instance.instanceId);
        digest.addI32(instance.objectId);
        digest.addI32(instance.roomId);
        digest.addI32(instance.x);
        digest.addI32(instance.y);
        digest.addF32(instance.scaleX);
        digest.addF32(instance.scaleY);
        digest.addF32(instance.rotation);
        digest.addF32(instance.imageSpeed);
        digest.addF32(instance.imageIndex);
        if (instance.enemyImageInitialized) {
            digest.addU32(instance.enemyImagePhase);
            digest.addU32(instance.enemyImageRemainder);
        }
        digest.addI32(instance.spriteId);
        digest.addI32(instance.maskId);
        digest.addI32(instance.creationCodeId);
        digest.addI32(instance.preCreateCodeId);
        digest.addU32(instance.layerId);
        digest.addI32(instance.depth);
        digest.addBool(instance.persistent);
        digest.addBool(instance.active);
        digest.addBool(instance.activationPending);
        digest.addI32(instance.health);
        digest.addI32(instance.reward);
        digest.addF32(instance.preciseX);
        digest.addF32(instance.preciseY);
        digest.addF32(instance.velocityX);
        digest.addF32(instance.velocityY);
        digest.addF32(instance.direction);
        digest.addF32(instance.moveSpeed);
        digest.addI32(instance.motionState);
        digest.addI32(instance.auxiliaryState);
        if (instance.objectId == 116) {
            digest.addBool(instance.batActivity.initialized);
            digest.addBool(instance.batActivity.flying);
            digest.addI32(instance.batActivity.hang);
            digest.addBool(instance.batHit.pending);
            digest.addBool(instance.batHit.ready);
            digest.addBool(instance.batHit.stunned);
            digest.addI32(instance.batHit.pendingDirection);
            digest.addI32(instance.batHit.direction);
            digest.addU32(instance.batHit.pendingFraction);
            digest.addU32(instance.batHit.fraction);
            digest.addU32(instance.batDirectionFraction);
        }
        if (instance.objectId == 111 || instance.objectId == 112) {
            digest.addI32(instance.walker.x);
            digest.addI32(instance.walker.y);
            digest.addI32(instance.walker.vx);
            digest.addI32(instance.walker.vy);
            digest.addI32(instance.walker.facing);
            digest.addI32(instance.walker.publishedX);
            digest.addI32(instance.walker.publishedY);
            digest.addBool(instance.walker.grounded);
            digest.addBool(instance.walker.initialized);
        }
        if (instance.objectId == 117) {
            digest.addI32(instance.snail.x);
            digest.addI32(instance.snail.y);
            digest.addI32(instance.snail.vy);
            digest.addI32(instance.snail.facing);
            digest.addI32(instance.snail.verticalFlip);
            digest.addBool(instance.snail.initialized);
            digest.addBool(instance.snail.attached);
            digest.addBool(instance.snailHit.pending);
            digest.addBool(instance.snailHit.ready);
            digest.addBool(instance.snailHit.stunned);
            digest.addI32(instance.snailHit.alarm2);
        }
        if (instance.objectId == 119) {
            digest.addI32(instance.crawler.x);
            digest.addI32(instance.crawler.y);
            digest.addI32(instance.crawler.vx);
            digest.addI32(instance.crawler.vy);
            digest.addU32(instance.crawler.direction);
            digest.addBool(instance.crawler.latched);
            digest.addBool(instance.crawler.initialized);
        }
        digest.addBool(instance.motionInitialized);
        digest.addBool(instance.grounded);
        if(instance.objectId==238)digest.addBool(instance.destroyRequested);
        digest.addBool(instance.frogLanded);
        if (instance.objectId == 113) {
            digest.addBool(instance.frogMotionInitialized);
            digest.addI32(instance.frogHorizontal.position);
            digest.addI32(instance.frogHorizontal.velocity);
            digest.addI32(instance.frogVertical.position);
            digest.addI32(instance.frogVertical.velocity);
            digest.addBool(instance.frogVertical.grounded);
            digest.addBool(instance.frogHit.pending);
            digest.addU32(instance.frogPresentation.desiredRate);
            digest.addBool(instance.frogHit.ready);
            digest.addU32(instance.frogHit.remaining);
        }
        for (std::size_t alarm = 0; alarm < AlarmBank::count; ++alarm)
            digest.addI32(instance.alarms.get(alarm));
    }
    return digest.value();
}

} // namespace downwell
