#include "AmigaGameplayRows.h"
#include "AmigaProfile.h"
#include "framework/Bitmap.h"
#include "framework/AmigaHardware.h"
#ifdef DOWNWELL_ROW_SYNC_AUDIT
extern unsigned amigaRowSyncVisited;
#endif

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
    for (unsigned row = first; row < first + count; ++row){
        dirty_[row] &= keep;
    }
}

void AmigaGameplayRows::queue(Bitmap& destination, const Bitmap& backing, unsigned buffer) {
    if (!pending_[buffer]) return;
    pending_[buffer] = false;
    queued_ = true;
    const unsigned char bit = static_cast<unsigned char>(1u << buffer);
    unsigned row = first_[buffer];
    const unsigned end = end_[buffer];
    first_[buffer] = downwellGameRingHeight;
    end_[buffer] = 0;
#ifdef DOWNWELL_EFFECT_BLIT_CENSUS
    unsigned long syncWords = 0;
    unsigned syncRuns = 0;
#endif
    while (row < end) {
#ifdef DOWNWELL_ROW_SYNC_AUDIT
        ++amigaRowSyncVisited;
#endif
        if (!(dirty_[row] & bit)) { ++row; continue; }
        const unsigned first = row;
        while (row < end && (dirty_[row] & bit)){
#ifdef DOWNWELL_ROW_SYNC_AUDIT
            ++amigaRowSyncVisited;
#endif
            dirty_[row++] &= static_cast<unsigned char>(~bit);
        }
        const unsigned x = initialized_[buffer] ? terrainX : 0;
        const unsigned width = initialized_[buffer] ? terrainWords * 16 : downwellDmaWidth;
        destination.copy(backing, x, first, x, first, width, row - first);
#ifdef DOWNWELL_EFFECT_BLIT_CENSUS
        syncWords += (unsigned long)(width / 16) * (row - first) * destination.bitplanes;
        ++syncRuns;
#endif
    }
#ifdef DOWNWELL_EFFECT_BLIT_CENSUS
    downwellRowSyncCensus(syncWords, syncRuns, 0);
#endif
    initialized_[buffer] = true;
}

void AmigaGameplayRows::sync(Bitmap& destination, const Bitmap& backing, unsigned buffer) {
    DW_PROFILE_SCOPE(profileGameCopy);
    queue(destination, backing, buffer);
    if (!queued_) return;
    queued_ = false;
#ifdef DOWNWELL_EFFECT_BLIT_CENSUS
    const unsigned long syncStart = downwellEffectCensusTick();
#endif
    AmigaHardware::setBlitterNasty(true);
    AmigaHardware::blitterDrain();
    AmigaHardware::setBlitterNasty(false);
#ifdef DOWNWELL_EFFECT_BLIT_CENSUS
    downwellRowSyncCensus(0, 0, downwellEffectCensusTick() - syncStart);
#endif
}
