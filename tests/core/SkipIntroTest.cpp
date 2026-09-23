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
    assert(host.startup().inPlayMenu() && host.startup().showSplash() == 0);
    assert(host.startup().controllerPersistent());
    assert(amiga.scene() == AmigaScene::styleSelect && amiga.splashStage() == 0);
    downwell::Input input;
    for (int i = 0; i < 120; ++i) { host.update(input); amiga.step(input); }
    assert(host.startup().inPlayMenu() && amiga.scene() == AmigaScene::styleSelect);
    input.fire = true;
    host.update(input); amiga.step(input);
    input.fire = false;
    for (int i = 0; i < 300; ++i) {
        if (!host.startup().inMainRoom()) host.update(input);
        if (amiga.scene() != AmigaScene::gameplay) amiga.step(input);
    }
    assert(host.startup().inMainRoom() && amiga.scene() == AmigaScene::gameplay);
    std::cout << "Skip-intro startup and style confirmation passed on both runtimes\n";
}
