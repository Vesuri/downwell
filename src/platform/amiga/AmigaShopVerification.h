#ifndef DOWNWELL_AMIGA_SHOP_VERIFICATION_H
#define DOWNWELL_AMIGA_SHOP_VERIFICATION_H
#include "core/GameplayWorld.h"

// Native counterpart of the paired shop interaction in AmigaSceneRuntimeTest.
// Fixture-only creation/free battery; input, purchase and grant timing stay live.
struct AmigaShopVerification {
    downwell::ShopWorld::Instance* shop = nullptr;
    unsigned frame = 0, coverage = 0, failures = 0;
    int capacity = 0;
    downwell::Input input(GameplayWorld& world) {
        downwell::Input result;
        if (!shop) {
            if (!world.player().grounded() || world.player().speedXFixed()) return result;
            shop = world.registerShop(252 + world.player().offsetX(),
                                      176 + world.player().offsetY());
            if (!shop) { ++failures; return result; }
            capacity = world.player().ammoCapacity();
        }
        ++frame;
        if (frame == 6) shop->stock.items[0] = {101, 1, 0};
        result.fire = frame == 2 || frame == 3 || frame == 5 || frame == 7 ||
                      frame == 9 || frame == 11 || frame == 13;
        result.left = frame == 8 || frame == 12;
        return result;
    }
    void check(const GameplayWorld& world) {
        if (!shop) return;
        bool ok = true;
        unsigned bit = 0;
        switch (frame) {
        case 2: bit = 1; ok = shop->menu.active && world.shops().noControl; break;
        case 3: bit = 2; ok = shop->stock.items[0].remaining == 1; break;
        case 5: bit = 4; ok = shop->stock.items[0].remaining == 1 && !world.currency(); break;
        case 7: bit = 8; ok = !shop->stock.items[0].remaining &&
            world.consumables().pending[1] == 1 && world.player().ammoCapacity() == capacity; break;
        case 8: bit = 16; ok = world.player().ammoCapacity() == capacity + 1 &&
            !world.consumables().pending[1] && shop->menu.cursor == 0; break;
        case 9: bit = 32; ok = !shop->menu.active && !world.shops().noControl; break;
        case 11: bit = 64; ok = shop->menu.active && !shop->stock.items[0].remaining; break;
        case 13: bit = 128; ok = !shop->menu.active && !world.shops().noControl; break;
        }
        if (!ok) ++failures;
        else coverage |= bit;
    }
};
#endif
