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

    std::array<SpriteCall, 160> calls{};
    std::size_t count = 0;
};

} // namespace

int main() {
    unsigned layer = 0;
    const int expectedX[] = {9, 10, 11, 10, 10};
    const int expectedY[] = {20, 19, 20, 21, 20};
    downwell::drawGemmyTextLayers(10, 20, [&](int x, int y, downwell::Color color) {
        require(layer < 5 && x == expectedX[layer] && y == expectedY[layer],
                "gemmy text offset passes follow original draw order");
        require(color.a == 255 && color.r == (layer == 2 || layer == 3 ? 0 : 255) &&
                color.g == (layer == 4 ? 255 : 0) && color.b == color.g,
                "gemmy text uses source red, black, white passes");
        ++layer;
    });
    require(layer == 5, "gemmy text emits all five source layers");
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
    downwell::drawHudHealthNumber(renderer, 100, 20, 12, 105);
    require(renderer.count == 24, "health emits every digit of both counters");
    const unsigned frames[] = {1, 2, 0, 1, 0, 5};
    const int positions[] = {79, 87, 96, 105, 113, 121};
    for (unsigned i = 0; i < 6; ++i) {
        const unsigned sprite = i == 2 ? 414 : 413;
        const unsigned shadow = i == 2 ? 2 : frames[i] + 20;
        const int dx[] = {1, 1, 0, 0};
        const int dy[] = {0, 1, 1, 0};
        for (unsigned pass = 0; pass < 4; ++pass) {
            const auto& call = renderer.calls[i * 4 + pass];
            require(call.sprite == static_cast<int>(sprite) &&
                    call.frame == (pass == 3 ? frames[i] : shadow) &&
                    call.x == positions[i] + dx[pass] && call.y == 15 + dy[pass],
                    "multi-digit health follows VM glyph positions and shadow order");
        }
    }
    renderer.count = 0;
    downwell::drawHealthNumberLayers(100, 20, 0xffffffffu, 0xffffffffu,
        [&](unsigned sprite, unsigned frame, int x, int y) {
            renderer.drawSprite(sprite, frame, x, y);
        });
    require(renderer.count == 84, "health layout supports full unsigned counters");
    renderer.count = 0;
    downwell::drawHudHealthNumber(renderer, 100, 20, -1, 0);
    require(renderer.count == 12 && renderer.calls[3].frame == 0 &&
            renderer.calls[11].frame == 0, "zero health remains a visible zero");

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

    renderer.count = 0;
    downwell::drawHudAmmoNumber(renderer, 100, 20, 12);
    require(renderer.count == 30 && renderer.calls[28].frame == 1 &&
                renderer.calls[28].x == 96 && renderer.calls[28].y == 20 &&
                renderer.calls[29].frame == 2 && renderer.calls[29].x == 104 &&
                renderer.calls[29].y == 21, "multi-digit ammo is centered with source diagonal spacing");
    for (unsigned i = 0; i < 28; ++i)
        require(renderer.calls[i].frame >= 10, "all ammo outlines precede foreground digits");
    unsigned calls = 0;
    downwell::drawAmmoNumberLayers(100, 20, 0xffffffffu, [&](unsigned, int, int) { ++calls; });
    require(calls == 150, "shared layout handles the entire unsigned counter range");
    std::puts("HUD number tests passed");
    return 0;
}
