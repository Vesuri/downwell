#include "core/Game.h"
#include "core/RoomPack.h"
#include "core/ObjectPack.h"
#include "core/BuilderPatternPack.h"
#include "platform/amiga/AmigaSceneRuntime.h"
#include <fstream>
#include <iterator>
#include <vector>
#include <cassert>
#include <iostream>

static std::vector<std::uint8_t> read(const char* path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}

int main() {
    const auto r = read("generated/downwell-rooms.dwrm");
    const auto o = read("generated/downwell-objects.dwob");
    const auto b = read("generated/downwell-builder-patterns.dwbp");
    downwell::RoomPack rooms(r.data(), r.size());
    downwell::ObjectPack objects(o.data(), o.size());
    downwell::BuilderPatternPack builders(b.data(), b.size());
    assert(rooms.valid() && objects.valid() && builders.valid());
    downwell::Game host(&rooms, &objects, &builders);
    AmigaSceneRuntime amiga;
    assert(host.startup().inMainRoom() && host.startup().showSplash() == 0);
    assert(host.startup().controllerPersistent() && host.startup().playStyle() == 0);
    assert(amiga.scene() == AmigaScene::gameplay && amiga.splashStage() == 0);

    downwell::StartupRuntime normal(downwell::PlatformProfile::desktop, &rooms,
        &objects, downwell::StartupRuntime::defaultRandomSeed, &builders);
    downwell::Input input;
    for (int i = 0; i < 4096 && !normal.inMainRoom(); ++i) {
        input = {};
        if (normal.inMenu()) input.right = true;
        if (normal.inPlayMenu() && normal.playMenuYOffset() == 0 && !normal.playMenuSelected())
            input.fire = true;
        normal.step(input, input.fire);
    }
    assert(normal.inMainRoom());
    std::cout << "Entry RNG draws: normal=" << normal.randomDrawCount()
              << " shortcut=" << host.startup().randomDrawCount() << std::endl;
    assert(normal.randomDrawCount() == host.startup().randomDrawCount());
    assert(normal.mainPlayerX() == host.startup().mainPlayerX());
    assert(normal.mainPlayerY() == host.startup().mainPlayerY());
    assert(normal.mainPlayerHealth() == host.startup().mainPlayerHealth());
    assert(normal.mainCameraViewY() == host.startup().mainCameraViewY());
    assert(normal.mainBuilderGenerationCount() == host.startup().mainBuilderGenerationCount());
    input = {};
    for (int i = 0; i < 180; ++i) {
        host.update(input); normal.step(input, false); amiga.step(input);
        assert(normal.randomDrawCount() == host.startup().randomDrawCount());
        assert(normal.mainPlayerX() == host.startup().mainPlayerX());
        assert(normal.mainPlayerY() == host.startup().mainPlayerY());
        assert(normal.mainCameraViewY() == host.startup().mainCameraViewY());
        assert(normal.mainBuilderGenerationCount() == host.startup().mainBuilderGenerationCount());
    }
    assert(host.startup().inMainRoom() && amiga.scene() == AmigaScene::gameplay);
    std::cout << "Direct gameplay startup, normal initialization and toggle precedence passed\n";
}
