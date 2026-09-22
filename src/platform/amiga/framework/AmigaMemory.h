#ifndef DOWNWELL_AMIGA_MEMORY_H
#define DOWNWELL_AMIGA_MEMORY_H

#include <exec/types.h>

class AmigaMemory {
public:
    AmigaMemory();
    ~AmigaMemory();

    bool allocate(ULONG gameBytes, ULONG hudBytes,
                  ULONG copperBytes, ULONG playerSpriteBytes,
                  ULONG bulletSpriteBytes, ULONG assetBytes = 0);
    void release();

    UBYTE* game() const { return game_; }
    UBYTE* hud() const { return hud_; }
    UWORD* copper() const { return copper_; }
    UWORD* nullSprite() const { return nullSprite_; }
    UWORD* playerSprite(unsigned index) const {
        return index < 2 ? playerSprite_[index] : 0;
    }
    UWORD* bulletSprite() const { return bulletSprite_; }
    UBYTE* assets() const { return assets_; }

private:
    AmigaMemory(const AmigaMemory&);
    AmigaMemory& operator=(const AmigaMemory&);

    UBYTE* game_;
    UBYTE* hud_;
    UWORD* copper_;
    UWORD* nullSprite_;
    UWORD* playerSprite_[2];
    UWORD* bulletSprite_;
    UBYTE* assets_;
    ULONG gameBytes_;
    ULONG hudBytes_;
    ULONG copperBytes_;
    ULONG playerSpriteBytes_;
    ULONG bulletSpriteBytes_;
    ULONG assetBytes_;
};

#endif
