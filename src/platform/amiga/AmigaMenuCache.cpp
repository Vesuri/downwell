#include "AmigaMenuCache.h"
#include "AmigaMenu.h"
#include "AmigaMenuScroll.h"
#include "framework/AmigaHardware.h"
#include "framework/Bitmap.h"

namespace {
UWORD word(const UBYTE* p) { return (UWORD)(((UWORD)p[0] << 8) | p[1]); }
ULONG longword(const UBYTE* p) { return ((ULONG)word(p) << 16) | word(p + 2); }

const UBYTE* find(const UBYTE* pack, const UBYTE* end, UWORD sprite, UWORD frame) {
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

bool drawMask(Bitmap& destination, const UBYTE* pack, const UBYTE* end,
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
    if (left + drawWidth > destination.width) drawWidth = destination.width - left;
    if (top + drawHeight > destination.height) drawHeight = destination.height - top;
    if (drawWidth <= 0 || drawHeight <= 0) return true;
    Bitmap mask((void*)(data + planeBytes * 2), width, height, 1,
                false, false, dataWidth);
    destination.patternWithMask(mask, (UWORD)left, (UWORD)top,
                                (UWORD)sourceX, (UWORD)sourceY,
                                (UWORD)drawWidth, (UWORD)drawHeight, 0xffff);
    return true;
}
}

bool amigaRenderMenuCache(const AmigaMenuDrawList& list,
                          const UBYTE* pack, const UBYTE* packEnd,
                          UBYTE* foreground, UBYTE* background,
                          UBYTE* entry) {
    Bitmap foregroundBitmap(foreground, amigaMenuCacheWidth,
                            amigaMenuCacheHeight, 1, false);
    Bitmap backgroundBitmap(background, amigaMenuCacheWidth,
                            amigaMenuCacheHeight, 1, false);
    Bitmap entryBitmap(entry, amigaMenuEntryWidth,
                       amigaMenuEntryHeight, 1, false);
    for (unsigned i = 0; i < list.count; ++i) {
        const AmigaMenuDrawItem& item = list.items[i];
        if (item.objectId == 312) {
            if (!drawMask(backgroundBitmap, pack, packEnd,
                          (UWORD)item.spriteId, (UWORD)item.frame, 120, 129))
                return false;
        } else {
            if (!drawMask(foregroundBitmap, pack, packEnd,
                          (UWORD)item.spriteId, (UWORD)item.frame,
                          item.x, item.y - amigaMenuCacheWorldY) ||
                !drawMask(entryBitmap, pack, packEnd,
                          (UWORD)item.spriteId, (UWORD)item.frame,
                          item.x - amigaMenuEntryWorldX, item.y))
                return false;
        }
    }
    AmigaHardware::blitterDrain();
    return true;
}
