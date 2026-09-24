#include "core/Consumables.h"
#include "core/ShopInventory.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
int main() {
    downwell::Consumables items;
    downwell::CurrencyAccount account;
    PlayerMotion player(false);
    downwell::HealthReserve reserve;
    int health = 4;
    for (unsigned id = 100; id <= 105; ++id) require(items.grant(id, account), "valid consumable");
    items.grant(100, account);
    require(health == 4 && player.ammoCapacity() == 8, "grant queues instead of applying at purchase");
    items.beginStep(player, health, reserve);
    require(health == 6 && reserve.maximum == 6 && reserve.pieces == 0 &&
            player.ammoCapacity() == 12 && player.ammo() == 12,
            "source ID order preserves overflow healing then Curry maximum increase");
    require(items.pending[0] == 1 && items.effects.count == 6 &&
            items.effects.healingSounds == 4 && items.effects.maximumHealthMessages == 1,
            "one unit per type and source presentation requests");
    const unsigned types[] = {1,0,1,0,2,1}, amounts[] = {1,1,2,2,1,1};
    for (unsigned i = 0; i < 6; ++i)
        require(items.effects.notifications[i].type == types[i] &&
                items.effects.notifications[i].amount == amounts[i], "notification order/type/amount");
    items.beginStep(player, health, reserve);
    require(reserve.pieces == 1 && items.pending[0] == 0 && items.effects.count == 1,
            "second stacked item waits for the next controller update");
    items.beginStep(player, health, reserve);
    require(items.effects.count == 0 && items.effects.healingSounds == 0, "empty update clears transient effects");
    items = {}; reserve = {}; health = 1;
    items.grant(105, account); items.beginStep(player, health, reserve);
    require(reserve.maximum == 5 && health == 2 && reserve.pieces == 0,
            "Curry heals only one: it does not fill an injured player's new maximum");
    items = {};
    account.currency = 150; account.gameGem = 55; account.streak = 12;
    for (unsigned i = 0; i < 100; ++i) items.grant(101, account);
    downwell::ShopInventory shop;
    shop.items[0] = {101,1,150};
    require(shop.purchase(1, account, [&](unsigned id) { items.grant(id, account); }) ==
            downwell::ShopInventory::Purchase::bought && account.currency == 100 && items.pending[1] == 100,
            "real capped grant refunds before the shop subtracts price");
    require(account.gameGem == 55 && account.streak == 12, "cap refund is not collected gems");
    require(!items.grant(99, account) && !items.grant(106, account), "other upgrades need their own handlers");
    std::puts("Deferred consumable effects and shop grant ordering passed");
}
