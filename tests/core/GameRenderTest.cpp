#include "core/Game.h"
#include <cstring>
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
    void drawText(std::int32_t, const char* text, float x, float,
                  downwell::Color color,
                  downwell::TextAlign align) override {
        if (color.r == 255 &&
            ((color.g == 255 && color.b == 255) ||
             (color.g == 0 && color.b == 0)) &&
            align == downwell::TextAlign::center) {
            ++creditForegroundCount;
            if (std::strcmp(text, "JOONAS TURNER") == 0 && x == 102.0F)
                sawLeftCredit = true;
            if (std::strcmp(text, "EIRIK SUHRKE") == 0 && x == 278.0F)
                sawRightCredit = true;
        }
    }

    downwell::Color clearColor{};
    int clearCount = 0;
    int setClipCount = 0;
    int clearClipCount = 0;
    int clipX = 0;
    int clipY = 0;
    int clipWidth = 0;
    int clipHeight = 0;
    int creditForegroundCount = 0;
    bool sawLeftCredit = false;
    bool sawRightCredit = false;
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

    downwell::Input input;
    input.fire = true;
    game.update(input);
    input.fire = false;
    game.update(input);
    input.fire = true;
    game.update(input);
    require(game.startup().showSplash() == 3,
            "two fire edges advance the reference to credits");
    renderer.clearClipCount = 0;
    game.render(renderer);
    require(renderer.clearClipCount == 1 &&
                renderer.creditForegroundCount == 6 &&
                renderer.sawLeftCredit && renderer.sawRightCredit,
            "credits release the playfield clip and use both 4:3 side areas");

    std::puts("game render tests passed");
    return 0;
}
