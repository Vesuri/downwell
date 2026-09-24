#ifndef DOWNWELL_CORE_LEVEL_CLEAR_LAYOUT_H
#define DOWNWELL_CORE_LEVEL_CLEAR_LAYOUT_H
#include "LevelClearController.h"
#include "LevelUpgradeArt.h"
namespace downwell {
// Source Result Draw layout. Trigonometric samples are supplied by the caller;
// all subsequent arithmetic and command ordering are shared, integer-only.
// Sink coordinates are world-space; the backend applies camera/viewport once.
template<class Sink>
void drawLevelClear(Sink& sink, const LevelClearController& room,
                    const LevelClearPresentation::Frame& frame,
                    const UpgradeInventory& inventory, HudReal absoluteCosine,
                    HudReal absoluteSine) {
    using C = LevelClearCoordinate;
    frame.borders([&](int area, int subimage, int x, int y) {
        sink.sprite(levelBorderSprite(area), unsigned(subimage), C::integer(x), C::integer(y));
    });
    sink.player(C::integer(80), C{room.motion.playerY, false});
    const auto shadow = [&](unsigned label, int y) {
        for (unsigned i = 0; i <= 6; ++i) {
            const C offset{absoluteCosine.timesSmall(i), true};
            sink.label(label, C::integer(80).add(offset), C::integer(y).add(offset), i == 6 ? 1 : 2);
        }
    };
    shadow(64000, 96);
    if (!frame.choices) return;
    shadow(64001, 278);
    for (unsigned i = 0; i <= room.selection.lastIndex; ++i) {
        const auto x = room.carousel.optionX(i);
        const auto distance = x.add(C::integer(-80)).magnitude.divideSmall(2);
        // Preserve the source's order: 310+36+distance, minus bob, plus 8.
        auto y = HudReal::integer(346).add(distance).subtract(absoluteSine.timesSmall(6)).add(HudReal::integer(8));
        if (HudReal::integer(354).less(y)) y = HudReal::integer(354);
        sink.sprite(levelUpgradeSprite(room.offers.ids[i]), 0, x, C{y, false});
    }
    const unsigned id = room.offers.ids[room.selection.cursor];
    const unsigned owned = id <= UpgradeInventory::highestId ? inventory.levels[id] : 0;
    sink.description(64100 + id, C::integer(80), C::integer(320), owned ? owned + 1 : 0);
    sink.description(64300 + id, C::integer(80), C::integer(382), 0);
}
}
#endif
