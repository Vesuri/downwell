#include "AmigaMemory.h"

#include <exec/memory.h>
#include <proto/exec.h>

AmigaMemory::AmigaMemory()
    : game_(0), hud_(0), copper_(0), nullSprite_(0), bulletSprite_(0),
      assets_(0),
      gameBytes_(0), hudBytes_(0), copperBytes_(0),
      playerSpriteBytes_(0), bulletSpriteBytes_(0), assetBytes_(0)
{
    playerSprite_[0] = 0;
    playerSprite_[1] = 0;
}

AmigaMemory::~AmigaMemory()
{
    release();
}

bool AmigaMemory::allocate(ULONG gameBytes, ULONG hudBytes,
                           ULONG copperBytes, ULONG playerSpriteBytes,
                           ULONG bulletSpriteBytes, ULONG assetBytes)
{
    release();
    gameBytes_ = gameBytes;
    hudBytes_ = hudBytes;
    copperBytes_ = copperBytes;
    playerSpriteBytes_ = playerSpriteBytes;
    bulletSpriteBytes_ = bulletSpriteBytes;
    assetBytes_ = assetBytes;
    game_ = (UBYTE*)AllocMem(gameBytes_, MEMF_CHIP | MEMF_CLEAR);
    hud_ = (UBYTE*)AllocMem(hudBytes_, MEMF_CHIP | MEMF_CLEAR);
    copper_ = (UWORD*)AllocMem(copperBytes_, MEMF_CHIP | MEMF_CLEAR);
    nullSprite_ = (UWORD*)AllocMem(8, MEMF_CHIP | MEMF_CLEAR);
    playerSprite_[0] =
        (UWORD*)AllocMem(playerSpriteBytes_, MEMF_CHIP | MEMF_CLEAR);
    playerSprite_[1] =
        (UWORD*)AllocMem(playerSpriteBytes_, MEMF_CHIP | MEMF_CLEAR);
    bulletSprite_ =
        (UWORD*)AllocMem(bulletSpriteBytes_, MEMF_CHIP | MEMF_CLEAR);
    assets_ = assetBytes_ != 0
                  ? (UBYTE*)AllocMem(assetBytes_, MEMF_CHIP) : 0;
    if (game_ && hud_ && copper_ && nullSprite_ &&
        playerSprite_[0] && playerSprite_[1] && bulletSprite_ &&
        (assetBytes_ == 0 || assets_))
        return true;
    release();
    return false;
}

void AmigaMemory::release()
{
    if (assets_) FreeMem(assets_, assetBytes_);
    if (bulletSprite_) FreeMem(bulletSprite_, bulletSpriteBytes_);
    if (playerSprite_[1]) FreeMem(playerSprite_[1], playerSpriteBytes_);
    if (playerSprite_[0]) FreeMem(playerSprite_[0], playerSpriteBytes_);
    if (nullSprite_) FreeMem(nullSprite_, 8);
    if (copper_) FreeMem(copper_, copperBytes_);
    if (hud_) FreeMem(hud_, hudBytes_);
    if (game_) FreeMem(game_, gameBytes_);
    game_ = 0;
    hud_ = 0;
    copper_ = 0;
    nullSprite_ = 0;
    playerSprite_[0] = 0;
    playerSprite_[1] = 0;
    bulletSprite_ = 0;
    assets_ = 0;
}
