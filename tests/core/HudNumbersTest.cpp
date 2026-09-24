#include "core/HudNumbers.h"
#include "core/AmmoHudMotion.h"
#include "core/AmmoGaugeRows.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>

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
    for (unsigned value : {0u, 9u, 10u, 99u, 100u, 1000u, 4294967295u}) {
        char text[11];
        std::snprintf(text, sizeof text, "%u", value);
        const unsigned digits = std::strlen(text);
        unsigned calls = 0;
        const int dx[] = {-1,0,1,0,0}, dy[] = {0,-1,0,1,0};
        downwell::drawGemmyNumberLayers(191, 51, value,
            [&](unsigned digit, int x, int y, downwell::Color color) {
                const unsigned layer = calls / digits, column = calls % digits;
                require(digit == unsigned(text[column] - '0') &&
                    x == 191 + dx[layer] - int(digits - 1) * 4 + int(column) * 8 &&
                    y == 51 + dy[layer], "decimal streak centers every width with whole-string layer order");
                require(color.r == (layer == 2 || layer == 3 ? 0 : 255) &&
                    color.g == (layer == 4 ? 255 : 0), "streak retains source shadow and foreground colours");
                require(x - 4 >= 148 && x + 4 <= 236, "all uint32 digits fit the native dirty region");
                ++calls;
            });
        require(calls == digits * 5, "all streak digits appear in all five source layers");
    }
    for (unsigned cap = 1; cap <= 512; ++cap)
        for (unsigned ammo = 0; ammo <= cap; ++ammo)
            require(downwell::ammoGaugeFilledRows(ammo, cap) ==
                    static_cast<unsigned>(std::ceil(184.0 * (static_cast<double>(ammo) / cap))),
                    "integer row threshold preserves VM binary64 boundary behavior");
    std::uint32_t random = 2;
    for (unsigned i = 0; i < 100000; ++i) {
        random = random * 1664525u + 1013904223u;
        const auto cap = random | 1u;
        random = random * 1664525u + 1013904223u;
        const auto ammo = random % cap;
        require(downwell::ammoGaugeFilledRows(ammo, cap) ==
                static_cast<unsigned>(std::ceil(184.0 * (static_cast<double>(ammo) / cap))),
                "row threshold handles full-width capacities without overflow");
    }
    downwell::AmmoGaugeRows rows;
    double original[184];
    for (auto& row : original) row = 3;
    for (unsigned tick = 0; tick < 1000; ++tick) {
        const unsigned ammo = (tick / 3) % 9;
        rows.step(ammo, 8);
        for (unsigned i = 0; i < 184; ++i) {
            if (i < 184.0 * (ammo / 8.0)) { if (original[i] <= 3) original[i] += .8; }
            else if (original[i] >= 1) original[i] -= .8;
        }
        unsigned count = 0;
        rows.draw(288, 229, [&](unsigned sprite, unsigned frame, int x, int y) {
            require(sprite == (count < 23 * ammo ? (count ? 419u : 421u) : 418u) &&
                    frame == static_cast<unsigned>(original[count]) && x == 288 &&
                    y == 229 - static_cast<int>(count), "row draws match original fractional animation");
            ++count;
        });
        require(count == 184, "all source gauge rows are emitted");
    }
    for (unsigned i = 0; i < 5; ++i) rows.step(8, 8);
    require(!rows.step(8, 8), "settled row animation stops requesting redraws");
    unsigned visibleRows = 0;
    rows.draw(288, 241, [&](unsigned, unsigned, int, int y) {
        require(y < 232, "recharge motion retains the source bottom clipping condition");
        ++visibleRows;
    });
    require(visibleRows == 174, "three-pixel meter offset shifts rows by twelve pixels");
    downwell::AmmoHudMotion ammoMotion;
    require(!ammoMotion.step(0) && ammoMotion.offset == 0, "settled ammo label stays clean");
    require(ammoMotion.step(1) && ammoMotion.offset == 2, "shot immediately offsets ammo label");
    require(ammoMotion.step(1) && ammoMotion.offset == 1, "ammo label returns one pixel per update");
    require(ammoMotion.step(2) && ammoMotion.offset == 2, "new shot overrides decay");
    require(!ammoMotion.step(3) && ammoMotion.offset == 2, "consecutive shots retain the two pixel offset");
    require(ammoMotion.step(3) && ammoMotion.offset == 1, "held input without a shot decays");
    require(ammoMotion.step(3) && ammoMotion.offset == 0 && !ammoMotion.step(3),
            "ammo motion stops requesting redraws after settling");
    require(ammoMotion.step(3, 1) && ammoMotion.meterOffset == 3,
            "recharge sets four then source Draw decays to three");
    for (unsigned expected : {2u, 1u, 0u})
        require(ammoMotion.step(3, 1) && ammoMotion.meterOffset == expected,
                "recharge gauge returns to rest one step at a time");
    require(!ammoMotion.step(3, 1), "settled recharge does not request a redraw");
    for (unsigned pieces = 0; pieces < 4; ++pieces) {
        unsigned phase = 0;
        downwell::drawHeartPieceMeter(8, 6, pieces, 4,
            [&](int x, int y, int w, int h) {
                require(phase++ == 0 && x == 12 && y == 20 &&
                        w == static_cast<int>(pieces * 20) && h == 2,
                        "heart-piece fill follows source gauge dimensions");
            },
            [&](unsigned sprite, unsigned frame, int x, int y) {
                require(sprite == 378 && frame == 2 && y == 18 &&
                        x == 10 + static_cast<int>(phase) * 20,
                        "heart-piece dividers follow fill in source order");
                ++phase;
            });
        require(phase == 4, "four-piece reserve has three dividers");
    }
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
