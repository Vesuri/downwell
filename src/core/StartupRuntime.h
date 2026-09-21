#ifndef DOWNWELL_CORE_STARTUP_RUNTIME_H
#define DOWNWELL_CORE_STARTUP_RUNTIME_H

#include "InstanceStore.h"
#include "DeferredInstanceQueue.h"
#include "GameMakerRandom.h"
#include "Input.h"
#include <array>
#include <cstdint>

namespace downwell {

class RoomPack;
class ObjectPack;
class BuilderPatternPack;
enum class RoomInstancePhase;

enum class PlatformProfile {
    desktop,
    nonDesktop
};

// Minimal, auditable model of the recovered rmInit startup path. This is kept
// separate from presentation so its tick behavior can be tested exactly.
class StartupRuntime {
public:
    struct MainBulletState {
        float x = 0.0F;
        float y = 0.0F;
    };

    static constexpr std::size_t instanceCapacity = 2048;
    static constexpr std::size_t mainBulletCapacity = 16;
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
                            const BuilderPatternPack* builderPatternPack = nullptr);

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
    bool inPlayMenu() const { return roomId_ == playMenuRoomId; }
    bool inMainRoom() const { return roomId_ == mainRoomId; }
    bool menuPlayerNapping() const { return menuPlayerNapping_; }
    int menuPlayerSpriteId() const { return menuPlayerSpriteId_; }
    float menuPlayerImageIndex() const { return menuPlayerImageIndex_; }
    int menuPlayerFacing() const { return menuPlayerFacing_; }
    float menuPlayerX() const { return menuPlayerX_; }
    float menuPlayerY() const { return menuPlayerY_; }
    bool menuPlayerGrounded() const { return menuPlayerGrounded_; }
    float menuCameraViewX() const { return menuCameraX_ - surfaceWidth / 2.0F; }
    float menuCameraViewY() const { return menuCameraY_ - surfaceHeight / 2.0F; }
    bool menuTitleReady() const { return menuTitleReady_; }
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
    float mainPlayerY() const { return mainPlayerY_; }
    float mainPlayerSpeedX() const { return mainPlayerSpeedX_; }
    float mainPlayerSpeedY() const { return mainPlayerSpeedY_; }
    int mainPlayerFacing() const { return mainPlayerFacing_; }
    int mainPlayerSpriteId() const { return mainPlayerSpriteId_; }
    float mainPlayerImageIndex() const { return mainPlayerImageIndex_; }
    float mainCameraViewX() const { return mainCameraX_ - surfaceWidth / 2.0F; }
    float mainCameraViewY() const { return mainCameraY_ - surfaceHeight / 2.0F; }
    int mainAmmo() const { return mainAmmo_; }
    std::uint32_t mainShotsFired() const { return mainShotsFired_; }
    std::size_t mainBulletCount() const { return mainBulletCount_; }
    const MainBulletState& mainBulletAt(std::size_t index) const {
        return mainBullets_[index];
    }
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
    std::uint32_t randomSeed() const { return random_.seed(); }
    std::uint64_t randomDrawCount() const { return random_.drawCount(); }
    std::uint64_t stateDigest() const;

private:
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
    bool mainPlayerCollides(float x, float y) const;
    bool moveMainPlayerAxis(float amount, bool horizontal);
    void fireMainGun(InstanceRecord& instance);
    void updateMainBullets();
    bool mainBulletCollides(float x, float y) const;
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
    const BuilderPatternPack* builderPatternPack_ = nullptr;
    std::uint32_t randomSeed_ = defaultRandomSeed;
    GameMakerRandom random_;
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
    DeferredInstanceQueue<128> deferredInstances_;
    std::uint32_t nextRuntimeInstanceId_ = 105186;
    bool deferredCommandsSucceeded_ = true;
    bool inputLeft_ = false;
    bool inputRight_ = false;
    bool inputLeftPressed_ = false;
    bool inputRightPressed_ = false;
    bool inputFire_ = false;
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
    int mainShootLeg_ = -1;
    std::uint32_t mainShotsFired_ = 0;
    std::array<MainBulletState, mainBulletCapacity> mainBullets_{};
    std::size_t mainBulletCount_ = 0;
    bool mainBuilderInitialized_ = false;
    int mainBuilderY_ = 704;
    int mainBuilderGenerationCount_ = -30;
    std::uint32_t mainBuilderRowsGenerated_ = 0;
    std::array<int, 2> mainBuilderRightRooms_{{-1, -1}};
    int mainBuilderStartPattern_ = -1;
    bool mainBuilderStartReversed_ = false;
    std::uint32_t mainBuilderStartRowsGenerated_ = 0;
    bool mainCameraInitialized_ = false;
    float mainCameraX_ = 240.0F;
    float mainCameraY_ = 176.0F;
    float mainCameraTargetY_ = 176.0F;
};

} // namespace downwell

#endif
