#ifndef DOWNWELL_AMIGA_CLEAR_OVERLAY_H
#define DOWNWELL_AMIGA_CLEAR_OVERLAY_H
#include "AmigaRenderer.h"
#include "core/Profile.h"

// One instance per retired overlay buffer. The owner keeps player sprites and
// hardware-scrolled borders separate and retires DMA before replacing the pack.
// Commands retain source draw order; pen -1 uses the original asset colors.
struct AmigaClearOverlay {
    static constexpr unsigned capacity = 28; // 14 title passes + 4 icons + 10 outlined-text passes
    struct Command {
        unsigned id = 0, frame = 0;
        int x = 0, y = 0, pen = -1;
        bool operator==(const Command& b) const {
            return id == b.id && frame == b.frame && x == b.x && y == b.y && pen == b.pen;
        }
    };
    struct Rect {
        int x = 0, y = 0, width = 0, height = 0;
        bool overlaps(const Rect& b) const {
            return x < b.x + b.width && b.x < x + width &&
                y < b.y + b.height && b.y < y + height;
        }
    };
    Command previous[capacity];
    Rect bounds[capacity];
    // Persistent scratch, not 1344 bytes on the 4 KiB AmigaDOS stack.
    // Each retained buffer owns its renderer state; render is not reentrant.
    Rect next[capacity];
    Rect dirty[capacity * 2];
    unsigned count = 0;
    bool initialized = false;
    void invalidate() { initialized = false; }

    // Caller invalidates after changing asset bytes, even if the bank address
    // is unchanged. Failed validation leaves both cache and pixels untouched.
    bool render(AmigaRenderer& renderer, const UBYTE* pack, const UBYTE* end,
                const Command* commands, unsigned size, unsigned& paintedPixels) {
        paintedPixels = 0;
        if (size > capacity || (size && !commands)) return false;
        for (unsigned i = 0; i < size; ++i) {
            const auto& c = commands[i];
            auto& r = next[i];
            if (c.id > 65535 || c.frame > 65535 || c.pen < -1 || c.pen > 3 ||
                !amigaRendererAssetBounds(pack, end, c.id, c.frame, c.x, c.y, 0,
                    &r.x, &r.y, &r.width, &r.height)) return false;
            const int right = r.x + r.width < 270 ? r.x + r.width : 270;
            const int bottom = r.y + r.height < 284 ? r.y + r.height : 284;
            if (r.x < 110) r.x = 110;
            if (r.y < 0) r.y = 0;
            r.width = right > r.x ? right - r.x : 0;
            r.height = bottom > r.y ? bottom - r.y : 0;
        }
        unsigned dirtyCount = 0;
        const auto add = [&](Rect r) {
            if (!r.width || !r.height) return;
            // Coalesce overlapping shadow/outline passes, preserving disjoint
            // text/icon regions instead of clearing their full bounding box.
            for (unsigned i = 0; i < dirtyCount;) {
                if (!r.overlaps(dirty[i])) { ++i; continue; }
                const auto old = dirty[i];
                const int right = r.x + r.width > old.x + old.width ? r.x + r.width : old.x + old.width;
                const int bottom = r.y + r.height > old.y + old.height ? r.y + r.height : old.y + old.height;
                if (old.x < r.x) r.x = old.x;
                if (old.y < r.y) r.y = old.y;
                r.width = right - r.x; r.height = bottom - r.y;
                dirty[i] = dirty[--dirtyCount];
                i = 0;
            }
            dirty[dirtyCount++] = r;
        };
        if (!initialized) add({110, 0, 160, 284});
        else {
            for (unsigned i = 0; i < count || i < size; ++i) {
                if (i < count && i < size && previous[i] == commands[i]) continue;
                if (i < count) add(bounds[i]);
                if (i < size) add(next[i]);
            }
        }
        const AmigaRenderer saved = renderer;
        renderer.target = renderer.hud;
        for (unsigned i = 0; i < dirtyCount; ++i) {
            DW_PROFILE_SCOPE(profileClearOverlayPaint);
            const auto& r = dirty[i];
            renderer.clipX = r.x; renderer.clipY = r.y;
            renderer.clipWidth = r.width; renderer.clipHeight = r.height;
            amigaRendererFillRect(&renderer, r.x, r.y, r.width, r.height, 0);
            paintedPixels += static_cast<unsigned short>(r.width) * static_cast<unsigned short>(r.height);
            for (unsigned j = 0; j < size; ++j) {
                if (!r.overlaps(next[j])) continue;
                const auto& c = commands[j];
                if (c.pen < 0) amigaRendererDrawAsset(&renderer, pack, end, c.id, c.frame, c.x, c.y, 0);
                else amigaRendererDrawAssetPen(&renderer, pack, end, c.id, c.frame, c.x, c.y, 0, c.pen);
            }
        }
        renderer = saved;
        for (unsigned i = 0; i < size; ++i) { previous[i] = commands[i]; bounds[i] = next[i]; }
        count = size; initialized = true;
        return true;
    }
};
#endif
