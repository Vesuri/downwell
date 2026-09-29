#ifndef DOWNWELL_AMIGA_GOAL_FIELD_H
#define DOWNWELL_AMIGA_GOAL_FIELD_H
#include "AmigaRenderer.h"
#include "core/GoalFieldOutline.h"
#include "core/TimeFieldOutlines.h"
#include "AmigaFieldWords.h"

// Per-display sparse word overlay. Restore only departing coverage; publish
// final colours directly, never clearing continuing pixels. Drain DMA first.
// Shared scratch lives only inside prepare(), so pending draws are independent.
template<class T,unsigned Capacity,bool External> struct AmigaFieldStorage { T words[Capacity]; };
template<class T,unsigned Capacity> struct AmigaFieldStorage<T,Capacity,true> {};
template<class Shape = downwell::GoalFieldOutline,bool ExchangeStorage=false> class AmigaFieldOverlay {
public:
    struct Word { UWORD offset, mask, under[2], paint[2], kept; };
    // Every word touched by the clipped viewport, including partial edge words.
    static constexpr unsigned capacity = ((downwellPlayfieldWidth + 30) / 16) * downwellDisplayHeight;
    struct Scratch {
        Word bank[capacity];
        Word* words=bank;
        UWORD index[downwellGameRingPlaneBytes / 2] = {};
        unsigned count = 0;
    };
private:
    AmigaFieldStorage<Word,capacity,ExchangeStorage> storage_;
    Word* saved_=nullptr;
    Scratch* workspace_=nullptr;
    unsigned count_ = 0, origin_ = 0;
    UBYTE* target_ = nullptr;
    bool valid_ = false, pending_ = false;
    Shape shape_;
    static Scratch& scratch() { static Scratch value{}; return value; }
    static UWORD read(const UBYTE* p) { return (UWORD(p[0]) << 8) | p[1]; }
    void publish(unsigned offset, UWORD value) {
        UBYTE* p = target_ + offset;
        if (read(p) == value) return;
        p[0] = value >> 8; p[1] = value;
        ++lastWordWrites;
    }
public:
    AmigaFieldOverlay(){
        static_assert(!ExchangeStorage,"Exchanged overlays require externally owned banks");
        saved_=storage_.words;
    }
    AmigaFieldOverlay(Word* bank,Scratch& workspace):saved_(bank),workspace_(&workspace){
        static_assert(ExchangeStorage,"External banks require exchange mode");
    }
    AmigaFieldOverlay(const AmigaFieldOverlay&)=delete;
    AmigaFieldOverlay& operator=(const AmigaFieldOverlay&)=delete;
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
        auto& next = [&]() -> Scratch& {
            if constexpr(ExchangeStorage)return *workspace_;
            else return scratch();
        }();
        for (unsigned i = 0; i < next.count; ++i) next.index[next.words[i].offset / 2] = 0;
        next.count = 0;
        bool overflow = false;
        amigaFieldWords(shape,downwellPlayfieldX,0,downwellPlayfieldX+downwellPlayfieldWidth,
                          downwellDisplayHeight,[&](int x,int y,unsigned mask,unsigned low,unsigned high) {
            unsigned row = origin + static_cast<unsigned>(y);
            if (row >= downwellGameRingHeight) row -= downwellGameRingHeight;
            const unsigned rowOffset = row * downwellBytesPerPlaneLine;
            const unsigned offset = rowOffset + (x / 16) * 2;
            auto& slot = next.index[offset / 2];
            if (!slot) {
                if (next.count == capacity) { overflow = true; return; }
                slot = ++next.count;
                next.words[slot - 1] = {static_cast<UWORD>(offset),0,{0,0},{0,0},0};
            }
            auto& word = next.words[slot - 1];
            word.mask |= mask;
            word.paint[0]=(word.paint[0]&static_cast<UWORD>(~mask))|low;
            word.paint[1]=(word.paint[1]&static_cast<UWORD>(~mask))|high;
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
        if constexpr(ExchangeStorage){
            // Clear the lookup while it still describes the completed bank.
            // The next prepare borrows the departing saved bank, whose offsets
            // need not match this frame's geometry. Pending draws own saved_.
            for(unsigned i=0;i<next.count;++i)next.index[next.words[i].offset/2]=0;
            Word* old=saved_;saved_=next.words;next.words=old;next.count=0;
        }else for (unsigned i = 0; i < count_; ++i) saved_[i] = next.words[i];
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
// Three banks total, just like two retained overlays plus their shared scratch.
// All banks outlive both overlays; no shared pointer can escape into a destroyed
// stack object. Either overlay may prepare first, or prepare again before draw.
class AmigaTimeFieldPair {
    using Overlay=AmigaFieldOverlay<downwell::TimeFieldOutlines,true>;
    Overlay::Word first_[Overlay::capacity],second_[Overlay::capacity];
    Overlay::Scratch scratch_;
    Overlay overlays_[2]={{first_,scratch_},{second_,scratch_}};
public:
    Overlay& operator[](unsigned index){return overlays_[index];}
    Overlay* begin(){return overlays_;}
    Overlay* end(){return overlays_+2;}
};
#endif
