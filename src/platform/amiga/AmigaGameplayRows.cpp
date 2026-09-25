#include "AmigaGameplayRows.h"
#include "AmigaProfile.h"
#include "framework/Bitmap.h"
#include "framework/AmigaHardware.h"

void AmigaGameplayRows::reset() {
    for (unsigned buffer = 0; buffer < 2; ++buffer) {
        initialized_[buffer] = pending_[buffer] = false;
        first_[buffer] = downwellGameRingHeight;
        end_[buffer] = 0;
    }
    for (auto& row : dirty_) row = 3;
}

void AmigaGameplayRows::damage(unsigned origin, unsigned y, unsigned height, unsigned char buffers) {
    unsigned first = (origin + y) % downwellGameRingHeight;
    unsigned end = first + height;
    if (end > downwellGameRingHeight) { first = 0; end = downwellGameRingHeight; }
    for (unsigned buffer = 0; buffer < 2; ++buffer) {
        if (!(buffers & (1u << buffer))) continue;
        pending_[buffer] = true;
        if (first < first_[buffer]) first_[buffer] = first;
        if (end > end_[buffer]) end_[buffer] = end;
    }
    unsigned physical = (origin + y) % downwellGameRingHeight;
    for (unsigned row = 0; row < height; ++row) {
        dirty_[physical] |= buffers;
        if (++physical == downwellGameRingHeight) physical = 0;
    }
}

void AmigaGameplayRows::copied(unsigned first, unsigned count, unsigned buffer) {
    const unsigned char keep = static_cast<unsigned char>(~(1u << buffer));
    for (unsigned row = first; row < first + count; ++row) dirty_[row] &= keep;
}

void AmigaGameplayRows::sync(Bitmap& destination, const Bitmap& backing, unsigned buffer) {
    DW_PROFILE_SCOPE(profileGameCopy);
    if (!pending_[buffer]) return;
    pending_[buffer] = false;
    const unsigned char bit = static_cast<unsigned char>(1u << buffer);
    unsigned row = first_[buffer];
    const unsigned end = end_[buffer];
    first_[buffer] = downwellGameRingHeight;
    end_[buffer] = 0;
    while (row < end) {
        if (!(dirty_[row] & bit)) { ++row; continue; }
        const unsigned first = row;
        while (row < end && (dirty_[row] & bit))
            dirty_[row++] &= static_cast<unsigned char>(~bit);
        const unsigned x = initialized_[buffer] ? terrainX : 0;
        const unsigned width = initialized_[buffer] ? terrainWords * 16 : downwellDmaWidth;
        destination.copy(backing, x, first, x, first, width, row - first);
    }
    AmigaHardware::setBlitterNasty(true);
    AmigaHardware::blitterDrain();
    AmigaHardware::setBlitterNasty(false);
    initialized_[buffer] = true;
}
