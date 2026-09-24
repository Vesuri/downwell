#ifndef DOWNWELL_CORE_AMMO_DIVIDER_LAYOUT_H
#define DOWNWELL_CORE_AMMO_DIVIDER_LAYOUT_H
#include "AmmoDividers.h"
namespace downwell {
// Retained geometry for the current one-charge weapon. No approximation of
// binary64 loop subtraction or raster rounding: evaluate once per capacity.
// Unbounded capacities/other consumption use the source-equivalent fallback.
struct AmmoDividerLayout {
    struct Entry { HudReal level; unsigned short y[4][3]; } entries[65];
    unsigned capacity = 0, count = 0;
    unsigned cachedAmmo = ~0u;
    HudReal filled;
    void prepare(unsigned cap) {
        if (capacity == cap) return;
        capacity = cap; count = 0; cachedAmmo = ~0u;
        if (!cap || cap > 64) return;
        const auto spacing = HudReal::ratio(1, cap).times184();
        const auto one = HudReal::integer(1);
        for (auto i = HudReal::integer(184); i.mantissa; i = i.subtract(spacing)) {
            auto& entry = entries[count++];
            entry.level = i;
            for (unsigned offset = 0; offset < 4; ++offset) {
                const auto position = HudReal::integer(229 + offset * 4).subtract(i);
                entry.y[offset][0] = position.floor();
                entry.y[offset][1] = position.add(one).floor();
                entry.y[offset][2] = position.subtract(one).floor();
            }
        }
    }
    template<class Draw> void draw(unsigned ammo, unsigned cap, unsigned consumption,
                                   int x, unsigned bottom, Draw drawSprite) {
        if (consumption != 1 || cap > 64 || bottom < 229 || bottom > 241 || (bottom-229)%4) {
            drawAmmoDividers(ammo, cap, consumption, x, bottom, drawSprite); return;
        }
        prepare(cap);
        if (!cap) return;
        if (ammo != cachedAmmo) { filled = HudReal::ratio(ammo, cap).times184(); cachedAmmo = ammo; }
        const unsigned offset = (bottom - 229) / 4;
        if (cap < 20) for (unsigned i = 0; i < count; ++i)
            if (entries[i].level.less(filled)) drawSprite(421, 1, x, entries[i].y[offset][0]);
        for (unsigned i = 0; i < count; ++i) {
            const auto& entry = entries[i];
            if (entry.level.less(filled)) {
                drawSprite(421, 0, x, entry.y[offset][0]);
                drawSprite(421, 1, x, entry.y[offset][1]);
                drawSprite(421, 1, x, entry.y[offset][2]);
            } else if (entry.level.equal(filled) ||
                       (entry.level.exponent == 0 && entry.level.mantissa == (std::uint64_t{1} << 52)))
                drawSprite(421, 1, x, entry.y[offset][1]);
        }
    }
};
}
#endif
