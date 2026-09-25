#ifndef DOWNWELL_AMIGA_LASER_SIGHT_H
#define DOWNWELL_AMIGA_LASER_SIGHT_H
#include "AmigaRenderer.h"
#include "core/LaserSightLine.h"

// One instance per display buffer. prepare() must run before any underlying
// terrain/BOB writes; draw() runs at the ray's painter-order position afterward.
// The pristine terrain backing is never modified. All CPU access requires DMA
// blits to this display to have drained before calling either method.
class AmigaLaserSight {
    struct ByteChange { UWORD offset; UBYTE mask, bits; };
    static_assert(sizeof(ByteChange) == 4, "ray damage entries must stay compact");
    // A monotonic integer line clipped to 160x284 visits at most 443 pixels.
    // At most two changed plane bytes per pixel. Retain actual bits and an
    // absolute plane offset, so restoration does not decode colours/planes.
    ByteChange saved_[2 * (downwellPlayfieldWidth + downwellDisplayHeight)];
    unsigned count_ = 0, origin_ = 0;
    UBYTE* target_ = nullptr;
    downwell::LaserSightLine line_{};
    bool valid_ = false, pending_ = false;
    static bool same(const downwell::LaserSightLine& a, const downwell::LaserSightLine& b) {
        return a.x0 == b.x0 && a.y0 == b.y0 && a.x1 == b.x1 && a.y1 == b.y1;
    }
    unsigned restore(const ByteChange& p) {
        UBYTE& byte = target_[p.offset];
        const UBYTE value = (byte & (UBYTE)~p.mask) | p.bits;
        if (byte == value) return 0;
        byte = value;
        return 1;
    }
public:
    unsigned lastByteWrites = 0;
    // origin is normalized and y is clipped to the visible 284 rows, so their
    // sum is below twice the 320-row ring. Avoid a 32-bit modulo per pixel.
    static unsigned physicalRow(unsigned origin, unsigned y) {
        const unsigned row = origin + y;
        return row >= downwellGameRingHeight ? row - downwellGameRingHeight : row;
    }
    bool present() const { return valid_ || pending_; }
    const downwell::LaserSightLine* line() const { return valid_ ? &line_ : nullptr; }
    unsigned origin() const { return origin_; }
    bool matches(UBYTE* target, unsigned origin, const downwell::LaserSightLine& line) const {
        return valid_ && target == target_ && origin == origin_ && same(line, line_);
    }
    // Only when a full display rebuild will replace the old pixels.
    void discard() { count_ = 0; valid_ = pending_ = false; }
    // underlyingChanged includes both already-dirty terrain rows and actors
    // about to overlap the ray. False is valid only for an untouched backing.
    // Keep the target's hot loops out of the very large main function. LTO
    // otherwise spills their pixel state to main's stack on almost every step.
#ifdef DOWNWELL_FREESTANDING
    __attribute__((noinline))
#endif
    bool prepare(UBYTE* target, unsigned origin, const downwell::LaserSightLine* line,
                 bool underlyingChanged) {
        lastByteWrites = 0;
        origin %= downwellGameRingHeight;
        if (target != target_) {
            // Different storage is a new scene, not permission to write into
            // the old allocation. Caller owns restoration before reusing it.
            count_ = 0; valid_ = false; target_ = target;
        }
        if (line && matches(target, origin, *line) && !underlyingChanged) {
            pending_ = false;
            return false;
        }
        unsigned writes = 0;
        const auto* end = saved_ + count_;
        for (const auto* entry = saved_; entry != end; ++entry) writes += restore(*entry);
        lastByteWrites = writes;
        count_ = 0; valid_ = false;
        origin_ = origin;
        pending_ = line != nullptr;
        if (line) line_ = *line;
        return pending_;
    }
#ifdef DOWNWELL_FREESTANDING
    __attribute__((noinline))
#endif
    void draw() {
        if (!pending_) return;
        auto* const begin = saved_ + count_;
        auto* next = begin;
        line_.draw([&](int x, int y) {
            if (x < downwellPlayfieldX || x >= downwellPlayfieldX + downwellPlayfieldWidth ||
                y < 0 || y >= downwellDisplayHeight) return;
            const unsigned row = physicalRow(origin_, static_cast<unsigned>(y));
            const UWORD offset = row * downwellBytesPerPlaneLine + (x >> 3);
            const UBYTE mask = 0x80u >> (x & 7);
            UBYTE& low = target_[offset];
            UBYTE& high = target_[offset + downwellGameRingPlaneBytes];
            const UBYTE previousLow = low, previousHigh = high;
            // These bytes were just read to save the original pen. Red is 10:
            // publish each changed plane directly, without rereading it or
            // composing a generic two-plane colour a second time.
            if (previousLow & mask) {
                *next++ = {offset, mask, mask};
                low = previousLow & (UBYTE)~mask;
            }
            if (!(previousHigh & mask)) {
                *next++ = {static_cast<UWORD>(offset + downwellGameRingPlaneBytes), mask, 0};
                high = previousHigh | mask;
            }
        });
        // Every newly saved entry corresponds to exactly one changed-byte
        // publication. Publish bookkeeping once, not for every plane/pixel.
        count_ = static_cast<unsigned>(next - saved_);
        lastByteWrites += static_cast<unsigned>(next - begin);
        pending_ = false; valid_ = true;
    }
};
#endif
