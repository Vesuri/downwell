#include "core/LevelClearPresentation.h"
#include "core/LevelClearLayout.h"
#include "core/LevelClearTrig.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
int main() {
    using namespace downwell;
    unsigned curveCases = 0;
    const auto checkCurve = [&](HudReal displacement, HudReal sine) {
        const auto actual = levelClearOptionHeight(displacement, sine);
        const auto expected = levelClearOptionHeightReference(displacement, sine);
        require(actual.equal(expected), "clamp admission must preserve exact source coordinates");
        ++curveCases;
    };
    // Both sides of the 12-pixel boundary, including adjacent binary64
    // values, zero/max bob and arbitrary samples outside the sine domain.
    for (int delta = -64; delta <= 64; ++delta) {
        auto displacement = HudReal::integer(12);
        displacement.mantissa += delta;
        for (unsigned tick = 0; tick < 2048; ++tick)
            checkCurve(displacement, LevelClearTrig::absoluteSine(tick));
        checkCurve(displacement, {});
        checkCurve(displacement, HudReal::integer(1));
        auto sine = HudReal::integer(1); ++sine.mantissa;
        checkCurve(displacement, sine);
        checkCurve(displacement, HudReal::integer(2));
    }
    // Real easing, interrupted/reversed cursor movement, every offered slot.
    for (unsigned options = 1; options <= 4; ++options) {
        LevelClearCarousel carousel; carousel.create(0);
        for (unsigned tick = 0; tick < 12000; ++tick) {
            carousel.step((tick / 17 + tick / 43) % options);
            const auto sine = LevelClearTrig::absoluteSine(tick);
            for (unsigned i = 0; i < options; ++i)
                checkCurve(carousel.optionX(i).add(LevelClearCoordinate::integer(-80)).magnitude, sine);
        }
    }
    for (unsigned i = 0; i <= 512; ++i) {
        const auto displacement = HudReal::ratio(i, 4);
        for (unsigned bob = 0; bob <= 64; ++bob)
            checkCurve(displacement, HudReal::ratio(bob, 32));
    }
    std::printf("%u clamped option coordinates match the source arithmetic exactly\n", curveCases);
    for (int area = 1; area <= 4; ++area) {
        for (int level = 1; level <= 3; ++level) {
            LevelClearPresentation state;
            int tileY = 0, border = 36;
            unsigned circle = 0, time = 0;
            for (unsigned tick = 0; tick < 500; ++tick) {
                const int next = tick < 60 ? 0 : 4;
                const bool choices = tick >= 60 && tick < 100;
                if (tick == 60) { state.circle = 0; circle = 0; }
                if (tick == 75) { state.time = 0; time = 0; }
                const auto frame = state.advance(area, level, next, choices);
                require(frame.area == area && frame.level == level,
                    "presentation snapshot retains stage for the composed heading");
                for (unsigned render = 0; render < 2; ++render) {
                    unsigned count = 0;
                    frame.borders([&](int emittedArea, int spriteFrame, int x, int y) {
                        const bool incoming = count >= unsigned(2 * (border + 1));
                        const unsigned local = incoming ? count - 2 * (border + 1) : count;
                        const int row = int(local / 2) + (incoming ? border : 0);
                        require(emittedArea == area + int(incoming) &&
                            spriteFrame == (local % 2 ? 7 : 13) &&
                            x == (incoming ? (local % 2 ? 152 : 8) : (local % 2 ? 160 : 0)) &&
                            y == tileY + row * 16, "source border command ordering and coordinates");
                        ++count;
                    });
                    require(count == 76, "inclusive boundary row is emitted for both areas");
                }
                tileY -= area == 3 ? 3 : 6;
                if (tileY <= -16) {
                    tileY += 16;
                    if (next > 2 && level == 3 && border > 0) --border;
                }
                circle += 3;
                if (choices) ++time;
                require(state.tileY == tileY && state.borderRow == border && frame.circle == circle &&
                    frame.time == time && frame.choices == choices,
                    "source scroll, boundary replacement and animation clocks");
            }
        }
    }
    LevelClearController room;
    room.offers.ids[0] = 0; room.offers.ids[1] = 6; room.offers.ids[2] = 13;
    UpgradeInventory inventory;
    inventory.levels[6] = 1;
    LevelClearPresentation::Frame frame;
    frame.choices = true;
    struct Sink {
        unsigned sprites = 0, players = 0, labels = 0, descriptions = 0;
        unsigned stageLabel = 64500;
        void sprite(unsigned id, unsigned subimage, LevelClearCoordinate x, LevelClearCoordinate y) {
            if (sprites >= 76) {
                const unsigned option = sprites - 76;
                const unsigned ids[] = {524, 522, 516};
                require(id == ids[option] && subimage == 0 && x.magnitude.floor() == 42 + 38 * option &&
                    y.magnitude.floor() == (option == 1 ? 348u : 354u),
                    "source icon curve is clamped after subtracting bob and adding eight");
            }
            ++sprites;
        }
        void player(LevelClearCoordinate x, LevelClearCoordinate y) {
            require(sprites == 76 && x.magnitude.floor() == 80 && !y.magnitude.mantissa,
                "player is drawn between borders and text"); ++players;
        }
        void label(unsigned id, LevelClearCoordinate x, LevelClearCoordinate y, unsigned pen) {
            const unsigned layer = labels % 7;
            require(id == (labels < 7 ? stageLabel : 64001u) &&
                x.magnitude.floor() == 80 - layer && y.magnitude.floor() == (labels < 7 ? 96u : 278u) - layer &&
                pen == (layer == 6 ? 1u : 2u), "six red shadow passes precede white title"); ++labels;
        }
        void description(unsigned id, LevelClearCoordinate x, LevelClearCoordinate y, unsigned suffix) {
            require(id == (descriptions ? 64306u : 64106u) && x.magnitude.floor() == 80 &&
                y.magnitude.floor() == (descriptions ? 382u : 320u) && suffix == (descriptions ? 0u : 2u),
                "selected owned upgrade name suffix and description anchors"); ++descriptions;
        }
    } sink;
    drawLevelClear(sink, room, frame, inventory, HudReal::integer(1), HudReal::integer(1));
    require(sink.sprites == 79 && sink.players == 1 && sink.labels == 14 && sink.descriptions == 2,
        "shared visible result layout emits every source layer");
    for (int area = 1; area <= 4; ++area) {
        for (int level = 1; level <= 3; ++level) {
            Sink stageSink;
            stageSink.stageLabel = 64500 + (area - 1) * 3 + level - 1;
            frame.area = area;
            frame.level = level;
            drawLevelClear(stageSink, room, frame, inventory, HudReal::integer(1), HudReal::integer(1));
            require(stageSink.labels == 14, "every stage uses its offline heading for all shadow passes");
        }
    }
    std::puts("Result border snapshots, clocks and shared layout passed");
}
