#ifndef DOWNWELL_AMIGA_GOAL_FIELD_H
#define DOWNWELL_AMIGA_GOAL_FIELD_H
#include "AmigaRenderer.h"
#include "core/GoalFieldOutline.h"
#include "core/TimeFieldOutlines.h"
#include "AmigaFieldSpans.h"

// Per-display sparse word overlay. Restore only departing coverage; publish
// final colours directly, never clearing continuing pixels. Drain DMA first.
// Shared scratch lives only inside prepare(), so pending draws are independent.
template<class Shape = downwell::GoalFieldOutline> class AmigaFieldOverlay {
    struct Word { UWORD offset, mask, under[2], paint[2], kept; };
    // Every word touched by the clipped viewport, including partial edge words.
    static constexpr unsigned capacity = ((downwellPlayfieldWidth + 30) / 16) * downwellDisplayHeight;
    Word saved_[capacity];
    unsigned count_ = 0, origin_ = 0;
    UBYTE* target_ = nullptr;
    bool valid_ = false, pending_ = false;
    Shape shape_;
    struct Scratch {
        Word words[capacity];
        UWORD index[downwellGameRingPlaneBytes / 2] = {};
        unsigned count = 0;
    };
    static Scratch& scratch() { static Scratch value{}; return value; }
    static UWORD read(const UBYTE* p) { return (UWORD(p[0]) << 8) | p[1]; }
    void publish(unsigned offset, UWORD value) {
        UBYTE* p = target_ + offset;
        if (read(p) == value) return;
        p[0] = value >> 8; p[1] = value;
        ++lastWordWrites;
    }
public:
    unsigned lastWordWrites = 0;
    bool present() const { return valid_ || pending_; }
    bool matches(UBYTE* target, unsigned origin, const Shape& shape) const {
        return valid_ && target == target_ && origin == origin_ && shape.same(shape_);
    }
    void discard() { count_ = 0; valid_ = pending_ = false; }
    template<class DirtyRow>
    bool prepare(UBYTE* target, unsigned origin, const Shape& shape,
                 bool underlyingChanged, DirtyRow dirtyRow) {
        lastWordWrites = 0;
        if (target != target_) { discard(); target_ = target; }
        if (matches(target, origin, shape) && !underlyingChanged) return true;
        auto& next = scratch();
        for (unsigned i = 0; i < next.count; ++i) next.index[next.words[i].offset / 2] = 0;
        next.count = 0;
        bool overflow = false;
        amigaFieldSpans(shape,downwellPlayfieldX,0,downwellPlayfieldX+downwellPlayfieldWidth,
                          downwellDisplayHeight,[&](int first,int end,int y,int pen) {
            unsigned row = origin + static_cast<unsigned>(y);
            if (row >= downwellGameRingHeight) row -= downwellGameRingHeight;
            const unsigned rowOffset = row * downwellBytesPerPlaneLine;
            for(int x=first;x<end;){
                const int wordEnd=(x|15)+1;
                const int nextX=wordEnd<end?wordEnd:end;
                const unsigned offset = rowOffset + (x / 16) * 2;
                auto& slot = next.index[offset / 2];
                if (!slot) {
                    if (next.count == capacity) { overflow = true; return; }
                    slot = ++next.count;
                    next.words[slot - 1] = {static_cast<UWORD>(offset),0,{0,0},{0,0},0};
                }
                auto& word = next.words[slot - 1];
                const UWORD mask = (0xffffu >> (x&15)) & (0xffffu << (15-((nextX-1)&15)));
                word.mask |= mask;
                for (unsigned plane = 0; plane < 2; ++plane)
                    word.paint[plane] = (pen & (1u << plane)) ? word.paint[plane] | mask
                        : word.paint[plane] & static_cast<UWORD>(~mask);
                x=nextX;
            }
        });
        if (overflow) return false;
        for (unsigned i = 0; i < count_; ++i) {
            const auto& old = saved_[i];
            const unsigned slot = next.index[old.offset / 2];
            const UWORD keep = slot && !dirtyRow(old.offset / downwellBytesPerPlaneLine)
                ? old.mask & next.words[slot - 1].mask : 0;
            const UWORD restore = old.mask & static_cast<UWORD>(~keep);
            for (unsigned plane = 0; plane < 2; ++plane) {
                const unsigned offset = old.offset + plane * downwellGameRingPlaneBytes;
                if (restore) publish(offset, (read(target_ + offset) & static_cast<UWORD>(~restore)) |
                                            (old.under[plane] & restore));
                if (keep) next.words[slot - 1].under[plane] |= old.under[plane] & keep;
            }
            if (keep) next.words[slot - 1].kept = keep;
        }
        count_ = next.count;
        for (unsigned i = 0; i < count_; ++i) saved_[i] = next.words[i];
        shape_ = shape; origin_ = origin; valid_ = false; pending_ = shape.visible;
        return true;
    }
    void draw() {
        if (!pending_) return;
        for (unsigned i = 0; i < count_; ++i) {
            auto& word = saved_[i];
            for (unsigned plane = 0; plane < 2; ++plane) {
                const unsigned offset = word.offset + plane * downwellGameRingPlaneBytes;
                const UWORD actual = read(target_ + offset);
                word.under[plane] |= actual & word.mask & static_cast<UWORD>(~word.kept);
                publish(offset, (actual & static_cast<UWORD>(~word.mask)) | word.paint[plane]);
            }
        }
        pending_ = false; valid_ = true;
    }
};
using AmigaGoalField = AmigaFieldOverlay<>;
using AmigaTimeFields = AmigaFieldOverlay<downwell::TimeFieldOutlines>;
#endif
