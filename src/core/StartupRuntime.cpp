#include "StartupRuntime.h"
#include "BuilderPatternPack.h"
#include "EventScheduler.h"
#include "EnemyPresentation.h"
#include "EnemyContact.h"
#include "FrogBehavior.h"
#include "FrogMotion.h"
#include "StandardBulletStep.h"
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
                               const BuilderPatternPack* builderPatternPack)
    : roomPack_(roomPack), objectPack_(objectPack),
      builderPatternPack_(builderPatternPack),
      randomSeed_(randomSeed), random_(randomSeed) {
    reset(profile);
}

void StartupRuntime::reset(PlatformProfile profile) {
    tick_ = 0;
    profile_ = profile;
    random_.setSeed(randomSeed_); // Result of the Windows runner's randomize().
    roomId_ = entryRoomId;
    roomDestination_ = 0;
    showSplash_ = 0;
    instances_.clear();
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
    inputLeft_ = false;
    inputRight_ = false;
    inputLeftPressed_ = false;
    inputRightPressed_ = false;
    inputFire_ = false;
    inputFirePressed_ = false;
    inputFireReleased_ = false;
    anyInput_ = false;
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
    mainPlayerInvulnerability_ = 0;
    mainShootLeg_ = -1;
    mainShotsFired_ = 0;
    mainBlocksDestroyed_ = 0;
    mainLandings_ = 0;
    mainActivationY_ = 0;
    mainEnemiesActivated_ = 0;
    mainInstancesRetired_ = 0;
    mainEnemiesDefeated_ = 0;
    mainJumps_ = 0;
    mainBulletCount_ = 0;
    mainCurrencyCount_ = 0;
    mainCurrencyTotal_ = 0;
    mainGameGemTotal_ = 0;
    mainGemStreak_ = 0;
    mainGemStreakTimer_ = 0;
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

    // rmInit instance order in the recovered room data.
    const RoomInstanceDefinition definitions[] = {
        {controllerInstanceId, 37, true},
        {initRoomInstanceId, 36, false}
    };
    const auto instanceHandler = [this](InstanceRecord& instance,
                                        RoomInstancePhase phase) {
        dispatchInstancePhase(instance, phase);
    };
    const auto roomCreationHandler = [this](std::int32_t roomId) {
        dispatchRoomCreation(roomId);
    };
    const auto deferredHandler = [this](InstanceStore<instanceCapacity>& instances) {
        flushDeferredInstances(instances, entryRoomId);
    };
    if (roomPack_ != nullptr) {
        roomLoadSucceeded_ = PackedRoomLoader<instanceCapacity>::enter(
            instances_, *roomPack_, entryRoomId, true,
            instanceHandler, roomCreationHandler,
            deferredHandler) == PackedRoomLoadResult::success;
    } else {
        roomLoadSucceeded_ = RoomLoader<instanceCapacity>::enter(
            instances_, entryRoomId, definitions, 2, true,
            instanceHandler, roomCreationHandler,
            deferredHandler) == LoadRoomResult::success;
    }
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
    skipIntroForTesting();
    if (roomLoadSucceeded_) {
        playStyle_ = 0;
        playMenuSelected_ = true;
        playMenuVerticalDitherY_ = 320.0F;
        // Take the normal confirmation transition, including same-frame
        // camera phases, without presenting or waiting through the menu.
        step(Input{}, false);
    }
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

void StartupRuntime::step(const Input& input, bool advancePressed) {
    stepInternal(advancePressed, input.left, input.right, input.fire,
                 input.left || input.right || input.fire || input.pause);
}

void StartupRuntime::stepInternal(bool advancePressed,
                                  bool left,
                                  bool right,
                                  bool fire,
                                  bool anyInput) {
    ++tick_;
    inputLeftPressed_ = left && !inputLeft_;
    inputRightPressed_ = right && !inputRight_;
    inputFirePressed_ = fire && !inputFire_;
    inputFireReleased_ = !fire && inputFire_;
    inputLeft_ = left;
    inputRight_ = right;
    inputFire_ = fire;
    anyInput_ = anyInput;

    for (std::size_t i = 0; i < instances_.size(); ++i) {
        auto& instance = instances_.at(i);
        if (instance.active && instance.objectId == 113) instance.frogHit.advanceFrame();
    }

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
        AlarmBank::FiredMask mask = 0;
        PackedObjectEvent event;
        for (std::int32_t alarm = 0;
             alarm < static_cast<std::int32_t>(AlarmBank::count);
             ++alarm) {
            if (objectPack_->findEvent(instance.objectId, 2, alarm, event))
                mask |= static_cast<AlarmBank::FiredMask>(1U << alarm);
        }
        return mask;
    };
    const auto handler = [this, advancePressed](InstanceRecord& instance,
                                                 StepEvent event) {
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
    EventScheduler<instanceCapacity>::runStep(instances_, alarmEvents, handler);
    if (deferredInstances_.size() != 0)
        flushDeferredInstances(instances_, roomId_);
    if (roomId_ == mainRoomId) {
        updateMainBullets();
        updateMainEnemies();
        updateMainEnemyInteractions();
        updateMainCurrency();
        updateMainLifecycle();
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

InstanceRecord* StartupRuntime::mainBulletCollision(int x, int fromY, int toY) {
    const StandardBulletPath path{x, fromY, toY};
    for (std::size_t index = 0; index < instances_.size(); ++index) {
        InstanceRecord& wall = instances_.at(index);
        if (!wall.active || (wall.objectId != 60 && wall.objectId != 92))
            continue;
        if (path.intersects(wall.x - 8, wall.x + 8, wall.y - 8, wall.y + 8))
            return &wall;
    }
    return nullptr;
}

InstanceRecord* StartupRuntime::mainBulletEnemyCollision(int x, int fromY, int toY) {
    const StandardBulletPath path{x, fromY, toY};
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
            enemyContactMask(enemy.objectId, enemy.spriteId)};
        if (path.intersects(contact))
            return &enemy;
    }
    return nullptr;
}

void StartupRuntime::emitMainBullet(const InstanceRecord& instance) {
    if (mainBulletCount_ < mainBulletCapacity) {
        mainBullets_[mainBulletCount_++] = {
            static_cast<float>(instance.x + mainShootLeg_),
            static_cast<float>(instance.y + 4)
        };
    }
    mainShootLeg_ *= -1;
    ++soundPlayRequests_;
}

void StartupRuntime::syncMainPlayerState() {
    mainPlayerX_ = mainPlayerOriginX_ + mainPlayerMotion_.offsetXFixed() / 256.0F;
    mainPlayerY_ = mainPlayerOriginY_ + mainPlayerMotion_.offsetYFixed() / 256.0F;
    mainPlayerSpeedX_ = mainPlayerMotion_.speedXFixed() / 256.0F;
    mainPlayerSpeedY_ = mainPlayerMotion_.speedYFixed() / 256.0F;
    mainPlayerGrounded_ = mainPlayerMotion_.grounded();
    mainPlayerFacing_ = mainPlayerMotion_.facing();
    mainPlayerSpriteId_ = mainPlayerMotion_.spriteId();
    mainPlayerImageIndex_ = static_cast<float>(mainPlayerMotion_.spriteFrame());
    mainJumpShootLocked_ = mainPlayerMotion_.jumpShootLocked();
    mainShotDelayed_ = mainPlayerMotion_.shotDelayed();
    mainAirborneShot_ = mainPlayerMotion_.airborneShot();
    mainAmmo_ = mainPlayerMotion_.ammo();
    mainJumps_ = mainPlayerMotion_.jumps();
    mainShotsFired_ = mainPlayerMotion_.shots();
}

void StartupRuntime::updateMainBullets() {
    std::size_t write = 0;
    for (std::size_t index = 0; index < mainBulletCount_; ++index) {
        MainBulletState bullet = mainBullets_[index];
        int nextY = static_cast<int>(bullet.y);
        bool alive = stepStandardBullet(static_cast<int>(bullet.x), nextY,
          [this](const StandardBulletPath& path) {
            InstanceRecord* enemy = mainBulletEnemyCollision(path.x, path.fromY, path.toY);
            if (enemy != nullptr) {
                // Ground turtles are bulletImmune in their recovered Create
                // event. Other area-one enemies take the usual gun's 10 dmg.
                if (enemy->objectId != 112) {
                    if (enemy->objectId == 113) enemy->frogHit.pending = true;
                    enemy->health -= 10;
                    if (enemy->health <= 0) {
                        spawnMainCurrency(static_cast<float>(enemy->x),
                                          static_cast<float>(enemy->y),
                                          enemy->reward);
                        enemy->active = false;
                        ++mainEnemiesDefeated_;
                    }
                }
                ++soundPlayRequests_;
                return true;
            }
            return false;
          }, [this](const StandardBulletPath& path) {
            InstanceRecord* wall = mainBulletCollision(path.x, path.fromY, path.toY);
            if (wall != nullptr) {
                if (wall->objectId == 92) {
                    // objBox_n starts with wallHp = 1. The usual gun's bdmg
                    // exhausts it on contact; its Step event destroys it.
                    wall->active = false;
                    ++mainBlocksDestroyed_;
                    ++soundPlayRequests_;
                }
                return true;
            }
            return false;
          });
        bullet.y = static_cast<float>(nextY);
        if (standardBulletOutsideView(static_cast<long>(nextY) * 4096,
                                      mainCameraMotion_.y - surfaceHeight / 2L * 4096,
                                      surfaceHeight))
            alive = false;
        if (alive)
            mainBullets_[write++] = bullet;
    }
    mainBulletCount_ = write;
}

void StartupRuntime::spawnMainCurrency(float x, float y, std::int32_t amount,
                                       bool attracted) {
    while (amount > 0 && mainCurrencyCount_ < mainCurrencyCapacity) {
        MainCurrencyState& gem = mainCurrencyItems_[mainCurrencyCount_++];
        gem = MainCurrencyState{};
        gem.x = x;
        gem.y = y;
        gem.large = amount >= 10;
        gem.amount = gem.large ? 10 : 2;
        gem.attracted = attracted;
        gem.motion.initialize(static_cast<int>(roundGameMakerCoordinate(x)) - mainPlayerOriginX_,
                              static_cast<int>(roundGameMakerCoordinate(y)) - mainPlayerOriginY_, attracted, random_);
        gem.speedX = gem.motion.vx / 12800.0F;
        gem.speedY = gem.motion.vy / 12800.0F;
        amount -= gem.amount;
    }
}

void StartupRuntime::updateMainCurrency() {
    if (mainGemStreakTimer_ > 0)
        --mainGemStreakTimer_;
    else
        mainGemStreak_ = 0;

    std::size_t write = 0;
    for (std::size_t index = 0; index < mainCurrencyCount_; ++index) {
        MainCurrencyState gem = mainCurrencyItems_[index];
        if (!gem.motion.step(mainPlayerMotion_.frameStartXFixed(), mainPlayerMotion_.frameStartYFixed(),
            [this](int x, int y) { return mainSolidAt(mainPlayerOriginX_ + x, mainPlayerOriginY_ + y); }, random_)) continue;
        gem.age = gem.motion.lifetime.age;
        gem.x = mainPlayerOriginX_ + gem.motion.x / 12800.0F;
        gem.y = mainPlayerOriginY_ + gem.motion.y / 12800.0F;
        gem.speedX = gem.motion.vx / 12800.0F;
        gem.speedY = gem.motion.vy / 12800.0F;
        gem.attracted = gem.motion.attracted;
        const bool collected = gem.motion.collectible(mainPlayerMotion_.offsetXFixed(),
                                                      mainPlayerMotion_.offsetYFixed(), gem.large);
        if (collected) {
            mainCurrencyTotal_ += static_cast<std::uint32_t>(gem.amount);
            mainGameGemTotal_ += static_cast<std::uint32_t>(gem.amount);
            mainGemStreak_ += static_cast<std::uint32_t>(gem.amount);
            mainGemStreakTimer_ = 300;
            ++soundPlayRequests_;
        } else {
            mainCurrencyItems_[write++] = gem;
        }
    }
    mainCurrencyCount_ = write;
}

void StartupRuntime::updateCrawler(InstanceRecord& enemy, bool alarm) {
    if (!enemy.crawler.initialized) enemy.crawler.initialize(enemy.x, enemy.y);
    const bool visible = CrawlerMotion::inView(enemy.x, enemy.y,
        mainCameraMotion_.x - 80L * 4096, mainCameraMotion_.y - 142L * 4096,
        mainCameraMotion_.x + 80L * 4096, mainCameraMotion_.y + 142L * 4096,
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
    enemy.rotation = enemy.crawler.direction * 90 + (enemy.motionState > 0 ? 0 : 180);
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
        if (!enemy.active || !enemyObject || enemy.health <= 0)
            continue;
        if (!enemy.motionInitialized) {
            enemy.preciseX = static_cast<float>(enemy.x);
            enemy.preciseY = static_cast<float>(enemy.y);
            enemy.motionInitialized = true;
        }

        enemy.imageIndex += enemy.imageSpeed;
        if (enemy.objectId == 111 || enemy.objectId == 112) {
            const float halfWidth = enemy.objectId == 112 ? 8.0F : 6.0F;
            const float foot = enemy.objectId == 112 ? 8.0F : 5.0F;
            enemy.velocityY = std::min(7.0F, enemy.velocityY + 0.2F);
            enemy.grounded = mainSolidAt(enemy.preciseX,
                                         enemy.preciseY + foot + 1.0F);
            if (enemy.grounded) {
                enemy.velocityY = 0.0F;
                const float ahead = enemy.preciseX +
                    (enemy.velocityX < 0.0F ? -halfWidth - 1.0F
                                             : halfWidth + 1.0F);
                if (!mainSolidAt(ahead, enemy.preciseY + foot + 1.0F))
                    enemy.velocityX *= -1.0F;
            }
            const float nextX = enemy.preciseX + enemy.velocityX;
            const float edge = nextX + (enemy.velocityX < 0.0F
                                             ? -halfWidth : halfWidth);
            if (mainSolidAt(edge, enemy.preciseY))
                enemy.velocityX *= -1.0F;
            else
                enemy.preciseX = nextX;
            if (!enemy.grounded) {
                const float nextY = enemy.preciseY + enemy.velocityY;
                if (enemy.velocityY > 0.0F &&
                    mainSolidAt(enemy.preciseX, nextY + foot)) {
                    while (!mainSolidAt(enemy.preciseX,
                                        enemy.preciseY + foot + 1.0F))
                        enemy.preciseY += 1.0F;
                    enemy.velocityY = 0.0F;
                    enemy.grounded = true;
                } else {
                    enemy.preciseY = nextY;
                }
            }
            enemy.scaleX = enemy.velocityX < 0.0F ? -1.0F : 1.0F;
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
            const bool hit = stepFrogMotion(enemy.frogHorizontal, enemy.frogVertical,
                enemy.frogHit, integrate, [this](int x, int y) {
                    return mainSolidAt(mainPlayerOriginX_ + x, mainPlayerOriginY_ + y);
                });
            if (hit) {
                enemy.auxiliaryState = 0;
                enemy.alarms.set(0, 0);
                enemy.alarms.set(1, 60);
                enemy.velocityX = enemy.frogHorizontal.velocity / 65536.0F;
                enemy.imageIndex = 0;
                ++soundPlayRequests_;
            }
            enemy.preciseY = mainPlayerOriginY_ + enemy.frogVertical.position / 1280.0F;
            enemy.velocityY = enemy.frogVertical.velocity / 1280.0F;
            enemy.grounded = enemy.frogVertical.grounded;
            if (!enemy.grounded) {
                enemy.preciseX = mainPlayerOriginX_ + enemy.frogHorizontal.position / 65536.0F;
                enemy.velocityX = enemy.frogHorizontal.velocity / 65536.0F;
            }
            if (enemy.velocityX != 0.0F)
                enemy.scaleX = enemy.velocityX < 0.0F ? -1.0F : 1.0F;
            // enmFrog Step's landed latch is distinct from current support.
            if (frogLandingEdge(enemy.grounded, enemy.frogLanded)) enemy.alarms.set(1, 60);
        } else if (enemy.objectId == 116) {
            BatMotion bat{
                static_cast<long>(std::round(enemy.preciseX * 256.0F)),
                static_cast<long>(std::round(enemy.preciseY * 256.0F)), 0, 0,
                static_cast<int>(enemy.direction),
                static_cast<int>(std::round(enemy.moveSpeed * 40.0F))
            };
            stepBatMotion(bat,
                mainPlayerOriginX_ * 256L + mainPlayerMotion_.frameStartXFixed(),
                mainPlayerOriginY_ * 256L + mainPlayerMotion_.frameStartYFixed(),
                random_.nextU32(),
                [this](int x, int y) { return mainSolidAt(x, y); });
            enemy.preciseX = bat.x / 256.0F;
            enemy.preciseY = bat.y / 256.0F;
            enemy.velocityX = bat.speedX / 256.0F;
            enemy.velocityY = bat.speedY / 256.0F;
            enemy.direction = static_cast<float>(bat.direction);
            enemy.moveSpeed = bat.speedFortieths / 40.0F;
            enemy.scaleX = enemy.velocityX < 0.0F ? -1.0F : 1.0F;
        } else if (enemy.objectId == 117) {
            if (enemy.motionState == 0) {
                if (mainSolidAt(enemy.preciseX + 8.0F, enemy.preciseY))
                    enemy.motionState = 1;
                else if (mainSolidAt(enemy.preciseX - 8.0F, enemy.preciseY))
                    enemy.motionState = -1;
                else
                    enemy.active = false;
                enemy.scaleX = static_cast<float>(enemy.motionState);
            }
            if (enemy.active) {
                const float vertical = enemy.velocityY < 0.0F ? -1.0F : 1.0F;
                if (mainSolidAt(enemy.preciseX, enemy.preciseY + vertical * 8.0F) ||
                    !mainSolidAt(enemy.preciseX + enemy.motionState * 16.0F,
                                 enemy.preciseY + vertical * 8.0F))
                    enemy.velocityY *= -1.0F;
                enemy.preciseY += enemy.velocityY;
                enemy.scaleY = enemy.velocityY < 0.0F ? 1.0F : -1.0F;
            }
        } else if (enemy.objectId == 119) {
            updateCrawler(enemy, false);
        }
        enemy.x = static_cast<std::int32_t>(roundGameMakerCoordinate(enemy.preciseX));
        enemy.y = static_cast<std::int32_t>(roundGameMakerCoordinate(enemy.preciseY));
        enemy.spriteId = enemyPresentationSprite(enemy.objectId, enemy.grounded,
                                                  enemy.auxiliaryState);
        if (enemy.objectId == 113) {
            if (enemy.frogHit.remaining != 0) {
                enemy.spriteId = 173;
            } else if (!enemy.grounded) {
                enemy.imageSpeed = 0.0F;
                enemy.imageIndex = enemy.velocityY < 0.0F ? 0.0F : 1.0F;
            } else {
                enemy.imageSpeed = 0.2F;
            }
        }
        const float frames = static_cast<float>(enemySpriteFrameCount(enemy.spriteId));
        if (enemy.imageIndex >= frames)
            enemy.imageIndex = std::fmod(enemy.imageIndex, frames);
    }
}

void StartupRuntime::updateMainEnemyInteractions() {
    if (mainPlayerInvulnerability_ > 0)
        --mainPlayerInvulnerability_;
    for (std::size_t index = 0; index < instances_.size(); ++index) {
        InstanceRecord& enemy = instances_.at(index);
        const bool stompable = enemy.objectId == 111 || enemy.objectId == 112 ||
                               enemy.objectId == 113 || enemy.objectId == 116;
        const bool contactOnly = enemy.objectId == 117 || enemy.objectId == 119;
        if (!enemy.active || enemy.health <= 0 || (!stompable && !contactOnly))
            continue;
        const ContactActor playerContact{
            mainPlayerMotion_.offsetXFixed(), mainPlayerMotion_.offsetYFixed(),
            mainPlayerMotion_.facing(), 1, 31};
        const ContactActor enemyContact{
            static_cast<long>(std::round((enemy.preciseX - mainPlayerOriginX_) * 256.0F)),
            static_cast<long>(std::round((enemy.preciseY - mainPlayerOriginY_) * 256.0F)),
            enemy.scaleX < 0 ? -1 : 1, enemy.scaleY < 0 ? -1 : 1,
            enemyContactMask(enemy.objectId, enemy.spriteId)};
        const auto contact = classifyEnemyContact(playerContact, enemyContact,
            enemy.objectId, mainPlayerMotion_.frameStartYFixed());
        if (contact == EnemyContact::none) continue;
        if (contact == EnemyContact::stomp) {
            spawnMainCurrency(enemy.preciseX, enemy.preciseY,
                              enemy.reward, true);
            enemy.health = 0;
            enemy.active = false;
            ++mainEnemiesDefeated_;
            mainPlayerMotion_.stompBounce();
            syncMainPlayerState();
            ++soundPlayRequests_;
            continue;
        }
        if (mainPlayerInvulnerability_ == 0 && mainPlayerHealth_ > 0) {
            --mainPlayerHealth_;
            mainPlayerInvulnerability_ = 90;
            mainPlayerMotion_.contactKnockback(
                static_cast<int>(roundGameMakerCoordinate(enemy.preciseX)) - mainPlayerOriginX_,
                static_cast<int>(roundGameMakerCoordinate(enemy.preciseY)) - mainPlayerOriginY_);
            syncMainPlayerState();
            ++soundPlayRequests_;
        }
    }
}

void StartupRuntime::updateMainLifecycle() {
    const float viewTop = mainCameraViewY();

    for (std::size_t index = 0; index < instances_.size(); ++index) {
        InstanceRecord& instance = instances_.at(index);
        if (!instance.active)
            continue;
        const bool generated = instance.objectId == 60 ||
                               instance.objectId == 92 ||
                               instance.objectId == 93 ||
                               instance.objectId == 111 ||
                               instance.objectId == 112 ||
                               instance.objectId == 113 ||
                               instance.objectId == 116 ||
                               instance.objectId == 117 ||
                               instance.objectId == 119 ||
                               instance.objectId == 289;
        if (generated &&
            (instance.y < viewTop - 160.0F ||
             instance.y > viewTop + 3000.0F)) {
            instance.active = false;
            ++mainInstancesRetired_;
        }
    }
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
    const bool generated = areaOneBuilder_.stepVisible(
        *builderPatternPack_, random_, static_cast<int>(std::ceil(viewBottom)),
        [](void* opaque, const DeferredCreateInstance& command) {
            StartupRuntime* runtime = static_cast<StartupRuntime*>(opaque);
            return runtime->deferredInstances_.enqueueCreate(command);
        },
        this, result);
    if (!generated) {
        deferredCommandsSucceeded_ = false;
        return;
    }
    instance.y = areaOneBuilder_.y();
    syncMainBuilderState();
    return;

    if (mainBuilderGenerationCount_ < -1) {
        random_.irandom(1); // choose(0, 1): unused orientation for empty rows.
        for (int cell = 0; cell < 19; ++cell)
            random_.nextU32(); // builderCavern on 18 cells and one newline.
        const bool left = deferredInstances_.enqueueCreate(60, 160, instance.y);
        const bool right = deferredInstances_.enqueueCreate(60, 320, instance.y);
        if (!left || !right) {
            deferredCommandsSucceeded_ = false;
            return;
        }
        instance.y += 16;
        mainBuilderY_ = instance.y;
        ++mainBuilderGenerationCount_;
        ++mainBuilderRowsGenerated_;
        return;
    }

    if (builderPatternPack_ == nullptr || !builderPatternPack_->valid() ||
        builderPatternPack_->patternCount() == 0)
        return;
    const auto enqueueObject = [this](std::int32_t objectId, int x, int y) {
        DeferredCreateInstance command{objectId, x, y};
        if (objectId == 92) {
            // objBox_n Create runs synchronously inside instance_create:
            // break sample, tile variant, and gem chance. rmMain is not a
            // ground room, so a successful gem roll also chooses 18/19.
            random_.irandom(3);
            command.imageSpeed = 0.0F;
            command.imageIndex = 16.0F + random_.irandom(1);
            if (random_.irandom(50) < 1) {
                random_.irandom(1);
                command.imageIndex = 22.0F;
            }
        }
        if (objectId == 111) {
            command.imageSpeed = 0.25F;
            command.velocityX = 0.5F;
            command.motionInitialized = true;
            command.grounded = true;
        }
        if (objectId == 112) {
            command.imageSpeed = 0.15F;
            command.velocityX = 0.25F;
            command.auxiliaryState = random_.irandom(1) == 0 ? 1 : -1;
            command.motionInitialized = true;
            command.grounded = true;
            command.alarms.set(1, 60 + static_cast<int>(random_.irandom(60)));
        }
        if (objectId == 113) {
            command.imageSpeed = 0.0F;
            command.motionInitialized = true;
            command.alarms.set(1,
                60 + static_cast<int>(random_.randomIntegerSmall(60)));
        }
        if (objectId == 116) {
            command.imageSpeed = 0.2F;
            random_.nextU32(); // Unused recovered vel = random_range(1, 1.5).
            command.direction = static_cast<float>(random_.irandom(359));
            command.moveSpeed = 1.25F;
            command.motionInitialized = true;
            command.active = false;
        }
        if (objectId == 117) {
            command.imageSpeed = 0.1F;
            command.velocityY = random_.irandom(1) == 0 ? 0.1F : -0.1F;
            command.motionInitialized = true;
            command.alarms.set(1, 60);
        }
        if (objectId == 119) {
            command.imageSpeed = 0.1F;
            command.motionState = random_.irandom(1) == 0 ? 1 : -1;
            command.moveSpeed = 1.0F;
            command.motionInitialized = true;
            command.alarms.set(0, 2);
        }
        if (objectId == 289) {
            command.imageSpeed = 0.0F;
            command.scaleX = random_.irandom(1) == 0 ? 1.0F : -1.0F;
            command.imageIndex = static_cast<float>(random_.irandom(15));
            if (random_.randomIntegerSmall(300) == 0)
                command.imageIndex = 16.0F;
        }
        if (objectId == 82)
            random_.nextU32(); // parentTimeField radius 24..26.
        if (objectId == 293) {
            command.imageSpeed = 0.0F;
            command.alarms.set(1, 30 + static_cast<int>(random_.irandom(30)));
            command.alarms.set(2, 180 + static_cast<int>(random_.irandom(180)));
        }
        if (objectId == 109) {
            command.imageSpeed = 0.0F;
            command.alarms.set(0, 10);
            command.alarms.set(3, 10);
            random_.irandom(1); // moduleType = choose(0, 1).
            std::uint64_t module = 2;
            while (module == 2)
                module = random_.irandom(6);
        }
        return deferredInstances_.enqueueCreate(command);
    };
    const auto emitPattern = [this, &instance, &enqueueObject](
                                 const PackedBuilderPattern& pattern,
                                 bool reversed,
                                 bool appendPillarRows,
                                 std::uint32_t& advancedRows) {
        for (std::uint16_t row = 0; row < pattern.rowCount; ++row) {
            const int rowY = instance.y + row * 16;
            std::uint32_t recoveredWidth = 0;
            if (!builderPatternPack_->rowWidth(pattern, row, recoveredWidth))
                return false;
            for (std::uint16_t column = 0; column < recoveredWidth; ++column) {
                char symbol = 0;
                if (!builderPatternPack_->cell(pattern, column, row, symbol)) {
                    return false;
                }
                const int ordinal = column + 1;
                const int buildX = reversed ? 320 - ordinal * 16
                                            : 160 + ordinal * 16;

                // builderCavern handles these decorations before its universal
                // area-one enemy roll.
                if ((symbol == 'I' || symbol == 'J') &&
                    !enqueueObject(296, buildX, rowY))
                    return false;
                if (symbol == '^' && random_.irandom(4) != 0 &&
                    !enqueueObject(289, buildX, rowY))
                    return false;
                // random(3) <= 1, compared directly in the recovered
                // unsigned 2^-32 domain (floor(2^32 / 3) = 0x55555555).
                if (random_.nextU32() <= 0x55555555U) {
                    std::int32_t enemyId = -1;
                    if (symbol == 'a') enemyId = 111;
                    if (symbol == 'b') enemyId = 116;
                    if (symbol == 'd') enemyId = 113;
                    if (symbol == 'e') enemyId = 117;
                    if (symbol == 't') enemyId = 112;
                    if (symbol == '%') {
                        constexpr std::int32_t enemies[] = {113, 111, 119, 112};
                        enemyId = enemies[random_.irandom(3)];
                    }
                    if (enemyId >= 0 && !enqueueObject(enemyId, buildX, rowY))
                        return false;
                }

                std::int32_t objectId = -1;
                if (symbol == '1') objectId = 60;
                if (symbol == '2' && random_.irandom(1) != 0) objectId = 60;
                if (symbol == '-') objectId = 93;
                if (symbol == 'B') objectId = 92;
                if (symbol == '3' && random_.irandom(1) != 0) objectId = 92;
                if (symbol == '|') objectId = 60;
                if (symbol == 'v') objectId = 376;
                if (symbol == 'O') objectId = 82;
                if (symbol == '_') objectId = 62;
                if (symbol == '$') objectId = 277;
                if (symbol == '\\') objectId = 285;
                if (symbol == '=') objectId = 306;
                if (symbol == '@') {
                    if (mainBuilderChunkOrCrate_ == 0) {
                        constexpr std::int32_t choices[] = {293, 109};
                        mainBuilderChunkOrCrate_ =
                            choices[random_.irandom(1)];
                    } else {
                        mainBuilderChunkOrCrate_ =
                            mainBuilderChunkOrCrate_ == 109 ? 293 : 109;
                    }
                    objectId = mainBuilderChunkOrCrate_;
                }
                if (symbol == '{' || symbol == '}') {
                    objectId = 60;
                    const int edgeX = reversed ? 16 : 464;
                    if (!enqueueObject(objectId, edgeX, rowY))
                        return false;
                    ++mainBuilderSpecialObjectsGenerated_;
                    objectId = -1;
                }
                if (objectId >= 0 && !enqueueObject(objectId, buildX, rowY))
                    return false;
                if (symbol == 'I' || symbol == 'J' || symbol == 'v' ||
                    symbol == 'O' || symbol == '_' || symbol == '$' ||
                    symbol == '\\' || symbol == '@')
                    ++mainBuilderSpecialObjectsGenerated_;
            }
            if (row + 1 < pattern.rowCount) {
                random_.nextU32(); // builderCavern sees the row newline.
                const bool sideRoom =
                    pattern.family == BuilderPatternFamily::areaOneRight ||
                    pattern.family == BuilderPatternFamily::areaOneShop;
                const int firstX = sideRoom
                                       ? (reversed ? 0 : 480)
                                       : (reversed ? 160 : 320);
                const int secondX = reversed ? 320 : 160;
                if (!enqueueObject(60, firstX, rowY) ||
                    !enqueueObject(60, secondX, rowY))
                    return false;
            }
        }
        advancedRows = pattern.rowCount - 1U;
        instance.y += static_cast<std::int32_t>(advancedRows * 16U);
        if (appendPillarRows) {
            const int firstX = reversed ? 160 : 320;
            const int secondX = reversed ? 320 : 160;
            for (int padding = 0; padding < 3; ++padding) {
                if (!enqueueObject(60, firstX, instance.y) ||
                    !enqueueObject(60, secondX, instance.y))
                    return false;
                instance.y += 16;
                ++advancedRows;
            }
        }
        return true;
    };

    PackedBuilderPattern pattern;
    std::uint32_t advancedRows = 0;
    if (mainBuilderGenerationCount_ == -1) {
        // scrStartPat1 stores its final valid index (four) in partStart[100].
        const std::uint16_t selected =
            static_cast<std::uint16_t>(random_.irandom(4));
        if (!builderPatternPack_->findPattern(
                BuilderPatternFamily::areaOneStart, selected, pattern)) {
            deferredCommandsSucceeded_ = false;
            return;
        }
        const bool reversed = random_.irandom(1) != 0; // choose(0, 1).
        mainBuilderStartPattern_ = selected;
        mainBuilderStartReversed_ = reversed;
        mainBuilderLastPatternReversed_ = reversed;
        if (!emitPattern(pattern, reversed, false, advancedRows)) {
            deferredCommandsSucceeded_ = false;
            return;
        }
        mainBuilderGenerationCount_ = 0;
        mainBuilderStartRowsGenerated_ = advancedRows;
    } else if (mainBuilderGenerationCount_ > 16) {
        const std::uint16_t selected =
            static_cast<std::uint16_t>(random_.irandom(3));
        if (!builderPatternPack_->findPattern(
                BuilderPatternFamily::areaOneEnd, selected, pattern)) {
            deferredCommandsSucceeded_ = false;
            return;
        }
        const bool reversed = random_.irandom(1) != 0;
        if (!emitPattern(pattern, reversed, true, advancedRows)) {
            deferredCommandsSucceeded_ = false;
            return;
        }
        mainBuilderEndPattern_ = selected;
        mainBuilderLastPatternReversed_ = reversed;
        mainBuilderEndReached_ = true;
    } else {
        if (!mainBuilderSpawnerMade_) {
            if (!enqueueObject(78, instance.x - 100, instance.y)) {
                deferredCommandsSucceeded_ = false;
                return;
            }
            mainBuilderSpawnerMade_ = true;
        }
        const bool scheduledSideRoom =
            mainBuilderGenerationCount_ == mainBuilderRightRooms_[0] ||
            mainBuilderGenerationCount_ == mainBuilderRightRooms_[1];
        std::uint16_t selected = 0;
        bool makeShop = false;
        if (scheduledSideRoom) {
            ++mainBuilderRightRoomsMade_;
            // scrLevelPat1 marks the shop as already made in ordinary 1-1,
            // deliberately suppressing it during the first playable slice.
            // Later area-one levels reach this recovered 50/50 choice.
            if (!mainBuilderShopMade_)
                makeShop = random_.irandom(1) != 0; // choose(0, 1).
            if (makeShop) {
                selected = static_cast<std::uint16_t>(random_.irandom(1));
                if (!builderPatternPack_->findPattern(
                        BuilderPatternFamily::areaOneShop, selected, pattern)) {
                    deferredCommandsSucceeded_ = false;
                    return;
                }
            } else {
                do {
                    selected = static_cast<std::uint16_t>(random_.irandom(9));
                } while (selected == mainBuilderLastRightPattern_);
                if (!builderPatternPack_->findPattern(
                        BuilderPatternFamily::areaOneRight, selected, pattern)) {
                    deferredCommandsSucceeded_ = false;
                    return;
                }
            }
        } else {
            do {
                selected = static_cast<std::uint16_t>(random_.irandom(40));
            } while (selected == mainBuilderLastMainPattern_);
            if (!builderPatternPack_->findPattern(
                    BuilderPatternFamily::areaOneMain, selected, pattern)) {
                deferredCommandsSucceeded_ = false;
                return;
            }
        }
        const bool reversed = makeShop ? false : random_.irandom(1) != 0;
        mainBuilderLastPatternReversed_ = reversed;
        if (!emitPattern(pattern, reversed, true, advancedRows)) {
            deferredCommandsSucceeded_ = false;
            return;
        }
        if (scheduledSideRoom) {
            if (makeShop) {
                mainBuilderLastShopPattern_ = selected;
                ++mainBuilderShopsGenerated_;
                mainBuilderShopMade_ = true;
            } else {
                mainBuilderLastRightPattern_ = selected;
                ++mainBuilderSideRoomsGenerated_;
            }
        } else {
            mainBuilderLastMainPattern_ = selected;
            ++mainBuilderMainPatternsGenerated_;
        }
        ++mainBuilderGenerationCount_;
    }
    mainBuilderY_ = instance.y;
    mainBuilderRowsGenerated_ += advancedRows;
}

void StartupRuntime::updateMainPlayer(InstanceRecord& instance) {
    if (!mainPlayerInitialized_)
        return;

    // The first operation in recovered objPlayer_n Begin Step, before motion.
    if (mainCameraViewY() + surfaceHeight > mainActivationY_ - 112) {
        mainActivationY_ += 16;
        for (std::size_t index = 0; index < instances_.size(); ++index) {
            InstanceRecord& enemy = instances_.at(index);
            const bool enemyObject = enemy.objectId == 111 || enemy.objectId == 112 ||
                enemy.objectId == 113 || enemy.objectId == 116 || enemy.objectId == 117 ||
                enemy.objectId == 119;
            if (!enemy.active && enemy.activationPending && enemyObject &&
                enemy.y <= mainActivationY_) {
                enemy.active = true;
                enemy.activationPending = false;
                ++mainEnemiesActivated_;
            }
        }
    }

    const bool wasGrounded = mainPlayerMotion_.grounded();
    const unsigned priorShots = mainPlayerMotion_.shots();
    const unsigned priorJumps = mainPlayerMotion_.jumps();
    const int priorAmmo = mainPlayerMotion_.ammo();
    // Rebind adapters per update so copying a runtime cannot leave callbacks
    // pointing into the original instance store.
    mainPlayerMotion_.configureSolidResolver([](void* context, int x, int y) {
        const auto& runtime = *static_cast<StartupRuntime*>(context);
        return runtime.mainSolidAt(x + runtime.mainPlayerOriginX_,
                                   y + runtime.mainPlayerOriginY_);
    }, this, 4 - mainPlayerOriginX_, 476 - mainPlayerOriginX_);
    mainPlayerMotion_.configureLandingResolver([](void* context, int x, int minimumY) {
        const auto& runtime = *static_cast<StartupRuntime*>(context);
        const int worldX = x + runtime.mainPlayerOriginX_;
        int nearest = 100000;
        for (std::size_t i = 0; i < runtime.instances_.size(); ++i) {
            const auto& tile = runtime.instances_.at(i);
            if (!tile.active || (tile.objectId != 60 && tile.objectId != 92 &&
                                 tile.objectId != 93)) continue;
            const int landing = tile.y - 16 - runtime.mainPlayerOriginY_;
            if (landing >= minimumY && landing < nearest &&
                worldX + 3 >= tile.x - 8 && worldX - 4 <= tile.x + 7)
                nearest = landing;
        }
        return nearest;
    }, this);
    mainPlayerMotion_.configureBreakBoxResolver([](void* context, long x, long y) {
        auto& runtime = *static_cast<StartupRuntime*>(context);
        for (std::size_t i = 0; i < runtime.instances_.size(); ++i) {
            auto& box = runtime.instances_.at(i);
            if (box.active && box.objectId == 92 && playerMaskMeetsBox(
                    x, y, (long)(box.x - runtime.mainPlayerOriginX_) * 1280,
                    (long)(box.y - runtime.mainPlayerOriginY_) * 1280, 1280L)) {
                box.active = false;
                ++runtime.mainBlocksDestroyed_;
                ++runtime.soundPlayRequests_;
                return true;
            }
        }
        return false;
    }, this);
    Input input;
    input.left = inputLeft_;
    input.right = inputRight_;
    input.fire = inputFire_;
    mainPlayerMotion_.step(input, controlsLocked_);
    mainPlayerMotion_.configureSolidResolver(nullptr, nullptr,
        4 - mainPlayerOriginX_, 476 - mainPlayerOriginX_);
    mainPlayerMotion_.configureLandingResolver(nullptr, nullptr);
    mainPlayerMotion_.configureBreakBoxResolver(nullptr, nullptr);
    syncMainPlayerState();
    if (mainJumps_ != priorJumps) ++soundPlayRequests_;
    if (mainShotsFired_ != priorShots) emitMainBullet(instance);
    if (!wasGrounded && mainPlayerGrounded_) {
        ++mainLandings_;
        if (priorAmmo < mainAmmo_) ++soundPlayRequests_;
    }
    instance.x = static_cast<std::int32_t>(roundGameMakerCoordinate(mainPlayerX_));
    instance.y = static_cast<std::int32_t>(roundGameMakerCoordinate(mainPlayerY_));
    instance.imageIndex = mainPlayerImageIndex_;
}

void StartupRuntime::updateMainCamera(InstanceRecord& instance) {
    if (!mainCameraInitialized_ || !mainPlayerInitialized_)
        return;
    mainCameraMotion_.step(static_cast<long>(std::round(mainPlayerX_ * 256.0F)),
                           static_cast<long>(std::round(mainPlayerY_ * 256.0F)), mainPlayerHealth_ == 0);
    mainCameraX_ = mainCameraMotion_.x / 4096.0F;
    mainCameraY_ = mainCameraMotion_.y / 4096.0F;
    mainCameraTargetY_ = mainCameraMotion_.targetY / 4096.0F;
    instance.x = static_cast<std::int32_t>(std::round(mainCameraX_));
    instance.y = static_cast<std::int32_t>(std::round(mainCameraY_));
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
                mainPlayerMotion_ = PlayerMotion(false);
                mainPlayerMotion_.initializeEntryInput(inputFire_);
                syncMainPlayerState();
                mainPlayerHealth_ = 4;
                mainPlayerInvulnerability_ = 0;
                mainShootLeg_ = -1;
                mainShotsFired_ = 0;
                mainBlocksDestroyed_ = 0;
                mainLandings_ = 0;
                mainActivationY_ = 0;
                mainEnemiesActivated_ = 0;
                mainInstancesRetired_ = 0;
                mainEnemiesDefeated_ = 0;
                mainJumps_ = 0;
                mainBulletCount_ = 0;
                mainCurrencyCount_ = 0;
                mainCurrencyTotal_ = 0;
                mainGameGemTotal_ = 0;
                mainGemStreak_ = 0;
                mainGemStreakTimer_ = 0;
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
            areaOneBuilder_.reset(instance->x, instance->y, random_);
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
            instance->imageSpeed = 0.25F;
            instance->health = 5;
            instance->reward = 6;
            if (!instance->motionInitialized) {
                instance->preciseX = static_cast<float>(instance->x);
                instance->preciseY = static_cast<float>(instance->y);
                instance->velocityX = 0.5F;
                instance->motionInitialized = true;
                instance->grounded = true;
            }
        }
        return;
    case 1198: // gml_Object_enmGroundTurtle_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            instance->imageSpeed = 0.15F;
            instance->health = 5;
            instance->reward = 14;
            if (!instance->motionInitialized) {
                instance->preciseX = static_cast<float>(instance->x);
                instance->preciseY = static_cast<float>(instance->y);
                instance->velocityX = 0.25F;
                instance->motionInitialized = true;
                instance->grounded = true;
            }
        }
        return;
    case 1204: // gml_Object_enmFrog_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            instance->imageSpeed = 0.0F;
            instance->health = 35;
            instance->reward = 20;
            if (!instance->motionInitialized) {
                instance->preciseX = static_cast<float>(instance->x);
                instance->preciseY = static_cast<float>(instance->y);
                instance->motionInitialized = true;
            }
        }
        return;
    case 1230: // gml_Object_enmBat_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            instance->imageSpeed = 0.2F;
            instance->health = 15;
            instance->reward = 12;
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
            instance->imageSpeed = 0.1F;
            instance->health = 20;
            instance->reward = 24;
            if (!instance->motionInitialized) {
                instance->preciseX = static_cast<float>(instance->x);
                instance->preciseY = static_cast<float>(instance->y);
                instance->velocityY = 0.1F;
                instance->motionInitialized = true;
            }
        }
        return;
    case 1252: // gml_Object_enmCrawler_Create_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            instance->imageSpeed = 0.1F;
            instance->health = 3;
            instance->reward = 6;
            if (!instance->motionInitialized) {
                instance->preciseX = static_cast<float>(instance->x);
                instance->preciseY = static_cast<float>(instance->y);
                instance->moveSpeed = 1.0F;
                instance->motionState = 1;
                instance->motionInitialized = true;
            }
        }
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
        const std::uint64_t choice = random_.irandom(5);
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
                random_.irandom(3));
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
            for (int effect = 0; effect < 20; ++effect) {
                random_.random(140.0);
                random_.random(32.0);
                random_.irandom(1);
                random_.random(0.165);
                random_.random(0.1);
            }
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
            if (menuTitleFirstBoot_ && instance->imageIndex < 9.0F &&
                random_.irandom(1) != 0) {
                random_.random(132.0);
                random_.random(32.0);
                random_.irandom(1);
                random_.random(0.4);
            }
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

void StartupRuntime::flushDeferredInstances(
    InstanceStore<instanceCapacity>& instances,
    std::int32_t roomId) {
    if (objectPack_ == nullptr || deferredInstances_.size() == 0)
        return;
    const auto createHandler = [this](InstanceRecord& instance) {
        dispatchInstancePhase(instance, RoomInstancePhase::preCreate);
        dispatchInstancePhase(instance, RoomInstancePhase::create);
    };
    const ApplyInstanceCommandsResult result = deferredInstances_.apply(
        instances, *objectPack_, roomId, nextRuntimeInstanceId_, createHandler);
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

void StartupRuntime::gotoRoom(int roomId) {
    const auto instanceHandler = [this](InstanceRecord& instance,
                                        RoomInstancePhase phase) {
        dispatchInstancePhase(instance, phase);
    };
    const auto roomCreationHandler = [this](std::int32_t loadedRoomId) {
        dispatchRoomCreation(loadedRoomId);
    };
    const auto deferredHandler = [this, roomId](
                                     InstanceStore<instanceCapacity>& instances) {
        flushDeferredInstances(instances, roomId);
    };
    if (roomPack_ != nullptr) {
        roomLoadSucceeded_ = PackedRoomLoader<instanceCapacity>::enter(
            instances_, *roomPack_, roomId, false,
            instanceHandler, roomCreationHandler,
            deferredHandler) == PackedRoomLoadResult::success;
    } else {
        roomLoadSucceeded_ = RoomLoader<instanceCapacity>::enter(
            instances_, roomId, nullptr, 0, false,
            instanceHandler, roomCreationHandler,
            deferredHandler) == LoadRoomResult::success;
    }
    if (roomLoadSucceeded_)
        roomId_ = roomId;
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
    digest.addU32(33); // Includes authoritative crawler motion.
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
    digest.addI32(mainPlayerInvulnerability_);
    digest.addI32(mainShootLeg_);
    digest.addU32(mainShotsFired_);
    digest.addU32(mainBlocksDestroyed_);
    digest.addU32(mainLandings_);
    digest.addI32(mainActivationY_);
    digest.addU32(mainEnemiesActivated_);
    digest.addU32(mainInstancesRetired_);
    digest.addU32(mainEnemiesDefeated_);
    digest.addU32(mainJumps_);
    digest.addU32(static_cast<std::uint32_t>(mainBulletCount_));
    for (std::size_t index = 0; index < mainBulletCount_; ++index) {
        digest.addF32(mainBullets_[index].x);
        digest.addF32(mainBullets_[index].y);
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
    digest.addU32(mainCurrencyTotal_);
    digest.addU32(mainGameGemTotal_);
    digest.addU32(mainGemStreak_);
    digest.addU32(mainGemStreakTimer_);
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
        digest.addI32(instance.spriteId);
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
        digest.addBool(instance.frogLanded);
        if (instance.objectId == 113) {
            digest.addBool(instance.frogMotionInitialized);
            digest.addI32(instance.frogHorizontal.position);
            digest.addI32(instance.frogHorizontal.velocity);
            digest.addI32(instance.frogVertical.position);
            digest.addI32(instance.frogVertical.velocity);
            digest.addBool(instance.frogVertical.grounded);
            digest.addBool(instance.frogHit.pending);
            digest.addBool(instance.frogHit.ready);
            digest.addU32(instance.frogHit.remaining);
        }
        for (std::size_t alarm = 0; alarm < AlarmBank::count; ++alarm)
            digest.addI32(instance.alarms.get(alarm));
    }
    return digest.value();
}

} // namespace downwell
