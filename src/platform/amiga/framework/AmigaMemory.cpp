#include "AmigaMemory.h"

#include <exec/memory.h>
#include <proto/exec.h>

// Debugger-only Chip allocation watcher reads these SDK fields even when
// unused debug types have been removed from the executable's companion ELF.
static_assert(__builtin_offsetof(MemHeader, mh_Attributes) == 14, "Chip audit ABI");
static_assert(__builtin_offsetof(MemHeader, mh_First) == 16, "Chip audit ABI");
static_assert(__builtin_offsetof(MemHeader, mh_Lower) == 20, "Chip audit ABI");
static_assert(__builtin_offsetof(MemHeader, mh_Upper) == 24, "Chip audit ABI");
static_assert(__builtin_offsetof(MemHeader, mh_Free) == 28, "Chip audit ABI");
static_assert(__builtin_offsetof(MemChunk, mc_Bytes) == 4, "Chip audit ABI");

AmigaMemory::AmigaMemory()
    : game_(0), hud_(0), menu_(0), copper_(0), nullSprite_(0), bulletSprite_(0),
      sceneCache_(0), assets_(0), gameAssets_(0), gameAssetBytes_(0),
      gameBytes_(0), hudBytes_(0), menuPlaneBytes_(0), menuEntryBytes_(0),
      copperBytes_(0),
      playerSpriteBytes_(0), bulletSpriteBytes_(0), enemySpriteBytes_(0),
      sceneCacheBytes_(0),
      assetBytes_(0)
{
    playerSprite_[0] = 0;
    playerSprite_[1] = 0;
    enemySprite_[0] = 0;
    enemySprite_[1] = 0;
}

AmigaMemory::~AmigaMemory()
{
    release();
}

bool AmigaMemory::allocate(ULONG gameBytes, ULONG hudBytes,
                           ULONG menuPlaneBytes, ULONG menuEntryBytes,
                           ULONG copperBytes, ULONG playerSpriteBytes,
                           ULONG bulletSpriteBytes, ULONG enemySpriteBytes,
                           ULONG sceneCacheBytes,
                           ULONG assetBytes)
{
    release();
    gameBytes_ = gameBytes;
    hudBytes_ = hudBytes;
    menuPlaneBytes_ = menuPlaneBytes;
    menuEntryBytes_ = menuEntryBytes;
    copperBytes_ = copperBytes;
    playerSpriteBytes_ = playerSpriteBytes;
    bulletSpriteBytes_ = bulletSpriteBytes;
    enemySpriteBytes_ = enemySpriteBytes;
    sceneCacheBytes_ = sceneCacheBytes;
    assetBytes_ = assetBytes;
    game_ = (UBYTE*)AllocMem(gameBytes_, MEMF_CHIP | MEMF_CLEAR);
    hud_ = (UBYTE*)AllocMem(hudBytes_, MEMF_CHIP | MEMF_CLEAR);
    menu_ = (UBYTE*)AllocMem(menuPlaneBytes_ * 2 + menuEntryBytes_,
                             MEMF_CHIP | MEMF_CLEAR);
    copper_ = (UWORD*)AllocMem(copperBytes_, MEMF_CHIP | MEMF_CLEAR);
    nullSprite_ = (UWORD*)AllocMem(8, MEMF_CHIP | MEMF_CLEAR);
    playerSprite_[0] =
        (UWORD*)AllocMem(playerSpriteBytes_, MEMF_CHIP | MEMF_CLEAR);
    playerSprite_[1] =
        (UWORD*)AllocMem(playerSpriteBytes_, MEMF_CHIP | MEMF_CLEAR);
    bulletSprite_ =
        (UWORD*)AllocMem(bulletSpriteBytes_, MEMF_CHIP | MEMF_CLEAR);
    enemySprite_[0] =
        (UWORD*)AllocMem(enemySpriteBytes_, MEMF_CHIP | MEMF_CLEAR);
    enemySprite_[1] =
        (UWORD*)AllocMem(enemySpriteBytes_, MEMF_CHIP | MEMF_CLEAR);
    sceneCache_ = (UBYTE*)AllocMem(sceneCacheBytes_,
                                   MEMF_CHIP | MEMF_CLEAR);
    assets_ = assetBytes_ != 0
                  ? (UBYTE*)AllocMem(assetBytes_, MEMF_CHIP) : 0;
    if (game_ && hud_ && menu_ && copper_ && nullSprite_ &&
        playerSprite_[0] && playerSprite_[1] && bulletSprite_ &&
        enemySprite_[0] && enemySprite_[1] && sceneCache_ &&
        (assetBytes_ == 0 || assets_))
        return true;
    release();
    return false;
}

bool AmigaMemory::acquireGameAssets(ULONG bytes)
{
    if (!bytes) return false;
    if (gameAssets_) return gameAssetBytes_ == bytes;
    gameAssets_ = (UBYTE*)AllocMem(bytes, MEMF_CHIP);
    gameAssetBytes_ = gameAssets_ ? bytes : 0;
    return gameAssets_ != 0;
}

void AmigaMemory::releaseGameAssets()
{
    if (gameAssets_) FreeMem(gameAssets_, gameAssetBytes_);
    gameAssets_ = 0;
    gameAssetBytes_ = 0;
}

void AmigaMemory::release()
{
    releaseGameAssets();
    if (assets_) FreeMem(assets_, assetBytes_);
    if (sceneCache_) FreeMem(sceneCache_, sceneCacheBytes_);
    if (enemySprite_[1]) FreeMem(enemySprite_[1], enemySpriteBytes_);
    if (enemySprite_[0]) FreeMem(enemySprite_[0], enemySpriteBytes_);
    if (bulletSprite_) FreeMem(bulletSprite_, bulletSpriteBytes_);
    if (playerSprite_[1]) FreeMem(playerSprite_[1], playerSpriteBytes_);
    if (playerSprite_[0]) FreeMem(playerSprite_[0], playerSpriteBytes_);
    if (nullSprite_) FreeMem(nullSprite_, 8);
    if (copper_) FreeMem(copper_, copperBytes_);
    if (menu_) FreeMem(menu_, menuPlaneBytes_ * 2 + menuEntryBytes_);
    if (hud_) FreeMem(hud_, hudBytes_);
    if (game_) FreeMem(game_, gameBytes_);
    game_ = 0;
    hud_ = 0;
    menu_ = 0;
    copper_ = 0;
    nullSprite_ = 0;
    playerSprite_[0] = 0;
    playerSprite_[1] = 0;
    bulletSprite_ = 0;
    enemySprite_[0] = 0;
    enemySprite_[1] = 0;
    sceneCache_ = 0;
    assets_ = 0;
}
