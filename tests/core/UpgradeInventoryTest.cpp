#include "core/UpgradeInventory.h"
#include "core/LevelUpgradeOffers.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
int main() {
    downwell::UpgradeInventory inventory;
    downwell::Consumables consumables;
    downwell::CurrencyAccount money;
    downwell::HealthReserve reserve;
    PlayerMotion player;
    int hp = 1;
    auto grant = [&](unsigned id) { return inventory.grant(id, consumables, money, player, hp, reserve); };
    require(grant(6) && hp == 2 && inventory.levels[6] == 1 && inventory.order[1] == 6,
        "Youth increments ownership and heals immediately");
    require(grant(6) && hp == 3 && money.currency == 100 && inventory.count == 1,
        "duplicate refund does not skip source immediate healing or append ownership");
    require(grant(13) && hp == 4 && reserve.pieces == 3 && inventory.effects.healing == 4,
        "Apple routes overflow healing through the shared reserve");
    require(grant(13) && hp == 5 && reserve.maximum == 5 && reserve.pieces == 3 &&
        inventory.effects.maximumHealthMessages == 1 && money.currency == 200,
        "duplicate Apple still heals and can increase maximum health");
    const int ammo = player.ammoCapacity();
    require(grant(19) && player.ammoCapacity() == ammo + 4 && player.ammo() == ammo + 4 &&
        money.currency == 300 && !inventory.eligible(19) && inventory.levels[19] == 0,
        "cap-zero Super Charge refunds then adds/refills charge without ownership");
    money.streak = 7; money.timer = 23;
    require(grant(21) && money.currency == 800 && money.gameGem == 400 &&
        money.streak == 7 && money.timer == 23 && inventory.effects.fortuneSound,
        "cap-zero Fortune refunds plus direct gems without pickup streak effects");
    const int beforeHp = hp;
    require(grant(100) && consumables.pending[0] == 1 && hp == beforeHp &&
        inventory.count == 2 && !inventory.effects.fortuneSound,
        "consumable fallback remains deferred and is absent from normal upgrade order");
    require(!grant(99) && !grant(106), "unknown resource IDs do not mutate inventory");
    for (unsigned id = 0; id <= inventory.highestId; ++id) {
        if (inventory.eligible(id)) require(grant(id), "normal upgrade grant");
    }
    require(inventory.count == 20 && !inventory.eligible(2) && inventory.levels[2] == 0,
        "all twenty enabled upgrades retain IDs; Dumbbell stays disabled with the other cap-zero entries");
    downwell::GameMakerRandom random(5);
    downwell::LevelUpgradeOffers offers;
    require(offers.randomize(2, inventory.highestId, random,
        [&](unsigned id) { return inventory.eligible(id); }) &&
        offers.ids[0] == 100 && offers.ids[1] == 100 && offers.ids[2] == 100 &&
        random.drawCount() == 3 * 61 * 2,
        "fully owned inventory drives the original exhausted-pool fallback");
    std::puts("Upgrade caps, acquisition order, immediate grants and fallback tests passed");
}
