#ifndef DOWNWELL_AMIGA_MEMORY_H
#define DOWNWELL_AMIGA_MEMORY_H

#include <exec/types.h>

class AmigaMemory {
public:
    AmigaMemory();
    ~AmigaMemory();

    bool allocate(ULONG gameBytes, ULONG hudBytes,
                  ULONG menuPlaneBytes, ULONG menuEntryBytes,
                  ULONG copperBytes, ULONG playerSpriteBytes,
                  ULONG bulletSpriteBytes, ULONG enemySpriteBytes,
                  ULONG sceneCacheBytes,
                  ULONG assetBytes = 0);
    void release();

    UBYTE* game() const { return game_; }
    UBYTE* hud() const { return hud_; }
    UBYTE* menuForeground() const { return menu_; }
    UBYTE* menuBackground() const { return menu_ + menuPlaneBytes_; }
    UBYTE* menuEntry() const { return menu_ + menuPlaneBytes_ * 2; }
    UWORD* copper() const { return copper_; }
    UWORD* nullSprite() const { return nullSprite_; }
    UWORD* playerSprite(unsigned index) const {
        return index < 2 ? playerSprite_[index] : 0;
    }
    UWORD* bulletSprite() const { return bulletSprite_; }
    UWORD* enemySprite(unsigned index) const {
        return index < 2 ? enemySprite_[index] : 0;
    }
    UBYTE* sceneCache() const { return sceneCache_; }
    UBYTE* assets() const { return assets_; }

private:
    AmigaMemory(const AmigaMemory&);
    AmigaMemory& operator=(const AmigaMemory&);

    UBYTE* game_;
    UBYTE* hud_;
    UBYTE* menu_;
    UWORD* copper_;
    UWORD* nullSprite_;
    UWORD* playerSprite_[2];
    UWORD* bulletSprite_;
    UWORD* enemySprite_[2];
    UBYTE* sceneCache_;
    UBYTE* assets_;
    ULONG gameBytes_;
    ULONG hudBytes_;
    ULONG menuPlaneBytes_;
    ULONG menuEntryBytes_;
    ULONG copperBytes_;
    ULONG playerSpriteBytes_;
    ULONG bulletSpriteBytes_;
    ULONG enemySpriteBytes_;
    ULONG sceneCacheBytes_;
    ULONG assetBytes_;
};

#endif
