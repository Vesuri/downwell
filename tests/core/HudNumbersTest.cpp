#include "core/HudNumbers.h"
#include <array>
#include <cstdio>
#include <cstdlib>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

struct SpriteCall {
    std::int32_t sprite;
    std::uint32_t frame;
    int x;
    int y;
};

class RecordingRenderer final : public downwell::Renderer {
public:
    void clear(downwell::Color) override {}
    void setClipRect(int, int, int, int) override {}
    void clearClipRect() override {}
    void fillRect(int, int, int, int, downwell::Color) override {}
    void drawSpriteTiled(std::int32_t, std::uint32_t, int, int) override {}
    void drawSprite(std::int32_t sprite, std::uint32_t frame,
                    int x, int y) override {
        calls[count++] = {sprite, frame, x, y};
    }
    void drawSpriteTransformed(std::int32_t, std::uint32_t, float, float,
                               float, float, float) override {}
    void drawText(std::int32_t, const char*, float, float, downwell::Color,
                  downwell::TextAlign) override {}

    std::array<SpriteCall, 64> calls{};
    std::size_t count = 0;
};

} // namespace

int main() {
    RecordingRenderer renderer;

    downwell::drawHudHealthNumber(renderer, 100, 20, 4, 4);
    require(renderer.count == 12, "health uses recovered digit and slash layers");
    require(renderer.calls[3].sprite == 413 && renderer.calls[3].frame == 4 &&
                renderer.calls[3].x == 87 && renderer.calls[3].y == 15,
            "health digit uses the recovered centred baseline");
    require(renderer.calls[7].sprite == 414 && renderer.calls[7].frame == 0 &&
                renderer.calls[7].x == 96 && renderer.calls[7].y == 15,
            "health slash uses the recovered centred baseline");

    renderer.count = 0;
    downwell::drawHudCurrencyNumber(renderer, 100, 20, 2);
    require(renderer.count == 5 && renderer.calls[4].frame == 2 &&
                renderer.calls[4].x == 92 && renderer.calls[4].y == 15,
            "currency digit sits five pixels above its supplied anchor");

    renderer.count = 0;
    downwell::drawHudAmmoNumber(renderer, 100, 20, 8);
    require(renderer.count == 15 && renderer.calls[14].frame == 8 &&
                renderer.calls[14].x == 100 && renderer.calls[14].y == 20,
            "single-digit ammo is centred on its recovered anchor");

    std::puts("HUD number tests passed");
    return 0;
}
