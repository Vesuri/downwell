#include "AmigaSelectionLayer.h"
#include "framework/Bitmap.h"

namespace {
unsigned word(const UBYTE* p) { return (unsigned(p[0]) << 8) | p[1]; }
unsigned long longword(const UBYTE* p) { return (static_cast<unsigned long>(word(p)) << 16) | word(p+2); }
const UBYTE* find(const UBYTE* pack, const UBYTE* end, unsigned id, unsigned frame) {
    if (!pack || !end || end - pack < 20 || longword(pack) != 0x44574150UL || word(pack+4) != 2) return nullptr;
    unsigned first = 0, last = longword(pack+8), directory = word(pack+6);
    if (directory > unsigned(end-pack) || last > (unsigned(end-pack)-directory)/24) return nullptr;
    const unsigned long key = (static_cast<unsigned long>(id) << 16) | frame;
    while (first < last) {
        const unsigned middle = first + (last-first)/2;
        const auto* record = pack + directory + middle*24;
        if (longword(record) < key) first = middle+1;
        else if (longword(record) > key) last = middle;
        else return record;
    }
    return nullptr;
}
}

bool AmigaSelectionLayer::render(UBYTE* target, const UBYTE* pack, const UBYTE* end,
    const Command* commands, unsigned count, unsigned& paintedPixels) {
    paintedPixels = 0;
    if (!target || count > capacity || (count && !commands)) return false;
    const UBYTE* records[capacity];
    for (unsigned i = 0; i < count; ++i) {
        const auto& c = commands[i];
        if (c.id > 65535 || c.frame > 65535 || c.pen < -1 || c.pen > 3) return false;
        const auto* r = records[i] = find(pack, end, c.id, c.frame);
        if (!r || word(r+14) != 3) return false;
        const unsigned w = word(r+8), h = word(r+10), dma = word(r+12);
        const unsigned offset = longword(r+16), length = longword(r+20);
        if (!w || !h || dma < w || (dma & 15) || offset > unsigned(end-pack) ||
            length > unsigned(end-pack)-offset || length != (dma/8)*h*3u || length/3 > 65535) return false;
        auto& b = next_[i];
        b = {c.x - static_cast<short>(word(r+4)), c.y - static_cast<short>(word(r+6)), int(w), int(h)};
        const int right = b.x+b.width < int(width) ? b.x+b.width : int(width);
        const int bottom = b.y+b.height < int(height) ? b.y+b.height : int(height);
        if (b.x < 0) b.x = 0;
        if (b.y < 0) b.y = 0;
        b.width = right > b.x ? right-b.x : 0;
        b.height = bottom > b.y ? bottom-b.y : 0;
    }
    unsigned dirtyCount = 0;
    const auto add = [&](Rect r) {
        if (!r.width || !r.height) return;
        for (unsigned i = 0; i < dirtyCount;) {
            if (!r.overlaps(dirty_[i])) { ++i; continue; }
            const auto old = dirty_[i];
            const int right = r.x+r.width > old.x+old.width ? r.x+r.width : old.x+old.width;
            const int bottom = r.y+r.height > old.y+old.height ? r.y+r.height : old.y+old.height;
            if (old.x < r.x) r.x = old.x;
            if (old.y < r.y) r.y = old.y;
            r.width = right-r.x; r.height = bottom-r.y;
            dirty_[i] = dirty_[--dirtyCount]; i = 0;
        }
        dirty_[dirtyCount++] = r;
    };
    Bitmap destination(target, width, height, 2, false, false, dmaWidth);
    const bool fresh = !initialized_ || target_ != target || pack_ != pack || end_ != end;
    if (fresh) {
        destination.clear(0, 0, dmaWidth, height);
        add({0, 0, int(width), int(height)});
    } else for (unsigned i = 0; i < count_ || i < count; ++i) {
        if (i < count_ && i < count && previous_[i] == commands[i]) continue;
        if (i < count_) add(bounds_[i]);
        if (i < count) add(next_[i]);
    }
    for (unsigned i = 0; i < dirtyCount; ++i) {
        const auto& d = dirty_[i];
        // Bitmap::clear is word-aligned; partial damage needs its masked
        // rectangle helper so adjacent retained pixels are not erased.
        if (!fresh) destination.solidRect(d.x, d.y, d.width, d.height, 0);
        paintedPixels += unsigned(d.width)*unsigned(d.height);
        for (unsigned j = 0; j < count; ++j) {
            const auto& b = next_[j];
            if (!d.overlaps(b)) continue;
            const auto& c = commands[j]; const auto* r = records[j];
            const int x = d.x > b.x ? d.x : b.x, y = d.y > b.y ? d.y : b.y;
            const int right = d.x+d.width < b.x+b.width ? d.x+d.width : b.x+b.width;
            const int bottom = d.y+d.height < b.y+b.height ? d.y+d.height : b.y+b.height;
            const int sx = x - (c.x - static_cast<short>(word(r+4)));
            const int sy = y - (c.y - static_cast<short>(word(r+6)));
            Bitmap source(const_cast<UBYTE*>(pack+longword(r+16)), word(r+8), word(r+10), 2, false, false, word(r+12));
            Bitmap mask(static_cast<UBYTE*>(source.data)+source.bitplaneSizeInBytes*2,
                        source.width, source.height, 1, false, false, source.dataWidth);
            if (c.pen < 0) destination.copyWithMask(source, mask, x,y,sx,sy,sx,sy,right-x,bottom-y);
            else for (unsigned plane = 0; plane < 2; ++plane) {
                Bitmap output(target+plane*planeBytes, width, height, 1, false, false, dmaWidth);
                output.patternWithMask(mask, x,y,sx,sy,right-x,bottom-y,(c.pen & (1u<<plane)) ? 0xffff : 0);
            }
        }
    }
    for (unsigned i = 0; i < count; ++i) { previous_[i] = commands[i]; bounds_[i] = next_[i]; }
    count_ = count; target_ = target; pack_ = pack; end_ = end; initialized_ = true;
    return true;
}
