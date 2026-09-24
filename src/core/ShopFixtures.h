#ifndef DOWNWELL_CORE_SHOP_FIXTURES_H
#define DOWNWELL_CORE_SHOP_FIXTURES_H
#include "ShopWorld.h"
namespace downwell {
// Static, depth-zero Shop Create fixtures. Emit original sprite IDs and world
// positions; backends own clipping and, on Amiga, cached planar composition.
// Merchant, item children and the active menu are separate drawing passes.
template<class Emit>
void drawShopFixtures(const ShopWorld& world, Emit emit) {
    for (const auto& shop : world.instances) if (shop.allocated) {
        if (shop.present) emit(480, 0, shop.x, shop.y);
        emit(481, 0, shop.x + 64, shop.y);
        for (int i = 0; i < 3; ++i) emit(482, 0, shop.x + 45 + i * 19, shop.y - 9);
    }
}
}
#endif
