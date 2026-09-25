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
        if (ready_ && target_ == target && pack_ == pack && end_ == end &&
            area_ == frame.area && row_ == frame.borderRow) return true;
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
#endif
