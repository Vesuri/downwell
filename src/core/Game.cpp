#include "Game.h"
namespace downwell {

void Game::update(const Input& input) {
    finished_ = finished_ || input.quit;
    startup_.step(input.fire && !fire_);
    fire_ = input.fire;
    ++tick_;
}

void Game::render(Renderer& renderer) const {
    // Asset-free diagnostic presentation of the recovered startup state. The
    // timing and room transition are modeled; original splash/menu art is not.
    renderer.clear({10, 10, 12, 255});
    if (startup_.inMenu()) {
        renderer.fillRect(0, 0, logicalWidth, logicalHeight,
                          {238, 238, 226, 255});
        renderer.fillRect(16, 24, logicalWidth - 32, 8,
                          {224, 44, 48, 255});
    } else {
        const int stage = startup_.showSplash();
        const int width = stage * 16;
        renderer.fillRect((logicalWidth - width) / 2,
                          logicalHeight / 2 - 4,
                          width, 8, {238, 238, 226, 255});
    }
}

} // namespace downwell
