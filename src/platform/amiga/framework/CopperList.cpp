#include "CopperList.h"

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
