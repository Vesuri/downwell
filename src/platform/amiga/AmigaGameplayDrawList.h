#ifndef DOWNWELL_AMIGA_GAMEPLAY_DRAW_LIST_H
#define DOWNWELL_AMIGA_GAMEPLAY_DRAW_LIST_H
#include "AmigaGameplayActors.h"
#include "core/GameplayWorld.h"
#include "core/EntryDitherPlacement.h"
#include "core/EffectMaskAssets.h"
class AmigaGameplayDrawList {
public:
    static constexpr unsigned capacity = 4 + 32 + 32 + 8 + GameplayWorld::bulletCapacity +
        GameplayWorld::enemyCapacity + GameplayWorld::gemCapacity + GameplayWorld::boneCapacity + downwell::scriptSpawnEffectCapacity +
        2 * downwell::ShopWorld::capacity;
    static constexpr int playerStartY = 142, replayGroundY = 200;
    GameplayBob actors[capacity]{};
    unsigned count = 0;
    UWORD failedSprite = 0, failedFrame = 0;
    downwell::LaserSightLine rayLine{};
    bool rayVisible = false;
    bool entryCoverVisible = false;
    downwell::EntryDitherPlacement entryCover{};
    // Read-only projection of shared simulation into original painter order.
    // No simulation stepping, asset conversion or framebuffer writes.
    bool collect(const GameplayWorld& world, const UBYTE* pack,
        const UBYTE* packEnd, int worldViewX, int worldViewY, bool compositeCurrency = false,
        const AmigaActorFrames* prepared = nullptr);
    bool entryCoverHardwareEligible(const GameplayWorld& world,int viewX,int viewY,bool& eligible)const;
    // Currency Draw walks slots newest to oldest. Promote only its topmost
    // contiguous suffix when shade requires a retained raster. A completely
    // unshaded pass uses the actor compositor, including BOB channel overflow.
    // Ordinary actors remain below both partitions. currencySpriteCount is
    // the admitted controller suffix length before sprite/BOB assignment.
    unsigned currencyRasterFirst=0,currencySpriteCount=0;
    bool currencyRasterRequired=true;
    bool promoteCurrency(const GameplayWorld& world,int viewX,int viewY,
        const downwell::EffectMaskAssets& assets,const AmigaActorFrames& frames);
    // Retained all-or-nothing reference for projection tests.
    bool promoteCurrencyComplete(const GameplayWorld& world,int viewX,int viewY,
        const downwell::EffectMaskAssets& assets,const AmigaActorFrames& frames);
private:
    // Admission workspace, never published until the source-ordered pass
    // succeeds. Keep the full family off the small AmigaDOS stack. This also
    // avoids the m68k toolchain's incorrect word-member stack displacement
    // for a large automatic GameplayBob array's aggregate assignments.
    GameplayBob currencyPending_[GameplayWorld::gemCapacity]{};
    unsigned currencySlots_[GameplayWorld::gemCapacity]{};
};
#endif
