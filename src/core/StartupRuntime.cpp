#include "StartupRuntime.h"
#include "EventScheduler.h"
#include "ObjectPack.h"
#include "PackedRoomLoader.h"
#include "RoomLoader.h"
#include "StateDigest.h"
#include <cmath>

namespace downwell {

StartupRuntime::StartupRuntime(PlatformProfile profile,
                               const RoomPack* roomPack,
                               const ObjectPack* objectPack)
    : roomPack_(roomPack), objectPack_(objectPack) {
    reset(profile);
}

void StartupRuntime::reset(PlatformProfile profile) {
    tick_ = 0;
    profile_ = profile;
    roomId_ = entryRoomId;
    roomDestination_ = 0;
    showSplash_ = 0;
    instances_.clear();
    initTimes_ = 0;
    achievementLoginCalls_ = 0;
    splashSpeed_ = 180;
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
    anyInput_ = false;
    menuPlayerInitialized_ = false;
    menuPlayerNapping_ = false;
    menuPlayerX_ = 112.0F;
    menuPlayerY_ = 512.0F;
    menuPlayerSpeedX_ = 0.0F;
    menuPlayerFacing_ = 1;
    menuPlayerSpriteId_ = 31;
    menuPlayerImageIndex_ = 0.0F;
    menuPlayerImageSpeed_ = 0.1F;
    menuCameraX_ = 112.0F;
    menuCameraY_ = 512.0F;
    menuTitleFirstBoot_ = false;
    menuTitleReady_ = false;
    menuTitleStartY_ = 432.0F;

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
    stepInternal(advancePressed, false, false, advancePressed);
}

void StartupRuntime::step(const Input& input, bool advancePressed) {
    stepInternal(advancePressed, input.left, input.right,
                 input.left || input.right || input.fire || input.pause);
}

void StartupRuntime::stepInternal(bool advancePressed,
                                  bool left,
                                  bool right,
                                  bool anyInput) {
    ++tick_;
    inputLeft_ = left;
    inputRight_ = right;
    anyInput_ = anyInput;

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
}

void StartupRuntime::updateMenuPlayer(InstanceRecord& instance) {
    if (!menuPlayerInitialized_)
        return;

    if (menuPlayerNapping_) {
        // groundPlayerSet() chooses one of six valid poses. The host reference
        // fixes choice zero until the recovered RNG stream is wired in.
        menuPlayerX_ = 112.0F;
        menuPlayerY_ = 506.0F;
        menuPlayerSpriteId_ = 11; // sprPlayerNap
        menuPlayerImageSpeed_ = 0.015F;
        if (anyInput_ && !controlsLocked_) {
            menuPlayerNapping_ = false;
            menuPlayerY_ = 512.0F;
            menuPlayerSpriteId_ = 31; // sprPlayerIdle
            menuPlayerImageIndex_ = 0.0F;
            menuPlayerImageSpeed_ = 0.1F;
        }
    } else {
        if (controlsLocked_ || (inputLeft_ && inputRight_)) {
            menuPlayerSpeedX_ = 0.0F;
        } else if (inputLeft_) {
            menuPlayerSpeedX_ -= 0.2F;
            if (menuPlayerSpeedX_ > 0.0F)
                menuPlayerSpeedX_ -= 2.0F;
            if (menuPlayerSpeedX_ < -2.0F)
                menuPlayerSpeedX_ = -2.0F;
            menuPlayerFacing_ = -1;
        } else if (inputRight_) {
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

        menuPlayerX_ += menuPlayerSpeedX_;
        if (menuPlayerX_ < 16.0F)
            menuPlayerX_ = 16.0F;
        if (menuPlayerX_ > 400.0F)
            menuPlayerX_ = 400.0F;
        menuPlayerSpriteId_ = menuPlayerSpeedX_ == 0.0F ? 31 : 27;
        menuPlayerImageSpeed_ = menuPlayerSpeedX_ == 0.0F ? 0.1F : 0.3F;
    }

    menuPlayerImageIndex_ += menuPlayerImageSpeed_;
    instance.x = static_cast<std::int32_t>(std::round(menuPlayerX_));
    instance.y = static_cast<std::int32_t>(std::round(menuPlayerY_));
    instance.imageIndex = menuPlayerImageIndex_;
    instance.imageSpeed = menuPlayerImageSpeed_;
}

void StartupRuntime::updateMenuCamera() {
    if (!menuPlayerInitialized_)
        return;
    float targetX = menuPlayerX_;
    if (menuPlayerSpeedX_ != 0.0F)
        targetX += 16.0F * menuPlayerFacing_;
    if (targetX < 80.0F)
        targetX = 80.0F;
    if (targetX > 336.0F)
        targetX = 336.0F;
    const float targetY = menuPlayerY_ - 32.0F;
    menuCameraX_ += (targetX - menuCameraX_) / 10.0F;
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
                menuPlayerInitialized_ = true;
                menuPlayerNapping_ = true;
                menuPlayerX_ = static_cast<float>(instance->x);
                menuPlayerY_ = static_cast<float>(instance->y);
                menuCameraX_ = menuPlayerX_;
                menuCameraY_ = menuPlayerY_;
            }
            // scrPlayerInit's full state comes later. These are the two
            // instance_create calls made directly by the recovered event.
            if (!deferredInstances_.enqueueCreate(66, -100, instance->y) ||
                !deferredInstances_.enqueueCreate(64, instance->x, instance->y))
                deferredCommandsSucceeded_ = false;
        }
        return;
    case 849: // gml_Object_objPlayer_n_Step_1 (ground-room subset)
        ++implementedCodeCalls_;
        if (instance != nullptr && instance->roomId == menuRoomId)
            updateMenuPlayer(*instance);
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
    case 997: // gml_Object_camMain_Create_0 (ground-room subset)
        ++implementedCodeCalls_;
        if (menuPlayerInitialized_) {
            menuCameraX_ = menuPlayerX_;
            menuCameraY_ = menuPlayerY_;
        }
        return;
    case 999: // gml_Object_camMain_Step_0 (free-camera ground-room subset)
        ++implementedCodeCalls_;
        updateMenuCamera();
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
        if (instance != nullptr)
            instance->alarms.set(1, 75);
        return;
    case 2499: // gml_Object_objTitle_Step_0
        ++implementedCodeCalls_;
        if (instance != nullptr) {
            if (!menuPlayerNapping_ &&
                menuPlayerX_ > instance->x - 4.0F &&
                menuPlayerX_ < instance->x + 4.0F &&
                !menuTitleFirstBoot_) {
                instance->alarms.set(2, 15);
                menuTitleFirstBoot_ = true;
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
    digest.addU32(9); // Startup-state digest schema.
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
    digest.addBool(menuPlayerInitialized_);
    digest.addBool(menuPlayerNapping_);
    digest.addF32(menuPlayerX_);
    digest.addF32(menuPlayerY_);
    digest.addF32(menuPlayerSpeedX_);
    digest.addI32(menuPlayerFacing_);
    digest.addI32(menuPlayerSpriteId_);
    digest.addF32(menuPlayerImageIndex_);
    digest.addF32(menuPlayerImageSpeed_);
    digest.addF32(menuCameraX_);
    digest.addF32(menuCameraY_);
    digest.addBool(menuTitleFirstBoot_);
    digest.addBool(menuTitleReady_);
    digest.addF32(menuTitleStartY_);
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
        digest.addI32(instance.creationCodeId);
        digest.addI32(instance.preCreateCodeId);
        digest.addU32(instance.layerId);
        digest.addI32(instance.depth);
        digest.addBool(instance.persistent);
        digest.addBool(instance.active);
        for (std::size_t alarm = 0; alarm < AlarmBank::count; ++alarm)
            digest.addI32(instance.alarms.get(alarm));
    }
    return digest.value();
}

} // namespace downwell
