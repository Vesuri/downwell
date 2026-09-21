#include "core/StartupRuntime.h"
#include "core/ObjectPack.h"
#include "core/RoomPack.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void stepMany(downwell::StartupRuntime& runtime, int count) {
    for (int i = 0; i < count; ++i)
        runtime.step(false);
}

void testDesktopBootState() {
    downwell::StartupRuntime runtime;
    require(runtime.roomId() == 7, "rmInit is the entry room");
    require(runtime.controllerPersistent(), "controller is persistent");
    require(runtime.activeInstanceCount() == 2, "rmInit creates two instances");
    require(runtime.hasInstance(100065), "controller uses recovered instance ID");
    require(runtime.hasInstance(100066), "transition object uses recovered instance ID");
    require(runtime.initTimes() == 1, "scrInitialize increments initTimes");
    require(runtime.showSplash() == 1, "desktop enables splash sequence");
    require(!runtime.roomDestinationSet(), "destination starts unset");
    require(runtime.cameraViewWidth() == 160 && runtime.cameraViewHeight() == 284,
            "rmInit creation code configures recovered camera size");
}

void testNonDesktopSkipsSplash() {
    downwell::StartupRuntime runtime(downwell::PlatformProfile::nonDesktop);
    runtime.step(false);
    require(runtime.roomId() == 20, "non-desktop reaches rmMenu immediately");
    require(runtime.roomDestination() == 20, "rmMenu is selected deterministically");
    require(runtime.achievementLoginCalls() == 1, "login is requested once");
    require(runtime.activeInstanceCount() == 1, "room transition removes room-owned instance");
    require(runtime.hasInstance(100065), "persistent controller survives room transition");
    require(!runtime.hasInstance(100066), "rmInit transition object is removed");
    require(runtime.backgroundMusicId() == 193,
            "rmMenu selects recovered bgmOverworld resource");
    require(runtime.controlsLocked(), "rmMenu locks player control");
    require(runtime.soundGainRequests() == 1 && runtime.soundPlayRequests() == 1,
            "rmMenu emits deterministic audio requests");
    require(runtime.ditherFadeSpawnRequests() == 1,
            "rmMenu records its deferred ditherFade spawn");
}

void testRecoveredDesktopTiming() {
    downwell::StartupRuntime runtime;
    runtime.step(false);
    require(runtime.alarm0() == 10, "first End Step arms ten-tick splash alarm");

    stepMany(runtime, 9);
    require(runtime.showSplash() == 1 && runtime.alarm0() == 1,
            "alarm has not fired a tick early");
    runtime.step(false);
    require(runtime.showSplash() == 2 && runtime.alarm0() == 180,
            "first splash alarm advances to stage two");

    stepMany(runtime, 180);
    require(runtime.showSplash() == 3 && runtime.alarm0() == 180,
            "stage three uses normal splash speed");
    stepMany(runtime, 180);
    require(runtime.showSplash() == 4 && runtime.alarm0() == 240,
            "stage four uses extended delay");
    stepMany(runtime, 240);
    require(runtime.showSplash() == 5 && runtime.alarm0() == 60,
            "later splash stages use short delay");
    stepMany(runtime, 60);
    require(runtime.showSplash() == 6 && runtime.roomId() == 7,
            "stage six remains in rmInit");
    stepMany(runtime, 60);
    require(runtime.showSplash() == 0 && runtime.roomId() == 20,
            "stage seven transitions to rmMenu");
    require(runtime.tick() == 731, "desktop automatic transition tick is stable");
}

void testAdvanceInputUsesPressEdges() {
    downwell::StartupRuntime runtime;
    runtime.step(false);
    for (int expected = 2; expected <= 6; ++expected) {
        runtime.step(true);
        require(runtime.showSplash() == expected, "advance press increments splash stage");
        runtime.step(false);
    }
    runtime.step(true);
    require(runtime.inMenu(), "six advance presses reach rmMenu");
}

void testGeneratedPackEntersRecoveredMenu(const char* roomPath,
                                          const char* objectPath) {
    std::ifstream input(roomPath, std::ios::binary);
    const std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());
    const downwell::RoomPack pack(bytes.data(), bytes.size());
    require(pack.valid(), "generated room pack is valid");

    std::vector<std::uint8_t> objectBytes;
    if (objectPath != nullptr) {
        std::ifstream objectInput(objectPath, std::ios::binary);
        objectBytes.assign(std::istreambuf_iterator<char>(objectInput),
                           std::istreambuf_iterator<char>());
    }
    const downwell::ObjectPack objectPack(objectBytes.data(), objectBytes.size());
    const downwell::ObjectPack* objects = objectPack.valid() ? &objectPack : nullptr;
    if (objectPath != nullptr)
        require(objects != nullptr, "generated object pack is valid");

    downwell::StartupRuntime runtime(
        downwell::PlatformProfile::nonDesktop, &pack, objects);
    require(runtime.roomLoadSucceeded(), "packed rmInit loads");
    require(runtime.activeInstanceCount() == 2,
            "packed rmInit creates two instances");
    if (objects != nullptr) {
        require(runtime.recoveredEventBindingsActive(),
                "recovered event bindings are active");
        require(runtime.implementedCodeCalls() == 3,
                "rmInit room code and both Create handlers route by code ID");
        require(runtime.unimplementedCodeCalls() > 0,
                "unsupported recovered startup code is measured explicitly");
    }

    runtime.step(false);
    require(runtime.inMenu(), "packed startup reaches rmMenu");
    require(runtime.roomLoadSucceeded(), "packed rmMenu loads");
    require(runtime.activeInstanceCount() == (objects != nullptr ? 201U : 193U),
            "rmMenu contains recovered, persistent, and translated instances");
    require(runtime.hasInstance(100065), "controller persists into packed rmMenu");
    require(!runtime.hasInstance(100066), "rmInit object leaves packed rmMenu");

    const downwell::InstanceRecord* player = runtime.instance(104218);
    require(player != nullptr, "first recovered rmMenu player exists");
    require(player->objectId == 35, "rmMenu player has recovered object ID");
    require(player->roomId == 20, "rmMenu player belongs to recovered room");
    if (objects != nullptr)
        require(runtime.implementedCodeCalls() == 12,
                "room transition and translated Create events route by code ID");
    if (objects != nullptr) {
        require(runtime.instance(105186) != nullptr &&
                    runtime.instance(105186)->objectId == 66,
                "player Create spawns the depth meter first");
        require(runtime.instance(105187) != nullptr &&
                    runtime.instance(105187)->objectId == 64,
                "player Create spawns camMain second");
        const downwell::InstanceRecord* title = runtime.instance(105192);
        require(title != nullptr && title->objectId == 317 &&
                    title->x == 328 && title->y == 432 &&
                    title->imageSpeed == 0.0F,
                "well Create spawns the stationary first-boot title");
        const downwell::InstanceRecord* fade = runtime.instance(105193);
        require(fade != nullptr && fade->objectId == 382 && fade->roomId == 20 &&
                    fade->x == 0 && fade->y == 0,
                "rmMenu creates ditherFade with the authoritative runtime ID");
        require(fade->imageSpeed == 0.0F && fade->alarms.get(0) == 30,
                "ditherFade Create freezes animation and arms Alarm 0");
        require(runtime.nextRuntimeInstanceId() == 105194,
                "runtime allocator advances over all recovered Create spawns");
        require(runtime.deferredCommandsSucceeded(),
                "deferred room-creation commands apply successfully");

        stepMany(runtime, 30);
        fade = runtime.instance(105193);
        require(fade != nullptr && fade->imageSpeed == 0.2F,
                "ditherFade Alarm 0 starts recovered animation speed");
        require(!runtime.controlsLocked(),
                "ditherFade Alarm 0 restores player control");

        require(runtime.menuPlayerNapping(),
                "groundPlayerSet starts the menu player asleep");
        downwell::Input wake;
        wake.right = true;
        runtime.step(wake, false);
        require(!runtime.menuPlayerNapping(),
                "the first enabled direction input wakes the player");
        require(runtime.menuPlayerSpriteId() == 31,
                "waking selects the default idle sprite");
        for (int tick = 0; tick < 10; ++tick)
            runtime.step(wake, false);
        require(runtime.menuPlayerX() > 122.9F && runtime.menuPlayerX() < 123.1F,
                "recovered ground acceleration reaches max speed over ten ticks");
        require(runtime.menuPlayerSpriteId() == 27,
                "horizontal movement selects the default run sprite");
        require(runtime.menuCameraViewX() > 32.0F,
                "free cam begins following the moving player");

        for (int tick = 0; tick < 105; ++tick)
            runtime.step(wake, false);
        title = runtime.instance(105192);
        require(title != nullptr && title->alarms.get(2) >= 0,
                "crossing beneath the title arms its recovered reveal delay");
        for (int tick = 0; tick < 50; ++tick)
            runtime.step(wake, false);
        title = runtime.instance(105192);
        require(title != nullptr && title->imageIndex == 9.0F &&
                    title->imageSpeed == 0.0F,
                "title reveal reaches and holds its final recovered frame");
        require(!runtime.menuPlayerGrounded() && runtime.menuPlayerY() >= 625.0F,
                "player falls through the recovered 64-pixel well opening");
        require(runtime.menuTeleportActive(),
                "touching the shaft trigger activates objTeleportMain");
        for (int tick = 0; tick < 76; ++tick)
            runtime.step(wake, false);
        require(runtime.menuTitleReady(),
                "title completion eventually enables the teleporter sequence");

        for (int tick = 0; tick < 140 && !runtime.inPlayMenu(); ++tick)
            runtime.step(wake, false);
        require(runtime.inPlayMenu(),
                "EnterSequence acceleration reaches rmPlayMenu");
        require(runtime.hasInstance(100067),
                "rmPlayMenu creates its recovered PlayMenu instance");
        require(runtime.activeInstanceCount() == 2,
                "only the persistent controller and PlayMenu survive transition");
        downwell::Input released;
        for (int tick = 0; tick < 100; ++tick)
            runtime.step(released, false);
        require(runtime.playMenuYOffset() == 0.0F,
                "style menu entrance easing settles exactly");
        runtime.step(wake, false);
        require(runtime.playMenuCursor() == 1,
                "a right press advances the recovered style carousel");

        downwell::Input fire;
        fire.fire = true;
        runtime.step(fire, true);
        require(!runtime.playMenuSelected(),
                "confirming a locked style has no effect");
        downwell::Input left;
        left.left = true;
        runtime.step(left, false);
        require(runtime.playMenuCursor() == 0,
                "a left press returns to the unlocked default style");
        runtime.step(released, false);
        runtime.step(fire, true);
        require(runtime.playMenuSelected() && runtime.playStyle() == 0,
                "fire confirms and stores the unlocked style");
        require(runtime.playMenuPlayerDescent() > 0.0F &&
                    runtime.playMenuVerticalDitherY() == -310.0F,
                "confirmation starts the recovered falling dither transition");
        for (int tick = 0; tick < 63; ++tick)
            runtime.step(released, false);
        require(runtime.inPlayMenu() &&
                    runtime.playMenuVerticalDitherY() == 320.0F,
                "the vertical dither takes exactly 64 frames to cross");
        runtime.step(released, false);
        require(runtime.inMainRoom(),
                "the next frame enters the recovered rmMain room");
        require(runtime.roomLoadSucceeded(),
                "rmMain fits and loads from the recovered room pack");
        require(runtime.hasInstance(100855),
                "rmMain creates its authoritative player instance");
        require(runtime.mainPlayerX() == 240.0F &&
                    runtime.mainPlayerY() == 176.0F,
                "rmMain initializes player motion from the recovered spawn");
        downwell::Input fallRight;
        fallRight.right = true;
        for (int tick = 0; tick < 10; ++tick)
            runtime.step(fallRight, false);
        require(runtime.mainPlayerX() > 250.9F &&
                    runtime.mainPlayerX() < 251.1F,
                "air control uses recovered acceleration over ten ticks");
        require(runtime.mainPlayerY() > 186.9F &&
                    runtime.mainPlayerY() < 187.1F,
                "rmMain gravity advances the initial fall deterministically");
        require(runtime.mainPlayerSpeedX() > 1.9F &&
                    runtime.mainPlayerSpeedX() < 2.1F &&
                    runtime.mainPlayerSpeedY() > 1.9F &&
                    runtime.mainPlayerSpeedY() < 2.1F,
                "air and gravity speeds retain recovered subpixel values");
        require(runtime.mainPlayerSpriteId() == 37 &&
                    runtime.mainPlayerImageIndex() == 4.0F,
                "fall speed selects the recovered airborne animation frame");
        require(runtime.mainCameraViewY() < 34.0F,
                "rmMain camera biases the falling player toward the lower view");
        for (int tick = 0; tick < 20; ++tick)
            runtime.step(fallRight, false);
        require(runtime.mainPlayerX() < 285.0F,
                "the recovered player mask cannot cross the shaft wall mask");
    }
}

} // namespace

int main(int argc, char** argv) {
    testDesktopBootState();
    testNonDesktopSkipsSplash();
    testRecoveredDesktopTiming();
    testAdvanceInputUsesPressEdges();
    if (argc >= 2)
        testGeneratedPackEntersRecoveredMenu(argv[1], argc >= 3 ? argv[2] : nullptr);
    std::cout << "startup runtime tests passed\n";
    return 0;
}
