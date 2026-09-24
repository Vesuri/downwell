#ifndef DOWNWELL_CORE_AMMO_DIVIDERS_H
#define DOWNWELL_CORE_AMMO_DIVIDERS_H
#include "HudReal.h"
namespace downwell {
// scrDrawHud4x3 blocks 40..70. consIndic/hla assignments in those loops do not
// affect the emitted draw_sprite arguments. Preserve both loops and draw order.
template<class Draw>
void drawAmmoDividers(unsigned ammo, unsigned capacity, unsigned consumption,
                      int x, unsigned bottom, Draw draw) {
    if (!capacity || !consumption) return;
    const auto length = HudReal::integer(184);
    const auto one = HudReal::integer(1);
    const auto y = HudReal::integer(bottom);
    const auto filled = HudReal::ratio(ammo, capacity).times184();
    if (capacity < 20) {
        const auto spacing = HudReal::ratio(1, capacity).times184();
        for (auto i = length; i.mantissa; i = i.subtract(spacing))
            if (i.less(filled)) draw(421, 1, x, y.subtract(i).floor());
    }
    const auto spacing = HudReal::ratio(consumption, capacity).times184();
    for (auto i = length; i.mantissa; i = i.subtract(spacing)) {
        const auto position = y.subtract(i);
        if (i.less(filled)) {
            draw(421, 0, x, position.floor());
            draw(421, 1, x, position.add(one).floor());
            draw(421, 1, x, position.subtract(one).floor());
        } else if (i.equal(filled) || i.equal(one)) {
            draw(421, 1, x, position.add(one).floor());
        }
    }
}
}
#endif
