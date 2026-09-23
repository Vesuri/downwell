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
    normal.skipIntroForTesting();
    downwell::Input input;
    for (int i = 0; i < 120; ++i) normal.step(input, false);
    input.fire = true;
    normal.step(input, true);
    input.fire = false;
    for (int i = 0; i < 300 && !normal.inMainRoom(); ++i) normal.step(input, false);
    assert(normal.inMainRoom());
    assert(normal.mainPlayerX() == host.startup().mainPlayerX());
    assert(normal.mainPlayerY() == host.startup().mainPlayerY());
    assert(normal.mainPlayerHealth() == host.startup().mainPlayerHealth());
    assert(normal.mainCameraViewY() == host.startup().mainCameraViewY());
    assert(normal.mainBuilderGenerationCount() == host.startup().mainBuilderGenerationCount());
    for (int i = 0; i < 120; ++i) { host.update(input); amiga.step(input); }
    assert(host.startup().inMainRoom() && amiga.scene() == AmigaScene::gameplay);
    std::cout << "Direct gameplay startup, normal initialization and toggle precedence passed\n";
}
