#ifndef DOWNWELL_AMIGA_GAMEPLAY_ROWS_H
#define DOWNWELL_AMIGA_GAMEPLAY_ROWS_H
#include "AmigaRenderer.h"
#include "AmigaRowDebt.h"
class Bitmap;

// Independent debts to the private terrain backing for two display rings.
// Physical row coordinates are used here, never camera/world coordinates.
class AmigaGameplayRows {
    unsigned char dirty_[downwellGameRingHeight]{};
    bool pending_[2]{}, initialized_[2]{}, queued_{};
    unsigned first_[2]{downwellGameRingHeight, downwellGameRingHeight};
    unsigned end_[2]{};
public:
    static constexpr unsigned terrainX = 96, terrainWords = 11;
    // Retired/reused display storage: no initialized margins. World preparation
    // must damage its completed rows before sync (reset alone schedules no DMA).
    void reset();
    void damage(unsigned origin, unsigned y, unsigned height, unsigned char buffers = 3);
    bool dirty(unsigned row, unsigned buffer) const {
        return (dirty_[row] & (1u << buffer)) != 0;
    }
    // Nonwrapping physical span already copied by the pipelined terrain path.
    // The pending range may
    // remain conservative; sync skips these rows without copying them again.
    void copied(unsigned first, unsigned count, unsigned buffer);
    // Queue the copies of every row dirty so far without waiting for them.
    // Only once the buffer's previous list is off screen, and only while no
    // display-resident pixels (laser, goal field) still have to be retired
    // before their rows are restored.
    void queue(Bitmap& destination, const Bitmap& backing, unsigned buffer);
    // Queue the remaining rows, then wait until the buffer matches the backing.
    void sync(Bitmap& destination, const Bitmap& backing, unsigned buffer);
};
#endif
