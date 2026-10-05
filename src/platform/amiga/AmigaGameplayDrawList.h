#ifndef DOWNWELL_AMIGA_GAMEPLAY_DRAW_LIST_H
#define DOWNWELL_AMIGA_GAMEPLAY_DRAW_LIST_H
#include "AmigaGameplayActors.h"
#include "core/GameplayWorld.h"
#include "core/EntryDitherPlacement.h"
class AmigaGameplayDrawList {
public:
    static constexpr unsigned capacity = 2 + 32 + 32 + 8 + GameplayWorld::bulletCapacity +
        GameplayWorld::enemyCapacity + GameplayWorld::gemCapacity + downwell::scriptSpawnEffectCapacity +
        2 * downwell::ShopWorld::capacity;
    static constexpr int playerStartY = 142, replayGroundY = 200;
    GameplayBob actors[capacity];
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
};
#endif
