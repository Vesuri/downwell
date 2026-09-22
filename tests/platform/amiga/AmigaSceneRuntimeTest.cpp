#include "platform/amiga/AmigaSceneRuntime.h"
#include <cstdlib>
#include <iostream>

namespace {
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
}

int main() {
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
    std::cout << "Amiga scene runtime tests passed\n";
    return 0;
}
