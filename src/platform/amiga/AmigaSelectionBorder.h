#ifndef DOWNWELL_AMIGA_SELECTION_BORDER_H
#define DOWNWELL_AMIGA_SELECTION_BORDER_H
#include "AmigaSelectionLayer.h"
#include "core/LevelClearPresentation.h"
#include "core/LevelUpgradeArt.h"

// The source's finite border loops depend on area and seam row only once
// tileY is removed. Ordinary camera/tile movement must not rebuild or compare
// their 76 draw commands. Each retired display slot owns its own geometry.
class AmigaSelectionBorder {
    static_assert(AmigaSelectionLayer::capacity >= 76, "Both inclusive source border loops must fit");
    AmigaSelectionLayer layer_;
    AmigaSelectionLayer::Command commands_[AmigaSelectionLayer::capacity];
    UBYTE* target_ = nullptr;
    const UBYTE* pack_ = nullptr;
    const UBYTE* end_ = nullptr;
    int area_ = 0, row_ = -1;
    bool ready_ = false;
public:
    bool matches(const UBYTE* target, const UBYTE* pack, const UBYTE* end,
                 const downwell::LevelClearPresentation::Frame& frame) const {
        return ready_ && target_ == target && pack_ == pack && end_ == end &&
            area_ == frame.area && row_ == frame.borderRow;
    }
    void invalidate() { ready_ = false; layer_.invalidate(); }
    void rebindEquivalentArtwork(const UBYTE* pack, const UBYTE* end) {
        pack_ = pack; end_ = end;
        layer_.rebindEquivalentArtwork(pack, end);
    }
    const UBYTE* view(unsigned top) const { return ready_ ? layer_.view(top) : nullptr; }
    bool render(UBYTE* target, const UBYTE* pack, const UBYTE* end,
        const downwell::LevelClearPresentation::Frame& frame, unsigned& pixels) {
        pixels = 0;
        if (!target || frame.borderRow < 0 || frame.borderRow > 36) return false;
        if (matches(target, pack, end, frame)) return true;
        unsigned count = 0;
        frame.borders([&](int area, int image, int x, int y) {
            commands_[count++] = {downwell::levelBorderSprite(area), unsigned(image),
                                 x, y - frame.tileY, -1};
        });
        if (!layer_.render(target, pack, end, commands_, count, pixels)) return false;
        target_ = target; pack_ = pack; end_ = end;
        area_ = frame.area; row_ = frame.borderRow; ready_ = true;
        return true;
    }
};

// Border ownership is independent of the animated overlay's back-buffer index.
// Both copper programs may read one immutable canvas. On a geometry miss draw
// only into the other canvas, never the one returned for the preceding frame.
// The caller publishes each successful result before requesting another frame,
// and retires display DMA before invalidate()/changing the target allocations.
class AmigaSelectionBorders {
    AmigaSelectionBorder borders_[2];
    unsigned selected_ = 2;
public:
    void invalidate() {
        selected_ = 2;
        for (auto& border : borders_) border.invalidate();
    }
    void rebindEquivalentArtwork(const UBYTE* pack, const UBYTE* end) {
        for (auto& border : borders_) border.rebindEquivalentArtwork(pack, end);
    }
    const UBYTE* view(unsigned top) const {
        return selected_ < 2 ? borders_[selected_].view(top) : nullptr;
    }
    bool render(unsigned back, UBYTE* first, UBYTE* second,
                const UBYTE* pack, const UBYTE* end,
                const downwell::LevelClearPresentation::Frame& frame, unsigned& pixels) {
        pixels = 0;
        if (back >= 2 || !first || !second || first == second ||
            frame.borderRow < 0 || frame.borderRow > 36) return false;
        UBYTE* targets[] = {first, second};
#ifdef DOWNWELL_REFERENCE_SELECTION_BORDER_OWNERSHIP
        const unsigned next = back;
#else
        for (unsigned i = 0; i < 2; ++i)
            if (borders_[i].matches(targets[i], pack, end, frame)) {
                selected_ = i;
                return true;
            }
        const unsigned next = selected_ < 2 ? selected_ ^ 1 : 0;
#endif
        if (!borders_[next].render(targets[next], pack, end, frame, pixels)) return false;
        selected_ = next;
        return true;
    }
};
#endif
