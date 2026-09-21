#include "Game.h"
namespace downwell {

Game::Game(const RoomPack* roomPack, const ObjectPack* objectPack)
    : startup_(PlatformProfile::desktop, roomPack, objectPack) {}

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
        // Camera-follow behavior is not translated yet, so use the diagnostic
        // view center while drawing the player's recovered idle frame.
        if (startup_.instance(104218) != nullptr)
            renderer.drawSprite(31, 0, logicalWidth / 2, logicalHeight / 2);
    } else {
        const int stage = startup_.showSplash();
        if (stage == 2)
            renderer.drawSprite(696, 0, 80, 140); // sprDevolverLogo
        if (stage == 4 || stage == 5)
            renderer.drawSprite(695, 0, 80, 140); // sprControls
        if (stage == 5 || stage == 6)
            renderer.drawSpriteTiled(733, 11, -300, 0); // sprDitherFade
    }
}

} // namespace downwell
