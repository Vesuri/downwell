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
                unsigned nextMeter, const downwell::AmmoGaugeRows* rows) {
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
            }
            for (unsigned i = 0; i < 184; ++i) {
                const unsigned a = previous.state[i] < 4 ? previous.state[i] : 3;
                const unsigned b = rows->state[i] < 4 ? rows->state[i] : 3;
                if (a != b || (i < previous.filled) != (i < rows->filled)) {
                    const int y = 229 + 4 * nextMeter - static_cast<int>(i);
                    if (y < 232) include(y, y + 8);
                }
            }
        }
        ammo = nextAmmo; capacity = nextCapacity; offset = nextOffset; meter = nextMeter;
        if (rows) previous = *rows;
        valid = rows != nullptr;
    }
};
#endif
