#ifndef DOWNWELL_AMIGA_SELECTION_LAYER_H
#define DOWNWELL_AMIGA_SELECTION_LAYER_H
#include "AmigaClearOverlay.h"

// Shared retained world-space canvas for selection scenes. Camera movement
// changes view() only; callers draw independent animation in world coordinates.
// Two canvases fit the existing two clear display slots without new Chip RAM.
// Native target/art must be Chip-resident; caller drains blits before publish
// or bank replacement. Invalidate when replacing artwork in place.
class AmigaSelectionLayer {
public:
    static constexpr unsigned width = 160, dmaWidth = 176, stride = 22;
    static constexpr unsigned height = 608, planeBytes = stride * height;
    static constexpr unsigned bytes = planeBytes * 2;
    using Command = AmigaClearOverlay::Command;
    using Rect = AmigaClearOverlay::Rect;
    static constexpr unsigned capacity = 80; // includes both border loops (76 commands)
    void invalidate() { initialized_ = false; }
    // Caller has proved every resource referenced by this layer has identical
    // geometry and pixels in the replacement bank. Directory offsets may
    // change: no record/payload pointers are retained across render() calls.
    void rebindEquivalentArtwork(const UBYTE* pack, const UBYTE* end) {
        pack_ = pack; end_ = end;
    }
    const UBYTE* view(unsigned top) const {
        return target_ && top <= height - 284 ? target_ + top * stride : nullptr;
    }
    bool render(UBYTE* target, const UBYTE* pack, const UBYTE* end,
                const Command* commands, unsigned count, unsigned& paintedPixels);
private:
    UBYTE* target_ = nullptr;
    const UBYTE* pack_ = nullptr;
    const UBYTE* end_ = nullptr;
    bool initialized_ = false;
    unsigned count_ = 0;
    Command previous_[capacity];
    Rect bounds_[capacity], next_[capacity], dirty_[capacity * 2];
};
#endif
