#ifndef DOWNWELL_CORE_STARTUP_RUNTIME_H
#define DOWNWELL_CORE_STARTUP_RUNTIME_H

#include "InstanceStore.h"
#include "DeferredInstanceQueue.h"
#include "Input.h"
#include <cstdint>

namespace downwell {

class RoomPack;
class ObjectPack;
enum class RoomInstancePhase;

enum class PlatformProfile {
    desktop,
    nonDesktop
};

// Minimal, auditable model of the recovered rmInit startup path. This is kept
// separate from presentation so its tick behavior can be tested exactly.
class StartupRuntime {
public:
    static constexpr std::size_t instanceCapacity = 256;
    static constexpr int entryRoomId = 7;
    static constexpr int menuRoomId = 20;
    static constexpr std::uint32_t controllerInstanceId = 100065;
    static constexpr std::uint32_t initRoomInstanceId = 100066;
    static constexpr int surfaceWidth = 160;
    static constexpr int surfaceHeight = 284;

    explicit StartupRuntime(PlatformProfile profile = PlatformProfile::desktop,
                            const RoomPack* roomPack = nullptr,
                            const ObjectPack* objectPack = nullptr);

    void reset(PlatformProfile profile);
    void step(bool advancePressed);
    void step(const Input& input, bool advancePressed);

    std::uint64_t tick() const { return tick_; }
    int roomId() const { return roomId_; }
    int roomDestination() const { return roomDestination_; }
    int showSplash() const { return showSplash_; }
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
    bool inMenu() const { return roomId_ == menuRoomId; }
    bool menuPlayerNapping() const { return menuPlayerNapping_; }
    int menuPlayerSpriteId() const { return menuPlayerSpriteId_; }
    float menuPlayerImageIndex() const { return menuPlayerImageIndex_; }
    int menuPlayerFacing() const { return menuPlayerFacing_; }
    float menuPlayerX() const { return menuPlayerX_; }
    float menuCameraViewX() const { return menuCameraX_ - surfaceWidth / 2.0F; }
    float menuCameraViewY() const { return menuCameraY_ - surfaceHeight / 2.0F; }
    bool menuTitleReady() const { return menuTitleReady_; }
    std::uint64_t stateDigest() const;

private:
    void controllerCreate(PlatformProfile profile);
    void initRoomCreate();
    void initRoomAlarm0();
    void initRoomEndStep(bool advancePressed);
    void stepInternal(bool advancePressed, bool left, bool right, bool anyInput);
    void updateMenuPlayer(InstanceRecord& instance);
    void updateMenuCamera();
    void dispatchInstancePhase(InstanceRecord& instance,
                               RoomInstancePhase phase);
    bool dispatchObjectEvent(InstanceRecord& instance,
                             std::int32_t eventType,
                             std::int32_t eventSubtype,
                             bool advancePressed = false);
    void dispatchRoomCreation(std::int32_t roomId);
    void flushDeferredInstances(InstanceStore<instanceCapacity>& instances,
                                std::int32_t roomId);
    void executeCode(InstanceRecord* instance,
                     std::int32_t codeId,
                     bool advancePressed = false);
    void gotoRoom(int roomId);
    InstanceRecord* initRoomInstance();
    const InstanceRecord* initRoomInstance() const;

    std::uint64_t tick_ = 0;
    int roomId_ = entryRoomId;
    int roomDestination_ = 0;
    int showSplash_ = 0;
    InstanceStore<instanceCapacity> instances_;
    const RoomPack* roomPack_ = nullptr;
    const ObjectPack* objectPack_ = nullptr;
    PlatformProfile profile_ = PlatformProfile::desktop;
    int initTimes_ = 0;
    int achievementLoginCalls_ = 0;
    int splashSpeed_ = 180;
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
    DeferredInstanceQueue<8> deferredInstances_;
    std::uint32_t nextRuntimeInstanceId_ = 105186;
    bool deferredCommandsSucceeded_ = true;
    bool inputLeft_ = false;
    bool inputRight_ = false;
    bool anyInput_ = false;
    bool menuPlayerInitialized_ = false;
    bool menuPlayerNapping_ = false;
    float menuPlayerX_ = 112.0F;
    float menuPlayerY_ = 512.0F;
    float menuPlayerSpeedX_ = 0.0F;
    int menuPlayerFacing_ = 1;
    int menuPlayerSpriteId_ = 31;
    float menuPlayerImageIndex_ = 0.0F;
    float menuPlayerImageSpeed_ = 0.1F;
    float menuCameraX_ = 112.0F;
    float menuCameraY_ = 512.0F;
    bool menuTitleFirstBoot_ = false;
    bool menuTitleReady_ = false;
    float menuTitleStartY_ = 432.0F;
};

} // namespace downwell

#endif
