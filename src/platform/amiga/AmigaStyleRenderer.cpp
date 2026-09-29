#include "AmigaStyleRenderer.h"
#include "AmigaSceneRuntime.h"
#include "AmigaProfile.h"
#include "framework/AmigaHardware.h"
#include "framework/Bitmap.h"
inline void* operator new(__SIZE_TYPE__, void* address) noexcept { return address; }

namespace {

UWORD word(const UBYTE* p) { return (UWORD)(((UWORD)p[0] << 8) | p[1]); }
ULONG longword(const UBYTE* p) { return ((ULONG)word(p) << 16) | word(p + 2); }

const UBYTE* find(const UBYTE* pack, const UBYTE* end,
                  UWORD sprite, UWORD frame) {
    if (end - pack < 20 || longword(pack) != 0x44574150UL) return 0;
    const UBYTE* records = pack + word(pack + 6);
    ULONG first = 0, last = longword(pack + 8);
    const ULONG wanted = ((ULONG)sprite << 16) | frame;
    if (records < pack || records + last * 24 > end) return 0;
    while (first < last) {
        const ULONG middle = first + (last - first) / 2;
        const UBYTE* record = records + middle * 24;
        const ULONG key = longword(record);
        if (key < wanted) first = middle + 1;
        else if (key > wanted) last = middle;
        else return record;
    }
    return 0;
}

struct PreparedStyleArt {
    int originX, originY;
    Bitmap source, mask;
    PreparedStyleArt(const UBYTE* pack, const UBYTE* record)
        : originX((WORD)word(record + 4)), originY((WORD)word(record + 6)),
          source((void*)(pack + longword(record + 16)), word(record + 8),
                 word(record + 10), 2, false, false, word(record + 12)),
          mask((UBYTE*)source.data + source.bitplaneSizeInBytes * 2,
               source.width, source.height, 1, false, false, source.dataWidth) {}
};
alignas(PreparedStyleArt) UBYTE styleArtStorage[11 * sizeof(PreparedStyleArt)];
AmigaStyleFrameState* activeStyleState;

bool drawPrepared(Bitmap& destination, unsigned index, int x, int y) {
    PreparedStyleArt& art = *reinterpret_cast<PreparedStyleArt*>(styleArtStorage + index * sizeof(PreparedStyleArt));
    int left = x - art.originX, top = y - art.originY;
    int sx = 0, sy = 0, width = art.source.width, height = art.source.height;
    if (left < 0) { sx = -left; width += left; left = 0; }
    if (top < 0) { sy = -top; height += top; top = 0; }
    if (left + width > amigaStyleVisibleWidth) width = amigaStyleVisibleWidth - left;
    if (top + height > destination.height) height = destination.height - top;
    if (width > 0 && height > 0) {
        destination.copyWithMask(art.source, art.mask, left, top, sx, sy, sx, sy, width, height);
        if (activeStyleState && activeStyleState->dirtyCount < 8) {
            AmigaStyleFrameState::Rect& rect = activeStyleState->dirty[activeStyleState->dirtyCount++];
            rect.x = left; rect.y = top; rect.width = width; rect.height = height;
        }
    }
    return true;
}

bool draw(Bitmap& destination, const UBYTE* pack, const UBYTE* end,
          UWORD sprite, UWORD frame, int anchorX, int anchorY,
          int forcedPen = -1) {
    if (sprite == 65500 && frame < 8) return drawPrepared(destination, frame, anchorX, anchorY);
    if (sprite >= 65501 && sprite <= 65503)
        return drawPrepared(destination, 8 + sprite - 65501, anchorX, anchorY);
    const UBYTE* record = find(pack, end, sprite, frame);
    if (!record) return false;
    const int originX = (WORD)word(record + 4);
    const int originY = (WORD)word(record + 6);
    const int width = word(record + 8);
    const int height = word(record + 10);
    const int dataWidth = word(record + 12);
    const ULONG planeBytes = (ULONG)(dataWidth >> 3) * height;
    const UBYTE* data = pack + longword(record + 16);
    if (data < pack || data + longword(record + 20) > end) return false;
    int left = anchorX - originX, top = anchorY - originY;
    int sourceX = 0, sourceY = 0, drawWidth = width, drawHeight = height;
    if (left < 0) { sourceX = -left; drawWidth += left; left = 0; }
    if (top < 0) { sourceY = -top; drawHeight += top; top = 0; }
    if (left + drawWidth > amigaStyleVisibleWidth)
        drawWidth = amigaStyleVisibleWidth - left;
    if (top + drawHeight > destination.height)
        drawHeight = destination.height - top;
    if (drawWidth <= 0 || drawHeight <= 0) return true;

    {
        Bitmap source((void*)data, width, height, 2, false, false, dataWidth);
        Bitmap mask((void*)(data + planeBytes * 2), width, height, 1,
                    false, false, dataWidth);
        if (forcedPen < 0) {
            destination.copyWithMask(source, mask, (UWORD)left, (UWORD)top,
                                     (UWORD)sourceX, (UWORD)sourceY,
                                     (UWORD)sourceX, (UWORD)sourceY,
                                     (UWORD)drawWidth, (UWORD)drawHeight);
        } else {
            Bitmap plane0(destination.data, destination.width,
                          destination.height, 1, false, false,
                          destination.dataWidth);
            plane0.patternWithMask(mask, (UWORD)left, (UWORD)top,
                                   (UWORD)sourceX, (UWORD)sourceY,
                                   (UWORD)drawWidth, (UWORD)drawHeight,
                                   forcedPen & 1 ? 0xffff : 0);
        }
        return true;
    }

}

} // namespace

bool amigaBuildStyleBorder(const UBYTE* pack, const UBYTE* packEnd,
                           UBYTE* border) {
    for (unsigned i = 0; i < 11; ++i) {
        const UBYTE* record = find(pack, packEnd, i < 8 ? 65500 : 65501 + i - 8, i < 8 ? i : 0);
        if (!record || longword(record + 16) > (ULONG)(packEnd - pack) ||
            longword(record + 20) > (ULONG)(packEnd - pack) - longword(record + 16)) return false;
        new (styleArtStorage + i * sizeof(PreparedStyleArt)) PreparedStyleArt(pack, record);
    }
    Bitmap bitmap(border, amigaStyleWidth, amigaStyleBorderHeight, 2, false);
    const UBYTE* left = find(pack,packEnd,51,13);
    const UBYTE* right = find(pack,packEnd,51,7);
    const bool repeat = left && right && word(left+8)==16 && word(right+8)==16 &&
        word(left+10)==16 && word(right+10)==16 && word(left+6)==8 && word(right+6)==8;
    bitmap.clear(0,0,amigaStyleWidth,repeat?16:amigaStyleBorderHeight);
    // The source anchors split a tile at row zero. Two anchor rows establish a
    // complete 16-row period; all remaining rows are exact planar copies.
    // Keep the general path if replacement resources change that geometry.
    const int last = repeat ? 1 : amigaStyleBorderHeight / 16 + 1;
    for (int row = 0; row <= last; ++row) {
        if (!draw(bitmap, pack, packEnd, 51, 13, 0, row * 16) ||
            !draw(bitmap, pack, packEnd, 51, 7,
                  amigaStyleVisibleWidth, row * 16))
            return false;
    }
    if(repeat)for(unsigned filled=16;filled<amigaStyleBorderHeight;){
        const unsigned remaining=amigaStyleBorderHeight-filled;
        const unsigned rows=filled<remaining?filled:remaining;
        // Disjoint source/destination ranges; queued blits preserve order.
        bitmap.copy(bitmap,0,filled,0,0,amigaStyleWidth,rows);
        filled+=rows;
    }
    AmigaHardware::blitterDrain();
    // Immutable PF2 opacity masks: transparent/gradient/solid for entry,
    // solid/gradient/transparent for exit. Only DMA pointers move per frame.
    UBYTE* masks = border + amigaStyleBorderBytes;
    for (unsigned exit = 0; exit < 2; ++exit) {
        const UBYTE* record = find(pack, packEnd, 65004 + exit, 0);
        if (!record || word(record + 8) != 160 || word(record + 10) != 80)
            return false;
        const unsigned stride = word(record + 12) / 8;
        const ULONG offset = longword(record + 16);
        if (offset > (ULONG)(packEnd - pack) ||
            stride * 80UL > (ULONG)(packEnd - pack) - offset) return false;
        // The edge is already planar in the offline pack. Stage its three
        // disjoint regions through the shared Bitmap backend, not 28 KB of
        // byte-at-a-time CPU writes to Chip RAM at the scene boundary.
        Bitmap mask(masks + exit * amigaStyleMaskBytes, amigaStyleWidth,
                    amigaStyleMaskHeight, 1, false);
        Bitmap edge((void*)(pack + offset), 160, 80, 1, false, false, stride * 8);
        mask.solidRect(0, 0, amigaStyleWidth, amigaStyleHeight, exit ? 1 : 0);
        mask.copy(edge, 0, amigaStyleHeight, 0, 0, 160, 80);
        mask.clear(160, amigaStyleHeight, amigaStyleWidth - 160, 80);
        mask.solidRect(0, amigaStyleHeight + 80, amigaStyleWidth,
                       amigaStyleHeight, exit ? 0 : 1);
    }
    AmigaHardware::blitterDrain(); // No in-flight writer when DMA is published.
    return true;
}

const UBYTE* amigaStyleMask(const UBYTE* cache, const AmigaSceneRuntime& scenes) {
    const bool exit = scenes.styleSelected();
    const int top = exit ? scenes.styleVerticalDitherY() : scenes.styleDitherY() - 400;
    int row = amigaStyleHeight - top;
    if (row < 0) row = 0;
    if (row > amigaStyleHeight + 80) row = amigaStyleHeight + 80;
    return cache + amigaStyleMaskOffset + (exit ? amigaStyleMaskBytes : 0) +
           row * amigaStyleBytesPerRow;
}

bool amigaRenderStyleFrame(const UBYTE* pack, const UBYTE* packEnd,
                           const UBYTE* border,
                           const AmigaSceneRuntime& scenes,
                           UBYTE* destination, AmigaStyleFrameState& state
#ifdef DOWNWELL_SCENE_VERIFY
                           , bool reference
#endif
                           ) {
#ifndef DOWNWELL_SCENE_VERIFY
    const bool reference = false;
#endif
    Bitmap output(destination, amigaStyleWidth, amigaStyleHeight, 2, false);
    Bitmap borderBitmap((void*)border, amigaStyleWidth,
                        amigaStyleBorderHeight, 2, false);
    const int borderY = -scenes.styleTileY();
    const int centerY = amigaStyleHeight / 2 + scenes.styleYOffset();
    const int smoothX = scenes.styleSmoothX();
    const int textPanel = scenes.styleCursor() == 0 ? 0 : 1;
    const bool hardwareBorder = !reference;
    const bool full = reference || !state.initialized || state.centerY != centerY || scenes.styleSelected() || state.hardwareBorder != hardwareBorder;
    const bool actorsChanged = full || state.smoothX != smoothX ||
        state.cursor != scenes.styleCursor() ||
        (scenes.styleCursor() == 0 && state.runFrame != scenes.styleRunFrame());
    DW_PROFILE_PHASE(profileStyleCopy);
    if (full) {
        if (hardwareBorder) output.clear();
        else output.copy(borderBitmap, 0, 0, 0, (UWORD)borderY,
                         amigaStyleWidth, amigaStyleHeight);
    } else if (hardwareBorder && actorsChanged) {
        // Restore only pixels occupied in THIS back buffer. Border pixels
        // are not stored here: PF1 supplies them through its DMA pointers.
        for (unsigned i = 0; i < state.dirtyCount; ++i) {
            const AmigaStyleFrameState::Rect& rect = state.dirty[i];
            output.clear(rect.x, rect.y, rect.width, rect.height);
        }
    }
    if (actorsChanged || !hardwareBorder) state.dirtyCount = 0;
    if (!scenes.styleSelected() && !reference) {
        const UBYTE* record = find(pack, packEnd, 65410 + textPanel, 0);
        if (!record) return false;
        Bitmap panel((void*)(pack + longword(record + 16)), 160, 128, 2, false);
        int top = centerY - 80;
        int sourceY = top < 0 ? -top : 0;
        int height = (full || state.textPanel != textPanel ? 128 : 88) - sourceY;
        if (top < 0) top = 0;
        if (hardwareBorder && !full) {
            sourceY = 90; top = centerY + 10;
            height = state.textPanel != textPanel ? 38 : 0;
        }
        if (top + height > amigaStyleHeight) height = amigaStyleHeight - top;
        if (height > 0)
            output.copy(panel, 0, top, 0, sourceY, 160, height);
        // Only the side strips scroll. The panel restore leaves the settled
        // lower text intact and removes all previous actors/arrows in this
        // back buffer, including partially clipped icons at either edge.
        if (!hardwareBorder) {
            output.copy(borderBitmap, 0, 0, 0, borderY, 16, amigaStyleHeight);
            output.copy(borderBitmap, 144, 0, 144, borderY, 16, amigaStyleHeight);
        }
    }

    activeStyleState = hardwareBorder ? &state : 0;
    DW_PROFILE_PHASE(profileStyleActors);
    if (!scenes.styleSelected() && (!hardwareBorder || actorsChanged)) {
        for (int style = 0; style < 5; ++style) {
            const int drawX = 80 + style * 48 - smoothX;
            if (drawX <= -16 || drawX >= 176) continue;
            const int distance = drawX - 80;
            const int drawY = centerY - 14 - distance * distance / 200;
            const bool unlocked = style == 0;
            if (!draw(output, pack, packEnd, unlocked ? (reference ? 27 : 65500) : (reference ? 43 : 65501),
                      unlocked && style == scenes.styleCursor()
                          ? (UWORD)scenes.styleRunFrame() : 0,
                      drawX, drawY))
                return false;
        }
        if (scenes.styleCursor() < 4 &&
            !draw(output, pack, packEnd, reference ? 44 : 65502, 0, 96, centerY - 10))
            return false;
        if (scenes.styleCursor() > 0 &&
            !draw(output, pack, packEnd, reference ? 65401 : 65503, 0, 64, centerY - 10))
            return false;
        DW_PROFILE_PHASE(profileStyleText);
#ifdef DOWNWELL_SCENE_VERIFY
        if (reference) {
            if (!draw(output, pack, packEnd, 65000, 0, 80, centerY - 70) ||
                !draw(output, pack, packEnd, textPanel ? 65003 : 65001, 0, 80, centerY + 10) ||
                (!textPanel && !draw(output, pack, packEnd, 65002, 0, 80, centerY + 32, 1)))
                return false;
        }
#endif
    } else if (scenes.styleSelected() && !draw(output, pack, packEnd, reference ? 27 : 65500,
                     (UWORD)scenes.styleRunFrame(), 80,
                     centerY - 14 + scenes.stylePlayerDescent())) {
        return false;
    }

    DW_PROFILE_PHASE(profileStyleDrain);
    AmigaHardware::blitterDrain();
    activeStyleState = 0;
    state.initialized = true;
    state.centerY = centerY;
    state.textPanel = textPanel;
    state.hardwareBorder = hardwareBorder;
    state.smoothX = smoothX;
    state.cursor = scenes.styleCursor();
    state.runFrame = scenes.styleRunFrame();
    return true;
}
