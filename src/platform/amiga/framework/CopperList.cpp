#include "CopperList.h"
#include "AmigaHardware.h"
#include "Bitmap.h"
#include "Palette.h"
#include "Sprite.h"

CopperList::CopperList(UWORD* words, unsigned capacityWords)
    : words_(words), capacity_(capacityWords), count_(0), valid_(words != 0)
{
}

void CopperList::reset()
{
    count_ = 0;
    valid_ = words_ != 0;
}

bool CopperList::move(UWORD reg, UWORD value)
{
    if (!valid_ || count_ + 2 > capacity_) {
        valid_ = false;
        return false;
    }
    words_[count_++] = reg;
    words_[count_++] = value;
    return true;
}

bool CopperList::pointer(UWORD reg, const void* value)
{
    const ULONG address = (ULONG)value;
    return move(reg, (UWORD)(address >> 16)) &&
           move((UWORD)(reg + 2), (UWORD)address);
}

bool CopperList::wait(UWORD vertical, UWORD horizontal)
{
    if (!valid_ || count_ + 2 > capacity_) {
        valid_ = false;
        return false;
    }
    words_[count_++] = (UWORD)(((vertical & 0xff) << 8) |
                               (horizontal & 0xfe) | 1);
    words_[count_++] = 0xfffe;
    return true;
}

bool CopperList::showBitmap(const Bitmap& bitmap, UWORD firstBitplane,
                            UWORD bitplaneNumberDelta,
                            int xOffset, int yOffset,
                            UWORD bitplaneCount)
{
    ULONG address = (ULONG)bitmap.data;
    if (xOffset) address += xOffset >> 3;
    if (yOffset) address += yOffset * bitmap.rowSizeInBytes;
    if (!bitplaneCount) bitplaneCount = bitmap.bitplanes;

    UWORD reg = (UWORD)(bpl1pth + ((firstBitplane - 1) << 2));
    for (UWORD plane = 0; plane < bitplaneCount; ++plane) {
        if (!pointer(reg, (const void*)address)) return false;
        reg = (UWORD)(reg + (bitplaneNumberDelta << 2));
        address += bitmap.interleaved
            ? bitmap.widthInBytes : bitmap.bitplaneSizeInBytes;
    }
    return true;
}

bool CopperList::showSprite(UWORD spriteNumber, const Sprite& sprite)
{
    return pointer((UWORD)(spr1pth + (spriteNumber << 2)), sprite.data());
}

bool CopperList::setPalette(const Palette& palette, UWORD colorIndex,
                            int fromIndex, int toIndex)
{
    if (toIndex < 0) toIndex = (int)palette.colorCount() - 1;
    for (int index = fromIndex; index <= toIndex; ++index) {
        if (!move((UWORD)(color00 + ((colorIndex + index - fromIndex) << 1)),
                  palette[index]))
            return false;
    }
    return true;
}

bool CopperList::finish()
{
    if (!valid_ || count_ + 2 > capacity_) {
        valid_ = false;
        return false;
    }
    words_[count_++] = 0xffff;
    words_[count_++] = 0xfffe;
    return true;
}
