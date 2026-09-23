#include "AmigaStyleRenderer.h"
#include "AmigaSceneRuntime.h"
#include "AmigaProfile.h"
#include "framework/AmigaHardware.h"
#include "framework/Bitmap.h"
extern "C" {
#include <string.h>
}

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

bool draw(Bitmap& destination, const UBYTE* pack, const UBYTE* end,
          UWORD sprite, UWORD frame, int anchorX, int anchorY,
          bool flipX = false, int forcedPen = -1) {
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

    if (!flipX) {
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

    DW_PROFILE_PHASE(profileStyleMirror);
    UBYTE* output = (UBYTE*)destination.data;
    const ULONG outputPlaneBytes = destination.bitplaneSizeInBytes;
    for (int y = 0; y < drawHeight; ++y) {
        const UBYTE* row = data + (sourceY + y) * (dataWidth >> 3);
        for (int x = 0; x < drawWidth; ++x) {
            const int sourcePixel = width - 1 - (sourceX + x);
            const UBYTE sourceMask = (UBYTE)(0x80u >> (sourcePixel & 7));
            unsigned pen = row[sourcePixel >> 3] & sourceMask ? 1u : 0u;
            pen |= row[planeBytes + (sourcePixel >> 3)] & sourceMask ? 2u : 0u;
            if (!pen) continue;
            const ULONG offset = (ULONG)(top + y) * amigaStyleBytesPerRow +
                                 (ULONG)(left + x) / 8;
            const UBYTE targetMask = (UBYTE)(0x80u >> ((left + x) & 7));
            if (pen & 1) output[offset] |= targetMask;
            if (pen & 2) output[outputPlaneBytes + offset] |= targetMask;
        }
    }
    DW_PROFILE_PHASE(profileStyleActors);
    return true;
}

} // namespace

bool amigaBuildStyleBorder(const UBYTE* pack, const UBYTE* packEnd,
                           UBYTE* border) {
    Bitmap bitmap(border, amigaStyleWidth, amigaStyleBorderHeight, 2, false);
    bitmap.clear();
    for (int row = 0; row <= amigaStyleBorderHeight / 16 + 1; ++row) {
        if (!draw(bitmap, pack, packEnd, 51, 13, 0, row * 16) ||
            !draw(bitmap, pack, packEnd, 51, 7,
                  amigaStyleVisibleWidth, row * 16))
            return false;
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
        UBYTE* mask = masks + exit * amigaStyleMaskBytes;
        memset(mask, exit ? 0xff : 0, amigaStylePlaneBytes);
        for (unsigned y = 0; y < 80; ++y) {
            UBYTE* row = mask + (amigaStyleHeight + y) * amigaStyleBytesPerRow;
            memcpy(row, pack + offset + y * stride, 20);
            memset(row + 20, 0, amigaStyleBytesPerRow - 20);
        }
        memset(mask + (amigaStyleHeight + 80) * amigaStyleBytesPerRow,
               exit ? 0 : 0xff, amigaStylePlaneBytes);
    }
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
                           UBYTE* destination) {
    Bitmap output(destination, amigaStyleWidth, amigaStyleHeight, 2, false);
    Bitmap borderBitmap((void*)border, amigaStyleWidth,
                        amigaStyleBorderHeight, 2, false);
    const int borderY = -scenes.styleTileY();
    const int centerY = amigaStyleHeight / 2 + scenes.styleYOffset();
    const int smoothX = scenes.styleSmoothX();
    DW_PROFILE_PHASE(profileStyleCopy);
    output.copy(borderBitmap, 0, 0, 0, (UWORD)borderY,
                amigaStyleWidth, amigaStyleHeight);

    DW_PROFILE_PHASE(profileStyleActors);
    if (!scenes.styleSelected()) {
        for (int style = 0; style < 5; ++style) {
            const int drawX = 80 + style * 48 - smoothX;
            const int distance = drawX - 80;
            const int drawY = centerY - 14 - distance * distance / 200;
            const bool unlocked = style == 0;
            if (!draw(output, pack, packEnd, unlocked ? 27 : 43,
                      unlocked && style == scenes.styleCursor()
                          ? (UWORD)scenes.styleRunFrame() : 0,
                      drawX, drawY))
                return false;
        }
        if (scenes.styleCursor() < 4 &&
            !draw(output, pack, packEnd, 44, 0, 96, centerY - 10))
            return false;
        if (scenes.styleCursor() > 0 &&
            !draw(output, pack, packEnd, 44, 0, 64, centerY - 10, true))
            return false;
        DW_PROFILE_PHASE(profileStyleText);
        if (!draw(output, pack, packEnd, 65000, 0, 80, centerY - 70) ||
            !draw(output, pack, packEnd,
                  scenes.styleCursor() == 0 ? 65001 : 65003,
                  0, 80, centerY + 10))
            return false;
        if (scenes.styleCursor() == 0 &&
            !draw(output, pack, packEnd, 65002, 0,
                  80, centerY + 32, false, 1))
            return false;
    } else if (!draw(output, pack, packEnd, 27,
                     (UWORD)scenes.styleRunFrame(), 80,
                     centerY - 14 + scenes.stylePlayerDescent())) {
        return false;
    }

    DW_PROFILE_PHASE(profileStyleDrain);
    AmigaHardware::blitterDrain();
    return true;
}
