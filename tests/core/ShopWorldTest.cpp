#include "core/RunSession.h"
#include "core/PlayerMotion.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::abort(); }
}
int main() {
    downwell::RunSession run;
    downwell::CurrencyAccount money;
    money.currency = 1000;
    unsigned draws = 0;
    auto range = [&](unsigned lo, unsigned) { ++draws; return lo + (draws % 4 == 2 ? 1 : 0); };
    auto* shop = run.shops.create(5, 0, range, 4, 1);
    require(shop && draws == 4, "registration consumes stock RNG once");
    PlayerMotion player;
    unsigned greetings = 0;
    auto tick = [&](bool fire, bool left = false, bool right = false) {
        downwell::Input input; input.fire = fire; input.left = left; input.right = right;
        player.step(input, run.shops.noControl, false, run.shops.interactable);
        run.shops.observePlayer(player.offsetX(), player.offsetY(),
            player.grounded(), player.speedXFixed());
        int facing = player.facing();
        run.shops.step(input, player.offsetX(), player.offsetY(), false, false,
            facing, money, [&] { ++greetings; return 1u; },
            [&](unsigned id) { run.consumables.grant(id, money); },
            [](auto&, auto) {});
    };
    tick(false); // Publish eligibility for the next player Begin Step.
    require(run.shops.interactable, "stationary grounded mask overlap is eligible");
    tick(true);
    require(shop->menu.active && run.shops.noControl && player.grounded() &&
        money.currency == 1000 && greetings == 1, "opening consumes press without jumping or buying");
    tick(true);
    require(money.currency == 1000, "held confirmation does not purchase");
    tick(false); tick(true);
    require(money.currency == 700 && run.consumables.pending[0] == 1 &&
        shop->stock.items[0].remaining == 0, "fresh confirmation purchases into run queue");
    tick(false, true); // First item -> Leave; locked player stays still.
    require(shop->menu.cursor == 0 && player.offsetX() == 0, "menu navigation does not move player");
    tick(true);
    require(!run.shops.noControl && !shop->menu.active, "Leave unlocks world controls");
    tick(false); tick(true);
    require(shop->menu.active && shop->stock.items[0].remaining == 0, "reentry retains sold stock");
    using World = downwell::ShopWorld;
    require(World::meets(*shop, -8, -8) && !World::meets(*shop, -9, -8) &&
        World::meets(*shop, 8, 12) && !World::meets(*shop, 9, 12), "inclusive source mask edges");
    run.shops.observePlayer(0, 0, true, 1);
    require(!run.shops.interactable, "horizontal movement disallows interaction");
    for (unsigned i = 1; i < World::capacity; ++i)
        require(run.shops.create(100, 0, range, 4, 1), "bounded registration succeeds");
    const unsigned before = draws;
    require(!run.shops.create(100, 0, range, 4, 1) && draws == before,
        "exhaustion is explicit and consumes no RNG");
    run.shops.endStep(320);
    require(shop->present, "retirement boundary is strict");
    run.shops.endStep(321);
    require(!shop->present, "offscreen shop retires at source threshold");
    run.reset();
    require(!run.shops.noControl && !run.shops.interactable &&
        !run.shops.previous.fire && !run.shops.instances[0].present &&
        run.consumables.pending[0] == 0, "new run clears shops, edges, locks and deferred grants");
    std::puts("Shop world ownership, player input ordering and purchase lifecycle passed");
}
