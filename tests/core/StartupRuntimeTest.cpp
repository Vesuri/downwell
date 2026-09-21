#include "core/StartupRuntime.h"
#include "core/BuilderPatternPack.h"
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
                                          const char* objectPath,
                                          const char* builderPatternPath) {
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

    std::vector<std::uint8_t> builderPatternBytes;
    if (builderPatternPath != nullptr) {
        std::ifstream patternInput(builderPatternPath, std::ios::binary);
        builderPatternBytes.assign(std::istreambuf_iterator<char>(patternInput),
                                   std::istreambuf_iterator<char>());
    }
    const downwell::BuilderPatternPack builderPatterns(
        builderPatternBytes.data(), builderPatternBytes.size());
    const downwell::BuilderPatternPack* patterns =
        builderPatterns.valid() ? &builderPatterns : nullptr;
    if (builderPatternPath != nullptr)
        require(patterns != nullptr, "generated builder-pattern pack is valid");

    downwell::StartupRuntime runtime(
        downwell::PlatformProfile::nonDesktop, &pack, objects,
        downwell::StartupRuntime::defaultRandomSeed, patterns);
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
    require(runtime.randomSeed() == downwell::StartupRuntime::defaultRandomSeed &&
                runtime.randomDrawCount() == 92,
            "menu creation preserves all recovered shared-stream RNG draws");
    if (objects != nullptr)
        require(runtime.implementedCodeCalls() == 29,
                "room transition and RNG-consuming Create events route by code ID");
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
        require(runtime.menuPlayerX() == 112.0F &&
                    runtime.menuPlayerY() == 512.0F &&
                    runtime.menuPlayerSpriteId() == 12,
                "the reference seed selects its recovered sleeping pose");
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
        require(runtime.mainBuilderInitialized() &&
                    runtime.mainBuilderY() == 704 &&
                    runtime.mainBuilderGenerationCount() == -30,
                "rmMain initializes the recovered first-boot builder prelude");
        require(runtime.mainBuilderRightRoom(0) >= 3 &&
                    runtime.mainBuilderRightRoom(0) <= 15 &&
                    runtime.mainBuilderRightRoom(1) >= 3 &&
                    runtime.mainBuilderRightRoom(1) <= 15 &&
                    std::abs(runtime.mainBuilderRightRoom(0) -
                             runtime.mainBuilderRightRoom(1)) > 2,
                "builder creation consumes the stream for valid side-room rows");
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
        require(runtime.mainCameraViewY() > 50.0F &&
                    runtime.mainCameraViewY() < 70.0F,
                "rmMain camera follows the fall with recovered smoothing and bias");
        for (int tick = 0; tick < 20; ++tick)
            runtime.step(fallRight, false);
        require(runtime.mainPlayerX() < 285.0F,
                "the recovered player mask cannot cross the shaft wall mask");
        downwell::Input fireMain;
        fireMain.fire = true;
        runtime.step(fireMain, true);
        require(runtime.mainAmmo() == 7 && runtime.mainShotsFired() == 1,
                "fire consumes one default machine-gun charge");
        require(runtime.mainPlayerSpeedY() == 0.0F &&
                    runtime.mainPlayerSpriteId() == 3,
                "default recoil arrests the fall and selects the shoot sprite");
        require(runtime.mainBulletCount() == 1,
                "the first gunboot shot creates a visible bullet");
        const float firstBulletY = runtime.mainBulletAt(0).y;
        for (int tick = 0; tick < 6; ++tick)
            runtime.step(fireMain, false);
        require(runtime.mainShotsFired() == 1,
                "held fire waits for the recovered seven-tick rate");
        require(runtime.mainBulletCount() == 1 &&
                    runtime.mainBulletAt(0).y > firstBulletY,
                "the machine-gun bullet travels downward each tick");
        runtime.step(fireMain, false);
        require(runtime.mainAmmo() == 6 && runtime.mainShotsFired() == 2,
                "held fire repeats when Alarm 2 clears the shot delay");

        downwell::Input releasedMain;
        const std::uint32_t rowsBefore = runtime.mainBuilderRowsGenerated();
        const int builderYBefore = runtime.mainBuilderY();
        const std::size_t instancesBefore = runtime.activeInstanceCount();
        for (int tick = 0;
             tick < 120 && runtime.mainBuilderRowsGenerated() == rowsBefore;
             ++tick)
            runtime.step(releasedMain, false);
        require(runtime.mainBuilderRowsGenerated() == rowsBefore + 1 &&
                    runtime.mainBuilderY() == builderYBefore + 16 &&
                    runtime.mainBuilderGenerationCount() ==
                        -30 + static_cast<int>(rowsBefore) + 1,
                "the first-boot builder advances one shared pattern row");
        require(runtime.activeInstanceCount() == instancesBefore + 2,
                "one builder row creates exactly two shaft-wall instances");
        bool foundLeftWall = false;
        bool foundRightWall = false;
        for (std::size_t index = 0; index < runtime.activeInstanceCount(); ++index) {
            const downwell::InstanceRecord& instance = runtime.instanceAt(index);
            if (instance.objectId != 60 || instance.y != builderYBefore)
                continue;
            foundLeftWall = foundLeftWall || instance.x == 160;
            foundRightWall = foundRightWall || instance.x == 320;
        }
        require(foundLeftWall && foundRightWall,
                "the generated row preserves the recovered shaft boundaries");
        for (int tick = 0;
             tick < 300 && runtime.mainBuilderRowsGenerated() < 29;
             ++tick)
            runtime.step(releasedMain, false);
        require(runtime.mainBuilderRowsGenerated() == 29 &&
                    runtime.mainBuilderY() == 1168 &&
                    runtime.mainBuilderGenerationCount() == -1,
                "the deterministic first-boot prelude emits all 29 shared rows");
        require(runtime.activeInstanceCount() == instancesBefore + 58,
                "the complete prelude retains its 58 generated boundary walls");
        for (int tick = 0;
             tick < 120 && runtime.mainBuilderStartPattern() < 0;
             ++tick)
            runtime.step(releasedMain, false);
        require(runtime.mainBuilderStartPattern() == 3 &&
                    runtime.mainBuilderStartReversed() &&
                    runtime.mainBuilderGenerationCount() == 0,
                "the reference stream selects and mirrors entrance fragment three");
        require(runtime.mainBuilderStartRowsGenerated() == 5 &&
                    runtime.mainBuilderRowsGenerated() == 34,
                "the complete fragment advances its five shared row intervals");
        require(runtime.activeInstanceCount() == instancesBefore + 78,
                "the fragment creates its exact walls and breakable blocks");
    }
}

} // namespace

int main(int argc, char** argv) {
    testDesktopBootState();
    testNonDesktopSkipsSplash();
    testRecoveredDesktopTiming();
    testAdvanceInputUsesPressEdges();
    if (argc >= 2)
        testGeneratedPackEntersRecoveredMenu(
            argv[1], argc >= 3 ? argv[2] : nullptr,
            argc >= 4 ? argv[3] : nullptr);
    std::cout << "startup runtime tests passed\n";
    return 0;
}
