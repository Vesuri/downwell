#include "Game.h"
#include <algorithm>

namespace downwell {

void Game::update(const Input& input) {
    finished_ = finished_ || input.quit;
    markerX_ += input.right ? 2 : 0;
    markerX_ -= input.left ? 2 : 0;
    markerX_ = std::clamp(markerX_, 8, logicalWidth - 8);
    fire_ = input.fire;
    ++tick_;
}

void Game::render(Renderer& renderer) const {
    // Asset-free smoke image using Downwell's characteristic three-color
    // presentation. This is not recovered game behavior.
    renderer.clear({10, 10, 12, 255});
    renderer.fillRect(0, logicalHeight - 48, logicalWidth, 48,
                      {238, 238, 226, 255});
    renderer.fillRect(markerX_ - 6, logicalHeight - 72, 12, 24,
                      fire_ ? Color{224, 44, 48, 255}
                            : Color{238, 238, 226, 255});

    const int pulse = static_cast<int>((tick_ / 8) % 24);
    renderer.fillRect(16 + pulse, 24, 72, 4, {224, 44, 48, 255});
}

} // namespace downwell

