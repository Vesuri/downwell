#ifndef DOWNWELL_AMIGA_PREPARED_DIVIDERS_H
#define DOWNWELL_AMIGA_PREPARED_DIVIDERS_H
#include "AmigaRenderer.h"
#include "core/NarrowArithmetic.h"
#ifdef DOWNWELL_INSTALLED_TABLES
#include "../../../generated/installed/amiga-ammo-dividers.h" // WI.3: bound from Downwell.tables
#else
#include "../../../generated/amiga-ammo-dividers.h"
#endif

// Asset validation happens once per pack, never on a shot. Synthetic/changed
// packs retain the sprite compositor instead of silently using different art.
inline bool amigaPreparedDividersMatch(const AmigaWordStamp* stamps) {
    for (unsigned frame = 0; frame < 2; ++frame)
        for (unsigned row = 0; row < 8; ++row)
            if (stamps[frame].low[row] != downwell_ammo_dividers::stamps[frame][row][0] ||
                stamps[frame].high[row] != downwell_ammo_dividers::stamps[frame][row][1] ||
                stamps[frame].mask[row] != ((stamps[frame].low[row] | stamps[frame].high[row]) & 0xfff0u)) return false;
    return true;
}

// The only caller admits capacity<=16.
inline AmigaHudBlockedColumn amigaPreparedDividers(unsigned ammo, unsigned capacity) {
    return {downwell_ammo_dividers::images[downwell::mulu16(capacity,capacity+1)/2 + ammo],
            &downwell_ammo_dividers::blocks[0][0]};
}
#endif
