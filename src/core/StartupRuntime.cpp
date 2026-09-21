#include "StartupRuntime.h"
#include "StateDigest.h"

namespace downwell {

StartupRuntime::StartupRuntime(PlatformProfile profile) {
    reset(profile);
}

void StartupRuntime::reset(PlatformProfile profile) {
    tick_ = 0;
    roomId_ = entryRoomId;
    roomDestination_ = 0;
    showSplash_ = 0;
    alarm0_ = -1;
    initTimes_ = 0;
    achievementLoginCalls_ = 0;
    splashSpeed_ = 180;
    controllerPersistent_ = false;
    roomDestinationSet_ = false;

    // rmInit instance order in the recovered room data.
    controllerCreate(profile);
    initRoomCreate();
}

void StartupRuntime::controllerCreate(PlatformProfile profile) {
    controllerPersistent_ = true;

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
    ++tick_;

    // GameMaker alarms count down once per step and dispatch on reaching zero.
    if (alarm0_ > 0) {
        --alarm0_;
        if (alarm0_ == 0)
            initRoomAlarm0();
    }

    if (roomId_ == entryRoomId)
        initRoomEndStep(advancePressed);
}

void StartupRuntime::initRoomAlarm0() {
    ++showSplash_;
    alarm0_ = splashSpeed_;
    if (showSplash_ == 4)
        alarm0_ = 240;
    if (showSplash_ >= 5)
        alarm0_ = 60;
}

void StartupRuntime::initRoomEndStep(bool advancePressed) {
    if (!roomDestinationSet_) {
        ++achievementLoginCalls_;

        // The recovered code assigns rRand = 0. With all five weights set to
        // one, the first branch deterministically selects rmMenu (ID 20).
        roomDestination_ = menuRoomId;
        roomDestinationSet_ = true;
        if (showSplash_ != 0)
            alarm0_ = 10;
        else
            gotoRoom(roomDestination_);
    }

    if (showSplash_ != 0) {
        if (advancePressed) {
            ++showSplash_;
            alarm0_ = 60;
        }
        if (showSplash_ >= 7) {
            showSplash_ = 0;
            gotoRoom(roomDestination_);
        }
    }
}

void StartupRuntime::gotoRoom(int roomId) {
    roomId_ = roomId;
    alarm0_ = -1;
}

std::uint64_t StartupRuntime::stateDigest() const {
    StateDigest digest;
    digest.addU32(1); // Startup-state digest schema.
    digest.addU64(tick_);
    digest.addI32(roomId_);
    digest.addI32(roomDestination_);
    digest.addI32(showSplash_);
    digest.addI32(alarm0_);
    digest.addI32(initTimes_);
    digest.addI32(achievementLoginCalls_);
    digest.addI32(splashSpeed_);
    digest.addBool(controllerPersistent_);
    digest.addBool(roomDestinationSet_);
    return digest.value();
}

} // namespace downwell
