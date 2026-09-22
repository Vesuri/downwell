#ifndef DOWNWELL_AMIGA_COPPER_LIST_H
#define DOWNWELL_AMIGA_COPPER_LIST_H

#include <exec/types.h>

class CopperList {
public:
    CopperList(UWORD* words, unsigned capacityWords);

    void reset();
    bool move(UWORD reg, UWORD value);
    bool pointer(UWORD reg, const void* value);
    bool finish();
    UWORD* data() const { return words_; }
    unsigned size() const { return count_; }
    bool valid() const { return valid_; }

private:
    UWORD* words_;
    unsigned capacity_;
    unsigned count_;
    bool valid_;
};

#endif
