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
#ifndef DOWNWELL_FRAME_WORK_CANDIDATE
    for (auto& row : dirty_) row = 3;
#else
    for(auto& debt:debt_)debt.reset();
#endif
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
#ifndef DOWNWELL_FRAME_WORK_CANDIDATE
    for (unsigned row = 0; row < height; ++row) {
        dirty_[physical] |= buffers;
        if (++physical == downwellGameRingHeight) physical = 0;
    }
#else
    for(unsigned b=0;b<2;++b)if(buffers&(1u<<b)){
        if(height>=downwellGameRingHeight)debt_[b].reset();
        else{
            const unsigned stop=physical+height;
            debt_[b].assign(physical,stop<downwellGameRingHeight?stop:downwellGameRingHeight,true);
            if(stop>downwellGameRingHeight)debt_[b].assign(0,stop-downwellGameRingHeight,true);
        }
    }
#endif
}

void AmigaGameplayRows::copied(unsigned first, unsigned count, unsigned buffer) {
#ifndef DOWNWELL_FRAME_WORK_CANDIDATE
    const unsigned char keep = static_cast<unsigned char>(~(1u << buffer));
    for (unsigned row = first; row < first + count; ++row){
        dirty_[row] &= keep;
    }
#else
    debt_[buffer].assign(first,first+count,false);
#endif
}

void AmigaGameplayRows::sync(Bitmap& destination, const Bitmap& backing, unsigned buffer) {
    DW_PROFILE_SCOPE(profileGameCopy);
    if (!pending_[buffer]) return;
    pending_[buffer] = false;
#ifndef DOWNWELL_FRAME_WORK_CANDIDATE
    const unsigned char bit = static_cast<unsigned char>(1u << buffer);
#endif
    unsigned row = first_[buffer];
    const unsigned end = end_[buffer];
    first_[buffer] = downwellGameRingHeight;
    end_[buffer] = 0;
    while (row < end) {
#ifndef DOWNWELL_FRAME_WORK_CANDIDATE
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
#else
        row=debt_[buffer].next(row,end,true);
        if(row==end)break;
        const unsigned first=row;
        row=debt_[buffer].next(row,end,false);
        debt_[buffer].assign(first,row,false);
#ifdef DOWNWELL_ROW_SYNC_AUDIT
        ++amigaRowSyncVisited; // One exact run, not a per-row walk.
#endif
#endif
        const unsigned x = initialized_[buffer] ? terrainX : 0;
        const unsigned width = initialized_[buffer] ? terrainWords * 16 : downwellDmaWidth;
        destination.copy(backing, x, first, x, first, width, row - first);
    }
    AmigaHardware::setBlitterNasty(true);
    AmigaHardware::blitterDrain();
    AmigaHardware::setBlitterNasty(false);
    initialized_[buffer] = true;
}
