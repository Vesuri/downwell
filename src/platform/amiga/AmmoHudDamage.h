#ifndef DOWNWELL_AMIGA_AMMO_HUD_DAMAGE_H
#define DOWNWELL_AMIGA_AMMO_HUD_DAMAGE_H
#include "core/AmmoGaugeRows.h"
// Retained presentation only: never advance the simulation's row animation.
struct AmmoHudDamage {
    downwell::AmmoGaugeRows previous;
    unsigned ammo = 0, capacity = 0, offset = 0, meter = 0;
    bool valid = false;
    int top = 256, bottom = 0;
    bool label = false;
    void include(int first, int last) {
        if (first < top) top = first;
        if (last > bottom) bottom = last;
    }
    void update(unsigned nextAmmo, unsigned nextCapacity, unsigned nextOffset,
                unsigned nextMeter, const downwell::AmmoGaugeRows* rows,
                bool useVisitedRange = true) {
        top = 256; bottom = 0;
        label = !valid || ammo != nextAmmo || offset != nextOffset;
        if (!valid || !rows || capacity != nextCapacity || meter != nextMeter) include(40,256);
        else {
            if (ammo != nextAmmo) {
                const unsigned oldFill = downwell::ammoGaugeFilledRows(ammo, capacity);
                const unsigned newFill = downwell::ammoGaugeFilledRows(nextAmmo, nextCapacity);
                const unsigned low = oldFill < newFill ? oldFill : newFill;
                const unsigned high = oldFill > newFill ? oldFill : newFill;
                // Fill top and all dividers whose threshold changed, including
                // 8-row artwork, +/-1 shadows and the low-charge positive fill.
                include(229 + 4 * nextMeter - static_cast<int>(high) - 2,
                        229 + 4 * nextMeter - static_cast<int>(low) + 10);
                // The white fill does not follow that shifted artwork. Its
                // positive-charge rectangle starts at 237 - 184*ammo/capacity;
                // include both old/new fixed boundaries (and rounding margin).
                // At meter=3, the shifted envelope alone misses the top of a
                // full fill when ammo changes without another jiggle change.
                const int fixedTop=235-static_cast<int>(high);
                // Very low charge uses the source's positive fill at y=232.
                include(fixedTop<232?fixedTop:232,239-static_cast<int>(low));
            }
            const int base = 229 + 4 * nextMeter;
            const unsigned firstVisible = base >= 232 ? base - 231 : 0;
            // Sprite-family changes occupy a contiguous interval. Include its
            // endpoints once, instead of testing the fill boundary per row.
            unsigned low = previous.filled < rows->filled ? previous.filled : rows->filled;
            unsigned high = previous.filled > rows->filled ? previous.filled : rows->filled;
            if (low < firstVisible) low = firstVisible;
            if (high > 184) high = 184;
            if (low < high) include(base - static_cast<int>(high) + 1,
                                    base - static_cast<int>(low) + 8);
            // Damage is a single bounding strip. Once its first and last
            // changed frames are known, inspecting the interior cannot alter it.
            auto changed = [&](unsigned i) {
                const unsigned a = previous.state[i], b = rows->state[i];
                return a != b && (a < 3 || b < 3); // states 3 and 4 both draw frame 3
            };
            unsigned first = firstVisible;
            unsigned end = 184;
            // Only a consecutive visual update has a complete producer range.
            // Missed draws or unadvanced externally edited test state retain
            // the full scan. Production state advances via step(); replacing
            // its timeline requires invalidating this observer as usual.
            if (useVisitedRange && rows->visualSerial() != 0xffffffffu &&
                rows->visualSerial() == previous.visualSerial() + 1u) {
                if (first < rows->visualFirst()) first = rows->visualFirst();
                end = rows->visualEnd();
            }
            while (first < end && !changed(first)) ++first;
            if (first < end) {
                unsigned last = end - 1u;
                while (last > first && !changed(last)) --last;
                include(base - static_cast<int>(last), base - static_cast<int>(first) + 8);
            }
        }
        ammo = nextAmmo; capacity = nextCapacity; offset = nextOffset; meter = nextMeter;
        if (rows) previous = *rows;
        valid = rows != nullptr;
    }
};
#endif
