#ifndef DOWNWELL_AMIGA_COPPER_LIST_H
#define DOWNWELL_AMIGA_COPPER_LIST_H

#include <exec/types.h>

class Bitmap;
class Palette;
class Sprite;

class CopperList {
public:
    CopperList(UWORD* words, unsigned capacityWords);

    void reset();
    bool move(UWORD reg, UWORD value);
    bool pointer(UWORD reg, const void* value);
    bool wait(UWORD vertical, UWORD horizontal);
    bool showBitmap(const Bitmap& bitmap, UWORD firstBitplane = 1,
                    UWORD bitplaneNumberDelta = 1,
                    int xOffset = 0, int yOffset = 0,
                    UWORD bitplaneCount = 0);
    bool showSprite(UWORD spriteNumber, const Sprite& sprite);
    bool setPalette(const Palette& palette, UWORD colorIndex = 0,
                    int fromIndex = 0, int toIndex = -1);
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
