#include "AmigaMemory.h"

#include <exec/memory.h>
#include <proto/exec.h>

AmigaMemory::AmigaMemory()
    : game_(0), hud_(0), copper_(0), nullSprite_(0),
      gameBytes_(0), hudBytes_(0), copperBytes_(0), spriteBytes_(0)
{
    playerSprite_[0] = 0;
    playerSprite_[1] = 0;
}

AmigaMemory::~AmigaMemory()
{
    release();
}

bool AmigaMemory::allocate(ULONG gameBytes, ULONG hudBytes,
                           ULONG copperBytes, ULONG spriteBytes)
{
    release();
    gameBytes_ = gameBytes;
    hudBytes_ = hudBytes;
    copperBytes_ = copperBytes;
    spriteBytes_ = spriteBytes;
    game_ = (UBYTE*)AllocMem(gameBytes_, MEMF_CHIP | MEMF_CLEAR);
    hud_ = (UBYTE*)AllocMem(hudBytes_, MEMF_CHIP | MEMF_CLEAR);
    copper_ = (UWORD*)AllocMem(copperBytes_, MEMF_CHIP | MEMF_CLEAR);
    nullSprite_ = (UWORD*)AllocMem(8, MEMF_CHIP | MEMF_CLEAR);
    playerSprite_[0] = (UWORD*)AllocMem(spriteBytes_, MEMF_CHIP | MEMF_CLEAR);
    playerSprite_[1] = (UWORD*)AllocMem(spriteBytes_, MEMF_CHIP | MEMF_CLEAR);
    if (game_ && hud_ && copper_ && nullSprite_ &&
        playerSprite_[0] && playerSprite_[1])
        return true;
    release();
    return false;
}

void AmigaMemory::release()
{
    if (playerSprite_[1]) FreeMem(playerSprite_[1], spriteBytes_);
    if (playerSprite_[0]) FreeMem(playerSprite_[0], spriteBytes_);
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
}
