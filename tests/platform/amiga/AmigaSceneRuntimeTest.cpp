#include "platform/amiga/AmigaSceneRuntime.h"
#include "core/BuilderPatternPack.h"
#include "core/ObjectPack.h"
#include "core/RoomPack.h"
#include "core/StartupRuntime.h"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {
int parityFrame = 0;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void pressFire(AmigaSceneRuntime& runtime) {
    downwell::Input fire;
    fire.fire = true;
    runtime.step(fire);
    runtime.step(downwell::Input{});
}

std::vector<std::uint8_t> readFile(const char* path) {
    std::ifstream input(path, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input),
                                     std::istreambuf_iterator<char>());
}

void requireNear(float actual, float expected, float tolerance,
                 const char* message) {
    if (std::fabs(actual - expected) > tolerance) {
        std::cerr << "FAILED: " << message << " (Amiga " << actual
                  << ", reference " << expected << ", parity frame "
                  << parityFrame << ")\n";
        std::exit(1);
    }
}

void compareMenu(const AmigaSceneRuntime& amiga,
                 const downwell::StartupRuntime& reference) {
    require(reference.inMenu(), "reference remains in menu during comparison");
    require(amiga.scene() == AmigaScene::menu,
            "Amiga remains in menu during comparison");
    requireNear(static_cast<float>(amiga.menuPlayerX()),
                reference.menuPlayerX(), 1.0F,
                "menu player X follows decoded runtime");
    requireNear(static_cast<float>(amiga.menuPlayerY()),
                reference.menuPlayerY(), 1.0F,
                "menu player Y follows decoded runtime");
    requireNear(static_cast<float>(amiga.menuCameraX()),
                reference.menuCameraViewX() + 80.0F, 4.0F,
                "menu camera X follows decoded runtime");
    requireNear(static_cast<float>(amiga.menuCameraY()),
                reference.menuCameraViewY() + 142.0F, 4.0F,
                "menu camera Y follows decoded runtime");
    require(amiga.menuPlayerNapping() == reference.menuPlayerNapping(),
            "menu sleeping state follows decoded runtime");
    require(amiga.menuPlayerVisible() == reference.menuPlayerVisible(),
            "menu visibility follows decoded runtime");
    require(amiga.menuTeleportActive() == reference.menuTeleportActive(),
            "menu teleport follows decoded runtime");
    require(amiga.menuEnterSequenceActive() == reference.enterSequenceActive(),
            "well-entry sequence follows decoded runtime");
    if (amiga.menuEnterSequenceActive())
        requireNear(static_cast<float>(amiga.menuEnterSequenceY()),
                    reference.enterSequenceY(), 1.0F,
                    "well-entry vertical motion follows decoded runtime");
}

void compareStyle(const AmigaSceneRuntime& amiga,
                  const downwell::StartupRuntime& reference) {
    require(reference.inPlayMenu(),
            "reference remains in style select during comparison");
    require(amiga.scene() == AmigaScene::styleSelect,
            "Amiga remains in style select during comparison");
    requireNear(static_cast<float>(amiga.styleYOffset()),
                reference.playMenuYOffset(), 1.0F,
                "style entrance easing follows decoded runtime");
    requireNear(static_cast<float>(amiga.styleTileY()),
                reference.playMenuTileY(), 1.0F,
                "style tile scroll follows decoded runtime");
    requireNear(static_cast<float>(amiga.styleDitherY()),
                reference.playMenuDitherY(), 1.0F,
                "style dither scroll follows decoded runtime");
    requireNear(static_cast<float>(amiga.styleSmoothX()),
                reference.playMenuSmoothX(), 1.0F,
                "style carousel easing follows decoded runtime");
    require(amiga.styleCursor() == reference.playMenuCursor(),
            "style cursor follows decoded runtime");
    require(amiga.styleSelected() == reference.playMenuSelected(),
            "style selection follows decoded runtime");
    requireNear(static_cast<float>(amiga.stylePlayerDescent()),
                reference.playMenuPlayerDescent(), 1.0F,
                "style player descent follows decoded runtime");
    requireNear(static_cast<float>(amiga.styleVerticalDitherY()),
                reference.playMenuVerticalDitherY(), 1.0F,
                "style exit dither follows decoded runtime");
}

void testDecodedSceneParity(const char* roomPath, const char* objectPath,
                            const char* builderPatternPath) {
    const std::vector<std::uint8_t> roomBytes = readFile(roomPath);
    const std::vector<std::uint8_t> objectBytes = readFile(objectPath);
    const std::vector<std::uint8_t> patternBytes = readFile(builderPatternPath);
    const downwell::RoomPack rooms(roomBytes.data(), roomBytes.size());
    const downwell::ObjectPack objects(objectBytes.data(), objectBytes.size());
    const downwell::BuilderPatternPack patterns(patternBytes.data(),
                                                patternBytes.size());
    require(rooms.valid() && objects.valid() && patterns.valid(),
            "generated recovery packs are valid for scene parity");

    downwell::StartupRuntime reference(
        downwell::PlatformProfile::nonDesktop, &rooms, &objects,
        downwell::StartupRuntime::defaultRandomSeed, &patterns);
    reference.step(false);
    require(reference.inMenu(), "decoded runtime enters menu");

    AmigaSceneRuntime amiga;
    downwell::Input fire;
    fire.fire = true;
    for (int stage = 0; stage < 5; ++stage) {
        amiga.step(fire);
        amiga.step(downwell::Input{});
    }
    amiga.step(fire);
    require(amiga.scene() == AmigaScene::menu,
            "Amiga runtime enters menu on the sixth press");

    downwell::Input released;
    for (int tick = 0; tick < 30; ++tick) {
        parityFrame = tick;
        amiga.step(released);
        reference.step(released, false);
        compareMenu(amiga, reference);
    }

    downwell::Input right;
    right.right = true;
    for (int tick = 0; tick < 500 && reference.inMenu(); ++tick) {
        parityFrame = 30 + tick;
        amiga.step(right);
        reference.step(right, false);
        if (!reference.inMenu() || amiga.scene() != AmigaScene::menu)
            break;
        compareMenu(amiga, reference);
    }
    if (reference.inMenu() || amiga.scene() != AmigaScene::styleSelect) {
        std::cerr << "FAILED: both runtimes reach style select on the same input "
                  << "tape (parity frame " << parityFrame
                  << ", reference menu " << reference.inMenu()
                  << ", Amiga scene " << static_cast<int>(amiga.scene())
                  << ")\n";
        std::exit(1);
    }

    for (int tick = 0; tick < 100; ++tick) {
        parityFrame = 366 + tick;
        amiga.step(released);
        reference.step(released, false);
        compareStyle(amiga, reference);
    }

    amiga.step(right);
    reference.step(right, false);
    compareStyle(amiga, reference);
    require(amiga.styleCursor() == 1,
            "both runtimes move to the locked second style");

    amiga.step(fire);
    reference.step(fire, true);
    compareStyle(amiga, reference);
    require(!amiga.styleSelected(),
            "both runtimes reject the locked style");

    downwell::Input left;
    left.left = true;
    amiga.step(left);
    reference.step(left, false);
    compareStyle(amiga, reference);
    require(amiga.styleCursor() == 0,
            "both runtimes return to the default style");

    amiga.step(released);
    reference.step(released, false);
    compareStyle(amiga, reference);
    amiga.step(fire);
    reference.step(fire, true);
    compareStyle(amiga, reference);
    require(amiga.styleSelected(),
            "both runtimes confirm the default style");

    for (int tick = 0; tick < 63; ++tick) {
        parityFrame = 470 + tick;
        amiga.step(released);
        reference.step(released, false);
        compareStyle(amiga, reference);
    }
    amiga.step(released);
    reference.step(released, false);
    require(amiga.scene() == AmigaScene::gameplay && reference.inMainRoom(),
            "both runtimes enter gameplay on the same frame");
}
}

int main(int argc, char** argv) {
    AmigaSceneRuntime runtime;
    require(runtime.scene() == AmigaScene::splash &&
            runtime.splashStage() == 1, "startup begins at splash stage one");
    for (int stage = 2; stage <= 6; ++stage) {
        pressFire(runtime);
        require(runtime.splashStage() == stage,
                "fire edge advances one splash stage");
    }
    pressFire(runtime);
    require(runtime.scene() == AmigaScene::menu && runtime.menuPlayerNapping(),
            "sixth advance enters the sleeping menu scene");

    for (int tick = 0; tick < 29; ++tick)
        runtime.step(downwell::Input{});
    downwell::Input right;
    right.right = true;
    runtime.step(right);
    require(!runtime.menuPlayerNapping(),
            "the first enabled direction input wakes the player");
    for (int tick = 0; tick < 10; ++tick)
        runtime.step(right);
    require(runtime.menuPlayerX() == 123,
            "menu acceleration reaches the recovered ten-tick position");
    for (int tick = 0; tick < 600 && runtime.scene() == AmigaScene::menu; ++tick)
        runtime.step(right);
    require(runtime.scene() == AmigaScene::styleSelect,
            "walking into the well reaches style select");
    require(runtime.menuTeleportActive() && !runtime.menuPlayerVisible() &&
            runtime.menuTitleFrame() == 9,
            "well transition hides the player and completes the title");

    downwell::Input released;
    for (int tick = 0; tick < 120 && runtime.styleYOffset() != 0; ++tick)
        runtime.step(released);
    require(runtime.styleYOffset() == 0, "style-select entrance settles");
    pressFire(runtime);
    require(runtime.styleSelected(), "default style can be selected");
    for (int tick = 0; tick < 100 && runtime.scene() != AmigaScene::gameplay; ++tick)
        runtime.step(released);
    require(runtime.scene() == AmigaScene::gameplay,
            "style descent enters gameplay");
    if (argc >= 4)
        testDecodedSceneParity(argv[1], argv[2], argv[3]);
    std::cout << "Amiga scene runtime tests passed\n";
    return 0;
}
