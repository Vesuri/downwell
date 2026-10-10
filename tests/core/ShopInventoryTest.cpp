#include "core/ShopInventory.h"
#include "core/GameMakerRounding.h"
#include "ShopPurchaseChecks.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::abort(); }
}
int main() {
    require(checkShopPurchasePrefixes(),"registered purchase preserves each accepted prefix on failure");
    using Shop = downwell::ShopInventory;
    const unsigned bases[] = {300,150,500,250,400,1000};
    for (unsigned area = 1; area <= 5; ++area)
        for (unsigned id = 100; id <= 105; ++id)
            for (unsigned flags = 0; flags < 8; ++flags) {
                const bool hard = flags & 1, discount = flags & 2, member = flags & 4;
                double expected = bases[id - 100] + 200 * (area - 1);
                if (hard) expected *= 2;
                if (discount) expected -= downwell::roundGameMakerCoordinate(expected / 10) * 3;
                if (member) expected -= downwell::roundGameMakerCoordinate(expected / 10);
                require(Shop::price(id, area, hard, discount ? 4 : 0, member) == expected,
                        "shop prices follow original sequential discounts");
            }
    require(Shop::price(101, 1, false, 4, true) == 95, "105/10 ties down to even ten");
    require(Shop::price(103, 1, false, 4, true) == 157, "175/10 ties up to even eighteen");
    Shop shop;
    const unsigned draws[] = {101,100,100,102,105,102,104};
    unsigned draw = 0;
    shop.create([&](unsigned low, unsigned high) {
        require(draw < 7, "stock consumes exactly the source draws");
        require(low == (draw < 4 ? 100u : 102u) && high == (draw == 0 ? 101u : draw < 4 ? 103u : 105u),
                "stock random range order");
        return draws[draw++];
    }, 2);
    require(draw == 7 && shop.items[0].upgrade == 100 && shop.items[1].upgrade == 102 &&
            shop.items[2].upgrade == 104, "low health override, rejection and discarded third-slot draw");
    downwell::CurrencyAccount account;
    account.currency = 299; account.gameGem = 999; account.streak = 88; account.timer = 42;
    unsigned grants = 0;
    auto grant = [&](unsigned id) {
        ++grants;
        require(id == 100 && shop.items[0].remaining == 0 && account.currency == 300,
                "grant observes stock decremented but payment not deducted");
        account.currency += 100; // scrGainUg's already-at-cap refund path
    };
    require(shop.purchase(1, account, grant) == Shop::Purchase::insufficientFunds &&
            grants == 0 && account.currency == 299 && shop.items[0].remaining == 1,
            "failed affordability has no mutation");
    account.currency = 300;
    require(shop.purchase(1, account, grant) == Shop::Purchase::bought && grants == 1 && account.currency == 100,
            "exact-price purchase grants before subtracting the full price");
    require(shop.purchase(1, account, grant) == Shop::Purchase::soldOut && grants == 1,
            "sold-out takes precedence over insufficient currency");
    require(shop.purchase(0, account, grant) == Shop::Purchase::notAnItem &&
            shop.purchase(4, account, grant) == Shop::Purchase::notAnItem && grants == 1,
            "Leave and invalid cursors cannot purchase");
    require(account.gameGem == 999 && account.streak == 88 && account.timer == 42,
            "spending changes neither run gem total nor streak timers");
    unsigned calls = 0;
    shop.create([&](unsigned low, unsigned) { ++calls; return low + (low == 100 && calls == 1); }, 4);
    require(calls == 4 && shop.items[0].upgrade == 101 && shop.items[0].remaining == 1,
            "healthy stock and recreation restore sold inventory and keep discarded draw");
    std::puts("Shop stock, source RNG ordering, prices and purchase transactions passed");
}
