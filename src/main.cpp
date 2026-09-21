#include "core/Game.h"
#include "platform/sdl/PlatformSDL.h"

int main() {
    downwell::PlatformSDL platform;
    if (!platform.ready())
        return 1;

    downwell::Game game;
    while (!game.finished()) {
        game.update(platform.pollInput());
        game.render(platform);
        platform.present();
        platform.waitForNextTick();
    }
    return 0;
}

