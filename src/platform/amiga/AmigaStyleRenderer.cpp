#include "AmigaStyleRenderer.h"
#include "AmigaSceneRuntime.h"
#include "framework/AmigaHardware.h"
#include "framework/Bitmap.h"

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
    return true;
}

void clearRect(Bitmap& bitmap, int x, int y, int width, int height) {
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > amigaStyleVisibleWidth) width = amigaStyleVisibleWidth - x;
    if (y + height > amigaStyleHeight) height = amigaStyleHeight - y;
    if (width > 0 && height > 0)
        bitmap.clear((UWORD)x, (UWORD)y, (UWORD)width, (UWORD)height);
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
    return true;
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
    output.copy(borderBitmap, 0, 0, 0, (UWORD)borderY,
                amigaStyleWidth, amigaStyleHeight);

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

    const int enterTop = scenes.styleDitherY() - 400;
    clearRect(output, 0, enterTop, amigaStyleVisibleWidth, 800);
    if (!draw(output, pack, packEnd, 65004, 0, 0, enterTop))
        return false;
    if (scenes.styleSelected()) {
        const int verticalEdge = scenes.styleVerticalDitherY();
        clearRect(output, 0, verticalEdge - 720,
                  amigaStyleVisibleWidth, 800);
        if (!draw(output, pack, packEnd, 65005, 0, 0, verticalEdge))
            return false;
    }
    AmigaHardware::blitterDrain();
    return true;
}
