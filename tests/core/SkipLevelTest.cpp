#include "core/Game.h"
#include "core/RoomPack.h"
#include "core/ObjectPack.h"
#include "core/BuilderPatternPack.h"
#include "platform/amiga/AmigaSceneRuntime.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

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
    downwell::Game game(&rooms, &objects, &builders);
    AmigaSceneRuntime scenes;
    const auto& state = game.startup();
    assert(state.inMainRoom() && state.showSplash() == 0 && state.playStyle() == 0);
    assert(scenes.scene() == AmigaScene::gameplay);
    assert(state.runStage().level == 1 && state.levelExitPresent());
    assert(!state.levelClearActive());
    bool sawWipe = false;
    unsigned tick = 0;
    for (; tick < 120 && !state.levelClearActive(); ++tick) {
        game.update({});
        sawWipe |= state.exitDither().active;
    }
    assert(tick == 96 && sawWipe && state.levelClearActive());
    for (unsigned i = 0; i < 60; ++i) game.update({});
    assert(state.levelClear().selection.visible);
    downwell::Input right; right.right = true;
    game.update(right);
    assert(state.levelClear().selection.cursor == 2);
    downwell::Input confirm; confirm.fire = true;
    for (unsigned i = 0; i < 240 && state.runStage().level == 1; ++i) game.update(confirm);
    assert(state.runStage().level == 2 && state.inMainRoom() && !state.levelClearActive());
    for (unsigned i = 0; i < 180; ++i) game.update({});
    assert(state.runStage().level == 2 && !state.levelClearActive() && !state.levelExitPresent());
    assert(state.roomLoadSucceeded() && state.deferredCommandsSucceeded());
    std::cout << "SKIP_LEVEL: exit wipe, completion, selection and unskipped next level passed\n";
}
