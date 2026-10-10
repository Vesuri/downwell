#ifndef DOWNWELL_CORE_SHOP_FIXTURES_H
#define DOWNWELL_CORE_SHOP_FIXTURES_H
#include "ShopWorld.h"
namespace downwell {
// Static, depth-zero Shop Create fixtures. Emit original sprite IDs and world
// positions; backends own clipping and, on Amiga, cached planar composition.
// Merchant and active menu are separate drawing passes.
inline int shopItemSprite(unsigned upgrade) {
    constexpr int sprites[] = {152, 143, 151, 147, 150, 149};
    return upgrade >= 100 && upgrade <= 105 ? sprites[upgrade - 100] : -1;
}
template<class Emit>
void drawShopFixtures(const ShopWorld& world, Emit emit) {
    for (const auto& shop : world.instances) if (shop.allocated) {
        if (shop.present) emit(480, 0, shop.x, shop.y);
        if(!shop.children.table)emit(481, 0, shop.x + 64, shop.y);
        for (int i = 0; i < 3; ++i) if(!shop.children.cases[i])emit(482, 0, shop.x + 45 + i * 19, shop.y - 9);
        for (int i = 0; i < 3; ++i) if (shop.feedback.itemPresent[i]&&!shop.children.items[i]) {
            const int sprite = shopItemSprite(shop.stock.items[i].upgrade);
            // objObtainableUg Create selects frame 1 and image_speed=0;
            // Step changes only sprite_index. Shop places items at case.y+1.
            if (sprite >= 0) emit(sprite, 1, shop.x + 45 + i * 19, shop.y - 8);
        }
    }
}
// Moving-object pass: never bake merchant animation into the terrain cache.
template<class Emit>
void drawShopPrompts(const ShopWorld& world, bool criminal, Emit emit) {
    if (criminal) return;
    for (const auto& shop : world.instances)
        if (shop.present && !shop.menu.speaking)
            emit(707, shop.prompt.frame(), shop.x, shop.y - 20);
}
template<class Emit>
void drawShopKeepers(const ShopWorld& world, Emit emit) {
    for (const auto& shop : world.instances)
        if (shop.allocated && shop.keeper.present && !shop.children.keeper)
            emit(shop.keeper.sprite, shop.keeper.frame(), shop.x + 18, shop.y);
}
}
#endif
