#ifndef DOWNWELL_CORE_STARTUP_RUNTIME_H
#define DOWNWELL_CORE_STARTUP_RUNTIME_H

#include <cstdint>

namespace downwell {

enum class PlatformProfile {
    desktop,
    nonDesktop
};

// Minimal, auditable model of the recovered rmInit startup path. This is kept
// separate from presentation so its tick behavior can be tested exactly.
class StartupRuntime {
public:
    static constexpr int entryRoomId = 7;
    static constexpr int menuRoomId = 20;
    static constexpr int surfaceWidth = 160;
    static constexpr int surfaceHeight = 284;

    explicit StartupRuntime(PlatformProfile profile = PlatformProfile::desktop);

    void reset(PlatformProfile profile);
    void step(bool advancePressed);

    std::uint64_t tick() const { return tick_; }
    int roomId() const { return roomId_; }
    int roomDestination() const { return roomDestination_; }
    int showSplash() const { return showSplash_; }
    int alarm0() const { return alarm0_; }
    int initTimes() const { return initTimes_; }
    int achievementLoginCalls() const { return achievementLoginCalls_; }
    bool controllerPersistent() const { return controllerPersistent_; }
    bool roomDestinationSet() const { return roomDestinationSet_; }
    bool inMenu() const { return roomId_ == menuRoomId; }

private:
    void controllerCreate(PlatformProfile profile);
    void initRoomCreate();
    void initRoomAlarm0();
    void initRoomEndStep(bool advancePressed);
    void gotoRoom(int roomId);

    std::uint64_t tick_ = 0;
    int roomId_ = entryRoomId;
    int roomDestination_ = 0;
    int showSplash_ = 0;
    int alarm0_ = -1;
    int initTimes_ = 0;
    int achievementLoginCalls_ = 0;
    int splashSpeed_ = 180;
    bool controllerPersistent_ = false;
    bool roomDestinationSet_ = false;
};

} // namespace downwell

#endif
