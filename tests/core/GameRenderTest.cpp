#include "core/Game.h"
#include <cstdio>
#include <cstdlib>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

class RecordingRenderer final : public downwell::Renderer {
public:
    void clear(downwell::Color color) override {
        clearColor = color;
        ++clearCount;
    }
    void setClipRect(int x, int y, int width, int height) override {
        clipX = x;
        clipY = y;
        clipWidth = width;
        clipHeight = height;
        ++setClipCount;
    }
    void clearClipRect() override { ++clearClipCount; }
    void fillRect(int, int, int, int, downwell::Color) override {}
    void drawSpriteTiled(std::int32_t, std::uint32_t, int, int) override {}
    void drawSprite(std::int32_t, std::uint32_t, int, int) override {}
    void drawSpriteTransformed(std::int32_t, std::uint32_t, float, float,
                               float, float, float) override {}
    void drawText(std::int32_t, const char*, float, float, downwell::Color,
                  downwell::TextAlign) override {}

    downwell::Color clearColor{};
    int clearCount = 0;
    int setClipCount = 0;
    int clearClipCount = 0;
    int clipX = 0;
    int clipY = 0;
    int clipWidth = 0;
    int clipHeight = 0;
};

} // namespace

int main() {
    downwell::Game game;
    RecordingRenderer renderer;
    game.render(renderer);

    require(renderer.clearCount == 1 && renderer.clearColor.r == 0 &&
                renderer.clearColor.g == 0 && renderer.clearColor.b == 0 &&
                renderer.clearColor.a == 255,
            "the full 4:3 canvas is cleared to opaque black");
    require(renderer.setClipCount == 1 &&
                renderer.clipX == downwell::Game::playfieldX &&
                renderer.clipY == 0 &&
                renderer.clipWidth == downwell::StartupRuntime::surfaceWidth &&
                renderer.clipHeight == downwell::Game::logicalHeight,
            "game rendering is clipped to the centred 160x284 well");
    require(renderer.clearClipCount == 1,
            "the game clip is released before the HUD layer");

    std::puts("game render tests passed");
    return 0;
}
