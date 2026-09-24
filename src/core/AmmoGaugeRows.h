#ifndef DOWNWELL_CORE_AMMO_GAUGE_ROWS_H
#define DOWNWELL_CORE_AMMO_GAUGE_ROWS_H
#include <cstdint>
namespace downwell {
// Number of integer rows i satisfying i < 184 * (ammo / capacity) in the VM.
// Avoid a 64-bit divide on 68000. Long multiplication maintains remainder < cap.
inline unsigned ammoGaugeFilledRows(std::uint32_t ammo, std::uint32_t cap) {
    if (!cap || !ammo) return 0;
    if (ammo >= cap) return 184;
    std::uint32_t remainder = 0;
    unsigned quotient = 0;
    for (unsigned bit = 128; bit; bit >>= 1) {
        quotient *= 2;
        if (remainder >= cap - remainder) {
            remainder -= cap - remainder;
            ++quotient;
        } else remainder += remainder;
        if (184 & bit) {
            if (remainder >= cap - ammo) {
                remainder -= cap - ammo;
                ++quotient;
            } else remainder += ammo;
        }
    }
    if (remainder) return quotient + 1;
    // At these exact rational boundaries the binary64 divide followed by
    // multiply rounds just ABOVE the integer, so the VM includes one more row.
    switch (quotient) {
        case 27: case 47: case 54: case 63: case 94:
        case 103: case 108: case 117: case 126: return quotient + 1;
        default: return quotient;
    }
}

struct AmmoGaugeRows {
    std::uint8_t state[184];
    unsigned filled = 184;
    std::uint32_t lastAmmo = 8, lastCapacity = 8;
    AmmoGaugeRows() { for (auto& row : state) row = 3; }
    bool step(std::uint32_t ammo, std::uint32_t capacity) {
        bool changed = false;
        if (ammo != lastAmmo || capacity != lastCapacity) {
            const unsigned next = ammoGaugeFilledRows(ammo, capacity);
            changed = next != filled;
            filled = next;
            lastAmmo = ammo;
            lastCapacity = capacity;
        }
        // Exhaustive reachable binary64 values from the original initial 3:
        // .6000000000000001, 1.4000000000000001, 2.2, 3, 3.8.
        // Adding/subtracting .8 walks this finite chain; integer frames are 0..3,3.
        for (unsigned i = 0; i < 184; ++i) {
            const unsigned old = state[i];
            if (i < filled) { if (state[i] < 4) ++state[i]; }
            else if (state[i]) --state[i];
            changed |= old != state[i];
        }
        return changed;
    }
    template<class Draw>
    void draw(int x, int bottom, Draw drawSprite) const {
        for (unsigned i = 0; i < 184; ++i) {
            const unsigned sprite = i < filled ? (i ? 419 : 421) : 418;
            const unsigned frame = state[i] < 4 ? state[i] : 3;
            drawSprite(sprite, frame, x, bottom - static_cast<int>(i));
        }
    }
};
}
#endif
