#include "AmigaSplashCache.h"
#include "framework/AmigaHardware.h"
#include "framework/Bitmap.h"

namespace {

enum {
    blankFrame,
    devolverFrame,
    controlsFrame
};

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
          UWORD sprite, UWORD frame, int anchorX, int anchorY) {
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
    if (left + drawWidth > destination.width)
        drawWidth = destination.width - left;
    if (top + drawHeight > destination.height)
        drawHeight = destination.height - top;
    if (drawWidth <= 0 || drawHeight <= 0) return true;
    Bitmap source((void*)data, width, height, 2, false, false, dataWidth);
    Bitmap mask((void*)(data + planeBytes * 2), width, height, 1,
                false, false, dataWidth);
    destination.copyWithMask(source, mask, (UWORD)left, (UWORD)top,
                             (UWORD)sourceX, (UWORD)sourceY,
                             (UWORD)sourceX, (UWORD)sourceY,
                             (UWORD)drawWidth, (UWORD)drawHeight);
    return true;
}

bool buildDitherTiles(UBYTE* destination, const UBYTE* pack,
                      const UBYTE* end) {
    for (unsigned frame = 0; frame < amigaSplashDitherFrameCount; ++frame) {
        const UBYTE* record = find(pack, end, 733, (UWORD)frame);
        if (!record || word(record + 8) != 8 || word(record + 10) != 8 ||
            word(record + 12) != 16)
            return false;
        const UBYTE* data = pack + longword(record + 16);
        const ULONG dataBytes = longword(record + 20);
        if (data < pack || data + dataBytes > end || dataBytes < 48)
            return false;
        const UBYTE* sourceMask = data + 32;
        UBYTE* tile = destination + frame * amigaSplashDitherTileBytes;
        for (unsigned y = 0; y < amigaSplashDitherTileHeight; ++y) {
            const UBYTE mask = sourceMask[y * 2];
            UBYTE* row = tile + y * amigaSplashBytesPerRow;
            for (unsigned x = 0; x < amigaSplashVisibleWidth / 8; ++x)
                row[x] = mask;
            row[20] = 0;
            row[21] = 0;
        }
    }
    return true;
}

} // namespace

bool amigaBuildSplashCache(const UBYTE* pack, const UBYTE* packEnd,
                           UBYTE* cache, UBYTE* wideCredits) {
    for (int frame = 0; frame < amigaSplashFrameCount; ++frame) {
        Bitmap bitmap(cache + frame * amigaSplashFrameBytes,
                      amigaSplashWidth, amigaSplashHeight, 2, false);
        bitmap.clear();
    }
    Bitmap devolver(cache + devolverFrame * amigaSplashFrameBytes,
                    amigaSplashWidth, amigaSplashHeight, 2, false);
    Bitmap controls(cache + controlsFrame * amigaSplashFrameBytes,
                    amigaSplashWidth, amigaSplashHeight, 2, false);
    Bitmap credits(wideCredits, 380, amigaSplashHeight, 2, false, false, 384);
    credits.clear();
    if (!draw(devolver, pack, packEnd, 696, 0, 80, 140) ||
        !draw(controls, pack, packEnd, 695, 0, 80, 140) ||
        !draw(credits, pack, packEnd, 65200, 0, 190, 130) ||
        !draw(credits, pack, packEnd, 65201, 0, 190, 150) ||
        !draw(credits, pack, packEnd, 65202, 0, 278, 200) ||
        !draw(credits, pack, packEnd, 65203, 0, 278, 210) ||
        !draw(credits, pack, packEnd, 65204, 0, 102, 200) ||
        !draw(credits, pack, packEnd, 65205, 0, 102, 210))
        return false;
    AmigaHardware::blitterDrain();
    return buildDitherTiles(
        cache + amigaSplashFrameBytes * amigaSplashFrameCount,
        pack, packEnd);
}

const UBYTE* amigaSplashFrame(const UBYTE* cache, int stage) {
    unsigned frame = blankFrame;
    if (stage == 2) frame = devolverFrame;
    else if (stage == 4 || stage == 5) frame = controlsFrame;
    return cache + frame * amigaSplashFrameBytes;
}

const UBYTE* amigaSplashDitherTile(const UBYTE* cache, unsigned frame) {
    if (frame >= amigaSplashDitherFrameCount)
        frame = amigaSplashDitherFrameCount - 1;
    return cache + amigaSplashFrameBytes * amigaSplashFrameCount +
           frame * amigaSplashDitherTileBytes;
}
