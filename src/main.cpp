#include "core/Game.h"
#include "core/RoomPack.h"
#include "platform/sdl/PlatformSDL.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

int main() {
    downwell::PlatformSDL platform;
    if (!platform.ready())
        return 1;

    std::ifstream roomFile("generated/downwell-rooms.dwrm", std::ios::binary);
    const std::vector<std::uint8_t> roomBytes(
        (std::istreambuf_iterator<char>(roomFile)),
        std::istreambuf_iterator<char>());
    const downwell::RoomPack roomPack(roomBytes.data(), roomBytes.size());
    if (!roomPack.valid())
        std::fprintf(stderr, "warning: native room pack unavailable; using bounded startup fixture\n");

    downwell::Game game(roomPack.valid() ? &roomPack : nullptr);
    while (!game.finished()) {
        game.update(platform.pollInput());
        game.render(platform);
        platform.present();
        platform.waitForNextTick();
    }
    return 0;
}
